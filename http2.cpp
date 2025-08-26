#include "main.h"

using namespace std;
//======================================================================
int EventHandlerClass::http2_connection(Connect *con)
{
    if (con->h2.con_status == PREFACE_MESSAGE)
    {
        char buf[25];

        int ret = read_from_client(con, buf, sizeof(buf) - 1);
        if (ret == 24)
        {
            buf[ret] = 0;
            if (memcmp(buf, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24))
            {
                print_err(con, "<%s:%d> Error ---PREFACE_MESSAGE--- %s\n", __func__, __LINE__, buf);
                ssl_shutdown(con);
                return -1;
            }

            if (conf->PrintDebugMsg)
                hex_print_stderr(__func__, __LINE__, buf, 24);
            con->sock_timer = 0;
            con->h2.con_status = SEND_SETTINGS;
            con->h2.init();
            con->tls.poll_event = POLLOUT;
        }
        else
        {
            if (ret == ERR_TRY_AGAIN)
            {
                return 0;
            }

            print_err(con, "<%s:%d> Error read_from_client()=%d\n", __func__, __LINE__, ret);
            ssl_shutdown(con);
            return -1;
        }
        return 0;
    }
    else if (con->h2.con_status == SSL_ACCEPT)
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
                close_connect(con);
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
                unsigned int proto_alpn_len = proto_alpn[0];
                if ((proto_alpn_len == datalen) && (!memcmp(proto_alpn + 1, data, datalen)))
                {
                    con->h2.con_status = PREFACE_MESSAGE;
                    con->sock_timer = 0;
                    return 0;
                }

                hex_print_stderr(__func__, __LINE__, data, datalen);
            }

            print_err(con, "<%s:%d> Error: no protocol has been selected\n", __func__, __LINE__);
            close_connect(con);
            return -1;
        }

        return 0;
    }
    else if (con->h2.con_status == SSL_SHUTDOWN)
    {
        ERR_clear_error();
        char buf[256];
        int err = SSL_read(con->tls.ssl, buf, sizeof(buf));
        if (err <= 0)
        {
            con->tls.err = SSL_get_error(con->tls.ssl, err);
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
                print_err(con, "<%s:%d> SSL_SHUTDOWN: SSL_read() - %s\n", __func__, __LINE__, ssl_strerror(con->tls.err));
                close_connect(con);
                return -1;
            }
        }
        else
        {
            print_err(con, "<%s:%d> SSL_SHUTDOWN: SSL_read()=%d\n", __func__, __LINE__, err);
            con->sock_timer = 0;
            if (conf->PrintDebugMsg)
                hex_print_stderr("recv SSL_SHUTDOWN", __LINE__, buf, err);
        }
        return 0;
    }
    else
    {
        print_err(con, "<%s:%d> !!! Error: type operation (%s)\n", __func__, __LINE__, get_str_operation(con->h2.con_status));
        ssl_shutdown(con);
        return -1;
    }

    return 0;
}
//======================================================================
int EventHandlerClass::recv_frame(Connect *con)
{
    int ret = recv_frame_(con);
    if (ret <= 0)
    {
        if (ret == ERR_TRY_AGAIN)
            return 0;
        ssl_shutdown(con);
        return -1;
    }

    ret = parse_frame(con);
    if (ret < 0)
    {
        if (ret == ERR_TRY_AGAIN)
            return 0;
        ssl_shutdown(con);
        return -1;
    }

    return 0;
}
//======================================================================
int EventHandlerClass::recv_frame_(Connect *con)
{
    if (con->h2.header_len < 9)
    {
        if (con->h2.header_len == 0)
            con->h2.init();
        int ret = read_from_client(con, con->h2.header + con->h2.header_len, 9 - con->h2.header_len);
        if (ret <= 0)
        {
            print_err(con, "<%s:%d> Error read_from_client()=%d\n", __func__, __LINE__, ret);
            return ret;
        }

        con->h2.header_len += ret;
        if (con->h2.header_len == 9)
        {
            con->h2.body_len = ((unsigned char)con->h2.header[0]<<16) +
                ((unsigned char)con->h2.header[1]<<8) + (unsigned char)con->h2.header[2];
            con->h2.type = (FRAME_TYPE)con->h2.header[3];
            con->h2.flags = con->h2.header[4];
            con->h2.id = (((unsigned char)con->h2.header[5] & 0x7f)<<16) + ((unsigned char)con->h2.header[6]<<16) +
                ((unsigned char)con->h2.header[7]<<8) + (unsigned char)con->h2.header[8];
            if (conf->PrintDebugMsg)
                hex_print_stderr(__func__, __LINE__, con->h2.header, 9);
        }
        else
        {
            print_err(con, "<%s:%d> Error read frame header (%s)\n", __func__, __LINE__, get_str_operation(con->h2.con_status));
            return -1;
        }
    }

    char buf[16384];
    if (con->h2.body_len > 0)
    {
        int len_rd = (int)sizeof(buf);
        if (con->h2.body_len < len_rd)
            len_rd = con->h2.body_len;
        int ret = read_from_client(con, buf, len_rd);
        if (ret <= 0)
        {
            if (ret == ERR_TRY_AGAIN)
                print_err(con, "<%s:%d> Error (SSL_ERROR_WANT_READ) read frame %s id=%d \n", __func__, __LINE__, get_str_frame_type(con->h2.type), con->h2.id);
            else
                print_err(con, "<%s:%d> Error read frame %s id=%d \n", __func__, __LINE__, get_str_frame_type(con->h2.type), con->h2.id);
            return ret;
        }

        con->h2.body.cat(buf, ret);
        con->h2.body_len -= ret;
        if (con->h2.body_len == 0)
            con->h2.header_len = 0;
        else if (con->h2.body_len > 0)
            return ERR_TRY_AGAIN;
    }
    else if (con->h2.body_len == 0)
    {
        con->h2.header_len = 0;
    }

    return 1;
}
//======================================================================
int EventHandlerClass::parse_frame(Connect *con)
{
    if (con->h2.type == SETTINGS)
    {
        con->sock_timer = 0;
        if (conf->PrintDebugMsg)
            hex_print_stderr("recv SETTINGS", __LINE__, con->h2.body.ptr(), con->h2.body.size());
        if (con->h2.body.size())
        {
            for (int i = 0; i < (con->h2.body.size()/6); ++i)
            {
                int ind = i * 6;
                if (con->h2.body.get_byte(ind + 1) == 1)
                {
                    long n = (unsigned char)con->h2.body.get_byte(ind + 5);
                    n += ((unsigned char)con->h2.body.get_byte(ind + 4)<<8);
                    n += ((unsigned char)con->h2.body.get_byte(ind + 3)<<16);
                    n += ((unsigned char)con->h2.body.get_byte(ind + 2)<<24);
                    if (conf->PrintDebugMsg)
                        print_err(con, "<%s:%d> SETTINGS_HEADER_TABLE_SIZE [%ld] id=%d \n", __func__, __LINE__, n, 0);
                }
                else if (con->h2.body.get_byte(ind + 1) == 4)
                {
                    con->h2.init_window_size = (unsigned char)con->h2.body.get_byte(ind + 5);
                    con->h2.init_window_size += ((unsigned char)con->h2.body.get_byte(ind + 4)<<8);
                    con->h2.init_window_size += ((unsigned char)con->h2.body.get_byte(ind + 3)<<16);
                    con->h2.init_window_size += ((unsigned char)con->h2.body.get_byte(ind + 2)<<24);
                    if (conf->PrintDebugMsg)
                        print_err(con, "<%s:%d> SETTINGS_INITIAL_WINDOW_SIZE [%ld] id=%d \n", __func__, __LINE__, con->h2.init_window_size, 0);
                }
                else if (con->h2.body.get_byte(ind + 1) == 5)
                {
                    int n = (unsigned char)con->h2.body.get_byte(ind + 5);
                    n += ((unsigned char)con->h2.body.get_byte(ind + 4)<<8);
                    n += ((unsigned char)con->h2.body.get_byte(ind + 3)<<16);
                    n += ((unsigned char)con->h2.body.get_byte(ind + 2)<<24);
                    if (n < conf->DataBufSize)
                    {
                        setDataBufSize(n);
                    }
                    if (conf->PrintDebugMsg)
                        print_err(con, "<%s:%d> SETTINGS_MAX_FRAME_SIZE [%ld] id=%d \n", __func__, __LINE__, n, 0);
                }
            }

            con->h2.connect_window_size += con->h2.init_window_size;
        }

        if (con->h2.header[4] == FLAG_ACK)
        {
            if (conf->PrintDebugMsg)
                print_err(con, "recv SETTINGS flag=ACK\n");
            con->h2.ack_recv = true;
            con->h2.con_status = PROCESSING_REQUESTS;
        }
        else
        {
            con->h2.settings.cpy("\x00\x00\x00\x04\x01\x00\x00\x00\x00", 9);
            con->h2.con_status = SEND_SETTINGS;
        }
    }
    else if (con->h2.type == DATA)
    {
        con->sock_timer = 0;
        Stream *resp = con->h2.get(con->h2.id);
        if (!resp)
        {
            print_err(con, "<%s:%d> recv DATA: Error list.get(id=%d), h2.body.size()=%d, flag=%d \n", __func__, __LINE__,
                            con->h2.id, con->h2.body.size(), (int)con->h2.header[4]);
            return 0;
        }

        if ((con->h2.body.size() == 0) && (con->h2.header[4] == 1))
        {
            if ((resp->cgi_type <= PHPCGI) || (resp->cgi_type == SCGI))
            {
                resp->cgi_status = CGI_STDOUT;
            }

            if (resp->cgi.to_script > 0)
                close(resp->cgi.to_script);
            resp->cgi.to_script = -1;
            return 0;
        }

        if (con->h2.header[4] & FLAG_END_STREAM)
        {
            resp->cgi.end_post_data = true;
            if (conf->PrintDebugMsg)
                print_err(resp, "recv DATA END_STREAM %d, con.cgi_window_size=%ld, stream.cgi_windows_size=%ld, id=%d \n", con->h2.body.size(), con->h2.cgi_window_size, resp->cgi.windows_size, resp->id);
        }

        int body_len = con->h2.body.size();
        const char *p_buf = NULL;
        if (con->h2.header[4] & FLAG_PADDED)
        {
            unsigned int padd = (unsigned char)con->h2.body.get_byte(0);
            body_len -= padd;
            p_buf = con->h2.body.ptr() + 1;
        }
        else
            p_buf = con->h2.body.ptr();
        if (conf->PrintDebugMsg)
        {
            if (body_len < 100)
                print_err(resp, "<%s:%d> recv DATA %d, con.cgi_window_size=%ld, stream.cgi_windows_size=%ld, id=%d \n", __func__, __LINE__, body_len, con->h2.cgi_window_size, resp->cgi.windows_size, resp->id);
        }

        con->h2.cgi_window_size -= body_len;
        resp->cgi.windows_size -= body_len;

        con->h2.cgi_window_update += body_len;
        resp->cgi.window_update += body_len;

        if ((resp->cgi_type <= PHPCGI) || (resp->cgi_type == SCGI))
        {
            resp->post_data.cat(p_buf, body_len);
        }
        else if ((resp->cgi_type == FASTCGI) || (resp->cgi_type == PHPFPM))
        {
            char s[8];
            fcgi_set_header(s, FCGI_STDIN, body_len);
            resp->post_data.cat(s, 8);
            if (body_len)
            {
                resp->post_data.cat(p_buf, body_len);
                if (resp->cgi.end_post_data)
                    resp->post_data.cat("\1\5\0\1\0\0\0\0", 8);
            }

            if (resp->cgi.fcgiContentLen)
            {
                print_err(resp, "<%s:%d> !!! resp->cgi.fcgiContentLen=%d(%lld), id=%d \n", __func__, __LINE__,
                        resp->cgi.fcgiContentLen, resp->post_cont_length, resp->id);
            }
            resp->cgi.fcgiContentLen += body_len;
        }

        if (resp->post_cont_length < 0)
        {
            print_err(resp, "<%s:%d> !!! Error: cont_length=%lld, body_len=%d, size=%d, id=%d \n", __func__, __LINE__,
                        resp->post_cont_length, body_len, resp->post_data.size(), resp->id);
            resp_500(resp);
            return 0;
        }
    }
    else if (con->h2.type == HEADERS)
    {
        if (conf->PrintDebugMsg)
            hex_print_stderr(__func__, __LINE__, con->h2.body.ptr(), con->h2.body.size());
        con->sock_timer = 0;
        con->h2.numReq++;
        Stream *resp = con->h2.add();
        if (resp == NULL)
        {
            print_err(con, "<%s:%d> Error id=%d \n", __func__, __LINE__, con->h2.id);
            set_rst_stream(con, con->h2.id, CANCEL);
            return 0;
        }

        if (con->h2.flags & FLAG_END_HEADERS)
        {
            set_response(con, resp);
            if (conf->PrintDebugMsg)
                print_err(resp, "\"%s\" new request headers.size=%d, id=%d \n", resp->decode_path.c_str(), resp->headers.size(), resp->id);
        }
        else
        {
            // frame CONTINUATION not support
            print_err(resp, "<%s:%d> Error: frame CONTINUATION not support, id=%d \n", __func__, __LINE__, con->h2.id);
            set_response(con, resp);
            print_err(resp, "\"%s\" new request headers.size=%d, id=%d \n", resp->decode_path.c_str(), resp->headers.size(), con->h2.id);
            resp_431(resp);
            return 0;
        }
    }
    else if (con->h2.type == CONTINUATION)
    {
        // frame CONTINUATION not support
        print_err(con, "<%s:%d> Error: frame CONTINUATION not support, id=%d \n", __func__, __LINE__, con->h2.id);
        set_rst_stream(con, con->h2.id, CANCEL);
        return 0;
    }
    else if (con->h2.type == GOAWAY)
    {
        if (conf->PrintDebugMsg)
        {
            print_err(con, "recv GOAWAY [%s]\n", get_str_error(con->h2.body.get_byte(7)));
            //hex_print_stderr("recv GOAWAY", __LINE__, con->h2.body.ptr(), con->h2.body.size());
        }

        return -1;
    }
    else if (con->h2.type == RST_STREAM)
    {
        con->sock_timer = 0;
        print_err(con, "recv RST_STREAM [%s] id=%d \n", get_str_error(con->h2.body.get_byte(3)), con->h2.id);
        if (con->h2.id == 0)
        {
            set_frame_goaway(con, PROTOCOL_ERROR);
            return 0;
        }

        Stream *resp = con->h2.get(con->h2.id);
        if (resp)
        {
            //print_err(resp, "<%s:%d> RST_STREAM  data.size=%d, id=%d\n", __func__, __LINE__, resp->data.size(), con->h2.id);
            if (resp->data.size() == 0)
            {
                print_log(resp);
                con->h2.close_stream(resp->id);
            }
            else
                resp->rst_stream = true;
        }
        else
            print_err(con, "<%s:%d> RST_STREAM Error stream id=%d does not exist\n", __func__, __LINE__, con->h2.id);
    }
    else if (con->h2.type == PING)
    {
        if (conf->PrintDebugMsg)
            hex_print_stderr("recv PING", __LINE__, con->h2.body.ptr(), con->h2.body.size());
        con->sock_timer = 0;
        print_err(con, "recv PING\n");
        con->h2.ping.cpy("\x0\x0\x8\x6\x1\x0\x0\x0\x0", 9);
        con->h2.ping.cat(con->h2.body.ptr(), con->h2.body.size());
    }
    else if (con->h2.type == WINDOW_UPDATE)
    {
        con->sock_timer = 0;
        long n = 0;
        n += (con->h2.body.get_byte(3));
        n += (con->h2.body.get_byte(2)<<8);
        n += (con->h2.body.get_byte(1)<<16);
        n += (con->h2.body.get_byte(0)<<24);

        if (con->h2.id == 0)
        {
            con->h2.connect_window_size += n;
            if (conf->PrintDebugMsg)
                print_err(con, "<%s:%d> WINDOW_UPDATE %ld[%ld] id=%d \n", __func__, __LINE__, n, con->h2.connect_window_size, con->h2.id);
        }
        else
        {
            con->h2.set_window_size(con->numConn, con->h2.id, n);
            if (conf->PrintDebugMsg)
                print_err(con, "<%s:%d> WINDOW_UPDATE %ld id=%d \n", __func__, __LINE__, n, con->h2.id);
        }
    }
    else if (con->h2.type == PRIORITY)
    {
        con->sock_timer = 0;
        if (conf->PrintDebugMsg)
            print_err(con, "<%s:%d> PRIORITY id=%d \n", __func__, __LINE__, con->h2.id);
    }
    else
    {
        con->sock_timer = 0;
        //if (conf->PrintDebugMsg)
            print_err(con, "<%s:%d> frame type: %s, id=%d \n", __func__, __LINE__, get_str_frame_type(con->h2.type), con->h2.id);
    }

    return 0;
}
//======================================================================
void EventHandlerClass::send_frames(Connect *con)
{
    int ret = send_frames_(con);
    if (ret < 0)
    {
        if (ret == ERR_TRY_AGAIN)
        {
            con->h2.try_again = true;
            return;
        }
        ssl_shutdown(con);
            return;
    }

    con->h2.try_again = false;
}
//======================================================================
int EventHandlerClass::send_frames_(Connect *con)
{
    if (con->h2.con_status == SEND_SETTINGS)
    {
        if (con->h2.settings.size() == 0)
        {
            print_err(con, "<%s:%d> !!! SEND_SETTINGS Error: settings.size() = 0\n", __func__, __LINE__);
            return -1;
        }

        int ret = send_frame_settings(con);
        if (ret < 0)
            return ret;
    }
    else if (con->h2.con_status == PROCESSING_REQUESTS)
    {
        if (con->h2.goaway.size())
        {
            return send_frame_goawey(con);
        }

        if (con->h2.ping.size())
        {
            int ret = send_frame_ping(con);
            if (ret < 0)
                return ret;
        }

        if (con->h2.start_list_send_frame)
        {
            return send_frame_rststream(con);
        }

        if (con->h2.frame_win_update.size() || (con->h2.cgi_window_update > 0))
        {
            if (con->h2.cgi_window_update > 32000)
            {
                print_err(con, "<%s:%d> ??? con->h2.server_window_size(%ld) > 32000\n", __func__, __LINE__, con->h2.cgi_window_update);
            }

            if (con->h2.frame_win_update.size() == 0)
                set_frame_window_update(con, con->h2.cgi_window_update);
            int ret = send_window_update(con);
            if (ret < 0)
                return ret;
        }
        //--------------------------------------------------------------
        int n = con->h2.size();
        if (n > conf->MaxConcurrentStreams)
        {
            print_err(con, "<%s:%d> ??? h2.size()=%d\n", __func__, __LINE__, n);
            return -1;
        }
        else if (n == 0)
            return 0;

        if (con->h2.work_stream == NULL)
            return 0;
        Stream *resp = con->h2.work_stream;
        if (resp == NULL)
        {
            if ((resp = con->h2.start_stream) == NULL)
            {
                print_err(con, "<%s:%d> ??? resp == NULL\n", __func__, __LINE__);
                return 0;
            }
        }

        for ( ; resp; resp = con->h2.work_stream)
        {
            if (resp->frame_win_update.size() || (resp->cgi.window_update > 0))
            {
                if (resp->cgi.window_update > 32000)
                {
                    if (conf->PrintDebugMsg)
                        print_err(resp, "<%s:%d> ??? resp->cgi.window_update(%ld) > 32000\n", __func__, __LINE__, resp->cgi.window_update);
                }

                if (resp->frame_win_update.size() == 0)
                    set_frame_window_update(resp, resp->cgi.window_update);
                int ret = send_window_update(con, resp);
                if (ret < 0)
                    return ret;
            }

            if (resp->headers.size())
            {
                int ret = send_frame_headers(con, resp);
                if (ret < 0)
                    return ret;
            }

            if (resp->send_headers && (!resp->send_end_stream))
            {
                int ret = send_frame_data(con, resp);
                if (ret < 0)
                    return ret;
            }

            if (con->h2.work_stream)
                con->h2.work_stream = con->h2.work_stream->next;
        }
    }
    else
    {
        print_err(con, "<%s:%d> !!! Error: connections status (%s)\n", __func__, __LINE__, get_str_operation(con->h2.con_status));
        return -1;
    }

    return 0;
}
//======================================================================
int EventHandlerClass::send_frame_headers(Connect *con, Stream *resp)
{
    if (resp->headers.size() && (!resp->send_headers))
    {
        int ret = write_to_client(con, resp->headers.ptr(), resp->headers.size(), resp->id);
        if (ret < 0)
        {
            if (ret == ERR_TRY_AGAIN)
            {
                //print_err(resp, "<%s:%d> Error send frame HEADERS: SSL_ERROR_WANT_WRITE, id=%d \n", __func__, __LINE__, resp->id);
            }
            else
                print_err(resp, "<%s:%d> Error send frame HEADERS: %d, id=%d \n", __func__, __LINE__, ret, resp->id);
            return ret;
        }

        con->sock_timer = 0;
        resp->send_headers = true;
        if (resp->headers.get_byte(4) & FLAG_END_STREAM)
        {
            if (conf->PrintDebugMsg)
            {
                print_err(resp, "<%s:%d>... send frame HEADERS, END_STREAM, [%s] send %lld bytes ... id=%d \n", 
                        __func__, __LINE__, resp->decode_path.c_str(), resp->send_bytes, resp->id);
            }

            resp->send_end_stream = true;
            print_log(resp);
            con->h2.close_stream(resp->id);
        }
        else
        {
            if (conf->PrintDebugMsg)
                print_err(resp, "<%s:%d> send frame HEADERS: %d, id=%d \n", __func__, __LINE__, ret, resp->id);
            resp->headers.init();
        }

        return 0;
    }
    else
    {
        print_err(resp, "<%s:%d> !!! Error id=%d \n", __func__, __LINE__, resp->id);
        return -1;
    }
}
//=============================================================================================================================
int EventHandlerClass::send_frame_data(Connect *con, Stream *resp)
{
    if (resp->data.size() == 0)
    {
        if (resp->rst_stream)
        {
            if (resp->content == DYN_PAGE)
                set_rst_stream(con, resp->id, CANCEL);
            else
            {
                print_log(resp);
                con->h2.close_stream(resp->id);
            }

            return 0;
        }
        else
        {
            if (con->h2.connect_window_size <= 0)
            {
                //print_err(resp, "<%s:%d> !!! connect_window_size(%ld) <= 0, id=%d \n", __func__, __LINE__, con->h2.connect_window_size, resp->id);
                return 0;
            }

            if (resp->stream_window_size <= 0)
            {
                //print_err(resp, "<%s:%d> !!! stream_window_size(%ld) <= 0, %ld, id=%d \n", __func__, __LINE__, resp->stream_window_size, con->h2.connect_window_size, resp->id);
                return 0;
            }

            int ret = set_frame_data(con, resp);
            if (ret < 0)
                return ret;
            else if (ret == 0)
                return 0;
        }
    }

    int ret = write_to_client(con, resp->data.ptr(), resp->data.size(), resp->id);
    if (ret < 0)
    {
        if (ret == ERR_TRY_AGAIN)
        {
            //print_err(resp, "<%s:%d> Error send frame DATA: %d, %d, id=%d \n", __func__, __LINE__,
            //                                    ret, resp->data.size(), resp->id);
        }
        else
        {
            print_err(resp, "<%s:%d> Error send frame DATA: %d, %d, send_bytes=%lld, id=%d \n", __func__, __LINE__,
                                                ret, resp->data.size(), resp->send_bytes, resp->id);
            resp->data.init();
        }

        return ret;
    }

    con->sock_timer = 0;
    resp->send_bytes += (ret - 9);
    resp->stream_window_size -= (ret - 9);
    con->h2.connect_window_size -= (ret - 9);

    if (conf->PrintDebugMsg)
    {
        print_err(resp, "<%s:%d> send frame DATA: %d, send_bytes=%lld, id=%d \n", __func__, __LINE__,
                                                ret, resp->send_bytes, resp->id);
    }

    if ((resp->data.get_byte(4) & FLAG_END_STREAM) || resp->rst_stream)
    {
        if (conf->PrintDebugMsg)
        {
            print_err(resp, "<%s:%d>... send frame DATA, END_STREAM, [%s] send %lld bytes, data.size=%d ... id=%d \n", 
                        __func__, __LINE__, resp->decode_path.c_str(), resp->send_bytes, resp->data.size(), resp->id);
        }

        resp->send_end_stream = true;
        print_log(resp);
        con->h2.close_stream(resp->id);
        return 0;
    }
    else
        resp->data.init();

    return 0;
}
//======================================================================
int EventHandlerClass::send_frame_settings(Connect *con)
{
    if (con->h2.settings.size() == 0)
    {
        print_err(con, "<%s:%d> !!! SEND_SETTINGS Error: settings.size() = 0\n", __func__, __LINE__);
        return -1;
    }

    int ret = write_to_client(con, con->h2.settings.ptr(), con->h2.settings.size(), 0);
    if (ret < 0)
    {
        print_err(con, "<%s:%d> Error send frame SETTINGS\n", __func__, __LINE__);
        if (ret != ERR_TRY_AGAIN)
            con->h2.settings.init();
        return ret;
    }

    if (conf->PrintDebugMsg)
        hex_print_stderr(__func__, __LINE__, con->h2.settings.ptr(), con->h2.settings.size());

    con->sock_timer = 0;
    con->h2.settings.init();
    if (con->h2.ack_recv)
        con->h2.con_status = PROCESSING_REQUESTS;
    else
        con->h2.con_status = RECV_SETTINGS;
    return 0;
}
//======================================================================
int EventHandlerClass::send_frame_ping(Connect *con)
{
    int ret = write_to_client(con, con->h2.ping.ptr(), con->h2.ping.size(), 0);
    if (ret < 0)
    {
        print_err(con, "<%s:%d> Error send frame PING, id=0 \n", __func__, __LINE__);
        if (ret != ERR_TRY_AGAIN)
            con->h2.ping.init();
        return ret;
    }

    if (conf->PrintDebugMsg)
        hex_print_stderr(__func__, __LINE__, con->h2.ping.ptr(), con->h2.ping.size());
    con->h2.ping.init();

    return 0;
}
//======================================================================
int EventHandlerClass::send_frame_goawey(Connect *con)
{
    int ret = write_to_client(con, con->h2.goaway.ptr(), con->h2.goaway.size(), 0);
    if (ret < 0)
    {
        print_err(con, "<%s:%d> Error send frame GOAWAY, id=0 \n", __func__, __LINE__);
        if (ret != ERR_TRY_AGAIN)
            con->h2.goaway.init();
        return ret;
    }

    if (conf->PrintDebugMsg)
        hex_print_stderr(__func__, __LINE__, con->h2.goaway.ptr(), con->h2.goaway.size());
    con->h2.goaway.init();
    return -1;
}
//======================================================================
int EventHandlerClass::send_frame_rststream(Connect *con)
{
    FrameRedySend *rf = con->h2.start_list_send_frame;
    if (!rf)
    {
        return 0;
    }

    int ret = write_to_client(con, rf->frame.ptr(), rf->frame.size(), 0);
    if (ret < 0)
    {
        print_err(con, "<%s:%d> Error send frame GOAWAY, id=0 \n", __func__, __LINE__);
        if (ret != ERR_TRY_AGAIN)
            con->h2.del_from_list(rf);
        return ret;
    }

    if (conf->PrintDebugMsg)
        hex_print_stderr(__func__, __LINE__, rf->frame.ptr(), rf->frame.size());
    con->h2.del_from_list(rf);
    return 0;
}
//======================================================================
int EventHandlerClass::send_window_update(Connect *con)
{
    int ret = 0;
    if ((ret = write_to_client(con, con->h2.frame_win_update.ptr(), con->h2.frame_win_update.size(), 0)) <= 0)
    {
        print_err(con, "<%s:%d> Error send frame WINDOW_UPDATE: %d, %d, id=0 \n", __func__, __LINE__, ret, con->h2.frame_win_update.size());
        return ret;
    }

    con->h2.cgi_window_size += con->h2.cgi_window_update;
    con->h2.cgi_window_update = 0;
    con->sock_timer = 0;
    con->h2.frame_win_update.init();

    return 0;
}
//======================================================================
int EventHandlerClass::send_window_update(Connect *con, Stream *resp)
{
    int ret = 0;
    if ((ret = write_to_client(con, resp->frame_win_update.ptr(), resp->frame_win_update.size(), resp->id)) <= 0)
    {
        print_err(resp, "<%s:%d> Error send frame WINDOW_UPDATE: %d, %d, id=%d \n", __func__, __LINE__, ret, resp->frame_win_update.size(), resp->id);
        return ret;
    }

    resp->cgi.windows_size += resp->cgi.window_update;
    resp->cgi.window_update = 0;
    con->sock_timer = 0;
    resp->frame_win_update.init();

    return 0;
}
//======================================================================
const char *static_tab[][2] = {
                         {"", ""},
                         {"authority", ""},
                         {"method", "GET"},
                         {"method", "POST"},
                         {"path", "/"},
                         {"path", "/index.html"},
                         {"scheme", "http"},
                         {"scheme", "https"},
                         {"status", "200"},
                         {"status", "204"},
                         {"status", "206"},
                         {"status", "304"},
                         {"status", "400"},
                         {"status", "404"},
                         {"status", "500"},
                         {"accept-charset", ""},
                         {"accept-encoding", "gzip, deflate"},
                         {"accept-language", ""},
                         {"accept-ranges", ""},
                         {"accept", ""},
                         {"access-control-allow-origin", ""},
                         {"age", ""},
                         {"allow", ""},
                         {"authorization", ""},
                         {"cache-control", ""},
                         {"content-disposition", ""},
                         {"content-encoding", ""},
                         {"content-language", ""},
                         {"content-length", ""},
                         {"content-location", ""},
                         {"content-range", ""},
                         {"content-type", ""},
                         {"cookie", ""},
                         {"date", ""},
                         {"etag", ""},
                         {"expect", ""},
                         {"expires", ""},
                         {"from", ""},
                         {"host", ""},
                         {"if-match", ""},
                         {"if-modified-since", ""},
                         {"if-none-match", ""},
                         {"if-range", ""},
                         {"if-unmodified-since", ""},
                         {"last-modified", ""},
                         {"link", ""},
                         {"location", ""},
                         {"max-forwards", ""},
                         {"proxy-authenticate", ""},
                         {"proxy-authorization", ""},
                         {"range", ""},
                         {"referer", ""},
                         {"refresh", ""},
                         {"retry-after", ""},
                         {"server", ""},
                         {"set-cookie", ""},
                         {"strict-transport-security", ""},
                         {"transfer-encoding", ""},
                         {"user-agent", ""},
                         {"vary", ""},
                         {"via", ""},
                         {"www-authenticate", ""},
                         {NULL, NULL}};
