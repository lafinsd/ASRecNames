//
//  c4thread.c
//  ASRecNames
//
//  Created by Larry on 7/28/24.
//

#include <sys/stat.h>
#include <sys/xattr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#include <signal.h>
#define _REENTRANT
#include <pthread.h>
#include "c4thread.h"
#include "gopt.h"
#include "util.h"

static void *conv2UTF16t (void *);
static void *conv2UTF8MACt(void *);

uint32_t tconv2UTF16(unsigned char *name, unsigned char *out, uint32_t len) {
    targs_t x;
    pthread_t tid;
    
    x.u16.name = (char *)name;
    x.u16.len  = len;
    x.u16.out  = (char *)out;
    
    pthread_create(&tid, NULL, conv2UTF16t, &x);
    pthread_join(tid, NULL);
    
    return x.u16.res;
}

void *conv2UTF16t (void *s) {
    targs_t *ls = (targs_t *)s;
    char *buf;
    FILE *fp;
    off_t fsize;
    char infname[50], outfname[50], cmd[L_tmpnam];
    
    sprintf(infname, "/tmp/UTF8i_%d", getpid());
    sprintf(outfname, "/tmp/UTF16o_%d", getpid());
    sprintf(cmd, "iconv -t UTF-16-SWAPPED -f UTF-8-MAC < %s > %s", infname, outfname);
    
    fp = fopen(infname, "wb");
    fwrite(ls->u16.name, ls->u16.len, 1, fp);
    fclose (fp);
    
    system(cmd);

    fsize = myfsize(outfname);
    buf   = malloc(fsize);
    fp    = fopen(outfname, "rb");
    fread(ls->u16.out,fsize,1,fp);
    fclose(fp);
    
#ifndef MY_DEBUG
    unlink(infname);
    unlink(outfname);
#endif
    
    ls->u16.res = (uint32_t)fsize;
    pthread_exit(NULL);
}


void tconv2UTF8MAC(unsigned char *name, uint32_t len) {
    targs_t x;
    pthread_t tid;
    
    x.u8.name = (char *)name;
    x.u8.len  = len;
    
    pthread_create(&tid, NULL, conv2UTF8MACt, &x);
    pthread_join(tid, NULL);
    return;
}

void *conv2UTF8MACt(void *s) {
    targs_t *ls = (targs_t *)s;
    char *buf;
    FILE *fp;
    off_t fsize;
    char infname[50], outfname[50], cmd[L_tmpnam];
    
    sprintf(infname, "/tmp/UTF16i_%d", getpid());
    sprintf(outfname, "/tmp/UTF8o_%d", getpid());
    sprintf(cmd, "iconv -f UTF-16-SWAPPED -t UTF-8-MAC < %s > %s", infname, outfname);
    
    fp = fopen(infname, "wb");
    fwrite(ls->u8.name, ls->u8.len, 1, fp);
    fclose (fp);

    system(cmd);
    
    fsize = myfsize(outfname);
    buf   = malloc(fsize);
    fp    = fopen(outfname, "rb");
    fread(buf,fsize,1,fp);
    fclose(fp);
    wprintf(L"%s", buf);
    free (buf);
    
#ifndef MY_DEBUG
    unlink(infname);
    unlink(outfname);
#endif
    
    return(NULL);
}


