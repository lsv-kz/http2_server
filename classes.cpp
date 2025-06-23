#include "main.h"

//======================================================================
Stream *http2::add()
{
    if (len_ >= size_)
    {
        print_err("<%s:%d> Error: num streams: %d\n", __func__, __LINE__, len_);
        return NULL;
    }

    Stream *resp = NULL;
    resp = new(std::nothrow) Stream;
    if (!resp)
    {
        print_err("<%s:%d> Error: %s\n", __func__, __LINE__, strerror(errno));
        return NULL;
    }

    resp->numConn = numConn;
    resp->numReq = numReq;

    resp->remoteAddr = remoteAddr;
    resp->remotePort = remotePort;

    resp->len = body_len;
    resp->type = type;
    resp->flags = flags;
    resp->id = id;

    if (init_window_size > 0)
    {
        resp->window_update = init_window_size;
    }
    else
    {
        resp->window_update = 65535;
        window_update = 65535;
    }

    int ret = parse(resp);
    if (ret)
    {
        return NULL;
    }

    resp->prev = end;
    resp->next = NULL;
    if (start)
    {
        end->next = resp;
        end = resp;
    }
    else
        start = end = resp;
    ++len_;
    return resp;
}
//------------------------------------------------------------------
void http2::del_from_list(Stream *r)
{
    if (r->prev && r->next)
    {
        r->prev->next = r->next;
        r->next->prev = r->prev;
    }
    else if (r->prev && !r->next)
    {
        r->prev->next = r->next;
        end = r->prev;
    }
    else if (!r->prev && r->next)
    {
        r->next->prev = r->prev;
        start = r->next;
    }
    else if (!r->prev && !r->next)
        start = end = NULL;
    --len_;
}
//------------------------------------------------------------------
int http2::close_stream(http2 *h2, int id, int *num_cgi)
{
    Stream *r = start, *next = NULL;
    for ( ; r; r = next)
    {
        next = r->next;
        if (r->id == id)
        {
            if (h2->next == r)
            {
                print_err("<%s:%d> !!!!! h2->next == r\n", __func__, __LINE__);
                h2->next = next;
            }

            if (r->cgi.start)
            {
                print_err("<%s:%d>~~~~~~~ close cgi stream, id=%d \n", __func__, __LINE__, r->id);
                if (*num_cgi <= 0)
                    print_err("<%s:%d>~~~~~~~ Error: *num_cgi=%d, id=%d \n", __func__, __LINE__, *num_cgi, r->id);
                else
                    (*num_cgi)--;
                if (r->cgi.cgi_type <= PHPCGI)
                {
                    h2->num_cgi--;
                    kill_chld(r);
                }
            }

            del_from_list(r);
            delete r;
            return id;
        }
    }

    return -1;
}
//------------------------------------------------------------------
int http2::set_window_size(unsigned long num_conn, int id, long n)
{
    Stream *r = start, *next = NULL;
    for ( ; r; r = next)
    {
        next = r->next;
        if (r->id == id)
        {
            r->window_update += n;
            return id;
        }
    }

    return id;
}
//------------------------------------------------------------------
Stream *http2::get(int id)
{
    Stream *r = start, *next = NULL;
    for ( ; r; r = next)
    {
        next = r->next;
        if (r->id == id)
            return r;
    }

    return NULL;
}
//------------------------------------------------------------------
Stream *http2::get()
{
    if (start)
    {
        Stream *rtmp = start;
        return rtmp;
    }
    else
        return NULL;
}
//------------------------------------------------------------------
int http2::size()
{
    return len_;
}
//------------------------------------------------------------------
int http2::pow_(int x, int y)
{
    if (y < 0)
        return -1;
    int m = 1;
    for (int i = 0; i < y; ++i)
        m = m * x;
    return m;
}
//------------------------------------------------------------------
int http2::bytes_to_int(unsigned char prefix, const char *s, int *len, int size)
{
    int pref_len = 0;
    for (int j = 0; j < 7; ++j)
    {
        if (((prefix)>>j) & 1)
            ++pref_len;
        else
            break;
    }

    int n = pow_(2, pref_len) - 1;
    if (n < 0)
        return -1;
    unsigned char ch;
    for (int i = 0; (*len) < size; ++i)
    {
        if ((ch = s[(*len)++]) == 0)
            return -1;
        n = n + ((ch & 0x7f)<<(i*7));
        if (!(ch & 0x80))
            break;
    }
    return n;
}
//------------------------------------------------------------------
int http2::parse(Stream *r)
{
    int len = 0;
    int ch;
    if (flags & 0x08) // PADDED (0x8)
        ++len;
    if (flags & 0x20) // PRIORITY (0x20)
        len += 5;
    std::string name;
    std::string val;

    for ( ; len < body.size(); )
    {
        if ((ch = body.get_byte(len++)) < 0)
        {
            fprintf(stderr, "<%s:%d> Error ch=%d, 0x%X\n", __func__, __LINE__, ch, ch);
            return -1;
        }

        if (ch >= 0x80) // <0x81 ... 0xFF> ; static table: <0x81 ... 0x3D> ; dynamic table: <0x3E ... (0xFF 0xXX ...)>
        {
            int ind = ch & 0x7f;
            if (ind > 61)
            {
                fprintf(stderr, "<%s:%d> Error ind: %d > 61\n", __func__, __LINE__, ind);
                return -1;
            }
            name = static_tab[ind][0];
            val = static_tab[ind][1];
        }
        else if ((ch >= 0x00) && (ch <= 0x0f)) // no; <0x01 ... 0x0F><len><val>, <0x00><len><name><len><val>
        {
            int ind = ch; // & 0x0f;
            int name_len;
            //--------------------- name ---------------------------
            if (ind == 0) // <0x00><len><name><len><val>
            {
                if ((ch = body.get_byte(len++)) < 0)
                {
                    return -1;
                }

                bool huffman = ch & 0x80;
                if ((ch & 0x7f) == 0x7f) // 0x7f or 0xff
                {
                    name_len = bytes_to_int(ch, body.ptr(), &len, body.size());
                    if (name_len <= 0)
                        return -1;
                }
                else
                    name_len = ch & 0x7f;

                if ((name_len + len) > body.size())
                {
                    return -1;
                }

                if (huffman)
                    huff.decode_bytearray(body.ptr() + len, name_len, name);
                else
                    name.assign(body.ptr() + len, name_len);
                len += (name_len);
            }
            else //  <0x01 ... 0x0F><len><val>
            {
                if (ind == 15) // 0x0f
                {
                    ind = bytes_to_int(ch & 0x0f, body.ptr(), &len, body.size());
                    if (ind <= 0)
                        return -1;
                }

                if (ind <= 61)
                {
                    name = static_tab[ind][0];
                    name_len = strlen(static_tab[ind][0]);
                }
                else
                {
                    fprintf(stderr, "<%s:%d> Error ind: %d > 61\n", __func__, __LINE__, ind);
                    return -1;
                }
            }
            //----------------------- val --------------------------
            if ((ch = body.get_byte(len++)) < 0)
            {
                return -1;
            }

            bool huffman = ch & 0x80;
            int val_len = ch & 0x7f;
            if ((val_len & 0x7f) < 0x7f)
            {
                val_len = ch & 0x7f;
            }
            else // 0x7f
            {
                val_len = bytes_to_int(ch, body.ptr(), &len, body.size());
                if (val_len <= 0)
                    return -1;
            }

            if ((val_len + len) > body.size())
            {
                return -1;
            }

            if (huffman)
                huff.decode_bytearray(body.ptr() + len, val_len, val);
            else
                val.assign(body.ptr() + len, val_len);

            len += val_len;
        }
        else if ((ch >= 0x40) && (ch <= 0x7f)) // <0x41 ... 0x7F><len><val>
        {
            int ind = ch & 0x3f;
            int name_len;
            //---------------------- name --------------------------
            if (ind == 0x00)
            {
                if ((ch = body.get_byte(len++)) < 0)
                {
                    return -1;
                }

                bool huffman = ch & 0x80;
                name_len = ch & 0x7f;
                if (name_len == 0x7f)
                {
                    name_len = bytes_to_int(ch, body.ptr(), &len, body.size());
                    if (name_len <= 0)
                        return -1;
                }

                if ((name_len + len) > body.size())
                {
                    return -1;
                }

                if (huffman)
                    huff.decode_bytearray(body.ptr() + len, name_len, name);
                else
                    name.assign(body.ptr() + len, name_len);
                len += (name_len);
            }
            else
            {
                if (ind == 0x7f)
                {
                    ind = bytes_to_int(ch & 0x3f, body.ptr(), &len, body.size());
                    if (ind <= 0)
                        return -1;
                }

                if (ind <= 61)
                {
                    name = static_tab[ind][0];
                }
                else
                {
                    fprintf(stderr, "<%s:%d> Error ind: %d > 61\n", __func__, __LINE__, ind);
                    return -1;
                }
            }
            //----------------------- val --------------------------
            if ((ch = body.get_byte(len++)) < 0)
            {
                return -1;
            }

            bool huffman = ch & 0x80;
            int val_len = ch & 0x7f;
            if (val_len == 0x7f)
            {
                val_len = bytes_to_int(ch, body.ptr(), &len, body.size());
                if (val_len <= 0)
                    return -1;
            }

            if ((val_len + len) > body.size())
            {
                return -1;
            }

            if (huffman)
                huff.decode_bytearray(body.ptr() + len, val_len, val);
            else
                val.assign(body.ptr() + len, val_len);
            len += (val_len);
        }
        else if ((ch >= 0x10) && (ch <= 0x1f)) // <0x11 ... 0x1F><len><val> 0001 xxxx 0x0f
        {
            //---------------------- name --------------------------
            int ind = ch & 0x0f;
            if (ind == 0x00) // <0x10><len><name><len><val>
            {
                if ((ch = body.get_byte(len++)) < 0)
                {
                    return -1;
                }

                bool huffman = ch & 0x80;
                int name_len = ch & 0x7f;
                if (name_len == 0x7f) // [H]111 1111
                {
                    name_len = bytes_to_int(ch, body.ptr(), &len, body.size());
                    if (name_len <= 0)
                        return -1;
                }

                if ((name_len + len) > body.size())
                {
                    return -1;
                }

                if (huffman)
                    huff.decode_bytearray(body.ptr() + len, name_len, name);
                else
                    name.assign(body.ptr() + len, name_len);
                len += (name_len);
            }
            else
            {
                if (ind == 0x0f) // <0x1F><len><val> 0001 11111
                {
                    ind = bytes_to_int(ind, body.ptr(), &len, body.size());
                    if (ind <= 0)
                        return -1;
                }

                if (ind <= 61)
                {
                    name = static_tab[ind][0];
                }
                else
                {
                    fprintf(stderr, "<%s:%d> Error ind: %d > 61\n", __func__, __LINE__, ind);
                    return -1;
                }
            }
            //------------------------ val -------------------------
            if ((ch = body.get_byte(len++)) < 0)
            {
                return -1;
            }

            bool huffman = ch & 0x80;
            int val_len = ch & 0x7f;
            if (val_len == 0x7f)
            {
                val_len = bytes_to_int(ch, body.ptr(), &len, body.size());
                if (val_len <= 0)
                    return -1;
            }

            if ((val_len + len) > body.size())
            {
                return -1;
            }

            if (huffman)
                huff.decode_bytearray(body.ptr() + len, val_len, val);
            else
                val.assign(body.ptr() + len, val_len);
            len += (val_len);
        }
        else if ((ch >= 0x20) && (ch <= 0x3f))
        {
            fprintf(stderr, "<%s:%d>--- Dynamic Table Size Update: %d ---\n", __func__, __LINE__, ch & 0x1f);
            continue;
        }
        else
        {
            fprintf(stderr, "<%s:%d> !!! 0x%02X\n", __func__, __LINE__, ch);
            return -1;
        }

        //fprintf(stderr, "<%s:%d> [%s: %s]\n", __func__, __LINE__, name.c_str(), val.c_str());
        if (name == "method")
        {
            if (strstr_case(val.c_str(), "post"))
                r->method = "POST";
            else if (strstr_case(val.c_str(), "get"))
                r->method = "GET";
            else
                r->method = val;
        }
        else if (name == "path")
            r->path = val;
        else if (name == "host")
            r->host = val;
        else if (name == "user-agent")
            r->user_agent = val;
        else if (name == "referer")
            r->referer = val;
        else if (name == "range")
            r->range = val;
        else if (name == "authority")
            r->authority = val;
        else if (name == "content-length")
        {
            r->content_length = val;
            try
            {
                r->post_cont_length = stoll(val, NULL, 10);
            }
            catch (...)
            {
                print_err("<%s:%d> Error stoll(\"%s\")\n", __func__, __LINE__, val.c_str());
                return -1;
            }
        }
        else if (name == "content-type")
        {
            r->content_type = val;
        }
    }
    return 0;
}

