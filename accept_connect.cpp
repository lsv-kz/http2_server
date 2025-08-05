#include "main.h"

using namespace std;
//======================================================================
static mutex mtx_conn;
static condition_variable cond_close_conn;

static int num_conn;
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
void is_maxconn()
{
unique_lock<mutex> lk(mtx_conn);
    while (num_conn >= conf->MaxAcceptConnections)
    {
        cond_close_conn.wait(lk);
    }
}
//======================================================================
void accept_connect(int serverSocket)
{
    unsigned long allConn = 0;
    num_conn = 0;
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

    fd_set readfds;
    FD_ZERO(&readfds);

    while (run)
    {
        struct sockaddr_storage clientAddr;
        socklen_t addrSize = sizeof(struct sockaddr_storage);

        is_maxconn();

        FD_SET(serverSocket, &readfds);
        int ret_sel = select(serverSocket + 1, &readfds, NULL, NULL, NULL);
        if (ret_sel <= 0)
        {
            print_err("<%s:%d> Error select()=%d: %s\n", __func__, __LINE__, ret_sel, strerror(errno));
            if (errno == EINTR)
                continue;
            break;
        }

        if (!FD_ISSET(serverSocket, &readfds))
            break;
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
        push_wait_list(con);
    }

    close_work_thread();
    work_thr.join();
    print_err("<%s:%d> all_conn=%lu, open_conn=%d\n", __func__, __LINE__, allConn, num_conn);
    usleep(100000);
}
