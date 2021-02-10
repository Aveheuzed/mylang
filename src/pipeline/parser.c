#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "headers/pipeline/parser.h"

#include "headers/utils/debug.h"

#define PEEK_TYPE(prsinfo) ((prsinfo)->last_produced.type)
#define ALLOCATE_SIMPLE_NODE(operator) (allocateNode(nb_operands[operator]))

typedef Node* (*UnaryParseFn)(parser_info *const state);
typedef Node* (*BinaryParseFn)(parser_info *const state, Node *const root);

typedef Node* (*StatementHandler)(parser_info *const state);

typedef enum Precedence {
        PREC_BAILOUT,
        PREC_NONE,
        PREC_OR,
        PREC_AND,
        PREC_COMPARISON,
        PREC_AFFECT,
        PREC_ADD,
        PREC_MUL,
        PREC_UNARY,
} Precedence;

static Node* parseExpression(parser_info *const state, const Precedence precedence);
static Node* prefixParseError(parser_info *const state);
static Node* infixParseError(parser_info *const state, Node *const root);

static const uintptr_t nb_operands[LEN_OPERATORS] = {
        [OP_VARIABLE] = 0,
        [OP_INT] = 0,
        [OP_BOOL] = 0,
        [OP_FLOAT] = 0,
        [OP_STR] = 0,
        [OP_NONE] = 0,

        [OP_UNARY_PLUS] = 1,
        [OP_UNARY_MINUS] = 1,
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

        [OP_BLOCK] = UINTPTR_MAX,
        [OP_IFELSE] = 3, // predicate, if_stmt, else_stmt
}; // set to UINTPTR_MAX for a variable number of operands

static inline void produce(parser_info *const prsinfo) {
        prsinfo->last_produced=lex(&(prsinfo->lxinfo));
}
static Node* allocateNode(const uintptr_t nb_children) {
        return malloc(offsetof(Node, operands) + sizeof(Node*)*nb_children);
}
static Node* reallocateNode(Node* nd, const uintptr_t nb_children) {
        return realloc(nd, offsetof(Node, operands) + sizeof(Node*)*nb_children);
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
inline parser_info mk_parser_info(FILE* file) {
        return (parser_info) {.lxinfo=mk_lexer_info(file)};
}

// --------------------- prefix parse functions --------------------------------

static Node* prefixParseError(parser_info *const state) {
        const Token tk = state->last_produced;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        return NULL;
}
static Node* unary_plus(parser_info *const state) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_PLUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_PLUS};
        new->operands[0] = operand;
        return new;
}
static Node* unary_minus(parser_info *const state) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_MINUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_MINUS};
        new->operands[0] = operand;
        return new;
}
static Node* integer(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INT);
        *new = (Node) {.token=state->last_produced, .operator=OP_INT};
        produce(state);
        return new;
}
static Node* boolean(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_BOOL);
        *new = (Node) {.token=state->last_produced, .operator=OP_BOOL};
        produce(state);
        return new;
}
static Node* fpval(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_FLOAT);
        *new = (Node) {.token=state->last_produced, .operator=OP_FLOAT};
        produce(state);
        return new;
}
static Node* string(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_STR);
        *new = (Node) {.token=state->last_produced, .operator=OP_STR};
        produce(state);
        return new;
}
static Node* grouping(parser_info *const state) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_NONE);
        if (operand == NULL) return NULL;

        if (PEEK_TYPE(state) == TOKEN_PCLOSE) produce(state);
        else return infixParseError(state, operand);

        return operand;
}
static Node* invert(parser_info *const state) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* none(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_NONE);
        *new = (Node) {.token=state->last_produced, .operator=OP_NONE};
        produce(state);
        return new;
}
static Node* identifier(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_VARIABLE);
        *new = (Node) {.token=state->last_produced, .operator=OP_VARIABLE};
        produce(state);
        return new;
}

// --------------------- infix parse functions ---------------------------------

static Node* infixParseError(parser_info *const state, Node *const root) {
        const Token tk = state->last_produced;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        freeNode(root);
        return NULL;
}
static Node* binary_plus(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_ADD);
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
static Node* binary_minus(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_ADD);
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
static Node* binary_star(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_MUL);
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
static Node* binary_slash(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_MUL);
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
static Node* binary_and(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_AND);
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
static Node* binary_or(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_OR);
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
static Node* lt(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
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
static Node* le(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
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
static Node* gt(parser_info *const state, Node *const root) {
        // > is implemented as !(<=)
        const Token operator = state->last_produced;
        Node* operand = le(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* ge(parser_info *const state, Node *const root) {
        // >= is implemented as !(<)
        const Token operator = state->last_produced;
        Node* operand = lt(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* eq(parser_info *const state, Node *const root) {
        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
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
static Node* ne(parser_info *const state, Node *const root) {
        // != is implemented as !(==)
        const Token operator = state->last_produced;
        Node* operand = eq(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* affect(parser_info *const state, Node *const root) {
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const Token operator = state->last_produced;
        produce(state);
        Node* operand = parseExpression(state, PREC_AFFECT-1);
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

static Node* parseExpression(parser_info *const state, const Precedence precedence) {
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
                [TOKEN_PCLOSE] = {prefixParseError, infixParseError, PREC_BAILOUT},
                [TOKEN_BOPEN] = {prefixParseError, infixParseError, PREC_NONE},
                [TOKEN_BCLOSE] = {prefixParseError, infixParseError, PREC_BAILOUT},

                [TOKEN_EQUAL] = {prefixParseError, affect, PREC_AFFECT},
                [TOKEN_SEMICOLON] = {prefixParseError, infixParseError, PREC_BAILOUT},
                [TOKEN_NOT] = {invert, infixParseError, PREC_UNARY},

                [TOKEN_IDENTIFIER] = {identifier, infixParseError, PREC_NONE},
                [TOKEN_INT] = {integer, infixParseError, PREC_NONE},
                [TOKEN_FLOAT] = {fpval, infixParseError, PREC_NONE},
                [TOKEN_STR] = {string, infixParseError, PREC_NONE},
                [TOKEN_TRUE] = {boolean, infixParseError, PREC_NONE},
                [TOKEN_FALSE] = {boolean, infixParseError, PREC_NONE},
                [TOKEN_NONE] = {none, infixParseError, PREC_NONE},

                [TOKEN_ERROR] = {prefixParseError, infixParseError, PREC_BAILOUT},
                [TOKEN_EOF] = {prefixParseError, infixParseError, PREC_BAILOUT},
        };

        Node* root;

        root = rules[PEEK_TYPE(state)].prefix(state);
        if (root == NULL) return root;

        while (precedence < rules[PEEK_TYPE(state)].precedence) {
                root = rules[PEEK_TYPE(state)].infix(state, root);
                if (root == NULL) break;
        }

        return root;
}

// ------------------ begin statement handlers ---------------------------------

static Node* simple_statement(parser_info *const state) {
        Node* stmt = parseExpression(state, PREC_NONE);
        if (stmt == NULL) return NULL;
        if (PEEK_TYPE(state) != TOKEN_SEMICOLON) {
                Token tk = state->last_produced;
                fprintf(stderr, "line %u, column %u, at \"%.*s\": expected ';'.\n", tk.line, tk.column, tk.length, tk.source);
                freeNode(stmt);
                stmt = NULL;
        }
        return stmt;
}

static Node* block_statement(parser_info *const state) {
        uintptr_t nb_children = 0;
        Node* stmt = allocateNode(nb_children + 1); // add one, for the length of the array
        stmt->token = state->last_produced;
        produce(state);
        stmt->operator = OP_BLOCK;
        while (PEEK_TYPE(state) != TOKEN_BCLOSE) {
                Node* substmt = parse_statement(state);
                if (substmt == NULL) {
                        freeNode(stmt);
                        return NULL;
                }
                stmt = reallocateNode(stmt, ++nb_children+1);
                stmt->operands[nb_children] = substmt;
        }
        stmt->operands[0] = (void*) nb_children;
        return stmt;
}

static Node* ifelse_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_IFELSE);
        *new = (Node) {.token=state->last_produced, .operator=OP_IFELSE};
        produce(state);
        if ((new->operands[0] = parseExpression(state, PREC_NONE)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if ((new->operands[1] = parse_statement(state)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if (PEEK_TYPE(state) == TOKEN_ELSE) {
                produce(state);
                if ((new->operands[2] = parse_statement(state)) == NULL) {
                        freeNode(new);
                        return NULL;
                }
        }
        else new->operands[2] = NULL;
        return new;
}

// ------------------ end statement handlers -----------------------------------

Node* parse_statement(parser_info *const state) {
        static const StatementHandler handlers[TOKEN_EOF] = {
                [TOKEN_BOPEN] = block_statement,
                [TOKEN_IF] = ifelse_statement,
        };

        LOG("Building a new statement");

        produce(state);

        if (PEEK_TYPE(state) == TOKEN_EOF || PEEK_TYPE(state) == TOKEN_ERROR) return NULL;

        StatementHandler handler = handlers[PEEK_TYPE(state)];
        if (handler == NULL) return simple_statement(state);
        else return handler(state);
}

#undef ALLOCATE_SIMPLE_NODE
#undef PEEK_TYPE
