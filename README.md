# Minimal SQL Processor Demo

`Minimal SQL Processor Demo`는 C로 만든 교육용 최소 SQL 처리기입니다.  
이 프로젝트는 "DBMS를 완성하는 것"보다, 아래 흐름을 직접 구현하고 읽어보는 데 초점을 둡니다.

```text
SQL 파일 읽기 -> 토큰화(tokenize) -> 파싱(parse) -> 실행(execute) -> CSV 저장/조회 -> 결과 출력
```

현재 구현은 작은 범위를 명확하게 보여주는 데 집중되어 있습니다.

- 파일 모드, 문자열 실행 모드, REPL 모드 제공
- 여러 SQL 문장을 한 파일에서 순서대로 처리
- `INSERT INTO table VALUES (...);`
- `SELECT * FROM table;`
- `SELECT col1, col2 FROM table;`
- `SELECT ... FROM table WHERE column operator value;`
- schema 파일 로딩
- CSV 파일 생성 / append / scan
- 명확한 에러 메시지
- 단위 테스트 + 통합 테스트
- 실제 구현 기준의 Lessons 문서

## 현재 구현 상태

현재 저장소에서 동작하는 기능은 아래와 같습니다.

### 지원 기능

- 파일 모드
  `./sql_processor sample.sql`
- 문자열 실행 모드
  `./sql_processor -e "SELECT * FROM users;"`
- REPL 모드
  `./sql_processor --repl`
- `INSERT`
- `SELECT *`
- projection이 있는 `SELECT`
- 단일 조건 `WHERE`
- `INT`, `TEXT` 타입 검사
- `TEXT`에 대한 `=` / `!=` 비교
- 여러 statement가 들어 있는 SQL 파일 처리
- data 파일이 없을 때 빈 테이블 SELECT 처리

### 검증한 상태

아래 명령은 현재 통과합니다.

```bash
make
make test
./sql_processor sample_sql/09_full_demo.sql
./sql_processor -e "SELECT * FROM users;"
./sql_processor --repl
```

`make test`는 unit test와 integration test를 모두 수행합니다.

## 지원 SQL 문법

### INSERT

```sql
INSERT INTO users VALUES (1, 'Alice', 30);
```

### SELECT 전체 컬럼

```sql
SELECT * FROM users;
```

### SELECT 일부 컬럼

```sql
SELECT id, name FROM users;
```

### SELECT + WHERE 단일 조건

```sql
SELECT id, name FROM users WHERE age >= 20;
SELECT * FROM users WHERE name = 'Alice';
```

### WHERE 지원 연산자

- `=`
- `!=`
- `<`
- `>`
- `<=`
- `>=`

단, `TEXT` 컬럼은 `=` 와 `!=`만 허용합니다.

## 지원하지 않는 것

이 프로젝트는 의도적으로 작은 범위만 구현합니다.

- `CREATE TABLE`
- `UPDATE`
- `DELETE`
- `JOIN`
- `ORDER BY`
- `GROUP BY`
- `AND`
- `OR`
- subquery
- alias
- aggregation
- parser generator
- 외부 DB / 외부 parser 라이브러리

## 데이터 구조와 저장 방식

### schema

각 테이블 정의는 `schema/<table>.schema`에 있습니다.

예:

```text
table=users
columns=id:INT,name:TEXT,age:INT
```

### data

실제 데이터는 `data/<table>.csv`에 저장됩니다.

예:

```csv
id,name,age
1,Alice,30
2,Bob,24
```

### 타입

- `INT`
- `TEXT`

### TEXT 제한

단순한 교육용 구현을 위해 TEXT에는 아래 제한이 있습니다.

- 쉼표 포함 금지
- 줄바꿈 포함 금지
- 작은따옴표 포함 금지
- 복잡한 escape 미지원

## 빠르게 실행해보기

### 1. 빌드

```bash
make
```

### 2. data 초기화

```bash
./scripts/reset_data.sh
```

### 3. 샘플 SQL 실행

```bash
./sql_processor sample_sql/01_users_bootstrap.sql
./sql_processor sample_sql/02_users_projection.sql
./sql_processor sample_sql/03_users_where_int.sql
./sql_processor sample_sql/04_users_where_text.sql
./sql_processor sample_sql/05_books_demo.sql
```

### 4. 문자열 실행 모드(`-e`)

```bash
./sql_processor -e "SELECT id, name FROM users WHERE age >= 29;"
./sql_processor -e "INSERT INTO users VALUES (4, 'Dave', 31);"
```

여러 문장도 가능합니다.

```bash
./sql_processor -e "INSERT INTO users VALUES (4, 'Dave', 31); SELECT * FROM users;"
```

### 5. REPL 모드

```bash
./sql_processor --repl
```

REPL 안에서는 세미콜론으로 문장을 끝내면 실행됩니다.

```text
sql> SELECT id, name
...> FROM users
...> WHERE age >= 29;
RESULT: users
id,name
1,Alice
3,Charlie
ROWS: 2
sql> quit
```

REPL 명령:

- `help`, `.help`
- `quit`, `.quit`
- `exit`, `.exit`

### 6. 전체 데모 실행

```bash
./scripts/reset_data.sh
./sql_processor sample_sql/09_full_demo.sql
```

## 출력 형식

### INSERT 성공

```text
OK: 1 row inserted into users
```

### SELECT 성공

```text
RESULT: users
id,name
1,Alice
2,Bob
ROWS: 2
```

### 에러 예시

```text
ERROR: column not found: nickname
ERROR: type mismatch in WHERE for column age
ERROR: unsupported TEXT comparison operator: >
```

## 테스트

### 전체 테스트

```bash
make test
```

### unit test

`unit_tests` 바이너리가 아래를 검증합니다.

- tokenizer
- parser
- schema / CSV storage / executor

관련 파일:

- `tests/test_tokenizer.c`
- `tests/test_parser.c`
- `tests/test_executor.c`

### integration test

`tests/integration_test.sh`는 실제 `sql_processor`를 실행해서 아래를 검증합니다.

- 파일 모드 실행
- `-e` 문자열 실행 모드
- REPL 모드
- `expected_output/*.txt`와 결과 비교
- 에러 케이스 종료 코드 확인

## 코드 구조

### 핵심 헤더

- `include/token.h`
- `include/ast.h`
- `include/schema.h`
- `include/csv_storage.h`
- `include/error.h`

### 핵심 구현

- `src/main.c`
  CLI 진입점과 모드 선택
- `src/file_reader.c`
  SQL 파일 전체 읽기
- `src/sql_runner.c`
  공통 SQL 실행 파이프라인
- `src/tokenizer.c`
  SQL 문자열 -> TokenArray
- `src/parser.c`
  TokenArray -> Program / Statement
- `src/schema.c`
  schema 파일 로딩
- `src/csv_storage.c`
  CSV 생성 / append / load
- `src/executor.c`
  INSERT / SELECT 실행, WHERE 평가, projection
- `src/printer.c`
  출력 포맷

## 실제 실행 흐름

현재 코드 기준으로 보면 전체 흐름은 아래와 같습니다.

```text
main
  -> 파일 모드면 run_sql_file
       -> read_entire_file
       -> run_sql_text
  -> -e 모드면 run_sql_text
  -> REPL 모드면 run_repl
       -> 입력 누적
       -> run_sql_text

run_sql_text
  -> tokenize_sql
  -> parse_program
  -> execute_program
       -> load_schema
       -> csv_append_row / csv_load_table
       -> build_projection
       -> validate_where_clause
       -> row_matches_where
       -> print_insert_success / print_result_header / print_result_row / print_result_footer
```

즉 입력 모드는 세 가지지만, SQL 실행 파이프라인은 하나로 공유됩니다.

## Docker / Dev Container 환경

이 프로젝트는 팀에서 사용하는 Docker / Dev Container 환경을 기준으로도 실행되도록 맞춰져 있습니다.

### 관련 설정

- `.devcontainer/Dockerfile`
- `.devcontainer/devcontainer.json`
- `.vscode/tasks.json`
- `.vscode/launch.json`

### 의미

- Ubuntu 기반 환경에서 빌드 / 실행
- `gcc`, `make`, `gdb`, `valgrind` 사용 가능
- VS Code Dev Container 안에서 바로 디버깅 가능

즉 로컬 환경이 다르더라도, 최종 기준 환경은 이 컨테이너 설정입니다.

## Lessons

`Lessons/` 폴더는 일반론 중심 문서가 아니라 **현재 구현된 프로젝트 해설 문서**입니다.

특히 아래 문서를 먼저 읽으면 좋습니다.

- `Lessons/00_start_here.md`
- `Lessons/03_cli_file_io_and_program_entry.md`
- `Lessons/04_tokenizer_in_this_project.md`
- `Lessons/05_parser_and_ast_in_this_project.md`
- `Lessons/06_execution_and_where_in_this_project.md`
- `Lessons/11_testing_debugging_and_common_bugs.md`

## 저장소 읽기 추천 순서

1. `README.md`
2. `src/main.c`
3. `src/sql_runner.c`
4. `src/tokenizer.c`
5. `src/parser.c`
6. `src/executor.c`
7. `src/schema.c`
8. `src/csv_storage.c`
9. `tests/`
10. `Lessons/`

## 제한 사항

이 프로젝트는 "작지만 구조가 선명한 데모"를 목표로 합니다.  
그래서 일부 기능은 일부러 구현하지 않았고, 저장 포맷도 단순 CSV로 제한했습니다.

성능보다는:

- 읽기 쉬운 구조
- 명확한 모듈 분리
- 초보자 친화적 주석
- 테스트 가능한 출력 형식

을 우선했습니다.

## 향후 확장 아이디어

- `AND`, `OR`를 위해 WHERE를 expression tree로 확장
- `ORDER BY` 추가
- schema lookup 자료구조 개선
- CSV 대신 binary page format 도입
- index 추가
- REPL 모드 추가

## 참고용 샘플 파일

### schema

- `schema/users.schema`
- `schema/books.schema`

### sample SQL

- `sample_sql/01_users_bootstrap.sql`
- `sample_sql/02_users_projection.sql`
- `sample_sql/03_users_where_int.sql`
- `sample_sql/04_users_where_text.sql`
- `sample_sql/05_books_demo.sql`
- `sample_sql/06_error_unknown_column.sql`
- `sample_sql/07_error_type_mismatch.sql`
- `sample_sql/08_error_unsupported_text_compare.sql`
- `sample_sql/09_full_demo.sql`

### expected output

- `expected_output/01_users_bootstrap.txt`
- `expected_output/02_users_projection.txt`
- `expected_output/03_users_where_int.txt`
- `expected_output/04_users_where_text.txt`
- `expected_output/05_books_demo.txt`
- `expected_output/06_error_unknown_column.txt`
- `expected_output/07_error_type_mismatch.txt`
- `expected_output/08_error_unsupported_text_compare.txt`
- `expected_output/09_full_demo.txt`
