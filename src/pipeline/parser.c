#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "pipeline/parser.h"
#include "utils/object.h"
#include "utils/string.h"
#include "utils/function.h"
#include "utils/error.h"

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
        [OP_LITERAL_INT] = 1,
        [OP_LITERAL_FLOAT] = 1,
        [OP_LITERAL_TRUE] = 0,
        [OP_LITERAL_FALSE] = 0,
        [OP_LITERAL_NONE] = 0,
        [OP_LITERAL_STR] = 1,
        [OP_LITERAL_FUNCTION] = 1,

        [OP_UNARY_PLUS] = 1,
        [OP_UNARY_MINUS] = 1,
        [OP_INVERT] = 1,
        [OP_RETURN] = 1,

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
        [OP_IADD] = 2,
        [OP_ISUB] = 2,
        [OP_IMUL] = 2,
        [OP_IDIV] = 2,

        [OP_CALL] = UINTPTR_MAX,

        [OP_NOP] = 0,

        [OP_BLOCK] = UINTPTR_MAX,
        [OP_IFELSE] = 3, // predicate, if_stmt, else_stmt
        [OP_WHILE] = 2, // predicate, loop body
}; // set to UINTPTR_MAX for a variable number of operands

static inline void refresh(parser_info *const prsinfo) {
        // call to guarantee the last produced token is not stale
        if (prsinfo->stale) {
                prsinfo->last_produced = lex(&(prsinfo->lxinfo));
                prsinfo->stale = 0;
        }
}
static inline LocalizedToken consume(parser_info *const prsinfo) {
        refresh(prsinfo);
        prsinfo->stale = 1;
        return prsinfo->last_produced;
}
static inline TokenType getTtype(parser_info *const prsinfo) {
        refresh(prsinfo);
        return prsinfo->last_produced.tok.type;
}
static Node* allocateNode(const uintptr_t nb_children) {
        return malloc(offsetof(Node, operands) + sizeof(Node*)*nb_children);
}
static Node* reallocateNode(Node* nd, const uintptr_t nb_children) {
        return realloc(nd, offsetof(Node, operands) + sizeof(Node*)*nb_children);
}
void freeNode(Node* node) {
        if (node == NULL) return;

        if (node->operator > LAST_OP_LITERAL) {
                uintptr_t nb = nb_operands[node->operator];
                uintptr_t index;
                if (nb == UINTPTR_MAX) index = 1, nb = node->operands[0].len;
                else index = 0;

                for (; index<nb; index++) freeNode(node->operands[index].nd);
        }
        free(node);
}
inline parser_info mk_parser_info(FILE* file) {
        return (parser_info) {.lxinfo=mk_lexer_info(file), .stale=1, .func_def_depth=0};
}
inline void del_parser_info(parser_info prsinfo) {
        del_lexer_info(prsinfo.lxinfo);
}

// --------------------- prefix parse functions --------------------------------

static Node* prefixParseError(parser_info *const state) {
        Error(&(state->last_produced), "Syntax error: unexpected token at this place.\n");
        return NULL;
}
static Node* unary_plus(parser_info *const state) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_PLUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_PLUS};
        new->operands[0].nd = operand;
        return new;
}
static Node* unary_minus(parser_info *const state) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_MINUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_MINUS};
        new->operands[0].nd = operand;
        return new;
}
static Node* integer(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LITERAL_INT);
        *new = (Node) {.token=consume(state), .operator=OP_LITERAL_INT};
        new->operands[0].obj.intval = atoll(new->token.tok.source);
        return new;
}
static Node* boolean(parser_info *const state) {
        LocalizedToken tk = consume(state);
        if (tk.tok.type == TOKEN_TRUE) {
                Node *const new = ALLOCATE_SIMPLE_NODE(OP_LITERAL_TRUE);
                *new = (Node) {.token=tk, .operator=OP_LITERAL_TRUE};
                return new;
        }
        else {
                Node *const new = ALLOCATE_SIMPLE_NODE(OP_LITERAL_FALSE);
                *new = (Node) {.token=tk, .operator=OP_LITERAL_FALSE};
                return new;
        }
}
static Node* fpval(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LITERAL_FLOAT);
        *new = (Node) {.token=consume(state), .operator=OP_LITERAL_FLOAT};
        new->operands[0].obj.intval = atof(new->token.tok.source);
        return new;
}
static Node* string(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LITERAL_STR);
        *new = (Node) {.token=consume(state), .operator=OP_LITERAL_STR};
        new->operands[0].obj.strval = makeString(new->token.tok.source+1, new->token.tok.length-2);
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
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0].nd = operand;
        return new;
}
static Node* none(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LITERAL_NONE);
        *new = (Node) {.token=consume(state), .operator=OP_LITERAL_NONE};
        return new;
}
static Node* identifier(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_VARIABLE);
        *new = (Node) {.token=consume(state), .operator=OP_VARIABLE};
        return new;
}
static Node* function(parser_info *const state) {
        Node* fct = ALLOCATE_SIMPLE_NODE(OP_LITERAL_FUNCTION);

        fct->token = consume(state);
        fct->operator = OP_LITERAL_FUNCTION;

        if (getTtype(state) != TOKEN_POPEN) return infixParseError(state, fct);
        consume(state);

        ObjFunction* func = createFunction(0);
        func->arity = 0;

        while (getTtype(state) != TOKEN_PCLOSE) {
                if (getTtype(state) == TOKEN_IDENTIFIER) {
                        func = reallocFunction(func, func->arity+1);
                        func->arguments[func->arity++] = consume(state).tok.source;
                }

                if (getTtype(state) == TOKEN_COMMA) consume(state);
                else if (getTtype(state) != TOKEN_PCLOSE) {
                        free_function(func);
                        return infixParseError(state, fct);
                }
        }
        consume(state);

        state->func_def_depth++;
        if ((func->body = parse_statement(state)) == NULL) {
                freeNode(fct);
                free_function(func);
                return NULL;
        }
        state->func_def_depth--;
        fct->operands[0].obj.funval = func;
        return fct;
}

// --------------------- infix parse functions ---------------------------------

static Node* infixParseError(parser_info *const state, Node *const root) {
        Error(&(state->last_produced), "Syntax error: unexpected token at this place.\n");
        return NULL;
}
static Node* binary_plus(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_SUM);
        *new = (Node) {.token=operator, .operator=OP_SUM};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_minus(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_DIFFERENCE);
        *new = (Node) {.token=operator, .operator=OP_DIFFERENCE};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_star(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_PRODUCT);
        *new = (Node) {.token=operator, .operator=OP_PRODUCT};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_slash(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_DIVISION);
        *new = (Node) {.token=operator, .operator=OP_DIVISION};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_and(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_AND);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_AND);
        *new = (Node) {.token=operator, .operator=OP_AND};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_or(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_OR);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_OR);
        *new = (Node) {.token=operator, .operator=OP_OR};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* lt(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LT);
        *new = (Node) {.token=operator, .operator=OP_LT};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* le(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LE);
        *new = (Node) {.token=operator, .operator=OP_LE};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* gt(parser_info *const state, Node *const root) {
        // > is implemented as !(<=)
        refresh(state);
        // not how we DON'T consume the token; `le` will do it
        const LocalizedToken operator = state->last_produced;
        Node* operand = le(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0].nd = operand;
        return new;
}
static Node* ge(parser_info *const state, Node *const root) {
        // >= is implemented as !(<)
        refresh(state);
        // not how we DON'T consume the token; `lt` will do it
        const LocalizedToken operator = state->last_produced;
        Node* operand = lt(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0].nd = operand;
        return new;
}
static Node* eq(parser_info *const state, Node *const root) {
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_EQ);
        *new = (Node) {.token=operator, .operator=OP_EQ};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* ne(parser_info *const state, Node *const root) {
        // != is implemented as !(==)
        refresh(state);
        // not how we DON'T consume the token; `eq` will do it
        const LocalizedToken operator = state->last_produced;
        Node* operand = eq(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT};
        new->operands[0].nd = operand;
        return new;
}
static Node* affect(parser_info *const state, Node *const root) {
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_AFFECT-1);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_AFFECT);
        *new = (Node) {.token=operator, .operator=OP_AFFECT};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* call(parser_info *const state, Node *const root) {
        // [count, fnode, argnode...]
        uintptr_t count = 1; // function node
        Node* new = allocateNode(count + 1);

        new->token = consume(state);
        new->operator = OP_CALL;
        new->operands[1].nd = root;

        while (getTtype(state) != TOKEN_PCLOSE) {
                Node* arg = parseExpression(state, PREC_NONE);
                if (arg == NULL) {
                        new->operands[0].len = count;
                        freeNode(new);
                        return NULL;
                }
                new = reallocateNode(new, ++count+1);
                new->operands[count].nd = arg;

                if (getTtype(state) == TOKEN_COMMA) consume(state);
                else if (getTtype(state) != TOKEN_PCLOSE) return infixParseError(state, new);
        }
        consume(state);
        new->operands[0].len = count;
        return new;

}
static Node* iadd(parser_info *const state, Node *const root) {
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_IADD);
        *new = (Node) {.token=operator, .operator=OP_IADD};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* isub(parser_info *const state, Node *const root) {
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_ISUB);
        *new = (Node) {.token=operator, .operator=OP_ISUB};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* imul(parser_info *const state, Node *const root) {
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_IMUL);
        *new = (Node) {.token=operator, .operator=OP_IMUL};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* idiv(parser_info *const state, Node *const root) {
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_IDIV);
        *new = (Node) {.token=operator, .operator=OP_IDIV};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
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
                [TOKEN_FUNC] = function,
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
                [TOKEN_IADD] = {.parse_fn=iadd, .precedence=PREC_AFFECT},
                [TOKEN_ISUB] = {.parse_fn=isub, .precedence=PREC_AFFECT},
                [TOKEN_IMUL] = {.parse_fn=imul, .precedence=PREC_AFFECT},
                [TOKEN_IDIV] = {.parse_fn=idiv, .precedence=PREC_AFFECT},
        };

        LOG("Appel avec une précédence de %d", precedence);

        Node* root;

        const UnaryParseFn prefrule = prefixRules[getTtype(state)];
        if (prefrule == NULL) return prefixParseError(state);
        else root = prefrule(state);

        BinaryParseRule rule;
        while (root != NULL
                && precedence < (rule=infixRules[getTtype(state)]).precedence) {
                if (rule.parse_fn == NULL) return infixParseError(state, root);
                root = rule.parse_fn(state, root);
        }

        return root;
}

// ------------------ begin statement handlers ---------------------------------

static Node* simple_statement(parser_info *const state) {
        Node* stmt = parseExpression(state, PREC_NONE);
        if (stmt == NULL) return NULL;
        if (getTtype(state) != TOKEN_SEMICOLON) {
                Error(&(state->last_produced), "Syntax error: expected a `;`.\n");
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
                        stmt->operands[0].len = nb_children;
                        freeNode(stmt);
                        return NULL;
                }
                stmt = reallocateNode(stmt, ++nb_children+1);
                stmt->operands[nb_children].nd = substmt;
        }
        stmt->operands[0].len = nb_children;
        consume(state);
        return stmt;
}

static Node* ifelse_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_IFELSE);
        *new = (Node) {.token=state->last_produced, .operator=OP_IFELSE};
        consume(state);
        if ((new->operands[0].nd = parseExpression(state, PREC_NONE)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if ((new->operands[1].nd = parse_statement(state)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if (getTtype(state) == TOKEN_ELSE) {
                consume(state);
                if ((new->operands[2].nd = parse_statement(state)) == NULL) {
                        freeNode(new);
                        return NULL;
                }
        }
        else new->operands[2].nd = NULL;
        return new;
}

static Node* while_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_WHILE);
        *new = (Node) {.token=state->last_produced, .operator=OP_WHILE};
        consume(state);
        if ((new->operands[0].nd = parseExpression(state, PREC_NONE)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if ((new->operands[1].nd = parse_statement(state)) == NULL) {
                freeNode(new);
                return NULL;
        }
        return new;
}

static Node* empty_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_NOP);
        new->token = consume(state);
        new->operator = OP_NOP;
        return new;
}

static Node* return_statement(parser_info *const state) {
        LOG("in");
        if (!state->func_def_depth) return prefixParseError(state);

        Node* stmt = ALLOCATE_SIMPLE_NODE(OP_RETURN);
        *stmt = (Node) {.token=state->last_produced, .operator=OP_RETURN};
        consume(state);

        Node* arg = simple_statement(state);
        if (arg == NULL) {
                freeNode(stmt);
                return NULL;
        }
        else {
                stmt->operands[0].nd = arg;
                LOG("out");
                return stmt;
        }

}

// ------------------ end statement handlers -----------------------------------

Node* parse_statement(parser_info *const state) {
        static const StatementHandler handlers[TOKEN_EOF] = {
                [TOKEN_BOPEN] = block_statement,
                [TOKEN_IF] = ifelse_statement,
                [TOKEN_WHILE] = while_statement,
                [TOKEN_SEMICOLON] = empty_statement,
                [TOKEN_RETURN] = return_statement,
        };

        LOG("Building a new statement");

        if (getTtype(state) == TOKEN_EOF || getTtype(state) == TOKEN_ERROR) return NULL;

        StatementHandler handler = handlers[getTtype(state)];
        if (handler == NULL) return simple_statement(state);
        else return handler(state);
}

#undef ALLOCATE_SIMPLE_NODE
