# 파일 구조와 함수 설계

## 목적
이 문서는 실제 C 프로젝트를 구현할 때 사용할 폴더 구조와 파일별 함수 책임을 구체적으로 정리한다.
즉, 이 문서를 기준으로 하면 구현자는 어떤 파일을 만들고 어떤 함수를 어디에 둘지 바로 결정할 수 있다.

## 권장 프로젝트 구조
```text
my_study/
├── README.md
├── Makefile
├── data/
│   ├── users.csv
│   └── users.idx
├── docs/
│   ├── 01-project-overview.md
│   ├── 02-sql-basics.md
│   ├── 03-processing-flow.md
│   ├── 04-storage-design.md
│   ├── 05-implementation-plan.md
│   ├── 06-users-table.md
│   └── 07-file-structure-and-functions.md
├── include/
│   ├── constants.h
│   ├── types.h
│   ├── repl.h
│   ├── parser.h
│   ├── executor.h
│   ├── btree.h
│   ├── storage.h
│   ├── printer.h
│   └── utils.h
└── src/
    ├── main.c
    ├── repl.c
    ├── parser.c
    ├── executor.c
    ├── btree.c
    ├── storage.c
    ├── printer.c
    └── utils.c
```

## 폴더별 역할
### `data/`
- 실제 사용자 데이터 CSV 파일을 저장한다.
- 1차 구현에서는 `users.csv` 하나만 사용한다.

### `include/`
- 여러 `.c` 파일에서 공유하는 구조체, 상수, 함수 선언을 둔다.

### `src/`
- 실제 로직 구현 파일을 둔다.

## 핵심 상수
`constants.h`에서 아래 상수를 관리한다.

- `DATA_FILE_PATH`
  - 값: `"data/users.csv"`
- `PROMPT_TEXT`
  - 값: `"MiniSQL> "`
- `CONTINUATION_PROMPT`
  - 값: `"...> "`
- `MAX_INPUT_LEN`
  - 예: `1024`
- `MAX_VALUE_COUNT`
  - 값: `5`
- `USER_INPUT_VALUE_COUNT`
  - 값: `5`
- `USER_COLUMN_COUNT`
  - 값: `6`
- `INITIAL_BUFFER_CAPACITY`
  - 예: `128`

## 핵심 구조체
`types.h`에는 아래 구조체와 enum을 둔다.

### `CommandType`
- `CMD_INSERT`
- `CMD_SELECT`
- `CMD_EXIT`
- `CMD_INVALID`

### `Condition`
- `char column[32]`
- `char *value`
- `int has_condition`

### `InsertCommand`
- `char table[32]`
- `char *values[MAX_VALUE_COUNT]`
- `int value_count`

설명:
- `values`에는 사용자가 입력한 `username`, `name`, `age`, `phone`, `email` 5개 값이 들어간다.
- `id`는 이 구조체에 담기지 않고 storage 단계에서 자동 생성된다.

### `SelectCommand`
- `char table[32]`
- `Condition condition`

### `Command`
- `CommandType type`
- `InsertCommand insert_cmd`
- `SelectCommand select_cmd`

### `ParseStatus`
- `PARSE_OK`
- `PARSE_MISSING_SEMICOLON`
- `PARSE_UNSUPPORTED_COMMAND`
- `PARSE_INVALID_INSERT`
- `PARSE_INVALID_SELECT`
- `PARSE_INVALID_WHERE`
- `PARSE_UNTERMINATED_STRING`
- `PARSE_UNSUPPORTED_QUOTED_FORMAT`
- `PARSE_OUT_OF_MEMORY`

### `ExecStatus`
- `EXEC_OK`
- `EXEC_UNSUPPORTED_TABLE`
- `EXEC_UNSUPPORTED_SELECT_COLUMNS`
- `EXEC_UNSUPPORTED_WHERE_CONDITION`
- `EXEC_INSERT_VALUE_COUNT_MISMATCH`
- `EXEC_INVALID_ID`
- `EXEC_INVALID_AGE`
- `EXEC_DATA_FILE_NOT_FOUND`
- `EXEC_READ_FAILED`
- `EXEC_WRITE_FAILED`
- `EXEC_MEMORY_ERROR`
- `EXEC_NO_ROWS_FOUND`

## 파일별 함수 설계
### `src/main.c`
역할:
- 프로그램 시작점
- 최소 초기화
- REPL 실행 함수 호출
- 종료 전 전역 storage 자원 정리

필요 함수:
- `int main(void);`
  - 전체 프로그램 진입점
  - `run_repl()` 호출 후 `storage_shutdown()`으로 전역 인덱스 해제

### `src/repl.c`
역할:
- `MiniSQL> ` 프롬프트 출력
- 반복 입력 루프 실행
- 세미콜론이 나올 때까지 여러 줄 입력 누적
- 빈 줄 무시
- `exit`, `quit`, EOF 처리
- 파서와 실행기 연결
- 오류 메시지 출력 후 프롬프트 유지

필요 함수:
- `int run_repl(void);`
  - REPL 전체 루프 실행
- `int process_input_line(const char *line);`
  - 한 줄 입력을 파싱하고 실행기로 전달
  - 성공/실패에 따라 메시지 출력
- `int is_exit_command(const char *line);`
  - `exit`, `quit` 여부 확인
- `int has_complete_statement(const char *buffer);`
  - 세미콜론 기준으로 문장 완료 여부 판단
- `int read_input_line(char **line_out);`
  - 동적 버퍼로 한 줄 입력 읽기

### `src/parser.c`
역할:
- MiniSQL 문자열을 내부 `Command` 구조체로 변환
- `INSERT`, `SELECT` 구분
- 문법 오류와 일부 지원 범위 오류 판정

필요 함수:
- `ParseStatus parse_command(const char *input, Command *command);`
  - 입력 문자열 전체를 파싱
- `ParseStatus parse_insert(const char *input, Command *command);`
  - INSERT 문장 파싱
- `ParseStatus parse_select(const char *input, Command *command);`
  - SELECT 문장 파싱
- `ParseStatus parse_condition(const char *input, Condition *condition);`
  - WHERE 절 파싱
- `void init_command(Command *command);`
  - 구조체 초기화
- `void free_command(Command *command);`
  - 동적으로 할당한 값 문자열 해제

### `src/executor.c`
역할:
- 파싱된 명령을 실제 동작으로 연결
- `INSERT`와 `SELECT` 분기 처리
- 값/타입 오류와 파일 관련 오류 처리
- `id` 기반 B-tree 인덱스 경로와 전체 스캔 경로를 분기 처리
- 자동 증가 `id` 생성 규칙에 맞는 INSERT 검증

필요 함수:
- `ExecStatus execute_command(const Command *command);`
  - 명령 종류에 따라 분기
- `ExecStatus execute_insert(const InsertCommand *insert_cmd);`
  - 저장소에 새 레코드 추가
- `ExecStatus execute_select(const SelectCommand *select_cmd);`
  - 저장소 조회 후 출력

### `src/storage.c`
역할:
- `data/users.csv` 파일 읽기/쓰기
- CSV quoting 규칙에 맞는 한 줄 append
- 전체 데이터 순회
- 메모리 B-tree 인덱스 구성 및 조회
- 동적 row / field 메모리 관리

필요 함수:
- `int append_user_record(const InsertCommand *insert_cmd);`
  - 새 `id`를 자동 생성한 뒤 CSV 끝에 한 줄 추가
- `int read_all_users(RowArray *row_array);`
  - 전체 행을 동적 배열로 읽기
- `int read_user_row_by_id(int id, char **row_out);`
  - B-tree로 파일 오프셋을 찾아 한 줄만 읽기
- `int split_csv_row(const char *row, char *values[USER_COLUMN_COUNT]);`
  - CSV 한 줄을 큰따옴표 규칙을 고려해 컬럼별로 분리
- `int row_matches_condition(char *const values[USER_COLUMN_COUNT], const Condition *condition);`
  - WHERE 조건 일치 여부 확인
- `void write_csv_field(FILE *fp, const char *value, int quote);`
  - 문자열 컬럼이면 큰따옴표를 붙여 CSV 필드 저장
- `void free_row_array(RowArray *row_array);`
  - 동적으로 읽은 행 배열 해제
- `void free_csv_values(char *values[USER_COLUMN_COUNT]);`
  - split된 컬럼 문자열 해제

### `src/btree.c`
역할:
- `id -> CSV 파일 오프셋` 매핑용 B-tree 구현
- 검색과 삽입을 담당

필요 함수:
- `void btree_init(BTreeIndex *tree);`
- `void btree_free(BTreeIndex *tree);`
- `int btree_search(const BTreeIndex *tree, int key, long *value_out);`
- `int btree_insert(BTreeIndex *tree, int key, long value);`
- `int btree_get_max(const BTreeIndex *tree, int *key_out, long *value_out);`

### `src/printer.c`
역할:
- SELECT 결과 출력 형식 담당
- 성공/오류 메시지 출력 담당

필요 함수:
- `void print_user_row(char *const values[USER_COLUMN_COUNT]);`
  - 사용자 한 행 출력
- `void print_select_header(void);`
  - 컬럼명 출력 여부를 통제
- `void print_rows_selected(int count);`
  - 선택된 행 수 출력
- `void print_message(const char *message);`
  - 일반 안내 메시지 출력
- `void print_error(const char *message);`
  - 오류 메시지 출력

### `src/utils.c`
역할:
- 문자열 전처리 보조 함수
- 공백 제거, 줄바꿈 제거, 따옴표 제거

필요 함수:
- `void trim_newline(char *str);`
- `void trim_spaces(char *str);`
- `void strip_matching_quotes(char *str);`
- `int starts_with_ignore_case(const char *str, const char *prefix);`
- `int is_quoted_string(const char *str);`
- `int is_blank_string(const char *str);`
- `int is_integer_string(const char *str);`
- `size_t copy_trimmed(char *dest, size_t dest_size, const char *src);`

## 헤더 파일 역할
### `include/repl.h`
- `run_repl` 관련 선언

### `include/parser.h`
- `parse_command` 관련 선언

### `include/executor.h`
- `execute_command` 관련 선언

### `include/storage.h`
- CSV 읽기/쓰기 함수 선언

### `include/printer.h`
- 출력 함수 선언

### `include/btree.h`
- B-tree 자료구조와 함수 선언

### `include/utils.h`
- 문자열 보조 함수 선언

### `include/types.h`
- 구조체와 enum 선언

### `include/constants.h`
- 경로, 길이 제한, 컬럼 개수 같은 상수 선언

## 파일 간 호출 흐름
```text
main.c
  -> repl.c
      -> parser.c
      -> executor.c
          -> storage.c
              -> btree.c
          -> printer.c
      -> utils.c
```

## 1차 구현 기준 세부 규칙
- MiniSQL 문장은 세미콜론이 나올 때까지 여러 줄로 입력할 수 있다.
- 키워드는 대소문자를 구분하지 않는다.
- 세미콜론이 나올 때까지 REPL은 입력을 계속 누적한다.
- 공백은 유연하게 허용한다.
- `INSERT`는 값 5개가 정확히 들어와야 한다.
- 숫자는 따옴표 없이 입력한다.
- 문자열은 작은따옴표 또는 큰따옴표로 감쌀 수 있다.
- 문자열의 시작과 끝은 같은 종류의 따옴표여야 한다.
- 문자열 내부 같은 종류의 따옴표는 1차 구현에서 지원하지 않는다.
- 빈 문자열을 허용한다.
- 문자열 내부 줄바꿈은 1차 구현에서 지원하지 않는다.
- `id`는 사용자가 입력하지 않고 자동 증가로 생성한다.
- `SELECT`는 `SELECT * FROM users;`만 허용한다.
- `WHERE`는 1개 조건만 허용한다.
- 1차 구현의 조건 비교는 `users` 테이블의 단일 컬럼 `= 값` 형태를 지원한다.
- CSV 저장 시 문자열 컬럼은 항상 큰따옴표로 저장한다.
- CSV 저장 시 숫자 컬럼은 따옴표 없이 저장한다.
- 긴 입력 버퍼와 일부 CSV 메모리는 `malloc`, `realloc`, `free`로 관리한다.
- `WHERE id = 값` 조회는 메모리 B-tree 인덱스를 사용한다.
- 지원하지 않는 문장은 `CMD_INVALID`로 처리한다.
- 빈 줄 입력은 조용히 무시한다.
- 파싱 오류 시 오류 메시지를 출력하고 프롬프트를 유지한다.
- `exit`, `quit` 입력 시 종료한다.
- EOF(`Ctrl + D`) 입력 시 조용히 종료한다.
- 오류 메시지는 문법 오류, 지원 범위 오류, 값/타입 오류, 파일 입출력 오류로 구분한다.
- INSERT 성공 시 `Inserted 1 row`를 출력한다.
- SELECT 결과가 없으면 `No rows found`를 출력한다.
- 새 행의 `id`는 현재 최대 `id + 1` 규칙으로 자동 생성한다.

## 구현 순서 추천
1. `constants.h`, `types.h`부터 만든다.
2. `main.c`에서 `run_repl()` 호출 구조를 만든다.
3. `repl.c`에 프롬프트 루프를 만든다.
4. `parser.c`에서 `INSERT`, `SELECT`를 구분한다.
5. `storage.c`에서 CSV append/read를 만든다.
6. `executor.c`에서 파서와 저장소를 연결한다.
7. `printer.c`로 출력 형식을 정리한다.
8. `utils.c`로 문자열 처리 중복을 제거한다.

## 구현 완료 후 확인할 것
- `MiniSQL> ` 프롬프트가 반복 출력되는가
- `exit` 입력 시 종료되는가
- `quit` 입력 시 종료되는가
- INSERT 후 `data/users.csv`에 행이 추가되는가
- SELECT 시 전체 행이 출력되는가
- `WHERE id = 1`, `WHERE name = 'Kim'`, `WHERE age = 25` 조건이 동작하는가
- 오류 입력 후에도 프로그램이 계속 실행되는가
