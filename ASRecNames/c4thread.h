//
//  c4thread.h
//  ASRecNames
//
//  Created by Larry on 7/28/24.
//

#ifndef c4thread_h
#define c4thread_h

uint32_t tconv2UTF16(unsigned char *, unsigned char *, uint32_t);
void tconv2UTF8MAC(unsigned char *, uint32_t);

typedef struct  {
    char     *name;
    uint32_t len;
    char     *out;
    uint32_t res;
} u16targs_t;

typedef struct  {
    char     *name;
    uint32_t len;
} u8targs_t;

typedef union {
    u16targs_t u16;
    u8targs_t  u8;
} targs_t;

#endif /* c4thread_h */

