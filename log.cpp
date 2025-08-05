#include "main.h"

using namespace std;

static int flog = STDOUT_FILENO, flog_err = STDERR_FILENO;
static mutex mtxLog;
static unsigned int num_log_records = 0, num_logerr_records = 0;
//======================================================================
void create_logfile(const string& log_dir)
{
    char buf[256];
    struct tm tm1;
    time_t t1;

    time(&t1);
    tm1 = *localtime(&t1);
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tm1);

    String fileName;
    fileName << log_dir << '/' << buf << '-' << conf->ServerSoftware << ".log";

    flog = open(fileName.c_str(), O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (flog == -1)
    {
        cerr << " Error create log: " << fileName.c_str() << "\n";
        exit(1);
    }
}
//======================================================================
void create_error_logfile(const string& log_dir)
{
    char buf[256];
    struct tm tm1;
    time_t t1;

    time(&t1);
    tm1 = *localtime(&t1);
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tm1);

    String fileName;
    fileName << log_dir << "/error_" << buf << '_' << conf->ServerSoftware << ".log";

    flog_err = open(fileName.c_str(), O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (flog_err == -1)
    {
        cerr << "  Error create log_err: " << fileName.c_str() << "\n";
        exit(1);
    }

    dup2(flog_err, STDERR_FILENO);
}
//======================================================================
void close_logs()
{
    close(flog);
    close(flog_err);
}
//======================================================================
void print_err(const char *format, ...)
{
    va_list ap;
    char buf[300];

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    String ss(256);
    ss << "[" << log_time() << "] - " << buf;
mtxLog.lock();
    write(flog_err, ss.c_str(), ss.size());
    num_logerr_records++;
    if (num_logerr_records > 500000)
    {
        exit(1);
        close(flog_err);
        create_error_logfile(conf->LogPath);
        num_logerr_records = 0;
    }
mtxLog.unlock();
}
//======================================================================
void print_err(Connect *req, const char *format, ...)
{
    va_list ap;
    char buf[300];

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    String ss(256);
    ss << "[" << log_time() << "] - [" << req->numConn << "] " << buf;

mtxLog.lock();
    write(flog_err, ss.c_str(), ss.size());
    num_logerr_records++;
    if (num_logerr_records > 500000)
    {
        exit(1);
        close(flog_err);
        create_error_logfile(conf->LogPath);
        num_logerr_records = 0;
    }
mtxLog.unlock();
}
//======================================================================
void print_err(Stream *resp, const char *format, ...)
{
    va_list ap;
    char buf[1024];
    if (!resp)
    {
        fprintf(stderr, "<%s:%d> !!! resp=NULL\n", __func__, __LINE__);
        return;
    }

    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    String ss(1024);
    ss << "[" << log_time() << "] - [" << resp->numConn << "/" << resp->numReq << "] " << buf;
    if (ss.size() > 800)
        return;

mtxLog.lock();
    write(flog_err, ss.c_str(), ss.size());
    num_logerr_records++;
    if (num_logerr_records > 500000)
    {
        exit(1);
        close(flog_err);
        create_error_logfile(conf->LogPath);
        num_logerr_records = 0;
    }
mtxLog.unlock();
}
//======================================================================
void print_log(Stream *resp)
{
    String ss(320);
    if (resp)
    {
        ss  << resp->numConn << "/" << resp->numReq << " - " << resp->remoteAddr << " - [" << log_time()
            << "] - \"" << resp->method.c_str() << " " << resp->decode_path.c_str() << " HTTP/2\" - "
            << resp->status << " " << resp->send_bytes << " - \"" << resp->user_agent << "\" - id=" << resp->id << " \n";
    }
    else
        return;

mtxLog.lock();
    write(flog, ss.c_str(), ss.size());
    num_log_records++;
    if (num_log_records > 500000)
    {
        close(flog);
        create_logfile(conf->LogPath);
        num_log_records = 0;
    }
mtxLog.unlock();
}
//======================================================================
void create_logfiles(const string& log_dir)
{
    create_logfile(log_dir);
    create_error_logfile(log_dir);
}
