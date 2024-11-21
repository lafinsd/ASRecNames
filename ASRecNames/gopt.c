//
//  gopt.c
//  ASRecNames
//
//  Created by Larry on 11/19/24.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>    // for basename()

#include "igbTool.h"
#include "gopt.h"

int procopt(GOPT *p) {
    
        int offset, gopt;
        
        opterr = 0;   // suppress error message from getopt()
        while ((gopt=getopt(p->argc, p->argv, OPT_STRING)) != -1) {
            errno = 0;
            switch (gopt) {
                case 'A':
                    /* This option occurs only when invoked from Automator and causes the
                     * invocation line to appear in the info file. Guard against accidental
                     * confusion if a bogus option interferes.
                     */
                    p->isauto = optind - 1;  // Set 'isauto' to index in argv of the option
                    if (optarg == 0) {
                        printf("Illegal option \"%s\"\n", p->argv[p->isauto]);
                        printf(USAGE_FMT, basename(p->argv[0]));
                        return (1);
                    }
                    else {
                        if (strcmp(OPT_HIDDEN, optarg) != 0) {
                            printf("Illegal option \"%s\"\n", p->argv[p->isauto]);
                            printf(USAGE_FMT, basename(p->argv[0]));
                            return (1);
                        }
                    }
                    break;
                
                case 'f':
                    // Offset provided
                    errno  = 0;
                    offset = (int)strtol(optarg,NULL,10);
                    if (offset > 0) {
                        if (p->pinp->initpage != INITCP) {
                            printf("Multiple non-zero offsets specified\n");
                            exit (-1);
                        }
                        p->pinp->initpage += offset;
                    }
                    else if (offset == 0) {
                        if (errno != 0) {
                            printf("Illegal or missing offset. The -f option requires an offset value.\n");
                            printf(USAGE_FMT, basename(p->argv[0]));
                            exit(-1);
                        }
                    }
                    else {
                        printf("Illegal or missing offset. The -f option requires an offset value.\n");
                        printf(USAGE_FMT, basename(p->argv[0]));
                        exit(-1);
                    }
                    break;
                
                case 'd':
                    // Add dummy entries if needed
                    p->isd = 1;
                    break;
                
                case 'p':
                    // No dummy page numbers in source file
                    p->pinp->isp = 1;
                    break;
                
                case 'c':
                    // Composer name provided
                    p->pinp->defAuthor = calloc((strlen(optarg)+1),1);
                    strcpy(p->pinp->defAuthor, optarg);
                    p->pinp->isc = 1;
                    break;
                
                case '?':
                default:
                    printf("Illegal option \"%s\"\n", p->argv[optind-1]);
                    printf(USAGE_FMT, basename(p->argv[0]));
                    exit(1);
            }
        }
    return 0;
}

