#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>

#define NOTIMPLEMENTED() (puts("Operation not implemented.\n"), exit(-1))

 Object interpretVariable(const Node* root) {
         NOTIMPLEMENTED();
 }
 Object interpretLiteral(const Node* root) {
         if (root->token.type == TOKEN_INT) {
                 return atoll(root->token.source);
         }
         else {
                 NOTIMPLEMENTED();
         }
 }
 Object interpretUnary(const Node* root) {
         switch (root->token.type) {
                 case TOKEN_PLUS:
                        return interpret(root->operands[0]);
                case TOKEN_MINUS:
                        return -interpret(root->operands[0]);

         }
 }
 Object interpretGroup(const Node* root) {
         return interpret(root->operands[0]);
 }
 Object interpretBinary(const Node* root) {
         Object opA = interpret(root->operands[0]);
         Object opB = interpret(root->operands[1]);
         switch (root->token.type) {
                 case TOKEN_PLUS:
                        return opA + opB;
                case TOKEN_MINUS:
                       return opA - opB;
                case TOKEN_SLASH:
                        return opA / opB;
                case TOKEN_STAR:
                       return opA * opB;

         }

 }
 Object interpretAffect(const Node* root) {
         NOTIMPLEMENTED();
 }
 Object interpretSemicolon(const Node* root) {
         interpret(root->operands[0]);
         return interpret(root->operands[1]);
 }

 const InterpretFn interpreters[] = {
        [OP_VARIABLE] = interpretVariable,
        [OP_LITERAL] = interpretLiteral,

        [OP_MATH_UNARY] = interpretUnary,
        [OP_GROUP] = interpretGroup,

        [OP_MATH_BINARY] = interpretBinary,
        [OP_AFFECT] = interpretAffect,

        [OP_SEMICOLON] = interpretSemicolon,
};

Object interpret(const Node* root) {
        return interpreters[root->operator](root);
}
