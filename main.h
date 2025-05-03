#ifndef SERVER_H_
#define SERVER_H_
#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <climits>
#include <iomanip>
#include <vector>

#include <mutex>
#include <thread>
#include <condition_variable>

#include <errno.h>
#include <signal.h>
#include <stdarg.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>
#include <sys/resource.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "string__.h"
#include "classes.h"

const int  MAX_PATH = 2048;
const int  MAX_NAME = 256;
const int  SIZE_BUF_REQUEST = 8192;
const int  MAX_HEADERS = 25;
const int  ERR_TRY_AGAIN = -1000;
const int  LIMIT_WORK_THREADS = 16;
const int  LIMIT_PARSE_REQ_THREADS = 128;
const int  MAX_STREAM = 128;

enum {
    RS101 = 101,
    RS200 = 200,RS204 = 204,RS206 = 206,
    RS301 = 301, RS302,
    RS400 = 400,RS401,RS402,RS403,RS404,RS405,RS406,RS407,
    RS408,RS411 = 411,RS413 = 413,RS414,RS415,RS416,RS417,RS418,RS429 = 429,
    RS500 = 500,RS501,RS502,RS503,RS504,RS505
};
enum {
    M_GET = 1, M_HEAD, M_POST, M_OPTIONS, M_PUT,
    M_PATCH, M_DELETE, M_TRACE, M_CONNECT
};

enum OPERATION_HTTP2 { SSL_ACCEPT = 1, PREFACE_MESSAGE, SEND_SETTINGS, WORK_STREAM, SSL_SHUTDOWN, };

enum IO_STATUS { WAIT = 1, WORK };
enum DIRECT { TIME_OUT, FROM_CLIENT, TO_CLIENT };

//----------------------------------------------------------------------
#define FCGI_KEEP_CONN  1
#define FCGI_RESPONDER  1

#define FCGI_VERSION_1           1
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE            (FCGI_UNKNOWN_TYPE)
#define requestId               1
//----------------------------------------------------------------------
typedef struct fcgi_list_addr
{
    std::string script_name;
    std::string addr;
    CGI_TYPE type;
    struct fcgi_list_addr *next;
} fcgi_list_addr;
//----------------------------------------------------------------------
class Config
{
    Config(const Config&){}
    Config& operator=(const Config&);

public:

    SSL_CTX *ctx;

    std::string ServerSoftware;

    std::string ServerAddr;
    std::string ServerPort;

    std::string DocumentRoot;
    std::string ScriptPath;
    std::string LogPath;
    std::string PidFilePath;

    std::string UsePHP;
    std::string PathPHP;

    int MaxConcurrentStreams;
    int InitialWindowSize;

    int ListenBacklog;
    char TcpNoDelay;

    int DataBufSize;

    int MaxAcceptConnections;

    int MaxCgiProc;

    int Timeout;
    int TimeoutPoll;
    int TimeoutCGI;

    long int ClientMaxBodySize;

    char ShowMediaFiles;

    std::string user;
    std::string group;

    uid_t server_uid;
    gid_t server_gid;

    fcgi_list_addr *fcgi_list;
    //------------------------------------------------------------------
    Config()
    {
        fcgi_list = NULL;
    }

    ~Config()
    {
        free_fcgi_list();
    }

    void free_fcgi_list()
    {
        fcgi_list_addr *t;
        while (fcgi_list)
        {
            t = fcgi_list;
            fcgi_list = fcgi_list->next;
            if (t)
                delete t;
        }
    }
};
//======================================================================
extern const Config* const conf;
//======================================================================
struct http2
{
    Response *start;
    Response *end;

    long init_window_size;
    long max_frame_size;
    OPERATION_HTTP2 type_op;

    int body_len;
    FRAME_TYPE type;
    int flags;
    int id;

    long window_update;
    long server_window_size;

    char header[9];
    int header_len;

    ByteArray body;
    ByteArray frame;
    ByteArray settings;
    ByteArray frame_win_update;

    bool ack_recv;

    int send_ready;

    int num_cgi;

    void init()
    {
        header_len = 0;
        body.init();
    }

    void init(int n)
    {
        size_ = n;
    }

    Response *add();
    void del_from_list(Response *r);
    int close_stream(http2 *h2, int id, int *num_cgi);
    int set_window_size(unsigned long num_conn, int id, long n);
    Response *get(int id);
    Response *get();
    int size();
    int pow_(int x, int y);
    int bytes_to_int(unsigned char prefix, const char *s, int *len, int size);
    int parse(Response *r);

    http2()
    {
        ack_recv = false;
        header_len = id = body_len = 0;
        window_update = 0;
        init_window_size = 0;
        server_window_size = 65535;
        max_frame_size = 0;
        num_cgi = 0;
        send_ready = 0;

        size_ = 0;
        len_ = err = 0;
        start = end = NULL;

        settings.cpy("\x00\x00\xc\x04\x00\x00\x00\x00\x00" // SETTINGS (type=0x4)
                    "\x00\x01\x00\x00\x00\x00"             // SETTINGS_HEADER_TABLE_SIZE (0x1)
                    "\x00\x03\x00\x00\x00\x00", 9 + 12);   // SETTINGS_MAX_CONCURRENT_STREAMS (0x3)

        settings.set_byte((conf->MaxConcurrentStreams>>24) & 0xff, 17);
        settings.set_byte((conf->MaxConcurrentStreams>>16) & 0xff, 18);
        settings.set_byte((conf->MaxConcurrentStreams>>8) & 0xff, 19);
        settings.set_byte(conf->MaxConcurrentStreams & 0xff, 20);

        settings.cat("\x00\x00\x00\x04\x01\x00\x00\x00\x00", 9);
    }

    ~http2()
    {
        Response *r = start, *next = NULL;
        for ( ; r; r = next)
        {
            next = r->next;
            delete r;
        }

        start = end = NULL;
    }
private:

    HuffmanCode huff;
    int size_;    //SETTINGS_MAX_CONCURRENT_STREAMS (0x3)
    int len_;
    int err;

    http2(const http2&);
    http2& operator=(const http2&);
};

//======================================================================
class Connect
{
    Connect(const Connect&){}
    Connect& operator=(const Connect&);
public:
    Connect(){}
    Connect *prev;
    Connect *next;

    static int serverSocket;

    char remoteAddr[NI_MAXHOST];
    char remotePort[NI_MAXSERV];
    char remoteHost[NI_MAXHOST + NI_MAXSERV];

    unsigned long numConn;
    int    clientSocket;
    int    err;
    time_t sock_timer;

    IO_STATUS io_status;
    int fd_revents;
    int fd_events;

    struct
    {
        SSL *ssl;
        int err;
        int poll_event;
    } tls;

    http2 h2;
    //------------------------------------------------------------------
    void init();
};
//======================================================================
class EventHandlerClass
{
    std::mutex mtx_thr, mtx_cgi;
    std::condition_variable cond_thr;

    int num_wait, num_work, num_cgi;
    int close_thr;
    unsigned long num_request;

    Connect **conn_array;
    struct pollfd *poll_fd;

    Connect *work_list_start;
    Connect *work_list_end;

    Connect *wait_list_start;
    Connect *wait_list_end;

    int send_part_file(Connect *c);
    void del_from_list(Connect *c);

    void choose_worker(Connect *c);
    void worker(Connect *c);

    int http2_worker_connections(Connect *c);
    int http2_worker_streams(Connect *c);

    int send_html(Connect *c);
    int set_pollfd_array(Connect *c, int i);
    int from_client(Connect *c);
    int send_headers(Connect *c, Response *r);
    int send_response(Connect *c, Response *r);

    void cgi_worker(Connect* c, Response *resp);

    int send_window_update(Connect *c);
    int send_window_update(Connect *c, Response *r);

    int cgi_create_proc(Connect *c, Response *r);
    int cgi_fork(Connect *c, Response *r, int* serv_cgi, int* cgi_serv);
    int cgi_stdin(Connect *c, Response *r, int fd);
    int cgi_stdout(Connect *c, Response *r, int fd);
    void cgi_worker(Connect *c, Response *r, struct pollfd *p);

    void scgi_worker(Connect* c, Response *r, struct pollfd *p);
    void fcgi_worker(Connect* c, Response *r, struct pollfd *p);
    void fcgi_get_headers(Connect* con, Response *r);

    void create_message(Response *r, int status);
    
    void close_stream(Connect *c, int id);

public:

    EventHandlerClass();
    ~EventHandlerClass();

    void init(int n);
    int wait_conn();

    void add_work_list();

    void close_connect(Connect *r);
    void ssl_shutdown(Connect *r);

    int cgi_set_poll();
    int set_poll();
    int poll_worker();
    void add_wait_list(Connect *c);
    void push_send_file(Connect *c);
    void push_pollin_list(Connect *c);
    void push_send_multipart(Connect *c);
    void push_send_html(Connect *c);
    void close_event_handler();
};
//----------------------------------------------------------------------
int write_to_fcgi(int fd, const char *buf, int len);
int create_fcgi_socket(Connect *c, Response *r);

int scgi_create_connect(Connect *c, Response *r);
int scgi_create_params(Connect *c, Response *r);
int scgi_set_param(Connect *c, Response *r);
int scgi_set_size_data(Connect* c, Response *r);

void fcgi_set_header(ByteArray* ba, unsigned char type);
int fcgi_create_connect(Connect *c, Response *r);
//----------------------------------------------------------------------
void kill_chld(Response *r);
int cgi_fork(Response *r, int* serv_cgi, int* cgi_serv);
int cgi_create_proc(Connect *c, Response *r);
int iscgi(Connect* conn, Response *resp);
//----------------------------------------------------------------------
extern char **environ;
//----------------------------------------------------------------------
int read_conf_file(const char *path_conf);
int set_max_fd(int max_open_fd);
//----------------------------------------------------------------------
int index_dir(Connect *c, std::string& path, Response *r);
//----------------------------------------------------------------------
int read_from_client(Connect *c, char *buf, int len);
int write_to_client(Connect *c, const char *buf, int len);
int read_request_headers(Connect* c);
//----------------------------------------------------------------------
int encode(const char *s_in, char *s_out, int len_out);
int decode(const char *s_in, int len_in, std::string& s_out);
//----------------------------------------------------------------------
std::string get_time();
std::string get_time(time_t t);
std::string log_time();
std::string log_time(time_t t);

const char *strstr_case(const char * s1, const char *s2);
int strlcmp_case(const char *s1, const char *s2, int len);

const char *get_str_frame_type(FRAME_TYPE);
const char *get_str_operation(OPERATION_HTTP2);
const char *get_cgi_type(CGI_TYPE n);

int clean_path(char *path, int len);
const char *content_type(const char *s);
long long file_size(const char *s);

void set_frame(Response *resp, char *s, int len, int type, HTTP2_FLAGS flags, int id);
void set_frame_headers(Response *r);
void set_frame_data(Response *resp, int len, int flag);
int set_frame_data(Connect *con, Response *r);
void add_header(Response *r, int ind);
void add_header(Response *r, int ind, const char *val);
void set_frame_window_update(Connect *c, int len);
void set_frame_window_update(Response *r, int len);
int send_rst_stream(Connect *c, int id);
int set_response(Connect *c, Response *r);
CONTENT_TYPE get_content_type(const char *path);
int parse_range(const char *s, long long file_size, long long *offset, long long *content_length);

void resp_200(Response *resp);
void resp_204(Response *resp);
void resp_400(Response *resp);
void resp_403(Response *resp);
void resp_404(Response *resp);
void resp_500(Response *resp);
void resp_502(Response *resp);
void resp_504(Response *resp);

const char *status_resp(int st);
//----------------------------------------------------------------------
void create_logfiles(const std::string &);
void close_logs();
void print_err(const char *format, ...);
void print_err(Connect *c, const char *format, ...);
void print_log(Connect *c, Response *r);
//----------------------------------------------------------------------
void ssl_shutdown(Connect *c);
void close_connect(Connect *c);
//----------------------------------------------------------------------
void push_pollin_list(Connect *c);
void event_handler(int);
void close_work_thread();
//----------------------------------------------------------------------
void init_openssl();
void cleanup_openssl();
SSL_CTX *create_context();
int configure_context(SSL_CTX *ctx);
SSL_CTX *Init_SSL();
const char *ssl_strerror(int err);
int ssl_read(Connect *c, char *buf, int len);
int ssl_write(Connect *c, const char *buf, int len);
//----------------------------------------------------------------------


#endif
