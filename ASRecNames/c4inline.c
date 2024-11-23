//
//  c4inline.c
//  ASRecNames
//
//  Created by Larry on 7/28/24.
//

#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#include <stdio.h>
#include "gopt.h"
#include "util.h"

void iconv2UTF8MAC(unsigned char *name, uint32_t len) {
    FILE *fp;
    off_t fsize;
    char infname[50], outfname[50], cmd[L_tmpnam], *buf;
    
    sprintf(infname, "/tmp/UTF16i_%d", getpid());
    sprintf(outfname, "/tmp/UTF8o_%d", getpid());
    sprintf(cmd, "iconv -f UTF-16-SWAPPED -t UTF-8-MAC < %s > %s", infname, outfname);
    
    fp = fopen(infname, "wb");
    fwrite(name, len, 1, fp);
    fclose (fp);
    
    system(cmd);

    fsize = myfsize(outfname);
    buf = malloc(fsize);
    fp = fopen(outfname, "rb");
    fread(buf,fsize,1,fp);
    fclose(fp);
    wprintf(L"%s", buf);
    free (buf);
#ifndef MY_DEBUG
    unlink(infname);
    unlink(outfname);
#endif
    return;
}

uint32_t iconv2UTF16(unsigned char *name, unsigned char *out, uint32_t len) {
    FILE *fp;
    off_t fsize;
    char infname[50], outfname[50], cmd[L_tmpnam];
    
    sprintf(infname, "/tmp/UTF8i_%d", getpid());
    sprintf(outfname, "/tmp/UTF16o_%d", getpid());
    sprintf(cmd, "iconv -t UTF-16-SWAPPED -f UTF-8-MAC < %s > %s", infname, outfname);
    
    fp = fopen(infname, "wb");
    fwrite(name, len, 1, fp);
    fclose (fp);
    
    system(cmd);

    fsize = myfsize(outfname);
    fp = fopen(outfname, "rb");
    fread(out,fsize,1,fp);
    fclose(fp);
#ifndef MY_DEBUG
    unlink(infname);
    unlink(outfname);
#endif
    return (uint32_t)fsize;
}


