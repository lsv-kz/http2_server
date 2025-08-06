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
int strcmp_case(const char *s1, const char *s2)
{
    char c1, c2;

    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    int diff = ('a' - 'A');

    for (; (*s1) && (*s2); ++s1, ++s2)
    {
        c1 = *s1;
        c2 = *s2;
        if (!c1 && !c2) return 0;

        c1 += (c1 >= 'A') && (c1 <= 'Z') ? diff : 0;
        c2 += (c2 >= 'A') && (c2 <= 'Z') ? diff : 0;

        if (c1 != c2) return (c1 - c2);
    }

    return (*s1 - *s2);
}
//======================================================================
int pow_(int x, int y)
{
    if (y < 0)
        return -1;
    int m = 1;
    for (int i = 0; i < y; ++i)
        m = m * x;
    return m;
}
//======================================================================
const char *get_str_frame_type(FRAME_TYPE t)
{
    switch (t)
    {
        case DATA: // 0
            return "DATA";
        case HEADERS: // 1
            return "HEADERS";
        case PRIORITY: // 2
            return "PRIORITY";
        case RST_STREAM: // 3
            return "RST_STREAM";
        case SETTINGS: // 4
            return "SETTINGS";
        case PUSH_PROMISE: // 5
            return "PUSH_PROMISE";
        case PING: // 6
            return "PING";
        case GOAWAY: // 7
            return "GOAWAY";
        case WINDOW_UPDATE: // 8
            return "WINDOW_UPDATE";
        case CONTINUATION: // 9
            return "CONTINUATION";
        case ALTSVC: // 10 (0xA)
            return "ALTSVC";
        case ORIGIN: // 12 (0xC)
            return "ORIGIN";
        case PRIORITY_UPDATE: // 16 (0x10)
            return "PRIORITY_UPDATE";
    }

    switch ((int)t)
    {
        case 11:
            return "11 (0XB)";
        case 13:
            return "13 (0xD)";
        case 14:
            return "14 (0xE)";
        case 15:
            return "15 (0xF)";
    }

    return "?";
}
//======================================================================
const char *get_str_operation(CONNECT_STATUS s)
{
    switch (s)
    {
        case SSL_ACCEPT:
            return "SSL_ACCEPT";
        case PREFACE_MESSAGE:
            return "PREFACE_MESSAGE";
        case RECV_SETTINGS:
            return "RECV_SETTINGS";
        case SEND_SETTINGS:
            return "SEND_SETTINGS";
        case PROCESSING_REQUESTS:
            return "PROCESSING_REQUESTS";
        case SSL_SHUTDOWN:
            return "SSL_SHUTDOWN";
    }

    return "?";
}
//======================================================================
const char *get_cgi_type(CGI_TYPE t)
{
    switch (t)
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
const char *get_str_error(int err)
{
    switch (err)
    {
        case NO_ERROR: // 0
            return "NO_ERROR";
        case PROTOCOL_ERROR: // 1
            return "PROTOCOL_ERROR";
        case INTERNAL_ERROR: // 2
            return "INTERNAL_ERROR";
        case FLOW_CONTROL_ERROR: // 3
            return "FLOW_CONTROL_ERROR";
        case SETTINGS_TIMEOUT: // 4
            return "SETTINGS_TIMEOUT";
        case STREAM_CLOSED: // 5
            return "STREAM_CLOSED";
        case FRAME_SIZE_ERROR: // 6
            return "FRAME_SIZE_ERROR";
        case REFUSED_STREAM: // 7
            return "REFUSED_STREAM";
        case CANCEL: // 8
            return "CANCEL";
        case COMPRESSION_ERROR: // 9
            return "COMPRESSION_ERROR";
        case CONNECT_ERROR: // 10 (0xA)
            return "CONNECT_ERROR";
        case ENHANCE_YOUR_CALM: // 11 (0xB)
            return "ENHANCE_YOUR_CALM";
        case INADEQUATE_SECURITY: // 12 (0xC)
            return "INADEQUATE_SECURITY";
        case HTTP_1_1_REQUIRED: // 13 (0xD)
            return "HTTP_1_1_REQUIRED";
    }

    switch ((int)err)
    {
        case 14:
            return "14 (0XE)";
        case 15:
            return "15 (0xF)";
        case 16:
            return "16 (0x10)";
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
int bytes_to_int(unsigned char prefix, int pref_len, const char *s, int *len, int size)
{
    int data = pow_(2, pref_len) - 1;
    if (prefix < data)
        data = prefix;
    else
    {
        unsigned char ch;
        for (int i = 0; (*len) < size; ++i)
        {
            ch = s[(*len)++];
            data = data + ((ch & 0x7f)<<(i*7));
            if (!(ch & 0x80))
                break;
        }
    }

    return data;
}
//======================================================================
int int_to_bytes(int data, int pref_len, ByteArray& buf)
{
    int ret = 0;

    if (data < (pow_(2, pref_len) - 1))
    {
        buf.cat((char)data);
        ++ret;
    }
    else
    {
        buf.cat(pow_(2, pref_len) - 1);
        ++ret;
        data = data - (pow_(2, pref_len) - 1);
        while (data > 128)
        {
            buf.cat(data % 128 + 128);
            ++ret;
            data = data / 128;
        }

        buf.cat((char)data);
        ++ret;
    }

    return ret;
}
//======================================================================
void set_frame(Stream *resp, char *s, int len, int type, HTTP2_FLAGS flags, int id)
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
}
//======================================================================
void set_frame_headers(Stream *resp)
{
    int id = resp->id;
    resp->headers.cpy("\0\0\0\1\4\0\0\0\0", 9);
    resp->headers.set_byte((id>>24) & 0x7f, 5);
    resp->headers.set_byte((id>>16) & 0xff, 6);
    resp->headers.set_byte((id>>8) & 0xff, 7);
    resp->headers.set_byte(id & 0xff, 8);
    if (resp->numReq == 1)
        resp->headers.cat(0x20);
}
//======================================================================
void add_header(Stream *resp, int ind)
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
void add_header(Stream *resp, int ind, const char *val)
{
    if ((ind >= 8) && (ind <= 14))
        resp->status = atoi(val);
    int len = (int)strlen(val);
    int_to_bytes(ind, 4, resp->headers);
    int_to_bytes(len, 7, resp->headers);
    resp->headers.cat(val, len);
    len = resp->headers.size() - 9;
    resp->headers.set_byte((len>>16) & 0xff, 0);
    resp->headers.set_byte((len>>8) & 0xff, 1);
    resp->headers.set_byte(len & 0xff, 2);
}
//======================================================================
void set_frame_window_update(Stream *resp, int len)
{
    int id = resp->id;
    char s[] = "\x00\x00\x04\x08\x00\x00\x00\x00\x00"  // 0-8
               "\x00\x00\x00\x00";                     // 9-12

    resp->frame_win_update.cpy(s, 13);

    resp->frame_win_update.set_byte((len>>24) & 0x7f, 9);
    resp->frame_win_update.set_byte((len>>16) & 0xff, 10);
    resp->frame_win_update.set_byte((len>>8) & 0xff, 11);
    resp->frame_win_update.set_byte(len & 0xff, 12);

    resp->frame_win_update.set_byte((id>>24) & 0x7f, 5);
    resp->frame_win_update.set_byte((id>>16) & 0xff, 6);
    resp->frame_win_update.set_byte((id>>8) & 0xff, 7);
    resp->frame_win_update.set_byte(id & 0xff, 8);
}
//======================================================================
void set_frame_window_update(Connect *con, int len)
{
    con->h2.frame_win_update.cpy("\x00\x00\x04\x08\x00\x00\x00\x00\x00"  // 0-8
                               "\x00\x00\x00\x00", 13);              // 9-12

    con->h2.frame_win_update.set_byte((len>>24) & 0x7f, 9);
    con->h2.frame_win_update.set_byte((len>>16) & 0xff, 10);
    con->h2.frame_win_update.set_byte((len>>8) & 0xff, 11);
    con->h2.frame_win_update.set_byte(len & 0xff, 12);
}
//======================================================================
void set_frame_goaway(Connect *con, HTTP2_ERRORS error)
{
    char buf[] = "\x0\x0\x0\x0\x0\x0\x0\x0";
    con->h2.goaway.cpy("\x0\x0\x8\x7\x0\x0\x0\x0\x0", 9);

    buf[4] = (unsigned char)((error>>24) & 0xff);
    buf[5] = (unsigned char)((error>>16) & 0xff);
    buf[6] = (unsigned char)((error>>8) & 0xff);
    buf[7] = (unsigned char)(error & 0xff);

    con->h2.goaway.cat(buf, 8);
}
//======================================================================
int set_rst_stream(Connect *c, int id, HTTP2_ERRORS error)
{
    c->h2.close_stream(id);

    FrameRedySend *rf = NULL;
    rf = new(std::nothrow) FrameRedySend;
    if (!rf)
    {
        print_err(c, "<%s:%d> Error: %s\n", __func__, __LINE__, strerror(errno));
        return -1;
    }

    c->h2.push_to_list(rf);

    rf->id = id;

    rf->frame.cpy("\0\0\4\3\0\0\0\0\0"
                         "\0\0\0\0", 13);
    rf->frame.set_byte((id>>24) & 0x7f, 5);
    rf->frame.set_byte((id>>16) & 0xff, 6);
    rf->frame.set_byte((id>>8) & 0xff, 7);
    rf->frame.set_byte(id & 0xff, 8);

    rf->frame.set_byte((error>>24) & 0xff, 9);
    rf->frame.set_byte((error>>16) & 0xff, 10);
    rf->frame.set_byte((error>>8) & 0xff, 11);
    rf->frame.set_byte(error & 0xff, 12);

    return 0;
}
//======================================================================
void set_frame_data(Stream *resp, int len, int flag)
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
}
//======================================================================
int set_frame_data(Connect *con, Stream *resp)
{
    resp->data.init();
    long data_len = 0;
    long min_window_size = (con->h2.connect_window_size > resp->stream_window_size) ? resp->stream_window_size : con->h2.connect_window_size;
    if (min_window_size <= 0)
    {
        print_err(resp, "<%s:%d> !!! connect_window_size=%ld, stream_window_size=%ld, id=%d \n", __func__, __LINE__, con->h2.connect_window_size, resp->stream_window_size, resp->id);
        return 0;
    }

    if (resp->content == DYN_PAGE)
    {
        if (resp->send_headers == false)
            return 0;
        if (resp->html.size_remain())
        {
            int len = resp->html.size_remain();
            if (len > conf->DataBufSize)
                len = conf->DataBufSize;
            set_frame_data(resp, len, 0);
            resp->data.cat(resp->html.ptr_remain(), len);
            resp->html.set_offset(len);
            if (resp->html.size_remain() == 0)
                resp->html.init();
        }
        else
        {
            if (resp->cgi.cgi_end)
                set_frame_data(resp, 0, FLAG_END_STREAM);
            else
                return 0;
        }
    }
    else
    {
        if (resp->content == REGFILE)
        {
            if (resp->send_cont_length > conf->DataBufSize)
                data_len = conf->DataBufSize;
            else
                data_len = (int)resp->send_cont_length;

            char buf[16384];
            if (data_len > 0)
            {
                if (data_len > min_window_size)
                {
                    //print_err(con, "<%s:%d> !!! data_len(%ld) > min_window_size(%ld), id=%d \n", __func__, __LINE__, data_len, min_window_size, resp->id);
                    data_len = min_window_size;
                }

                int ret = read(resp->fd, buf, data_len);
                if (ret <= 0)
                {
                    print_err(con, "<%s:%d> Error read(fd=%d)=%d: %s, id=%d \n", __func__, __LINE__, resp->fd, ret, strerror(errno), resp->id);
                    close(resp->fd);
                    resp->fd = -1;
                    return -1;
                }

                data_len = ret;
            }

            resp->send_cont_length -= data_len;
            int flag = (resp->send_cont_length > 0) ? 0 : FLAG_END_STREAM;
            set_frame_data(resp, data_len, flag);
            if (data_len > 0)
                resp->data.cat(buf, data_len);
            if (resp->send_cont_length == 0)
            {
                close(resp->fd);
                resp->fd = -1;
            }
        }
        else if (resp->content == DIRECTORY)
        {
            if (resp->send_cont_length > conf->DataBufSize)
                data_len = conf->DataBufSize;
            else
                data_len = (int)resp->send_cont_length;

            if (data_len > min_window_size)
                data_len = min_window_size;

            if ((resp->html.get_offset() + data_len) > resp->html.size())
            {
                print_err(con, "<%s:%d> Error\n", __func__, __LINE__);
                return -1;
            }

            resp->send_cont_length -= data_len;
            int flag = (resp->send_cont_length > 0) ? 0 : FLAG_END_STREAM;
            set_frame_data(resp, data_len, flag);
            resp->data.cat(resp->html.ptr_remain(), data_len);
            resp->html.set_offset(data_len);
        }
    }

    return 1;
}
//======================================================================
int set_response(Connect *con, Stream *resp)
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
        if (len > (resp->size_uri - 1))
        {
            resp_414(resp);
            return 0;
        }

        resp->cgi.query_string = p + 1;
    }
    else
        len = resp->decode_path.size();

    if (len > (resp->size_uri - 1))
    {
        print_err(con, "<%s:%d> Error: 414 URI Too Long [%d], id=%d \n", __func__, __LINE__, len, resp->id);
        resp_414(resp);
        return 0;
    }

    memcpy(resp->uri, resp->decode_path.c_str(), len);
    resp->uri[len] = 0;
    int err = clean_path(resp->uri, len);
    if (err <= 0)
    {
        resp_400(resp);
        return 0;
    }

    if (resp->method == "POST")
    {
        if (resp->content_type.size() == 0)
        {
            print_err(con, "<%s:%d> Content-Type \?\n", __func__, __LINE__);
            resp_400(resp);
            return 0;
        }

        if (resp->content_length.size() == 0)
        {
            print_err(con, "<%s:%d> 411 Length Required\n", __func__, __LINE__);
            resp_411(resp);
            return 0;
        }

        if (resp->post_cont_length >= conf->ClientMaxBodySize)
        {
            print_err(con, "<%s:%d> 413 Request entity too large: %lld\n", __func__, __LINE__, resp->post_cont_length);
            resp_413(resp);
            return 0;
        }
    }
    //-------------------------------
    if (!strncmp(resp->decode_path.c_str(), "/cgi-bin/", 9))
    {
        resp->content = DYN_PAGE;
        resp->cgi_type = CGI;
    }
    else if (strstr(resp->decode_path.c_str(), ".php"))
    {
        resp->content = DYN_PAGE;
        if (conf->UsePHP == "php-cgi")
            resp->cgi_type = PHPCGI;
        else if (conf->UsePHP == "php-fpm")
            resp->cgi_type = PHPFPM;
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
        add_header(resp, 54, conf->ServerSoftware.c_str());
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
            char flag = resp->headers.get_byte(4);
            resp->headers.set_byte(flag | FLAG_END_STREAM, 4);
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
            print_err(con, "<%s:%d> Error open(%s): %s\n", __func__, __LINE__, resp->decode_path.c_str(), strerror(errno));
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
            add_header(resp, 54, conf->ServerSoftware.c_str());
            add_header(resp, 33, get_time().c_str());
            add_header(resp, 46, resp->path.append("/").c_str());
            add_header(resp, 31, "text/plain");
            resp->create_headers = true;

            ByteArray smg;
            smg.cpy_str("301 Moved Permanently\n");
            smg.cat(resp->path.c_str(), resp->path.size());

            char s[128];
            snprintf(s, sizeof(s), "%d", smg.size());
            add_header(resp, 28, s);

            set_frame_data(resp, smg.size(), FLAG_END_STREAM);
            resp->data.cat(smg.ptr(), smg.size());
            return 0;
        }

        int err = index_dir(con, path, resp);
        if (err)
        {
            print_err(con, "<%s:%d> Error index_dir(): %d\n", __func__, __LINE__, -err);
            resp_500(resp);
            return 0;
        }
        //------------- headers frame --------------
        set_frame_headers(resp);
        add_header(resp, 8);
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/html");
        resp->create_headers = true;
    }
    else if (resp->content == DYN_PAGE)
    {
        resp->data.init();
        resp->cgi_status = CGI_CREATE;
        resp->status = RS200;
    }
    else
    {
        if (iscgi(resp))
        {
            resp->data.init();
            resp->cgi_status = CGI_CREATE;
            resp->status = RS200;
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
void resp_200(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8);
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "200 OK";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_204(Stream *resp)
{
    resp->content = ERROR_TYPE;
    set_frame_headers(resp);
    add_header(resp, 8, "204");
    add_header(resp, 54, conf->ServerSoftware.c_str());
    add_header(resp, 33, get_time().c_str());
    add_header(resp, 28, "0");
    resp->create_headers = true;
    set_frame_data(resp, 0, FLAG_END_STREAM);
}
//======================================================================
void resp_400(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 12);
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "400 Bad Request";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_403(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "403");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "403 Forbidden";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_404(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 13);
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "404 Not Found";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_408(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "408");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "408 Request Timeout";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_411(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "411");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "411 Length Required";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_413(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "413");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "413 Request entity too large";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_414(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "414");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "414 URI Too Long";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_431(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "431");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "431 Request Header Fields Too Large";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_500(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 14);
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "500 Internal Server Error";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_502(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "502");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "502 Bad Gateway";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
void resp_504(Stream *resp)
{
    resp->content = ERROR_TYPE;
    if (resp->send_headers == false)
    {
        set_frame_headers(resp);
        add_header(resp, 8, "504");
        add_header(resp, 54, conf->ServerSoftware.c_str());
        add_header(resp, 33, get_time().c_str());
        add_header(resp, 31, "text/plain");
        resp->create_headers = true;
    }

    const char *err = "504 Gateway Time-out";
    int len = strlen(err);
    set_frame_data(resp, len, FLAG_END_STREAM);
    resp->data.cat(err, len);
}
//======================================================================
const char *status_resp(int st)
{
    switch (st)
    {
        case RS200:
            return "200";
        case RS204:
            return "204";
        case RS206:
            return "206";
        case RS301:
            return "301";
        case RS302:
            return "302";
        case RS400:
            return "400";
        case RS401:
            return "401";
        case RS402:
            return "402";
        case RS403:
            return "403";
        case RS404:
            return "404";
        case RS405:
            return "405";
        case RS406:
            return "406";
        case RS407:
            return "407";
        case RS408:
            return "408";
        case RS411:
            return "411";
        case RS413:
            return "413";
        case RS414:
            return "414";
        case RS416:
            return "416";
        case RS429:
            return "429";
        case RS500:
            return "500";
        case RS501:
            return "501";
        case RS502:
            return "502";
        case RS503:
            return "503";
        case RS504:
            return "504";
        case RS505:
            return "505";
        default:
            return "500";
    }
    return "";
}
//======================================================================
void hex_print_stderr(const char *s, int line, const void *p, int n)
{
    int count, addr = 0, col;
    unsigned char *buf = (unsigned char*)p;
    char str[18];
    fprintf(stderr, "<%s:%d>--------------------------------\n", s, line);
    for(count = 0; count < n;)
    {
        fprintf(stderr, "%08X  ", addr);
        for(col = 0, addr = addr + 0x10; (count < n) && (col < 16); count++, col++)
        {
            if (col == 8) fprintf(stderr, " ");
            fprintf(stderr, "%02X ", *(buf+count));
            str[col] = (*(buf + count) >= 32 && *(buf + count) < 127) ? *(buf + count) : '.';
        }
        str[col] = 0;
        if (col <= 8) fprintf(stderr, " ");
        fprintf(stderr, "%*s  %s\n",(16 - (col)) * 3, "", str);
    }

    //fprintf(stderr, "\n");
}
