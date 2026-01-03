#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

// ------------------------- Data structures

#define OBJ_INT 1
#define OBJ_STR 2
#define OBJ_BOOL 3
#define OBJ_LIST 4
#define OBJ_SYMBOL 5

typedef struct obj {
    int refcount;
    int type; // OBJ_*
    union {
        int i;
        struct {
            char * ptr;
            size_t len;
        } str;
        struct {
            struct obj **elements;
            size_t len;
        } list;
    };
} obj;

typedef struct parser {
    char *prg; // entire program
    char *p; // next token to parse
} parser;

// execution context
typedef struct ctx {
    obj *stack;
} ctx;

// ------------------------- allocation wrappers

void *xmalloc(size_t size) {
    void *res = malloc(size);
    if (res == NULL) {
        fprintf(stderr, "unable to allocate %zu bytes of memory", size);
        exit(1);
    }

    return res;
}

// ------------------------- DS functions

obj *createobj(int t) {
    obj * o = xmalloc(sizeof(obj));
    o->type = t;
    o->refcount = 1;

    return o;
}

obj *makeint(int i) {
    obj *res = createobj(OBJ_INT);
    res->i = i;

    return res;
}

obj *makestring(char *p, int len) {
    obj *res = createobj(OBJ_STR);

    res->str.ptr = p;
    res->str.len = len;

    return res;
}

obj *makebool(int i) {
    obj *res = createobj(OBJ_BOOL);
    res->i = i;

    return res;
}

// list

obj *makelist() {
    obj *res = createobj(OBJ_LIST);

    res->list.elements = NULL;
    res->list.len = 0;

    return res;
}

void listPush(obj * o, obj * o2) {
    o->list.elements = realloc(o->list.elements, sizeof(obj*) * (o->list.len + 1));
    o->list.elements[o->list.len] = o2;
    o->list.len++;
}

// symbol is our function symbol
obj *makesymbol() {
    obj *res = createobj(OBJ_STR);

    res->type = OBJ_SYMBOL;

    return res;
}

void parseSpaces(parser *par) {
    while ( isspace(*par->p) ) {
        par->p++;
    }
}

#define MAX_NUM_LEN 128
obj *parseNumber(parser *par) {
    char buf[MAX_NUM_LEN];
    char *start = par->p;
    char *end;

    if (par->p[0] == '-') par->p++;
    while (par->p[0] && isdigit(par->p[0])) {
        par->p++;
    }
    end = par->p;
    int numlen = end - start;
    if (end - start > MAX_NUM_LEN) {
        return NULL;
    }

    memcpy(buf, start, numlen);
    buf[numlen] = 0; // explicitly set to 0


    return makeint(atoi(buf));
}

// compile

obj * compile(char * prg) {
    parser * par = xmalloc(sizeof(parser));
    par->prg = prg;
    par->p = prg;

    obj * parsed = makelist();

    while (par->p) {
        obj *o;
        char * token_start = par->p;

        parseSpaces(par);

        if (*par->p == 0) { break; }

        if (isdigit(par->p[0]) || par->p[0] == '-') {
            o = parseNumber(par);
        } else {
            o = NULL;
        }

        if (o == NULL) {
            printf("unexpected token %10s ...\n", token_start);

            return NULL;
        } else {
            listPush(parsed, o);
        }
    }

    return parsed;
}

// ------------------------- execution

void exec(obj *prg) {

    for (size_t j = 0; j < prg->list.len; j++) {
        obj *o = prg->list.elements[j];
        switch (o->type) {
            case OBJ_INT:
                printf("%d", o->i);
                break;
            default:
                printf("?");
                break;
        }

        printf(" ");
    }

    printf("\n");
}


// ------------------------- Main


// argc argument count
// argv argument vector

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);

        return 1;
    }

    // read program into memory for parsing
    printf("reading file: %s...\n", argv[1]);

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "unable to open file %s\n", argv[1]);

        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fprintf(stdout, "file length is %d\n", len);

    char * prg_text = xmalloc(len+1); // +1 so we have a null byte in the end.

    rewind(fp);
    size_t bytesread = fread(prg_text, 1, len, fp);
    printf("read byte %lu\n", bytesread);
    prg_text[len] = 0; // explicit null termination (?)
    fclose(fp);
    // done reading a file!

    printf("printing file contents...\n");
    printf("%s\n", prg_text);

    obj *prg = compile(prg_text);
    exec(prg);

    return 0;
}
