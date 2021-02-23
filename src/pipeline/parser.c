#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "headers/pipeline/parser.h"

#define ALLOCATE_SIMPLE_NODE(operator) (allocateNode(nb_operands[operator]))

typedef enum Precedence {
        PREC_NONE = 1,
        PREC_OR,
        PREC_AND,
        PREC_COMPARISON,
        PREC_AFFECT,
        PREC_ADD,
        PREC_MUL,
        PREC_UNARY,
        PREC_CALL,
} Precedence;

typedef Node* (*StatementHandler)(parser_info *const state);
typedef Node* (*UnaryParseFn)(parser_info *const state);
typedef struct BinaryParseRule {
        Node* (*parse_fn)(parser_info *const state, Node *const root);
        Precedence precedence;
} BinaryParseRule;

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

        [OP_CALL] = UINTPTR_MAX,

        [OP_BLOCK] = UINTPTR_MAX,
        [OP_IFELSE] = 3, // predicate, if_stmt, else_stmt
}; // set to UINTPTR_MAX for a variable number of operands

static inline void refresh(parser_info *const prsinfo) {
        // call to guarantee the last produced token is not stale
        if (prsinfo->stale) {
                prsinfo->last_produced=lex(&(prsinfo->lxinfo));
                prsinfo->stale = 0;
        }
}
static inline Token consume(parser_info *const prsinfo) {
        refresh(prsinfo);
        prsinfo->stale = 1;
        return prsinfo->last_produced;
}
static inline TokenType getTtype(parser_info *const prsinfo) {
        refresh(prsinfo);
        return prsinfo->last_produced.type;
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
        return (parser_info) {.lxinfo=mk_lexer_info(file), .stale=1};
}
inline void del_parser_info(parser_info prsinfo) {
        del_lexer_info(prsinfo.lxinfo);
}

// --------------------- prefix parse functions --------------------------------

static Node* prefixParseError(parser_info *const state) {
        const Token tk = state->last_produced;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.line, tk.column, tk.length, tk.source);
        return NULL;
}
static Node* unary_plus(parser_info *const state) {
        const Token operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_PLUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_PLUS};
        new->operands[0] = operand;
        return new;
}
static Node* unary_minus(parser_info *const state) {
        const Token operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_MINUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_MINUS};
        new->operands[0] = operand;
        return new;
}
static Node* integer(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INT);
        *new = (Node) {.token=consume(state), .operator=OP_INT};
        return new;
}
static Node* boolean(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_BOOL);
        *new = (Node) {.token=consume(state), .operator=OP_BOOL};
        return new;
}
static Node* fpval(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_FLOAT);
        *new = (Node) {.token=consume(state), .operator=OP_FLOAT};
        return new;
}
static Node* string(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_STR);
        *new = (Node) {.token=consume(state), .operator=OP_STR};
        return new;
}
static Node* grouping(parser_info *const state) {
        consume(state);
        Node* operand = parseExpression(state, PREC_NONE);
        if (operand == NULL) return NULL;

        if (getTtype(state) == TOKEN_PCLOSE) consume(state);
        else return infixParseError(state, operand);

        return operand;
}
static Node* invert(parser_info *const state) {
        const Token operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* none(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_NONE);
        *new = (Node) {.token=consume(state), .operator=OP_NONE};
        return new;
}
static Node* identifier(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_VARIABLE);
        *new = (Node) {.token=consume(state), .operator=OP_VARIABLE};
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
        const Token operator = consume(state);
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
        const Token operator = consume(state);
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
        const Token operator = consume(state);
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
        const Token operator = consume(state);
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
        const Token operator = consume(state);
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
        const Token operator = consume(state);
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
        const Token operator = consume(state);
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
        const Token operator = consume(state);
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
        refresh(state);
        // not how we DON'T consume the token; `le` will do it
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
        refresh(state);
        // not how we DON'T consume the token; `lt` will do it
        const Token operator = state->last_produced;
        Node* operand = lt(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0] = operand;
        return new;
}
static Node* eq(parser_info *const state, Node *const root) {
        const Token operator = consume(state);
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
        refresh(state);
        // not how we DON'T consume the token; `eq` will do it
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

        const Token operator = consume(state);
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
static Node* call(parser_info *const state, Node *const root) {
        // [count, fnode, argnode...]
        uintptr_t count = 1; // function node
        Node* new = allocateNode(count + 1);

        new->token = consume(state);
        new->operator = OP_CALL;
        new->operands[1] = root;

        while (getTtype(state) != TOKEN_PCLOSE) {
                Node* arg = parseExpression(state, PREC_NONE);
                if (arg == NULL) {
                        new->operands[0] = (void*) count;
                        freeNode(new);
                        return NULL;
                }
                new = reallocateNode(new, ++count+1);
                new->operands[count] = arg;

                if (getTtype(state) == TOKEN_COMMA) consume(state);
                else if (getTtype(state) != TOKEN_PCLOSE) return infixParseError(state, new);
        }
        consume(state);
        new->operands[0] = (void*) count;
        return new;

}

// ------------------ end parse functions --------------------------------------

static Node* parseExpression(parser_info *const state, const Precedence precedence) {
        static const UnaryParseFn prefixRules[TOKEN_EOF+1] = {
                [TOKEN_PLUS] = unary_plus,
                [TOKEN_MINUS] = unary_minus,
                [TOKEN_POPEN] = grouping,
                [TOKEN_NOT] = invert,
                [TOKEN_IDENTIFIER] = identifier,
                [TOKEN_INT] = integer,
                [TOKEN_FLOAT] = fpval,
                [TOKEN_STR] = string,
                [TOKEN_TRUE] = boolean,
                [TOKEN_FALSE] = boolean,
                [TOKEN_NONE] = none,
        };
        static const BinaryParseRule infixRules[TOKEN_EOF+1] = {
                [TOKEN_PLUS] = {.parse_fn=binary_plus, .precedence=PREC_ADD},
                [TOKEN_MINUS] = {.parse_fn=binary_minus, .precedence=PREC_ADD},
                [TOKEN_STAR] = {.parse_fn=binary_star, .precedence=PREC_MUL},
                [TOKEN_SLASH] = {.parse_fn=binary_slash, .precedence=PREC_MUL},
                [TOKEN_AND] = {.parse_fn=binary_and, .precedence=PREC_AND},
                [TOKEN_OR] = {.parse_fn=binary_or, .precedence=PREC_OR},
                [TOKEN_EQ] = {.parse_fn=eq, .precedence=PREC_COMPARISON},
                [TOKEN_NE] = {.parse_fn=ne, .precedence=PREC_COMPARISON},
                [TOKEN_GE] = {.parse_fn=ge, .precedence=PREC_COMPARISON},
                [TOKEN_LE] = {.parse_fn=le, .precedence=PREC_COMPARISON},
                [TOKEN_GT] = {.parse_fn=gt, .precedence=PREC_COMPARISON},
                [TOKEN_LT] = {.parse_fn=lt, .precedence=PREC_COMPARISON},
                [TOKEN_POPEN] = {.parse_fn=call, .precedence=PREC_CALL},
                [TOKEN_EQUAL] = {.parse_fn=affect, .precedence=PREC_AFFECT},
        };

        LOG("Appel avec une précédence de %d", precedence);

        Node* root;

        {
                const UnaryParseFn prefrule = prefixRules[getTtype(state)];
                if (prefrule == NULL) return prefixParseError(state);
                else root = prefrule(state);
        }

        {
                BinaryParseRule rule;
                while (root != NULL
                        && precedence < (rule=infixRules[getTtype(state)]).precedence
                ) {
                        if (rule.parse_fn == NULL) return infixParseError(state, root); // should be unreachable?
                        root = rule.parse_fn(state, root);
        }
}

        return root;
}

// ------------------ begin statement handlers ---------------------------------

static Node* simple_statement(parser_info *const state) {
        Node* stmt = parseExpression(state, PREC_NONE);
        if (stmt == NULL) return NULL;
        if (getTtype(state) != TOKEN_SEMICOLON) {
                Token tk = state->last_produced;
                fprintf(stderr, "line %u, column %u, at \"%.*s\": expected ';'.\n", tk.line, tk.column, tk.length, tk.source);
                freeNode(stmt);
                stmt = NULL;
        } else {
                consume(state);
        }
        return stmt;
}

static Node* block_statement(parser_info *const state) {
        uintptr_t nb_children = 0;
        Node* stmt = allocateNode(nb_children + 1); // add one, for the length of the array
        stmt->token = consume(state);
        stmt->operator = OP_BLOCK;
        while (getTtype(state) != TOKEN_BCLOSE) {
                Node* substmt = parse_statement(state);
                if (substmt == NULL) {
                        stmt->operands[0] = (void*) nb_children;
                        freeNode(stmt);
                        return NULL;
                }
                stmt = reallocateNode(stmt, ++nb_children+1);
                stmt->operands[nb_children] = substmt;
        }
        stmt->operands[0] = (void*) nb_children;
        consume(state);
        return stmt;
}

static Node* ifelse_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_IFELSE);
        *new = (Node) {.token=state->last_produced, .operator=OP_IFELSE};
        consume(state);
        if ((new->operands[0] = parseExpression(state, PREC_NONE)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if ((new->operands[1] = parse_statement(state)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if (getTtype(state) == TOKEN_ELSE) {
                consume(state);
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

        if (getTtype(state) == TOKEN_EOF || getTtype(state) == TOKEN_ERROR) return NULL;

        StatementHandler handler = handlers[getTtype(state)];
        if (handler == NULL) return simple_statement(state);
        else return handler(state);
}

#undef ALLOCATE_SIMPLE_NODE
#undef getTtype
