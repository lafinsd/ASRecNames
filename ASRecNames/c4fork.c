//
//  c4fork.c
//  ASRecNames
//
//  Created by Larry on 7/28/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>
#include "gopt.h"
#include "util.h"

void fconv2UTF8MAC(unsigned char *name, uint32_t len) {
    pid_t pid;
    FILE *fp;
    off_t fsize;
    char infname[50], outfname[50], cmd[L_tmpnam], *buf;
    
    pid = fork();
    
    if (pid > 0) {
        int status;
        
        do {
            waitpid(pid, &status, 0);
        } while (WIFEXITED(status) == 0);
    }
    else if (pid == 0) {
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
        exit(0);
    }
    else {
        printf("fork() failed\n");
    }
    return;
}

uint32_t fconv2UTF16(unsigned char *name, unsigned char *out, uint32_t len) {
    pid_t pid;
    int fd[2];
    
    pipe(fd);
    pid = fork();
    
    if (pid > 0) {
        int status;
        off_t res;
        
        do {
            waitpid(pid, &status, 0);
        } while (WIFEXITED(status) == 0);
        
        read(fd[0], &res, sizeof(off_t));
        read(fd[0], out, res);
        close(fd[0]);

        return (int32_t)res;
    }
    else if (pid == 0) {
        char *buf;
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
        buf = malloc(fsize);
        fp = fopen(outfname, "rb");
        fread(buf,fsize,1,fp);
        fclose(fp);
#ifndef MY_DEBUG
        unlink(infname);
        unlink(outfname);
#endif
        write(fd[1], &fsize, sizeof(off_t));
        write(fd[1],buf,fsize);
        close(fd[1]);
        free(buf);
        exit(0);
    } else {
        printf("fork() failed\n");
        return 0;
    }
}

