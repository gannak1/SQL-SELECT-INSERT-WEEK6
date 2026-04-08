#include "storage.h"

#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "utils.h"

static const wchar_t* RESULT_DIRECTORY_PATH = L"C:\\Users\\User\\OneDrive\\바탕 화면\\수요코딩회\\6주차\\result";
static const char* CSV_HEADER = "id,login_id,name,password,created_at\n";
static const char* SCHEMA_COLUMNS[SCHEMA_COLUMN_COUNT] = {
    "id",
    "login_id",
    "name",
    "password",
    "created_at"
};

static int build_table_file_path(const char* table_name, wchar_t* table_file_path, size_t path_size) {
    wchar_t wide_table_name[MAX_ITEM_LENGTH];
    size_t converted = 0;

    if (table_name == NULL || table_name[0] == '\0') {
        return 0;
    }

    if (mbstowcs_s(&converted, wide_table_name, MAX_ITEM_LENGTH, table_name, _TRUNCATE) != 0) {
        return 0;
    }

    return swprintf_s(table_file_path, path_size, L"%ls\\%ls.csv", RESULT_DIRECTORY_PATH, wide_table_name) > 0;
}

static int ensure_result_directory(char* error_message, size_t error_size) {
    int make_result = _wmkdir(RESULT_DIRECTORY_PATH);

    if (make_result == 0 || errno == EEXIST) {
        return 1;
    }

    snprintf(error_message, error_size, "result 폴더를 만들 수 없습니다.");
    return 0;
}

static int ensure_table_file(const char* table_name, int create_if_missing, char* error_message, size_t error_size) {
    FILE* file = NULL;
    FILE* create_file = NULL;
    wchar_t table_file_path[MAX_INPUT_LENGTH];

    if (!ensure_result_directory(error_message, error_size)) {
        return 0;
    }

    if (!build_table_file_path(table_name, table_file_path, MAX_INPUT_LENGTH)) {
        snprintf(error_message, error_size, "유효하지 않은 테이블 이름입니다.");
        return 0;
    }

    if (_wfopen_s(&file, table_file_path, L"r") == 0 && file != NULL) {
        fclose(file);
        return 1;
    }

    if (!create_if_missing) {
        snprintf(error_message, error_size, "테이블 파일이 존재하지 않습니다: %s.csv", table_name);
        return 0;
    }

    if (_wfopen_s(&create_file, table_file_path, L"w") != 0 || create_file == NULL) {
        snprintf(error_message, error_size, "%s.csv 파일을 생성할 수 없습니다.", table_name);
        return 0;
    }

    fputs(CSV_HEADER, create_file);
    fclose(create_file);
    return 1;
}

static void write_csv_field(FILE* file, const char* value) {
    const char* cursor = value;
    int needs_quotes = 0;

    while (*cursor != '\0') {
        if (*cursor == ',' || *cursor == '"' || *cursor == '\n' || *cursor == '\r') {
            needs_quotes = 1;
            break;
        }
        cursor++;
    }

    if (!needs_quotes) {
        fputs(value, file);
        return;
    }

    fputc('"', file);
    cursor = value;

    while (*cursor != '\0') {
        if (*cursor == '"') {
            fputc('"', file);
        }
        fputc(*cursor, file);
        cursor++;
    }

    fputc('"', file);
}

static int find_schema_column_index(const char* column_name) {
    int index = 0;

    for (index = 0; index < SCHEMA_COLUMN_COUNT; ++index) {
        if (equals_ignore_case(column_name, SCHEMA_COLUMNS[index])) {
            return index;
        }
    }

    return -1;
}

static int parse_csv_line(const char* line, char fields[][MAX_ITEM_LENGTH], int max_fields, int* field_count) {
    const char* cursor = line;
    int current_field = 0;
    int current_length = 0;
    int in_quotes = 0;

    memset(fields, 0, sizeof(char[MAX_ITEM_COUNT][MAX_ITEM_LENGTH]));
    *field_count = 0;

    if (max_fields <= 0) {
        return 0;
    }

    while (*cursor != '\0' && *cursor != '\n' && *cursor != '\r') {
        if (current_field >= max_fields) {
            return 0;
        }

        if (in_quotes) {
            if (*cursor == '"' && cursor[1] == '"') {
                if (current_length + 1 >= MAX_ITEM_LENGTH) {
                    return 0;
                }
                fields[current_field][current_length++] = '"';
                cursor += 2;
                continue;
            }

            if (*cursor == '"') {
                in_quotes = 0;
                cursor++;
                continue;
            }
        } else {
            if (*cursor == '"') {
                in_quotes = 1;
                cursor++;
                continue;
            }

            if (*cursor == ',') {
                fields[current_field][current_length] = '\0';
                current_field++;
                current_length = 0;
                cursor++;
                continue;
            }
        }

        if (current_length + 1 >= MAX_ITEM_LENGTH) {
            return 0;
        }

        fields[current_field][current_length++] = *cursor;
        cursor++;
    }

    fields[current_field][current_length] = '\0';
    *field_count = current_field + 1;
    return 1;
}

static int build_selected_columns(
    const Query* query,
    int selected_indexes[],
    int* selected_count,
    char* error_message,
    size_t error_size
) {
    int index = 0;

    if (query->type != QUERY_SELECT) {
        snprintf(error_message, error_size, "현재 SELECT 명령만 조회할 수 있습니다.");
        return 0;
    }

    if (query->item_count == 1 && strcmp(query->items[0], "*") == 0) {
        for (index = 0; index < SCHEMA_COLUMN_COUNT; ++index) {
            selected_indexes[index] = index;
        }
        *selected_count = SCHEMA_COLUMN_COUNT;
        return 1;
    }

    if (query->item_count > SCHEMA_COLUMN_COUNT) {
        snprintf(error_message, error_size, "SELECT 컬럼 개수가 스키마보다 많습니다.");
        return 0;
    }

    for (index = 0; index < query->item_count; ++index) {
        int column_index = find_schema_column_index(query->items[index]);

        if (column_index < 0) {
            snprintf(error_message, error_size, "존재하지 않는 컬럼입니다: %s", query->items[index]);
            return 0;
        }

        selected_indexes[index] = column_index;
    }

    *selected_count = query->item_count;
    return 1;
}

int append_insert_to_csv(const InsertCommand* insert_command, char* error_message, size_t error_size) {
    FILE* file = NULL;
    int index = 0;
    wchar_t table_file_path[MAX_INPUT_LENGTH];

    if (!ensure_table_file("table", 1, error_message, error_size)) {
        return 0;
    }

    if (!build_table_file_path("table", table_file_path, MAX_INPUT_LENGTH)) {
        snprintf(error_message, error_size, "table.csv 경로를 만들 수 없습니다.");
        return 0;
    }

    if (_wfopen_s(&file, table_file_path, L"a") != 0 || file == NULL) {
        snprintf(error_message, error_size, "table.csv 파일을 열 수 없습니다.");
        return 0;
    }

    for (index = 0; index < SCHEMA_COLUMN_COUNT; ++index) {
        if (index > 0) {
            fputc(',', file);
        }
        write_csv_field(file, insert_command->values[index].text);
    }

    fputc('\n', file);
    fclose(file);
    return 1;
}

static int evaluate_where_condition(const Query* query, const char* field_text) {
    if (query->where_value.type == VALUE_BOOLEAN) {
        if (query->where_operator != OPERATOR_EQUAL) {
            return 0;
        }
        return equals_ignore_case(field_text, query->where_value.text);
    }

    if (query->where_value.type == VALUE_INTEGER) {
        long field_number = strtol(field_text, NULL, 10);
        long where_number = strtol(query->where_value.text, NULL, 10);

        switch (query->where_operator) {
        case OPERATOR_EQUAL:
            return field_number == where_number;
        case OPERATOR_GREATER:
            return field_number > where_number;
        case OPERATOR_LESS:
            return field_number < where_number;
        default:
            return 0;
        }
    }

    switch (query->where_operator) {
    case OPERATOR_EQUAL:
        return strcmp(field_text, query->where_value.text) == 0;
    case OPERATOR_GREATER:
        return strcmp(field_text, query->where_value.text) > 0;
    case OPERATOR_LESS:
        return strcmp(field_text, query->where_value.text) < 0;
    default:
        return 0;
    }
}

// SELECT 쿼리 실행. CSV 파일 읽어서 조건에 맞는 행 출력
int execute_select_query(const Query* query, char* error_message, size_t error_size) {
    // 선택된 컬럼의 스키마 인덱스 배열 ex) name=2, id=0
    int selected_indexes[SCHEMA_COLUMN_COUNT];
    // 선택된 컬럼 개수
    int selected_count = 0;
    FILE* file = NULL;
    // CSV 한 줄 읽기용 버퍼
    char line[MAX_INPUT_LENGTH];
    // CSV 한 줄을 콤마 기준으로 분리한 필드 배열
    char fields[MAX_ITEM_COUNT][MAX_ITEM_LENGTH];
    int field_count = 0;
    int row_count = 0;
    int index = 0;
    // WHERE 컬럼의 스키마 인덱스. WHERE 없으면 -1
    int where_column_index = -1;
    wchar_t table_file_path[MAX_INPUT_LENGTH];

    // query->items를 스키마 인덱스로 변환. * 이면 전체 선택
    if (!build_selected_columns(query, selected_indexes, &selected_count, error_message, error_size)) {
        return 0;
    }

    // FROM 테이블명 없으면 리턴
    if (query->table_name[0] == '\0') {
        snprintf(error_message, error_size, "SELECT에는 FROM table 구문이 필요합니다.");
        return 0;
    }

    // WHERE 있으면 해당 컬럼이 스키마에 존재하는지 확인
    if (query->has_where) {
        where_column_index = find_schema_column_index(query->where_column);
        // 스키마에 없는 컬럼이면 리턴
        if (where_column_index < 0) {
            snprintf(error_message, error_size, "WHERE 컬럼이 존재하지 않습니다: %s", query->where_column);
            return 0;
        }
    }

    // CSV 파일 존재 여부 확인. 없으면 에러 (INSERT와 달리 생성 안함)
    if (!ensure_table_file(query->table_name, 0, error_message, error_size)) {
        return 0;
    }

    // 테이블명으로 CSV 전체 경로 생성
    if (!build_table_file_path(query->table_name, table_file_path, MAX_INPUT_LENGTH)) {
        snprintf(error_message, error_size, "테이블 경로를 만들 수 없습니다.");
        return 0;
    }

    // CSV 파일 읽기 모드로 열기
    if (_wfopen_s(&file, table_file_path, L"r") != 0 || file == NULL) {
        snprintf(error_message, error_size, "%s.csv 파일을 읽을 수 없습니다.", query->table_name);
        return 0;
    }

    printf("\n=== Select Result ===\n");

    // 선택된 컬럼명 헤더 출력. | 구분자로 연결
    for (index = 0; index < selected_count; ++index) {
        if (index > 0) {
            printf(" | ");
        }
        printf("%s", SCHEMA_COLUMNS[selected_indexes[index]]);
    }
    printf("\n");

    // 첫 줄은 CSV 헤더(id,login_id,name...)니까 skip
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        printf("(empty)\n");
        return 1;
    }

    // 한 줄씩 읽으면서 처리
    while (fgets(line, sizeof(line), file) != NULL) {
        // 한 줄을 콤마 기준으로 fields[] 배열로 분리
        if (!parse_csv_line(line, fields, MAX_ITEM_COUNT, &field_count)) {
            fclose(file);
            snprintf(error_message, error_size, "CSV 데이터 형식이 올바르지 않습니다.");
            return 0;
        }

        // 분리된 필드 개수가 스키마 컬럼 개수보다 적으면 잘못된 데이터
        if (field_count < SCHEMA_COLUMN_COUNT) {
            fclose(file);
            snprintf(error_message, error_size, "CSV 컬럼 개수가 부족합니다.");
            return 0;
        }

        // WHERE 조건 있으면 해당 행 필터링. 조건 안맞으면 skip
        if (query->has_where && !evaluate_where_condition(query, fields[where_column_index])) {
            continue;
        }

        // 선택된 컬럼값만 | 구분자로 출력
        for (index = 0; index < selected_count; ++index) {
            if (index > 0) {
                printf(" | ");
            }
            printf("%s", fields[selected_indexes[index]]);
        }
        printf("\n");
        row_count++;
    }

    fclose(file);

    // 조건에 맞는 행이 하나도 없으면 empty 출력
    if (row_count == 0) {
        printf("(empty)\n");
    }

    return 1;
}