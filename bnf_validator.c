#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "objects.h"

typedef enum {
    TOK_START = 256,
    TOK_TOKEN,
    TOK_ARROW,
    TOK_PIPE,
    TOK_IDENT,
    TOK_EOF,
    TOK_NEWLINE,
    TOK_ERROR
} TokenType;

#define MAX_IDENT 50

typedef struct {
    char ptr[MAX_IDENT + 1];
    int len;
    TokenType type;
} Token;

/* Lame attempt at reentrancy */
typedef struct State{
    FILE * ptr;
    char * filename;
    int line_no;
} Reader;

static inline char * tok_err_string(Token * tok){
    switch((int) tok->type){
        case TOK_ARROW:
            return "->";
        case TOK_START:
            return "%start";
        case TOK_PIPE:
            return "|";
        case TOK_IDENT:
            return tok->ptr;
        case TOK_NEWLINE:
            return "double newline";
        case TOK_EOF:
            return "End of file";
        case TOK_ERROR:
            {
                if (tok->len != 0)
                    return tok->ptr;
                else return "an invalid token";
            }
        default:
            return "an unknown token";
    }
}


/* Each rule is a representationof the number of symbols in a formulation
 * if s is a grammar of s -> a b | c '+' d. S would be represented as an array of a rule
 *  [Rule(a b), Rule(c, '+', 'd')]. S is  therefore productive if any of these rules
 *  is productive */

Token lex(Reader * file){
    int c;

    // Skip whitespace, track double newlines
    while (1) {
        c = getc(file->ptr);
        if (c != ' ' && c != '\t' && c != '\n') break;

        if (c == '\n') {
            file->line_no++;
            int next = getc(file->ptr);
            if (next == '\n') {
                file->line_no++;
                return (Token){.type = TOK_NEWLINE};
            }
            ungetc(next, file->ptr);
        }
    }

    if (c == EOF) return (Token){.type = TOK_EOF};

    switch(c){
        case '|':
        {
            Token t =  (Token){.type = TOK_PIPE};
            t.ptr[0] = '|', t.ptr[1] = '\0';
            t.len = 1;
            return t;
        }

        case '-':
        {
            if ((c = getc(file->ptr)) != '>') {
                return (Token){.type = TOK_ERROR};
            }
            Token t = (Token){.type = TOK_ARROW};
            t.ptr[0] = '-', t.ptr[1] = '>', t.ptr[2] = '\0';
            t.len = 2;
            return t;
        }

        case '%':
        {
            char buf[10];
            int i = 0;
            while (i < 9 && isalpha((c = getc(file->ptr)))) {
                buf[i++] = c;
            }
            buf[i] = '\0';
            ungetc(c, file->ptr);
            Token t;
            t.ptr[0] = '%';
            memcpy(t.ptr+1, buf, i+1);

            if (strcmp(buf, "start") == 0) { t.type = TOK_START; return t; }
            if (strcmp(buf, "token") == 0) {t.type = TOK_TOKEN; return t; };

            fprintf(stderr, "Unknown directive %%%s at line %d\n", buf, file->line_no);
            return (Token){.type = TOK_ERROR};
        }

        default:
        {
            if (!isalpha(c) && c != '_'){
                return (Token){.type = TOK_ERROR};
            }

            char identifier[MAX_IDENT + 1];
            int len = 0;
            identifier[len++] = c;

            while (len < MAX_IDENT && (isalnum((c = getc(file->ptr))) || c == '_')){
                identifier[len++] = c;
            }

            if (len >= MAX_IDENT && (isalnum(c) || c == '_')){
                fprintf(stderr, "Identifier too long at line %d\n", file->line_no);
                return (Token){.type = TOK_ERROR};
            }

            ungetc(c, file->ptr);
            identifier[len] = '\0';

            Token tok = (Token){.len = len, .type = TOK_IDENT};
            memcpy(tok.ptr, identifier, len + 1);
            return tok;
        }
    }
}
