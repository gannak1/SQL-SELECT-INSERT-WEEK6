/*
 * 파일 목적:
 *   TokenArray를 읽어 INSERT / SELECT statement 구조체로 바꾸는 parser를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer 다음 단계이며,
 *   executor가 다루기 쉬운 Program 구조를 만드는 핵심 연결 고리다.
 *
 * 주요 내용:
 *   - INSERT parser
 *   - SELECT parser
 *   - WHERE 단일 조건 parser
 *   - Program 배열 확장
 *
 * 초보자 포인트:
 *   parser는 "토큰의 순서"를 검사한다.
 *   예를 들어 SELECT 뒤에 컬럼 목록이 오고 FROM 뒤에 테이블명이 와야 한다.
 *
 * 메모리 / 문자열 주의점:
 *   파싱 도중 일부만 성공한 상태에서 실패할 수 있으므로,
 *   매 단계마다 부분 할당 메모리를 정리하는 코드가 중요하다.
 */

#include "parser.h"

#include <stdlib.h>
#include <string.h>

#include "common.h"

typedef struct {
    const TokenArray *tokens;
    size_t current;
    SqlError *error;
} Parser;

static const Token *peek_token(Parser *parser);
static const Token *previous_token(Parser *parser);
static int is_at_end(Parser *parser);
static const Token *advance_token(Parser *parser);
static int check_token(Parser *parser, TokenType type);
static int match_token(Parser *parser, TokenType type);
static int check_keyword(Parser *parser, const char *keyword);
static int match_keyword(Parser *parser, const char *keyword);
static int parser_error_at(Parser *parser, const Token *token, const char *message);
static int expect_token(Parser *parser, TokenType type, const Token **out_token, const char *message);
static int expect_keyword(Parser *parser, const char *keyword);
static int append_statement(Program *program, Statement statement, SqlError *error);
static int append_literal(InsertStatement *statement, LiteralValue value, SqlError *error);
static int append_selected_column(SelectStatement *statement, char *owned_name, SqlError *error);
static int parse_literal(Parser *parser, LiteralValue *out_value);
static int parse_operator(Parser *parser, OperatorType *out_operator);
static int parse_insert_statement(Parser *parser, Statement *out_statement);
static int parse_select_statement(Parser *parser, Statement *out_statement);

/*
 * 목적:
 *   현재 parser가 보고 있는 토큰을 반환한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *
 * 반환:
 *   current 인덱스의 토큰 포인터.
 *
 * 동작 요약:
 *   배열을 직접 참조한다.
 *
 * 초보자 주의:
 *   current는 아직 소비하지 않은 토큰 위치를 가리킨다.
 */
static const Token *peek_token(Parser *parser) {
    return &parser->tokens->items[parser->current];
}

/*
 * 목적:
 *   직전에 소비한 토큰을 반환한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *
 * 반환:
 *   current 바로 앞 토큰 포인터.
 *
 * 동작 요약:
 *   advance_token 이후에 방금 먹은 토큰을 확인할 때 사용한다.
 *
 * 초보자 주의:
 *   current가 0일 때 호출하면 안 되므로 호출 위치가 중요하다.
 */
static const Token *previous_token(Parser *parser) {
    return &parser->tokens->items[parser->current - 1];
}

/*
 * 목적:
 *   현재 위치가 EOF 토큰인지 검사한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *
 * 반환:
 *   EOF면 1, 아니면 0.
 *
 * 동작 요약:
 *   peek_token의 type을 확인한다.
 *
 * 초보자 주의:
 *   SQL 파일 끝에 EOF 토큰을 하나 넣어두면 경계 조건 처리가 쉬워진다.
 */
static int is_at_end(Parser *parser) {
    return peek_token(parser)->type == TOKEN_EOF;
}

/*
 * 목적:
 *   현재 토큰을 소비하고 다음 위치로 이동한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *
 * 반환:
 *   방금 소비한 토큰 포인터.
 *
 * 동작 요약:
 *   EOF가 아니면 current를 1 증가시키고 이전 토큰을 반환한다.
 *
 * 초보자 주의:
 *   "읽기만 하는지"와 "실제로 소비하는지"를 분리하면 parser 버그가 줄어든다.
 */
static const Token *advance_token(Parser *parser) {
    if (!is_at_end(parser)) {
        parser->current += 1;
    }
    return previous_token(parser);
}

/*
 * 목적:
 *   현재 토큰이 특정 타입인지 확인한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   type - 기대하는 토큰 타입.
 *
 * 반환:
 *   맞으면 1, 아니면 0.
 *
 * 동작 요약:
 *   EOF 여부와 타입을 비교한다.
 *
 * 초보자 주의:
 *   check는 위치를 움직이지 않는다.
 */
static int check_token(Parser *parser, TokenType type) {
    if (is_at_end(parser)) {
        return type == TOKEN_EOF;
    }
    return peek_token(parser)->type == type;
}

/*
 * 목적:
 *   현재 토큰이 특정 타입이면 소비하고 true를 반환한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   type - 기대하는 토큰 타입.
 *
 * 반환:
 *   일치하면 1, 아니면 0.
 *
 * 동작 요약:
 *   check_token이 참일 때만 advance_token을 호출한다.
 *
 * 초보자 주의:
 *   optional 문법을 읽을 때 이런 함수가 특히 편리하다.
 */
static int match_token(Parser *parser, TokenType type) {
    if (!check_token(parser, type)) {
        return 0;
    }
    advance_token(parser);
    return 1;
}

/*
 * 목적:
 *   현재 토큰이 특정 keyword인지 검사한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   keyword - 기대하는 keyword 문자열.
 *
 * 반환:
 *   맞으면 1, 아니면 0.
 *
 * 동작 요약:
 *   토큰 타입이 KEYWORD인지 보고 lexeme을 비교한다.
 *
 * 초보자 주의:
 *   tokenizer에서 keyword를 대문자로 정규화해 두었기 때문에 비교가 단순하다.
 */
static int check_keyword(Parser *parser, const char *keyword) {
    return check_token(parser, TOKEN_KEYWORD) && strcmp(peek_token(parser)->lexeme, keyword) == 0;
}

/*
 * 목적:
 *   현재 토큰이 특정 keyword이면 소비한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   keyword - 기대하는 keyword 문자열.
 *
 * 반환:
 *   일치하면 1, 아니면 0.
 *
 * 동작 요약:
 *   check_keyword가 참일 때만 토큰을 소비한다.
 *
 * 초보자 주의:
 *   SELECT 뒤에 WHERE가 있는지처럼 선택적 구문을 처리할 때 자주 쓴다.
 */
static int match_keyword(Parser *parser, const char *keyword) {
    if (!check_keyword(parser, keyword)) {
        return 0;
    }
    advance_token(parser);
    return 1;
}

/*
 * 목적:
 *   특정 토큰 위치를 기준으로 문법 오류 메시지를 만든다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   token - 문제가 발생한 토큰.
 *   message - 설명 문자열.
 *
 * 반환:
 *   항상 0.
 *
 * 동작 요약:
 *   line/column 정보를 포함해 SqlError에 메시지를 기록한다.
 *
 * 초보자 주의:
 *   에러 메시지에 위치를 넣으면 디버깅 시간이 크게 줄어든다.
 */
static int parser_error_at(Parser *parser, const Token *token, const char *message) {
    sql_error_set(
        parser->error,
        "ERROR: %s at line %d, column %d near %s",
        message,
        token->line,
        token->column,
        token->lexeme
    );
    return 0;
}

/*
 * 목적:
 *   현재 토큰이 기대한 타입인지 확인하고 소비한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   type - 기대 타입.
 *   out_token - 성공 시 소비한 토큰을 받을 포인터.
 *   message - 실패 시 에러 메시지 앞부분.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   타입이 맞으면 advance_token, 아니면 parser_error_at을 호출한다.
 *
 * 초보자 주의:
 *   "반드시 있어야 하는 토큰"은 이런 expect 함수로 처리하면 코드가 읽기 쉽다.
 */
static int expect_token(Parser *parser, TokenType type, const Token **out_token, const char *message) {
    if (!check_token(parser, type)) {
        return parser_error_at(parser, peek_token(parser), message);
    }

    *out_token = advance_token(parser);
    return 1;
}

/*
 * 목적:
 *   현재 토큰이 특정 keyword인지 확인하고 소비한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   keyword - 기대하는 keyword.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   check_keyword로 검사한 뒤 실패하면 위치 기반 에러를 만든다.
 *
 * 초보자 주의:
 *   INSERT 다음에 INTO가 없으면 여기서 바로 문법 오류를 잡는다.
 */
static int expect_keyword(Parser *parser, const char *keyword) {
    if (!check_keyword(parser, keyword)) {
        return parser_error_at(parser, peek_token(parser), "unexpected token");
    }

    advance_token(parser);
    return 1;
}

/*
 * 목적:
 *   Program 끝에 statement 하나를 추가한다.
 *
 * 입력:
 *   program - Program 배열.
 *   statement - 추가할 statement 값.
 *   error - 메모리 실패 시 기록할 에러 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   필요 시 realloc으로 배열을 늘린 뒤 statement를 복사 저장한다.
 *
 * 초보자 주의:
 *   구조체 자체는 값 복사되지만, 그 안의 포인터가 가리키는 실제 메모리는 그대로 공유된다.
 */
static int append_statement(Program *program, Statement statement, SqlError *error) {
    Statement *resized_items;
    size_t new_capacity;

    if (program->count == program->capacity) {
        new_capacity = program->capacity == 0 ? 8 : program->capacity * 2;
        resized_items = (Statement *)realloc(program->statements, new_capacity * sizeof(Statement));
        if (resized_items == NULL) {
            sql_error_set(error, "ERROR: out of memory while growing statement list");
            return 0;
        }
        program->statements = resized_items;
        program->capacity = new_capacity;
    }

    program->statements[program->count] = statement;
    program->count += 1;
    return 1;
}

/*
 * 목적:
 *   INSERT statement의 values 배열 끝에 literal을 추가한다.
 *
 * 입력:
 *   statement - INSERT statement.
 *   value - 추가할 literal 값.
 *   error - 메모리 실패 시 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   values 배열을 한 칸 늘려 뒤에 새 literal을 복사한다.
 *
 * 초보자 주의:
 *   literal 안에 TEXT 문자열이 있으면 구조체 값 복사만으로도 포인터가 함께 옮겨진다.
 */
static int append_literal(InsertStatement *statement, LiteralValue value, SqlError *error) {
    LiteralValue *resized_values;
    size_t new_count;

    new_count = statement->value_count + 1;
    resized_values = (LiteralValue *)realloc(statement->values, new_count * sizeof(LiteralValue));
    if (resized_values == NULL) {
        sql_error_set(error, "ERROR: out of memory while growing INSERT values");
        return 0;
    }

    statement->values = resized_values;
    statement->values[statement->value_count] = value;
    statement->value_count = new_count;
    return 1;
}

/*
 * 목적:
 *   SELECT statement의 selected_columns 배열 끝에 컬럼 이름을 추가한다.
 *
 * 입력:
 *   statement - SELECT statement.
 *   owned_name - 이미 할당된 컬럼 이름 문자열 소유권.
 *   error - 메모리 실패 시 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   selected_columns 배열을 realloc으로 늘리고 새 문자열을 저장한다.
 *
 * 초보자 주의:
 *   실패 시 owned_name도 잃어버리지 않도록 직접 free해준다.
 */
static int append_selected_column(SelectStatement *statement, char *owned_name, SqlError *error) {
    char **resized_columns;
    size_t new_count;

    new_count = statement->selected_column_count + 1;
    resized_columns = (char **)realloc(statement->selected_columns, new_count * sizeof(char *));
    if (resized_columns == NULL) {
        free(owned_name);
        sql_error_set(error, "ERROR: out of memory while growing SELECT columns");
        return 0;
    }

    statement->selected_columns = resized_columns;
    statement->selected_columns[statement->selected_column_count] = owned_name;
    statement->selected_column_count = new_count;
    return 1;
}

/*
 * 목적:
 *   현재 토큰이 int literal 또는 string literal인지 읽어 LiteralValue로 만든다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   out_value - 성공 시 채워질 literal 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   토큰 타입을 보고 long 정수 또는 TEXT 복사본을 만든다.
 *
 * 초보자 주의:
 *   string literal은 tokenizer에서 작은따옴표를 제거했기 때문에 그대로 복사하면 된다.
 */
static int parse_literal(Parser *parser, LiteralValue *out_value) {
    const Token *token;
    char *end_ptr;

    memset(out_value, 0, sizeof(*out_value));

    if (check_token(parser, TOKEN_INT_LITERAL)) {
        token = advance_token(parser);
        out_value->type = DATA_TYPE_INT;
        out_value->int_value = strtol(token->lexeme, &end_ptr, 10);
        if (*end_ptr != '\0') {
            return parser_error_at(parser, token, "invalid integer literal");
        }
        return 1;
    }

    if (check_token(parser, TOKEN_STRING_LITERAL)) {
        token = advance_token(parser);
        out_value->type = DATA_TYPE_TEXT;
        out_value->text_value = sql_strdup(token->lexeme);
        if (out_value->text_value == NULL) {
            sql_error_set(parser->error, "ERROR: out of memory while copying string literal");
            return 0;
        }
        return 1;
    }

    return parser_error_at(parser, peek_token(parser), "expected literal value");
}

/*
 * 목적:
 *   현재 연산자 토큰을 OperatorType enum으로 변환한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   out_operator - 성공 시 결과 연산자.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   연산자 lexeme 문자열을 비교해 enum 값으로 대응시킨다.
 *
 * 초보자 주의:
 *   tokenizer는 문자열만 알고 있으므로,
 *   실행 단계에서 쓰기 좋은 enum 변환은 parser에서 하는 편이 좋다.
 */
static int parse_operator(Parser *parser, OperatorType *out_operator) {
    const Token *token;

    if (!expect_token(parser, TOKEN_OPERATOR, &token, "expected operator")) {
        return 0;
    }

    if (strcmp(token->lexeme, "=") == 0) {
        *out_operator = OP_EQ;
    } else if (strcmp(token->lexeme, "!=") == 0) {
        *out_operator = OP_NEQ;
    } else if (strcmp(token->lexeme, "<") == 0) {
        *out_operator = OP_LT;
    } else if (strcmp(token->lexeme, ">") == 0) {
        *out_operator = OP_GT;
    } else if (strcmp(token->lexeme, "<=") == 0) {
        *out_operator = OP_LTE;
    } else if (strcmp(token->lexeme, ">=") == 0) {
        *out_operator = OP_GTE;
    } else {
        return parser_error_at(parser, token, "unsupported operator");
    }

    return 1;
}

/*
 * 목적:
 *   INSERT 문 하나를 파싱한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   out_statement - 성공 시 결과 statement.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   INSERT INTO table VALUES (...) ; 순서를 검사하고 value list를 모은다.
 *
 * 초보자 주의:
 *   값 목록은 하나 이상이어야 하므로 빈 괄호는 허용하지 않는다.
 */
static int parse_insert_statement(Parser *parser, Statement *out_statement) {
    const Token *table_token;
    Statement statement;

    memset(&statement, 0, sizeof(statement));
    statement.type = STMT_INSERT;

    if (!expect_keyword(parser, "INSERT") || !expect_keyword(parser, "INTO")) {
        return 0;
    }

    if (!expect_token(parser, TOKEN_IDENTIFIER, &table_token, "expected table name")) {
        return 0;
    }

    statement.as.insert.table_name = sql_strdup(table_token->lexeme);
    if (statement.as.insert.table_name == NULL) {
        sql_error_set(parser->error, "ERROR: out of memory while copying table name");
        return 0;
    }

    if (!expect_keyword(parser, "VALUES")) {
        free_statement(&statement);
        return 0;
    }

    if (!match_token(parser, TOKEN_LPAREN)) {
        free_statement(&statement);
        return parser_error_at(parser, peek_token(parser), "expected '(' after VALUES");
    }

    while (1) {
        LiteralValue value;

        if (!parse_literal(parser, &value)) {
            free_statement(&statement);
            return 0;
        }

        if (!append_literal(&statement.as.insert, value, parser->error)) {
            free_literal_value(&value);
            free_statement(&statement);
            return 0;
        }

        if (!match_token(parser, TOKEN_COMMA)) {
            break;
        }
    }

    if (statement.as.insert.value_count == 0) {
        free_statement(&statement);
        return parser_error_at(parser, peek_token(parser), "INSERT requires at least one value");
    }

    if (!match_token(parser, TOKEN_RPAREN)) {
        free_statement(&statement);
        return parser_error_at(parser, peek_token(parser), "expected ')' after value list");
    }

    if (!match_token(parser, TOKEN_SEMICOLON)) {
        free_statement(&statement);
        return parser_error_at(parser, peek_token(parser), "expected ';' after INSERT statement");
    }

    *out_statement = statement;
    return 1;
}

/*
 * 목적:
 *   SELECT 문 하나를 파싱한다.
 *
 * 입력:
 *   parser - 현재 파서 상태.
 *   out_statement - 성공 시 결과 statement.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   SELECT 목록, FROM 테이블, optional WHERE, 세미콜론 순서로 읽는다.
 *
 * 초보자 주의:
 *   WHERE는 선택 사항이지만, 있다면 column / operator / literal 세 요소가 모두 있어야 한다.
 */
static int parse_select_statement(Parser *parser, Statement *out_statement) {
    const Token *token;
    Statement statement;

    memset(&statement, 0, sizeof(statement));
    statement.type = STMT_SELECT;

    if (!expect_keyword(parser, "SELECT")) {
        return 0;
    }

    if (match_token(parser, TOKEN_ASTERISK)) {
        statement.as.select.select_all = 1;
    } else {
        while (1) {
            char *column_name;

            if (!expect_token(parser, TOKEN_IDENTIFIER, &token, "expected column name in SELECT list")) {
                free_statement(&statement);
                return 0;
            }

            column_name = sql_strdup(token->lexeme);
            if (column_name == NULL) {
                free_statement(&statement);
                sql_error_set(parser->error, "ERROR: out of memory while copying column name");
                return 0;
            }

            if (!append_selected_column(&statement.as.select, column_name, parser->error)) {
                free_statement(&statement);
                return 0;
            }

            if (!match_token(parser, TOKEN_COMMA)) {
                break;
            }
        }
    }

    if (!expect_keyword(parser, "FROM")) {
        free_statement(&statement);
        return 0;
    }

    if (!expect_token(parser, TOKEN_IDENTIFIER, &token, "expected table name after FROM")) {
        free_statement(&statement);
        return 0;
    }

    statement.as.select.table_name = sql_strdup(token->lexeme);
    if (statement.as.select.table_name == NULL) {
        free_statement(&statement);
        sql_error_set(parser->error, "ERROR: out of memory while copying table name");
        return 0;
    }

    if (match_keyword(parser, "WHERE")) {
        statement.as.select.has_where = 1;

        if (!expect_token(parser, TOKEN_IDENTIFIER, &token, "expected column name after WHERE")) {
            free_statement(&statement);
            return 0;
        }

        statement.as.select.where.column_name = sql_strdup(token->lexeme);
        if (statement.as.select.where.column_name == NULL) {
            free_statement(&statement);
            sql_error_set(parser->error, "ERROR: out of memory while copying WHERE column");
            return 0;
        }

        if (!parse_operator(parser, &statement.as.select.where.op)) {
            free_statement(&statement);
            return 0;
        }

        if (!parse_literal(parser, &statement.as.select.where.value)) {
            free_statement(&statement);
            return 0;
        }
    }

    if (!match_token(parser, TOKEN_SEMICOLON)) {
        free_statement(&statement);
        return parser_error_at(parser, peek_token(parser), "expected ';' after SELECT statement");
    }

    *out_statement = statement;
    return 1;
}

/*
 * 목적:
 *   토큰 배열 전체를 Program 구조체로 파싱한다.
 *
 * 입력:
 *   tokens - tokenizer가 만든 토큰 배열.
 *   out_program - 성공 시 결과 Program.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   EOF 전까지 statement를 하나씩 읽어 Program 배열에 담는다.
 *
 * 초보자 주의:
 *   여러 SQL 문장이 한 파일에 들어올 수 있으므로 while 루프로 끝까지 읽는다.
 */
int parse_program(const TokenArray *tokens, Program *out_program, SqlError *error) {
    Parser parser;

    if (tokens == NULL || out_program == NULL) {
        sql_error_set(error, "ERROR: parser received invalid input");
        return 0;
    }

    memset(out_program, 0, sizeof(*out_program));

    parser.tokens = tokens;
    parser.current = 0;
    parser.error = error;

    while (!is_at_end(&parser)) {
        Statement statement;

        memset(&statement, 0, sizeof(statement));

        if (check_keyword(&parser, "INSERT")) {
            if (!parse_insert_statement(&parser, &statement)) {
                free_program(out_program);
                return 0;
            }
        } else if (check_keyword(&parser, "SELECT")) {
            if (!parse_select_statement(&parser, &statement)) {
                free_program(out_program);
                return 0;
            }
        } else {
            parser_error_at(&parser, peek_token(&parser), "unsupported statement");
            free_program(out_program);
            return 0;
        }

        if (!append_statement(out_program, statement, error)) {
            free_statement(&statement);
            free_program(out_program);
            return 0;
        }
    }

    return 1;
}
