#include "main.h"

using namespace std;
//======================================================================
static mutex mtx_conn;
static condition_variable cond_close_conn;

static int num_conn, close_server;
//======================================================================
void start_conn()
{
mtx_conn.lock();
    ++num_conn;
mtx_conn.unlock();
}
//======================================================================
void wait_close_all_conn()
{
unique_lock<mutex> lk(mtx_conn);
    while (num_conn > 0)
    {
        cond_close_conn.wait(lk);
    }
}
//======================================================================
int is_maxconn()
{
unique_lock<mutex> lk(mtx_conn);
    while ((num_conn >= conf->MaxAcceptConnections) && (!close_server))
    {
        cond_close_conn.wait(lk);
    }

    return close_server;
}
//======================================================================
void close_connect(Connect *r)
{
    if (r->tls.ssl)
    {
        SSL_free(r->tls.ssl);
    }
    else
        print_err(r, "<%s:%d> !!!!! close connect: tls.ssl=NULL\n", __func__, __LINE__);

    shutdown(r->clientSocket, SHUT_RDWR);
    if (close(r->clientSocket))
    {
        print_err(r, "<%s:%d> Error close(): %s\n", __func__, __LINE__, strerror(errno));
    }

    delete r;

mtx_conn.lock();
    --num_conn;
mtx_conn.unlock();
    cond_close_conn.notify_all();
}
//======================================================================
void ssl_shutdown(Connect *r)
{
    if (r->tls.ssl)
    {
        if ((r->tls.err != SSL_ERROR_SSL) &&
            (r->tls.err != SSL_ERROR_SYSCALL))
        {
            int ret = SSL_shutdown(r->tls.ssl);
            if (ret == -1)
            {
                r->tls.err = SSL_get_error(r->tls.ssl, ret);
                print_err(r, "<%s:%d> Error SSL_shutdown()=%d, err=%s\n", __func__, __LINE__, ret, ssl_strerror(r->tls.err));
                if (r->tls.err == SSL_ERROR_ZERO_RETURN)
                {
                    close_connect(r);
                    return;
                }
                else if (r->tls.err == SSL_ERROR_WANT_READ)
                {
                    r->io_status = WAIT;
                    r->tls.poll_event = POLLIN;
                    push_ssl_shutdown(r);
                    return;
                }
                else if (r->tls.err == SSL_ERROR_WANT_WRITE)
                {
                    r->io_status = WAIT;
                    r->tls.poll_event = POLLOUT;
                    push_ssl_shutdown(r);
                    return;
                }
            }
            else if (ret == 0)
            {
                r->io_status = WORK;
                push_ssl_shutdown(r);
                return;
            }
        }
        else
        {
            print_err(r, "<%s:%d> tls.err: %s\n", __func__, __LINE__, ssl_strerror(r->tls.err));
        }
    }
print_err(r, "<%s:%d> SSL shutdown\n", __func__, __LINE__);
    close_connect(r);
}
//======================================================================
Connect *create_req();
//======================================================================
void manager(int sockServer)
{
    unsigned long allConn = 0;
    num_conn = close_server = 0;
    if (chdir(conf->DocumentRoot.c_str()))
    {
        print_err("<%s:%d> Error chdir(%s): %s\n", __func__, __LINE__, conf->DocumentRoot.c_str(), strerror(errno));
        exit(EXIT_FAILURE);
    }
    //------------------------------------------------------------------
    printf(" +++++ pid=%u, uid=%u, gid=%u +++++\n",
                                getpid(), getuid(), getgid());
    //------------------------------------------------------------------
    thread work_thr;
    try
    {
        work_thr = thread(event_handler, 0);
    }
    catch (...)
    {
        print_err("<%s:%d> Error create thread(event_handler): errno=%d\n", __func__, __LINE__, errno);
        exit(errno);
    }
    //------------------------------------------------------------------
    int run = 1;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockServer, &readfds);

    while (run)
    {
        struct sockaddr_storage clientAddr;
        socklen_t addrSize = sizeof(struct sockaddr_storage);

        if (is_maxconn())
            break;

        FD_SET(sockServer, &readfds);
        int ret_sel = select(sockServer + 1, &readfds, NULL, NULL, NULL);
        if (ret_sel <= 0)
        {
            print_err("<%s:%d> Error select()=%d: %s\n", __func__, __LINE__, ret_sel, strerror(errno));
            fprintf(stderr, "<%s:%d> Error select()=%d: %s\n", __func__, __LINE__, ret_sel, strerror(errno));
            //if (errno == EINTR)
            //    continue;
            break;
        }

        if (!FD_ISSET(sockServer, &readfds))
            break;
        int clientSocket = accept(sockServer, (struct sockaddr *)&clientAddr, &addrSize);
        if (clientSocket == -1)
        {
            print_err("<%s:%d>  Error accept(): %s\n", __func__, __LINE__, strerror(errno));
            if ((errno == EMFILE) || (errno == ENFILE)) // (errno == EINTR)
                continue;
            break;
        }

        Connect *req;
        req = create_req();
        if (!req)
        {
            shutdown(clientSocket, SHUT_RDWR);
            close(clientSocket);
            continue;
        }

        int flags = 1;
        flags = fcntl(clientSocket, F_GETFD);
        if (flags == -1)
        {
            print_err("<%s:%d> Error fcntl(F_GETFD): %s\n", __func__, __LINE__, strerror(errno));
            break;
        }

        flags |= FD_CLOEXEC;
        if (fcntl(clientSocket, F_SETFD, flags) == -1)
        {
            print_err("<%s:%d> Error fcntl(F_SETFD, FD_CLOEXEC): %s\n", __func__, __LINE__, strerror(errno));
            break;
        }

        req->init();
        req->numConn = ++allConn;
        req->serverSocket = sockServer;
        req->clientSocket = clientSocket;

        int err;
        if ((err = getnameinfo((struct sockaddr *)&clientAddr,
                addrSize,
                req->remoteAddr,
                sizeof(req->remoteAddr),
                req->remotePort,
                sizeof(req->remotePort),
                NI_NUMERICHOST | NI_NUMERICSERV)))
        {
            print_err(req, "<%s:%d> Error getnameinfo()=%d: %s\n", __func__, __LINE__, err, gai_strerror(err));
            req->remoteAddr[0] = 0;
            req->remotePort[0] = 0;
            shutdown(clientSocket, SHUT_RDWR);
            close(clientSocket);
            delete req;
            continue;
        }

        req->tls.err = 0;
        req->tls.ssl = SSL_new(conf->ctx);
        if (!req->tls.ssl)
        {
            print_err(req, "<%s:%d> Error SSL_new()\n", __func__, __LINE__);
            shutdown(clientSocket, SHUT_RDWR);
            close(clientSocket);
            delete req;
            break;
        }

        int ret = SSL_set_fd(req->tls.ssl, req->clientSocket);
        if (ret == 0)
        {
            req->tls.err = SSL_get_error(req->tls.ssl, ret);
            print_err(req, "<%s:%d> Error SSL_set_fd(): %s\n", __func__, __LINE__, ssl_strerror(req->tls.err));
            SSL_free(req->tls.ssl);
            shutdown(clientSocket, SHUT_RDWR);
            close(clientSocket);
            delete req;
            continue;
        }

        req->tls.poll_event = POLLIN | POLLOUT;
        req->h2.type_op = SSL_ACCEPT;
        start_conn();
        push_pollin_list(req);
    }

    close_work_thread();
    print_err("<%s:%d> all_conn=%lu, open_conn=%d\n",
                    __func__, __LINE__, allConn, num_conn);

    usleep(100000);
}
//======================================================================
Connect *create_req(void)
{
    Connect *req = new(nothrow) Connect;
    if (!req)
        print_err("<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
    return req;
}
//======================================================================
void server_stop()
{
    {
    lock_guard<mutex> lk(mtx_conn);
        close_server = 1;
    }

    cond_close_conn.notify_all();
}
