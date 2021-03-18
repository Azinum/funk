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
static i32 func_params(Parser* p);
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

i32 func_params(Parser* p) {
  for (;;) {
    struct Token token = get_token(p->l);
    switch (token.type) {
      case T_IDENTIFIER: {
        ast_add_node(p->ast, token);
        next_token(p->l);
        break;
      }
      case T_CLOSEDPAREN: {
        return NO_ERR;
      }
      default:
        parse_error("Expected identifier in parameter list\n");
        return p->status = ERR;
    }
  }
  return NO_ERR;
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
      case T_DIV:
      case T_LT:
      case T_GT:
      case T_EQ: {
        Ast* orig = p->ast;
        Ast op_branch = ast_add_node(p->ast, token);  // Add operator
        next_token(p->l); // Skip operator
        p->ast = &op_branch;

        simple_expr(p);

        i32 child_count = ast_child_count(p->ast);
        if (child_count != 2) {
          p->ast = orig;
          parse_error("Invalid number of parameters (got %i, should be %i)\n", child_count, 2);
          return p->status = ERR;
        }
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
        next_token(p->l); // Skip identifier

        // Explicit value type
        if (expect(p, T_COLON)) {
          next_token(p->l); // Skip ':'
          token = get_token(p->l);
          if (token.type > T_TYPES && token.type < T_NO_TYPE) {
            ast_add_node(p->ast, get_token(p->l));  // Add type
            next_token(p->l); // Skip type
          }
          else {
            parse_error("The type '%.*s' is not defined\n", token.length, token.string);
            return p->status = ERR;
          }
        }

        Ast assign_branch = ast_get_last(p->ast);
        p->ast = &assign_branch;

        simple_expr(p);

        i32 assign_branch_child_count = ast_child_count(p->ast);
        if (assign_branch_child_count != 1) {
          p->ast = orig; // Return to the original branch, so that we don't iterate/write on an empty branch
          parse_error("Invalid number of expressions in value assignment (got %i, should be %i)\n", assign_branch_child_count, 1);
          return p->status = ERR;
        }

        p->ast = orig;
        break;
      }

      // (if (cond) (true-expr) (false-expr))
      // (if (cond) (true-expr))
      case T_IF: {
        Ast* orig = p->ast;
        ast_add_node(p->ast, token); // Add 'if'
        next_token(p->l); // Skip 'if'

        Ast if_branch = ast_get_last(p->ast);
        p->ast = &if_branch;

        ast_add_node(p->ast, new_token(T_EXPR));
        Ast cond = ast_get_last(p->ast);
        p->ast = &cond;

        // Condition
        if (expression(p) != NO_ERR) {
          p->ast = orig;
          parse_error("Missing condition in if expression\n");
          return p->status;
        }

        p->ast = &if_branch;
        ast_add_node(p->ast, new_token(T_EXPR));
        Ast true_body = ast_get_last(p->ast);
        p->ast = &true_body;

        // True expression body
        if (expression(p) != NO_ERR) {
          parse_error("Missing if body\n");
          p->ast = orig;
          return p->status;
        }

        p->ast = &if_branch;
        ast_add_node(p->ast, new_token(T_EXPR));
        Ast false_body = ast_get_last(p->ast);
        p->ast = &false_body;

        if (expect(p, T_OPENPAREN)) {
          // False expression body (is optional)
          if (expression(p) != NO_ERR) {
            p->ast = orig;
            return p->status;
          }
        }
        p->ast = orig;
        break;
      }
      // (define name (args) (body))
      case T_DEFINE: {
        Ast* orig = p->ast;
        Ast define_branch = ast_add_node(p->ast, token); // Add 'define'
        token = next_token(p->l); // Skip 'define'

        if (!expect(p, T_IDENTIFIER)) {
          parse_error("Expected identifier\n");
          return p->status = ERR;
        }

        p->ast = &define_branch;

        ast_add_node(p->ast, token);  // Add function identifier
        next_token(p->l); // Skip identifier

        if (!expect(p, T_OPENPAREN)) {
          p->ast = orig;
          parse_error("Expected parameter list in function definition\n");
          return p->status = ERR;
        }
        next_token(p->l); // Skip '('

        p->ast = &define_branch;
        ast_add_node(p->ast, new_token(T_EXPR));
        Ast args = ast_get_last(p->ast);
        p->ast = &args;

        func_params(p);  // Parse function parameters

        if (!expect(p, T_CLOSEDPAREN)) {
          p->ast = orig;
          parse_error("Missing closing ')' parenthesis in parameter list\n");
          return p->status = ERR;
        }
        next_token(p->l); // Skip ')'

        p->ast = &define_branch;
        ast_add_node(p->ast, new_token(T_EXPR));
        Ast body = ast_get_last(p->ast);
        p->ast = &body;

        simple_expr(p); // Parse function body

        p->ast = orig;
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
        return p->status = ERR;
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
      Ast* orig = p->ast;
      Ast expr_branch = ast_add_node(p->ast, new_token(T_EXPR));

      p->ast = &expr_branch;
      simple_expr(p);
      p->ast = orig;

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
