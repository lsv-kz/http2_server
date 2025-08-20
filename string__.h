#ifndef CLASS_STRING_H_
#define CLASS_STRING_H_

#include <iostream>
#include <string>
//======================================================================
class String
{
    char *buf;
    unsigned long buf_size;
    unsigned long buf_len;
    unsigned long index_ = 0;
    int err = 0;
    //------------------------------------------------------------------
    int is_space(char c)
    {
        switch (c)
        {
            case '\x20':
            case '\t':
            case '\r':
            case '\n':
            case '\0':
                return 1;
        }
        return 0;
    }
    //------------------------------------------------------------------
    void append(const char ch)
    {
        if ((ch > 0) && (err == 0))
        {
            unsigned long n = buf_len + 1;
            if (buf_size <= n)
            {
                if (reserve(n + 16))
                    return;
            }

            buf[buf_len++] = ch;
            buf[buf_len] = 0;
        }
    }
    //------------------------------------------------------------------
    void append(const char *s)
    {
        if ((s == NULL) || err)
            return;
        append(s, strlen(s));
    }
    //------------------------------------------------------------------
    void append(char *s)
    {
        if ((s == NULL) || err)
            return;
        append(s, strlen(s));
    }
    //------------------------------------------------------------------
    void append(const char *s, unsigned int len)
    {
        if ((s == NULL) || (len == 0) || err)
            return;
        unsigned long n = buf_len + len;
        if (buf_size <= n)
        {
            if (reserve(n + 64))
                return;
        }

        memcpy(buf + buf_len, s, len);
        buf_len += len;
        buf[buf_len] = 0;
    }
    //------------------------------------------------------------------
    void append(const std::string& s)
    {
        append(s.c_str(), s.size());
    }
    //------------------------------------------------------------------
    void append(const String& s)
    {
        append(s.buf, s.buf_len);
    }
    //------------------------------------------------------------------
    void init()
    {
        buf = NULL;
        buf_size = buf_len = err = 0; index_ = 0;
    }

public:

    String() { init(); }
    explicit String(unsigned int n) { init(); reserve(n); }
    String(const char *s) { init(); append(s); }
    //------------------------------------------------------------------
    String(const String&) = delete;
    String& operator >> (double&) = delete;
    String & operator << (double f)=delete;
    String& operator >> (char*) = delete;
    //------------------------------------------------------------------
    ~String()
    {
        if (buf)
            delete [] buf;
    }
    //------------------------------------------------------------------
    String & operator = (const char *s)
    {
        clear();
        if (s)
            append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String & operator = (const std::string& s)
    {
        clear();
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator = (const String& s)
    {
        clear();
        append(s.buf, s.buf_len);
        return *this;
    }
    //------------------------------------------------------------------
    friend bool operator == (const String& s1, const char *s2)
    {
        if ((s1.buf == NULL) || (s2 == NULL))
            return false;
        if (!strcmp(s1.buf, s2))
            return true;
        else
            return false;
    }
    //------------------------------------------------------------------
    friend bool operator != (const String& s1, const char *s2)
    {
        if (s1 == s2)
            return false;
        else
            return true;
    }
    //------------------------------------------------------------------
    String & operator += (char c)
    {
        append(c);
        return *this;
    }
    //------------------------------------------------------------------
    String & operator += (const char *s)
    {
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String & operator += (const String& s)
    {
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    const char operator[] (unsigned long n) const
    {
        if (n >= (unsigned int)buf_size)
            return '\0';
        return buf[n];
    }
    //------------------------------------------------------------------
    String& operator << (const char ch)
    {
        append(ch);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator << (const char* s)
    {
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator << (char* s)
    {
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator << (const String& s)
    {
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator << (const std::string& s)
    {
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator << (long long ll)
    {
        char s[32];
        snprintf(s, sizeof(s), "%lld", ll);
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator << (long unsigned int ul)
    {
        char s[32];
        snprintf(s, sizeof(s), "%lu", ul);
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator << (int i)
    {
        char s[32];
        snprintf(s, sizeof(s), "%d", i);
        append(s);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator >> (String& s)
    {
        for (; index_ < buf_size; ++index_)
            if (!is_space(buf[index_]))
                break;
        unsigned long start = index_;
        for (; index_ < buf_size; index_++)
            if (is_space(buf[index_]))
                break;
        s.append(buf + start, index_ - start);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator >> (std::string& s)
    {
        for (; index_ < buf_size; ++index_)
            if (!is_space(buf[index_]))
                break;
        unsigned long start = index_;
        for (; index_ < buf_size; index_++)
            if (is_space(buf[index_]))
                break;
        s.append(buf + start, index_ - start);
        return *this;
    }
    //------------------------------------------------------------------
    String& operator >> (long long& ll)
    {
        ll = 0;
        for (; (index_ < buf_len); ++index_)
            if (!is_space(buf[index_]))
                break;
        char *p;
        ll = strtoll(buf + index_, &p, 10);
        if (!p)
        {
            err = 1;
            ll = 0;
        }
        else
            index_ += (p - (buf + index_));
        return *this;
    }
    //------------------------------------------------------------------
    String& operator >> (long& li)
    {
        li = 0;
        for (; (index_ < buf_len); ++index_)
            if (!is_space(buf[index_]))
                break;
        char *p;
        li = strtol(buf + index_, &p, 10);
        if (!p)
        {
            err = 1;
            li = 0;
        }
        else
            index_ += (p - (buf + index_));
        return *this;
    }
    //------------------------------------------------------------------
    String& operator >> (int& li)
    {
        li = 0;
        for (; (index_ < buf_len); ++index_)
            if (!is_space(buf[index_]))
                break;
        char *p;
        li = strtol(buf + index_, &p, 10);
        if (!p)
        {
            err = 1;
            li = 0;
        }
        else
            index_ += (p - (buf + index_));
        return *this;
    }
    //------------------------------------------------------------------
    String& operator >> (unsigned int& li)
    {
        li = 0;
        for (; (index_ < buf_len); ++index_)
            if (!is_space(buf[index_]))
                break;
        if (buf[index_] == '-')
        {
            fprintf(stderr, "<%s:%d> Error: integer is signed\n", __func__, __LINE__);
            return *this;
        }
        char *p;
        li = strtol(buf + index_, &p, 10);
        if (!p)
        {
            err = 1;
            li = 0;
        }
        else
            index_ += (p - (buf + index_));
        return *this;
    }
    //------------------------------------------------------------------
    const char *c_str() const
    {
        if (!buf)
            return "";
        else
            return buf;
    }
    //------------------------------------------------------------------
    int reserve(unsigned long n)
    {
        if (buf_size >= n)
            return 0;
        if (n > 65536)
        {
            fprintf(stderr, "<%s:%d> Error limit: length=%lu\n", __func__, __LINE__, n);
            return (err = 1);
        }

        ++n;
        char *tmp_buf = new(std::nothrow) char [n];
        if (!tmp_buf)
        {
            fprintf(stderr, "<%s:%d> Error new char [%lu]\n", __func__, __LINE__, n);
            return (err = 1);
        }

        if (buf)
        {
            memcpy(tmp_buf, buf, buf_len + 1);
            delete [] buf;
        }
        buf = tmp_buf;
        buf_size = n;
        return 0;
    }
    //------------------------------------------------------------------
    unsigned long size() const { return buf_len; }
    unsigned long capacity() const { return buf_size; }

    void resize(unsigned long n)
    {
        if (n >= buf_len)
            return;
        buf_len = n;
        buf[buf_len] = 0;
    }
    //------------------------------------------------------------------
    const String& str() const
    {
        return *this;
    }
    //------------------------------------------------------------------
    void clear()
    {
        buf_len = index_ = 0;
        err = 0;
        if (buf)
            *buf = '\0';
    }
    //------------------------------------------------------------------
    int error() const { return err; }
};

#endif
