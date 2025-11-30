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

/* parsing state */
typedef struct rule Rule;
typedef struct Symbol{
    enum {TERM, NONTERM} sym_type;
    bool visited;
    bool productive;
    bool is_defined;
    Token token;
    Rule * productions;
    struct Symbol * next;
} Symbol;

typedef struct rule{
    int len;
    int value;
    Symbol * formulations[50]; // A wild guess, a grammar wouldn't probably reach 50 productions without pipe
    struct rule * next;
} Rule;

struct parse_info{
    Symbol * start_symbol;
    Symbol * terminals;
    Symbol * non_terminals;
    int terminal_counts;
    int non_terminal_counts;
    bool set_start;
}ParseInfo;

#define MAXPOOL 4098
Symbol SymbolPool[MAXPOOL];
int symbol_idx;
Rule RulePool[MAXPOOL];
int rule_idx;

//todo: make reentrant
static inline Symbol * symbol_alloc(){
    if (symbol_idx >= MAXPOOL){
        fprintf(stderr, "Error: Symbols too many\n");
        return NULL;
    }
    return &SymbolPool[symbol_idx++];
}

static inline Rule * rule_alloc(){
    if (rule_idx >= MAXPOOL){
        fprintf(stderr, "Error: Rule too many\n");
        return NULL;
    }
    return &RulePool[rule_idx++];
}

Map * SymbolTable;

struct parse_token{
    Token cur;
    Token peek;
} ParseToken;

#define peek_token (ParseToken.peek)
#define cur_token (ParseToken.cur)
#define next_token(state) do {ParseToken.cur = ParseToken.peek, ParseToken.peek = lex(state);} while (0)
#define init_token(state) do {ParseToken.cur = lex(state), ParseToken.peek = lex(state);} while (0)

static inline Symbol * make_terminal(Token tok, int line_no){
    Symbol * sym = NULL;
    if (map_get(SymbolTable, tok.ptr, cur_token.len, 
                (void *) &sym) == -1) {
        sym = symbol_alloc();
        if (sym == NULL){
            return NULL;
        }
        sym->is_defined = false;
        sym->token = tok;
        sym->sym_type = TERM;
        sym->productive = true;
        sym->visited = false;
        sym->productions = NULL;
        sym->next = ParseInfo.terminals;
        ParseInfo.terminals = sym;
        ParseInfo.terminal_counts++;
    } 
    return sym;
}

static inline Symbol * make_non_terminal(Token tok, int line_no){
    Symbol * sym = NULL;
    if (map_get(SymbolTable, tok.ptr, tok.len, (void **) &sym) == -1){
        sym = symbol_alloc();
        if (sym == NULL){
            return NULL;
        }
        sym->is_defined = false;
        sym->productions = NULL;
        sym->productive = false;
        sym->sym_type = NONTERM;
        sym->visited = false;
        sym->token = cur_token;
        sym->next = ParseInfo.non_terminals;
        ParseInfo.non_terminals = sym;
        ParseInfo.non_terminal_counts++;
    }
    if (sym->sym_type == TERM){
        fprintf(stderr, "Error: Can't redefined previous terminal as terminal in line %d\n", line_no);
        return NULL;
    }
    return sym;
}


int parse(Reader * state){
    init_token(state);
    bool err_state = false;
    while (cur_token.type != TOK_EOF && cur_token.type !=TOK_ERROR && !err_state){
        switch((int)cur_token.type){
            case TOK_IDENT:
               if(parse_formulation(state) != 0) return -1;
                next_token(state);
                break;
            case TOK_TOKEN:
                if (parse_token(state) != 0) return -1;
                next_token(state);
                break;
            case TOK_START:
                if (parse_start(state) != 0) return -1;
                next_token(state);
                break;
        }
    }
    return 0;
}
