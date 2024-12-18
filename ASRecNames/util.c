//
//  util.c
//  ASRecNames
//
//  Created by Larry on 7/28/24.
//

#include <sys/xattr.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "c4thread.h"
#include "c4fork.h"
#include "c4inline.h"
#include "gopt.h"
#include "util.h"

typedef struct {
    E_MODEL  hint;
    char     *method;
    void     (*f8)(unsigned char *,uint32_t);
    uint32_t (*f16)(unsigned char *,unsigned char *,uint32_t);
} method_t;

method_t mstruct[] = {
    {M_THREAD, "THREAD", tconv2UTF8MAC, tconv2UTF16},
    {M_FORK, "FORK", fconv2UTF8MAC, fconv2UTF16},
    {M_INLINE, "INLINE", iconv2UTF8MAC, iconv2UTF16}
};

int init(E_MODEL which, char *arg0, void (**pf8)(unsigned char *,uint32_t), uint32_t (**pf16)(unsigned char *, unsigned char *,uint32_t)) {
    
    char *nres = (char *)calloc(30,1);
    char *name = "user.method";
    ssize_t retsz;
    int i, idx = -1;
    
#if defined THREAD
    idx = 0;
#elif defined FORK
    idx = 1;
#elif defined INLINE
    idx = 2;
#endif
    
    // Reset the threading model if an option was supplied to override the default.
    for (i=0; i<(sizeof(mstruct)/sizeof(mstruct[0])); ++i) {
        if (which == mstruct[i].hint){
            idx = i;
            break;
        }
    }
    
    // Done if no override provided.
    if (idx < 0) {
        return 1;
    }
    
    // Override provided. Get the method attribute from the file.
    retsz = getxattr(arg0, name, (void *)nres, 30, 0, 0);
    
    /* If there was no method attribute or there was but it doesn't match
     * the the chosen one rewrite the attribute.
     */
    if ((retsz < 1) || (strcmp(mstruct[idx].method, nres) != 0)) {
        char *cmd  = calloc(300,1);
        
        sprintf(cmd, "xattr -w user.method %s %s", mstruct[idx].method, arg0);
        system(cmd);
        free (cmd);
    }
    
    // Set the support function routine pointers.
    *pf8  = mstruct[idx].f8;
    *pf16 = mstruct[idx].f16;
    
    free (nres);
    
    return 0;
}

// Counts in output list file are big endian but are little endian on the Mac. Use this
// routine because the source numbers may not be aligned in memory.
uint32_t REVE4(unsigned char *s)
{
    uint32_t np;
    
    memcpy((char *)&np, s, 4);
    np = (((np&0xFF)<<24) | ((np&0xFF00)<<8) | ((np&0xFF0000)>>8) | ((np&0xFF000000)>>24));
    return np;
}

// Increment a big-endian value
void incBE(unsigned char *s, int x) {
    uint32_t np = REVE4(s);
        
    np+=x;
    np = REVE4((unsigned char *)&np);
    memcpy((char *)s, (char *)&np,  4);
}

off_t myfsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return 0;
}


