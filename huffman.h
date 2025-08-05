#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <iostream>

#include "bytes_array.h"

extern const char *huf_map[][2];
//======================================================================
class HuffmanCode
{
    ByteArray bits_text;
    ByteArray buf_out;

    HuffmanCode(const HuffmanCode&);
    HuffmanCode& operator=(const HuffmanCode&);

public:
    HuffmanCode()
    {
        //fprintf(stderr, "<%s:%d> +++++ create HuffmanCode\n", __func__, __LINE__);
    }

    ~HuffmanCode()
    {
        //fprintf(stderr, "<%s:%d> ~~~~ delete HuffmanCode\n", __func__, __LINE__);
    }
    //------------------------------------------------------------------
    int text_to_text_bits_array(const char *in)
    {
        int len_in = strlen(in);
        bits_text.init();
        while (*in)
        {
            char c = *(in++);
            int i = 0;

            while (huf_map[i][1])
            {
                if (c == huf_map[i][0][0])
                {
                    int len = strlen(huf_map[i][1]);
                    if (len > 0)
                    {
                        bits_text.cat(huf_map[i][1], len);
                    }
                    else
                    {
                        fprintf(stderr, "<%s:%d> Error\n", __func__, __LINE__);
                        return -1;
                    }
                }
                i++;
            }
        }

        int m = bits_text.size();
        if (m % 8)
        {
            int n = 8 - (m % 8);
            char end[] = "11111111";
            bits_text.cat(end,  n);
        }

        return len_in;
    }
    //------------------------------------------------------------------
    int encode(const char *s, ByteArray& out)
    {
        int len_in = text_to_text_bits_array(s);
        if (len_in < 0)
        {
            return -1;
        }

        int i = 0;
        buf_out.init();

        while (i < bits_text.size())
        {
            char t = 0;
            int j = 7;
            while (j >= 0)
            {
                char c = bits_text.ptr()[i++];
                t |= ((c == '0') ? 0 : 1)<<j;
                j--;
            }

            buf_out.cat(&t, 1);
        }
        out.cpy(buf_out.ptr(), buf_out.size());
        return buf_out.size();
    }
    //------------------------------------------------------------------
    int hex_to_bytearray(const char *s, int len_in, char *s_out)
    {
        int sout_len = 0;
        int sout_size = len_in;
        int data;
        char t[16] = "";
        int j = 0, fl = 0;
        int  i = 0;
        while (i < len_in)
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
                        if (sout_size <= (sout_len + 1))
                        {
                            fprintf(stderr, "<%s:%d> Error \n", __func__, __LINE__);
                            return -1;
                        }
                        s_out[sout_len++] = (char)data;
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
        return sout_len;
    }
    //------------------------------------------------------------------
    int decode_bytearray(const char *s, int len, std::string& s_out)
    {
        int n = 0;
        bits_text.init();
        if (len <= 0)
        {
            fprintf(stderr, "<%s:%d> Error len=%d\n", __func__, __LINE__, len);
            return -1;
        }

        while (n < len)
        {
            char ch = s[n];
            char t[16];
            int j = 0;
            while (j < 8)
            {
                t[j] = ((ch & 0x80) ? 1 : 0) + 0x30;
                ch = (ch<<1);
                j++;
            }

            t[j] = 0;
            bits_text.cat(t , 8);
            n++;
        }

        return decode(s_out);
    }
    //------------------------------------------------------------------
    int decode(std::string& s_out)
    {
        s_out = "";
        int len = bits_text.size();
        buf_out.init();
        int n = 0;
        while (len > 0)
        {
            int j = 0;
            while (huf_map[j][0])
            {
                int i = strlen(huf_map[j][1]);
                if (i > 0)
                {
                    if (!memcmp(huf_map[j][1], bits_text.ptr() + n, i))
                    {
                        n += i;
                        len -= i;
                        s_out += huf_map[j][0][0];
                        break;
                    }
                    else
                    {
                        if ((int)strspn(bits_text.ptr() + n, "1") == len)
                        {
                            len = 0;
                            break;
                        }
                    }
                }
                j++;
            }

            if (huf_map[j][1] == NULL)
            {
                fprintf(stderr, "<%s:%d> Error j=%d, len=%d\n", __func__, __LINE__, j, len);
                return -1;
            }
        }

        return buf_out.size();
    }
};

#endif
