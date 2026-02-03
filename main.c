#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int debug = 0;

// ------------------------- Data structures

#define OBJ_INT 1
#define OBJ_STR 2
#define OBJ_BOOL 3
#define OBJ_LIST 4
#define OBJ_SYMBOL 5

typedef struct obj {
	int refcount; // 4 bytes
	int type; // OBJ_*; 4 bytes
	union {
		int i; // 4 bytes;
		struct {
			char *ptr; // 8 bytes;
			size_t len; // 8 bytes;
		} str;
		struct {
			struct obj **elements; // 8 bytes;
			size_t len; // 8 bytes;
		} list;
	};
} obj;

typedef struct parser {
	char *prg; // entire program
	char *p; // next token to parse
} parser;

typedef struct ctx ctx;

// FunctionTableEntry represents a function name assiciated with its implementation.
typedef struct {
	obj *name;
	// this is builtin function.
	int (*callback)(ctx *c, char *o);
	// this is user defined function.
	int (*userfunction)(ctx *c, char *o);
} FunctionTableEntry;

typedef struct {
	FunctionTableEntry **entries;
	size_t len;
} FunctionTable;

// execution context
typedef struct ctx {
	obj *stack;
	FunctionTable functable;
} ctx;

// ------------------------- allocation wrappers

// xmalloc wraps around malloc,
// iensures memory is allocated, othewise entire
// program will crash.
void *xmalloc(size_t size) {
	void *res = malloc(size);
	if (res == NULL) {
		fprintf(stderr, "unable to allocate %zu bytes of memory", size);
		exit(1);
	}

	return res;
}

// xrealloc is a wrapper around realloc
void *xrealloc(void *oldptr, size_t size) {
	void *ptr = realloc(oldptr, size);
	if (ptr == NULL) {
		fprintf(stderr, "unable to allocate %zu bytes of memory", size);
		exit(1);
	}

	return ptr;
}

// ------------------------- DS functions

obj *createobj(int t) {
	obj *o = xmalloc(sizeof(obj));
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

	res->str.ptr = xmalloc(len + 1);
	res->str.len = len;

	memcpy(res->str.ptr, p, len);
	res->str.ptr[len] = 0;

	return res;
}

obj *makebool(int i) {
	obj *res = createobj(OBJ_BOOL);
	res->i = i;

	return res;
}

void printObject(obj *o) {
	switch (o->type) {
	case OBJ_SYMBOL:
		printf("%s", o->str.ptr);

		break;
	case OBJ_STR:
		printf("\"%s\"", o->str.ptr);

		break;
	case OBJ_INT:
		printf("%d", o->i);

		break;
	case OBJ_LIST:
		printf("[");
		for (size_t j = 0; j < o->list.len; j++) {
			obj *elem = o->list.elements[j];
			printObject(elem);
			if (j != o->list.len - 1) {
				printf(",");
			}
		}
		printf("]");
		printf("\n");

		break;
	default:
		printf("unexpected object type, got %d", o->type);

		return;
	}
}

// ----------------------------- list & symbol objects ------------------------

obj *makelist() {
	obj *res = createobj(OBJ_LIST);

	res->list.elements = NULL;
	res->list.len = 0;

	return res;
}

// symbol is our function symbol
obj *makesymbol(char *p, int len) {
	obj *res = createobj(OBJ_SYMBOL);

	res->str.ptr = xmalloc(len + 1);
	res->str.len = len;

	memcpy(res->str.ptr, p, len);
	res->str.ptr[len] = 0;

	return res;
}

obj *cloneobj(obj *o) {
	if (o == NULL) {
		return o;
	}

	switch (o->type) {
	case OBJ_INT:
		obj *o2 = makeint(o->i);

		return o2;
	}

	return NULL;
}

// compareStringObjects does return 0 if both objects are equal,
// returns 1 otherwise.
int compareStringObjects(obj *a, obj *b) {
	assert(a != NULL && b != NULL);
	assert((a->type == OBJ_STR || a->type == OBJ_SYMBOL) && (b->type == OBJ_STR || b->type == OBJ_SYMBOL));

	if (a->str.len != b->str.len) {
		return 1;
	}

	int cmp = memcmp(a->str.ptr, b->str.ptr, a->str.len);
	if (cmp != 0) {
		return 1;
	}

	return 0;
}

// ---------------------------------------------------------------------------

void listPush(obj *o, obj *o2) {
	o->list.elements =
	    xrealloc(o->list.elements, sizeof(obj *) * (o->list.len + 1));

	o->list.elements[o->list.len] = o2;
	o->list.len++;
}

void parseSpaces(parser *par) {
	while (isspace(*par->p)) {
		par->p++;
	}
}

#define MAX_NUM_LEN 128
obj *parseNumber(parser *par) {
	char buf[MAX_NUM_LEN];
	char *start = par->p;
	char *end = start;

	if (par->p[0] == '-') {
		par->p++;
	}

	while (par->p[0] && isdigit(par->p[0])) {
		par->p++;
		end = par->p;

		if (end - start >= MAX_NUM_LEN) {
			return NULL;
		}
	}

	int numlen = end - start;

	memcpy(buf, start, numlen);

	// explicitly set to 0 since atoi expects null terminated buffer
	buf[numlen] = 0;

	return makeint(atoi(buf));
}

int issymbolchar(int c) {
	char symbols[] = "+-/*%.";

	if (isalpha(c)) {
		return 1;
	}

	if (strchr(symbols, c) != NULL) {
		return 1;
	}

	return 0;
}

obj *parseSymbol(parser *par) {
	char *start = par->p;

	while (par->p[0] && issymbolchar(par->p[0])) {
		par->p++;
	}

	int len = par->p - start;

	return makesymbol(start, len);
}

// compile

void release(obj *o);

obj *compile(char *prg) {
	if (debug) {
		fprintf(stdout, "debug: compilation started\n");
	}

	parser *par = xmalloc(sizeof(parser));
	par->prg = prg;
	par->p = prg;

	obj *parsed = makelist();

	while (par->p) {
		obj *o;
		char *token_start = par->p;

		parseSpaces(par);

		if (*par->p == 0) {
			break;
		}

		if (isdigit(par->p[0]) ||
		    (par->p[0] == '-' && par->p[1] != '\0' && isdigit(par->p[1]))) {
			o = parseNumber(par);
		} else if (issymbolchar(par->p[0])) {
			o = parseSymbol(par);
		} else {
			o = NULL;
		}

		if (o == NULL) {
			release(parsed);
			printf("unexpected token %10s ...\n", token_start);

			return NULL;
		} else {
			listPush(parsed, o);
		}
	}

	if (debug) {
		fprintf(stdout, "debug: compilation finished\n");
	}

	free(par);

	return parsed;
}

// ------------------------- execution

char *filegetcontents(char *pathname) {
	if (debug) {
		fprintf(stdout, "debug: reading file: %s...\n", pathname);
	}

	FILE *fp = fopen(pathname, "r");
	if (fp == NULL) {
		fprintf(stderr, "unable to open file %s\n", pathname);

		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);

	if (debug) {
		fprintf(stdout, "debug: file length is %d\n", len);
	}

	char *prg_text = xmalloc(len + 1); // +1 so we have a null byte in the end.

	rewind(fp);
	size_t bytesread = fread(prg_text, 1, len, fp);
	if (debug) {
		printf("debug: read byte %lu\n", bytesread);
	}

	prg_text[len] = 0; // explicit null termination (?), yes
	fclose(fp);
	// done reading a file!

	if (debug) {
		fprintf(stdout, "debug: printing file contents...\n");
		fprintf(stdout, "debug: %s\n", prg_text);
	}

	return prg_text;
}

void retain(obj *o) {
	o->refcount++;
}

void freeObject(obj *o) {
	switch (o->type) {
	case OBJ_LIST:

		for (size_t i = 0; i < o->list.len; i++) {
			obj *elem = o->list.elements[i];

			release(elem); // what if we can not free o after all?
		}

		break;
	case OBJ_SYMBOL:
	case OBJ_STR:
		free(o->str.ptr);

		break;
	}

	free(o);
}

void release(obj *o) {
	assert(o->refcount > 0);

	o->refcount--;

	if (o->refcount == 0) {
		freeObject(o);
	}
}

// ------------------------- exec with context

int stackLength(ctx *c) {
	assert(c != NULL);
	assert(c->stack->type == OBJ_LIST);

	return c->stack->list.len;
}

obj *stackPop(ctx *c, int type) {
	assert(c != NULL);
	assert(c->stack->type == OBJ_LIST);

	(void)(type);

	int len = stackLength(c);
	if (len == 0) {
		return NULL;
	}

	obj *stack = c->stack;

	// lets just reuse that space?
	// I am thinking no need to call xrealloc here
	// hmm...
	// so I'd have [1,2,3] with lee=3
	// after pop
	// I'd have [1,2,NULL] with len=2

	/* o->list.elements = */
	/*     xrealloc(o->list.elements, sizeof(obj *) * (o->list.len + 1)); */

	obj *pop = stack->list.elements[c->stack->list.len - 1];

	if (stack->list.len == 0) {
		free(stack->list.elements);
		stack->list.elements = NULL;
	}

	stack->list.elements[stack->list.len - 1] = NULL;
	stack->list.len--;

	return pop;
}

void stackPush(ctx *c, obj *o) {
	listPush(c->stack, o);
}

// -------------------------------- std library

int basicMathFunction(ctx *c, char *name) {
	if (stackLength(c) < 2) {
		fprintf(stderr, "Error: stack underflow\n");
		return 1;
	}

	obj *b = stackPop(c, OBJ_INT);
	obj *a = stackPop(c, OBJ_INT);

	if (a == NULL || b == NULL) {
		fprintf(stderr, "Error: expected integer\n");
		return 1;
	}

	int result = 0;

	switch (name[0]) {
	case '+':
		result = a->i + b->i;
		break;
	case '-':
		result = a->i - b->i;
		break;
	case '*':
		result = a->i * b->i;
		break;
	case '/':
		result = a->i / b->i;
		break;
	case '%':
		result = a->i % b->i;
		break;
	}

	stackPush(c, makeint(result));

	return 0;
}

int duplicate(ctx *c, char *unused) {
	if (stackLength(c) == 0) {
		fprintf(stderr, "DUP attempted while stack is empty\n");
		return 1;
	}

	(void)(unused);

	obj *o2 = c->stack->list.elements[c->stack->list.len - 1];

	stackPush(c, o2);
	return 0;
}

// getFunctionByName iterates over function table and returns
// function entry if present.
// todo: why do we search based on obj * name?
// I think we can just use string...
//
// curious from video, we are not modifying reference counting
// ownership transfer is assumed.
FunctionTableEntry *getFunctionByName(ctx *c, obj *o) {
	for (size_t i = 0; i < c->functable.len; i++) {
		FunctionTableEntry *fe = c->functable.entries[i];

		int res = compareStringObjects(fe->name, o);
		if (res == 0) {
			return fe;
		}
	}

	return NULL;
}

FunctionTableEntry *initFunction(ctx *c, obj *o) {
	c->functable.entries = xrealloc(c->functable.entries, sizeof(FunctionTableEntry *) * (c->functable.len + 1));
	FunctionTableEntry *fte = xmalloc(sizeof(FunctionTableEntry));

	c->functable.entries[c->functable.len] = fte;
	c->functable.len++;

	fte->name = o;
	retain(o);

	fte->callback = NULL;
	fte->userfunction = NULL;

	return fte;
}

// registerFunction does install a function into current execution context
//  with given name. The function can't fail since if the function by the same
//  name already exists, it will be replaced by the new one.
void registerFunction(
    ctx *c,
    char *name,
    int (*callback)(ctx *c, char *o)) {

	obj *o2 = makestring(name, strlen(name));
	FunctionTableEntry *fte = getFunctionByName(c, o2);

	if (fte == NULL) {
		// function doesn't exist, create it
		fte = initFunction(c, o2);
		fte->callback = callback;
		return;
	}

	// function exists, update it
	if (fte->userfunction != NULL) {
		fte->userfunction = NULL;
	}

	fte->callback = callback;
	release(o2);
}

// what is it? execution context
// why do we need it?
// dunno
// but for now we throw on stack only numeric values. functions and such are not being put on stack
ctx *createContext(void) {
	ctx *c = xmalloc(sizeof(*c));
	c->stack = makelist();

	c->functable.entries = NULL;
	c->functable.len = 0;

	registerFunction(c, "+", basicMathFunction);
	registerFunction(c, "-", basicMathFunction);
	registerFunction(c, "*", basicMathFunction);
	registerFunction(c, "/", basicMathFunction);
	registerFunction(c, "%", basicMathFunction);
	registerFunction(c, "%", basicMathFunction);
	registerFunction(c, "dup", duplicate);

	return c;
}

// callSymbol is being called during exec-ution.
// it tries to resolve and call the function associated with word, `o` in our case.
// returns 0 on success, 1 on error.
int callSymbol(ctx *c, obj *o) {
	//
	// todo: assert o is of type OBJ_SYMBOL.
	//

	FunctionTableEntry *fte = getFunctionByName(c, o);
	if (fte == NULL) {
		return 1;
	}

	if (fte->userfunction) {
		fte->userfunction(c, o->str.ptr); // ??
	} else if (fte->callback) {
		return fte->callback(c, o->str.ptr); // ??
	}

	// todo: run function...?

	return 0;
}

// exec is the main function of this whole language.
//
int exec(ctx *c, obj *o) {
	assert(o->type == OBJ_LIST);

	for (size_t i = 0; i < o->list.len; i++) {
		obj *word = o->list.elements[i];
		switch (word->type) {
		case OBJ_SYMBOL:
			if (callSymbol(c, word) != 0) {
				printf("Run time errors, oopsied doopsie.");

				return 1;
			}
			break;
		default:
			listPush(c->stack, word);
			retain(word);
		}
	}

	return 0;
}

// ------------------------- main

// argc: argument count
// argv: argument vector

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: %s [-d|--debug] <filename>\n", argv[0]);

		return 1;
	}

	int argidx = 1;
	if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--debug") == 0) {
		debug = 1;
		argidx++;
	}

	char *prg_text = filegetcontents(argv[argidx]);

	// basically a tokenize step.
	obj *prg = compile(prg_text);
	free(prg_text);
	printObject(prg);

	printf("printing program parsed");

	ctx *c = createContext();

	int statuscode = exec(c, prg);
	if (statuscode != 0) {
		printf("failed to execute");
	}

	release(prg);

	printf("Stack context at end: \n");
	printObject(c->stack);

	// freeContext(c);

	return 0;
}
