#include <stdlib.h>
#include <stdio.h>

#include "parser.h"

#define CONSUME(tkptrptr) (*(*tkptrptr)++)

Node* allocateNode(Node *const root, Node *const operand, Token token, Operator operator) {
        Node *const node = malloc(sizeof(Node));
        *node = (Node){
                .operands = {root, operand},
                .token = token,
                .operator = operator,
        };
        return node;
}
void freeNode(Node* node) {
        if (node == NULL) return;
        freeNode(node->operands[0]);
        freeNode(node->operands[1]);
        free(node);
}

// --------------------- prefix parse functions --------------------------------

Node* prefixParseError(Token ** tokens) {
        const Token tk = **tokens;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        return NULL;
}
Node* unary_plusminus(Token ** tokens) {
        const Token operator = CONSUME(tokens);
        return allocateNode(parse(tokens, PREC_ADD), NULL, operator, OP_MATH_UNARY);
}
Node* literal(Token ** tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_LITERAL);
}
Node* grouping(Token ** tokens) {
        const Token operator = CONSUME(tokens);
        Node* node = parse(tokens, PREC_GROUPING);
        if ((*tokens)->type == TOKEN_PCLOSE) CONSUME(tokens);
        else prefixParseError(tokens);
        return node;
}

// --------------------- infix parse functions ---------------------------------

Node* infixParseError(Token ** tokens, Node *const root) {
        const Token tk = **tokens;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        return root;
}
Node* binary_math(Token ** tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Precedence prec;
        switch (operator.type) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                        prec = PREC_ADD; break;
                case TOKEN_STAR:
                case TOKEN_SLASH:
                        prec = PREC_MUL; break;
        }
        return allocateNode(root, parse(tokens, prec), operator, OP_MATH_BINARY);
}
Node* semicolon(Token ** tokens, Node *const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = NULL;
        if ((*tokens)->type != TOKEN_EOF) operand = parse(tokens, PREC_SEMICOLON);
        return allocateNode(root, operand, operator, OP_SEMICOLON);
}

// ------------------ end parse functions --------------------------------------

const struct {UnaryParseFn prefix; BinaryParseFn infix; Precedence precedence;} rules[] = {
        [TOKEN_PLUS] = {unary_plusminus, binary_math, PREC_ADD},
        [TOKEN_MINUS] = {unary_plusminus, binary_math, PREC_ADD},
        [TOKEN_STAR] = {prefixParseError, binary_math, PREC_MUL},
        [TOKEN_SLASH] = {prefixParseError, binary_math, PREC_MUL},
        [TOKEN_POPEN] = {grouping, infixParseError, PREC_NONE},
        [TOKEN_PCLOSE] = {prefixParseError, infixParseError, PREC_NONE},

        [TOKEN_EQUAL] = {prefixParseError, infixParseError, PREC_AFFECT},
        [TOKEN_SEMICOLON] = {prefixParseError, semicolon, PREC_SEMICOLON},

        [TOKEN_IDENTIFIER] = {prefixParseError, infixParseError, PREC_NONE},
        [TOKEN_INT] = {literal, infixParseError, PREC_NONE},
        [TOKEN_FLOAT] = {literal, infixParseError, PREC_NONE},
        [TOKEN_STR] = {literal, infixParseError, PREC_NONE},

        [TOKEN_ERROR] = {prefixParseError, infixParseError, PREC_NONE},
        [TOKEN_EOF] = {prefixParseError, infixParseError, PREC_NONE},
};

Node* parse(Token ** tokens, const Precedence precedence) {
        Node* root;

        root = rules[(*tokens)->type].prefix(tokens);
        if (root == NULL) return root;

        while (precedence <= rules[(*tokens)->type].precedence) {
                if ((*tokens)->type == TOKEN_EOF) break;
                root = rules[(*tokens)->type].infix(tokens, root);
        }
        return root;
}

#undef CONSUME
