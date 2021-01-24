#include <stdlib.h>
#include <stdio.h>

#include "parser.h"

#define CONSUME(tkptrptr) (*(*tkptrptr)++)
#define PEEK_TYPE(tkptrptr) ((*tkptrptr)->type)

typedef Node* (*UnaryParseFn)(Token **const tokens);
typedef Node* (*BinaryParseFn)(Token **const tokens, Node* const root);

typedef enum Precedence {
        PREC_SEMICOLON,
        PREC_NONE,
        PREC_GROUPING,
        PREC_OR,
        PREC_AND,
        PREC_COMPARISON,
        PREC_AFFECT,
        PREC_ADD,
        PREC_MUL,
        PREC_UNARY,
} Precedence;

Node* parse(Token **const tokens, const Precedence precedence);
static Node* prefixParseError(Token **const tokens);
static Node* infixParseError(Token **const tokens, Node *const root);


static Node* allocateNode(Node *const root, Node *const operand, Token token, Operator operator) {
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

static Node* prefixParseError(Token **const tokens) {
        const Token tk = **tokens;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        return NULL;
}
static Node* unary_plus(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_UNARY);
        if (operand == NULL) return NULL;
        else return allocateNode(operand, NULL, operator, OP_UNARY_PLUS);
}
static Node* unary_minus(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_UNARY);
        if (operand == NULL) return NULL;
        else return allocateNode(operand, NULL, operator, OP_UNARY_MINUS);
}
static Node* integer(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_INT);
}
static Node* boolean(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_BOOL);
}
static Node* fpval(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_FLOAT);
}
static Node* string(Token **const tokens) {
        return allocateNode(NULL, NULL, CONSUME(tokens), OP_STR);
}
static Node* grouping(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_GROUPING);
        if (operand == NULL) return NULL;

        Node* node = allocateNode(operand, NULL, operator, OP_GROUP);

        if (PEEK_TYPE(tokens) == TOKEN_PCLOSE) CONSUME(tokens);
        else return infixParseError(tokens, node);

        return node;
}
static Node* invert(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_UNARY);
        if (operand == NULL) return NULL;
        else return allocateNode(operand, NULL, operator, OP_INVERT);
}

// --------------------- infix parse functions ---------------------------------

static Node* infixParseError(Token **const tokens, Node *const root) {
        const Token tk = **tokens;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        freeNode(root);
        return NULL;
}
static Node* binary_plus(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_SUM);
}
static Node* binary_minus(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_DIFFERENCE);
}
static Node* binary_star(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_PRODUCT);
}
static Node* binary_slash(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_DIVISION);
}
static Node* binary_and(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_AND);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_AND);
}
static Node* binary_or(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_OR);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_OR);
}
static Node* lt(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_LT);

}
static Node* le(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_LE);

}
static Node* gt(Token **const tokens, Node* const root) {
        // > is implemented as !(<=)
        const Token operator = **tokens;
        Node* operand = le(tokens, root);
        if (operand == NULL) return NULL;
        else return allocateNode(operand, NULL, operator, OP_INVERT);

}
static Node* ge(Token **const tokens, Node* const root) {
        // >= is implemented as !(<)
        const Token operator = **tokens;
        Node* operand = lt(tokens, root);
        if (operand == NULL) return NULL;
        else return allocateNode(operand, NULL, operator, OP_INVERT);

}
static Node* eq(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        else return allocateNode(root, operand, operator, OP_EQ);
}
static Node* ne(Token **const tokens, Node* const root) {
        // != is implemented as !(==)
        const Token operator = **tokens;
        Node* operand = eq(tokens, root);
        if (operand == NULL) return NULL;
        else return allocateNode(operand, NULL, operator, OP_INVERT);
}

// ------------------ end parse functions --------------------------------------

Node* parse(Token **const tokens, const Precedence precedence) {
        static const struct {UnaryParseFn prefix; BinaryParseFn infix; Precedence precedence;} rules[] = {
                [TOKEN_PLUS] = {unary_plus, binary_plus, PREC_ADD},
                [TOKEN_MINUS] = {unary_minus, binary_minus, PREC_ADD},
                [TOKEN_STAR] = {prefixParseError, binary_star, PREC_MUL},
                [TOKEN_SLASH] = {prefixParseError, binary_slash, PREC_MUL},
                [TOKEN_AND] {prefixParseError, binary_and, PREC_AND},
                [TOKEN_OR] = {prefixParseError, binary_or, PREC_OR},
                [TOKEN_EQ] = {prefixParseError, eq, PREC_COMPARISON},
                [TOKEN_NE] = {prefixParseError, ne, PREC_COMPARISON},
                [TOKEN_GE] = {prefixParseError, ge, PREC_COMPARISON},
                [TOKEN_LE] = {prefixParseError, le, PREC_COMPARISON},
                [TOKEN_GT] = {prefixParseError, gt, PREC_COMPARISON},
                [TOKEN_LT] = {prefixParseError, lt, PREC_COMPARISON},
                [TOKEN_POPEN] = {grouping, infixParseError, PREC_NONE},
                [TOKEN_PCLOSE] = {prefixParseError, infixParseError, PREC_NONE},

                [TOKEN_EQUAL] = {prefixParseError, infixParseError, PREC_AFFECT},
                [TOKEN_SEMICOLON] = {prefixParseError, infixParseError, PREC_SEMICOLON},
                [TOKEN_NOT] = {invert, infixParseError, PREC_UNARY},

                [TOKEN_IDENTIFIER] = {prefixParseError, infixParseError, PREC_NONE},
                [TOKEN_INT] = {integer, infixParseError, PREC_NONE},
                [TOKEN_FLOAT] = {fpval, infixParseError, PREC_NONE},
                [TOKEN_STR] = {string, infixParseError, PREC_NONE},
                [TOKEN_TRUE] = {boolean, infixParseError, PREC_NONE},
                [TOKEN_FALSE] = {boolean, infixParseError, PREC_NONE},

                [TOKEN_ERROR] = {prefixParseError, infixParseError, PREC_NONE},
                [TOKEN_EOF] = {prefixParseError, infixParseError, PREC_NONE},
        };

        Node* root;

        root = rules[PEEK_TYPE(tokens)].prefix(tokens);
        if (root == NULL) return root;

        while (precedence < rules[PEEK_TYPE(tokens)].precedence) {
                root = rules[PEEK_TYPE(tokens)].infix(tokens, root);
                if (root == NULL) break;
        }

        return root;
}

Node* parse_statement(Token **const tokens) {
        if (PEEK_TYPE(tokens) == TOKEN_EOF) return NULL;

        Node* stmt = parse(tokens, PREC_NONE);
        if (stmt == NULL) return NULL;
        if (PEEK_TYPE(tokens) == TOKEN_SEMICOLON) {
                CONSUME(tokens);
                return stmt;
        }
        else {
                Token tk = **tokens;
                fprintf(stderr, "line %u, column %u, at \"%.*s\": expected ';'.\n", tk.line, tk.column, tk.length, tk.source);
                freeNode(stmt);
                return NULL;
        }
}

#undef PEEK_TYPE
#undef CONSUME
