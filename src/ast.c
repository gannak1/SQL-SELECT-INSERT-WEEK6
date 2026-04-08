/*
 * 파일 목적:
 *   parser가 만든 AST 비슷한 구조체들의 해제 함수를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   main은 실행이 끝난 뒤 Program 전체를 free_program으로 정리한다.
 *
 * 주요 내용:
 *   - operator_type_name
 *   - literal / statement / program 해제
 *
 * 초보자 포인트:
 *   중첩된 구조체를 해제할 때는 "가장 안쪽 문자열 -> 배열 -> 바깥 구조체" 순서로 정리한다.
 *
 * 메모리 / 문자열 주의점:
 *   union 안에는 실제 타입에 맞는 필드만 유효하므로,
 *   statement->type을 보고 어느 쪽을 해제할지 결정해야 한다.
 */

#include "ast.h"

#include <stdlib.h>

/*
 * 목적:
 *   OperatorType을 사람이 읽을 수 있는 연산자 문자열로 바꾼다.
 *
 * 입력:
 *   op - 변환할 연산자 enum.
 *
 * 반환:
 *   대응되는 문자열 상수.
 *
 * 동작 요약:
 *   switch 문으로 각 enum에 SQL 연산자 문자열을 대응시킨다.
 *
 * 초보자 주의:
 *   이 함수는 에러 메시지와 Lessons 설명에서 같은 표현을 재사용하는 데 도움이 된다.
 */
const char *operator_type_name(OperatorType op) {
    switch (op) {
        case OP_EQ:
            return "=";
        case OP_NEQ:
            return "!=";
        case OP_LT:
            return "<";
        case OP_GT:
            return ">";
        case OP_LTE:
            return "<=";
        case OP_GTE:
            return ">=";
        default:
            return "?";
    }
}

/*
 * 목적:
 *   LiteralValue 내부에서 TEXT가 할당되어 있으면 해제한다.
 *
 * 입력:
 *   value - 정리할 literal 포인터.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   TEXT literal일 때만 text_value를 free한 뒤 필드를 초기화한다.
 *
 * 초보자 주의:
 *   INT literal은 별도 heap 메모리를 쓰지 않으므로 문자열 해제가 필요 없다.
 */
void free_literal_value(LiteralValue *value) {
    if (value == NULL) {
        return;
    }

    if (value->type == DATA_TYPE_TEXT) {
        free(value->text_value);
    }

    value->text_value = NULL;
    value->int_value = 0;
}

/*
 * 목적:
 *   Statement 하나가 소유한 모든 메모리를 해제한다.
 *
 * 입력:
 *   statement - 정리할 statement 포인터.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   INSERT와 SELECT를 구분해 내부 문자열, 배열, WHERE literal을 순서대로 해제한다.
 *
 * 초보자 주의:
 *   SELECT는 selected_columns와 WHERE 조건 문자열까지 있어 INSERT보다 해제 대상이 많다.
 */
void free_statement(Statement *statement) {
    size_t index;

    if (statement == NULL) {
        return;
    }

    if (statement->type == STMT_INSERT) {
        free(statement->as.insert.table_name);
        for (index = 0; index < statement->as.insert.value_count; ++index) {
            free_literal_value(&statement->as.insert.values[index]);
        }
        free(statement->as.insert.values);
        statement->as.insert.table_name = NULL;
        statement->as.insert.values = NULL;
        statement->as.insert.value_count = 0;
    } else if (statement->type == STMT_SELECT) {
        for (index = 0; index < statement->as.select.selected_column_count; ++index) {
            free(statement->as.select.selected_columns[index]);
        }
        free(statement->as.select.selected_columns);
        free(statement->as.select.table_name);

        if (statement->as.select.has_where) {
            free(statement->as.select.where.column_name);
            free_literal_value(&statement->as.select.where.value);
        }

        statement->as.select.selected_columns = NULL;
        statement->as.select.selected_column_count = 0;
        statement->as.select.table_name = NULL;
        statement->as.select.has_where = 0;
    }
}

/*
 * 목적:
 *   Program 전체와 그 안의 모든 statement를 해제한다.
 *
 * 입력:
 *   program - 정리할 Program 포인터.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   statements 배열을 순회하며 각 statement를 해제한 뒤 배열 자체를 free한다.
 *
 * 초보자 주의:
 *   parser 실패 중간에도 일부 statement만 채워질 수 있으므로,
 *   count 기준으로 순회하는 방식이 안전하다.
 */
void free_program(Program *program) {
    size_t index;

    if (program == NULL) {
        return;
    }

    for (index = 0; index < program->count; ++index) {
        free_statement(&program->statements[index]);
    }

    free(program->statements);
    program->statements = NULL;
    program->count = 0;
    program->capacity = 0;
}
