#ifndef TYPES_H
#define TYPES_H

#define MAX_INPUT_LENGTH 512
#define MAX_ITEM_COUNT 32
#define MAX_ITEM_LENGTH 128
#define SCHEMA_COLUMN_COUNT 5

typedef enum QueryType {
    QUERY_UNKNOWN,
    QUERY_SELECT,
    QUERY_INSERT
} QueryType;

typedef enum ValueType {
    VALUE_STRING,
    VALUE_INTEGER,
    VALUE_BOOLEAN
} ValueType;

typedef enum ComparisonOperator {
    OPERATOR_EQUAL,
    OPERATOR_GREATER,
    OPERATOR_LESS
} ComparisonOperator;

typedef struct ParsedValue {
    ValueType type;
    char raw[MAX_ITEM_LENGTH];
    char text[MAX_ITEM_LENGTH];
} ParsedValue;

typedef struct Query {
    QueryType type;
    char raw_input[MAX_INPUT_LENGTH]; // 명령 요청 쿼리
    char command[MAX_ITEM_LENGTH]; // SELECT or INSERT
    char table_name[MAX_ITEM_LENGTH];
    char items[MAX_ITEM_COUNT][MAX_ITEM_LENGTH];
    int item_count;
    int has_where;
    char where_column[MAX_ITEM_LENGTH];
    ComparisonOperator where_operator;
    ParsedValue where_value;
} Query;

typedef struct InsertCommand {
    ParsedValue values[SCHEMA_COLUMN_COUNT];
} InsertCommand;

#endif