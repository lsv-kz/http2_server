#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <iostream>

#include "bytes_array.h"
//======================================================================
static const char *huffman_map[][2] = {
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

            while (huffman_map[i][1])
            {
                if (c == huffman_map[i][0][0])
                {
                    int len = strlen(huffman_map[i][1]);
                    if (len > 0)
                    {
                        bits_text.cat(huffman_map[i][1], len);
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
            while (huffman_map[j][0])
            {
                int i = strlen(huffman_map[j][1]);
                if (i > 0)
                {
                    if (!memcmp(huffman_map[j][1], bits_text.ptr() + n, i))
                    {
                        n += i;
                        len -= i;
                        s_out += huffman_map[j][0][0];
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

            if (huffman_map[j][1] == NULL)
            {
                fprintf(stderr, "<%s:%d> Error j=%d, len=%d\n", __func__, __LINE__, j, len);
                return -1;
            }
        }

        return buf_out.size();
    }
};

#endif
