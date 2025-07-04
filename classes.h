#ifndef CLASSES_H_
#define CLASSES_H_
#define _FILE_OFFSET_BITS 64

#include <iostream>
#include "huffman.h"
//======================================================================
struct Header
{
    std::string name;
    std::string val;
};

struct Param
{
    std::string name;
    std::string val;
};

enum FRAME_TYPE
{
    DATA, HEADERS, PRIORITY, RST_STREAM, SETTINGS, PUSH_PROMISE, PING,
    GOAWAY, WINDOW_UPDATE, CONTINUATION, ALTSVC, ORIGIN = 0x0C, CACHE_DIGEST,
};

enum HTTP2_FLAGS { FLAG_ACK = 0x1, FLAG_END_STREAM = 0x1, FLAG_END_HEADERS = 0x4, FLAG_PADDED = 0x8, FLAG_PRIORITY = 0x20 };

enum  CONTENT_TYPE { ERROR_TYPE, DIRECTORY = 1, REGFILE, DYN_PAGE, };

enum CGI_OPERATION { CGI_CREATE, CGI_STDIN, CGI_STDOUT, };
enum SCGI_OPERATION { SCGI_PARAMS, SCGI_STDIN, SCGI_STDOUT };
enum FCGI_OPERATION { FASTCGI_BEGIN, FASTCGI_PARAMS, FASTCGI_STDIN, FASTCGI_STDOUT };

enum CGI_TYPE { CGI, PHPCGI, PHPFPM, FASTCGI, SCGI, };

const int FRAME_DATA_READY = 0x01, FRAME_HEADERS_READY = 0x02, FRAME_WINUPDATE_CONNECT_READY = 0x04, FRAME_WINUPDATE_STREAM_READY = 0x08;
const int FRAME_PING_READY = 0x10, FRAME_SETTINGS_READY = 0x20;

extern const char *static_tab[][2];

void print_err(const char *format, ...);
//======================================================================
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
    bool PrintDebugMsg;

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

    int ListenBacklog;
    bool TcpNoDelay;

    int DataBufSize;

    int MaxAcceptConnections;

    int MaxCgiProc;

    int Timeout;
    int TimeoutKeepAlive;
    int TimeoutPoll;
    int TimeoutCGI;

    long int ClientMaxBodySize;

    bool ShowMediaFiles;

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

extern const Config* const conf;
//======================================================================
struct Stream
{
    Stream *prev;
    Stream *next;

    unsigned long numConn;
    unsigned long numReq;
    
    std::string remoteAddr;
    std::string remotePort;

    int id;
    int len;
    FRAME_TYPE type;
    int flags;
    time_t Time;

    long window_update;

    std::string method;
    std::string path;
    std::string decode_path;
    char uri[4096];

    std::string authority;
    std::string host;
    std::string user_agent;
    std::string referer;
    std::string range;
    std::string content_type;
    std::string content_length;

    ByteArray headers;
    ByteArray data;
    ByteArray post_data;
    ByteArray html;
    ByteArray frame_win_update;

    int status;
    const char *respContentType;
    long long offset;
    long long send_cont_length;
    long long post_cont_length;
    long long file_size;

    CONTENT_TYPE content;
    int fd;

    long long send_bytes;

    bool rst_stream;
    bool create_headers;
    bool send_headers;
    int send_ready;

    bool send_end_stream;

    struct
    {
        CGI_TYPE cgi_type;
        CGI_OPERATION op;
        bool end_post_data;
        bool start;
        bool cgi_end;
        time_t timer;

        std::string path;
        std::string script_name;
        std::string query_string;
        pid_t pid;

        // CGI, PHPCGI
        int to_script;
        int from_script;

        // PHPFPM, FASTCGI, SCGI
        const std::string *socket;
        int fd;
        int i_param;
        int size_par;
        std::vector <Param> vPar;
        ByteArray buf_param;

        SCGI_OPERATION scgi_op;

        FCGI_OPERATION fcgi_op;
        int fcgi_type;
        int fcgiContentLen;
        int fcgiPaddingLen;
        //-----------------------
        long recv_from_cgi;
        long send_to_cgi;
        long window_update;
    } cgi;

    Stream()
    {
        numConn = numReq = 0;
        status = 0;
        uri[0] = 0;
        id = 0;
        Time = time(NULL);
        fd = -1;
        window_update = 0;
        rst_stream = create_headers = send_headers = send_end_stream = false;
        send_ready = 0;
        offset = 0;
        file_size = 0;
        send_cont_length = post_cont_length = 0;

        cgi.start = cgi.cgi_end = cgi.end_post_data = false;
        cgi.fd = cgi.to_script = cgi.from_script = -1;
        cgi.fcgi_type = 0;
        cgi.fcgiContentLen = cgi.fcgiPaddingLen = 0;
        cgi.timer = 0;
        cgi.send_to_cgi = cgi.recv_from_cgi = 0;
        cgi.window_update = 0;
    }

    ~Stream()
    {
        //if (conf->PrintDebugMsg == 'y')
        {
            if ((send_bytes != file_size) && (content == REGFILE))
                print_err("<%s:%d> !!! ~Resp(%s), send_bytes=%lld(%lld), wind_update=%ld, id=%d\n", 
                        __func__, __LINE__, uri, send_bytes, file_size, window_update, id);
        }

        if (fd > 0)
        {
            close(fd);
            fd = -1;
        }
        if (cgi.fd > 0)
        {
            close(cgi.fd);
            cgi.fd = 1;
        }
        if (cgi.to_script > 0)
        {
            close(cgi.to_script);
            cgi.to_script = -1;
        }
        if (cgi.from_script > 0)
        {
            close(cgi.from_script);
            cgi.from_script = -1;
        }
    }

private:
    Stream(const Stream&);
    Stream& operator=(const Stream&);
};
//======================================================================
class DynamicTable
{
    Header *table;
    int table_size;
    int table_len;
    int offset;
    int err;

    DynamicTable();
    DynamicTable(const DynamicTable&);
    DynamicTable& operator= (const DynamicTable&);

public:

    DynamicTable(int n, int offs)
    {
        table_size = n;
        offset = offs;
        table_len = err = 0;
        table = new(std::nothrow) Header [table_size];
        if (!table)
        {
            fprintf(stderr, "<%s:%d> Error: %s\n", __func__, __LINE__, strerror(errno));
            table_size = 0;
            err = 1;
            return;
        }
    }
    //------------------------------------------------------------------
    ~DynamicTable()
    {
        if (table)
        {
            fprintf(stderr, "<%s:%d> delete dyn table\n", __func__, __LINE__);
            delete [] table;
            table = NULL;
        }
    }
    //------------------------------------------------------------------
    void add(const char *name, const char *val)
    {
        if (table_len == table_size)
            --table_len;

        for ( int i = table_len; i > 0; --i)
        {
            table[i] = table[i - 1];
        }

        table[0] = Header{name, val};
        ++table_len;
    }
    //------------------------------------------------------------------
    void print()
    {
        for ( int i = 0; i < table_len; ++i)
        {
            fprintf(stderr, " name: %s, val: %s\n", table[i].name.c_str(), table[i].val.c_str());
        }
    }
    //------------------------------------------------------------------
    Header *get(int n)
    {
        if ((n < offset) || (n >= (table_len + offset)))
        {
            fprintf(stderr, "<%s:%d> Error ind=%d\n", __func__, __LINE__, n);
            return NULL;
        }

        return &table[n - offset];
    }
};

#endif
