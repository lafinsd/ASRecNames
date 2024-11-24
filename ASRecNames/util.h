//
//  util.h
//  ASRecNames
//
//  Created by Larry on 7/28/24.
//

#ifndef util_h
#define util_h

#define BUFSIZE 200
#define MY_DEBUG
#define USAGE_FMT "%s [-t|-k|-i] [-p] <infile>\n"

int      init(E_MODEL, char *, void (**)(unsigned char *, uint32_t), uint32_t (**)(unsigned char *, unsigned char *, uint32_t));
void     incBE(unsigned char *, int);
uint32_t REVE4(unsigned char *);
off_t    myfsize(const char *filename);

#endif /* util_h */

