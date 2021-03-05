// parser.c

#include "common.h"
#include "ast.h"
#include "util.h"
#include "lexer.h"
#include "token.h"
#include "parser.h"

#define parse_error(fmt, ...) \
  fprintf(stderr, "parse-error: %s:%i:%i: " fmt, p->l->filename, p->l->line, p->l->count, ##__VA_ARGS__)

static void parser_init(Parser* parser, Lexer* lexer, Ast* ast);
static i32 expect(Parser* p, i32 type);
static i32 end(Parser* p);
static i32 expr_end(Parser* p);
static i32 simple_expr(Parser* p);
static i32 expression(Parser* p);

void parser_init(Parser* p, Lexer* l, Ast* ast) {
  p->l = l;
  p->ast = ast;
  p->status = 0;
}

i32 expect(Parser* p, i32 type) {
  return p->l->token.type == type;
}

i32 end(Parser* p) {
  return expect(p, T_EOF);
}

i32 expr_end(Parser* p) {
  return expect(p, T_CLOSEDPAREN);
}

// Identifier, number, string, operator
// operator '(' expr ')' | symbol
i32 simple_expr(Parser* p) {
  while (!end(p) && !expr_end(p)) {
    struct Token token = get_token(p->l);
    switch (token.type) {
      case T_ADD:
      case T_SUB:
      case T_MULT:
      case T_DIV: {
        Ast* orig = p->ast;
        ast_add_node(p->ast, token);
        next_token(p->l);
        Ast op_branch = ast_get_last(p->ast);
        p->ast = &op_branch;

        expression(p);

        p->ast = orig;  // Return to original branch
        break;
      }

      case T_STRING:
      case T_NUMBER:
      case T_IDENTIFIER: {
        ast_add_node(p->ast, token);
        next_token(p->l);
        break;
      }
      case T_OPENPAREN: {
        expression(p);
        break;
      }
      default:
        parse_error("Unrecognized token\n");
        next_token(p->l);
        return ERR;
    }
  }
  return NO_ERR;
}

// '(' ... ')' | simple_expr
i32 expression(Parser* p) {
  while (!end(p) && !expr_end(p)) {
    struct Token token = get_token(p->l);
    switch (token.type) {
      case T_EOF:
        return NO_ERR;
      case T_OPENPAREN: {
        next_token(p->l); // Skip '('
        simple_expr(p);
        if (!expect(p, T_CLOSEDPAREN)) {
          parse_error("Missing closing ')' parenthesis in expression\n");
          return p->status = ERR;
        }
        next_token(p->l); // Skip ')'
        break;
      }
      default:
        simple_expr(p);
        break;
    }
    if (p->status != NO_ERR)
      return p->status;
  }
  return p->status;
}

i32 parser_parse(char* input, char* filename, Ast* ast) {
  Lexer lexer;
  lexer_init(&lexer, input, filename);

  Parser parser;
  parser_init(&parser, &lexer, ast);

  next_token(parser.l);
  expression(&parser);
  return parser.status;
}
