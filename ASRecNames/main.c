//
//  main.c
//  ASRecNames
//
//  Created by Larry on 8/28/24.
//
// This routine creates a list of Applescript Record key names. If the
// Value of a Key:Value pair is also a Record or a list containing a Record
// a sublist will be (recursively) created.
//
// The approach here was to reverse engineer the binary format of the Applescipt
// Record file. This of course is fraught with risk. I test this code every time
// I run across a Record definition in Applescript code I've never seen before.
//

#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "util.h"

static int procRec(void);
static int procRecList(void);
static int procList(unsigned char *);
static int procToken(unsigned char *);
static void procText(int);
static uint32_t procOther(uint32_t);
static uint32_t procOtherRec(uint32_t, unsigned char *);

// pretty-print support
static void pname(unsigned char *, uint32_t);
static void pLB(void);
static void pRB(void);
static void (*pfunc2UTF8MAC)(unsigned char *, uint32_t);
static uint32_t (*pfunc2UTF16)(unsigned char *, unsigned char *, uint32_t);

static unsigned char *gIbuf;
static unsigned char *gObuf;
static unsigned char *gAllocIbuf;
static uint32_t gOPtr = 0;
static char gLP = ' ';


int main(int argc, const char * argv[]) {
    FILE *fp, *fpout;
    off_t fsize;
    int res, os = 0;
    unsigned char *allocOBuf, whichVersion = ' ';
    
    if (argc > 1) {
        whichVersion = *argv[1];
        if ((whichVersion == 't') || (whichVersion == 'k') || (whichVersion == 'i')) {
            os++;
            argc--;
        }
    }
    
    if ((res=init(whichVersion, argv[0], &pfunc2UTF8MAC, &pfunc2UTF16)) != 0) {
        printf("init() failed: error code %d\n", res);
        return res;
    }

    if ((argc < 3) || (argc > 4)) {
        printf("\nERROR: bad arg count %d\n", argc - 1);
        return 1;
    }
    
    whichVersion = *argv[1+os];
    
    if (whichVersion != 'f' && whichVersion != 'p') {
        printf("\nERROR: bad arg %c\n", whichVersion);
        return 1;
    }
    
    if (whichVersion == 'p' && argc != 3) {
        printf("\nERROR: bad arg count\n");
        return 1;
    }
    
    if (whichVersion == 'f' && argc != 4) {
        printf("\nERROR: bad arg count\n");
        return 1;
    }
    
    fp = fopen(argv[2+os], "rb");
    if (fp == NULL) {
        printf("\n**ERROR: file %s does not exist\n", argv[2+os]);
        return 1;
    }
    fsize = myfsize(argv[2+os]);
    
    if (fsize == 0) {
        printf("\n**ERROR: file %s: bad file size\n", argv[2+os]);
        return 1;
    }
    
    gAllocIbuf  = gIbuf  = (unsigned char *)malloc((unsigned long)fsize);
    
    fread(gIbuf,fsize,1,fp);
    fclose (fp);
    
    if (whichVersion == 'f') {
        fpout = fopen(argv[3+os], "wb");
        if (fpout == NULL) {
            printf("\n**ERROR: file %s cannot be opened\n", argv[3+os]);
            return 1;
        }
        // Output file won't be bigger than input file
        allocOBuf = gObuf = (unsigned char *)malloc((unsigned long)fsize);
#ifdef MY_DEBUG
        memset(gObuf, 0xFF, fsize);
#endif
        res = procRecList();
        fwrite(gObuf, gOPtr, 1, fpout);
        fclose (fpout);
        free (allocOBuf);
    }
    else {
        res = procRec();
        printf("\n\nDone\n\n");
    }
    
    free (gAllocIbuf);
    
    return 0;
}


// Applescript classes (like 'boolean' or 'color') can be record names. They
// really should not be used. If these are really needed they should be enclosed
// in protective vertical bars, for example, |record|. If these reserved words
// are used without the vertical bars they will appear first in a properly
// formatted Reocrd object as a four character token. They are processed here.
//
// Returns:
//   1: The current item is 'usrf'. More processing will follow.
//   0: No more items
//

static uint32_t procOtherRec(uint32_t nrectypes, unsigned char *lcntptr) {
    uint32_t i;
    
    for (i=1; i<=nrectypes; ++i) {
        //check clue
        if(strncmp((const char *)gIbuf, "usrf", 4) == 0) {
            // A user list now follows. Return for more processing
            return 1;
        }
        
        // Create list item in output buffer.
        memcpy(gObuf+gOPtr, "utxt", 4);
        gOPtr += 4;
        {
            uint32_t n1, n = 8;
            n1 = REVE4((unsigned char *)&n);
            memcpy((char *)(gObuf+gOPtr), (char *)&n1, 4);
            gOPtr += 4;
        }
        
        // Applescript uses UTF-16 for the text rendering. Convert
        // the clue and place it in the output.
        gOPtr += pfunc2UTF16((unsigned char *)gIbuf, gObuf+gOPtr, 4);
        gIbuf += 4;
        
        // List of record names will contain this new item. Increment the list item count.
        incBE(lcntptr, 1);
        
        // Key processed. Now process the Value.
        procToken(lcntptr);
    }
    return 0;
}

// This processes the 'other' Record names but for the printing context.
// See 'procOtherRec()' above.
static uint32_t procOther(uint32_t ntypes) {
    char *name;
    uint32_t i;
    
    for (i=1; i<=ntypes; ++i) {
        //check clue
        if(strncmp((const char *)gIbuf, "usrf", 4) == 0) {
            return 1;
        }
        
        name = calloc(5,1);
        strncpy(name, (const char *)gIbuf, 4);
        gIbuf += 4;
        pname((unsigned char *)name, 0);
        free (name);
        
        // Process the Token ('Value') part of the Key:Value pair. This item
        // may be another Record, or a list containing a Record.
        procToken(0);
    }
    return 0;
}

// Main routine to prcoess a Record (recursively) for the prinitng context.
static int procRec (void) {
    uint32_t i, tmp, nrectypes, nrt;
    volatile uint32_t numpairs;

    // sanity check
    if (strncmp((const char *)gIbuf, "reco", 4) != 0) {
        printf("\n**ERROR: 'reco' not found where expected: %p\n", gIbuf);
        return 1;
    }
    gIbuf += 4;  // skip over 'reco'
    
    nrectypes = REVE4(gIbuf); // number of record types
    gIbuf += 4;
    
    pLB();
    
    // First look for Record names ('Keys') that are Applescript classes. A class
    // (such as 'boolean' or 'integer') can be Record name.
    nrt = procOther(nrectypes);
    if (nrt == 0) {
        // We're done. Only classes were Record names.
        pRB();
        return 0;
    }
    
    // 'usrf' was found. There follows a list of User names for Record names.
    // Process them.
    gIbuf += 4; // skip 'usrf'
    
    if (strncmp((const char *)gIbuf, "list", 4) != 0) {
        printf("\n**ERROR: 'list' not found where expected\n: %p\n", gIbuf);
        return 1;
    }
    gIbuf += 4; // skip 'list'
    
    // next is the number of key/value items
    memcpy(&tmp, gIbuf, 4);
    
#ifdef MY_DEBUG
    numpairs = REVE4((unsigned char *)&tmp);
    if ((numpairs & 1) == 1) {
        printf("Odd numpairs\n");
    }
    numpairs = numpairs/2;
#else
    numpairs = REVE4((unsigned char *)&tmp)/2;
#endif
    
    gIbuf += 4;
    
    for (i=1; i<=numpairs; ++i) {
        // should start with 'utxt' as the key name
        if (strncmp((const char *)gIbuf, "utxt", 4) != 0) {
            printf("\n**ERROR: 'utxt' not found where expected: %p\n", gIbuf);
            return 1;
        }
        
        procText(1);
        
        if (strncmp((const char *)(gIbuf), "reco", 4) == 0) {
            procRec();
            continue;
        }
        
        procToken(0);
    }
    pRB();
    
    return 0;
}


static int procToken(unsigned char *ptr) {
    const char *tok = (const char *)gIbuf;
    uint32_t skip;
    
    // The token argument can be almost any object. Don't know if I'll
    // ever catch them all. Many require specific processing and may
    // lead to recursive calls.
    if (strncmp(tok, "furl", 4) == 0) {
        gIbuf += 4; // skip the token describing the value associated with the key
        skip = REVE4 (gIbuf) + 1;
    }
    else if (strncmp(tok, "list", 4) == 0) {
        procList(ptr);
        return 0;
    }
    else if (strncmp(tok, "scpt", 4) == 0) {
        gIbuf += 4; // skip the token describing the value associated with the key
        skip = REVE4 (gIbuf);
    }
    else if (strncmp(tok, "alis", 4) == 0) {
        gIbuf += 4; // skip the token describing the value associated with the key
        skip = REVE4 (gIbuf);
    }
    else if (strncmp(tok, "type", 4) == 0) {
        gIbuf += 4; // skip "type"
        skip = REVE4 (gIbuf); // # of characters in the type description
    }
    else if (strncmp(tok, "reco", 4) == 0) {
        if (ptr != 0) {
            incBE(ptr, 1);
            procRecList();
        }
        else {
            procRec();
        }
        return 0;
    }
    else {
        gIbuf += 4; // skip the token describing the value associated with the key
        skip = REVE4 (gIbuf); // # of characters in the value
    }
    gIbuf += 4; // skip the skip count
    gIbuf += skip; // skip the Value characters
    
    return 0;
}

static int procList(unsigned char *ptr) {
    uint32_t numitems, i;
    
    gIbuf += 4; // skip 'list'
    
    numitems = REVE4 (gIbuf);
    gIbuf += 4; // skip item count

    for (i=1; i<=numitems; ++i) {
        const char *tok = (const char *)gIbuf;
        
        if (strncmp(tok, "utxt", 4) == 0) {
            procText(0);
        }
        else if (strncmp(tok, "long", 4) == 0) {
            uint32_t lsz;
            
            gIbuf += 4;
            lsz = REVE4 (gIbuf);
            gIbuf += lsz + 4; // number of bytes in the long plus the length clue
        }
        else if (strncmp(tok, "doub", 4) == 0) {
            uint32_t lsz;
            
            gIbuf += 4;
            lsz = REVE4 (gIbuf);
            gIbuf += lsz + 4; // number of bytes in the double plus the length clue
        }
        else if (strncmp(tok, "scpt", 4) == 0) {
            gIbuf += 4; // skip the token describing the value associated with the key
            gIbuf +=  (REVE4 (gIbuf) + 1);
        }
        else if (strncmp(tok, "reco", 4) == 0) {
            if (ptr != 0) {
                incBE(ptr, 1);
                procRecList();
            }
            else {
                procRec();
            }
        }
        else if (strncmp(tok, "list", 4) == 0) {
            procList(ptr);
        }
#ifdef MY_DEBUG
        else {
            printf("procList() unknown type at: %p\n", gIbuf);
        }
#endif
    }
    return 0;
}

// Skip past a name. It could be just a list name. Print it and return it
// if requested: it's a Record name and we're in the print context.
static void procText(int prnt) {
    int namechars;
    
    gIbuf += 4; // skip 'utxt'
    namechars = REVE4 (gIbuf);
    gIbuf += 4; // skip character count
    
    if (prnt != 0) {
        pname(gIbuf, namechars);
    }
    
    gIbuf+=namechars; // skip what we just processed
    
    return;
}

// Main routine to process a Record (recursively) for creating an Applescript
// list of all record names. The flow is the same as 'procRec()': process the
// Record names that are Applescript classes, then the 'usrf' User specified
// names.
static int procRecList(void) {
    uint32_t i, numpairs, nrectypes;
    unsigned char *lcntptr;
    
    // sanity check
    if (strncmp((const char *)gIbuf, "reco", 4) != 0) {
        printf("\n**ERROR: 'reco' not found where expected: %p\n", gIbuf);
        return 1;
    }
    gIbuf += 4;  // skip over 'reco'
    
    memcpy((char *)(gObuf+gOPtr), "list", 4);
    gOPtr += 4;
    
    lcntptr = gObuf + gOPtr;
    memset(lcntptr, 0x00, 4);
    gOPtr += 4;
    
    nrectypes = REVE4(gIbuf); // number of record types
    gIbuf += 4;  // skip over record count
    
    if (procOtherRec(nrectypes, lcntptr) == 0) {
        // Done...no more record names after processing the special ones.
        return 0;
    }
    
    // sanity checks
    if (strncmp((const char *)gIbuf, "usrf", 4) != 0) {
        printf("\n**ERROR: 'usrf' not found where expected: %p\n", gIbuf);
        return 1;
    }
    gIbuf += 4; // skip 'usrf'
    
    if (strncmp((const char *)gIbuf, "list", 4) != 0) {
        printf("\n**ERROR: 'list' not found where expected: %p\n", gIbuf);
        return 1;
    }
    gIbuf += 4; // skip 'list'
    
    // next is the number of key/value items
    memcpy(&numpairs, gIbuf, 4);
    numpairs = REVE4((unsigned char *)&numpairs);
    
#ifdef MY_DEBUG
    if ((numpairs & 1) == 1) {
        printf("Odd numpairs\n");
    }
    numpairs = numpairs/2;
#else
    numpairs = numpairs/2; // this is the number of list items: list is a list of keys of each pair
#endif
    
    gIbuf += 4;
    
    incBE(lcntptr, numpairs);
    
    for (i=0; i<numpairs; ++i) {
        uint32_t cnum;
        
        // should start with 'utxt' as the key name
        if (strncmp((const char *)gIbuf, "utxt", 4) != 0) {
            printf("\n**ERROR: 'utxt' not found where expected: %p\n", gIbuf);
            return 1;
        }
        memcpy((char *)(gObuf+gOPtr), gIbuf, 8);  // copy 'utxt' and BE count of UTF-8 characters
        gOPtr+=8;
        
        gIbuf+=4;
        cnum = REVE4 (gIbuf);
        gIbuf+=4;
        memcpy((char *)(gObuf+gOPtr), gIbuf, cnum);
        gOPtr+=cnum;
        gIbuf+=cnum;
        
        // An embedded record (when the value of a record key is also a record) increases the resulting list count
        // in the output buffer since the output record is a list of record keys. This new list will be
        // a sublist so the count has to be incremented.
        if (strncmp((const char *)(gIbuf), "reco", 4) == 0) {
            incBE(lcntptr, 1);
            procRecList();
            continue;
        }
        
        procToken(lcntptr);
    }
    
    return 0;
}

static void pRB(void) {
    printf("}");
    gLP = '}';
    
    return;
}
    
static void pLB(void) {
    if (gLP == 'n') {
        printf(", ");
    }
    printf ("{");
    gLP = '{';
        
    return;
}

static void pname (unsigned char *name, uint32_t len) {
    switch (gLP) {
    case '}':
    case 'n':
        printf(", ");
    }
    
    if (len != 0) {
        // Applescript uses UTF-16 internally. Convert for printing.
        pfunc2UTF8MAC(name, len);
    } else {
        printf("%s", name);
    }
    
    gLP = 'n';
    return;
}
