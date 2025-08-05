#include "main.h"

using namespace std;
//======================================================================
const char *get_script_name(const char *name)
{
    const char *p;
    if (!name)
        return "";

    if ((p = strchr(name + 1, '/')))
        return p;

    return "";
}
//======================================================================
const char *base_name(const char *path)
{
    const char *p;

    if (!path)
        return NULL;

    p = strrchr(path, '/');
    if (p)
        return p + 1;

    return path;
}
//======================================================================
void kill_chld(Stream *r)
{
    if (r->cgi.pid > 0)
    {
        if (waitpid(r->cgi.pid, NULL, WNOHANG) == 0)
            kill(r->cgi.pid, SIGKILL);
    }
}
//======================================================================
int EventHandlerClass::cgi_fork(Stream *resp, int* serv_cgi, int* cgi_serv)
{
    struct stat st;

    if (resp->cgi_type == CGI)
    {
        resp->cgi.path = conf->ScriptPath;
        resp->cgi.script_name = get_script_name(resp->uri);
    }
    else if (resp->cgi_type == PHPCGI)
    {
        resp->cgi.path = conf->DocumentRoot;
        resp->cgi.script_name = resp->uri;
    }

    resp->cgi.path += resp->cgi.script_name;
    if (stat(resp->cgi.path.c_str(), &st) == -1)
    {
        print_err(resp, "<%s:%d> script (%s) not found\n", __func__, __LINE__, resp->cgi.path.c_str());
        resp_404(resp);
        return -RS404;
    }
    //--------------------------- fork ---------------------------------
    pid_t pid = fork();
    if (pid < 0)
    {
        resp->cgi.pid = pid;
        print_err(resp, "<%s:%d> Error fork(): %s\n", __func__, __LINE__, strerror(errno));
        return -1;
    }
    else if (pid == 0)
    {
        //----------------------- child --------------------------------
        close(cgi_serv[0]);
        cgi_serv[0] = -1;

        int fd = open("/dev/null", O_RDONLY);
        if (fd > 0)
        {
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        if (resp->method == "POST")
        {
            close(serv_cgi[1]);
            if (serv_cgi[0] != STDIN_FILENO)
            {
                if (dup2(serv_cgi[0], STDIN_FILENO) < 0)
                {
                    print_err(resp, "<%s:%d> Error dup2(): %s\n", __func__, __LINE__, strerror(errno));
                    exit(1);
                }
                close(serv_cgi[0]);
                serv_cgi[0] = -1;
            }
        }

        if (cgi_serv[1] != STDOUT_FILENO)
        {
            if (dup2(cgi_serv[1], STDOUT_FILENO) < 0)
            {
                print_err(resp, "<%s:%d> Error dup2(): %s\n", __func__, __LINE__, strerror(errno));
                goto to_pipe;
            }
            close(cgi_serv[1]);
            cgi_serv[1] = -1;
        }

        if (resp->cgi_type == PHPCGI)
            setenv("REDIRECT_STATUS", "true", 1);
        setenv("SERVER_SOFTWARE", conf->ServerSoftware.c_str(), 1);
        setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
        setenv("DOCUMENT_ROOT", conf->DocumentRoot.c_str(), 1);
        setenv("REMOTE_ADDR", resp->remoteAddr.c_str(), 1);
        setenv("REMOTE_PORT", resp->remotePort.c_str(), 1);
        setenv("REQUEST_URI", resp->path.c_str(), 1);
        setenv("REQUEST_METHOD", resp->method.c_str(), 1);
        setenv("SERVER_PROTOCOL", "HTTP/2", 1);

        if (resp->referer.size())
            setenv("HTTP_REFERER", resp->referer.c_str(), 1);
        if (resp->user_agent.size())
            setenv("HTTP_USER_AGENT", resp->user_agent.c_str(), 1);

        setenv("SCRIPT_NAME", resp->cgi.script_name.c_str(), 1);
        setenv("SCRIPT_FILENAME", resp->cgi.path.c_str(), 1);

        if (resp->method == "POST")
        {
            if (resp->content_type.size())
                setenv("CONTENT_TYPE", resp->content_type.c_str(), 1);

            if (resp->content_length.size())
                setenv("CONTENT_LENGTH", resp->content_length.c_str(), 1);
        }

        if (resp->cgi.query_string.size())
            setenv("QUERY_STRING", resp->cgi.query_string.c_str(), 1);

        if (resp->cgi_type == CGI)
        {
            execl(resp->cgi.path.c_str(), base_name(resp->cgi.path.c_str()), NULL);
        }
        else if (resp->cgi_type == PHPCGI)
        {
            execl(conf->PathPHP.c_str(), base_name(conf->PathPHP.c_str()), NULL);
        }
        print_err(resp, "<%s:%d> Error execl(%s, %s): %s\n", __func__, __LINE__,
                        resp->cgi.path.c_str(), base_name(resp->cgi.path.c_str()), strerror(errno));

    to_pipe:
        char err_msg[] = "Status: 500 Internal Server Error\r\n"
                "Content-type: text/html; charset=UTF-8\r\n"
                "\r\n"
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head>\n"
                "<title>500 Internal Server Error</title>\n"
                "<meta http-equiv=\"content-type\" content=\"text/html\">\n"
                "</head>\n"
                "<body>\n"
                "<p> 500 Internal Server Error</p>\n"
                "</body>\n"
                "</html>";
        write(STDOUT_FILENO, err_msg, strlen(err_msg));
        exit(EXIT_FAILURE);
    }
    else
    {
        resp->cgi.pid = pid;
        int opt = 1;
        if (resp->method == "POST")
        {
            ioctl(serv_cgi[1], FIONBIO, &opt);
        }

        ioctl(cgi_serv[0], FIONBIO, &opt);
    }

    return 0;
}
//======================================================================
int EventHandlerClass::cgi_create_proc(Stream *resp)
{
    int serv_cgi[2], cgi_serv[2];

    int n = pipe(cgi_serv);
    if (n == -1)
    {
        print_err(resp, "<%s:%d> Error pipe()=%d, id=%d \n", __func__, __LINE__, n, resp->id);
        return -1;
    }

    if (resp->method == "POST")
    {
        n = pipe(serv_cgi);
        if (n == -1)
        {
            print_err(resp, "<%s:%d> Error pipe()=%d, id=%d \n", __func__, __LINE__, n, resp->id);
            close(cgi_serv[0]);
            cgi_serv[0] = -1;

            close(cgi_serv[1]);
            cgi_serv[1] = -1;
            return -1;
        }
    }
    else
    {
        serv_cgi[0] = -1;
        serv_cgi[1] = -1;
    }

    n = cgi_fork(resp, serv_cgi, cgi_serv);
    if (n < 0)
    {
        if (resp->method == "POST")
        {
            if (serv_cgi[0] > 0)
            {
                close(serv_cgi[0]);
                serv_cgi[0] = -1;
            }
            if (serv_cgi[1] > 0)
            {
                close(serv_cgi[1]);
                serv_cgi[1] = -1;
            }
        }

        close(cgi_serv[0]);
        cgi_serv[0] = -1;

        close(cgi_serv[1]);
        cgi_serv[1] = -1;
        return n;
    }
    else
    {
        if (resp->method == "POST")
        {
            if (serv_cgi[0] > 0)
            {
                close(serv_cgi[0]);
                serv_cgi[0] = -1;
            }
        }

        close(cgi_serv[1]);
        cgi_serv[1] = -1;

        resp->cgi.from_script = cgi_serv[0];
        resp->cgi.to_script = serv_cgi[1];
    }

    return 0;
}
//======================================================================
int EventHandlerClass::cgi_stdin(Stream *resp, int fd)
{
    int ret = write(fd, resp->post_data.ptr_remain(), resp->post_data.size_remain());
    if (ret <= 0)
    {
        if (errno == EAGAIN)
        {
            return ERR_TRY_AGAIN;
        }
        else
        {
            print_err(resp, "<%s:%d> Error write()=%d: %s; id=%d \n", __func__, __LINE__, ret, strerror(errno), resp->id);
            return -1;
        }
    }

    resp->post_cont_length -= ret;
    resp->cgi.send_to_cgi += ret;
    if (ret != resp->post_data.size_remain())
    {
        resp->post_data.set_offset(ret);
        resp->cgi.timer = 0;
    }
    else
    {
        resp->cgi.timer = 0;
        resp->post_data.init();

        if (resp->post_cont_length == 0)
        {
            resp->cgi_status = CGI_STDOUT;
            if (resp->cgi_type <= PHPCGI)
            {
                if (resp->cgi.to_script > 0)
                {
                    close(resp->cgi.to_script);
                    resp->cgi.to_script = -1;
                }
            }
        }
    }

    return ret;
}
//======================================================================
int EventHandlerClass::cgi_stdout(Stream *resp, int fd)
{
    char buf[16384];

    int ret = read(fd, buf, conf->DataBufSize);
    if (ret == -1)
    {
        print_err(resp, "<%s:%d> Error read(): %s\n", __func__, __LINE__, strerror(errno));
        if (errno == EAGAIN)
            return ERR_TRY_AGAIN;
        return -1;
    }
    else if (ret > 0)
    {
        resp->cgi.recv_from_cgi += ret;
        resp->html.cat(buf, ret);
        resp->cgi.timer = 0;
    }

    return ret;
}
//======================================================================
int iscgi(Stream *resp)
{
    const char *p = strrchr(resp->uri, '/');
    if (!p)
        return 0;
    fcgi_list_addr *i = conf->fcgi_list;
    for (; i; i = i->next)
    {
        if (i->script_name[0] == '~')
        {
            if (!strcmp(p, i->script_name.c_str() + 1))
                break;
        }
        else
        {
            if (resp->uri == i->script_name)
                break;
        }
    }

    if (!i)
        return 0;

    resp->cgi.socket = &i->addr;
    resp->content = DYN_PAGE;
    if (i->type == FASTCGI)
        resp->cgi_type = FASTCGI;
    else if (i->type == SCGI)
        resp->cgi_type = SCGI;
    else
    {
        resp->content = ERROR_TYPE;
        return 0;
    }
    resp->cgi.script_name = i->script_name;

    return 1;
}
//======================================================================
void EventHandlerClass::cgi_worker(Connect *c, Stream *resp, struct pollfd *poll_fd)
{
    int revents = poll_fd->revents;
    int events = poll_fd->events;
    int fd = poll_fd->fd;

    if (resp->cgi_status == CGI_STDIN)
    {
        if (resp->cgi_type <= PHPCGI)
        {
            if (resp->cgi.to_script != fd)
            {
                print_err(resp, "<%s:%d> Error cgi.to_script=%d, fd=%d, id=%d \n", __func__, __LINE__,
                                        resp->cgi.to_script, fd, resp->id);
                resp_500(resp);
                return;
            }
        }
        else
        {
            if (resp->cgi.fd != fd)
            {
                print_err(resp, "<%s:%d> Error cgi.fd=%d, fd=%d, 0x%02X,  id=%d \n", __func__, __LINE__,
                                        resp->cgi.fd, fd, revents, resp->id);
                resp_502(resp);
                return;
            }
        }

        if (revents == POLLOUT)
        {
            int ret = cgi_stdin(resp, fd);
            if (ret == ERR_TRY_AGAIN)
            {
                print_err(resp, "<%s:%d> Error cgi_stdin ERR_TRY_AGAIN, id=%d \n", __func__, __LINE__, resp->id);
                return;
            }
            else if (ret < 0)
            {
                print_err(resp, "<%s:%d> Error cgi_stdin()=%d\n", __func__, __LINE__, ret);
                resp_502(resp);
                return;
            }
        }
        else if (revents)
        {
            print_err(resp, "<%s:%d> Error events/revents=0x%02X/0x%02X, fd=%d,   id=%d \n", __func__, __LINE__,
                    events, revents, fd, resp->id);
            resp_502(resp);
        }
    }
    else if (resp->cgi_status == CGI_STDOUT)
    {
        if (resp->cgi_type <= PHPCGI)
        {
            if (resp->cgi.from_script != fd)
            {
                print_err(resp, "<%s:%d> Error cgi.from_script=%d, fd=%d, 0x%02X,  id=%d \n", __func__, __LINE__,
                                        resp->cgi.from_script, fd, revents, resp->id);
                resp_502(resp);
                return;
            }
        }
        else
        {
            if (resp->cgi.fd != fd)
            {
                print_err(resp, "<%s:%d> Error cgi.fd=%d, fd=%d, 0x%02X,  id=%d \n", __func__, __LINE__,
                                        resp->cgi.fd, fd, revents, resp->id);
                resp_502(resp);
                return;
            }
        }

        if (revents & POLLIN)
        {
            if (resp->html.size() >= conf->DataBufSize)
                return;
            int ret = cgi_stdout(resp, fd);
            if (ret == ERR_TRY_AGAIN)
            {
                print_err(resp, "<%s:%d> cgi_stdout()=ERR_TRY_AGAIN, id=%d \n", __func__, __LINE__, resp->id);
            }
            else if (ret < 0)
            {
                print_err(resp, "<%s:%d> Error cgi_stdout()=%d, id=%d \n", __func__, __LINE__, ret, resp->id);
                resp_502(resp);
            }
            else if (ret == 0)
            {
                if (resp->html.size())
                    return;
                if (resp->cgi_type <= PHPCGI)
                {
                    close(resp->cgi.from_script);
                    resp->cgi.from_script = -1;
                }

                set_frame_data(resp, 0, FLAG_END_STREAM);
                resp->cgi.cgi_end = true;
            }
            else
            {
                if ((!resp->send_headers) && (!resp->create_headers))
                {
                    const char *p1 = resp->html.ptr(), *p = NULL;
                    for (int i = 0; i < resp->html.size(); ++i)
                    {
                        if (*(p1++) == '\n')
                        {
                            if (*(p1) == '\r')
                            {
                                p1++;
                                if (*(p1) == '\n')
                                {
                                    p1++;
                                    p = p1;
                                    break;
                                }
                            }
                            else if (*(p1) == '\n')
                            {
                                p1++;
                                p = p1;
                                break;
                            }
                        }
                    }

                    if (p)
                    {
                        const char *p3 = NULL;
                        if ((p3 = strstr_case(resp->html.ptr(), "Status:")))
                        {
                            sscanf(p3 + 7, "%d", &resp->status);
                            if (resp->status == RS204)
                            {
                                resp_204(resp);
                                set_frame_data(resp, 0, FLAG_END_STREAM);

                                if (resp->cgi_type <= PHPCGI)
                                {
                                    if (resp->cgi.from_script > 0)
                                    {
                                        close(resp->cgi.from_script);
                                        resp->cgi.from_script = -1;
                                    }
                                }

                                resp->cgi.cgi_end = true;
                                return;
                            }
                        }

                        if ((p3 = strstr_case(resp->html.ptr(), "Content-Type:")))
                        {
                            char cont_type[64] = "NO";
                            int j = 0;
                            for (int i = 0; i < 64; ++i)
                            {
                                char ch = *(p3 + 13 + i);
                                if ((ch == ' ') && (j == 0))
                                    continue;
                                else if ((ch == '\r') || (ch == '\n'))
                                    break;
                                else
                                    cont_type[j++] = ch;
                            }

                            cont_type[j] = 0;
                            if (conf->PrintDebugMsg)
                                print_err(resp, "<%s:%d> Content-Type: %s, id=%d \n", __func__, __LINE__, cont_type, resp->id);
                            set_frame_headers(resp);
                            add_header(resp, 8, status_resp(resp->status));
                            add_header(resp, 33, get_time().c_str());
                            add_header(resp, 31, cont_type);
                            resp->create_headers = true;
                            resp->html.set_offset(p - resp->html.ptr());
                        }
                        else
                        {
                            resp_502(resp);
                        }
                    }
                }
            }
        }
        else if (revents)
        {
            if (resp->headers.size() || resp->data.size() || resp->html.size_remain())
                return;

            if (resp->cgi_type <= PHPCGI)
            {
                close(resp->cgi.from_script);
                resp->cgi.from_script = -1;
            }

            resp->html.init();
            if (resp->send_headers)
                set_frame_data(resp, 0, FLAG_END_STREAM);
            else
            {
                resp_500(resp);
                return;
            }
            //print_err(resp, "<%s:%d> revents=0x%02X, html.size_remain()=%d, id=%d \n", __func__, __LINE__, revents, resp->html.size_remain(), resp->id);
            resp->cgi.cgi_end = true;
        }
    }
}
