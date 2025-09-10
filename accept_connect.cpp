#include "main.h"

using namespace std;
//======================================================================
static mutex mtx_conn;
static condition_variable cond_close_conn;

static Connect *list_start;
static Connect *list_end;
static struct pollfd *poll_fd;

static int num_conn;
//======================================================================
static int ssl_accept(Connect *c);
static void close_connect(Connect *c);
//======================================================================
static void push_list(Connect *c)
{
    c->next = NULL;
    c->prev = list_end;
    if (list_end)
        list_end->next = c;
    list_end = c;
    if (!list_start)
        list_start = c;
}
//======================================================================
static void delete_from_list(Connect *c)
{
    if (!c->next && !c->prev)
    {
        list_start = list_end = NULL;
    }
    else if (!c->next && c->prev)
    {
        list_end = c->prev;
        c->prev->next = NULL;
    }
    else if (c->next && c->prev)
    {
        c->next->prev = c->prev;
        c->prev->next = c->next;
    }
    else if (c->next && !c->prev)
    {
        list_start = c->next;
        c->next->prev = NULL;
    }
}
//======================================================================
void start_conn()
{
mtx_conn.lock();
    ++num_conn;
mtx_conn.unlock();
}
//======================================================================
void close_conn()
{
mtx_conn.lock();
    --num_conn;
mtx_conn.unlock();
    cond_close_conn.notify_one();
}
//======================================================================
bool is_maxconn()
{
unique_lock<mutex> lk(mtx_conn);
    if (num_conn < conf->MaxAcceptConnections)
        return false;
    else
        return true;
}
//======================================================================
void accept_connect(int serverSocket)
{
    unsigned long allConn = 0;
    num_conn = 0;
    list_start = list_end = NULL;

    poll_fd = new(nothrow) struct pollfd [conf->MaxAcceptConnections];
    if (!poll_fd)
    {
        print_err("<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }

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
        work_thr = thread(event_handler);
    }
    catch (...)
    {
        print_err("<%s:%d> Error create thread(event_handler): errno=%d\n", __func__, __LINE__, errno);
        exit(errno);
    }
    //------------------------------------------------------------------
    int run = 1;
    int num_wait = 0;

    poll_fd[0].fd = serverSocket;
    poll_fd[0].events = POLLIN;

    while (run)
    {
        struct sockaddr_storage clientAddr;
        socklen_t addrSize = sizeof(struct sockaddr_storage);

        if (is_maxconn())
            poll_fd[0].events = 0;
        else
            poll_fd[0].events = POLLIN;

        Connect *c = list_start, *next = NULL;
        for ( num_wait = 1; c; ++num_wait, c = next )
        {
            next = c->next;
            poll_fd[num_wait].fd = c->clientSocket;
            poll_fd[num_wait].events = c->tls.poll_event;
//print_err("<%s:%d> num_wait=%d, 0x%02X\n", __func__, __LINE__, num_wait, poll_fd[num_wait].events);
        }

        int timeout = -1;
        if (num_wait > 1)
            timeout = conf->TimeoutPoll;
        int ret_poll = poll(poll_fd, num_wait, timeout);
        if (ret_poll < 0)
        {
            print_err("<%s:%d> Error poll()=-1: %s\n", __func__, __LINE__, strerror(errno));
            if (errno == EINTR)
                continue;
            break;
        }
        else if (ret_poll == 0)
            continue;

        if (poll_fd[0].revents == POLLIN)
        {
            int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize);
            if (clientSocket == -1)
            {
                print_err("<%s:%d>  Error accept(): %s\n", __func__, __LINE__, strerror(errno));
                if ((errno == EMFILE) || (errno == ENFILE)) // (errno == EINTR)
                    continue;
                break;
            }

            Connect *con = new(nothrow) Connect;
            if (!con)
            {
                print_err("<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
                shutdown(clientSocket, SHUT_RDWR);
                close(clientSocket);
                continue;
            }

            con->numConn = ++allConn;
            con->h2.numConn = con->numConn;

            int flags = 1;
            if (ioctl(clientSocket, FIONBIO, &flags) == -1)
            {
                print_err("<%s:%d> Error ioctl(, FIONBIO, 1): %s\n", __func__, __LINE__, strerror(errno));
                break;
            }

            flags = fcntl(clientSocket, F_GETFD);
            if (flags == -1)
            {
                print_err("<%s:%d> Error fcntl(, F_GETFD): %s\n", __func__, __LINE__, strerror(errno));
                break;
            }

            flags |= FD_CLOEXEC;
            if (fcntl(clientSocket, F_SETFD, flags) == -1)
            {
                print_err("<%s:%d> Error fcntl(, F_SETFD, FD_CLOEXEC): %s\n", __func__, __LINE__, strerror(errno));
                break;
            }

            con->h2.numReq = 0;
            con->serverSocket = serverSocket;
            con->clientSocket = clientSocket;

            int err;
            if ((err = getnameinfo((struct sockaddr *)&clientAddr,
                    addrSize,
                    con->h2.remoteAddr,
                    sizeof(con->h2.remoteAddr),
                    con->h2.remotePort,
                    sizeof(con->h2.remotePort),
                    NI_NUMERICHOST | NI_NUMERICSERV)))
            {
                print_err(con, "<%s:%d> Error getnameinfo()=%d: %s\n", __func__, __LINE__, err, gai_strerror(err));
                con->h2.remoteAddr[0] = 0;
                con->h2.remotePort[0] = 0;
                shutdown(clientSocket, SHUT_RDWR);
                close(clientSocket);
                delete con;
                continue;
            }

            con->tls.err = 0;
            con->tls.ssl = SSL_new(conf->ctx);
            if (!con->tls.ssl)
            {
                print_err(con, "<%s:%d> Error SSL_new()\n", __func__, __LINE__);
                shutdown(clientSocket, SHUT_RDWR);
                close(clientSocket);
                delete con;
                break;
            }

            int ret = SSL_set_fd(con->tls.ssl, con->clientSocket);
            if (ret == 0)
            {
                con->tls.err = SSL_get_error(con->tls.ssl, ret);
                print_err(con, "<%s:%d> Error SSL_set_fd(): %s\n", __func__, __LINE__, ssl_strerror(con->tls.err));
                SSL_free(con->tls.ssl);
                shutdown(clientSocket, SHUT_RDWR);
                close(clientSocket);
                delete con;
                continue;
            }

            con->tls.poll_event = POLLIN | POLLOUT;
            con->h2.con_status = SSL_ACCEPT;
            start_conn();
            push_list(con);
        }
        else if (poll_fd[0].revents & POLLERR)
        {
            print_err("<%s:%d> Error revents=0x%02X, num_wait=%d, timeout=%d\n", __func__, __LINE__, 
                    poll_fd[0].revents, num_wait, timeout);
            break;
        }

        if (num_wait <= 1)
            continue;

        c = list_start;
        next = NULL;
        for ( int i = 1; i < num_wait; ++i, c = next )
        {
            next = c->next;
            int revents = poll_fd[i].revents;
            if (revents & (POLLIN | POLLOUT))
            {
                int ret = ssl_accept(c);
                if (ret == 1)
                {
                    delete_from_list(c);
                    c->tls.poll_event = POLLIN;
                    push_wait_list(c);
                }
                else if (ret < 0)
                {
                    close_connect(c);
                }
            }
            else if (revents)
            {
                print_err("<%s:%d>  Error revents=0x%02X\n", __func__, __LINE__, poll_fd[0].revents);
                close_connect(c);
            }
        }
    }

    close_work_thread();
    work_thr.join();
    print_err("<%s:%d> all_conn=%lu, open_conn=%d\n", __func__, __LINE__, allConn, num_conn);
    usleep(100000);
}
//======================================================================
int ssl_accept(Connect *con)
{
    ERR_clear_error();
    int ret = SSL_accept(con->tls.ssl);
    if (ret < 1)
    {
        con->tls.err = SSL_get_error(con->tls.ssl, ret);
        if (conf->PrintDebugMsg)
            print_err(con, "<%s:%d> Error SSL_accept()=%d: %s\n", __func__, __LINE__, ret, ssl_strerror(con->tls.err));
        if (con->tls.err == SSL_ERROR_WANT_READ)
        {
            con->tls.poll_event = POLLIN;
        }
        else if (con->tls.err == SSL_ERROR_WANT_WRITE)
        {
            con->tls.poll_event = POLLOUT;
        }
        else
        {
            if (conf->PrintDebugMsg == false)
                print_err(con, "<%s:%d> Error SSL_accept(): %s\n", __func__, __LINE__, ssl_strerror(con->tls.err));
            return -1;
        }
    }
    else
    {
        const unsigned char *data = NULL;
        unsigned int datalen = 0;
        SSL_get0_alpn_selected(con->tls.ssl, &data, &datalen);
        if (data)
        {
            unsigned int proto_alpn_len = sizeof(proto_alpn);
            for ( unsigned int i = 0; i < proto_alpn_len; i += (unsigned int)(proto_alpn[i] + 1))
            {
                if (datalen != (unsigned int)proto_alpn[i])
                    continue;
                if (memcmp(data, &proto_alpn[i + 1], datalen) == 0)
                {
                    //hex_print_stderr(__func__, __LINE__, data, datalen);
                    con->h2.con_status = PREFACE_MESSAGE;
                    con->sock_timer = 0;
                    return 1;
                }
            }

            hex_print_stderr(__func__, __LINE__, data, datalen);
        }

        print_err(con, "<%s:%d> Error: no protocol has been selected\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}
//======================================================================
void close_connect(Connect *c)
{
    delete_from_list(c);
    if (c->tls.ssl)
    {
        SSL_clear(c->tls.ssl);
        SSL_free(c->tls.ssl);
    }

    shutdown(c->clientSocket, SHUT_RDWR);
    close(c->clientSocket);
    delete_from_list(c);
    delete c;
    close_conn();
}
