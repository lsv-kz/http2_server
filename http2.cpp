#include "main.h"

using namespace std;
//======================================================================
int EventHandlerClass::http2_worker_connections(Connect *con)
{
    if (con->h2.type_op == PREFACE_MESSAGE)
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
            con->h2.type_op = SEND_SETTINGS;
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
    else if (con->h2.type_op == SEND_SETTINGS)
    {
        if (con->h2.settings.size_remain() == 0)
        {
            print_err(con, "<%s:%d> !!! SEND_SETTINGS Error: settings.size_remain() == 0\n", __func__, __LINE__);
            return 0;
        }

        int ret = write_to_client(con, con->h2.settings.ptr_remain(), con->h2.settings.size_remain(), 0);
        if (ret <= 0)
        {
            if (ret == ERR_TRY_AGAIN)
                return 0;
            print_err(con, "<%s:%d> Error send frame SETTINGS\n", __func__, __LINE__);
            ssl_shutdown(con);
            return -1;
        }

        if (conf->PrintDebugMsg)
            hex_print_stderr("send SETTINGS", __LINE__, con->h2.settings.ptr_remain(), con->h2.settings.size_remain());

        con->h2.settings.set_offset(ret);

        if (con->h2.settings.size_remain() == 0)
        {
            con->sock_timer = 0;
            con->h2.settings.init();
            if (con->h2.ack_recv)
                con->h2.type_op = WORK_STREAM;
            else
                con->h2.type_op = RECV_SETTINGS;
        }
    }
    else if (con->h2.type_op == SSL_ACCEPT)
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
                print_err(con, "<%s:%d> Error SSL_accept(): %s\n", __func__, __LINE__, ssl_strerror(con->tls.err));
                close_connect(con);
                return -1;
            }
        }
        else
        {
            const unsigned char *alpn = NULL;
            unsigned int alpnlen = 0;
            SSL_get0_next_proto_negotiated(con->tls.ssl, &alpn, &alpnlen);
            if (alpn == NULL)
                SSL_get0_alpn_selected(con->tls.ssl, &alpn, &alpnlen);
            if (alpn)
            {
                if (!memcmp("h2", alpn, alpnlen))
                {
                    con->h2.type_op = PREFACE_MESSAGE;
                    con->sock_timer = 0;
                    return 0;
                }

                hex_print_stderr(__func__, __LINE__, alpn, alpnlen);
            }

            print_err(con, "<%s:%d> Error: no protocol has been selected\n", __func__, __LINE__);
            close_connect(con);
            return -1;
        }

        return 0;
    }
    else if (con->h2.type_op == SSL_SHUTDOWN)
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
    else if ((con->h2.type_op == WORK_STREAM) || (con->h2.type_op == RECV_SETTINGS))
    {
        if ((con->fd_revents & POLLIN) || con->ssl_pending)
        {
            if (recv_from_client(con) < 0)
            {
                ssl_shutdown(con);
                return -1;
            }
        }
    }

    return 0;
}
//======================================================================
int EventHandlerClass::send_window_update(Connect *con)
{
    int ret = 0;
    if ((ret = write_to_client(con, con->h2.frame_win_update.ptr(), con->h2.frame_win_update.size(), 0)) <= 0)
    {
        print_err(con, "<%s:%d> Error send frame WINDOW_UPDATE: %d, %d, id=0 \n", __func__, __LINE__, ret, con->h2.frame_win_update.size());
        if (ret == ERR_TRY_AGAIN)
            return 0;
        return -1;
    }

    con->h2.server_window_size = 0;
    con->sock_timer = 0;
    con->h2.frame_win_update.init();
    con->h2.send_ready &= (~FRAME_WINUPDATE_CONNECT_READY);

    return 0;
}
//======================================================================
int EventHandlerClass::send_window_update(Connect *con, Stream *resp)
{
    int ret = 0;
    if ((ret = write_to_client(con, resp->frame_win_update.ptr(), resp->frame_win_update.size(), resp->id)) <= 0)
    {
        print_err(con, "<%s:%d> Error send frame WINDOW_UPDATE: %d, %d, id=%d \n", __func__, __LINE__, ret, resp->frame_win_update.size(), resp->id);
        if (ret == ERR_TRY_AGAIN)
            return 0;
        close_stream(con, resp->id);
        return -1;
    }

    resp->cgi.window_update = 0;
    con->sock_timer = 0;
    resp->frame_win_update.init();
    resp->send_ready &= (~FRAME_WINUPDATE_STREAM_READY);

    return 0;
}
//======================================================================
int EventHandlerClass::recv_from_client(Connect *con)
{
    con->h2.init();
    int n = read_from_client(con, con->h2.header + con->h2.header_len, 9 - con->h2.header_len);
    if (n < 0)
    {
        if (n == ERR_TRY_AGAIN)
        {
            return 0;
        }
        else
        {
            print_err(con, "<%s:%d> Error read_from_client\n", __func__, __LINE__);
            return -1;
        }
    }
    else if (n == 9)
    {
        if (conf->PrintDebugMsg)
            hex_print_stderr(__func__, __LINE__, con->h2.header, 9);

        con->h2.body_len = ((unsigned char)con->h2.header[0]<<16) +
            ((unsigned char)con->h2.header[1]<<8) + (unsigned char)con->h2.header[2];
        con->h2.type = (FRAME_TYPE)con->h2.header[3];
        con->h2.flags = con->h2.header[4];
        con->h2.id = (((unsigned char)con->h2.header[5] & 0x7f)<<16) + ((unsigned char)con->h2.header[6]<<16) +
            ((unsigned char)con->h2.header[7]<<8) + (unsigned char)con->h2.header[8];
        con->h2.init();

        char buf[con->h2.body_len];
        if (con->h2.body_len > 0)
        {
            int read_body = read_from_client(con, buf, con->h2.body_len);
            if (read_body != con->h2.body_len)
            {
                if (read_body == ERR_TRY_AGAIN)
                {
                    return 0;
                }

                if (con->h2.type == HEADERS) /* -------------- */
                {
                    send_rst_stream(con, con->h2.id);
                }

                return -1;
            }

            con->h2.body.cat(buf, read_body);
        }

        if (con->h2.type == SETTINGS)
        {
            con->sock_timer = 0;
            if (conf->PrintDebugMsg)
            {
                hex_print_stderr("recv SETTINGS", __LINE__, con->h2.header, 9);
                hex_print_stderr("recv SETTINGS", __LINE__, con->h2.body.ptr(), con->h2.body.size());
            }

            for (int i = 0; i < (con->h2.body_len/6); ++i)
            {
                int ind = i * 6;
                if (buf[ind + 1] == 4)
                {
                    con->h2.init_window_size = (unsigned char)buf[ind + 5];
                    con->h2.init_window_size += ((unsigned char)buf[ind + 4]<<8);
                    con->h2.init_window_size += ((unsigned char)buf[ind + 3]<<16);
                    con->h2.init_window_size += ((unsigned char)buf[ind + 2]<<24);
                    con->h2.window_update = con->h2.init_window_size;
                }
            }

            if (con->h2.header[4] == FLAG_ACK)
            {
                if (conf->PrintDebugMsg)
                    print_err(con, "<%s:%d> recv SETTINGS flag=ACK\n", __func__, __LINE__);
                con->h2.ack_recv = true;
                con->h2.type_op = WORK_STREAM;
            }
            else
            {
                con->h2.settings.cat("\x00\x00\x00\x04\x01\x00\x00\x00\x00", 9);
                con->h2.type_op = SEND_SETTINGS;
            }
        }
        else if (con->h2.type == DATA)
        {
            con->sock_timer = 0;
            Stream *resp = con->h2.get(con->h2.id);
            if (!resp)
            {
                print_err(con, "<%s:%d> Error list.get(id=%d), h2.body_len=%d, flag=%d\n", __func__, __LINE__,
                                con->h2.id, con->h2.body_len, (int)con->h2.header[4]);
                return -1;
            }

            if ((con->h2.body_len == 0) && (con->h2.header[4] == 1))
            {
                if ((resp->cgi.cgi_type <= PHPCGI) || (resp->cgi.cgi_type == SCGI))
                {
                    resp->cgi.op = CGI_STDOUT;
                }

                if (resp->cgi.to_script > 0)
                    close(resp->cgi.to_script);
                resp->cgi.to_script = -1;
                return 0;
            }

            if (con->h2.header[4] & FLAG_END_STREAM)
            {
                resp->cgi.end_post_data = true;
            }

            const char *p_buf = NULL;
            if (con->h2.header[4] & FLAG_PADDED)
            {
                unsigned int padd = (unsigned char)buf[0];
                con->h2.body_len -= padd;
                p_buf = buf + 1;
            }
            else
                p_buf = buf;
            if (conf->PrintDebugMsg)
            {
                if (con->h2.body_len < 100)
                    print_err(resp, "<%s:%d> +++ DATA %d, con.serv_wind_size=%ld, stream.wind_update=%ld, id=%d \n", __func__, __LINE__, con->h2.body_len, con->h2.server_window_size, resp->cgi.window_update, resp->id);
            }

            con->h2.server_window_size += con->h2.body_len;
            resp->cgi.window_update += con->h2.body_len;

            if ((resp->cgi.cgi_type <= PHPCGI) || (resp->cgi.cgi_type == SCGI))
            {
                resp->post_data.cat(p_buf, con->h2.body_len);
            }
            else if ((resp->cgi.cgi_type == FASTCGI) || (resp->cgi.cgi_type == PHPFPM))
            {
                char s[8];
                fcgi_set_header(s, FCGI_STDIN, con->h2.body_len);
                resp->post_data.cat(s, 8);
                if (con->h2.body_len)
                {
                    resp->post_data.cat(p_buf, con->h2.body_len);
                    if (resp->cgi.end_post_data)
                        resp->post_data.cat("\1\5\0\1\0\0\0\0", 8);
                }

                if (resp->cgi.fcgiContentLen)
                {
                    print_err(resp, "<%s:%d> !!! resp->cgi.fcgiContentLen=%d, id=%d \n", __func__, __LINE__,
                            resp->cgi.fcgiContentLen, resp->id);
                }
                resp->cgi.fcgiContentLen += con->h2.body_len;
            }

            if (resp->post_cont_length < 0)
            {
                print_err(resp, "<%s:%d> !!! Error: cont_length=%lld, h2.body_len=%d, size=%d, id=%d \n", __func__, __LINE__,
                            resp->post_cont_length, con->h2.body_len, resp->post_data.size(), resp->id);
                send_rst_stream(con, con->h2.id);
                close_stream(con, con->h2.id);
                return -1;
            }
        }
        else if (con->h2.type == HEADERS)
        {
            con->sock_timer = 0;
            con->h2.numReq++;
            Stream *resp = con->h2.add();
            if (resp == NULL)
            {
                print_err(con, "<%s:%d> Error id=%d \n", __func__, __LINE__, con->h2.id);
                send_rst_stream(con, con->h2.id);
                close_stream(con, con->h2.id);
                return 0;
            }

            if (set_response(con, resp))
            {
                print_err(resp, "<%s:%d> !!! Error set_response id=%d \n", __func__, __LINE__, con->h2.id);
                send_rst_stream(con, con->h2.id);
                close_stream(con, con->h2.id);
                return 0;
            }

            //if (conf->PrintDebugMsg)
                print_err(resp, "\"%s\" new request headers.size=%d, id=%d \n", resp->decode_path.c_str(), resp->headers.size(), resp->id);
        }
        else if (con->h2.type == GOAWAY)
        {
            //if (conf->PrintDebugMsg)
                print_err(con, "<%s:%d> GOAWAY [%u] id=%d \n", __func__, __LINE__, (unsigned int)buf[7], con->h2.id);
            return -1;
        }
        else if (con->h2.type == PING)
        {
            con->sock_timer = 0;
            print_err(con, "<%s:%d> recv PING id=%d \n", __func__, __LINE__, con->h2.id);
            con->h2.frame.cpy("\x0\x0\x8\x6\x1\x0\x0\x0\x0", 9);
            con->h2.frame.cat(buf, 8);
            int ret = write_to_client(con, con->h2.frame.ptr(), con->h2.frame.size(), 0);
            if (ret <= 0)
            {
                print_err(con, "<%s:%d> Error send frame %s,   id=%d \n", __func__, __LINE__, get_str_frame_type(con->h2.type), con->h2.id);
                return -1;
            }

            con->h2.frame.init();
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
                con->h2.window_update += n;
                if (conf->PrintDebugMsg)
                    print_err(con, "<%s:%d> WINDOW_UPDATE %ld[%ld] id=%d \n", __func__, __LINE__, n, con->h2.window_update, con->h2.id);
            }
            else
            {
                con->h2.set_window_size(con->numConn, con->h2.id, n);
                if (conf->PrintDebugMsg)
                    print_err(con, "<%s:%d> WINDOW_UPDATE %ld id=%d \n", __func__, __LINE__, n, con->h2.id);
            }
        }
        else if (con->h2.type == RST_STREAM)
        {
            con->sock_timer = 0;
            print_err(con, "<%s:%d> RST_STREAM [%u] id=%d \n", __func__, __LINE__, (unsigned int)buf[3], con->h2.id);
            Stream *str = con->h2.get(con->h2.id);
            if (str)
                str->rst_stream = true;
            else
                print_err(con, "<%s:%d> RST_STREAM Error stream id=%d does not exist\n", __func__, __LINE__, con->h2.id);
        }
        else if (con->h2.type == PRIORITY)
        {
            con->sock_timer = 0;
            if (conf->PrintDebugMsg)
                print_err(con, "<%s:%d> PRIORITY id=%d\n", __func__, __LINE__, con->h2.id);
        }
        else
        {
            con->sock_timer = 0;
            //if (conf->PrintDebugMsg)
                print_err(con, "<%s:%d> frame type: %d, id=%d \n", __func__, __LINE__, con->h2.type, con->h2.id);
        }
    }
    else
    {
        print_err(con, "<%s:%d> Error read frame header\n", __func__, __LINE__);
        return -1;
    }
    return 0;
}
//======================================================================
void EventHandlerClass::http2_worker_streams(Connect *con)
{
    if (con->h2.server_window_size > 0)
    {
        if (con->h2.server_window_size > 32000)
        {
            print_err(con, "<%s:%d> ??? con->h2.server_window_size(%ld) > 16000\n", __func__, __LINE__, con->h2.server_window_size);
        }

        set_frame_window_update(con, con->h2.server_window_size);
        if (send_window_update(con) < 0)
        {
            ssl_shutdown(con);
            return;
        }
    }

    int n = con->h2.size();
    if (n > conf->MaxConcurrentStreams)
    {
        print_err(con, "<%s:%d> ??? h2.size()=%d\n", __func__, __LINE__, n);
        ssl_shutdown(con);
        return;
    }
    else if (n == 0)
        return;

    if (con->h2.next == NULL)
        return;
    Stream *resp = con->h2.next;
    if (resp == NULL)
    {
        if ((resp = con->h2.start) == NULL)
        {
            print_err(con, "<%s:%d> ??? resp == NULL\n", __func__, __LINE__);
            return;
        }
    }

    if (resp->cgi.window_update > 0)
    {
        if (resp->cgi.window_update > 32000)
        {
            if (conf->PrintDebugMsg)
                print_err(resp, "<%s:%d> ??? resp->cgi.window_update(%ld) > 16000\n", __func__, __LINE__, resp->cgi.window_update);
        }

        set_frame_window_update(resp, resp->cgi.window_update);
        if (send_window_update(con, resp) < 0)
        {
            ssl_shutdown(con);
            return;
        }
    }

    if (resp->send_ready & FRAME_HEADERS_READY)
    {
        int ret = send_headers(con, resp);
        if (ret < 0)
        {
            if (ret == ERR_TRY_AGAIN)
                return;
            ssl_shutdown(con);
            return;
        }
    }

    int ret = send_response(con, resp);
    if (ret < 0)
    {
        if (ret == ERR_TRY_AGAIN)
            return;
        ssl_shutdown(con);
        return;
    }

    if (con->h2.next)
        con->h2.next = con->h2.next->next;
}
//======================================================================
int EventHandlerClass::send_headers(Connect *con, Stream *resp)
{
    if (resp->headers.size() && (!resp->send_headers))
    {
        int ret = write_to_client(con, resp->headers.ptr(), resp->headers.size(), resp->id);
        if (ret <= 0)
        {
            if (ret == ERR_TRY_AGAIN)
            {
                print_err(resp, "<%s:%d> Error send frame HEADERS: SSL_ERROR_WANT_WRITE, id=%d \n", __func__, __LINE__, resp->id);
                return ERR_TRY_AGAIN;
            }

            print_err(resp, "<%s:%d> Error send frame HEADERS: (%p)%d, id=%d \n", __func__, __LINE__, resp, ret, resp->id);
            return -1;
        }
        else if (ret != resp->headers.size())
        {
            print_err(resp, "<%s:%d> !!! send frame HEADERS: %d/%d, id=%d \n", __func__, __LINE__, 
                                ret, resp->headers.size(), resp->id);
            return -1;
        }

        resp->send_ready &= (~FRAME_HEADERS_READY);
        con->sock_timer = 0;
        resp->send_headers = true;
        resp->headers.init();
        return 0;
    }
    else
    {
        print_err(resp, "<%s:%d> !!! Error id=%d\n", __func__, __LINE__, resp->id);
        return -1;
    }
}
//=============================================================================================================================
int EventHandlerClass::send_response(Connect *con, Stream *resp)
{
    if (resp->send_headers == false)
        return 0;

    if (resp->data.size() == 0)
    {
        if (resp->rst_stream)
        {
            set_frame_data(resp, 0, FLAG_END_STREAM);
        }
        else
        {
            if (con->h2.window_update <= 0)
            {
                //print_err(resp, "<%s:%d> !!! h2.window_update(%ld) <= 0, id=%d \n", __func__, __LINE__, con->h2.window_update, resp->id);
                return 0;
            }

            if (resp->window_update <= 0)
            {
                //print_err(resp, "<%s:%d> !!! window_update(%ld) <= 0, %ld, id=%d \n", __func__, __LINE__, resp->window_update, con->h2.window_update, resp->id);
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
    if (ret <= 0)
    {
        if (ret == ERR_TRY_AGAIN)
        {
            print_err(resp, "<%s:%d> Error send frame DATA: %d, %d, id=%d \n", __func__, __LINE__,
                                                ret, resp->data.size(), resp->id);
            return ERR_TRY_AGAIN;
        }

        print_err(resp, "<%s:%d> Error send frame DATA: %d, %d, send_bytes=%lld, id=%d \n", __func__, __LINE__,
                                                ret, resp->data.size(), resp->send_bytes, resp->id);
        return -1;
    }

    resp->send_ready &= (~FRAME_DATA_READY);
    con->sock_timer = 0;
    resp->send_bytes += (ret - 9);
    resp->window_update -= (ret - 9);
    con->h2.window_update -= (ret - 9);

    if (ret != resp->data.size())
    {
        print_err(resp, "<%s:%d> Error send frame DATA send %d bytes, size=%d, id=%d \n", 
                    __func__, __LINE__, ret, resp->data.size(), resp->id);
        resp->data.init();
        return -1;
    }

    if (resp->data.get_byte(4) == FLAG_END_STREAM)
    {
        if (conf->PrintDebugMsg)
        {
            print_err(resp, "<%s:%d>... send frame DATA, END_STREAM, end request send %lld bytes, data.size=%d ... id=%d \n", 
                        __func__, __LINE__, resp->send_bytes, resp->data.size(), resp->id);
        }

        resp->data.init();
        resp->send_end_stream = true;
        print_log(con->h2.get(resp->id));
        close_stream(con, resp->id);
        return 0;
    }
    else
        resp->data.init();

    return 0;
}
//======================================================================
const char *huf_map[][2] = {
                        {" ", "010100"},
                        {"!", "1111111000"},
                        {"\"", "1111111001"},
                        {"#", "111111111010"},
                        {"$", "1111111111001"},
                        {"%%", "010101"},
                        {"&", "11111000"},
                        {"'", "11111111010"},
                        {"(", "1111111010"},
                        {")", "1111111011"},
                        {"*", "11111001"},
                        {"+", "11111111011"},
                        {",", "11111010"},
                        {"-", "010110"},
                        {".", "010111"},
                        {"/", "011000"},
                        {"0", "00000"},
                        {"1", "00001"},
                        {"2", "00010"},
                        {"3", "011001"},
                        {"4", "011010"},
                        {"5", "011011"},
                        {"6", "011100"},
                        {"7", "011101"},
                        {"8", "011110"},
                        {"9", "011111"},
                        {":", "1011100"},
                        {";", "11111011"},
                        {"<", "111111111111100"},
                        {"=", "100000"},
                        {">", "111111111011"},
                        {"?", "1111111100"},
                        {"@", "1111111111010"},
                        {"A", "100001"},
                        {"B", "1011101"},
                        {"C", "1011110"},
                        {"D", "1011111"},
                        {"E", "1100000"},
                        {"F", "1100001"},
                        {"G", "1100010"},
                        {"H", "1100011"},
                        {"I", "1100100"},
                        {"J", "1100101"},
                        {"K", "1100110"},
                        {"L", "1100111"},
                        {"M", "1101000"},
                        {"N", "1101001"},
                        {"O", "1101010"},
                        {"P", "1101011"},
                        {"Q", "1101100"},
                        {"R", "1101101"},
                        {"S", "1101110"},
                        {"T", "1101111"},
                        {"U", "1110000"},
                        {"V", "1110001"},
                        {"W", "1110010"},
                        {"X", "11111100"},
                        {"Y", "1110011"},
                        {"Z", "11111101"},
                        {"[", "1111111111011"},
                        {"\\", "1111111111111110000"},
                        {"]", "1111111111100"},
                        {"^", "11111111111100"},
                        {"_", "100010"},
                        {"`", "111111111111101"},
                        {"a", "00011"},
                        {"b", "100011"},
                        {"c", "00100"},
                        {"d", "100100"},
                        {"e", "00101"},
                        {"f", "100101"},
                        {"g", "100110"},
                        {"h", "100111"},
                        {"i", "00110"},
                        {"j", "1110100"},
                        {"k", "1110101"},
                        {"l", "101000"},
                        {"m", "101001"},
                        {"n", "101010"},
                        {"o", "00111"},
                        {"p", "101011"},
                        {"q", "1110110"},
                        {"r", "101100"},
                        {"s", "01000"},
                        {"t", "01001"},
                        {"u", "101101"},
                        {"v", "1110111"},
                        {"w", "1111000"},
                        {"x", "1111001"},
                        {"y", "1111010"},
                        {"z", "1111011"},
                        {"{", "111111111111110"},
                        {"|", "11111111100"},
                        {"}", "11111111111101"},
                        {"~", "1111111111101"},
                        {NULL, NULL}};
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
