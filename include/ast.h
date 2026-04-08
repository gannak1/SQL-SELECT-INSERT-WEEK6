/*
 * 파일 목적:
 *   parser가 만드는 구조화된 statement와 literal, condition 자료구조를 정의한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer가 만든 토큰을 parser가 읽어 AST 비슷한 Program 구조로 만들고,
 *   executor는 그 Program을 해석해 실제 CSV 작업을 수행한다.
 *
 * 주요 내용:
 *   - LiteralValue, Condition, InsertStatement, SelectStatement
 *   - Statement, Program
 *   - 해제 함수
 *
 * 초보자 포인트:
 *   AST를 두면 "SELECT의 컬럼 목록", "WHERE 조건", "INSERT 값 목록"이
 *   명시적인 구조체로 남아 실행 단계가 쉬워진다.
 *
 * 메모리 / 문자열 주의점:
 *   parser가 문자열과 배열을 복사해 담기 때문에,
 *   Program 전체를 다 쓴 뒤 free_program으로 한 번에 정리해야 한다.
 */

#ifndef AST_H
#define AST_H

#include <stddef.h>

#include "common.h"

typedef struct {
    DataType type;
    long int_value;
    char *text_value;
} LiteralValue;

typedef enum {
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE
} OperatorType;

typedef struct {
    char *column_name;
    OperatorType op;
    LiteralValue value;
} Condition;

typedef struct {
    char *table_name;
    LiteralValue *values;
    size_t value_count;
} InsertStatement;

typedef struct {
    int select_all;
    char **selected_columns;
    size_t selected_column_count;
    char *table_name;
    int has_where;
    Condition where;
} SelectStatement;

typedef enum {
    STMT_INSERT,
    STMT_SELECT
} StatementType;

typedef struct {
    StatementType type;
    union {
        InsertStatement insert;
        SelectStatement select;
    } as;
} Statement;

typedef struct {
    Statement *statements;
    size_t count;
    size_t capacity;
} Program;

const char *operator_type_name(OperatorType op);
void free_literal_value(LiteralValue *value);
void free_statement(Statement *statement);
void free_program(Program *program);

#endif
