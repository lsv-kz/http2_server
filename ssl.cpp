#include "main.h"

using namespace std;

//======================================================================
void init_openssl()
{
    SSL_library_init();
    OpenSSL_add_ssl_algorithms();
    //OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_crypto_strings();
}
//======================================================================
void cleanup_openssl()
{
    EVP_cleanup();
}
//======================================================================
SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    method = TLS_server_method();
    //method = TLSv1_2_server_method();
    return SSL_CTX_new(method);
}
//======================================================================
static int select_alpn(const unsigned char **out, unsigned char *outlen,
                       const unsigned char *in, unsigned int inlen,
                       const char *key, unsigned int keylen)
{
    unsigned int i;
    for (i = 0; i + keylen <= inlen; i += (unsigned int)(in[i] + 1))
    {
        if (memcmp(&in[i], key, keylen) == 0)
        {
            *out = (unsigned char *)&in[i + 1];
            *outlen = in[i];
            if (conf->PrintDebugMsg)
                hex_print_stderr(__func__, __LINE__, &in[i + 1], in[i]);
            return 0;
        }
        else
            hex_print_stderr(__func__, __LINE__, &in[i + 1], in[i]);
    }
    return -1;
}
//======================================================================
int select_next_protocol(unsigned char **out, unsigned char *outlen,
                                 const unsigned char *in, unsigned int inlen)
{
    const char proto_alpn[] = { 2, 'h', '2' };
    unsigned int proto_alpn_len = sizeof(proto_alpn);

    if (select_alpn((const unsigned char **)out, outlen, in, inlen,
                  proto_alpn, proto_alpn_len) == 0)
    {
        return 1;
    }

    return 0;
}
//======================================================================
static int alpn_select_proto_cb(SSL *ssl, const unsigned char **out,
                                unsigned char *outlen, const unsigned char *in,
                                unsigned int inlen, void *arg)
{
    if (conf->PrintDebugMsg)
        hex_print_stderr(__func__, __LINE__, in, inlen);
    int rv = select_next_protocol((unsigned char **)out, outlen, in, inlen);
    if (rv == 0)
    {
        fprintf(stderr, "<%s:%d> alpn_select_proto_cb(): SSL_TLSEXT_ERR_NOACK\n", __func__, __LINE__);
        return SSL_TLSEXT_ERR_NOACK;
    }

    return SSL_TLSEXT_ERR_OK;
}
//======================================================================
int configure_context(SSL_CTX *ctx)
{
    if (SSL_CTX_use_certificate_file(ctx, "cert/cert.pem", SSL_FILETYPE_PEM) != 1)
    {
        fprintf(stderr, "<%s:%d> SSL_CTX_use_certificate_file failed\n", __func__, __LINE__);
        return -1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "cert/key.pem", SSL_FILETYPE_PEM) != 1)
    {
        fprintf(stderr, "<%s:%d> SSL_CTX_use_PrivateKey_file failed\n", __func__, __LINE__);
        return -1;
    }

    SSL_CTX_set_alpn_select_cb(ctx, alpn_select_proto_cb, NULL);

    return 0;
}
//======================================================================
SSL_CTX *Init_SSL(void)
{
    SSL_CTX *ctx;
    init_openssl();
    ctx = create_context();
    if (!ctx)
    {
        fprintf(stderr, "Unable to create SSL context\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (configure_context(ctx))
    {
        fprintf(stderr, "Error configure_context()\n");
        exit(EXIT_FAILURE);
    }
    else
        return ctx;
}
//======================================================================
const char *ssl_strerror(int err)
{
    switch (err)
    {
        case SSL_ERROR_NONE: // 0
            return "SSL_ERROR_NONE";
        case SSL_ERROR_SSL:  // 1
            return "SSL_ERROR_SSL";
        case SSL_ERROR_WANT_READ:  // 2
            return "SSL_ERROR_WANT_READ";
        case SSL_ERROR_WANT_WRITE:  // 3
            return "SSL_ERROR_WANT_WRITE";
        case SSL_ERROR_WANT_X509_LOOKUP:  // 4
            return "SSL_ERROR_WANT_X509_LOOKUP";
        case SSL_ERROR_SYSCALL:  // 5
            print_err("SSL_ERROR_SYSCALL(%s)\n", strerror(errno));
            return "SSL_ERROR_SYSCALL";
        case SSL_ERROR_ZERO_RETURN:  // 6
            return "SSL_ERROR_ZERO_RETURN";
        case SSL_ERROR_WANT_CONNECT:  // 7
            return "SSL_ERROR_WANT_CONNECT";
        case SSL_ERROR_WANT_ACCEPT:  // 8
            return "SSL_ERROR_WANT_ACCEPT";
    }

    return "?";
}
//======================================================================
int ssl_read(Connect *con, char *buf, int len)
{
    ERR_clear_error();
    int ret = SSL_read(con->tls.ssl, buf, len);
    if (ret <= 0)
    {
        con->tls.err = SSL_get_error(con->tls.ssl, ret);
        if (con->tls.err == SSL_ERROR_ZERO_RETURN)
        {
            if (conf->PrintDebugMsg)
                print_err(con, "<%s:%d> Error SSL_read(): SSL_ERROR_ZERO_RETURN\n", __func__, __LINE__);
            return 0;
        }
        else if (con->tls.err == SSL_ERROR_WANT_READ)
        {
            con->tls.err = 0;
            return ERR_TRY_AGAIN;
        }
        else if (con->tls.err == SSL_ERROR_WANT_WRITE)
        {
            print_err(con, "<%s:%d> ??? Error SSL_read(): SSL_ERROR_WANT_WRITE\n", __func__, __LINE__);
            con->tls.err = 0;
            return ERR_TRY_AGAIN;
        }
        else
        {
            print_err(con, "<%s:%d> Error SSL_read(, %p, %d)=%d: %s\n", __func__, __LINE__, buf, len, ret, ssl_strerror(con->tls.err));
            return -1;
        }
    }
    else
        return ret;
}
//======================================================================
int ssl_write(Connect *con, const char *buf, int len, int id)
{
    ERR_clear_error();
    if ((con == NULL) || (buf == NULL) || (len <= 0) || (len > (conf->DataBufSize + 9)))
    {
        print_err("<%s:%d> ??? Error conn=%p, buf=%p, len=%d\n", __func__, __LINE__, con, buf, len);
        return -1;
    }

    int ret = SSL_write(con->tls.ssl, buf, len);
    if (ret <= 0)
    {
        con->tls.err = SSL_get_error(con->tls.ssl, ret);
        if (con->tls.err == SSL_ERROR_WANT_WRITE)
        {
            con->tls.err = 0;
            return ERR_TRY_AGAIN;
        }
        else if (con->tls.err == SSL_ERROR_WANT_READ)
        {
            print_err(con, "<%s:%d> ??? Error SSL_write(): SSL_ERROR_WANT_READ\n", __func__, __LINE__);
            con->tls.err = 0;
            return ERR_TRY_AGAIN;
        }
        print_err(con, "<%s:%d> Error SSL_write(, %p, %d)=%d: %s, errno=%d, id=%d\n", __func__, __LINE__,
                    buf, len, ret, ssl_strerror(con->tls.err), errno, id);
        return -1;
    }

    return ret;
}
