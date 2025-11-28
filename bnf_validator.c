#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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

/* Lame attempt at reentrancy */
typedef struct State{
    FILE * ptr;
    int line_no;
    int column;
} Reader;

#define MAX_IDENT 50

typedef struct {
    char ptr[MAX_IDENT + 1];
    int len;
    TokenType type;
} Token;

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
            return (Token){.type = TOK_PIPE};

        case '-':
            if ((c = getc(file->ptr)) != '>') {
                return (Token){.type = TOK_ERROR};
            }
            return (Token){.type = TOK_ARROW};

        case '%':
        {
            c = getc(file->ptr);
            if (c == 's') {
                char *word = "tart";
                for (int i = 0; i < 4; i++){
                    c = getc(file->ptr);
                    if (c != word[i]){
                        return (Token){.type = TOK_ERROR};
                    }
                }
                c = getc(file->ptr);
                if (!isspace(c)) {
                    return (Token){.type = TOK_ERROR};
                }
                return (Token){.type = TOK_START};
            }
            else if (c == 't') {
                char *word = "oken";
                for (int i = 0; i < 4; i++){
                    c = getc(file->ptr);
                    if (c != word[i]){
                        return (Token){.type = TOK_ERROR};
                    }
                }
                c = getc(file->ptr);
                if (!isspace(c)) {
                    return (Token){.type = TOK_ERROR};
                }
                return (Token){.type = TOK_TOKEN};
            }
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
