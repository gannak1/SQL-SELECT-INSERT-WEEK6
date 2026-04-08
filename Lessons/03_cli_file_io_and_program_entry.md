# Lesson 03. CLI, 파일 I/O, 프로그램 시작점

## 이 문서의 목적

프로그램이 어디서 시작되고, SQL 파일을 어떻게 읽어 전체 파이프라인으로 넘기는지 설명합니다.

## 실제 코드 위치

- `src/main.c`
  프로그램 시작점과 모드 선택
- `src/sql_runner.c`
  공통 SQL 실행 경로
- `src/file_reader.c`
  파일 전체 읽기
- `include/error.h`, `src/error.c`
  공통 에러 처리
- `Makefile`
  CLI 바이너리와 테스트 바이너리 빌드 규칙

## 실행 흐름

현재 `main`은 세 가지 입력 모드를 지원합니다.

1. 파일 모드
   `./sql_processor sample.sql`
2. 문자열 실행 모드
   `./sql_processor -e "SELECT * FROM users;"`
3. REPL 모드
   `./sql_processor --repl`

중요한 점은, 세 모드가 모두 결국 `run_sql_text`로 이어진다는 것입니다.

## CLI 인자 처리

현재 사용법은 아래와 같습니다.

```bash
./sql_processor sample_sql/09_full_demo.sql
./sql_processor -e "SELECT * FROM users WHERE age >= 29;"
./sql_processor --repl
```

help:

```text
./sql_processor --help
```

`main`은 인자를 보고:

- 파일 경로면 `run_sql_file`
- `-e`면 `run_sql_text`
- `--repl`이면 `run_repl`

로 분기합니다.

## 공통 실행 경로

`src/sql_runner.c`는 입력 방식과 무관하게 같은 경로를 제공합니다.

```text
run_sql_text
  -> tokenize_sql
  -> parse_program
  -> execute_program
```

즉, 이 프로젝트는 "입력 획득"과 "SQL 실행"을 나눠 두었습니다.

## 파일 모드

파일 모드는 기존 방식 그대로입니다.

```bash
./sql_processor sample_sql/09_full_demo.sql
```

이 모드에서만 `read_entire_file`가 사용됩니다.

## 문자열 실행 모드(`-e`)

`-e`는 파일을 만들지 않고 SQL 문자열을 바로 실행하는 모드입니다.

```bash
./sql_processor -e "SELECT id, name FROM users WHERE age >= 29;"
./sql_processor -e "INSERT INTO users VALUES (4, 'Dave', 31); SELECT * FROM users;"
```

학습 포인트:

- 파일 모드와 달리 `read_entire_file`를 건너뜁니다.
- 하지만 tokenizer, parser, executor는 완전히 같습니다.

## REPL 모드

REPL은 `Read-Eval-Print Loop`입니다.

```bash
./sql_processor --repl
```

사용 예:

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

현재 REPL 특징:

- 여러 줄 입력을 누적할 수 있습니다.
- 문자열 바깥의 마지막 의미 있는 문자가 `;`이면 실행합니다.
- `help`, `.help`, `exit`, `quit`, `.exit`, `.quit`를 지원합니다.
- prompt는 stderr에 출력해서 결과 stdout과 분리합니다.

## SQL 파일 읽기

`read_entire_file`은 `src/file_reader.c`에 있습니다.

핵심 단계:

1. `fopen`
2. `fseek(..., SEEK_END)`로 파일 크기 확인
3. `ftell`
4. `fseek(..., SEEK_SET)`로 처음으로 복귀
5. `malloc(file_size + 1)`
6. `fread`
7. 마지막에 `buffer[file_size] = '\0'`

이 덕분에 파일 모드도 결국 tokenizer에게는 "문자열 하나"를 넘기는 것으로 끝납니다.

## 에러 처리

모든 단계는 `SqlError`를 공유합니다.

- `sql_error_clear`
- `sql_error_set`
- `sql_error_print`

장점:

- tokenizer, parser, executor가 각자 상세 메시지를 남길 수 있습니다.
- `main`은 어디서 실패했는지 몰라도 마지막 메시지만 출력하면 됩니다.

## Docker / Dev Container 관점

이 프로젝트는 팀 Docker 환경을 염두에 두고 있습니다.

- `.devcontainer/Dockerfile`
  Ubuntu + gcc + make + gdb + valgrind
- `.vscode/tasks.json`
  gcc 빌드 task
- `.vscode/launch.json`
  gdb launch 설정

즉 `main`과 Makefile은 "컨테이너 안에서 바로 빌드/실행되는 단순 CLI"를 전제로 유지하고 있습니다.

## 초보자 실수 포인트

- 세 모드가 각각 완전히 다른 실행 엔진을 쓴다고 생각하기
- `read_entire_file`가 모든 모드에서 쓰인다고 생각하기
- `main`에서 cleanup을 빼먹기
- stderr와 stdout를 구분하지 않기
- `schema`와 `data` 경로가 현재 작업 디렉터리 기준이라는 점을 놓치기

## 디버깅 포인트

- 파일 모드에서만 파일 경로 문제를 먼저 의심합니다.
- `-e`가 안 되면 셸 인용(quote) 문제를 먼저 확인합니다.
- REPL이 실행되지 않으면 세미콜론 누적 규칙을 먼저 확인합니다.
- 컨테이너 안에서 실행한다면 `pwd`와 `ls schema data sample_sql`부터 확인합니다.
- tokenizer나 parser 전에 죽으면 `run_sql_text`에 들어가기 전 입력 확보 단계부터 봅니다.

## 더 생각해볼 질문

- 왜 `-e`와 REPL은 "새로운 SQL 엔진"이 아니라 "새 입력 모드"라고 보는 것이 맞을까?
- SQL 파일 경로 외에 `--schema-dir`, `--data-dir` 옵션을 넣으려면 CLI 구조가 어떻게 바뀔까?
- REPL에서 여러 statement를 history와 함께 다루려면 무엇이 더 필요할까?

## 생각 확장 방향

지금의 시작점은 입력 모드를 세 개로 늘렸지만, 핵심 실행 경로는 하나로 유지합니다. 이 구조가 앞으로 옵션 파서, schema/data 경로 설정, 더 풍부한 REPL 기능을 넣을 때도 좋은 출발점이 됩니다.
