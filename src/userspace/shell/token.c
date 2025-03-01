#include <token.h>


// Tokenize input command string
size_t tokenize(const char *input, Token *tokens, size_t max_tokens) {
    Tokenizer tokenizer = { .input = input, .pos = 0};
    size_t token_count = 0;

    while (token_count < max_tokens) {
        Token token;
        next_token(&tokenizer, &token);
        tokens[token_count++] = token;
        if (token.type == TOKEN_END) break;
    }

    return token_count - 1; // Exclude TOKEN_END from count
}


int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

int is_special(char c) {
    return c == '|' || c == '>' || c == '<';
}

void next_token(Tokenizer *tokenizer, Token *token) {
    // Skip whitespace
    while (is_whitespace(tokenizer->input[tokenizer->pos])) {
        tokenizer->pos++;
    }

    char c = tokenizer->input[tokenizer->pos];

    // End of input
    if (c == '\0') {
        token->type = TOKEN_END;
        token->value[0] = '\0';
        return;
    }

    // Handle special characters
    if (is_special(c)) {
        token->type = (c == '|') ? TOKEN_PIPE :
                      (c == '>') ? TOKEN_REDIRECT_OUT :
                      TOKEN_REDIRECT_IN;
        token->value[0] = c;
        token->value[1] = '\0';
        tokenizer->pos++;
        return;
    }

    // Handle words (commands, arguments, filenames)
    size_t i = 0;
    while (tokenizer->input[tokenizer->pos] && !is_whitespace(tokenizer->input[tokenizer->pos]) &&
           !is_special(tokenizer->input[tokenizer->pos])) {
        if (i < MAX_TOKEN_LEN - 1) {
            token->value[i++] = tokenizer->input[tokenizer->pos];
        }
        tokenizer->pos++;
    }
    token->value[i] = '\0';
    token->type = TOKEN_WORD;
}






















