#include <iostream>
#include <unistd.h>

#include "bytes_array.h"
//======================================================================
static const int huffman_map[][3] = {
                        {  0, 0x1ff8, 13},
                        {  1, 0x7fffd8, 23},
                        {  2, 0xfffffe2, 28},
                        {  3, 0xfffffe3, 28},
                        {  4, 0xfffffe4, 28},
                        {  5, 0xfffffe5, 28},
                        {  6, 0xfffffe6, 28},
                        {  7, 0xfffffe7, 28},
                        {  8, 0xfffffe8, 28},
                        {  9, 0xffffea, 24},
                        { 10, 0x3ffffffc, 30},
                        { 11, 0xfffffe9,28 },
                        { 12, 0xfffffea, 28},
                        { 13, 0x3ffffffd, 30},
                        { 14, 0xfffffeb, 28},
                        { 15, 0xfffffec, 28},
                        { 16, 0xfffffed, 28},
                        { 17, 0xfffffee, 28},
                        { 18, 0xfffffef, 28},
                        { 19, 0xffffff0, 28},
                        { 20, 0xffffff1, 28},
                        { 21, 0xffffff2, 28},
                        { 22, 0x3ffffffe, 30},
                        { 23, 0xffffff3, 28},
                        { 24, 0xffffff4, 28},
                        { 25, 0xffffff5, 28},
                        { 26, 0xffffff6, 28},
                        { 27, 0xffffff7, 28},
                        { 28, 0xffffff8, 28},
                        { 29, 0xffffff9, 28},
                        { 30, 0xffffffa, 28},
                        { 31, 0xffffffb, 28},
                        { 32, 0x14, 6},
                        { 33, 0x3f8, 10},
                        { 34, 0x3f9, 10},
                        { 35, 0xffa, 12},
                        { 36, 0x1ff9, 13},
                        { 37, 0x15, 6},
                        { 38, 0xf8, 8},
                        { 39, 0x7fa, 11},
                        { 40, 0x3fa, 10},
                        { 41, 0x3fb, 10},
                        { 42, 0xf9, 8},
                        { 43, 0x7fb, 11},
                        { 44, 0xfa, 8},
                        { 45, 0x16, 6},
                        { 46, 0x17, 6},
                        { 47, 0x18, 6},
                        { 48, 0x0, 5},    // 0
                        { 49, 0x1, 5},
                        { 50, 0x2, 5},
                        { 51, 0x19, 6},
                        { 52, 0x1a, 6},
                        { 53, 0x1b, 6},
                        { 54, 0x1c, 6},
                        { 55, 0x1d, 6},
                        { 56, 0x1e, 6},
                        { 57, 0x1f, 6},  // 9
                        { 58, 0x5c, 7},
                        { 59, 0xfb, 8},
                        { 60, 0x7ffc, 15},
                        { 61, 0x20, 6},
                        { 62, 0xffb, 12},
                        { 63, 0x3fc, 10},
                        { 64, 0x1ffa, 13},
                        { 65, 0x21, 6},
                        { 66, 0x5d, 7},
                        { 67, 0x5e, 7},
                        { 68, 0x5f, 7},
                        { 69, 0x60, 7},
                        { 70, 0x61, 7},
                        { 71, 0x62, 7},
                        { 72, 0x63, 7},
                        { 73, 0x64, 7},
                        { 74, 0x65, 7},
                        { 75, 0x66, 7},
                        { 76, 0x67, 7},
                        { 77, 0x68, 7},
                        { 78, 0x69, 7},
                        { 79, 0x6a, 7},
                        { 80, 0x6b, 7},
                        { 81, 0x6c, 7},
                        { 82, 0x6d, 7},
                        { 83, 0x6e, 7},
                        { 84, 0x6f, 7},
                        { 85, 0x70, 7},
                        { 86, 0x71, 7},
                        { 87, 0x72, 7},
                        { 88, 0xfc, 8},
                        { 89, 0x73, 7},
                        { 90, 0xfd, 8},
                        { 91, 0x1ffb, 13},
                        { 92, 0x7fff0, 19},
                        { 93, 0x1ffc, 13},
                        { 94, 0x3ffc, 14},
                        { 95, 0x22, 6},
                        { 96, 0x7ffd, 15},
                        { 97, 0x3, 5},
                        { 98, 0x23, 6},
                        { 99, 0x4, 5},
                        {100, 0x24, 6},
                        {101, 0x5, 5},
                        {102, 0x25, 6},
                        {103, 0x26, 6},
                        {104, 0x27, 6},
                        {105, 0x6, 5},
                        {106, 0x74, 7},
                        {107, 0x75, 7},
                        {108, 0x28, 6},
                        {109, 0x29, 6},
                        {110, 0x2a, 6},
                        {111, 0x7, 5},
                        {112, 0x2b, 6},
                        {113, 0x76, 7},
                        {114, 0x2c, 6},
                        {115, 0x8, 5},
                        {116, 0x9, 5},
                        {117, 0x2d, 6},
                        {118, 0x77, 7},
                        {119, 0x78, 7},
                        {120, 0x79, 7},
                        {121, 0x7a, 7},
                        {122, 0x7b, 7},
                        {123, 0x7ffe, 15},
                        {124, 0x7fc, 11},
                        {125, 0x3ffd, 14},
                        {126, 0x1ffd, 13},
                        {127, 0xffffffc, 28},
                        {128, 0xfffe6, 20},
                        {129, 0x3fffd2, 22},
                        {130, 0xfffe7, 20},
                        {131, 0xfffe8, 20},
                        {132, 0x3fffd3, 22},
                        {133, 0x3fffd4, 22},
                        {134, 0, 0},
                        {135, 0, 0},
                        {136, 0, 0},
                        {137, 0, 0},
                        {138, 0, 0},
                        {139, 0, 0},
                        {140, 0, 0},
                        {141, 0, 0},
                        {142, 0, 0},
                        {143, 0, 0},
                        {144, 0, 0},
                        {145, 0, 0},
                        {146, 0, 0},
                        {147, 0, 0},
                        {148, 0, 0},
                        {149, 0, 0},
                        {150, 0, 0},
                        {151, 0, 0},
                        {152, 0, 0},
                        {153, 0, 0},
                        {154, 0, 0},
                        {155, 0, 0},
                        {156, 0, 0},
                        {157, 0, 0},
                        {158, 0, 0},
                        {159, 0, 0},
                        {160, 0, 0},
                        {161, 0, 0},
                        {162, 0, 0},
                        {163, 0, 0},
                        {164, 0, 0},
                        {165, 0, 0},
                        {166, 0, 0},
                        {167, 0, 0},
                        {168, 0, 0},
                        {169, 0, 0},
                        {170, 0, 0},
                        {171, 0, 0},
                        {172, 0, 0},
                        {173, 0, 0},
                        {174, 0, 0},
                        {175, 0, 0},
                        {176, 0, 0},
                        {177, 0, 0},
                        {178, 0, 0},
                        {179, 0, 0},
                        {180, 0, 0},
                        {181, 0, 0},
                        {182, 0, 0},
                        {183, 0, 0},
                        {184, 0, 0},
                        {185, 0, 0},
                        {186, 0, 0},
                        {187, 0, 0},
                        {187, 0, 0},
                        {189, 0, 0},
                        {190, 0, 0},
                        {191, 0, 0},
                        {192, 0, 0},
                        {193, 0, 0},
                        {194, 0, 0},
                        {195, 0, 0},
                        {196, 0, 0},
                        {197, 0, 0},
                        {198, 0, 0},
                        {199, 0, 0},
                        {200, 0, 0},
                        {201, 0, 0},
                        {202, 0, 0},
                        {203, 0, 0},
                        {204, 0, 0},
                        {205, 0, 0},
                        {206, 0, 0},
                        {207, 0, 0},
                        {208, 0, 0},
                        {209, 0, 0},
                        {210, 0, 0},
                        {211, 0, 0},
                        {212, 0, 0},
                        {213, 0, 0},
                        {214, 0, 0},
                        {215, 0, 0},
                        {216, 0, 0},
                        {217, 0, 0},
                        {218, 0, 0},
                        {219, 0, 0},
                        {220, 0, 0},
                        {221, 0, 0},
                        {222, 0, 0},
                        {223, 0, 0},
                        {224, 0, 0},
                        {225, 0, 0},
                        {226, 0, 0},
                        {227, 0, 0},
                        {228, 0, 0},
                        {229, 0, 0},
                        {230, 0, 0},
                        {231, 0, 0},
                        {232, 0, 0},
                        {233, 0, 0},
                        {234, 0, 0},
                        {235, 0, 0},
                        {236, 0, 0},
                        {237, 0, 0},
                        {238, 0, 0},
                        {239, 0, 0},
                        {240, 0, 0},
                        {241, 0, 0},
                        {242, 0, 0},
                        {243, 0, 0},
                        {244, 0, 0},
                        {245, 0, 0},
                        {246, 0, 0},
                        {247, 0, 0},
                        {248, 0, 0},
                        {249, 0, 0},
                        {250, 0, 0},
                        {251, 0, 0},
                        {252, 0, 0},
                        {253, 0, 0},
                        {254, 0, 0},
                        {255, 0, 0},
                    };
//======================================================================
static int find_char(const int in, int size, int code_size, char *ch)
{
    int n = 0;
    int int_numbits = sizeof(int) * 8;
    for (int i = 0; i < code_size; ++i)
    {
        int d = huffman_map[i][1];
        int len = huffman_map[i][2];
        d = d << (int_numbits - len);
        int m = in;
        for ( n = 0; (n < len) && (n < size); n++)
        {
            if ((m & 0x80000000) != (d & 0x80000000))
                break;
            m = m<<1;
            d = d<<1;
        }

        if (n == len)
        {
            *ch = i;
            return len;
        }
        
        n = 0;
    }

    return n;
}
//======================================================================
void huffman_decode(const char *s, int len, std::string& out)
{
    out = "";
    unsigned int fifo_buf = 0;
    int fifo_max_size = sizeof(fifo_buf) * 8;
    int fifo_size = fifo_max_size;
    unsigned int buf;
    int huffman_map_size = sizeof(huffman_map)/(sizeof(int) * 3);

    for ( ; ; )
    {
        for ( ; len > 0; )
        {
            int lshift;
            if (fifo_size > 8)
                lshift = fifo_size - 8;
            else
                break;
            buf = *((unsigned char*)s++);
            len--;
            buf = buf << lshift;
            fifo_buf = fifo_buf | buf;
            fifo_size -= 8;
        }

        char ch;
        int n = find_char(fifo_buf, fifo_max_size - fifo_size, huffman_map_size, &ch);
        if (n > 0)
        {
            out += ch;
            fifo_buf = fifo_buf<<n;
            fifo_size += n;
        }
        else
            break;
    }
}
//======================================================================
int huffman_encode(const char *in, ByteArray& out)
{
    out.init();
    unsigned int index;
    int huff_buf;
    int buf_len;
    int ret = 0;

    unsigned char out_byte = 0;
    int out_bits_len = 0;

    while (*in)
    {
        index = (unsigned char)*(in++);
        huff_buf = huffman_map[index][1];
        buf_len = huffman_map[index][2];
        if (buf_len < 5)
        {
            fprintf(stderr, "<%s:%d> Error len=%d\n", __func__, __LINE__, buf_len);
            ret = -1;
            break;
        }

        while (true)
        {
            if ((buf_len + out_bits_len) < 8)
            {
                out_bits_len += buf_len;
                out_byte |= (0xff & (huff_buf << (8 - out_bits_len)));
                buf_len = 0;
            }
            else
            {
                buf_len += (out_bits_len - 8);
                out_byte |= (0xff & (huff_buf >> buf_len));
                out_bits_len = 8;
                out.cat(out_byte);
                out_bits_len = 0;
                out_byte = 0;
            }

            if (buf_len == 0)
                break;
            else if (buf_len < 0)
            {
                fprintf(stderr, "<%s:%d> Error len=%d\n", __func__, __LINE__, buf_len);
                return -1;
            }
        }
    }

    if (out_bits_len > 0)
    {
        unsigned char ch = 0xff;
        ch >>= out_bits_len;
        out_byte |= ch;
        out.cat(out_byte);
    }

    return ret;
}
