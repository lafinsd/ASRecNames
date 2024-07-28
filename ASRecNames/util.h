//
//  util.h
//  ASRecNames
//
//  Created by Larry on 7/28/24.
//

#ifndef util_h
#define util_h

#define THREAD
#define MY_DEBUG

int      init(char, const char *, void (**)(unsigned char *, uint32_t), uint32_t (**)(unsigned char *, unsigned char *, uint32_t));
void     incBE(unsigned char *, int);
uint32_t REVE4(unsigned char *);
off_t    myfsize(const char *filename);

#endif /* util_h */

