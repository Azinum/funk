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
static i32 expressions(Parser* p);

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
      case T_MUL:
      case T_DIV: {
        Ast* orig = p->ast;
        ast_add_node(p->ast, token);
        next_token(p->l); // Skip operator
        Ast op_branch = ast_get_last(p->ast);
        p->ast = &op_branch;

        simple_expr(p);

        p->ast = orig;  // Return to original branch
        break;
      }

      case T_LET: {
        Ast* orig = p->ast;
        ast_add_node(p->ast, token); // Add 'let'
        token = next_token(p->l); // Skip 'let'
        if (!expect(p, T_IDENTIFIER)) {
          parse_error("Expected identifier\n");
          return p->status = ERR;
        }
        ast_add_node(p->ast, token);  // Add identifier
        next_token(p->l);
        Ast assign_branch = ast_get_last(p->ast);
        p->ast = &assign_branch;

        simple_expr(p);

        p->ast = orig;
        break;
      }

      case T_IDENTIFIER:
      case T_NUMBER: {
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

// '(' ... ')'
i32 expression(Parser* p) {
  struct Token token = get_token(p->l);
  switch (token.type) {
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
    default: {
      parse_error("Expected expression\n");
      return p->status = ERR;
    }
  }
  if (p->status != NO_ERR)
    return p->status;
  return p->status;
}

i32 expressions(Parser* p) {
  while (!end(p)) {
    expression(p);
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
  expressions(&parser);
  return parser.status;
}
