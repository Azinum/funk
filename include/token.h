// token.h

#ifndef _TOKEN_H
#define _TOKEN_H

#include "common.h"

enum Token_type {
  T_UNKNOWN,
  T_EOF,

  T_OPERATOR,

  // Arithmetic operators
  T_ASSIGN,         // '='
  T_ADD,
  T_SUB,
  T_MUL,
  T_DIV,
  T_INCREMENT,  // '++'
  T_DECREMENT,  // '--'
  T_MOD,  // '%'

  // Comparison operatos
  T_EQ,   // '=='
  T_NEQ,  // '!='
  T_LT,   // '<'
  T_GT,   // '>'
  T_LEQ,  // '<='
  T_GEQ,  // '>='

  // Bitwise operators
  T_BAND,   // Bitwise '&'
  T_BOR,    // '|'
  T_BXOR,   // '^'
  T_BNOT,   // '~'
  T_BLEFTSHIFT,  // '<<'
  T_BRIGHTSHIFT, // '>>'

  // Logical operators
  T_NOT,  // '!'
  T_AND,  // '&&'
  T_OR,   // '||'

  // Compound assignment operators
  T_ADD_ASSIGN,
  T_SUB_ASSIGN,
  T_MULT_ASSIGN,
  T_DIV_ASSIGN,
  T_MOD_ASSIGN,    // '%='
  T_BAND_ASSIGN,   // '&='
  T_BOR_ASSIGN,    // '|='
  T_BXOR_ASSIGN,   // '^='
  T_BLEFTSHIFT_ASSIGN,  // '<<='
  T_BRIGHTSHIFT_ASSIGN, // '>>='

  T_OPENPAREN,      // '('
  T_CLOSEDPAREN,    // ')'
  T_OPENBRACKET,    // '['
  T_CLOSEDBRACKET,  // ']'
  T_BLOCKBEGIN,     // '{'
  T_BLOCKEND,       // '}'
  T_SEMICOLON,      // ';'
  T_COLON,          // ':'
  T_COMMA,          // ','
  T_DOT,            // '.'
  T_ARROW,          // '->'
  T_DOLLAR,         // '$'

  T_NO_OPERATOR,

  T_LET,

  T_STRING,
  T_NUMBER,
  T_IDENTIFIER,
};

struct Token {
  char* string;
  i32 length;
  i32 type;
};

#define TOKEN_LET "let"

void token_print(FILE* file, struct Token token);

void token_printline(FILE* file, struct Token token);

#endif
