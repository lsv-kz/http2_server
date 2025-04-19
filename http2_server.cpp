#include "main.h"

using namespace std;
//======================================================================
static int sockServer = -1;
int Connect::serverSocket;

int create_server_socket(const Config *c);
void manager(int);
void free_fcgi_list();

static string pidFile;
const char *nameConfifFile = "http2_server.conf";
static string confPath;
static string cwd;
static string myFileName;
//======================================================================
static void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        fprintf(stderr, "[%s] - <%s> ####### SIGINT #######\n", log_time().c_str(), __func__);
        shutdown(sockServer, SHUT_RDWR);
        close(sockServer);
        sockServer = -1;
    }
    else if (signo == SIGSEGV)
    {
        fprintf(stderr, "[%s] - <%s> ####### SIGSEGV #######\n", log_time().c_str(), __func__);
        abort();
    }
    else
        fprintf(stderr, "[%s] - <%s> ? signo=%d (%s)\n", log_time().c_str(), __func__, signo, strsignal(signo));
}
//======================================================================
int main(int argc, char *argv[])
{
    pid_t pid;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        fprintf(stderr, "<%s:%d> Error signal(SIGPIPE): %s\n", __func__, __LINE__, strerror(errno));
        return 1;
    }

    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
    {
        fprintf(stderr, "<%s:%d> Error signal(SIGCHLD): %s\n", __func__, __LINE__, strerror(errno));
        return 1;
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "<%s:%d> Error signal(SIGINT): %s\n", __func__, __LINE__, strerror(errno));
        return 1;
    }

    if (signal(SIGSEGV, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "<%s:%d> Error signal(SIGSEGV): %s\n", __func__, __LINE__, strerror(errno));
        return 1;
    }
    //------------------------------------------------------------------
    confPath = nameConfifFile;
    if (read_conf_file(confPath.c_str()))
        return 1;
    //------------------------------------------------------------------
    sockServer = create_server_socket(conf);
    if (sockServer == -1)
    {
        fprintf(stderr, "<%s:%d> Error: create_server_socket(%s:%s)\n", __func__, __LINE__,
                    conf->ServerAddr.c_str(), conf->ServerPort.c_str());
        return 1;
    }

    Connect::serverSocket = sockServer;
    //------------------------------------------------------------------
    create_logfiles(conf->LogPath);
    //------------------------------------------------------------------
    pid = getpid();
    cout << "\n[" << get_time().c_str() << "] - server \"" << conf->ServerSoftware.c_str()
         << "\" run, port: " << conf->ServerPort.c_str()
         << "\nhardware_concurrency = " << thread::hardware_concurrency() << "\n";
    {
        SSL  *ssl = SSL_new(conf->ctx);
        cout << "   SSL version: " << SSL_get_version(ssl) << "\n";
        SSL_free(ssl);
    }

    pid_t gid = getgid();
    cout << "pid="  << pid << "; uid=" << getuid() << "; gid=" << gid << "\n";
    cerr << "   pid="  << pid << "; uid=" << getuid() << "; gid=" << gid
         << "\n   NumCpuCores: " << thread::hardware_concurrency() << "\n";
    cout << "   ------------- FastCGI/SCGI -------------\n";
    fcgi_list_addr *i = conf->fcgi_list;
    for (; i; i = i->next)
    {
        cout << "   [" << i->script_name.c_str() << " : " << i->addr.c_str() << "] - " << get_cgi_type(i->type) << "\n";
    }
    //------------------------------------------------------------------
    for ( int i = 0; environ[i]; )
    {
        char *p, buf[512];
        if ((p = (char*)memccpy(buf, environ[i], '=', strlen(environ[i]))))
        {
            if (strstr(environ[i], "DISPLAY") || strstr(environ[i], "XDG_RUNTIME_DIR") ||
                strstr(environ[i], "HOME") ||
                strstr(environ[i], "SESSION_MANAGER") ||
                strstr(environ[i], "PATH")
             )
            {
                i++;
                continue;
            }

            *(p - 1) = 0;
            unsetenv(buf);
        }
    }
    //------------------------------------------------------------------
    manager(sockServer);

    if (sockServer > 0)
    {
        shutdown(sockServer, SHUT_RDWR);
        close(sockServer);
        sockServer = -1;
    }

    SSL_CTX_free(conf->ctx);
    cleanup_openssl();

    return 0;
}
