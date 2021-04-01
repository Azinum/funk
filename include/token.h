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

  T_EXPR, // NOTE(lucas): Tag to identify expression branches in ast

  T_LET,
  T_IF,
  T_DEFINE,

  T_TYPES, // Explicit types

  T_STRING,
  T_NUMBER,
  T_IDENTIFIER,
  T_FUNCTION,
  T_CFUNCTION,

  T_NO_TYPE,  // End of explicit types
};

struct Token {
  char* string;
  i32 length;
  i32 type;

  union {
    i32 number;
  } value;

  i32 line;
  i32 count;
  const char* filename;
  char* source;
};

#define TOKEN_LET "let"
#define TOKEN_IF "if"
#define TOKEN_DEFINE "define"
#define TOKEN_INT "int"
#define TOKEN_STRING "string"

void token_print(FILE* file, struct Token token);

void token_printline(FILE* file, struct Token token);

#endif
