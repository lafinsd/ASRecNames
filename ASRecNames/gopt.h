//
//  gopt.h
//  ASRecNames
//
//  Created by Larry on 11/21/24.
//

#ifndef gopt_h
#define gopt_h

#define OPT_STRING "tikp"

typedef enum {
    M_NONE = 0,
    M_THREAD,
    M_INLINE,
    M_FORK
} E_MODEL;

typedef enum {
    O_NONE = 0,
    O_FILE,
    O_PRINT
} E_OUTT;

typedef struct {
    char    *name;
    E_MODEL  model;
} M_PAIR;

typedef struct {
    int     argc;
    char  **argv;
    E_MODEL model;
    E_OUTT  out_type;
    int     optind;
} GOPT;

extern int procopt(GOPT *);

#endif /* gopt_h */
