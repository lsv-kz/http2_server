#include <iostream>

#include "bytes_array.h"
//======================================================================
static const unsigned int huffman_decode_table[][3] = {
    { 48, 0x00000000, 5}, // '0'
    { 49, 0x08000000, 5}, // '1'
    { 50, 0x10000000, 5}, // '2'
    { 97, 0x18000000, 5}, // 'a'
    { 99, 0x20000000, 5}, // 'c'
    {101, 0x28000000, 5}, // 'e'
    {105, 0x30000000, 5}, // 'i'
    {111, 0x38000000, 5}, // 'o'
    {115, 0x40000000, 5}, // 's'
    {116, 0x48000000, 5}, // 't'
    { 32, 0x50000000, 6}, // ' '
    { 37, 0x54000000, 6}, // '%'
    { 45, 0x58000000, 6}, // '-'
    { 46, 0x5c000000, 6}, // '.'
    { 47, 0x60000000, 6}, // '/'
    { 51, 0x64000000, 6}, // '3'
    { 52, 0x68000000, 6}, // '4'
    { 53, 0x6c000000, 6}, // '5'
    { 54, 0x70000000, 6}, // '6'
    { 55, 0x74000000, 6}, // '7'
    { 56, 0x78000000, 6}, // '8'
    { 57, 0x7c000000, 6}, // '9'
    { 61, 0x80000000, 6}, // '='
    { 65, 0x84000000, 6}, // 'A'
    { 95, 0x88000000, 6}, // '_'
    { 98, 0x8c000000, 6}, // 'b'
    {100, 0x90000000, 6}, // 'd'
    {102, 0x94000000, 6}, // 'f'
    {103, 0x98000000, 6}, // 'g'
    {104, 0x9c000000, 6}, // 'h'
    {108, 0xa0000000, 6}, // 'l'
    {109, 0xa4000000, 6}, // 'm'
    {110, 0xa8000000, 6}, // 'n'
    {112, 0xac000000, 6}, // 'p'
    {114, 0xb0000000, 6}, // 'r'
    {117, 0xb4000000, 6}, // 'u'
    { 58, 0xb8000000, 7}, // ':'
    { 66, 0xba000000, 7}, // 'B'
    { 67, 0xbc000000, 7}, // 'C'
    { 68, 0xbe000000, 7}, // 'D'
    { 69, 0xc0000000, 7}, // 'E'
    { 70, 0xc2000000, 7}, // 'F'
    { 71, 0xc4000000, 7}, // 'G'
    { 72, 0xc6000000, 7}, // 'H'
    { 73, 0xc8000000, 7}, // 'I'
    { 74, 0xca000000, 7}, // 'J'
    { 75, 0xcc000000, 7}, // 'K'
    { 76, 0xce000000, 7}, // 'L'
    { 77, 0xd0000000, 7}, // 'M'
    { 78, 0xd2000000, 7}, // 'N'
    { 79, 0xd4000000, 7}, // 'O'
    { 80, 0xd6000000, 7}, // 'P'
    { 81, 0xd8000000, 7}, // 'Q'
    { 82, 0xda000000, 7}, // 'R'
    { 83, 0xdc000000, 7}, // 'S'
    { 84, 0xde000000, 7}, // 'T'
    { 85, 0xe0000000, 7}, // 'U'
    { 86, 0xe2000000, 7}, // 'V'
    { 87, 0xe4000000, 7}, // 'W'
    { 89, 0xe6000000, 7}, // 'Y'
    {106, 0xe8000000, 7}, // 'j'
    {107, 0xea000000, 7}, // 'k'
    {113, 0xec000000, 7}, // 'q'
    {118, 0xee000000, 7}, // 'v'
    {119, 0xf0000000, 7}, // 'w'
    {120, 0xf2000000, 7}, // 'x'
    {121, 0xf4000000, 7}, // 'y'
    {122, 0xf6000000, 7}, // 'z'
    { 38, 0xf8000000, 8}, // '&'
    { 42, 0xf9000000, 8}, // '*'
    { 44, 0xfa000000, 8}, // ','
    { 59, 0xfb000000, 8}, // ';'
    { 88, 0xfc000000, 8}, // 'X'
    { 90, 0xfd000000, 8}, // 'Z'
    { 33, 0xfe000000, 10}, // '!'
    { 34, 0xfe400000, 10}, // '"'
    { 40, 0xfe800000, 10}, // '('
    { 41, 0xfec00000, 10}, // ')'
    { 63, 0xff000000, 10}, // '?'
    { 39, 0xff400000, 11}, // '''
    { 43, 0xff600000, 11}, // '+'
    {124, 0xff800000, 11}, // '|'
    { 35, 0xffa00000, 12}, // '#'
    { 62, 0xffb00000, 12}, // '>'
    {  0, 0xffc00000, 13}, // '\x0'
    { 36, 0xffc80000, 13}, // '$'
    { 64, 0xffd00000, 13}, // '@'
    { 91, 0xffd80000, 13}, // '['
    { 93, 0xffe00000, 13}, // ']'
    {126, 0xffe80000, 13}, // '~'
    { 94, 0xfff00000, 14}, // '^'
    {125, 0xfff40000, 14}, // '}'
    { 60, 0xfff80000, 15}, // '<'
    { 96, 0xfffa0000, 15}, // '`'
    {123, 0xfffc0000, 15}, // '{'
    { 92, 0xfffe0000, 19}, // '\'
    {195, 0xfffe2000, 19},
    {208, 0xfffe4000, 19},
    {128, 0xfffe6000, 20},
    {130, 0xfffe7000, 20},
    {131, 0xfffe8000, 20},
    {162, 0xfffe9000, 20},
    {194, 0xfffeb000, 20},
    {184, 0xfffea000, 20},
    {224, 0xfffec000, 20},
    {226, 0xfffed000, 20},
    {153, 0xfffee000, 21},
    {161, 0xfffee800, 21},
    {167, 0xfffef000, 21},
    {172, 0xfffef800, 21},
    {176, 0xffff0000, 21},
    {177, 0xffff0800, 21},
    {179, 0xffff1000, 21},
    {209, 0xffff1800, 21},
    {216, 0xffff2000, 21},
    {217, 0xffff2800, 21},
    {227, 0xffff3000, 21},
    {229, 0xffff3800, 21},
    {230, 0xffff4000, 21},
    {129, 0xffff4800, 22},
    {132, 0xffff4c00, 22},
    {133, 0xffff5000, 22},
    {134, 0xffff5400, 22},
    {136, 0xffff5800, 22},
    {146, 0xffff5c00, 22},
    {154, 0xffff6000, 22},
    {156, 0xffff6400, 22},
    {160, 0xffff6800, 22},
    {163, 0xffff6c00, 22},
    {164, 0xffff7000, 22},
    {169, 0xffff7400, 22},
    {170, 0xffff7800, 22},
    {173, 0xffff7c00, 22},
    {178, 0xffff8000, 22},
    {181, 0xffff8400, 22},
    {185, 0xffff8800, 22},
    {186, 0xffff8c00, 22},
    {187, 0xffff9000, 22},
    {189, 0xffff9400, 22},
    {190, 0xffff9800, 22},
    {196, 0xffff9c00, 22},
    {198, 0xffffa000, 22},
    {228, 0xffffa400, 22},
    {232, 0xffffa800, 22},
    {233, 0xffffac00, 22},
    {  1, 0xffffb000, 23},
    {135, 0xffffb200, 23},
    {137, 0xffffb400, 23},
    {138, 0xffffb600, 23},
    {139, 0xffffb800, 23},
    {140, 0xffffba00, 23},
    {141, 0xffffbc00, 23},
    {143, 0xffffbe00, 23},
    {147, 0xffffc000, 23},
    {149, 0xffffc200, 23},
    {150, 0xffffc400, 23},
    {151, 0xffffc600, 23},
    {152, 0xffffc800, 23},
    {155, 0xffffca00, 23},
    {157, 0xffffcc00, 23},
    {158, 0xffffce00, 23},
    {165, 0xffffd000, 23},
    {166, 0xffffd200, 23},
    {168, 0xffffd400, 23},
    {174, 0xffffd600, 23},
    {175, 0xffffd800, 23},
    {180, 0xffffda00, 23},
    {182, 0xffffdc00, 23},
    {183, 0xffffde00, 23},
    {188, 0xffffe000, 23},
    {191, 0xffffe200, 23},
    {197, 0xffffe400, 23},
    {231, 0xffffe600, 23},
    {239, 0xffffe800, 23},
    {  9, 0xffffea00, 24},
    {142, 0xffffeb00, 24},
    {144, 0xffffec00, 24},
    {145, 0xffffed00, 24},
    {148, 0xffffee00, 24},
    {159, 0xffffef00, 24},
    {171, 0xfffff000, 24},
    {206, 0xfffff100, 24},
    {215, 0xfffff200, 24},
    {225, 0xfffff300, 24},
    {236, 0xfffff400, 24},
    {237, 0xfffff500, 24},
    {199, 0xfffff600, 25},
    {207, 0xfffff680, 25},
    {234, 0xfffff700, 25},
    {235, 0xfffff780, 25},
    {192, 0xfffff800, 26},
    {193, 0xfffff840, 26},
    {200, 0xfffff880, 26},
    {201, 0xfffff8c0, 26},
    {202, 0xfffff900, 26},
    {205, 0xfffff940, 26},
    {210, 0xfffff980, 26},
    {213, 0xfffff9c0, 26},
    {218, 0xfffffa00, 26},
    {219, 0xfffffa40, 26},
    {238, 0xfffffa80, 26},
    {240, 0xfffffac0, 26},
    {242, 0xfffffb00, 26},
    {243, 0xfffffb40, 26},
    {255, 0xfffffb80, 26},
    {203, 0xfffffbc0, 27},
    {204, 0xfffffbe0, 27},
    {211, 0xfffffc00, 27},
    {212, 0xfffffc20, 27},
    {214, 0xfffffc40, 27},
    {221, 0xfffffc60, 27},
    {222, 0xfffffc80, 27},
    {223, 0xfffffca0, 27},
    {241, 0xfffffcc0, 27},
    {244, 0xfffffce0, 27},
    {245, 0xfffffd00, 27},
    {246, 0xfffffd20, 27},
    {247, 0xfffffd40, 27},
    {248, 0xfffffd60, 27},
    {250, 0xfffffd80, 27},
    {251, 0xfffffda0, 27},
    {252, 0xfffffdc0, 27},
    {253, 0xfffffde0, 27},
    {254, 0xfffffe00, 27},
    {  2, 0xfffffe20, 28},
    {  3, 0xfffffe30, 28},
    {  4, 0xfffffe40, 28},
    {  5, 0xfffffe50, 28},
    {  6, 0xfffffe60, 28},
    {  7, 0xfffffe70, 28},
    {  8, 0xfffffe80, 28},
    { 11, 0xfffffe90, 28},
    { 12, 0xfffffea0, 28},
    { 14, 0xfffffeb0, 28},
    { 15, 0xfffffec0, 28},
    { 16, 0xfffffed0, 28},
    { 17, 0xfffffee0, 28},
    { 18, 0xfffffef0, 28},
    { 19, 0xffffff00, 28},
    { 20, 0xffffff10, 28},
    { 21, 0xffffff20, 28},
    { 23, 0xffffff30, 28},
    { 24, 0xffffff40, 28},
    { 25, 0xffffff50, 28},
    { 26, 0xffffff60, 28},
    { 27, 0xffffff70, 28},
    { 28, 0xffffff80, 28},
    { 29, 0xffffff90, 28},
    { 30, 0xffffffa0, 28},
    { 31, 0xffffffb0, 28},
    {127, 0xffffffc0, 28},
    {220, 0xffffffd0, 28},
    {249, 0xffffffe0, 28},
    { 10, 0xfffffff0, 30},
    { 13, 0xfffffff4, 30},
    { 22, 0xfffffff8, 30},
    {256, 0xfffffffc, 30}
};
//======================================================================
static const unsigned int huffman_encode_table[][3] = {
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
    { 48, 0x0, 5},
    { 49, 0x1, 5},
    { 50, 0x2, 5},
    { 51, 0x19, 6},
    { 52, 0x1a, 6},
    { 53, 0x1b, 6},
    { 54, 0x1c, 6},
    { 55, 0x1d, 6},
    { 56, 0x1e, 6},
    { 57, 0x1f, 6},
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
    {134, 0x3fffd5, 22},
    {135, 0x7fffd9, 23},
    {136, 0x3fffd6, 22},
    {137, 0x7fffda, 23},
    {138, 0x7fffdb, 23},
    {139, 0x7fffdc, 23},
    {140, 0x7fffdd, 23},
    {141, 0x7fffde, 23},
    {142, 0xffffeb, 24},
    {143, 0x7fffdf, 23},
    {144, 0xffffec, 24},
    {145, 0xffffed, 24},
    {146, 0x3fffd7, 22},
    {147, 0x7fffe0, 23},
    {148, 0xffffee, 24},
    {149, 0x7fffe1, 23},
    {150, 0x7fffe2, 23},
    {151, 0x7fffe3, 23},
    {152, 0x7fffe4, 23},
    {153, 0x1fffdc, 21},
    {154, 0x3fffd8, 22},
    {155, 0x7fffe5, 23},
    {156, 0x3fffd9, 22},
    {157, 0x7fffe6, 23},
    {158, 0x7fffe7, 23},
    {159, 0xffffef, 24},
    {160, 0x3fffda, 22},
    {161, 0x1fffdd, 21},
    {162, 0xfffe9, 20},
    {163, 0x3fffdb, 22},
    {164, 0x3fffdc, 22},
    {165, 0x7fffe8, 23},
    {166, 0x7fffe9, 23},
    {167, 0x1fffde, 21},
    {168, 0x7fffea, 23},
    {169, 0x3fffdd, 22},
    {170, 0x3fffde, 22},
    {171, 0xfffff0, 24},
    {172, 0x1fffdf, 21},
    {173, 0x3fffdf, 22},
    {174, 0x7fffeb, 23},
    {175, 0x7fffec, 23},
    {176, 0x1fffe0, 21},
    {177, 0x1fffe1, 21},
    {178, 0x3fffe0, 22},
    {179, 0x1fffe2, 21},
    {180, 0x7fffed, 23},
    {181, 0x3fffe1, 22},
    {182, 0x7fffee, 23},
    {183, 0x7fffef, 23},
    {184, 0xfffea, 20},
    {185, 0x3fffe2, 22},
    {186, 0x3fffe3, 22},
    {187, 0x3fffe4, 22},
    {188, 0x7ffff0, 23},
    {189, 0x3fffe5, 22},
    {190, 0x3fffe6, 22},
    {191, 0x7ffff1, 23},
    {192, 0x3ffffe0, 26},
    {193, 0x3ffffe1, 26},
    {194, 0xfffeb, 20},
    {195, 0x7fff1, 19},
    {196, 0x3fffe7, 22},
    {197, 0x7ffff2, 23},
    {198, 0x3fffe8, 22},
    {199, 0x1ffffec, 25},
    {200, 0x3ffffe2, 26},
    {201, 0x3ffffe3, 26},
    {202, 0x3ffffe4, 26},
    {203, 0x7ffffde, 27},
    {204, 0x7ffffdf, 27},
    {205, 0x3ffffe5, 26},
    {206, 0xfffff1, 24},
    {207, 0x1ffffed, 25},
    {208, 0x7fff2, 19},
    {209, 0x1fffe3, 21},
    {210, 0x3ffffe6, 26},
    {211, 0x7ffffe0, 27},
    {212, 0x7ffffe1, 27},
    {213, 0x3ffffe7, 26},
    {214, 0x7ffffe2, 27},
    {215, 0xfffff2, 24},
    {216, 0x1fffe4, 21},
    {217, 0x1fffe5, 21},
    {218, 0x3ffffe8, 26},
    {219, 0x3ffffe9, 26},
    {220, 0xffffffd, 28},
    {221, 0x7ffffe3, 27},
    {222, 0x7ffffe4, 27},
    {223, 0x7ffffe5, 27},
    {224, 0xfffec, 20},
    {225, 0xfffff3, 24},
    {226, 0xfffed, 20},
    {227, 0x1fffe6, 21},
    {228, 0x3fffe9, 22},
    {229, 0x1fffe7, 21},
    {230, 0x1fffe8, 21},
    {231, 0x7ffff3, 23},
    {232, 0x3fffea, 22},
    {233, 0x3fffeb, 22},
    {234, 0x1ffffee, 25},
    {235, 0x1ffffef, 25},
    {236, 0xfffff4, 24},
    {237, 0xfffff5, 24},
    {238, 0x3ffffea, 26},
    {239, 0x7ffff4, 23},
    {240, 0x3ffffeb, 26},
    {241, 0x7ffffe6, 27},
    {242, 0x3ffffec, 26},
    {243, 0x3ffffed, 26},
    {244, 0x7ffffe7, 27},
    {245, 0x7ffffe8, 27},
    {246, 0x7ffffe9, 27},
    {247, 0x7ffffea, 27},
    {248, 0x7ffffeb, 27},
    {249, 0xffffffe, 28},
    {250, 0x7ffffec, 27},
    {251, 0x7ffffed, 27},
    {252, 0x7ffffee, 27},
    {253, 0x7ffffef, 27},
    {254, 0x7fffff0, 27},
    {255, 0x3ffffee, 26},
    {256, 0x3fffffff, 30}
};
//======================================================================
static const unsigned int mask_table[] = {
    0x00000000, // 0
    0x80000000, // 1
    0xc0000000, // 2
    0xe0000000, // 3
    0xf0000000, // 4
    0xf8000000, // 5
    0xfc000000, // 6
    0xfe000000, // 7
    0xff000000, // 8
    0xff800000, // 9
    0xffc00000, // 10
    0xffe00000, // 11
    0xfff00000, // 12
    0xfff80000, // 13
    0xfffc0000, // 14
    0xfffe0000, // 15
    0xffff0000, // 16
    0xffff8000, // 17
    0xffffc000, // 18
    0xffffe000, // 19
    0xfffff000, // 20
    0xfffff800, // 21
    0xfffffc00, // 22
    0xfffffe00, // 23
    0xffffff00, // 24
    0xffffff80, // 25
    0xffffffc0, // 26
    0xffffffe0, // 27
    0xfffffff0, // 28
    0xfffffff8, // 29
    0xfffffffc, // 30
    0xfffffffe, // 31
    0xffffffff  // 32
};

static const int code_table_size = 256;
//======================================================================
static int find_char(const unsigned int in, int size, char *ch)
{
    for (int i = 0; i < code_table_size; ++i)
    {
        unsigned int d = huffman_decode_table[i][1];
        int len = huffman_decode_table[i][2];
        if ((d == (in & mask_table[len])) && (size >= len))
        {
            *ch = huffman_decode_table[i][0];
            return len;
        }
    }

    return 0;
}
//======================================================================
const unsigned char mask[8] = {0, 1, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f};
//======================================================================
void huffman_decode(const char *s, int len, std::string& out)
{
    out = "";
    unsigned int fifo_buf = 0;
    unsigned int buf = 0;

    int fifo_max_size = 32;
    int fifo_size = fifo_max_size;
    int buf_ind = 0;

    for ( ; ; )
    {
        for ( ; fifo_size > 0; )
        {
            if ((len > 0) && (buf_ind == 0))
            {
                buf = *((unsigned char*)s++);
                buf_ind = 8;
                len--;
            }

            if (buf_ind == 0)
                break;

            if (fifo_size < buf_ind)
            {
                buf_ind -= fifo_size;
                fifo_size = 0;
                fifo_buf |= (buf >> buf_ind);
                buf &= mask[buf_ind];
            }
            else // fifo_size > buf_ind
            {
                fifo_size -= buf_ind;
                buf_ind = 0;
                if (fifo_size)
                    buf <<= fifo_size;
                fifo_buf |= buf;
            }
        }

        char ch;
        int n = find_char(fifo_buf, fifo_max_size - fifo_size, &ch);
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
    int index;
    unsigned int huff_buf;
    int buf_len;
    int ret = 0;

    unsigned char out_byte = 0;
    int out_bits_len = 0;

    while (*in)
    {
        index = (unsigned char)*(in++);
        huff_buf = huffman_encode_table[index][1];
        buf_len = huffman_encode_table[index][2];
        //if (index > 127)
        //    fprintf(stderr, "? utf-8 <%s:%d> index=%d\n", __func__, __LINE__, index);
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
                buf_len -= (8 - out_bits_len);
                out_byte |= (0xff & (huff_buf >> buf_len));
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
