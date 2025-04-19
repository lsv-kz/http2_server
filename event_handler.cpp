#include "main.h"

using namespace std;
//======================================================================
int create_multipart_head(Connect *req);
void cgi_worker(Connect *con, Response *resp, struct pollfd*);

static EventHandlerClass event_handler_cl;
//======================================================================
EventHandlerClass::~EventHandlerClass()
{
    delete [] conn_array;
    delete [] poll_fd;
}
//----------------------------------------------------------------------
EventHandlerClass::EventHandlerClass()
{
    num_request = 0;
    close_thr = num_wait = num_work = num_cgi = 0;
    work_list_start = work_list_end = wait_list_start = wait_list_end = NULL;
    
    conn_array = new(nothrow) Connect* [conf->MaxAcceptConnections];

    poll_fd = new(nothrow) struct pollfd [conf->MaxAcceptConnections];
    if (!poll_fd)
    {
        print_err("<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
        exit(1);
    }
}
//----------------------------------------------------------------------
void EventHandlerClass::init(int n)
{
    
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

    if (r->h2.type_op == CLOSE_CONNECT)
        close_connect(r);
    else
    {
        ssl_shutdown(r);
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
//----------------------------------------------------------------------
int EventHandlerClass::cgi_set_poll()
{
    num_wait = 0, num_cgi = 0;
    time_t t = time(NULL);
    Connect *c = work_list_start, *next = NULL;
    if (c == NULL)
        return 0;
    for ( ; c; c = next)
    {
        next = c->next;

        c->h2.send_ready &= (~RECV_FROM_CLIENT_WAIT);
        c->h2.num_cgi = 0;
        Response *resp_next = NULL, *resp = c->h2.get();
        if (!resp)
        {
            continue;
        }
        //--------------------------------------------------------------
        for ( ; resp; resp = resp_next)
        {
            resp_next = resp->next;

            if (resp->content != DYN_PAGE)
                continue;

            if (resp->cgi.timer == 0)
                resp->cgi.timer = t;
            if ((t - resp->cgi.timer) >= conf->TimeoutCGI)
            {
                print_err(c, "<%s:%d> TimeoutCGI=%ld, post_data.size()=%d, io_status=%d, cgi.op=%d, id=%d\n", __func__, __LINE__, 
                        t - resp->cgi.timer, resp->post_data.size(), c->io_status, resp->cgi.op, resp->id);
                resp_504(resp);
            }
            else
            {
                ++num_cgi;
                if (resp->cgi.op == CGI_STDIN)
                {
                    if (resp->cgi.cgi_type <= PHPCGI)
                    {
                        if (resp->post_data.size())
                        {
                            conn_array[num_wait] = c;
                            poll_fd[num_wait].fd = resp->cgi.to_script;
                            poll_fd[num_wait++].events = POLLOUT;
                            c->h2.num_cgi++;
                        }
                    }
                    else if (resp->cgi.cgi_type == SCGI)
                    {
                        if (resp->cgi.scgi_op == SCGI_PARAMS)
                        {
                            conn_array[num_wait] = c;
                            poll_fd[num_wait].fd = resp->cgi.fd;
                            poll_fd[num_wait++].events = POLLOUT;
                            c->h2.num_cgi++;
                        }
                        else if (resp->cgi.scgi_op == SCGI_STDIN)
                        {
                            if (resp->post_data.size())
                            {
                                conn_array[num_wait] = c;
                                poll_fd[num_wait].fd = resp->cgi.fd;
                                poll_fd[num_wait++].events = POLLOUT;
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
                            conn_array[num_wait] = c;
                            poll_fd[num_wait].fd = resp->cgi.fd;
                            poll_fd[num_wait++].events = POLLOUT;
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
                            conn_array[num_wait] = c;
                            poll_fd[num_wait].fd = resp->cgi.from_script;
                            poll_fd[num_wait++].events = POLLIN;
                            c->h2.num_cgi++;
                        }
                    }
                    else if (resp->cgi.cgi_type == SCGI)
                    {
                        if (resp->data.size() == 0)
                        {
                            conn_array[num_wait] = c;
                            poll_fd[num_wait].fd = resp->cgi.fd;
                            poll_fd[num_wait++].events = POLLIN;
                            c->h2.num_cgi++;
                        }
                    }
                    else if ((resp->cgi.cgi_type == PHPFPM) || (resp->cgi.cgi_type == FASTCGI))
                    {
                        if (resp->data.size() == 0)
                        {
                            conn_array[num_wait] = c;
                            poll_fd[num_wait].fd = resp->cgi.fd;
                            poll_fd[num_wait++].events = POLLIN;
                            c->h2.num_cgi++;
                        }
                    }
                }
            }
        }
    }
    //------------------------------------------------------------------
    if (num_wait == 0)
        return 0;
    int ret_poll = poll(poll_fd, num_wait, conf->TimeoutPoll);
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
    for ( int conn_ind = 0; conn_ind < num_wait; ++conn_ind)
    {
        c = conn_array[conn_ind];
        if (c->h2.num_cgi == 0)
            continue;

        Response *resp_next = NULL, *resp = c->h2.get();
        if (!resp)
            continue;
        //------------------------ loop Responses -----------------------
        int i = 0;
        for ( ; resp && (i < c->h2.num_cgi); resp = resp_next)
        {
            resp_next = resp->next;
            
            if (resp->content != DYN_PAGE)
                continue;

            if (resp->cgi.cgi_type <= PHPCGI)
            {
                cgi_worker(c, resp, &poll_fd[i]);
            }
            else if (resp->cgi.cgi_type <= FASTCGI)
            {
                fcgi_worker(c, resp, &poll_fd[i]);
            }
            else if (resp->cgi.cgi_type == SCGI)
            {
                if (resp->cgi.scgi_op == SCGI_PARAMS)
                    scgi_worker(c, resp, &poll_fd[i]);
                else
                    cgi_worker(c, resp, &poll_fd[i]);
            }
            else
            {
                print_err("<%s:%d> Error cgi_type=%d, id=%d\n", __func__, __LINE__, resp->cgi.cgi_type, resp->id);
            }
            ++i;
        }
    }
    return 0;
}
//======================================================================
int EventHandlerClass::set_poll()
{
    num_work = num_wait = 0;
    time_t t = time(NULL);
    Connect *c = work_list_start, *next = NULL;
    for ( ; c; c = next)
    {
        next = c->next;

        if (c->sock_timer == 0)
            c->sock_timer = t;

        if ((t - c->sock_timer) >= conf->Timeout)
        {
            print_err(c, "<%s:%d> Timeout=%ld, %s, io_status=%d, SSL_pending()=%d\n", __func__, __LINE__, 
                        t - c->sock_timer, get_str_operation(c->h2.type_op), c->io_status, SSL_pending(c->tls.ssl));
            {
                if ((c->h2.type_op == SSL_ACCEPT) || 
                    (c->h2.type_op == SSL_SHUTDOWN))
                {
                    c->h2.type_op = CLOSE_CONNECT;
                }

                c->err = -1;
                del_from_list(c);
            }
        }
        else
        {
            conn_array[num_wait] = c;

            if ((c->h2.type_op == PREFACE_MESSAGE) || (c->h2.type_op == SSL_ACCEPT) || (c->h2.type_op == SSL_SHUTDOWN))
            {
                poll_fd[num_wait].fd = c->clientSocket;
                if (c->io_status == WORK)
                {
                    ++num_work;
                    poll_fd[num_wait].events = 0;
                }
                else
                {
                    if (c->h2.type_op == PREFACE_MESSAGE)
                        poll_fd[num_wait].events = POLLIN;
                    else if ((c->h2.type_op == SSL_ACCEPT) || (c->h2.type_op == SSL_SHUTDOWN))
                        poll_fd[num_wait].events = c->tls.poll_event;
                }
                
                ++num_wait;
            }
            else
            {
                poll_fd[num_wait].fd = c->clientSocket;

                if (c->io_status == WORK)
                {
                    ++num_work;
                    poll_fd[num_wait].events = 0;
                }
                else
                    poll_fd[num_wait].events = POLLIN;

                Response *resp_next = NULL, *resp = c->h2.get();
                if (!resp)
                {
                    ++num_wait;
                    continue;
                }

                if (c->h2.frame_win_update.size())
                    poll_fd[num_wait].events |= POLLOUT;

                for ( ; resp; resp = resp_next)
                {
                    resp_next = resp->next;
                    if ((resp->post_data.size()) && (poll_fd[num_wait].events & POLLIN))
                    {
                        poll_fd[num_wait].events &= (~POLLIN);
                    }

                    if ((resp->frame_win_update.size() || resp->data.size() || resp->headers.size()) && 
                      ( !(poll_fd[num_wait].events & POLLOUT) )
                    )
                    {
                        poll_fd[num_wait].events |= POLLOUT;
                    }
                }
                
                ++num_wait;
            }
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
        if ((num_work + num_cgi) > 0)
            time_poll = 0;
        ret_poll = poll(poll_fd, num_wait, time_poll);
        if (ret_poll == -1)
        {
            print_err("<%s:%d> Error poll(): %s\n", __func__, __LINE__, strerror(errno));
            return -1;
        }
        else if (ret_poll == 0)
        {
            if (num_work == 0)
                return 0;
        }
    }
    else
    {
        if (num_work == 0)
            return 0;
    }
//print_err("<%s:%d> num_wait=%d, num_work=%d, poll()=%d\n", __func__, __LINE__, num_wait, num_work, ret_poll);
    int i = 0;
    Connect *con = work_list_start;
    for ( ; i < num_wait; ++i)
    {
        con = conn_array[i];

        if ((con->h2.type_op == PREFACE_MESSAGE) || (con->h2.type_op == SSL_ACCEPT) || (con->h2.type_op == SSL_SHUTDOWN))
        {
            if (con->io_status == WORK)
            {
                poll_fd[i].revents = POLLIN | POLLOUT;
                http2_worker_connections(con);
            }
            else
            {
                if (poll_fd[i].revents & (POLLIN | POLLOUT))
                    http2_worker_connections(con);
            }
            continue;
        }
        
        int revents = poll_fd[i].revents;
        int events = poll_fd[i].events;
        int fd = poll_fd[i].fd;
//print_err("<%s:%d> con->io_status=%d revents=0x%02X, events=0x%02X, fd=%d\n", __func__, __LINE__, con->io_status, revents, events, fd);
        if (con->io_status == WORK)
        {
            con->fd_revents = POLLIN;
            if (con->h2.send_ready)
                con->fd_revents |= POLLOUT;
            if (http2_worker_connections(con) < 0)
                continue;
        }
        else
        {
            if (revents & POLLIN)
            {
                con->fd_revents = POLLIN;
                if (http2_worker_connections(con) < 0)
                {
                    continue;
                }
            }
        }

        revents &= (~POLLIN);
        if (revents == POLLOUT)
        {
            if (fd != con->clientSocket)
            {
                print_err(con, "<%s:%d> !!! Error: fd=%d/clientSocket=%d\n", __func__, __LINE__, fd, con->clientSocket);
                exit(2);
            }

            con->fd_revents = revents;
            http2_worker_streams(con);
        }
        else if (revents)
        {
            print_err(con, "<%s:%d> !!! Error: fd=%d, events/revents=0x%x/0x%x\n", __func__, __LINE__, fd, events, revents);
            if ((con->h2.type_op == SSL_ACCEPT) || 
                (con->h2.type_op == SSL_SHUTDOWN))
            {
                con->h2.type_op = CLOSE_CONNECT;
            }

            con->err = -1;
            del_from_list(con);
        }
    }

    return i;
}
//----------------------------------------------------------------------
int EventHandlerClass::wait_conn()
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
void EventHandlerClass::add_wait_list(Connect *r)
{
    r->io_status = WAIT;// WORK;
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
void EventHandlerClass::push_pollin_list(Connect *r)
{
    add_wait_list(r);
}
//----------------------------------------------------------------------
void EventHandlerClass::push_ssl_shutdown(Connect *r)
{
    r->h2.type_op = SSL_SHUTDOWN;
    add_wait_list(r);
}
//----------------------------------------------------------------------
void EventHandlerClass::close_event_handler()
{
    close_thr = 1;
    cond_thr.notify_one();
}
//----------------------------------------------------------------------
int EventHandlerClass::set_pollfd_array(Connect *r, int i)
{
    poll_fd[i].fd = r->clientSocket;
    if (r->io_status == WAIT)
        poll_fd[i].events = POLLIN;
    else
        poll_fd[i].events = 0;

    if (r->h2.size())
        poll_fd[i].events |= POLLOUT;
    r->fd_events = poll_fd[i].events;
    return 0;
}

//======================================================================
void event_handler(int n_thr)
{
    event_handler_cl.init(n_thr);
printf(" +++++ worker thread %d run +++++\n", n_thr);
    while (1)
    {
        if (event_handler_cl.wait_conn())
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
void push_pollin_list(Connect *r)
{
    event_handler_cl.push_pollin_list(r);
}
//======================================================================
void push_ssl_shutdown(Connect *r)
{
    event_handler_cl.push_ssl_shutdown(r);
}
//======================================================================
void close_work_thread()
{
    event_handler_cl.close_event_handler();
}
