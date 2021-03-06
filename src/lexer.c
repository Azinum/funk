// lexer.c

#include "common.h"
#include "util.h"
#include "error.h"
#include "lexer.h"

#define lex_error(fmt, ...) \
  fprintf(stderr, "lex-error: %s:%i:%i: " fmt, l->filename, l->line, l->count, ##__VA_ARGS__); \
  error_printline(l->source, l->token)

static i32 is_alpha(char ch);
static i32 is_number(char ch);
static i32 end_of_line(Lexer* l);
static i32 match(struct Token token, const char* compare);
static struct Token read_symbol(Lexer* l);
static struct Token read_number(Lexer* l);
static void next(Lexer* l);

i32 is_alpha(char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

i32 is_number(char ch) {
  return ch >= '0' && ch <= '9';
}

i32 end_of_line(Lexer* l) {
  return *l->index == '\n' || *l->index == '\r';
}

i32 match(struct Token token, const char* compare) {
  const char* index = compare;
  for (int i = 0; i < token.length; i++, index++) {
    if (token.string[i] != *index) {
      return 0;
    }
  }
  return (*index == 0);
}

struct Token read_symbol(Lexer* l) {
  while (
    is_alpha(*l->index) ||
    is_number(*l->index) ||
    *l->index == '_'
  ) {
    l->index++;
    l->count++;
  }
  l->token.length = l->index - l->token.string;
  if (match(l->token, TOKEN_LET)) {
    l->token.type = T_LET;
  }
  else if (match(l->token, TOKEN_IF)) {
    l->token.type = T_IF;
  }
  else if (match(l->token, TOKEN_DEFINE)) {
    l->token.type = T_DEFINE;
  }
  else if (match(l->token, TOKEN_INT)) {
    l->token.type = T_NUMBER;
    l->token.value.number = 0;
  }
  else if (match(l->token, TOKEN_STRING)) {
    l->token.type = T_STRING;
  }
  else {
    l->token.type = T_IDENTIFIER;
  }
  return l->token;
}

struct Token read_number(struct Lexer* lexer) {
  while (
    is_number(*lexer->index) ||
    *lexer->index == '.' ||
    *lexer->index == 'x' ||
    (*lexer->index >= 'a' && *lexer->index <= 'f') ||
    (*lexer->index >= 'A' && *lexer->index <= 'F'))
  {
    lexer->index++;
    lexer->count++;
  }
  lexer->token.length = lexer->index - lexer->token.string;
  lexer->token.type = T_NUMBER;
  string_to_int(lexer->token.string, lexer->token.length, &lexer->token.value.number);
  return lexer->token;
}

void next(Lexer* l) {
  l->token.string = l->index++;
  l->token.length = 1;
  l->count++;
}

void lexer_init(Lexer* l, char* input, const char* filename) {
  assert(input);
  l->source = &input[0];
  l->index = &input[0];
  l->line = 1;
  l->count = 1;
  l->token = (struct Token) { .type = T_EOF };
  l->filename = filename;
}

struct Token next_token(Lexer* l) {
  for (;;) {
begin_loop:
    next(l);
    char ch = *l->token.string;
    switch (ch) {
      case '\n':
      case '\r':
        l->line++;
        l->count = 1;
        break;

      case ' ':
      case '\t':
      case '\v':
      case '\f':
        break;

      case '=':
        l->token.type = T_ASSIGN;
        if (*l->index == '=') {
          l->token.type = T_EQ;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '+':
        l->token.type = T_ADD;
        if (*l->index == '=') {
          l->token.type = T_ADD_ASSIGN;
          l->token.length++;
          l->index++;
        }
        else if (*l->index == '+') {
          l->token.type = T_INCREMENT;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '-':
        l->token.type = T_SUB;
        if (*l->index == '=') {
          l->token.type = T_SUB_ASSIGN;
          l->token.length++;
          l->index++;
        }
        else if (*l->index == '-') {
          l->token.type = T_DECREMENT;
          l->token.length++;
          l->index++;
        }
        else if (*l->index == '>') {
          l->token.type = T_ARROW;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '*':
        l->token.type = T_MUL;
        if (*l->index == '=') {
          l->token.type = T_MULT_ASSIGN;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '/':
        if (*l->index == '/') {
          next(l);
          while (!end_of_line(l) && *l->index != '\0') {
            next(l);
          }
          l->line++;
          l->count = 1;
          next(l);
          break;
        }
        else if (*l->index == '*') {
          next(l);
          while (*l->index != '\0') {
            if (*l->index == '*') {
              next(l);
              if (*l->index == '/') {
                next(l);
                goto begin_loop;
              }
            }
            else if (end_of_line(l)) {
              l->line++;
              l->count = 1;
              next(l);
            }
            else {
              next(l);
            }
          }
          return l->token;
        }
        else {
          l->token.type = T_DIV;
          if (*l->index == '=') {
            l->token.type = T_DIV_ASSIGN;
            l->token.length++;
            l->index++;
          }
        }
        return l->token;
      case '%':
        l->token.type = T_MOD;
        if (*l->index == '=') {
          l->token.type = T_MOD_ASSIGN;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '<':
        l->token.type = T_LT;
        if (*l->index == '=') {
          l->token.type = T_LEQ;
          l->token.length++;
          l->index++;
        }
        else if (*l->index == '<') {
          l->token.type = T_BLEFTSHIFT;
          l->token.length++;
          l->index++;
          if (*l->index == '=') {
            l->token.type = T_BLEFTSHIFT_ASSIGN;
            l->token.length++;
            l->index++;
          }
        }
        return l->token;
      case '>':
        l->token.type = T_GT;
        if (*l->index == '=') {
          l->token.type = T_GEQ;
          l->token.length++;
          l->index++;
        }
        else if (*l->index == '>') {
          l->token.type = T_BRIGHTSHIFT;
          l->token.length++;
          l->index++;
          if (*l->index == '=') {
            l->token.type = T_BRIGHTSHIFT_ASSIGN;
            l->token.length++;
            l->index++;
          }
        }
        return l->token;
      case '&':
        l->token.type = T_BAND;
        if (*l->index == '=') {
          l->token.type = T_BAND_ASSIGN;
          l->token.length++;
          l->index++;
        }
        else if (*l->index == '&') {
          l->token.type = T_AND;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '|':
        l->token.type = T_BOR;
        if (*l->index == '=') {
          l->token.type = T_BOR_ASSIGN;
          l->token.length++;
          l->index++;
        }
        else if (*l->index == '|') {
          l->token.type = T_OR;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '^':
        l->token.type = T_BXOR;
        if (*l->index == '=') {
          l->token.type = T_BXOR_ASSIGN;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '~':
        l->token.type = T_BNOT;
        return l->token;
      case '!':
        l->token.type = T_NOT;
        if (*l->index == '=') {
          l->token.type = T_NEQ;
          l->token.length++;
          l->index++;
        }
        return l->token;
      case '(':
        l->token.type = T_OPENPAREN;
        return l->token;
      case ')':
        l->token.type = T_CLOSEDPAREN;
        return l->token;
      case '[':
        l->token.type = T_OPENBRACKET;
        return l->token;
      case ']':
        l->token.type = T_CLOSEDBRACKET;
        return l->token;
      case '{':
        l->token.type = T_BLOCKBEGIN;
        return l->token;
      case '}':
        l->token.type = T_BLOCKEND;
        return l->token;
      case ';':
        l->token.type = T_SEMICOLON;
        return l->token;
      case ':':
        l->token.type = T_COLON;
        return l->token;
      case ',':
        l->token.type = T_COMMA;
        return l->token;
      case '.':
        l->token.type = T_DOT;
        return l->token;
      case '$':
        l->token.type = T_DOLLAR;
        return l->token;

      case '\'':
      case '"': {
        char delim = ch;
        for (;;) {
          if (*l->index == '\0') {
            lex_error("Unfinished string\n");
            l->token.type = T_EOF;
            return l->token;
          }
          else if (*l->index == delim) {
            break;
          }
          l->index++;
          l->count++;
        }
        l->token.string++;
        l->count++;
        l->token.type = T_STRING;
        l->token.length = l->index - l->token.string;
        l->index++;
        return l->token;
      }

      case '\0':
        l->token.type = T_EOF;
        return l->token;

      default: {
        if (is_number(ch)) {
          return read_number(l);
        }
        else if (is_alpha(ch) || ch == '_') {
          return read_symbol(l);
        }
        else {
          lex_error("Unrecognized character\n");
          l->token.type = T_EOF;
          return l->token;
        }
        break;
      }
    }
  }
  return l->token;
}

struct Token get_token(Lexer* l) {
  l->token.line = l->line;
  l->token.count = l->count;
  l->token.filename = l->filename;
  l->token.source = l->source;
  return l->token;
}

struct Token new_token(i32 type) {
  return (struct Token) {
    .string = NULL,
    .length = 0,
    .type = type,
  };
}
