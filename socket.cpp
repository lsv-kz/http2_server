#include "main.h"

//======================================================================
int create_server_socket(const Config *conf)
{
    int sockfd, n;
    const int sock_opt = 1;
    struct addrinfo  hints, *result, *rp;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((n = getaddrinfo(conf->ServerAddr.c_str(), conf->ServerPort.c_str(), &hints, &result)) != 0)
    {
        fprintf(stderr, "Error getaddrinfo(%s:%s): %s\n", conf->ServerAddr.c_str(), conf->ServerPort.c_str(), gai_strerror(n));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(sockfd);
    }

    freeaddrinfo(result);

    if (rp == NULL)
    {
        fprintf(stderr, "Error: failed to bind\n");
        return -1;
    }

    if (conf->TcpNoDelay)
    {
        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&sock_opt, sizeof(sock_opt)); // SOL_TCP
    }

    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1)
    {
        fprintf(stderr, "Error fcntl(, F_GETFL, ): %s\n", strerror(errno));
    }
    else
    {
        flags |= O_NONBLOCK;
        if (fcntl(sockfd, F_SETFL, flags) == -1)
        {
            fprintf(stderr, "Error fcntl(, F_SETFL, ): %s\n", strerror(errno));
        }
    }

    flags = fcntl(sockfd, F_GETFD);
    if (flags == -1)
    {
        print_err("<%s:%d> Error fcntl(F_GETFD): %s\n", __func__, __LINE__, strerror(errno));
        close(sockfd);
        return -1;
    }

    flags |= FD_CLOEXEC;
    if (fcntl(sockfd, F_SETFD, flags) == -1)
    {
        print_err("<%s:%d> Error fcntl(F_SETFD, FD_CLOEXEC): %s\n", __func__, __LINE__, strerror(errno));
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, conf->ListenBacklog) == -1)
    {
        fprintf(stderr, "Error listen(): %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}
//======================================================================
int create_fcgi_socket(Stream *resp)
{
    int sockfd, n;
    char addr[256];
    char port[16];

    n = sscanf(resp->cgi.socket->c_str(), "%[^:]:%s", addr, port);
    if (n == 2) //==== AF_INET ====
    {
        struct sockaddr_in sock_addr;
        memset(&sock_addr, 0, sizeof(sock_addr));

        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd == -1)
        {
            print_err(resp, "<%s:%d> Error socket(): %s\n", __func__, __LINE__, strerror(errno));
            return -1;
        }

        const int sock_opt = 1;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&sock_opt, sizeof(sock_opt)))
        {
            print_err(resp, "<%s:%d> Error setsockopt(TCP_NODELAY): %s\n", __func__, __LINE__, strerror(errno));
            close(sockfd);
            return -1;
        }

        sock_addr.sin_port = htons(atoi(port));
        sock_addr.sin_family = AF_INET;
        if (inet_aton(addr, &(sock_addr.sin_addr)) == 0)
//      if (inet_pton(AF_INET, addr, &(sock_addr.sin_addr)) < 1)
        {
            print_err(resp, "<%s:%d> Error inet_pton(%s): %s\n", __func__, __LINE__, addr, strerror(errno));
            close(sockfd);
            return -1;
        }

        int flags = 1;
        if (ioctl(sockfd, FIONBIO, &flags) == -1)
        {
            print_err(resp, "<%s:%d> Error ioctl(FIONBIO, 1): %s\n", __func__, __LINE__, strerror(errno));
            return -1;
        }

        if (connect(sockfd, (struct sockaddr *)(&sock_addr), sizeof(sock_addr)) != 0)
        {
            if (errno != EINPROGRESS)
            {
                print_err(resp, "<%s:%d> Error connect(%s): %s\n", __func__, __LINE__, resp->cgi.socket->c_str(), strerror(errno));
                close(sockfd);
                return -1;
            }
            else
            {
                errno = 0;
            }
        }
    }
    else //==== PF_UNIX ====
    {
        struct sockaddr_un sock_addr;
        sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            print_err(resp, "<%s:%d> Error socket(): %s\n", __func__, __LINE__, strerror(errno));
            return -1;
        }

        sock_addr.sun_family = AF_UNIX;
        snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path), "%s", resp->cgi.socket->c_str());

        int flags = fcntl(sockfd, F_GETFL);
        if (flags == -1)
        {
            print_err(resp, "<%s:%d> Error fcntl(, F_GETFL, ): %s\n", __func__, __LINE__, strerror(errno));
            return -1;
        }
        else
        {
            flags |= O_NONBLOCK;
            if (fcntl(sockfd, F_SETFL, flags) == -1)
            {
                print_err(resp, "<%s:%d> Error fcntl(, F_SETFL, ): %s\n", __func__, __LINE__, strerror(errno));
                return -1;
            }
        }

        if (connect(sockfd, (struct sockaddr *) &sock_addr, SUN_LEN(&sock_addr)) == -1)
        {
            if (errno != EINPROGRESS)
            {
                print_err(resp, "<%s:%d> Error connect(%s): %s\n", __func__, __LINE__, resp->cgi.socket->c_str(), strerror(errno));
                close(sockfd);
                return -1;
            }
            else
            {
                errno = 0;
            }
        }
    }

    int flags = fcntl(sockfd, F_GETFD);
    if (flags == -1)
    {
        print_err(resp, "<%s:%d> Error fcntl(F_GETFD): %s\n", __func__, __LINE__, strerror(errno));
        close(sockfd);
        return -1;
    }

    flags |= FD_CLOEXEC;
    if (fcntl(sockfd, F_SETFD, flags) == -1)
    {
        print_err(resp, "<%s:%d> Error fcntl(F_SETFD, FD_CLOEXEC): %s\n", __func__, __LINE__, strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}
//======================================================================
int write_to_client(Connect *req, const char *buf, int len, int id)
{
    return ssl_write(req, buf, len, id);
}
//======================================================================
int read_from_client(Connect *req, char *buf, int len)
{
    return ssl_read(req, buf, len);
}
//======================================================================
int write_to_fcgi(int fd, const char *buf, int len)
{
    int ret = write(fd, buf, len);
    if (ret == -1)
    {
        if (errno == EAGAIN)
            return ERR_TRY_AGAIN;
        else
        {
            print_err("<%s:%d> Error write to fcgi: %s\n", __func__, __LINE__, strerror(errno));
            return -1;
        }
    }

    return ret;
}
//======================================================================
int get_size_sock_buf(int domain, int optname, int type, int protocol)
{
    int sock = socket(domain, type, protocol);
    if (sock < 0)
    {
        fprintf(stderr, "<%s:%d> Error socketpair(): %s\n", __func__, __LINE__, strerror(errno));
        return -errno;
    }

    int sndbuf;
    socklen_t optlen = sizeof(sndbuf);
    if (getsockopt(sock, SOL_SOCKET, optname, &sndbuf, &optlen) < 0)
    {
        fprintf(stderr, "<%s:%d> Error getsockopt(SO_SNDBUF): %s\n", __func__, __LINE__, strerror(errno));
        close(sock);
        return -errno;
    }

    close(sock);
    return sndbuf;
}
