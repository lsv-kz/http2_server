#include "main.h"

using namespace std;

//======================================================================
string get_time()
{
    struct tm t;
    char s[40];
    time_t now = time(NULL);

    gmtime_r(&now, &t);
    strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %Z", &t);
    return s;
}
//======================================================================
string get_time(time_t now)
{
    struct tm t;
    char s[40];

    gmtime_r(&now, &t);
    strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %Z", &t);
    return s;
}
//======================================================================
string log_time()
{
    struct tm t;
    char s[40];
    time_t now = time(NULL);

    localtime_r(&now, &t);
    strftime(s, sizeof(s), "%d/%b/%Y:%H:%M:%S %Z", &t);
    return s;
}
//======================================================================
string log_time(time_t now)
{
    struct tm t;
    char s[40];

    localtime_r(&now, &t);
    strftime(s, sizeof(s), "%d/%b/%Y:%H:%M:%S %Z", &t);
    return s;
}
//======================================================================
const char *strstr_case(const char *s1, const char *s2)
{
    const char *p1, *p2;
    char c1, c2;

    if (!s1 || !s2) return NULL;
    if (*s2 == 0) return s1;

    int diff = ('a' - 'A');

    for (; ; ++s1)
    {
        c1 = *s1;
        if (!c1) break;
        c2 = *s2;
        c1 += (c1 >= 'A') && (c1 <= 'Z') ? diff : 0;
        c2 += (c2 >= 'A') && (c2 <= 'Z') ? diff : 0;
        if (c1 == c2)
        {
            p1 = s1++;
            p2 = s2 + 1;

            for (; ; ++s1, ++p2)
            {
                c2 = *p2;
                if (!c2) return p1;

                c1 = *s1;
                if (!c1) return NULL;

                c1 += (c1 >= 'A') && (c1 <= 'Z') ? diff : 0;
                c2 += (c2 >= 'A') && (c2 <= 'Z') ? diff : 0;
                if (c1 != c2)
                    break;
            }
        }
    }

    return NULL;
}
//======================================================================
int strlcmp_case(const char *s1, const char *s2, int len)
{
    char c1, c2;

    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    int diff = ('a' - 'A');

    for (; len > 0; --len, ++s1, ++s2)
    {
        c1 = *s1;
        c2 = *s2;
        if (!c1 && !c2) return 0;

        c1 += (c1 >= 'A') && (c1 <= 'Z') ? diff : 0;
        c2 += (c2 >= 'A') && (c2 <= 'Z') ? diff : 0;

        if (c1 != c2) return (c1 - c2);
    }

    return 0;
}
//======================================================================
const char *get_str_frame_type(FRAME_TYPE t)
{
    switch (t)
    {
        case DATA:
            return "DATA";
        case HEADERS:
            return "HEADERS";
        case PRIORITY:
            return "PRIORITY";
        case RST_STREAM:
            return "RST_STREAM";
        case SETTINGS:
            return "SETTINGS";
        case PUSH_PROMISE:
            return "PUSH_PROMISE";
        case PING:
            return "PING";
        case GOAWAY:
            return "GOAWAY";
        case WINDOW_UPDATE:
            return "WINDOW_UPDATE";
        case CONTINUATION:
            return "CONTINUATION";
        case ALTSVC:
            return "ALTSVC";
        case ORIGIN:
            return "ORIGIN";
        case CACHE_DIGEST:
            return "CACHE_DIGEST";
    }

    return "?";
}
//======================================================================
const char *get_str_operation(OPERATION_HTTP2 n)
{
    switch (n)
    {
        case SSL_ACCEPT:
            return "SSL_ACCEPT";
        case PREFACE_MESSAGE:
            return "PREFACE_MESSAGE";
        case SEND_SETTINGS:
            return "SEND_SETTINGS";
        case WORK_STREAM:
            return "WORK_STREAM";
        case SSL_SHUTDOWN:
            return "SSL_SHUTDOWN";
    }

    return "?";
}
//======================================================================
const char *get_cgi_type(CGI_TYPE n)
{
    switch (n)
    {
        case CGI:
            return "CGI";
        case PHPCGI:
            return "PHPCGI";
        case PHPFPM:
            return "PHPFPM";
        case FASTCGI:
            return "FASTCGI";
        case SCGI:
            return "SCGI";
    }

    return "?";
}
//======================================================================
const char *content_type(const char *s)
{
    const char *p = strrchr(s, '.');

    if (!p)
        goto end;

    //       video
    if (!strlcmp_case(p, ".ogv", 4))
        return "video/ogg";
    else if (!strlcmp_case(p, ".mp4", 4))
        return "video/mp4";
    else if (!strlcmp_case(p, ".avi", 4))
        return "video/x-msvideo";
    else if (!strlcmp_case(p, ".mov", 4))
        return "video/quicktime";
    else if (!strlcmp_case(p, ".mkv", 4))
        return "video/x-matroska";
    else if (!strlcmp_case(p, ".flv", 4))
        return "video/x-flv";
    else if (!strlcmp_case(p, ".mpeg", 5) || !strlcmp_case(p, ".mpg", 4))
        return "video/mpeg";
    else if (!strlcmp_case(p, ".asf", 4))
        return "video/x-ms-asf";
    else if (!strlcmp_case(p, ".wmv", 4))
        return "video/x-ms-wmv";
    else if (!strlcmp_case(p, ".swf", 4))
        return "application/x-shockwave-flash";
    else if (!strlcmp_case(p, ".3gp", 4))
        return "video/video/3gpp";

    //       sound
    else if (!strlcmp_case(p, ".mp3", 4))
        return "audio/mpeg";
    else if (!strlcmp_case(p, ".wav", 4))
        return "audio/x-wav";
    else if (!strlcmp_case(p, ".ogg", 4))
        return "audio/ogg";
    else if (!strlcmp_case(p, ".pls", 4))
        return "audio/x-scpls";
    else if (!strlcmp_case(p, ".aac", 4))
        return "audio/aac";
    else if (!strlcmp_case(p, ".aif", 4))
        return "audio/x-aiff";
    else if (!strlcmp_case(p, ".ac3", 4))
        return "audio/ac3";
    else if (!strlcmp_case(p, ".voc", 4))
        return "audio/x-voc";
    else if (!strlcmp_case(p, ".flac", 5))
        return "audio/flac";
    else if (!strlcmp_case(p, ".amr", 4))
        return "audio/amr";
    else if (!strlcmp_case(p, ".au", 3))
        return "audio/basic";

    //       image
    else if (!strlcmp_case(p, ".gif", 4))
        return "image/gif";
    else if (!strlcmp_case(p, ".svg", 4) || !strlcmp_case(p, ".svgz", 5))
        return "image/svg+xml";
    else if (!strlcmp_case(p, ".png", 4))
        return "image/png";
    else if (!strlcmp_case(p, ".ico", 4))
        return "image/vnd.microsoft.icon";
    else if (!strlcmp_case(p, ".jpeg", 5) || !strlcmp_case(p, ".jpg", 4))
        return "image/jpeg";
    else if (!strlcmp_case(p, ".djvu", 5) || !strlcmp_case(p, ".djv", 4))
        return "image/vnd.djvu";
    else if (!strlcmp_case(p, ".tiff", 5))
        return "image/tiff";
    //       text
    else if (!strlcmp_case(p, ".txt", 4))
        return "text/plain; charset=UTF-8";
    else if (!strlcmp_case(p, ".html", 5) || !strlcmp_case(p, ".htm", 4) || !strlcmp_case(p, ".shtml", 6))
        return "text/html";
    else if (!strlcmp_case(p, ".css", 4))
        return "text/css";

    //       application
    else if (!strlcmp_case(p, ".pdf", 4))
        return "application/pdf";
    else if (!strlcmp_case(p, ".gz", 3))
        return "application/gzip";
end:
    return NULL;
}
//======================================================================
int clean_path(char *path, int len)
{
    int i = 0, j = 0, level_dir = 0;
    char ch;
    char prev_ch = '\0';
    int index_slash[64] = {0};

    while ((ch = *(path + j)) && (len > 0))
    {
        if (prev_ch == '/')
        {
            if (ch == '/')
            {
                --len;
                ++j;
                continue;
            }

            switch (len)
            {
                case 1:
                    if (ch == '.')
                    {
                        --len;
                        ++j;
                        continue;
                    }
                    break;
                case 2:
                    if (!memcmp(path + j, "..", 2))
                    {
                        if (level_dir > 1)
                        {
                            j += 2;
                            len -= 2;
                            --level_dir;
                            i = index_slash[level_dir];
                            continue;
                        }
                        else
                        {
                            return -RS400;
                        }
                    }
                    else if (!memcmp(path + j, "./", 2))
                    {
                        len -= 2;
                        j += 2;
                        continue;
                    }
                    break;
                case 3:
                    if (!memcmp(path + j, "../", 3))
                    {
                        if (level_dir > 1)
                        {
                            j += 3;
                            len -= 3;
                            --level_dir;
                            i = index_slash[level_dir];
                            continue;
                        }
                        else
                        {
                            return -RS400;
                        }
                    }
                    else if (!memcmp(path + j, "./.", 3))
                    {
                        len -= 3;
                        j += 3;
                        continue;
                    }
                    else if (!memcmp(path + j, ".//", 3))
                    {
                        len -= 3;
                        j += 3;
                        continue;
                    }
                    break;
                default:
                    if (!memcmp(path + j, "../", 3))
                    {
                        if (level_dir > 1)
                        {
                            j += 3;
                            len -= 3;
                            --level_dir;
                            i = index_slash[level_dir];
                            continue;
                        }
                        else
                        {
                            return -RS400;
                        }
                    }
                    else if (!memcmp(path + j, "...", 3))
                    {
                        break;
                    }
                    else if (!memcmp(path + j, "./", 2))
                    {
                        len -= 2;
                        j += 2;
                        continue;
                    }
                    else if (ch == '.')
                    {
                        return -RS404;
                    }
            }
        }

        *(path + i) = ch;
        ++i;
        ++j;
        --len;
        prev_ch = ch;
        if (ch == '/')
        {
            if (level_dir >= (int)(sizeof(index_slash)/sizeof(int)))
                return -RS404;
            ++level_dir;
            index_slash[level_dir] = i;
        }
    }

    *(path + i) = 0;
    return i;
}
//======================================================================
long long file_size(const char *s)
{
    struct stat st;

    if (!stat(s, &st))
        return st.st_size;
    else
        return -1;
}
//======================================================================
void set_frame(Response *resp, char *s, int len, int type, HTTP2_FLAGS flags, int id)
{
    s[0] = (len>>16) & 0xff;
    s[1] = (len>>8) & 0xff;
    s[2] = len & 0xff;

    s[3] = (unsigned char)type;
    s[4] = (unsigned char)flags;

    s[5] = (id>>24) & 0x7f;
    s[6] = (id>>16) & 0xff;
    s[7] = (id>>8) & 0xff;
    s[8] = id & 0xff;

    if (type == HEADERS)
        resp->send_ready |= FRAME_HEADERS_READY;
    else if (type == DATA)
        resp->send_ready |= FRAME_DATA_READY;
}
//======================================================================
void set_frame_headers(Response *resp)
{
    int id = resp->id;
    resp->headers.cpy("\0\0\0\1\4\0\0\0\0", 9);

    int len = resp->headers.size() - 9;
    resp->headers.set_byte((len>>16) & 0xff, 0);
    resp->headers.set_byte((len>>8) & 0xff, 1);
    resp->headers.set_byte(len & 0xff, 2);

    resp->headers.set_byte((id>>24) & 0x7f, 5);
    resp->headers.set_byte((id>>16) & 0xff, 6);
    resp->headers.set_byte((id>>8) & 0xff, 7);
    resp->headers.set_byte(id & 0xff, 8);
    resp->send_ready |= FRAME_HEADERS_READY;
}
//======================================================================
void add_header(Response *resp, int ind)
{
    if ((ind >= 8) && (ind <= 14))
        resp->status = atoi(static_tab[ind][1]);
    char s[8];
    s[0] = (ind | 0x80);
    resp->headers.cat(s, 1);
    int len = resp->headers.size() - 9;
    resp->headers.set_byte((len>>16) & 0xff, 0);
    resp->headers.set_byte((len>>8) & 0xff, 1);
    resp->headers.set_byte(len & 0xff, 2);
}
//======================================================================
void add_header(Response *resp, int ind, const char *val)
{
    if ((ind >= 8) && (ind <= 14))
        resp->status = atoi(val);
    int len = (int)strlen(val);
    char s[8];
    s[0] = (ind | 0x40);
    s[1] = (char)len;
    resp->headers.cat(s, 2);
    resp->headers.cat(val, len);
    len = resp->headers.size() - 9;
    resp->headers.set_byte((len>>16) & 0xff, 0);
    resp->headers.set_byte((len>>8) & 0xff, 1);
    resp->headers.set_byte(len & 0xff, 2);
}
//======================================================================
void set_frame_window_update(Response *resp, int len)
{
    int id = resp->id;
    char s[] = "\x00\x00\x04\x08\x00\x00\x00\x00\x00"  // 0-8
               "\x00\x00\x00\x00";                     // 9-12

    resp->frame_win_update.cpy(s, 13);

    resp->frame_win_update.set_byte((len>>24) & 0xff, 9);
    resp->frame_win_update.set_byte((len>>16) & 0xff, 10);
    resp->frame_win_update.set_byte((len>>8) & 0xff, 11);
    resp->frame_win_update.set_byte(len & 0xff, 12);

    resp->frame_win_update.set_byte((id>>24) & 0x7f, 5);
    resp->frame_win_update.set_byte((id>>16) & 0xff, 6);
    resp->frame_win_update.set_byte((id>>8) & 0xff, 7);
    resp->frame_win_update.set_byte(id & 0xff, 8);

    resp->send_ready |= FRAME_WINUPDATE_STREAM_READY;
}
//======================================================================
void set_frame_window_update(Connect *con, int len)
{
    con->h2.frame_win_update.cpy("\x00\x00\x04\x08\x00\x00\x00\x00\x00"  // 0-8
                               "\x00\x00\x00\x00", 13);              // 9-12

    con->h2.frame_win_update.set_byte((len>>24) & 0xff, 9);
    con->h2.frame_win_update.set_byte((len>>16) & 0xff, 10);
    con->h2.frame_win_update.set_byte((len>>8) & 0xff, 11);
    con->h2.frame_win_update.set_byte(len & 0xff, 12);

    con->h2.send_ready |= FRAME_WINUPDATE_CONNECT_READY;
}
//======================================================================
int send_rst_stream(Connect *c, int id)
{
     c->h2.frame.cpy("\0\0\4\3\0\0\0\0\0"
                         "\0\0\0\0", 13);
    c->h2.frame.set_byte((id>>24) & 0x7f, 5);
    c->h2.frame.set_byte((id>>16) & 0xff, 6);
    c->h2.frame.set_byte((id>>8) & 0xff, 7);
    c->h2.frame.set_byte(id & 0xff, 8);

    if (write_to_client(c, c->h2.frame.ptr(), c->h2.frame.size(), id) <= 0)
    {
        print_err(c, "<%s:%d> Error send frame RST_STREAM\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}
//======================================================================
void set_frame_data(Response *resp, int len, int flag)
{
    int id = resp->id;
    resp->data.cpy("\0\0\0\0\0\0\0\0\0", 9);
    resp->data.set_byte(flag, 4);
    resp->data.set_byte((len>>16) & 0xff, 0);
    resp->data.set_byte((len>>8) & 0xff, 1);
    resp->data.set_byte(len & 0xff, 2);

    resp->data.set_byte((id>>24) & 0x7f, 5);
    resp->data.set_byte((id>>16) & 0xff, 6);
    resp->data.set_byte((id>>8) & 0xff, 7);
    resp->data.set_byte(id & 0xff, 8);

    resp->send_ready |= FRAME_DATA_READY;
}
//======================================================================
int set_frame_data(Connect *con, Response *resp)
{
    if (resp->content == DYN_PAGE)
    {
        if ((resp->create_headers == false) && (resp->send_headers == false))
            return 0;
        resp->data.init();
        if (resp->html.size() > resp->html.get_offset())
        {
            int offs = resp->html.get_offset();
            set_frame_data(resp, resp->html.size() - offs, 0);
            resp->data.cat(resp->html.ptr() + offs, resp->html.size() - offs);
            resp->html.init();
        }

        if (resp->cgi.cgi_end)
        {
            char s[] = "\0\0\0\0\1\0\0\0\0";
            s[5] = (resp->id>>24) & 0x7f;
            s[6] = (resp->id>>16) & 0xff;
            s[7] = (resp->id>>8) & 0xff;
            s[8] = resp->id & 0xff;
            resp->data.cat(s, 9);
            resp->send_ready |= FRAME_DATA_READY;
        }
    }
    else
    {
        resp->data.init();
        long data_len = 0;
        long win_update = (con->h2.window_update > resp->window_update) ? resp->window_update : con->h2.window_update;
        if (win_update <= 0)
        {
            return 0;
        }

        if (resp->content == REGFILE)
        {
            if (resp->send_cont_length > conf->DataBufSize)
            {
                data_len = conf->DataBufSize;
            }
            else
            {
                data_len = (int)resp->send_cont_length;
            }

            if (data_len > win_update)
            {
                //print_err(con, "<%s:%d> !!! data_len(%ld) > win_update(%ld), id=%d\n", __func__, __LINE__, data_len, win_update, resp->id);
                data_len = win_update;
            }

            resp->send_cont_length -= data_len;

            char buf[conf->DataBufSize];
            int n = read(resp->fd, buf, data_len);
            if (n <= 0)
            {
                print_err(con, "<%s:%d> Error read(fd=%d)=%d: %s, id=%d\n", __func__, __LINE__, resp->fd, n, strerror(errno), resp->id);
                close(resp->fd);
                resp->fd = -1;
                return -1;
            }

            int flag = 0;
            if (resp->send_cont_length == 0)
            {
                close(resp->fd);
                resp->fd = -1;
                flag = FLAG_END_STREAM;
            }

            set_frame_data(resp, data_len, flag);
            resp->data.cat(buf, data_len);
        }
        else if (resp->content == DIRECTORY)
        {
            if (resp->send_cont_length > conf->DataBufSize)
            {
                data_len = conf->DataBufSize;
            }
            else
            {
                data_len = (int)resp->send_cont_length;
            }

            if (data_len > win_update)
                data_len = win_update;

            resp->send_cont_length -= data_len;

            if ((resp->html.get_offset() + data_len) > resp->html.size())
            {
                print_err(con, "<%s:%d> Error\n", __func__, __LINE__);
                return -1;
            }

            int flag = (resp->send_cont_length > 0) ? 0 : FLAG_END_STREAM;
            set_frame_data(resp, data_len, flag);
            resp->data.cat(resp->html.ptr_remain(), data_len);
            resp->html.offset_add(data_len);
        }
    }

    return 1;
}
//======================================================================
int set_response(Connect *con, Response *resp)
{
    resp->send_bytes = 0;

    string path = ".";
    decode(resp->path.c_str(), resp->path.size(), resp->decode_path);
    //--------------------------
    int len = 0;
    const char *p = strchr(resp->decode_path.c_str(), '?');
    if (p)
    {
        len = p - resp->decode_path.c_str();
        if (len > 1024)
        {
            return -1;
        }

        resp->cgi.query_string = p + 1;
    }
    else
        len = resp->decode_path.size();

    memcpy(resp->uri, resp->decode_path.c_str(), len);
    resp->uri[len] = 0;
    int err = clean_path(resp->uri, len);
    if (err <= 0)
    {
        resp_400(resp);
        return 0;
    }
    //-------------------------------
    if (!strncmp(resp->decode_path.c_str(), "/cgi-bin/", 9))
    {
        resp->content = DYN_PAGE;
        resp->cgi.cgi_type = CGI;
    }
    else if (strstr(resp->decode_path.c_str(), ".php"))
    {
        resp->content = DYN_PAGE;
        if (conf->UsePHP == "php-cgi")
            resp->cgi.cgi_type = PHPCGI;
        else if (conf->UsePHP == "php-fpm")
            resp->cgi.cgi_type = PHPFPM;
        else
        {
            resp_500(resp);
            return 0;
        }
    }
    else
    {
        path += resp->decode_path;
        resp->content = get_content_type(path.c_str());
    }

    if (resp->content == REGFILE)//-----------------------------------
    {
        // ----- file -----
        resp->file_size = (long long)file_size(path.c_str());
        if (resp->file_size < 0)
        {
            print_err(con, "<%s:%d> Error file_size(%s)\n", __func__, __LINE__, path.c_str());
            resp_500(resp);
            return 0;
        }

        if (resp->range.size())
        {
            if (parse_range(resp->range.c_str(), resp->file_size, &resp->offset, &resp->send_cont_length))
            {
                print_err(con, "<%s:%d> Error parse_range(%s)\n", __func__, __LINE__, resp->range.c_str());
                resp_400(resp);
                return 0;
            }
        }
        else
            resp->send_cont_length = resp->file_size;
        //----------- frame headers ----------------
        set_frame_headers(resp);
        if (resp->range.size())
            add_header(resp, 10);
        else
            add_header(resp, 8);
        add_header(resp, 33, get_time().c_str());
        const char  *cont_type = content_type(resp->path.c_str());
        if (cont_type)
            add_header(resp, 31, cont_type);

        char s[128];
        snprintf(s, sizeof(s), "%lld", resp->send_cont_length);
        add_header(resp, 28, s);
        add_header(resp, 18, "bytes");
        add_header(resp, 24, "no-cache, no-store, must-revalidate");

        if (resp->file_size == 0)
        {
            set_frame_data(resp, 0, FLAG_END_STREAM);
            return 0;
        }

        if (resp->range.size())
        {
            char s[128];
            snprintf(s, sizeof(s), "bytes %lld-%lld/%lld", resp->offset, resp->offset + resp->send_cont_length - 1, resp->file_size);
            resp->file_size = resp->send_cont_length;
            add_header(resp, 30, s);
        }

        resp->create_headers = true;
        resp->fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (resp->fd == -1)
        {
            print_err(con, "<%s:%d> Error open(): %s\n", __func__, __LINE__, strerror(errno));
            if (errno == EACCES)
                resp_403(resp);
            else if (errno == ENOENT)
                resp_404(resp);
            else
                resp_500(resp);
            return 0;
        }

        if (resp->offset > 0)
            lseek(resp->fd, resp->offset, SEEK_SET);
    }
    else if (resp->content == DIRECTORY)
    {
        if (resp->decode_path[resp->decode_path.size() - 1] != '/')
        {
            set_frame_headers(resp);
            add_header(resp, 8, "301");
            add_header(resp, 33, get_time().c_str());
            add_header(resp, 46, resp->path.append("/").c_str());
            add_header(resp, 31, "text/plain");
            resp->create_headers = true;

            ByteArray smg;
            smg.cpy_str("301 Moved Permanently\n");
            smg.cat(resp->path.c_str(), resp->path.size());

            resp->send_cont_length = smg.size();
            char s[128];
            snprintf(s, sizeof(s), "%lld", resp->send_cont_length);
            add_header(resp, 28, s);

            set_frame_data(resp, smg.size(), FLAG_END_STREAM);
            resp->data.cat(smg.ptr(), smg.size());
            return 0;
        }

        int err = index_dir(con, path, resp);
        if (err)
        {
            print_err(con, "<%s:%d> Error index_dir(): %d\n", __func__, __LINE__, -err);
            return -1;
        }
        //------------- headers frame --------------
        set_frame_headers(resp);
        add_header(resp, 8);
        add_header(resp, 33, get_time().c_str());
        resp->create_headers = true;
    }
    else if (resp->content == DYN_PAGE)
    {
        resp->data.init();
        resp->cgi.op = CGI_CREATE;
    }
    else
    {
        if (iscgi(con, resp))
        {
            resp->data.init();
            resp->cgi.op = CGI_CREATE;
        }
        else
        {
            print_err(con, "<%s:%d> Error: CONTENT_TYPE %s, create_headers=%d, send_headers=%d\n", __func__, __LINE__, path.c_str(), resp->create_headers, resp->send_headers);
            resp_404(resp);
        }
    }

    return 0;
}
//======================================================================
CONTENT_TYPE get_content_type(const char *path)
{
    struct stat st;
    int ret = lstat(path, &st);
    if (ret == -1)
    {
        return ERROR_TYPE;
    }

    if (S_ISDIR(st.st_mode))
        return DIRECTORY;
    else if (S_ISREG(st.st_mode))
        return REGFILE;
    else
        return ERROR_TYPE;
}
//======================================================================
int parse_range(const char *s, long long file_size, long long *offset, long long *content_length)
{
    const char *p = strchr(s, '=');
    if (p == NULL)
        return -1;
    if (*(p + 1) == '-')
    {
        if (strlen(p + 2) == 0)
            return -1;
        sscanf(p + 2, "%lld", content_length);
        if (*content_length > file_size)
            return -1;
        *offset = file_size - *content_length;
    }
    else
    {
        sscanf(p + 1, "%lld", offset);
        if (*offset >= file_size)
            return -1;
        p = strchr(p + 1, '-');
        if (p == NULL)
            return -1;
        if (strlen(p + 1) == 0)
            *content_length = file_size - *offset;
        else
        {
            long long end = 0;
            if (sscanf(p + 1, "%lld", &end) != 1)
                return -1;
            *content_length = end + 1 - *offset;
        }
    }
    return 0;
}
//======================================================================
void resp_200(Response *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8);
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "200 OK";
    int len = strlen(err);
    resp->send_cont_length = 0;
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_204(Response *resp)
{
    resp->content = ERROR_TYPE;
    set_frame_headers(resp);
    add_header(resp, 8, "204");
    add_header(resp, 33, get_time().c_str());
    add_header(resp, 28, "0");
    resp->create_headers = true;

    resp->send_cont_length = 0;
    set_frame_data(resp, 0, FLAG_END_STREAM);
}
//======================================================================
void resp_400(Response *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 12);
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "400 Bad Request";
    int len = strlen(err);
    resp->send_cont_length = 0;
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_403(Response *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "403");
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "403 Forbidden";
    int len = strlen(err);
    resp->send_cont_length = 0;
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_404(Response *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 13);
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "404 Not Found";
    int len = strlen(err);
    resp->send_cont_length = 0;
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_500(Response *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 14);
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "500 Internal Server Error";
    int len = strlen(err);
    resp->send_cont_length = 0;
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_502(Response *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "502");
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "502 Bad Gateway";
    int len = strlen(err);
    resp->send_cont_length = 0;
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_504(Response *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "504");
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "504 Gateway Time-out";
    int len = strlen(err);
    resp->send_cont_length = 0;
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
const char *status_resp(int st)
{
    switch (st)
    {
        case 0:
            return "";
        case RS101:
            return "101 Switching Protocols";
        case RS200:
            return "200 OK";
        case RS204:
            return "204 No Content";
        case RS206:
            return "206 Partial Content";
        case RS301:
            return "301 Moved Permanently";
        case RS302:
            return "302 Moved Temporarily";
        case RS400:
            return "400 Bad Request";
        case RS401:
            return "401 Unauthorized";
        case RS402:
            return "402 Payment Required";
        case RS403:
            return "403 Forbidden";
        case RS404:
            return "404 Not Found";
        case RS405:
            return "405 Method Not Allowed";
        case RS406:
            return "406 Not Acceptable";
        case RS407:
            return "407 Proxy Authentication Required";
        case RS408:
            return "408 Request Timeout";
        case RS411:
            return "411 Length Required";
        case RS413:
            return "413 Request entity too large";
        case RS414:
            return "414 Request-URI Too Large";
        case RS416:
            return "416 Range Not Satisfiable";
        case RS429:
            return "429 Too Many Requests";
        case RS500:
            return "500 Internal Server Error";
        case RS501:
            return "501 Not Implemented";
        case RS502:
            return "502 Bad Gateway";
        case RS503:
            return "503 Service Unavailable";
        case RS504:
            return "504 Gateway Time-out";
        case RS505:
            return "505 HTTP Version not supported";
        default:
            return "500 Internal Server Error";
    }
    return "";
}

