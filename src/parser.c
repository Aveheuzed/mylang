#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>


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


static Node* allocateNode(const unsigned int nb_children) {
        return malloc(offsetof(Node, operands) + sizeof(Node*)*nb_children);
}
void freeNode(Node* node) {
        if (node == NULL) return;
        for (unsigned int i = 0; i<node->nb_operands; i++) freeNode(node->operands[i]);
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
        Node *const new = allocateNode(1);
        *new = (Node) {operator, OP_UNARY_PLUS, 1};
        new->operands[0] = operand;
        return new;
}
static Node* unary_minus(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = allocateNode(1);
        *new = (Node) {operator, OP_UNARY_MINUS, 1};
        new->operands[0] = operand;
        return new;
}
static Node* integer(Token **const tokens) {
        Node *const new = allocateNode(0);
        *new = (Node) {CONSUME(tokens), OP_INT, 0};
        return new;
}
static Node* boolean(Token **const tokens) {
        Node *const new = allocateNode(0);
        *new = (Node) {CONSUME(tokens), OP_BOOL, 0};
        return new;
}
static Node* fpval(Token **const tokens) {
        Node *const new = allocateNode(0);
        *new = (Node) {CONSUME(tokens), OP_FLOAT, 0};
        return new;
}
static Node* string(Token **const tokens) {
        Node *const new = allocateNode(0);
        *new = (Node) {CONSUME(tokens), OP_STR, 0};
        return new;
}
static Node* grouping(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_GROUPING);
        if (operand == NULL) return NULL;

        if (PEEK_TYPE(tokens) == TOKEN_PCLOSE) CONSUME(tokens);
        else return infixParseError(tokens, operand);

        Node *const new = allocateNode(1);
        *new = (Node) {operator, OP_GROUP, 1};
        new->operands[0] = operand;
        return new;
}
static Node* invert(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = allocateNode(1);
        *new = (Node) {operator, OP_INVERT, 1};
        new->operands[0] = operand;
        return new;
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
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_SUM, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* binary_minus(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_DIFFERENCE, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* binary_star(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_PRODUCT, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* binary_slash(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_DIVISION, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* binary_and(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_AND);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_AND, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* binary_or(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_OR);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_OR, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* lt(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_LT, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* le(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_LE, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* gt(Token **const tokens, Node* const root) {
        // > is implemented as !(<=)
        const Token operator = **tokens;
        Node* operand = le(tokens, root);
        if (operand == NULL) return NULL;

        Node *const new = allocateNode(1);
        *new = (Node) {operator, OP_INVERT, 1};
        new->operands[0] = operand;
        return new;
}
static Node* ge(Token **const tokens, Node* const root) {
        // >= is implemented as !(<)
        const Token operator = **tokens;
        Node* operand = lt(tokens, root);
        if (operand == NULL) return NULL;

        Node *const new = allocateNode(1);
        *new = (Node) {operator, OP_INVERT, 1};
        new->operands[0] = operand;
        return new;
}
static Node* eq(Token **const tokens, Node* const root) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = allocateNode(2);
        *new = (Node) {operator, OP_EQ, 2};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* ne(Token **const tokens, Node* const root) {
        // != is implemented as !(==)
        const Token operator = **tokens;
        Node* operand = eq(tokens, root);
        if (operand == NULL) return NULL;

        Node *const new = allocateNode(1);
        *new = (Node) {operator, OP_INVERT, 1};
        new->operands[0] = operand;
        return new;
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
