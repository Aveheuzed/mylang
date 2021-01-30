#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include "parser.h"

#define CONSUME(tkptrptr) (*(*tkptrptr)++)
#define PEEK_TYPE(tkptrptr) ((*tkptrptr)->type)
#define ALLOCATE_SIMPLE_NODE(operator) (allocateNode(nb_operands[operator]))

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

static Node* parse(Token **const tokens, const Precedence precedence);
static Node* prefixParseError(Token **const tokens);
static Node* infixParseError(Token **const tokens, Node *const root);

static const uintptr_t nb_operands[LEN_OPERATORS] = {
        [OP_VARIABLE] = 0,
        [OP_INT] = 0,
        [OP_BOOL] = 0,
        [OP_FLOAT] = 0,
        [OP_STR] = 0,

        [OP_UNARY_PLUS] = 1,
        [OP_UNARY_MINUS] = 1,
        [OP_GROUP] = 1,
        [OP_INVERT] = 1,

        [OP_SUM] = 2,
        [OP_DIFFERENCE] = 2,
        [OP_PRODUCT] = 2,
        [OP_DIVISION] = 2,
        [OP_AFFECT] = 2,
        [OP_AND] = 2,
        [OP_OR] = 2,
        [OP_EQ] = 2,
        [OP_LT] = 2,
        [OP_LE] = 2,
}; // set to UINTPTR_MAX for a variable number of operands

static Node* allocateNode(const uintptr_t nb_children) {
        return malloc(offsetof(Node, operands) + sizeof(Node*)*nb_children);
}
void freeNode(Node* node) {
        if (node == NULL) return;

        uintptr_t nb = nb_operands[node->operator];
        uintptr_t index;
        if (nb == UINTPTR_MAX) index = 1, nb = (uintptr_t)node->operands[0];
        else index = 0;

        for (; index<nb; index++) freeNode(node->operands[index]);
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_PLUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_PLUS};
        new->operands[0] = operand;
        return new;
}
static Node* unary_minus(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_MINUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_MINUS};
        new->operands[0] = operand;
        return new;
}
static Node* integer(Token **const tokens) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INT);
        *new = (Node) {.token=CONSUME(tokens), .operator=OP_INT};
        return new;
}
static Node* boolean(Token **const tokens) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_BOOL);
        *new = (Node) {.token=CONSUME(tokens), .operator=OP_BOOL};
        return new;
}
static Node* fpval(Token **const tokens) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_FLOAT);
        *new = (Node) {.token=CONSUME(tokens), .operator=OP_FLOAT};
        return new;
}
static Node* string(Token **const tokens) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_STR);
        *new = (Node) {.token=CONSUME(tokens), .operator=OP_STR};
        return new;
}
static Node* grouping(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_GROUPING);
        if (operand == NULL) return NULL;

        if (PEEK_TYPE(tokens) == TOKEN_PCLOSE) CONSUME(tokens);
        else return infixParseError(tokens, operand);

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_GROUP);
        *new = (Node) {.token=operator, .operator=OP_GROUP};
        new->operands[0] = operand;
        return new;
}
static Node* invert(Token **const tokens) {
        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* identifier(Token **const tokens) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_VARIABLE);
        *new = (Node) {.token=CONSUME(tokens), .operator=OP_VARIABLE};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_SUM);
        *new = (Node) {.token=operator, .operator=OP_SUM};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_DIFFERENCE);
        *new = (Node) {.token=operator, .operator=OP_DIFFERENCE};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_PRODUCT);
        *new = (Node) {.token=operator, .operator=OP_PRODUCT};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_DIVISION);
        *new = (Node) {.token=operator, .operator=OP_DIVISION};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_AND);
        *new = (Node) {.token=operator, .operator=OP_AND};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_OR);
        *new = (Node) {.token=operator, .operator=OP_OR};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LT);
        *new = (Node) {.token=operator, .operator=OP_LT};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LE);
        *new = (Node) {.token=operator, .operator=OP_LE};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* gt(Token **const tokens, Node* const root) {
        // > is implemented as !(<=)
        const Token operator = **tokens;
        Node* operand = le(tokens, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* ge(Token **const tokens, Node* const root) {
        // >= is implemented as !(<)
        const Token operator = **tokens;
        Node* operand = lt(tokens, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
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
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_EQ);
        *new = (Node) {.token=operator, .operator=OP_EQ};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}
static Node* ne(Token **const tokens, Node* const root) {
        // != is implemented as !(==)
        const Token operator = **tokens;
        Node* operand = eq(tokens, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* affect(Token **const tokens, Node *const root) {
        if (root->operator != OP_VARIABLE) return infixParseError(tokens, root);

        const Token operator = CONSUME(tokens);
        Node* operand = parse(tokens, PREC_AFFECT-1);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_AFFECT);
        *new = (Node) {.token=operator, .operator=OP_AFFECT};
        new->operands[0] = root;
        new->operands[1] = operand;
        return new;
}

// ------------------ end parse functions --------------------------------------

static Node* parse(Token **const tokens, const Precedence precedence) {
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

                [TOKEN_EQUAL] = {prefixParseError, affect, PREC_AFFECT},
                [TOKEN_SEMICOLON] = {prefixParseError, infixParseError, PREC_SEMICOLON},
                [TOKEN_NOT] = {invert, infixParseError, PREC_UNARY},

                [TOKEN_IDENTIFIER] = {identifier, infixParseError, PREC_NONE},
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

#undef ALLOCATE_SIMPLE_NODE
#undef PEEK_TYPE
#undef CONSUME
