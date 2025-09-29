#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_TOKENS 64
#define MAX_TOKEN_LEN 128

typedef enum {
    TOKEN_WORD,       // Regular word (command or argument)
    TOKEN_PIPE,       // "|"
    TOKEN_REDIRECT_OUT, // ">"
    TOKEN_REDIRECT_IN,  // "<"
    TOKEN_END        // End of tokens
} TokenType;

typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LEN];
} Token;

typedef struct {
    const char *input;
    size_t pos;
} Tokenizer;


size_t tokenize(const char *input, Token *tokens, size_t max_tokens);
void next_token(Tokenizer *tokenizer, Token *token);



#endif
