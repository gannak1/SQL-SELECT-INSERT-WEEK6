#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btree.h"
#include "utils.h"

static int append_field_separator(FILE *fp, int index);
static int ensure_user_index_loaded(void);
static int reset_user_index(void);
static int rebuild_user_index(FILE *fp);
static int get_next_user_id(int *next_id_out);
static int read_dynamic_line(FILE *fp, char **line_out);
static int append_row(RowArray *row_array, char *row);
static char *duplicate_string(const char *value);
static char *duplicate_range(const char *start, size_t len);
static int write_index_snapshot(void);
static int grow_buffer(char **buffer, size_t *capacity);

/* g_user_index는 프로그램 실행 중 메모리에만 유지되는 B-tree 인덱스다. */
static BTreeIndex g_user_index;
/* g_index_loaded는
   "users.csv를 읽어서 B-tree를 이미 만들어 두었는가?"를 기억한다. */
static int g_index_loaded = 0;

/* storage_shutdown()은 프로그램 종료 직전에
   storage 계층이 들고 있던 전역 B-tree 메모리를 정리하는 함수다. */
void storage_shutdown(void) {
    /* 프로그램 종료 직전에 메모리 인덱스를 명시적으로 정리한다. */
    btree_free(&g_user_index);
    g_index_loaded = 0;
}

/* append_user_record()는 InsertCommand 값을 받아
   새 auto-increment id를 만들고 CSV 파일 끝에 한 행을 추가한다. */
int append_user_record(const InsertCommand *insert_cmd) {
    /* INSERT 데이터를 CSV 한 줄로 바꿔 파일 끝에 추가한다.
       쓰기 전에 B-tree 인덱스를 준비해 두고,
       새 id는 기존 최대 id + 1 규칙으로 자동 생성한다. */
    FILE *fp;
    /* row_offset은 "이번 행이 파일의 몇 번째 바이트 위치에서 시작하는지"를 저장한다.
       나중에 B-tree의 value로 넣어서 id -> 파일 위치 검색에 사용한다. */
    long row_offset;
    int id;
    int status;

    if (insert_cmd == NULL) {
        return -1;
    }

    if (ensure_user_index_loaded() != 0) {
        return -1;
    }

    if (get_next_user_id(&id) != 0) {
        return -1;
    }
    /* 여기서 만들어진 id가 이번 INSERT의 실제 사용자 번호가 된다. */

    fp = fopen(DATA_FILE_PATH, "a+");
    if (fp == NULL) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    row_offset = ftell(fp);
    if (row_offset < 0) {
        fclose(fp);
        return -1;
    }
    /* 아직 쓰기 전 위치이므로,
       이 값이 곧 "새 행의 시작 위치"가 된다. */

    /* 저장되는 실제 CSV 순서는 id, username, name, age, phone, email 이다. */
    if (append_field_separator(fp, 0) != 0) {
        fclose(fp);
        return -1;
    }
    fprintf(fp, "%d", id);

    if (append_field_separator(fp, 1) != 0) {
        fclose(fp);
        return -1;
    }
    write_csv_field(fp, insert_cmd->values[0], 1);

    if (append_field_separator(fp, 2) != 0) {
        fclose(fp);
        return -1;
    }
    write_csv_field(fp, insert_cmd->values[1], 1);

    if (append_field_separator(fp, 3) != 0) {
        fclose(fp);
        return -1;
    }
    write_csv_field(fp, insert_cmd->values[2], 0);

    if (append_field_separator(fp, 4) != 0) {
        fclose(fp);
        return -1;
    }
    write_csv_field(fp, insert_cmd->values[3], 1);

    if (append_field_separator(fp, 5) != 0) {
        fclose(fp);
        return -1;
    }
    write_csv_field(fp, insert_cmd->values[4], 1);

    /* 한 행 기록이 끝났으므로 줄바꿈 추가 */
    if (fputc('\n', fp) == EOF) {
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        return -1;
    }

    status = btree_insert(&g_user_index, id, row_offset);
    if (status != 0) {
        return -1;
    }

    if (write_index_snapshot() != 0) {
        return -1;
    }

    return 0;
}

/* read_all_users()는 users.csv 전체를 읽어서
   동적 RowArray 구조에 한 줄씩 담아 executor 쪽에 넘긴다. */
int read_all_users(RowArray *row_array) {
    /* CSV 파일 전체를 한 줄씩 읽어 동적 배열에 담는다. */
    FILE *fp;
    /* line은 파일에서 막 읽은 "한 줄 문자열"을 가리킨다.
       append_row()에 넘기기 전까지는 이 함수가 소유한다. */
    char *line = NULL;
    int status;

    if (row_array == NULL) {
        return -1;
    }

    row_array->rows = NULL;
    row_array->count = 0;
    row_array->capacity = 0;

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return -1;
    }

    while ((status = read_dynamic_line(fp, &line)) > 0) {
        if (is_blank_string(line)) {
            free(line);
            line = NULL;
            continue;
        }

        if (append_row(row_array, line) != 0) {
            free(line);
            fclose(fp);
            free_row_array(row_array);
            return -1;
        }

        /* append_row()가 line 포인터를 row_array 안으로 넘겨받았으므로
           다음 반복에서는 새 줄용 포인터를 다시 받아야 한다. */
        line = NULL;
    }

    if (status < 0 || ferror(fp)) {
        fclose(fp);
        free_row_array(row_array);
        return -1;
    }

    fclose(fp);
    return 0;
}

/* read_user_row_by_id()는 B-tree 인덱스를 이용해
   특정 id가 있는 파일 위치를 찾고 그 한 줄만 읽어 온다. */
int read_user_row_by_id(int id, char **row_out) {
    /* B-tree 인덱스를 통해 id가 가리키는 파일 오프셋을 찾고,
       해당 한 줄만 직접 읽어온다. */
    FILE *fp;
    /* row_offset은 B-tree가 찾아준 "이 id 행의 파일 시작 위치"다. */
    long row_offset;
    int found;
    int status;

    if (row_out == NULL) {
        return -1;
    }

    *row_out = NULL;

    if (ensure_user_index_loaded() != 0) {
        return -1;
    }

    found = btree_search(&g_user_index, id, &row_offset);
    if (!found) {
        /* 1은 "읽기 실패"가 아니라 "그 id가 없음"이라는 뜻으로 사용한다. */
        return 1;
    }

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        return -1;
    }

    if (fseek(fp, row_offset, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    status = read_dynamic_line(fp, row_out);
    fclose(fp);

    if (status <= 0) {
        free(*row_out);
        *row_out = NULL;
        return -1;
    }

    return 0;
}

/* split_csv_row()는 CSV 한 줄을 id, username, name, age, phone, email
   6개 칼럼 문자열로 분해하는 함수다. */
int split_csv_row(const char *row, char *values[USER_COLUMN_COUNT]) {
    /* CSV 한 줄을 6개 칼럼으로 나눈다.
       큰따옴표 안의 쉼표는 데이터로 취급해야 하므로
       in_quotes 상태를 같이 관리한다. */
    const char *cursor;
    /* column은 현재 몇 번째 CSV 칼럼을 만들고 있는지 나타낸다. */
    int column = 0;
    /* in_quotes가 1이면 지금 큰따옴표 문자열 안에 있다는 뜻이다.
       이 상태에서는 쉼표를 만나도 칼럼 종료로 보면 안 된다. */
    int in_quotes = 0;
    /* field는 "현재 칼럼 하나를 임시로 모으는 버퍼"다. */
    char *field;
    size_t field_cap = INITIAL_BUFFER_CAPACITY;
    /* field_len은 field 안에 실제로 몇 글자를 채웠는지 센다. */
    size_t field_len = 0;

    if (row == NULL || values == NULL) {
        return -1;
    }

    for (column = 0; column < USER_COLUMN_COUNT; column++) {
        values[column] = NULL;
    }

    field = (char *) malloc(field_cap);
    if (field == NULL) {
        return -1;
    }

    cursor = row;
    /* 본격적으로 row 문자열을 처음부터 스캔하기 시작한다. */
    column = 0;

    while (*cursor != '\0' && column < USER_COLUMN_COUNT) {
        /* 따옴표 밖의 쉼표는 "다음 칼럼으로 이동" 의미다. */
        if (!in_quotes && *cursor == ',') {
            field[field_len] = '\0';
            /* 지금까지 모은 field를 values[column]에 복사해 두고
               다음 칼럼을 위해 column을 1 증가시킨다. */
            values[column] = duplicate_string(field);
            if (values[column] == NULL) {
                free(field);
                free_csv_values(values);
                return -1;
            }
            column++;
            /* 새 칼럼을 시작하므로 임시 버퍼 길이도 0으로 초기화한다. */
            field_len = 0;
            cursor++;
            continue;
        }

        /* 큰따옴표는
           - 문자열 시작/끝 표시
           - "" 형태면 실제 따옴표 문자
           를 구분해서 처리해야 한다. */
        if (*cursor == '"') {
            if (in_quotes && cursor[1] == '"') {
                if (field_len + 2 > field_cap && grow_buffer(&field, &field_cap) != 0) {
                    free(field);
                    free_csv_values(values);
                    return -1;
                }
                field[field_len++] = '"';
                cursor += 2;
                continue;
            }

            /* 일반 큰따옴표는 in_quotes 상태를 반전시킨다. */
            in_quotes = !in_quotes;
            /* 큰따옴표 기호 자체는 결과 문자열에 저장하지 않는다. */
            cursor++;
            continue;
        }

        /* 일반 문자는 현재 칼럼 버퍼에 그대로 쌓는다. */
        if (field_len + 2 > field_cap && grow_buffer(&field, &field_cap) != 0) {
            free(field);
            free_csv_values(values);
            return -1;
        }

        field[field_len++] = *cursor;
        cursor++;
    }

    /* 문자열 따옴표가 안 닫힌 상태로 끝났으면 잘못된 CSV */
    if (in_quotes) {
        free(field);
        free_csv_values(values);
        return -1;
    }

    field[field_len] = '\0';
    if (column >= USER_COLUMN_COUNT) {
        free(field);
        free_csv_values(values);
        return -1;
    }

    /* 마지막 칼럼도 문자열 종료 표시 */
    values[column] = duplicate_string(field);
    free(field);
    if (values[column] == NULL) {
        free_csv_values(values);
        return -1;
    }
    column++;

    /* users 행은 정확히 6개 칼럼이어야 한다. */
    if (column != USER_COLUMN_COUNT) {
        free_csv_values(values);
        return -1;
    }

    return 0;
}

/* row_matches_condition()은 이미 분해된 CSV 칼럼 배열이
   WHERE column = value 조건과 맞는지 검사한다. */
int row_matches_condition(char *const values[USER_COLUMN_COUNT], const Condition *condition) {
    /* 칼럼 이름을 보고 실제 CSV 배열 위치와 비교한다. */
    if (values == NULL || condition == NULL || !condition->has_condition || condition->value == NULL) {
        return 0;
    }

    if (strcmp(condition->column, "id") == 0) {
        /* id는 CSV 0번째 칼럼 */
        return strcmp(values[0], condition->value) == 0;
    }

    if (strcmp(condition->column, "username") == 0) {
        /* username은 CSV 1번째 칼럼 */
        return strcmp(values[1], condition->value) == 0;
    }

    if (strcmp(condition->column, "name") == 0) {
        /* name은 CSV 2번째 칼럼 */
        return strcmp(values[2], condition->value) == 0;
    }

    if (strcmp(condition->column, "age") == 0) {
        /* age는 CSV 3번째 칼럼 */
        return strcmp(values[3], condition->value) == 0;
    }

    if (strcmp(condition->column, "phone") == 0) {
        /* phone은 CSV 4번째 칼럼 */
        return strcmp(values[4], condition->value) == 0;
    }

    if (strcmp(condition->column, "email") == 0) {
        /* email은 CSV 5번째 칼럼 */
        return strcmp(values[5], condition->value) == 0;
    }

    return 0;
}

/* write_csv_field()는 값 하나를 CSV 규칙에 맞게 파일에 쓰는 함수다.
   문자열이면 큰따옴표와 이스케이프를 처리하고,
   숫자면 그대로 쓴다. */
void write_csv_field(FILE *fp, const char *value, int quote) {
    /* 문자열 안에 " 가 있으면 CSV 규칙에 맞게 "" 로 두 번 써야 한다. */
    const char *cursor;

    if (fp == NULL || value == NULL) {
        return;
    }

    /* 숫자 컬럼은 따옴표 없이 그대로 쓴다. */
    if (!quote) {
        fputs(value, fp);
        return;
    }

    /* 문자열 컬럼은 큰따옴표로 감싸서 CSV 규칙에 맞게 저장한다. */
    fputc('"', fp);
    cursor = value;
    while (*cursor != '\0') {
        /* 실제 따옴표 문자는 "" 로 두 번 써서 저장해야 한다. */
        if (*cursor == '"') {
            fputc('"', fp);
        }
        fputc(*cursor, fp);
        cursor++;
    }
    fputc('"', fp);
}

/* free_row_array()는 read_all_users()가 만든
   동적 행 목록 전체를 한 번에 해제한다. */
void free_row_array(RowArray *row_array) {
    /* i는 현재 해제 중인 행 번호다. */
    int i;

    if (row_array == NULL) {
        return;
    }

    for (i = 0; i < row_array->count; i++) {
        /* rows[i] 하나가 CSV 한 줄 문자열 1개다. */
        free(row_array->rows[i]);
    }
    /* 행 포인터 배열 자체도 heap 메모리이므로 마지막에 한 번 더 free한다. */
    free(row_array->rows);
    row_array->rows = NULL;
    row_array->count = 0;
    row_array->capacity = 0;
}

/* free_csv_values()는 split_csv_row()가 만들어 둔
   칼럼별 heap 문자열들을 모두 해제한다. */
void free_csv_values(char *values[USER_COLUMN_COUNT]) {
    /* i는 id, username, name ... 각 칼럼 슬롯을 순회한다. */
    int i;

    if (values == NULL) {
        return;
    }

    for (i = 0; i < USER_COLUMN_COUNT; i++) {
        /* split_csv_row()가 칼럼마다 malloc 했던 문자열을 해제한다. */
        free(values[i]);
        values[i] = NULL;
    }
}

/* append_field_separator()는 CSV 출력 중
   첫 칼럼을 제외한 나머지 칼럼 앞에 쉼표를 넣기 위한 helper다. */
static int append_field_separator(FILE *fp, int index) {
    /* 첫 칼럼 앞에는 쉼표가 없고, 그 다음 칼럼부터 쉼표를 넣는다. */
    if (index > 0 && fputc(',', fp) == EOF) {
        return -1;
    }
    return 0;
}

/* ensure_user_index_loaded()는 메모리 B-tree가 아직 준비되지 않았다면
   users.csv를 읽어 한 번 인덱스를 재구성하는 함수다. */
static int ensure_user_index_loaded(void) {
    FILE *fp;

    if (g_index_loaded) {
        /* 이미 한 번 인덱스를 만들었다면 같은 실행 중에는 재구성하지 않는다. */
        return 0;
    }

    if (reset_user_index() != 0) {
        return -1;
    }

    fp = fopen(DATA_FILE_PATH, "r");
    if (fp == NULL) {
        /* 데이터 파일이 아직 없어도 "빈 인덱스" 상태로 시작할 수 있다. */
        g_index_loaded = 1;
        return 0;
    }

    if (rebuild_user_index(fp) != 0) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    g_index_loaded = 1;
    return 0;
}

/* reset_user_index()는 기존 B-tree 내용을 비우고
   완전히 빈 인덱스로 다시 시작하게 만든다. */
static int reset_user_index(void) {
    /* 기존 B-tree를 버리고 완전히 빈 트리로 다시 초기화한다. */
    btree_free(&g_user_index);
    btree_init(&g_user_index);
    return 0;
}

/* rebuild_user_index()는 users.csv를 처음부터 끝까지 읽어
   id -> 파일 위치 관계를 메모리 B-tree에 다시 쌓는 함수다. */
static int rebuild_user_index(FILE *fp) {
    /* line은 파일에서 읽은 한 줄,
       values는 그 한 줄을 CSV 칼럼으로 분리한 결과다. */
    char *line = NULL;
    char *values[USER_COLUMN_COUNT];
    long row_offset;
    int status;
    int id;

    while (1) {
        row_offset = ftell(fp);
        if (row_offset < 0) {
            return -1;
        }
        /* 다음에 읽을 줄이 바로 여기서 시작하므로,
           지금 ftell 위치를 그 행의 파일 오프셋으로 저장한다. */

        status = read_dynamic_line(fp, &line);
        if (status == 0) {
            return 0;
        }
        if (status < 0) {
            return -1;
        }

        if (is_blank_string(line)) {
            free(line);
            line = NULL;
            continue;
        }

        if (split_csv_row(line, values) != 0) {
            free(line);
            return -1;
        }

        /* users.csv 첫 칼럼 values[0]이 id 문자열이다.
           이것을 정수로 바꾸어 B-tree key로 사용한다. */
        id = atoi(values[0]);
        /* CSV 첫 칼럼이 id이므로 그것을 정수로 바꿔 인덱스 key로 사용한다. */
        free_csv_values(values);
        free(line);
        line = NULL;

        /* value로는 그 행의 파일 시작 위치 row_offset을 저장한다.
           나중에 id 조회 시 해당 위치로 바로 seek할 수 있게 된다. */
        status = btree_insert(&g_user_index, id, row_offset);
        if (status < 0) {
            return -1;
        }
    }
}

/* get_next_user_id()는 현재 인덱스에서 가장 큰 id를 찾아
   다음 INSERT에 사용할 새 id를 계산한다. */
static int get_next_user_id(int *next_id_out) {
    /* max_id는 현재 인덱스 안에서 가장 큰 id를 담는다. */
    int max_id;

    if (next_id_out == NULL) {
        return -1;
    }

    if (ensure_user_index_loaded() != 0) {
        return -1;
    }
    /* 여기까지 오면 g_user_index 안에는
       현재 CSV 파일 상태를 반영한 id 목록이 들어 있다고 볼 수 있다. */

    if (!btree_get_max(&g_user_index, &max_id, NULL)) {
        /* 데이터가 하나도 없다면 첫 id는 1로 시작한다. */
        *next_id_out = 1;
        return 0;
    }

    /* 이미 최대 id가 있다면 그 다음 번호를 새 id로 사용한다. */
    *next_id_out = max_id + 1;
    return 0;
}

/* read_dynamic_line()은 파일에서 한 줄을 읽어
   길이에 맞는 heap 문자열로 돌려주는 함수다. */
static int read_dynamic_line(FILE *fp, char **line_out) {
    int ch;
    /* buffer는 파일에서 읽은 한 줄을 담는 heap 메모리다. */
    char *buffer;
    size_t capacity = INITIAL_BUFFER_CAPACITY;
    /* length는 buffer에 실제로 채운 글자 수다. */
    size_t length = 0;

    if (fp == NULL || line_out == NULL) {
        return -1;
    }

    *line_out = NULL;
    /* 한 줄을 담을 시작 버퍼를 먼저 만든다.
       줄이 길어지면 아래 while 안에서 grow_buffer()로 더 늘린다. */
    buffer = (char *) malloc(capacity);
    if (buffer == NULL) {
        return -1;
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (length + 2 > capacity && grow_buffer(&buffer, &capacity) != 0) {
            /* 새 문자 1개와 마지막 '\0' 자리가 모두 필요하므로 +2를 본다. */
            free(buffer);
            return -1;
        }

        if (ch == '\n') {
            /* 줄바꿈은 "한 줄 끝" 의미이므로 buffer에는 넣지 않고 종료한다. */
            break;
        }

        if (ch != '\r') {
            /* 실제 데이터 문자만 저장하고 CR은 버린다. */
            buffer[length++] = (char) ch;
        }
    }

    if (ch == EOF && length == 0) {
        free(buffer);
        return 0;
    }

    /* C 문자열로 사용하려면 마지막에 반드시 '\0'가 있어야 한다. */
    buffer[length] = '\0';
    *line_out = buffer;
    return 1;
}

/* append_row()는 RowArray 끝에 새 행 문자열 하나를 추가하고,
   필요하면 행 포인터 배열을 realloc으로 확장한다. */
static int append_row(RowArray *row_array, char *row) {
    char **new_rows;
    /* new_capacity는 행 배열을 늘릴 때 사용할 새 크기다. */
    int new_capacity;

    if (row_array == NULL || row == NULL) {
        return -1;
    }

    if (row_array->count == row_array->capacity) {
        /* 행 배열이 꽉 찼으면 2배로 늘려서 더 담을 공간을 만든다. */
        new_capacity = (row_array->capacity == 0) ? 8 : row_array->capacity * 2;
        new_rows = (char **) realloc(row_array->rows, sizeof(char *) * (size_t) new_capacity);
        if (new_rows == NULL) {
            return -1;
        }
        /* realloc 성공 후에만 rows와 capacity를 새 값으로 갱신한다. */
        row_array->rows = new_rows;
        row_array->capacity = new_capacity;
    }

    /* count 위치가 "다음 빈칸"이므로 그 자리에 row를 넣고,
       다음 행을 위해 count를 1 증가시킨다. */
    row_array->rows[row_array->count++] = row;
    return 0;
}

/* duplicate_string()은 문자열 전체를 새 heap 메모리로 복사하는 helper다. */
static char *duplicate_string(const char *value) {
    /* len은 value 문자열 길이이고,
       duplicate_range()에 그대로 넘겨서 복사 크기를 정한다. */
    size_t len;

    if (value == NULL) {
        return NULL;
    }

    len = strlen(value);
    return duplicate_range(value, len);
}

/* duplicate_range()는 문자열의 일부 구간만 잘라
   새 heap 문자열로 만드는 helper다. */
static char *duplicate_range(const char *start, size_t len) {
    /* copy는 start부터 len글자를 옮겨 담을 새 heap 메모리다. */
    char *copy;

    if (start == NULL) {
        return NULL;
    }

    copy = (char *) malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }

    /* len글자만 복사하고 마지막에는 문자열 종료 문자 '\0'를 붙인다. */
    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

/* write_index_snapshot()은 현재 인덱스 사용 방식을 설명하는
   보조 파일 users.idx를 갱신한다. */
static int write_index_snapshot(void) {
    FILE *fp;

    /* 이번 구현의 B-tree는 메모리 인덱스가 핵심이다.
       users.idx는 "인덱스 사용 중"임을 보여주는 설명용 파일로 유지한다. */
    fp = fopen(INDEX_FILE_PATH, "w");
    if (fp == NULL) {
        return -1;
    }

    fputs("MiniSQL users.id B-tree index is maintained in memory.\n", fp);
    fputs("The index is rebuilt from data/users.csv when the program starts.\n", fp);
    fputs("New INSERT operations update the in-memory index immediately.\n", fp);

    if (fclose(fp) != 0) {
        return -1;
    }

    return 0;
}

/* grow_buffer()는 동적 문자열 버퍼 공간이 부족할 때
   realloc으로 크기를 2배로 늘리는 공통 helper다. */
static int grow_buffer(char **buffer, size_t *capacity) {
    /* new_buffer는 realloc 성공 시 새 주소를 담는다.
       realloc은 주소가 바뀔 수 있으므로 임시 변수로 먼저 받는 편이 안전하다. */
    char *new_buffer;

    if (buffer == NULL || *buffer == NULL || capacity == NULL) {
        return -1;
    }

    /* 현재 버퍼가 부족하면 크기를 2배로 늘린다. */
    *capacity *= 2;
    new_buffer = (char *) realloc(*buffer, *capacity);
    if (new_buffer == NULL) {
        return -1;
    }

    /* 새 주소가 반환될 수 있으므로 실제 포인터도 함께 갱신한다. */
    *buffer = new_buffer;
    return 0;
}
