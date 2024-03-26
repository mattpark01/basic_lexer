#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Token types and scanner code
typedef enum {
  TOKEN_VAR,
  TOKEN_SEMICOLON,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE, TOKEN_FOR, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_POWER, TOKEN_EOF, TOKEN_INDENT, TOKEN_DEDENT, TOKEN_NEWLINE,
  TOKEN_LET, TOKEN_FUNC, TOKEN_RETURN, TOKEN_WHILE, TOKEN_TRUE,
  TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_COLON,
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
    double literal;
} Token;

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

typedef enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_GROUPING,
    EXPR_VARIABLE,
    EXPR_ASSIGN,
    EXPR_LOGICAL,
    EXPR_CALL
} ExprType;

typedef struct Expr {
    ExprType type;
    Token token;
    struct Expr* left;
    struct Expr* right;
    struct Expr* expr;
    struct Expr* callee;
    double value;
    const char* name;
    int arg_count;
    struct Expr** args;
    struct {
        int depth;
        bool is_captured;
    } variable;
} Expr;

typedef struct {
    Token name;
    Expr* initializer;
} VarDecl;

typedef enum {
    STMT_EXPR,
    STMT_VAR,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FUNC,
    STMT_RETURN
} StmtType;

typedef struct Stmt {
    StmtType type;
    Expr* expr;
    VarDecl var_decl;
    struct Stmt** stmts;
    int stmt_count;
    struct Stmt* then_branch;
    struct Stmt* else_branch;
    Token name;
    struct Stmt** body;
    int param_count;
    Token* params;
} Stmt;

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

typedef struct {
    const char* name;
    int depth;
    bool is_const;
} Variable;

typedef struct {
    Variable* variables;
    int capacity;
    int count;
} VariableArray;

typedef struct {
    VariableArray variables;
    struct Scope* enclosing;
} Scope;

typedef struct {
    Scope* current;
} Analyzer;

// Function declarations

Stmt* parse(Scanner* scanner);
void resolve(Stmt* stmt, Analyzer* analyzer);

// Scanner functions

void init_scanner(Scanner* scanner, const char* source) {
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
}

bool is_at_end(Scanner* scanner) {
    return *scanner->current == '\0';
}

char advance(Scanner* scanner) {
    scanner->current++;
    return scanner->current[-1];
}

bool match(Scanner* scanner, char expected) {
    if (is_at_end(scanner)) return false;
    if (*scanner->current != expected) return false;
    scanner->current++;
    return true;
}

Token make_token(Scanner* scanner, TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

Token error_token(Scanner* scanner, const char* message) {
    Token token;
    token.type = TOKEN_EOF;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner->line;
    return token;
}

void skip_whitespace(Scanner* scanner) {
    for (;;) {
        char c = *scanner->current;
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;
            case '\n':
                scanner->line++;
                advance(scanner);
                break;
            case '#':
                while (*scanner->current != '\n' && !is_at_end(scanner)) advance(scanner);
                break;
            default:
                return;
        }
    }
}

Token string(Scanner* scanner) {
    while (*scanner->current != '"' && !is_at_end(scanner)) {
        if (*scanner->current == '\n') scanner->line++;
        advance(scanner);
    }

    if (is_at_end(scanner)) return error_token(scanner, "Unterminated string.");

    advance(scanner);
    return make_token(scanner, TOKEN_STRING);
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

Token number(Scanner* scanner) {
    while (is_digit(*scanner->current)) advance(scanner);

    if (*scanner->current == '.' && is_digit(scanner->current[1])) {
        advance(scanner);
        while (is_digit(*scanner->current)) advance(scanner);
    }

    return make_token(scanner, TOKEN_NUMBER);
}

bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

TokenType identifier_type(Scanner* scanner, int start, int length) {
    switch (scanner->start[start]) {
        case 'a': return check_keyword(scanner, start, length, "nd", TOKEN_AND);
        case 'c':
            if (scanner->current - scanner->start > start + 1) {
                switch (scanner->start[start + 1]) {
                    case 'l': return check_keyword(scanner, start, length, "ass", TOKEN_CLASS);
                    case 'o': return check_keyword(scanner, start, length, "nst", TOKEN_VAR);
                }
            }
            break;
        case 'e': return check_keyword(scanner, start, length, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner->current - scanner->start > start + 1) {
                switch (scanner->start[start + 1]) {
                    case 'a': return check_keyword(scanner, start, length, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(scanner, start, length, "r", TOKEN_FOR);
                    case 'u': return check_keyword(scanner, start, length, "nc", TOKEN_FUNC);
                }
            }
            break;
        case 'i': return check_keyword(scanner, start, length, "f", TOKEN_IF);
        case 'n': return check_keyword(scanner, start, length, "il", TOKEN_NIL);
        case 'o': return check_keyword(scanner, start, length, "r", TOKEN_OR);
        case 'r': return check_keyword(scanner, start, length, "eturn", TOKEN_RETURN);
        case 't': return check_keyword(scanner, start, length, "rue", TOKEN_TRUE);
        case 'w': return check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

Token identifier(Scanner* scanner) {
    while (is_alpha(*scanner->current) || is_digit(*scanner->current)) advance(scanner);
    return make_token(scanner, identifier_type(scanner));
}

Token scan_token(Scanner* scanner) {
    skip_whitespace(scanner);

    scanner->start = scanner->current;

    if (is_at_end(scanner)) return make_token(scanner, TOKEN_EOF);

    char c = advance(scanner);

    if (is_alpha(c)) return identifier(scanner);
    if (is_digit(c)) return number(scanner);

    switch (c) {
        case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
        case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
        case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
        case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
        case ';': return make_token(scanner, TOKEN_SEMICOLON);
        case ',': return make_token(scanner, TOKEN_COMMA);
        case '.': return make_token(scanner, TOKEN_DOT);
        case '-': return make_token(scanner, TOKEN_MINUS);
        case '+': return make_token(scanner, TOKEN_PLUS);
        case '/': return make_token(scanner, TOKEN_SLASH);
        case '*':
            return make_token(scanner, match(scanner, '*') ? TOKEN_POWER : TOKEN_STAR);
        case '!':
            return make_token(scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make_token(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string(scanner);
    }

    return error_token(scanner, "Unexpected character.");
}

// Parser functions

Expr* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

Expr* parse_equality(Parser* parser) {
    Expr* expr = parse_comparison(parser);

    while (parser->current.type == TOKEN_EQUAL_EQUAL || parser->current.type == TOKEN_BANG_EQUAL) {
        Token op = parser->current;
        advance(parser);
        Expr* right = parse_comparison(parser);
        expr = binary_expr(expr, op, right);
    }

    return expr;
}

Expr* parse_comparison(Parser* parser) {
    Expr* expr = parse_term(parser);

    while (parser->current.type == TOKEN_LESS || parser->current.type == TOKEN_LESS_EQUAL ||
           parser->current.type == TOKEN_GREATER || parser->current.type == TOKEN_GREATER_EQUAL) {
        Token op = parser->current;
        advance(parser);
        Expr* right = parse_term(parser);
        expr = binary_expr(expr, op, right);
    }

    return expr;
}

Expr* parse_term(Parser* parser) {
    Expr* expr = parse_factor(parser);

    while (parser->current.type == TOKEN_PLUS || parser->current.type == TOKEN_MINUS) {
        Token op = parser->current;
        advance(parser);
        Expr* right = parse_factor(parser);
        expr = binary_expr(expr, op, right);
    }

    return expr;
}

Expr* parse_factor(Parser* parser) {
    Expr* expr = parse_unary(parser);

    while (parser->current.type == TOKEN_STAR || parser->current.type == TOKEN_SLASH) {
        Token op = parser->current;
        advance(parser);
        Expr* right = parse_unary(parser);
        expr = binary_expr(expr, op, right);
    }

    return expr;
}

Expr* parse_unary(Parser* parser) {
    if (parser->current.type == TOKEN_BANG || parser->current.type == TOKEN_MINUS) {
        Token op = parser->current;
        advance(parser);
        Expr* right = parse_unary(parser);
        return unary_expr(op, right);
    }

    return parse_call(parser);
}

Expr* parse_primary(Parser* parser) {
    switch (parser->current.type) {
        case TOKEN_FALSE:
            advance(parser);
            return literal_expr(false);
        case TOKEN_TRUE:
            advance(parser);
            return literal_expr(true);
        case TOKEN_NIL:
            advance(parser);
            return literal_expr(0);
        case TOKEN_NUMBER:
        case TOKEN_STRING:
            return literal_expr(parser->previous.literal);
        case TOKEN_IDENTIFIER:
            return parse_variable(parser);
        case TOKEN_LEFT_PAREN:
            advance(parser);
            Expr* expr = parse_expression(parser);
            consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
            return grouping_expr(expr);
        default:
            error_at_current(parser, "Expect expression.");
            return NULL;
    }
}

Expr* parse_variable(Parser* parser) {
    consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
    return variable_expr(parser->previous);
}

Expr* parse_assignment(Parser* parser) {
    Expr* expr = parse_logical_or(parser);

    if (parser->current.type == TOKEN_EQUAL) {
        Token equals = parser->current;
        advance(parser);
        Expr* value = parse_assignment(parser);

        if (expr->type == EXPR_VARIABLE) {
            Token name = expr->token;
            return assign_expr(name, value);
        }

        error_at_current(parser, "Invalid assignment target.");
    }

    return expr;
}

Expr* parse_logical_or(Parser* parser) {
    Expr* expr = parse_logical_and(parser);

    while (parser->current.type == TOKEN_OR) {
        Token op = parser->current;
        advance(parser);
        Expr* right = parse_logical_and(parser);
        expr = logical_expr(expr, op, right);
    }

    return expr;
}

Expr* parse_logical_and(Parser* parser) {
    Expr* expr = parse_equality(parser);

    while (parser->current.type == TOKEN_AND) {
        Token op = parser->current;
        advance(parser);
        Expr* right = parse_equality(parser);
        expr = logical_expr(expr, op, right);
    }

    return expr;
}

Expr* parse_call(Parser* parser) {
    Expr* expr = parse_primary(parser);

    while (true) {
        if (parser->current.type == TOKEN_LEFT_PAREN) {
            advance(parser);
            expr = finish_call(parser, expr);
        } else {
            break;
        }
    }

    return expr;
}

Expr* parse_arguments(Parser* parser) {
    Expr** args = NULL;
    int arg_count = 0;

    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        do {
            if (arg_count >= 255) {
                error_at_current(parser, "Can't have more than 255 arguments.");
            }

            args = realloc(args, (arg_count + 1) * sizeof(Expr*));
            args[arg_count++] = parse_expression(parser);
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

    Expr* expr = call_expr(parser->previous, args, arg_count);
    free(args);
    return expr;
}

Stmt* parse_declaration(Parser* parser) {
    if (match(parser, TOKEN_VAR)) {
        return parse_var_declaration(parser);
    }

    return parse_statement(parser);
}

Stmt* parse_var_declaration(Parser* parser) {
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");

    Expr* initializer = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        initializer = parse_expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    return var_stmt(name, initializer);
}

Stmt* parse_statement(Parser* parser) {
    if (match(parser, TOKEN_LEFT_BRACE)) {
        return parse_block_statement(parser);
    }

    if (match(parser, TOKEN_IF)) {
        return parse_if_statement(parser);
    }

    if (match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }

    if (match(parser, TOKEN_FUNC)) {
        return parse_func_declaration(parser);
    }

    if (match(parser, TOKEN_RETURN)) {
        return parse_return_statement(parser);
    }

    return parse_expr_statement(parser);
}

Stmt* parse_expr_statement(Parser* parser) {
    Expr* expr = parse_expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    return expr_stmt(expr);
}

Stmt* parse_block_statement(Parser* parser) {
    Stmt** stmts = NULL;
    int stmt_count = 0;

    while (parser->current.type != TOKEN_RIGHT_BRACE && parser->current.type != TOKEN_EOF) {
        stmts = realloc(stmts, (stmt_count + 1) * sizeof(Stmt*));
        stmts[stmt_count++] = parse_declaration(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    return block_stmt(stmts, stmt_count);
}

Stmt* parse_if_statement(Parser* parser) {
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    Expr* condition = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");

    Stmt* then_branch = parse_statement(parser);
    Stmt* else_branch = NULL;

    if (match(parser, TOKEN_ELSE)) {
        else_branch = parse_statement(parser);
    }

    return if_stmt(condition, then_branch, else_branch);
}

Stmt* parse_while_statement(Parser* parser) {
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    Expr* condition = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after while condition.");

    Stmt* body = parse_statement(parser);

    return while_stmt(condition, body);
}

Stmt* parse_func_declaration(Parser* parser) {
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expect function name.");

    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    Token* params = NULL;
    int param_count = 0;

    if (parser->current.type != TOKEN_RIGHT_PAREN) {
        do {
            if (param_count >= 255) {
                error_at_current(parser, "Can't have more than 255 parameters.");
            }

            params = realloc(params, (param_count + 1) * sizeof(Token*));
            params[param_count++] = consume(parser, TOKEN_IDENTIFIER, "Expect parameter name.");
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before function body.");

    Stmt** body = NULL;
    int body_count = 0;

    while (parser->current.type != TOKEN_RIGHT_BRACE && parser->current.type != TOKEN_EOF) {
        body = realloc(body, (body_count + 1) * sizeof(Stmt*));
        body[body_count++] = parse_declaration(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after function body.");

    return func_stmt(name, params, param_count, body, body_count);
}

Stmt* parse_return_statement(Parser* parser) {
    Token keyword = parser->previous;
    Expr* value = NULL;

    if (parser->current.type != TOKEN_SEMICOLON) {
        value = parse_expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value.");
    return return_stmt(keyword, value);
}

// Semantic analyzer functions

void resolve_stmt(Stmt* stmt, Analyzer* analyzer) {
    switch (stmt->type) {
        case STMT_EXPR:
            resolve_expr(stmt->expr, analyzer);
            break;
        case STMT_VAR:
            resolve_var_decl(stmt->var_decl, analyzer);
            break;
        case STMT_BLOCK:
            resolve_block_stmt(stmt, analyzer);
            break;
        case STMT_IF:
            resolve_if_stmt(stmt, analyzer);
            break;
        case STMT_WHILE:
            resolve_while_stmt(stmt, analyzer);
            break;
        case STMT_FUNC:
            resolve_func_decl(stmt, analyzer);
            break;
        case STMT_RETURN:
            resolve_return_stmt(stmt, analyzer);
            break;
    }
}

void resolve_expr(Expr* expr, Analyzer* analyzer) {
    switch (expr->type) {
        case EXPR_BINARY:
            resolve_binary_expr(expr, analyzer);
            break;
        case EXPR_UNARY:
            resolve_unary_expr(expr, analyzer);
            break;
        case EXPR_LITERAL:
            resolve_literal_expr(expr, analyzer);
            break;
        case EXPR_GROUPING:
            resolve_grouping_expr(expr, analyzer);
            break;
        case EXPR_VARIABLE:
            resolve_variable_expr(expr, analyzer);
            break;
        case EXPR_ASSIGN:
            resolve_assign_expr(expr, analyzer);
            break;
        case EXPR_LOGICAL:
            resolve_logical_expr(expr, analyzer);
            break;
        case EXPR_CALL:
            resolve_call_expr(expr, analyzer);
            break;
    }
}

void resolve_var_decl(VarDecl var_decl, Analyzer* analyzer) {
    declare_variable(analyzer, &var_decl.name, false);
    if (var_decl.initializer != NULL) {
        resolve_expr(var_decl.initializer, analyzer);
    }
    define_variable(analyzer, &var_decl.name);
}

void resolve_block_stmt(Stmt* stmt, Analyzer* analyzer) {
    begin_scope(analyzer);
    for (int i = 0; i < stmt->stmt_count; i++) {
        resolve_stmt(stmt->stmts[i], analyzer);
    }
    end_scope(analyzer);
}

void resolve_if_stmt(Stmt* stmt, Analyzer* analyzer) {
    resolve_expr(stmt->expr, analyzer);
    resolve_stmt(stmt->then_branch, analyzer);
    if (stmt->else_branch != NULL) {
        resolve_stmt(stmt->else_branch, analyzer);
    }
}

void resolve_while_stmt(Stmt* stmt, Analyzer* analyzer) {
    resolve_expr(stmt->expr, analyzer);
    resolve_stmt(stmt->stmts[0], analyzer);
}

void resolve_func_decl(Stmt* stmt, Analyzer* analyzer) {
    declare_variable(analyzer, &stmt->name, false);
    define_variable(analyzer, &stmt->name);

    begin_scope(analyzer);
    for (int i = 0; i < stmt->param_count; i++) {
        declare_variable(analyzer, &stmt->params[i], false);
        define_variable(analyzer, &stmt->params[i]);
    }

    for (int i = 0; i < stmt->stmt_count; i++) {
        resolve_stmt(stmt->body[i], analyzer);
    }
    end_scope(analyzer);
}

void resolve_return_stmt(Stmt* stmt, Analyzer* analyzer) {
    if (stmt->expr != NULL) {
        resolve_expr(stmt->expr, analyzer);
    }
}

void resolve_binary_expr(Expr* expr, Analyzer* analyzer) {
    resolve_expr(expr->left, analyzer);
    resolve_expr(expr->right, analyzer);
}

void resolve_unary_expr(Expr* expr, Analyzer* analyzer) {
    resolve_expr(expr->right, analyzer);
}

void resolve_literal_expr(Expr* expr, Analyzer* analyzer) {
    // No resolution needed for literal expressions
}

void resolve_grouping_expr(Expr* expr, Analyzer* analyzer) {
    resolve_expr(expr->expr, analyzer);
}

void resolve_variable_expr(Expr* expr, Analyzer* analyzer) {
    if (analyzer->current->variables.count > 0 &&
        lookup_variable(analyzer->current, expr->name) == NULL) {
        error(&expr->token, "Cannot read local variable in its own initializer.");
    }
    resolve_local(analyzer, expr, expr->name);
}

void resolve_assign_expr(Expr* expr, Analyzer* analyzer) {
    resolve_expr(expr->right, analyzer);
    resolve_local(analyzer, expr, &expr->token);
}

void resolve_logical_expr(Expr* expr, Analyzer* analyzer) {
    resolve_expr(expr->left, analyzer);
    resolve_expr(expr->right, analyzer);
}

void resolve_call_expr(Expr* expr, Analyzer* analyzer) {
    resolve_expr(expr->callee, analyzer);

    for (int i = 0; i < expr->arg_count; i++) {
        resolve_expr(expr->args[i], analyzer);
    }
}

// Variable array functions

void init_variable_array(VariableArray* array) {
    array->variables = NULL;
    array->capacity = 0;
    array->count = 0;
}

void free_variable_array(VariableArray* array) {
    free(array->variables);
    init_variable_array(array);
}

void write_variable(VariableArray* array, Token* name, bool is_const) {
    if (array->capacity < array->count + 1) {
        int new_capacity = array->capacity < 8 ? 8 : array->capacity * 2;
        array->variables = realloc(array->variables, new_capacity * sizeof(Variable));
        array->capacity = new_capacity;
    }

    array->variables[array->count].name = name->start;
    array->variables[array->count].depth = array->count;
    array->variables[array->count].is_const = is_const;
    array->count++;
}

bool resolve_local(Analyzer* analyzer, Expr* expr, Token* name) {
    for (int i = analyzer->current->variables.count - 1; i >= 0; i--) {
        Variable* variable = &analyzer->current->variables.variables[i];
        if (variable->depth != -1 && variable->depth < analyzer->current->variables.count) {
            break;
        }

        if (memcmp(name->start, variable->name, name->length) == 0 && name->length == strlen(variable->name)) {
            expr->variable.depth = analyzer->current->variables.count - variable->depth;
            expr->variable.is_captured = false;
            return true;
        }
    }

    return false;
}

void declare_variable(Analyzer* analyzer, Token* name, bool is_const) {
    if (analyzer->current->variables.count == 0) return;

    for (int i = analyzer->current->variables.count - 1; i >= 0; i--) {
        Variable* variable = &analyzer->current->variables.variables[i];
        if (variable->depth != -1 && variable->depth < analyzer->current->variables.count) {
            break;
        }

        if (memcmp(name->start, variable->name, name->length) == 0 && name->length == strlen(variable->name)) {
            error(name, "Variable with this name already declared in this scope.");
        }
    }

    write_variable(&analyzer->current->variables, name, is_const);
}

void define_variable(Analyzer* analyzer, Token* name) {
    if (analyzer->current->variables.count == 0) return;
    analyzer->current->variables.variables[analyzer->current->variables.count - 1].depth = analyzer->current->variables.count;
}

void define_variable(Analyzer* analyzer, Token* name) {
    if (analyzer->current->variables.count == 0) return;
    analyzer->current->variables.variables[analyzer->current->variables.count - 1].depth = analyzer->current->variables.count;
}
// Scope functions

Scope* new_scope(Scope* enclosing) {
    Scope* scope = malloc(sizeof(Scope));
    scope->enclosing = enclosing;
    init_variable_array(&scope->variables);
    return scope;
}

void free_scope(Scope* scope) {
    free_variable_array(&scope->variables);
    free(scope);
}

void begin_scope(Analyzer* analyzer) {
    Scope* scope = new_scope(analyzer->current);
    analyzer->current = scope;
}

void end_scope(Analyzer* analyzer) {
    Scope* scope = analyzer->current;
    analyzer->current = scope->enclosing;
    free_scope(scope);
}

// Error handling functions

void error(Token* token, const char* message) {
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, "[line %d] Error at end: %s\n", token->line, message);
    } else {
        fprintf(stderr, "[line %d] Error at '%.*s': %s\n", token->line, token->length, token->start, message);
    }
}

void error_at_current(Parser* parser, const char* message) {
    error(&parser->current, message);
}

void error_at_previous(Parser* parser, const char* message) {
    error(&parser->previous, message);
}
// Parsing and resolution functions

Stmt* parse(Scanner* scanner) {
    Parser parser;
    parser.current = scan_token(scanner);
    parser.previous = parser.current;
    parser.had_error = false;
    parser.panic_mode = false;

    Stmt* stmt = parse_declaration(&parser);

    if (parser.had_error) {
        // Handle parsing error
        return NULL;
    }

    return stmt;
}

void resolve(Stmt* stmt, Analyzer* analyzer) {
    resolve_stmt(stmt, analyzer);
}

// Main function

int main() {
    const char* input_code = "\n"
        "\"example\" # STRING\n"
        "{ # LEFT_BRACE\n"
        "    \"nested\" # STRING\n"
        "} # RIGHT_BRACE\n"
        "const number = 12345\n"
        "if number > 0:\n"
        "    print(\"Positive\")\n"
        "else:\n"
        "    print(\"Negative\")\n"
        "func greet(name):\n"
        "    print(\"Hello, \" + name + \"!\")\n"
        "greet(\"World\")\n";

    Scanner scanner;
    init_scanner(&scanner, input_code);

    Stmt* stmt = parse(&scanner);
    if (stmt == NULL) {
        printf("Parsing failed.\n");
        return 1;
    }

    Analyzer analyzer;
    Scope global_scope;
    init_variable_array(&global_scope.variables);
    global_scope.enclosing = NULL;
    analyzer.current = &global_scope;

    resolve(stmt, &analyzer);

    // Execute the parsed and analyzed code
    // ...

    free_scope(&global_scope);

    return 0;
}