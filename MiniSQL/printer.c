#include "printer.h"

#include <stdio.h>

// enum 값을 사람이 읽기 쉬운 문자열로 바꿔 출력할 때 사용한다.
const char* query_type_to_string(QueryType type) {
    switch (type) {
    case QUERY_SELECT:
        return "SELECT";
    case QUERY_INSERT:
        return "INSERT";
    default:
        return "UNKNOWN";
    }
}

// INSERT 값의 세부 타입을 사람이 읽기 쉽게 보여준다.
const char* value_type_to_string(ValueType type) {
    switch (type) {
    case VALUE_STRING:
        return "STRING";
    case VALUE_INTEGER:
        return "INTEGER";
    case VALUE_BOOLEAN:
        return "BOOLEAN";
    default:
        return "UNKNOWN";
    }
}

// WHERE 비교 연산자를 사람이 읽기 쉬운 문자열로 바꾼다.
static const char* comparison_operator_to_string(ComparisonOperator where_operator) {
    switch (where_operator) {
    case OPERATOR_EQUAL:
        return "=";
    case OPERATOR_GREATER:
        return ">";
    case OPERATOR_LESS:
        return "<";
    default:
        return "?";
    }
}

// 파싱 결과를 콘솔에서 바로 확인할 수 있게 보기 좋게 출력한다.
void print_query(const Query* query) {
    int i = 0;

    printf("\n=== Parse Result ===\n");
    printf("Raw Input   : %s\n", query->raw_input);
    printf("Type        : %s\n", query_type_to_string(query->type));
    printf("Command     : %s\n", query->command);
    if (query->table_name[0] != '\0') {
        printf("Table       : %s\n", query->table_name);
    }
    printf("Item Count  : %d\n", query->item_count);
    printf("Items       :\n");

    for (i = 0; i < query->item_count; ++i) {
        printf("  [%d] %s\n", i + 1, query->items[i]);
    }

    if (query->item_count == 0) {
        printf("  (none)\n");
    }

    if (query->has_where) {
        printf(
            "Where       : %s %s %s\n",
            query->where_column,
            comparison_operator_to_string(query->where_operator),
            query->where_value.text
        );
    }
}

// INSERT가 실제 저장되기 직전 어떤 값으로 정리됐는지 출력한다.
void print_insert_command(const InsertCommand* insert_command) {
    int index = 0;
    const char* column_names[SCHEMA_COLUMN_COUNT] = {
        "id",
        "login_id",
        "name",
        "password",
        "created_at"
    };

    printf("\n=== Insert Values ===\n");
    for (index = 0; index < SCHEMA_COLUMN_COUNT; ++index) {
        printf(
            "  [%d] %s = %s (%s)\n",
            index + 1,
            column_names[index],
            insert_command->values[index].text,
            value_type_to_string(insert_command->values[index].type)
        );
    }
}
