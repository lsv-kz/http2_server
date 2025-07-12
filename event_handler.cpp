#include "main.h"

using namespace std;
//======================================================================
int create_multipart_head(Connect *req);
void cgi_worker(Connect *con, Stream *resp, struct pollfd*);

static EventHandlerClass event_handler_cl;
//======================================================================
EventHandlerClass::~EventHandlerClass()
{
    if (conn_array)
        delete [] conn_array;
    if (poll_fd)
        delete [] poll_fd;
}
//----------------------------------------------------------------------
EventHandlerClass::EventHandlerClass()
{
    num_request = 0;
    close_thr = num_wait = num_work = all_cgi = 0;
    work_list_start = work_list_end = wait_list_start = wait_list_end = NULL;
}
//----------------------------------------------------------------------
void EventHandlerClass::init(int n)
{
    conn_array = new(nothrow) Connect* [conf->MaxAcceptConnections];
    if (!conn_array)
    {
        print_err("<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
        exit(1);
    }

    poll_fd = new(nothrow) struct pollfd [conf->MaxAcceptConnections];
    if (!poll_fd)
    {
        print_err("<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
        exit(1);
    }
}
//----------------------------------------------------------------------
void EventHandlerClass::del_from_list(Connect *r)
{
    if (r->prev && r->next)
    {
        r->prev->next = r->next;
        r->next->prev = r->prev;
    }
    else if (r->prev && !r->next)
    {
        r->prev->next = r->next;
        work_list_end = r->prev;
    }
    else if (!r->prev && r->next)
    {
        r->next->prev = r->prev;
        work_list_start = r->next;
    }
    else if (!r->prev && !r->next)
        work_list_start = work_list_end = NULL;
}
//----------------------------------------------------------------------
void EventHandlerClass::close_stream(Connect *con, int id)
{
    con->h2.close_stream(&con->h2, id, &all_cgi);
}
//----------------------------------------------------------------------
void EventHandlerClass::create_message(Stream *r, int status)
{
    switch (status)
    {
        case RS200:
            resp_200(r);
            break;
        case RS204:
            resp_204(r);
            break;
        case RS400:
            resp_400(r);
            break;
        case RS403:
            resp_403(r);
            break;
        case RS404:
            resp_404(r);
            break;
        case RS414:
            resp_414(r);
            break;
        case RS500:
            resp_500(r);
            break;
        case RS502:
            resp_502(r);
            break;
        case RS504:
            resp_504(r);
            break;
        default:
            print_err("<%s:%d> Error status=%d, id=%d \n", __func__, __LINE__, status, r->id);
            resp_500(r);
            break;
    }
}
//----------------------------------------------------------------------
void EventHandlerClass::add_work_list()
{
mtx_thr.lock();
    if (wait_list_start)
    {
        if (work_list_end)
            work_list_end->next = wait_list_start;
        else
            work_list_start = wait_list_start;

        wait_list_start->prev = work_list_end;
        work_list_end = wait_list_end;
        wait_list_start = wait_list_end = NULL;
    }
mtx_thr.unlock();
}
//======================================================================
int EventHandlerClass::cgi_set_poll()
{
    int cgi_ind_array = 0;
    num_cgi_poll = 0;
    time_t t = time(NULL);
    Connect *c = work_list_start, *next = NULL;
    if (c == NULL)
        return 0;

    for ( ; c; c = next)
    {
        next = c->next;
        Stream *resp_next = NULL, *resp = c->h2.get();
        if (!resp)
        {
            continue;
        }
        //--------------------------------------------------------------
        c->h2.num_cgi = 0;
        for ( ; resp; resp = resp_next)
        {
            resp_next = resp->next;
            if (resp->content != DYN_PAGE)
            {
                continue;
            }

            if ((resp->cgi.timer == 0) || (resp->cgi.start == false))
                resp->cgi.timer = t;
            if ((t - resp->cgi.timer) >= conf->TimeoutCGI)
            {
                print_err(resp, "<%s:%d> TimeoutCGI=%ld, post_data.size()=%d, cgi.op=%d, id=%d \n", __func__, __LINE__, 
                        t - resp->cgi.timer, resp->post_data.size(), resp->cgi.op, resp->id);
                c->h2.frame_win_update.init();
                resp->frame_win_update.init();
                create_message(resp, RS504);
            }
            else
            {
                if ((resp->cgi.op == CGI_CREATE) && (all_cgi < conf->MaxCgiProc))
                {
                    if ((resp->cgi.cgi_type == CGI) || (resp->cgi.cgi_type == PHPCGI))
                    {
                        int ret = cgi_create_proc(resp);
                        if (ret < 0)
                        {
                            print_err(c, "<%s:%d> Error cgi_create_proc()\n", __func__, __LINE__);
                            if (ret == -1)
                                create_message(resp, RS500);
                            continue;
                        }

                        if (resp->method == "POST")
                        {
                            if(resp->post_cont_length <= 0)
                            {
                                resp->cgi.op = CGI_STDOUT;
                                close(resp->cgi.to_script);
                                resp->cgi.to_script = -1;
                            }
                            else
                            {
                                resp->cgi.op = CGI_STDIN;
                            }
                        }
                        else
                        {
                            resp->cgi.op = CGI_STDOUT;
                        }
                    }
                    else if ((resp->cgi.cgi_type == PHPFPM) || (resp->cgi.cgi_type == FASTCGI))
                    {
                        int ret = fcgi_create_connect(resp);
                        if (ret < 0)
                        {
                            create_message(resp, ret);
                            continue;
                        }
                        else
                        {
                            resp->cgi.fcgi_op = FASTCGI_BEGIN;
                            resp->cgi.op = CGI_STDIN;
                        }
                    }
                    else if (resp->cgi.cgi_type == SCGI)
                    {
                        int ret = scgi_create_connect(c, resp);
                        if (ret < 0)
                        {
                            create_message(resp, RS500);
                            continue;
                        }
                        else
                        {
                            resp->cgi.scgi_op = SCGI_PARAMS;
                            resp->cgi.op = CGI_STDIN;
                        }
                    }

                    resp->cgi.start = true;
                    ++all_cgi;
                }

                if (resp->cgi.op == CGI_STDIN)
                {
                    if (resp->cgi.cgi_type <= PHPCGI)
                    {
                        if (resp->post_data.size())
                        {
                            poll_fd[num_cgi_poll].fd = resp->cgi.to_script;
                            poll_fd[num_cgi_poll++].events = POLLOUT;
                            c->h2.num_cgi++;
                        }
                    }
                    else if (resp->cgi.cgi_type == SCGI)
                    {
                        if (resp->cgi.scgi_op == SCGI_PARAMS)
                        {
                            poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                            poll_fd[num_cgi_poll++].events = POLLOUT;
                            c->h2.num_cgi++;
                        }
                        else if (resp->cgi.scgi_op == SCGI_STDIN)
                        {
                            if (resp->post_data.size())
                            {
                                poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                                poll_fd[num_cgi_poll++].events = POLLOUT;
                                c->h2.num_cgi++;
                            }
                        }
                    }
                    else if ((resp->cgi.cgi_type == PHPFPM) || (resp->cgi.cgi_type == FASTCGI))
                    {
                        if (resp->cgi.buf_param.size() ||
                           (resp->cgi.fcgi_op == FASTCGI_PARAMS) ||
                           resp->post_data.size()
                        )
                        {
                            poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                            poll_fd[num_cgi_poll++].events = POLLOUT;
                            c->h2.num_cgi++;
                        }
                    }
                }
                else if (resp->cgi.op == CGI_STDOUT)
                {
                    if (resp->cgi.cgi_type <= PHPCGI)
                    {
                        if (resp->data.size() == 0)
                        {
                            poll_fd[num_cgi_poll].fd = resp->cgi.from_script;
                            poll_fd[num_cgi_poll++].events = POLLIN;
                            c->h2.num_cgi++;
                        }
                    }
                    else if (resp->cgi.cgi_type == SCGI)
                    {
                        if (resp->data.size() == 0)
                        {
                            poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                            poll_fd[num_cgi_poll++].events = POLLIN;
                            c->h2.num_cgi++;
                        }
                    }
                    else if ((resp->cgi.cgi_type == PHPFPM) || (resp->cgi.cgi_type == FASTCGI))
                    {
                        if (resp->data.size() == 0)
                        {
                            poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                            poll_fd[num_cgi_poll++].events = POLLIN;
                            c->h2.num_cgi++;
                        }
                    }
                }
            }
        }
        if (c->h2.num_cgi)
        {
            conn_array[cgi_ind_array++] = c;
        }
    }
    //------------------------------------------------------------------
    if (num_cgi_poll == 0)
        return 0;
    int ret_poll = poll(poll_fd, num_cgi_poll, conf->TimeoutPoll);
    if (ret_poll == -1)
    {
        print_err("<%s:%d> Error poll(): %s\n", __func__, __LINE__, strerror(errno));
        return -1;
    }
    else if (ret_poll == 0)
    {
        return 0;
    }
    //-------------------------- loop Connects -------------------------
    for ( int conn_ind = 0, poll_ind = 0; (conn_ind < cgi_ind_array); ++conn_ind)
    {
        c = conn_array[conn_ind];
        Stream *resp_next = NULL, *resp = c->h2.get();
        if (!resp)
        {
            continue;
        }
        //------------------------ loop Responses -----------------------
        for ( int n = 0; resp; resp = resp_next)
        {
            resp_next = resp->next;
            if (resp->content != DYN_PAGE)
            {
                continue;
            }

            if (n >= c->h2.num_cgi)
                break;
            if (poll_ind >= num_cgi_poll)
            {
                print_err(c, "<%s:%d> !!! n=%d, poll_ind(%d)>=num_cgi_poll(%d), c->h2.num_cgi=%d,  id=%d \n", __func__, __LINE__, n, poll_ind, num_cgi_poll, c->h2.num_cgi, resp->id);
                exit(11);
            }

            int fd = poll_fd[poll_ind].fd;
            int revents = poll_fd[poll_ind].revents;
            if ((fd != resp->cgi.to_script) && (fd != resp->cgi.from_script) && (fd != resp->cgi.fd))
            {
                if (conf->PrintDebugMsg)
                    print_err("<%s:%d> revents=0x%02X, fd=%d (fd != cgi.fd),  id=%d \n", __func__, __LINE__, revents, fd, resp->id);
                continue;
            }

            n++;

            if (revents)
            {
                if (resp->cgi.cgi_type <= PHPCGI)
                {
                    if (resp->cgi.op == CGI_STDIN)
                    {
                        cgi_worker(c, resp, &poll_fd[poll_ind]);
                    }
                    else if (resp->cgi.op == CGI_STDOUT)
                    {
                        if (fd != resp->cgi.from_script)
                        {
                            print_err(c, "<%s:%d> ??? fd=%d/%d,  id=%d \n", __func__, __LINE__, 
                                                fd, resp->cgi.from_script, resp->id);
                            create_message(resp, RS500);
                        }
                        else
                            cgi_worker(c, resp, &poll_fd[poll_ind]);
                    }
                }
                else if (resp->cgi.cgi_type <= FASTCGI)
                {
                    fcgi_worker(c, resp, &poll_fd[poll_ind]);
                }
                else if (resp->cgi.cgi_type == SCGI)
                {
                    if (resp->cgi.scgi_op == SCGI_PARAMS)
                        scgi_worker(c, resp, &poll_fd[poll_ind]);
                    else
                        cgi_worker(c, resp, &poll_fd[poll_ind]);
                }
                else
                {
                    print_err("<%s:%d> Error cgi_type=%d, id=%d \n", __func__, __LINE__, resp->cgi.cgi_type, resp->id);
                }
            }
            else
            {
                if (conf->PrintDebugMsg)
                    print_err("<%s:%d> revents=0, id=%d \n", __func__, __LINE__, resp->id);
            }

            poll_ind++;
        }
    }

    return 0;
}
//======================================================================
int EventHandlerClass::set_poll()
{
    num_wait = 0;
    time_t t = time(NULL);
    int Timeout = 0;
    Connect *c = work_list_start, *next = NULL;
    for ( ; c; c = next)
    {
        next = c->next;

        if (c->sock_timer == 0)
            c->sock_timer = t;

        if ((c->h2.type_op == PREFACE_MESSAGE) ||
                (c->h2.type_op == SSL_ACCEPT) ||
                (c->h2.type_op == SSL_SHUTDOWN) ||
                (c->h2.type_op == RECV_SETTINGS) ||
                (c->h2.type_op == SEND_SETTINGS)
        )
        {
            Timeout = conf->Timeout;
        }
        else
            Timeout = conf->TimeoutKeepAlive;

        if ((t - c->sock_timer) >= Timeout)
        {
            print_err(c, "<%s:%d> Timeout=%ld, %s\n", __func__, __LINE__, t - c->sock_timer, get_str_operation(c->h2.type_op));
            if ((c->h2.type_op == SSL_ACCEPT) || 
                (c->h2.type_op == SSL_SHUTDOWN)
            )
                close_connect(c);
            else
                ssl_shutdown(c);
        }
        else
        {
            int ret = 0;
            while ((c->ssl_pending = SSL_pending(c->tls.ssl)) && (c->h2.type_op != SEND_SETTINGS))
            {
                if (conf->PrintDebugMsg)
                {
                    print_err(c, "<%s:%d> ***** SSL_pending()=%d, %s\n", __func__, __LINE__, 
                                c->ssl_pending, get_str_operation(c->h2.type_op));
                }

                if ((c->h2.type_op == RECV_SETTINGS) || (c->h2.type_op == WORK_STREAM))
                {
                    if ((ret = recv_frame(c)) < 0)
                        break;
                }
                else if ((c->h2.type_op == SSL_ACCEPT) || (c->h2.type_op == PREFACE_MESSAGE) || (c->h2.type_op == SSL_SHUTDOWN))
                {
                    if ((ret = http2_connection(c)) < 0)
                        break;
                }
            }

            if (ret < 0)
                continue;
            if (c->h2.next == NULL)
                c->h2.next = c->h2.start_stream;

            poll_fd[num_wait].fd = c->clientSocket;
            poll_fd[num_wait].events = 0;
            if ((c->h2.type_op == SSL_ACCEPT) || (c->h2.type_op == SSL_SHUTDOWN))
                poll_fd[num_wait].events = c->tls.poll_event;
            else if ((c->h2.type_op == PREFACE_MESSAGE) || (c->h2.type_op == RECV_SETTINGS))
                poll_fd[num_wait].events = POLLIN;
            else if (c->h2.type_op == SEND_SETTINGS)
                poll_fd[num_wait].events = POLLOUT;
            else // h2.type_op = WORK_STREAM
            {
                poll_fd[num_wait].events = POLLIN;
                if (c->h2.connect_window_size <= 0)
                {
                    if (conf->PrintDebugMsg)
                        print_err(c, "<%s:%d> connect_window_size <= 0\n", __func__, __LINE__);
                    conn_array[num_wait++] = c;
                    continue;
                }

                if (c->h2.frame_win_update.size() || c->h2.goaway.size() || c->h2.ping.size() || c->h2.start_frame)
                {
                    poll_fd[num_wait].events |= POLLOUT;
                    conn_array[num_wait++] = c;
                    continue;
                }

                Stream *resp_next = NULL, *resp = c->h2.get();
                for ( int i = 0; resp; resp = resp_next, i++)
                {
                    resp_next = resp->next;
                    if (resp->frame_win_update.size() || 
                        resp->headers.size() || 
                        resp->data.size() || 
                        resp->stream_window_size || 
                        resp->rst_stream
                    )
                    {
                        poll_fd[num_wait].events |= POLLOUT;
                        break;
                    }
                }
            }

            if (poll_fd[num_wait].events)
                conn_array[num_wait++] = c;
        }
    }

    return 0;
}
//----------------------------------------------------------------------
int EventHandlerClass::poll_worker()
{
    int ret_poll = 0;
    if (num_wait > 0)
    {
        int time_poll = conf->TimeoutPoll;
        if (all_cgi > 0)
            time_poll = 0;
        ret_poll = poll(poll_fd, num_wait, time_poll);
        if (ret_poll == -1)
        {
            print_err("<%s:%d> Error poll(): %s\n", __func__, __LINE__, strerror(errno));
            return -1;
        }
        else if (ret_poll == 0)
        {
            return 0;
        }
    }
    else
    {
        //print_err("<%s:%d> Error num_wait()=0\n", __func__, __LINE__);
        return 0;
    }

    for ( int conn_ind = 0; conn_ind < num_wait; ++conn_ind)
    {
        Connect *con = conn_array[conn_ind];
        if (poll_fd[conn_ind].revents == 0)
            continue;

        int revents = poll_fd[conn_ind].revents;
        if (revents & ((~POLLIN) & (~POLLOUT)))
        {
            print_err(con, "<%s:%d> !!! Error: revents=0x%02x, type_op=%d\n", __func__, __LINE__, revents, con->h2.type_op);
            if ((con->h2.type_op == SSL_ACCEPT) || 
                (con->h2.type_op == SSL_SHUTDOWN))
            {
                close_connect(con);
            }
            else
                ssl_shutdown(con);
            continue;
        }

        if (poll_fd[conn_ind].fd != con->clientSocket)
        {
            print_err(con, "<%s:%d> !!! Error: fd=%d/clientSocket=%d\n", __func__, __LINE__, poll_fd[conn_ind].fd, con->clientSocket);
            ssl_shutdown(con);
            continue;
        }

        con->fd_revents = revents;
        if (poll_fd[conn_ind].revents & POLLIN)
        {
            if ((con->h2.type_op == SSL_ACCEPT) || (con->h2.type_op == PREFACE_MESSAGE) || (con->h2.type_op == SSL_SHUTDOWN))
            {
                if (http2_connection(con) < 0)
                    continue;
            }
            else if ((con->h2.type_op == RECV_SETTINGS) || (con->h2.type_op == WORK_STREAM))
            {
                if (recv_frame(con) < 0)
                    continue;
            }
        }

        if (poll_fd[conn_ind].revents & POLLOUT)
        {
            if ((con->h2.type_op == SSL_ACCEPT) || (con->h2.type_op == SSL_SHUTDOWN))
            {
                http2_connection(con);
            }
            else if ((con->h2.type_op == SEND_SETTINGS) || (con->h2.type_op == WORK_STREAM))
            {
                send_frames(con);
            }
        }
    }

    return 0;
}
//----------------------------------------------------------------------
int EventHandlerClass::wait_connection()
{
    {
    unique_lock<mutex> lk(mtx_thr);
        while ((!work_list_start) && (!wait_list_start) && (!close_thr))
        {
            cond_thr.wait(lk);
        }
    }

    if (close_thr)
        return 1;
    return 0;
}
//----------------------------------------------------------------------
void EventHandlerClass::push_wait_list(Connect *r)
{
    r->sock_timer = 0;
    r->next = NULL;
mtx_thr.lock();
    r->prev = wait_list_end;
    if (wait_list_start)
    {
        wait_list_end->next = r;
        wait_list_end = r;
    }
    else
        wait_list_start = wait_list_end = r;
mtx_thr.unlock();
    cond_thr.notify_one();
}
//----------------------------------------------------------------------
void EventHandlerClass::close_event_handler()
{
    close_thr = 1;
    cond_thr.notify_one();
}
//======================================================================
void event_handler(int n_thr)
{
    event_handler_cl.init(n_thr);
printf(" +++++ worker thread %d run +++++\n", n_thr);
    while (1)
    {
        if (event_handler_cl.wait_connection())
            break;
        event_handler_cl.add_work_list();
        event_handler_cl.cgi_set_poll();
        event_handler_cl.set_poll();
        if (event_handler_cl.poll_worker() < 0)
            break;
    }

    print_err("<%s:%d> ***** exit thread %d *****\n", __func__, __LINE__, n_thr);
}
//======================================================================
void push_wait_list(Connect *r)
{
    event_handler_cl.push_wait_list(r);
}
//======================================================================
void close_work_thread()
{
    event_handler_cl.close_event_handler();
}
