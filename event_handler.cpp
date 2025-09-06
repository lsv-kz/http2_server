#include "main.h"

using namespace std;
//======================================================================
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
void EventHandlerClass::init()
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
void EventHandlerClass::del_from_list(Connect *con)
{
    if (con->prev && con->next)
    {
        con->prev->next = con->next;
        con->next->prev = con->prev;
    }
    else if (con->prev && !con->next)
    {
        con->prev->next = con->next;
        work_list_end = con->prev;
    }
    else if (!con->prev && con->next)
    {
        con->next->prev = con->prev;
        work_list_start = con->next;
    }
    else if (!con->prev && !con->next)
        work_list_start = work_list_end = NULL;
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
int EventHandlerClass::cgi_poll()
{
    int cgi_ind_array = 0;
    num_cgi_poll = 0;
    time_t t = time(NULL);
    Connect *c = work_list_start, *next = NULL;
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
                        t - resp->cgi.timer, resp->post_data.size(), resp->cgi_status, resp->id);
                c->h2.frame_win_update.init();
                resp->frame_win_update.init();
                if (c->h2.try_again == true)
                    resp->rst_stream = true;
                else
                {
                    if (resp->send_headers)
                        set_rst_stream(c, resp->id, CANCEL);
                    else
                        resp_504(resp);
                }
            }
            else
            {
                if ((resp->cgi_status == CGI_CREATE) && (all_cgi < conf->MaxCgiProc))
                {
                    if ((resp->cgi_type == CGI) || (resp->cgi_type == PHPCGI))
                    {
                        int ret = cgi_create_proc(resp);
                        if (ret < 0)
                        {
                            print_err(c, "<%s:%d> Error cgi_create_proc()\n", __func__, __LINE__);
                            if (ret == -1)
                                resp_500(resp);
                            continue;
                        }

                        if (resp->method == "POST")
                        {
                            if(resp->post_cont_length <= 0)
                            {
                                resp->cgi_status = CGI_STDOUT;
                                close(resp->cgi.to_script);
                                resp->cgi.to_script = -1;
                            }
                            else
                                resp->cgi_status = CGI_STDIN;
                        }
                        else
                            resp->cgi_status = CGI_STDOUT;
                    }
                    else if ((resp->cgi_type == PHPFPM) || (resp->cgi_type == FASTCGI))
                    {
                        int ret = fcgi_create_connect(resp);
                        if (ret < 0)
                        {
                            resp_500(resp);
                            continue;
                        }
                        else
                            resp->cgi_status = FASTCGI_BEGIN;
                    }
                    else if (resp->cgi_type == SCGI)
                    {
                        int ret = scgi_create_connect(resp);
                        if (ret < 0)
                        {
                            resp_500(resp);
                            continue;
                        }
                        else
                            resp->cgi_status = SCGI_PARAMS;
                    }

                    resp->cgi.start = true;
                    ++all_cgi;
                }

                if ((resp->cgi_status == FASTCGI_BEGIN) || (resp->cgi_status == FASTCGI_PARAMS) || (resp->cgi_status == SCGI_PARAMS))
                {
                    poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                    poll_fd[num_cgi_poll++].events = POLLOUT;
                    c->h2.num_cgi++;
                }
                else if ((resp->cgi_status == CGI_STDIN) && resp->post_data.size())
                {
                    if ((resp->cgi_type == CGI) || (resp->cgi_type == PHPCGI))
                    {
                        poll_fd[num_cgi_poll].fd = resp->cgi.to_script;
                        poll_fd[num_cgi_poll++].events = POLLOUT;
                        c->h2.num_cgi++;
                    }
                    else
                    {
                        poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                        poll_fd[num_cgi_poll++].events = POLLOUT;
                        c->h2.num_cgi++;
                    }
                }
                else if ((resp->cgi_status == CGI_STDOUT) && (resp->data.size() == 0))
                {
                    if ((resp->cgi_type == CGI) || (resp->cgi_type == PHPCGI))
                    {
                        poll_fd[num_cgi_poll].fd = resp->cgi.from_script;
                        poll_fd[num_cgi_poll++].events = POLLIN;
                        c->h2.num_cgi++;
                    }
                    else
                    {
                        poll_fd[num_cgi_poll].fd = resp->cgi.fd;
                        poll_fd[num_cgi_poll++].events = POLLIN;
                        c->h2.num_cgi++;
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
                return -1;
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
                if ((resp->cgi_type == CGI) || (resp->cgi_type == PHPCGI))
                {
                    cgi_worker(c, resp, &poll_fd[poll_ind]);
                }
                else if ((resp->cgi_type == PHPFPM) || (resp->cgi_type == FASTCGI))
                {
                    fcgi_worker(c, resp, &poll_fd[poll_ind]);
                }
                else if (resp->cgi_type == SCGI)
                {
                    if (resp->cgi_status == SCGI_PARAMS)
                        scgi_worker(c, resp, &poll_fd[poll_ind]);
                    else
                        cgi_worker(c, resp, &poll_fd[poll_ind]);
                }
                else
                {
                    print_err("<%s:%d> Error cgi_type=%d, id=%d \n", __func__, __LINE__, resp->cgi_type, resp->id);
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
void EventHandlerClass::http2_set_poll()
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

        if ((c->h2.con_status == PREFACE_MESSAGE) ||
                (c->h2.con_status == SSL_ACCEPT) ||
                (c->h2.con_status == SSL_SHUTDOWN) ||
                (c->h2.con_status == RECV_SETTINGS) ||
                (c->h2.con_status == SEND_SETTINGS)
        )
        {
            Timeout = conf->Timeout;
        }
        else
            Timeout = conf->TimeoutKeepAlive;

        if ((t - c->sock_timer) >= Timeout)
        {
            print_err(c, "<%s:%d> Timeout=%ld, %s\n", __func__, __LINE__, t - c->sock_timer, get_str_operation(c->h2.con_status));
            if ((c->h2.con_status == SSL_ACCEPT) || (c->h2.con_status == SSL_SHUTDOWN))
                close_connect(c);
            else
                ssl_shutdown(c);
        }
        else
        {
            int ret = 0, pending = 0;
            while ((pending = SSL_pending(c->tls.ssl)) && (c->h2.con_status != SEND_SETTINGS))
            {
                if (conf->PrintDebugMsg)
                {
                    print_err(c, "<%s:%d> ***** SSL_pending()=%d, %s\n", __func__, __LINE__, 
                                pending, get_str_operation(c->h2.con_status));
                }

                if ((c->h2.con_status == RECV_SETTINGS) || (c->h2.con_status == PROCESSING_REQUESTS))
                {
                    if (c->h2.goaway.size())
                    {
                        print_err(c, "<%s:%d> c->h2.goaway.size() > 0, %s\n", __func__, __LINE__, get_str_operation(c->h2.con_status));
                        break;
                    }

                    if ((ret = recv_frame(c)) < 0)
                        break;
                }
                else if ((c->h2.con_status == SSL_ACCEPT) || (c->h2.con_status == PREFACE_MESSAGE) || (c->h2.con_status == SSL_SHUTDOWN))
                {
                    if ((ret = http2_connection(c)) < 0)
                        break;
                }
            }

            if (ret < 0)
                continue;
            if (c->h2.work_stream == NULL)
                c->h2.work_stream = c->h2.start_stream;

            poll_fd[num_wait].fd = c->clientSocket;
            poll_fd[num_wait].events = 0;
            if ((c->h2.con_status == SSL_ACCEPT) || (c->h2.con_status == SSL_SHUTDOWN))
                poll_fd[num_wait].events = c->tls.poll_event;
            else if ((c->h2.con_status == PREFACE_MESSAGE) || (c->h2.con_status == RECV_SETTINGS))
                poll_fd[num_wait].events = POLLIN;
            else if (c->h2.con_status == SEND_SETTINGS)
                poll_fd[num_wait].events = POLLOUT;
            else // h2.con_status = WORK_STREAM
            {
                if (c->h2.goaway.size() || c->h2.ping.size() || c->h2.start_list_send_frame || c->h2.try_again)
                {
                    poll_fd[num_wait].events = POLLOUT;
                    conn_array[num_wait++] = c;
                    continue;
                }

                if (c->h2.frame_win_update.size() || (c->h2.cgi_window_update > 0))
                {
                    poll_fd[num_wait].events = POLLOUT;
                    conn_array[num_wait++] = c;
                    continue;
                }

                poll_fd[num_wait].events = POLLIN;
                if (c->h2.work_stream)
                {
                    if ((c->h2.connect_window_size <= 0) && c->h2.work_stream->send_headers)
                    {
                        if (conf->PrintDebugMsg)
                            print_err(c, "<%s:%d> connect_window_size <= 0\n", __func__, __LINE__);
                        conn_array[num_wait++] = c;
                        continue;
                    }
                }

                Stream *resp = c->h2.work_stream, *resp_next = NULL;
                for ( ; resp; resp = resp_next)
                {
                    resp_next = resp->next;
                    if (resp_next == NULL)
                        resp_next = c->h2.start_stream;

                    if (resp->frame_win_update.size() || 
                        resp->headers.size() || 
                        resp->data.size() || 
                        resp->rst_stream ||
                        (resp->send_headers && (resp->stream_window_size > 0))
                    )
                    {
                        poll_fd[num_wait].events |= POLLOUT;
                        c->h2.work_stream = resp;
                        break;
                    }
                    
                    if (resp_next == c->h2.work_stream)
                        break;
                }
            }

            if (poll_fd[num_wait].events)
                conn_array[num_wait++] = c;
        }
    }
}
//======================================================================
int EventHandlerClass::http2_poll()
{
    if (num_wait == 0)
        return 0;
    int time_poll = conf->TimeoutPoll;
    if (all_cgi > 0)
        time_poll = 0;
    int ret_poll = poll(poll_fd, num_wait, time_poll);
    if (ret_poll == -1)
    {
        print_err("<%s:%d> Error poll(): %s\n", __func__, __LINE__, strerror(errno));
        return -1;
    }
    else if (ret_poll == 0)
    {
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
            print_err(con, "<%s:%d> !!! Error: events=0x%02x, revents=0x%02x, %s\n", __func__, __LINE__, poll_fd[conn_ind].events, revents, get_str_operation(con->h2.con_status));
            if ((con->h2.con_status == SSL_ACCEPT) || 
                (con->h2.con_status == SSL_SHUTDOWN))
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
            if ((con->h2.con_status == SSL_ACCEPT) || (con->h2.con_status == PREFACE_MESSAGE) || (con->h2.con_status == SSL_SHUTDOWN))
            {
                if (http2_connection(con) < 0)
                    continue;
            }
            else if ((con->h2.con_status == RECV_SETTINGS) || (con->h2.con_status == PROCESSING_REQUESTS))
            {
                if (recv_frame(con) < 0)
                    continue;
            }
        }

        if (poll_fd[conn_ind].revents & POLLOUT)
        {
            if ((con->h2.con_status == SSL_ACCEPT) || (con->h2.con_status == SSL_SHUTDOWN))
            {
                http2_connection(con);
            }
            else if ((con->h2.con_status == SEND_SETTINGS) || (con->h2.con_status == PROCESSING_REQUESTS))
            {
                send_frames(con);
            }
        }
    }

    return 0;
}
//======================================================================
void EventHandlerClass::close_connect(Connect *con)
{
    if (conf->PrintDebugMsg)
        print_err(con, "<%s:%d> Close connection\n", __func__, __LINE__);

    if (con->tls.ssl)
    {
        SSL_clear(con->tls.ssl);
        SSL_free(con->tls.ssl);
    }

    shutdown(con->clientSocket, SHUT_RDWR);
    if (close(con->clientSocket))
    {
        print_err(con, "<%s:%d> Error close(): %s\n", __func__, __LINE__, strerror(errno));
    }

    del_from_list(con);
    delete con;
    close_conn();
}
//======================================================================
void EventHandlerClass::ssl_shutdown(Connect *con)
{
    if (conf->PrintDebugMsg)
        print_err(con, "<%s:%d> ssl_shutdown\n", __func__, __LINE__);
    if (con->tls.ssl)
    {
        if ((con->tls.err != SSL_ERROR_SSL) && (con->tls.err != SSL_ERROR_SYSCALL))
        {
            con->h2.con_status = SSL_SHUTDOWN;
            for ( int i = 0; i < 2; ++i)
            {
                int ret = SSL_shutdown(con->tls.ssl);
                if (conf->PrintDebugMsg)
                    print_err(con, "<%s:%d> SSL_shutdown()=%d\n", __func__, __LINE__, ret);
                if (ret == -1)
                {
                    con->tls.err = SSL_get_error(con->tls.ssl, ret);
                    if (conf->PrintDebugMsg)
                        print_err(con, "<%s:%d> Error SSL_shutdown()=%d: %s\n", __func__, __LINE__, ret, ssl_strerror(con->tls.err));
                    if (con->tls.err == SSL_ERROR_ZERO_RETURN)
                    {
                        close_connect(con);
                        return;
                    }
                    else if (con->tls.err == SSL_ERROR_WANT_READ)
                    {
                        con->tls.poll_event = POLLIN;
                        con->sock_timer = 0;
                        return;
                    }
                    else if (con->tls.err == SSL_ERROR_WANT_WRITE)
                    {
                        con->tls.poll_event = POLLOUT;
                        con->sock_timer = 0;
                        return;
                    }
                    else
                    {
                        //if (conf->PrintDebugMsg == false)
                            //print_err(con, "<%s:%d> Error: %s\n", __func__, __LINE__, ssl_strerror(con->tls.err));
                        break;
                    }
                }
                else if (ret == 0)
                {
                    continue;
                }
                else
                    break;
            }
        }
        else
        {
            print_err(con, "<%s:%d> tls.err: %s\n", __func__, __LINE__, ssl_strerror(con->tls.err));
        }
    }

    close_connect(con);
}
//======================================================================
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
//======================================================================
void EventHandlerClass::push_wait_list(Connect *con)
{
    con->sock_timer = 0;
    con->next = NULL;
mtx_thr.lock();
    con->prev = wait_list_end;
    if (wait_list_start)
    {
        wait_list_end->next = con;
        wait_list_end = con;
    }
    else
        wait_list_start = wait_list_end = con;
mtx_thr.unlock();
    cond_thr.notify_one();
}
//======================================================================
void EventHandlerClass::close_connections()
{
    if (work_list_start)
    {
        Connect *con = work_list_start, *next = NULL;
        for ( ; con; con = next)
        {
            next = con->next;
            if (con->tls.ssl)
            {
                if((con->tls.err != SSL_ERROR_SSL) && (con->tls.err != SSL_ERROR_SYSCALL))
                    SSL_shutdown(con->tls.ssl);
                SSL_free(con->tls.ssl);
            }

            shutdown(con->clientSocket, SHUT_RDWR);
            close(con->clientSocket);
            delete con;
        }
    }

    if (wait_list_start)
    {
        Connect *con = wait_list_start, *next = NULL;
        for ( ; con; con = next)
        {
            next = con->next;
            if (con->tls.ssl)
            {
                if((con->tls.err != SSL_ERROR_SSL) && (con->tls.err != SSL_ERROR_SYSCALL))
                    SSL_shutdown(con->tls.ssl);
                SSL_free(con->tls.ssl);
            }

            shutdown(con->clientSocket, SHUT_RDWR);
            close(con->clientSocket);
            delete con;
        }
    }
}
//======================================================================
void EventHandlerClass::close_event_handler()
{
    close_thr = 1;
    cond_thr.notify_one();
}
//======================================================================
void EventHandlerClass::inc_all_cgi()
{
    --all_cgi;
}
//======================================================================
void event_handler()
{
    event_handler_cl.init();
printf(" +++++ worker thread run +++++\n");
    while (1)
    {
        if (event_handler_cl.wait_connection())
            break;
        event_handler_cl.add_work_list();
        if (event_handler_cl.cgi_poll() < 0)
            break;
        event_handler_cl.http2_set_poll();
        if (event_handler_cl.http2_poll() < 0)
            break;
    }

    event_handler_cl.close_connections();
    print_err("<%s:%d> ***** exit event_handler *****\n", __func__, __LINE__);
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
//======================================================================
void inc_all_cgi()
{
    event_handler_cl.inc_all_cgi();
}
