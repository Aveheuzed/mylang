#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "lexer.h"
#include "compiler/parser.h"
#include "compiler/node.h"

#include "compiler/runtime_types.h"
#include "error.h"
#include "compiler/builtins.h"

#define ALLOCATE_SIMPLE_NODE(operator) (allocateNode(nb_operands[operator]))

typedef enum Precedence {
        PREC_NONE = 1,
        PREC_OR,
        PREC_AND,
        PREC_COMPARISON,
        PREC_ADD,
        PREC_MUL,
        PREC_UNARY,
        PREC_CALL,
} Precedence;

typedef struct ResolverRecord {
        size_t allocated;
        size_t len;
        struct {
                char const* key;
                size_t arity; // SIZE_MAX for non-functions
                RuntimeType type; // return type for functions
        } record[];
} ResolverRecord;

typedef Node* (*StatementHandler)(parser_info *const state);
typedef Node* (*UnaryParseFn)(parser_info *const state);
typedef Node* (*BinaryParseFunction)(parser_info *const state, Node *const root);
typedef struct BinaryParseRule {
        BinaryParseFunction parse_fn;
        Precedence precedence;
} BinaryParseRule;

static Node* parseExpression(parser_info *const state, const Precedence precedence);
static Node* prefixParseError(parser_info *const state);
static Node* infixParseError(parser_info *const state, Node *const root);

static const uintptr_t nb_operands[LEN_OPERATORS] = {
        [OP_VARIABLE] = 0,
        [OP_INT] = 0,
        [OP_STR] = 0,

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
        [OP_NE] = 2,
        [OP_LT] = 2,
        [OP_LE] = 2,
        [OP_IADD] = 2,
        [OP_ISUB] = 2,
        [OP_IMUL] = 2,
        [OP_IDIV] = 2,

        [OP_DECLARE] = 2, // variable, initializer (type as operator)

        [OP_IFELSE] = 3, // condition, consequence, else
        [OP_DOWHILE] = 2, // body, condition
        [OP_WHILE] = 2, // condition, body

        [OP_CALL] = UINTPTR_MAX,

        [OP_NOP] = 0,

        [OP_BLOCK] = UINTPTR_MAX,
}; // set to UINTPTR_MAX for a variable number of operands


// --------------------- end of declarations -----------------------------------

static ResolverRecord* mk_record(void) {
        ResolverRecord *const record = malloc(offsetof(ResolverRecord, record)
                + 16*sizeof((ResolverRecord){0}.record[0]));
        record->allocated = 16;
        record->len = 0;
        return record;
}
static void free_record(ResolverRecord* record) {
        free(record);
}
static ResolverRecord* grow_record(ResolverRecord* record, const size_t new_size) {
        record = realloc(record, offsetof(ResolverRecord, record)
                + new_size*sizeof((ResolverRecord){0}.record[0]));
        record->allocated = new_size;
        return record;
}

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

        uintptr_t nb = nb_operands[node->operator];
        uintptr_t index;
        if (nb == UINTPTR_MAX) index = 1, nb = node->operands[0].len;
        else index = 0;

        for (; index<nb; index++) freeNode(node->operands[index].nd);
        free(node);
}

ResolverRecord* record_variable(ResolverRecord* record, char const* key, size_t arity, RuntimeType type) {
        LOG("Recording %s as %d", key, type);
        if (record->len >= record->allocated) {
                record = grow_record(record, record->allocated*2);
        }

        record->record[record->len++] = (typeof(record->record[0])) {.key=key, .type=type, .arity=arity};

        return record;
}
static RuntimeType resolve_variable(const ResolverRecord* record, const char* key) {
        for (size_t i=record->len; i-->0; ) if (record->record[i].key==key) {
                return record->record[i].type;
        }
        return TYPEERROR;
}
static RuntimeType resolve_function(const ResolverRecord* record, const char* key, const size_t expected_arity) {
        for (size_t i=record->len; i-->0; ) if (record->record[i].key==key) {
                if (record->record[i].arity != expected_arity) return TYPEERROR;
                else return record->record[i].type;
        }
        return TYPEERROR;
}

void mk_parser_info(parser_info *const prsinfo) {
        prsinfo->stale = 1;
        ResolverRecord* record = mk_record();

        for (size_t i = 0; i < nb_builtins; i++) {
                record = record_variable(record, builtins[i].name,  builtins[i].arity, builtins[i].returnType);
        }

        prsinfo->resolv = record;
}
void del_parser_info(parser_info *const prsinfo) {
        free_record(prsinfo->resolv);
}

static inline int is_affectation_target(const Node* node) {
        return node->token.tok.type == TOKEN_IDENTIFIER;
}

static inline Node* semicolon_or_error(parser_info *const state, Node *const stmt) {
        if (getTtype(state) != TOKEN_SEMICOLON) {
                LocalizedToken tk = state->last_produced;
                fprintf(stderr, "line %u, column %u, at \"%.*s\": expected ';'.\n", tk.pos.line, tk.pos.column, tk.tok.length, tk.tok.source);
                freeNode(stmt);
                return NULL;
        } else {
                consume(state);
                return stmt;
        }
}

// --------------------- prefix parse functions --------------------------------

static Node* prefixParseError(parser_info *const state) {
        const LocalizedToken tk = state->last_produced;
        fprintf(stderr, "Parse error at line %u, column %u, at \"%.*s\"\n", tk.pos.line, tk.pos.column, tk.tok.length, tk.tok.source);
        return NULL;
}
static Node* unary_plus(parser_info *const state) {
        static const RuntimeType types[LEN_TYPES] = {
                [TYPE_INT]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_PLUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_PLUS, .type=types[operand->type]};
        new->operands[0].nd = operand;
        return new;
}
static Node* unary_minus(parser_info *const state) {
        static const RuntimeType types[LEN_TYPES] = {
                [TYPE_INT]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_UNARY_MINUS);
        *new = (Node) {.token=operator, .operator=OP_UNARY_MINUS, .type=types[operand->type]};
        new->operands[0].nd = operand;
        return new;
}
static Node* integer(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INT);
        *new = (Node) {.token=consume(state), .operator=OP_INT, .type=TYPE_INT};
        return new;
}
static Node* string(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_STR);
        *new = (Node) {.token=consume(state), .operator=OP_STR, .type=TYPE_STR};
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
        static const RuntimeType types[LEN_TYPES] = {
                [TYPE_INT]=TYPE_INT,
                [TYPE_STR]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_UNARY);
        if (operand == NULL) return NULL;
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT, .type=types[operand->type]};
        new->operands[0].nd = operand;
        return new;
}
static Node* identifier(parser_info *const state) {
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_VARIABLE);
        *new = (Node) {.token=consume(state), .operator=OP_VARIABLE};
        if ((new->type = resolve_variable(state->resolv, new->token.tok.source)) == TYPEERROR) {
                Error(&(new->token), "Can't resolve identifier %s.\n", new->token.tok.source);
                freeNode(new);
                return NULL;
        }
        return new;
}

// --------------------- infix parse functions ---------------------------------

static Node* infixParseError(parser_info *const state, Node *const root) {
        const LocalizedToken tk = state->last_produced;
        Error(&tk, "ParseError.\n");
        freeNode(root);
        return NULL;
}
static Node* binary_plus(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_SUM);
        *new = (Node) {.token=operator, .operator=OP_SUM, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_minus(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_ADD);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_DIFFERENCE);
        *new = (Node) {.token=operator, .operator=OP_DIFFERENCE, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_star(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_INT][TYPE_STR]=TYPE_INT,
                [TYPE_STR][TYPE_INT]=TYPE_INT,

        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_PRODUCT);
        *new = (Node) {.token=operator, .operator=OP_PRODUCT, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_slash(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_MUL);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_DIVISION);
        *new = (Node) {.token=operator, .operator=OP_DIVISION, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_and(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_STR,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_AND);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_AND);
        *new = (Node) {.token=operator, .operator=OP_AND, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* binary_or(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_STR,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_OR);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_OR);
        *new = (Node) {.token=operator, .operator=OP_OR, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* lt(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LT);
        *new = (Node) {.token=operator, .operator=OP_LT, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* le(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_LE);
        *new = (Node) {.token=operator, .operator=OP_LE, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* gt(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_INT,
        };
        // > is implemented as !(<=)
        refresh(state);
        // not how we DON'T consume the token; `le` will do it
        const LocalizedToken operator = state->last_produced;
        Node* operand = le(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT, .type=types[root->type][operand->type]};
        new->operands[0].nd = operand;
        return new;
}
static Node* ge(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_INT,
        };
        // >= is implemented as !(<)
        refresh(state);
        // not how we DON'T consume the token; `lt` will do it
        const LocalizedToken operator = state->last_produced;
        Node* operand = lt(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT, .type=types[root->type][operand->type]};
        new->operands[0].nd = operand;
        return new;
}
static Node* ne(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_INT,
        };
        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_COMPARISON);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_NE);
        *new = (Node) {.token=operator, .operator=OP_NE, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* eq(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_INT,
        };
        // == is implemented as !(!=)
        refresh(state);
        // not how we DON'T consume the token; `eq` will do it
        const LocalizedToken operator = state->last_produced;
        Node* operand = ne(state, root);
        if (operand == NULL) return NULL;

        Node *const new = ALLOCATE_SIMPLE_NODE(OP_INVERT);
        *new = (Node) {.token=operator, .operator=OP_INVERT, .type=types[root->type][operand->type]};
        new->operands[0].nd = operand;
        return new;
}
static Node* affect(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_STR,
        };
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_NONE);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_AFFECT);
        *new = (Node) {.token=operator, .operator=OP_AFFECT, .type=types[root->type][operand->type]};
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
        if ((new->type = resolve_function(state->resolv, new->operands[1].nd->token.tok.source, count-1)) == TYPEERROR) {
                Error(&(new->token), "TypeError : %s is not callable.\n", new->operands[1].nd->token.tok.source);
                freeNode(new);
                return NULL;
        }
        return new;

}
static Node* iadd(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_STR]=TYPE_STR,
        };
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_NONE);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_IADD);
        *new = (Node) {.token=operator, .operator=OP_IADD, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* isub(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
        };
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_NONE);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_ISUB);
        *new = (Node) {.token=operator, .operator=OP_ISUB, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* imul(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
                [TYPE_STR][TYPE_INT]=TYPE_STR,
                [TYPE_INT][TYPE_STR]=TYPE_STR,
        };
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_NONE);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_IMUL);
        *new = (Node) {.token=operator, .operator=OP_IMUL, .type=types[root->type][operand->type]};
        new->operands[0].nd = root;
        new->operands[1].nd = operand;
        return new;
}
static Node* idiv(parser_info *const state, Node *const root) {
        static const RuntimeType types[LEN_TYPES][LEN_TYPES] = {
                [TYPE_INT][TYPE_INT]=TYPE_INT,
        };
        if (root->operator != OP_VARIABLE) return infixParseError(state, root);

        const LocalizedToken operator = consume(state);
        Node* operand = parseExpression(state, PREC_NONE);
        if (operand == NULL) {
                freeNode(root);
                return NULL;
        }
        Node *const new = ALLOCATE_SIMPLE_NODE(OP_IDIV);
        *new = (Node) {.token=operator, .operator=OP_IDIV, .type=types[root->type][operand->type]};
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
                [TOKEN_STR] = string,
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
        };

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
        static const BinaryParseFunction rules[TOKEN_EOF+1] = {
                [TOKEN_EQUAL] = affect,
                [TOKEN_IADD] = iadd,
                [TOKEN_ISUB] = isub,
                [TOKEN_IMUL] = imul,
                [TOKEN_IDIV] = idiv,
        };

        Node* stmt = parseExpression(state, PREC_NONE);
        if (stmt == NULL) return NULL;
        if (is_affectation_target(stmt) && rules[getTtype(state)] != NULL) {
                stmt = rules[getTtype(state)](state, stmt);
        }

        return semicolon_or_error(state, stmt);
}

static Node* declare_statement(parser_info *const state) {
        static const RuntimeType types[TOKEN_EOF] = {
                [TOKEN_KW_INT]=TYPE_INT,
                [TOKEN_KW_STR]=TYPE_STR,
        };
        Node* new = NULL;

        if (types[getTtype(state)] != TYPEERROR) {
                new = ALLOCATE_SIMPLE_NODE(OP_DECLARE);
                new->type = types[getTtype(state)];
                new->token = consume(state);
                new->operator = OP_DECLARE;
        }
        else {
                return prefixParseError(state);
        }

        if (getTtype(state) == TOKEN_IDENTIFIER) {
                new->operands[0].nd = ALLOCATE_SIMPLE_NODE(OP_VARIABLE);
                *(new->operands[0].nd) = (Node) {.token=consume(state), .operator=OP_VARIABLE};
        }
        else {
                return infixParseError(state, new);
        }

        if (getTtype(state) == TOKEN_EQUAL) {
                consume(state);
                new->operands[1].nd = parseExpression(state, PREC_NONE);
                if (new->operands[1].nd == NULL){
                        freeNode(new);
                        return NULL;
                }
                if (new->operands[1].nd->type != new->type) {
                        Error(&(new->token), "TypeError: type mismatch at declaration.\n");
                        freeNode(new);
                        return NULL;
                }
        }

        if ((new = semicolon_or_error(state, new)) != NULL) {
                state->resolv = record_variable(state->resolv, new->operands[0].nd->token.tok.source, SIZE_MAX, new->type);
        }

        return new;
}

static Node* block_statement(parser_info *const state) {
        const size_t resolv_size = state->resolv->len;
        uintptr_t nb_children = 0;
        Node* stmt = allocateNode(nb_children + 1); // add one, for the length of the array
        stmt->token = consume(state);
        stmt->operator = OP_BLOCK;
        stmt->type=TYPE_VOID;
        while (getTtype(state) != TOKEN_BCLOSE) {
                Node* substmt = parse_statement(state);
                if (substmt == NULL) {
                        stmt->operands[0].len = nb_children;
                        freeNode(stmt);
                        state->resolv->len = resolv_size;
                        return NULL;
                }
                stmt = reallocateNode(stmt, ++nb_children+1);
                stmt->operands[nb_children].nd = substmt;
        }
        stmt->operands[0].len = nb_children;
        consume(state);
        state->resolv->len = resolv_size;
        return stmt;
}

static Node* empty_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_NOP);
        new->token = consume(state);
        new->operator = OP_NOP;
        new->type=TYPE_VOID;
        return new;
}

static Node* if_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_IFELSE);
        new->token = consume(state);
        new->operator = OP_IFELSE;
        new->type = TYPE_VOID;
        new->operands[0].nd = parseExpression(state, PREC_NONE);
        if (new->operands[0].nd == NULL) {
                freeNode(new);
                return NULL;
        }
        if (new->operands[0].nd->type != TYPE_INT) {
                Error(&(new->operands[0].nd->token), "TypeError: can't cast to boolean.\n");
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
        return new;
}

static Node* dowhile_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_DOWHILE);
        new->token = consume(state);
        new->operator = OP_DOWHILE;
        new->type = TYPE_VOID;
        if ((new->operands[0].nd = parse_statement(state)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if (getTtype(state) != TOKEN_WHILE) {
                return infixParseError(state, new);
        }

        consume(state); // `while` token

        new->operands[1].nd = parseExpression(state, PREC_NONE);

        if (new->operands[1].nd == NULL) {
                freeNode(new);
                return NULL;
        }
        if (new->operands[1].nd->type != TYPE_INT) {
                Error(&(new->operands[1].nd->token), "TypeError: can't cast to boolean.\n");
                freeNode(new);
                return NULL;
        }

        return semicolon_or_error(state, new);
}

static Node* while_statement(parser_info *const state) {
        Node* new = ALLOCATE_SIMPLE_NODE(OP_WHILE);
        new->token = consume(state);
        new->operator = OP_WHILE;
        new->type = TYPE_VOID;
        if ((new->operands[0].nd = parseExpression(state, PREC_NONE)) == NULL) {
                freeNode(new);
                return NULL;
        }
        if (new->operands[0].nd->type != TYPE_INT) {
                Error(&(new->operands[0].nd->token), "TypeError: can't cast to boolean.\n");
                freeNode(new);
                return NULL;
        }
        if ((new->operands[1].nd = parse_statement(state)) == NULL) {
                freeNode(new);
                return NULL;
        }
        return new;
}

// ------------------ end statement handlers -----------------------------------

Node* parse_statement(parser_info *const state) {
        static const StatementHandler handlers[TOKEN_EOF] = {
                [TOKEN_BOPEN] = block_statement,
                [TOKEN_KW_INT] = declare_statement,
                [TOKEN_KW_STR] = declare_statement,
                [TOKEN_SEMICOLON] = empty_statement,
                [TOKEN_IF] = if_statement,
                [TOKEN_DO] = dowhile_statement,
                [TOKEN_WHILE] = while_statement,
        };

        if (getTtype(state) == TOKEN_EOF || getTtype(state) == TOKEN_ERROR) return NULL;

        StatementHandler handler = handlers[getTtype(state)];
        if (handler == NULL) return simple_statement(state);
        else return handler(state);
}

#undef ALLOCATE_SIMPLE_NODE
