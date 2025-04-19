#ifndef BYTESARRAY_H_
#define BYTESARRAY_H_

#include <iostream>

//======================================================================
class ByteArray
{
    ByteArray *prev;
    ByteArray *next;
    
    char *buf;
    int buf_size;
    int buf_len;
    int offset;
    
    //------------------------------------------------------------------
    int resize(int size_new)
    {
        if ((buf_size >= size_new) ||(size_new <= 0))
            return -1;
        char *tmp_buf = new(std::nothrow) char [size_new];
        if (!tmp_buf)
        {
            fprintf(stderr, "<%s:%d> Error new char [%d]\n", __func__, __LINE__, size_new);
            return -1;
        }

        if (buf)
        {
            if (buf_len > 0)
                memcpy(tmp_buf, buf, buf_len + 1);
            delete [] buf;
        }

        buf = tmp_buf;
        buf_size = size_new;

        return 0;
    }
    
    ByteArray(const ByteArray&);
    ByteArray& operator=(const ByteArray&);
    
public:
    
    ByteArray()
    {
        buf = NULL;
        buf_size = buf_len = offset = 0;
        prev = next = NULL;
    }
    //------------------------------------------------------------------
    ~ByteArray()
    {
        if (buf)
        {
            //fprintf(stderr, "<%s:%d> ~~~ delete ByteArray %p, size=%d, len=%d\n", __func__, __LINE__, buf, buf_size, buf_len);
            delete [] buf;
            buf = NULL;
//fprintf(stderr, "<%s:%d> ~~~ delete ByteArray %p, size=%d, len=%d\n", __func__, __LINE__, buf, buf_size, buf_len);
        }
    }
    //------------------------------------------------------------------
    void init()
    {
        //fprintf(stderr, "<%s:%d> !!!!! init ByteArray, size=%d, len=%d, offs=%d\n", __func__, __LINE__, buf_size, buf_len, offset);
        buf_len = offset = 0;
    }
    //------------------------------------------------------------------
    int cpy(const char *b, int len)
    {
        if (buf_size <= len)
        {
            if (resize(len * 2 + 32))
                return -1;
        }

        memcpy(buf, b, len);
        buf_len = len;
        offset = 0;
        buf[buf_len] = 0;
        return 0;
    }
    //------------------------------------------------------------------
    int cat(const char *b, int len)
    {
        if (buf_size <= (buf_len + len))
        {
            if (resize((buf_len + len) * 2 + 32))
                    return -1;
        }

        memcpy(buf + buf_len, b, len);
        buf_len += len;
        buf[buf_len] = 0;
        return 0;
    }
    //------------------------------------------------------------------
    int cpy_str(const char *s)
    {
        int len = (int)strlen(s);
        if (len)
            return cpy(s, len);
        else
        {
            buf_len = 0;
            offset = 0;
            return 0;
        }
    }
    //------------------------------------------------------------------
    int cat_str(const char *s)
    {
        int len = (int)strlen(s);
        if (len)
            return cat(s, len);
        else
        {
            return 0;
        }
    }
    //------------------------------------------------------------------
    int cpy_hex(const char *s)
    {
        buf_len = 0;
        offset = 0;
        return cat_hex_(s);
    }
    //------------------------------------------------------------------
    int cat_hex(const char *s)
    {
        return cat_hex_(s);
    }
    //------------------------------------------------------------------
    int cat_hex_(const char *s)
    {
        int data;
        char t[16] = "";
        int i = 0, j = 0, fl = 0;
        while (s[i])
        {
            if (fl == 0)
            {
                if ((s[i] != ' ') && (s[i] != '\t'))
                {
                    fl = 1;
                    t[j++] = s[i];
                }
            }
            else if (fl)
            {
                if ((s[i] == ' ') || (s[i] == '\t'))
                {
                    if (j != 2)
                    {
                        fprintf(stderr, "<%s:%d> Error j=%d\n", __func__, __LINE__, j);
                        return -1;
                    }
                }
                else
                {
                    t[j++] = s[i];
                    if (j == 2)
                    {
                        t[j] = 0;
                        sscanf(t, "%x", &data);
                        if (buf_size <= (buf_len + 1))
                        {
                            if (resize(buf_len * 2 + 64))
                                return -1;
                        }
                        buf[buf_len++] = (char)data;
                        j = 0;
                        fl = 0;
                    }
                }
            }

            ++i;
        }

        if (j)
        {
            fprintf(stderr, "<%s:%d> Error j=%d\n", __func__, __LINE__, j);
            return -1;
        }

        buf[buf_len] = 0;
        return buf_len;
    }
    //------------------------------------------------------------------
    const char *ptr()
    {
        if (buf)
        {
            buf[buf_len] = 0;
            return buf;
        }
        else
            return "";
    }
    //------------------------------------------------------------------
    const char *ptr_remain()
    {
        if (buf)
        {
            buf[buf_len] = 0;
            return buf + offset;
        }
        else
            return "";
    }
    //------------------------------------------------------------------
    int get_byte(int i)
    {
        if ((i >= buf_size) || (i < 0))
            return -1;
        return (unsigned char)buf[i];
    }
    //------------------------------------------------------------------
    int set_byte(char ch, int i)
    {
        if ((i < 0) || (i >= buf_len) || (buf_size == 0))
            return -1;
        else
        {
            buf[i] = ch;
            return 0;
        }
    }
    //------------------------------------------------------------------
    int size() { return buf_len; }
    int size_remain() { return buf_len - offset; }
    int get_offset () { return offset; }
    int offset_inc (int n)
    {
        if ((offset + n) > buf_len)
        {
            offset = buf_len;
            return -1;
        }

        offset += n;
        return offset;
    }
};

#endif
