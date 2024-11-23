//
//  gopt.c
//  ASRecNames
//
//  Created by Larry on 11/19/24.
//

#include <stdio.h>
#include <getopt.h>
#include <libgen.h>    // for basename()

#include "gopt.h"
#include "util.h"

int procopt(GOPT *p) {
    
    int gopt;
    
    opterr = 0;   // suppress error message from getopt()
    p->model    = M_NONE;
    p->out_type = O_FILE;
    while ((gopt=getopt(p->argc, p->argv, OPT_STRING)) != -1) {
        switch (gopt) {
            case 't':
                if (p->model != M_NONE) {
                    printf("Multiple model options \"%s\"\n", p->argv[optind-1]);
                    printf(USAGE_FMT, basename(p->argv[0]));
                    return (1);
                }
                p->model = M_THREAD;
                break;
                
            case 'i':
                if (p->model != M_NONE) {
                    printf("Multiple model options \"%s\"\n", p->argv[optind-1]);
                    printf(USAGE_FMT, basename(p->argv[0]));
                    return (1);
                }
                p->model = M_INLINE;
                break;
                
            case 'k':
                if (p->model != M_NONE) {
                    printf("Multiple model options \"%s\"\n", p->argv[optind-1]);
                    printf(USAGE_FMT, basename(p->argv[0]));
                    return (1);
                }
                p->model = M_FORK;
                break;
   
            case 'p':
                p->out_type = O_PRINT;
                break;
                
            case '?':
            default:
                printf("Illegal option \"%s\"\n", p->argv[optind-1]);
                printf(USAGE_FMT, basename(p->argv[0]));
                return (1);
        }
    }
    
    if (optind == ((p->argc)-1)) {
        printf("No output file specified.\n");
        return (1);
    }
    
    p->optind = optind;
    return 0;
}

