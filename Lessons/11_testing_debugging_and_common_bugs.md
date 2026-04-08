# Lesson 11. 테스트, 디버깅, 흔한 버그

## 이 문서의 목적

현재 저장소의 테스트 구조를 설명하고, 실제로 어떤 버그를 어디서 잡을 수 있는지 정리합니다.

## 실제 코드 위치

- `tests/test_main.c`
  unit test 진입점
- `tests/test_tokenizer.c`
  tokenizer 테스트
- `tests/test_parser.c`
  parser 테스트
- `tests/test_executor.c`
  schema / storage / executor 테스트
- `tests/integration_test.sh`
  실제 CLI 통합 테스트
- `Makefile`
  `make test` 규칙

## make test가 하는 일

`make test`는 아래 순서로 진행됩니다.

1. `unit_tests` 빌드
2. `./unit_tests` 실행
3. `./tests/integration_test.sh` 실행

즉 로직 단위 검증과 CLI 단위 회귀 검증을 둘 다 합니다.

## 단위 테스트가 보는 것

### tokenizer

`tests/test_tokenizer.c`는 아래를 확인합니다.

- `SELECT id, name FROM users WHERE age >= 20;` 토큰 시퀀스
- 음수 int literal
- string literal 처리
- 쉼표가 들어간 TEXT literal 거부

### parser

`tests/test_parser.c`는 아래를 확인합니다.

- INSERT + SELECT가 한 Program에 잘 들어가는지
- WHERE가 `Condition` 구조로 파싱되는지
- 세미콜론 누락 시 parser가 실패하는지

### executor / storage

`tests/test_executor.c`는 아래를 확인합니다.

- schema 로딩
- CSV append / load round-trip
- WHERE 타입 불일치 에러
- TEXT 비교 연산자 제한 에러

## 통합 테스트가 보는 것

`tests/integration_test.sh`는 실제 `sql_processor`를 실행합니다.

정상 케이스:

- `01_users_bootstrap.sql`
- `02_users_projection.sql`
- `03_users_where_int.sql`
- REPL 다중 줄 입력 케이스
- `04_users_where_text.sql`
- `05_books_demo.sql`
- `09_full_demo.sql`

에러 케이스:

- `06_error_unknown_column.sql`
- `07_error_type_mismatch.sql`
- `08_error_unsupported_text_compare.sql`

그리고 결과를 `expected_output/*.txt`와 `diff -u`로 비교합니다.

지금은 세 입력 모드를 모두 확인합니다.

- 파일 모드
- `-e` 문자열 실행 모드
- REPL 모드

## 지금 구조가 좋은 이유

- tokenizer 버그를 parser 이전에 잡을 수 있습니다.
- parser 버그를 executor 이전에 잡을 수 있습니다.
- executor 출력 회귀를 실제 CLI의 세 입력 모드에서 다시 확인할 수 있습니다.

즉 "어디서 망가졌는지"를 좁히기 쉽습니다.

## 흔한 버그 유형

- 토큰 개수나 토큰 종류가 어긋남
- 세미콜론 누락이 tokenizer에서 잡히지 않는데 이상하다고 느끼기
- schema 컬럼 이름과 SELECT 컬럼 이름 비교가 어긋남
- CSV header가 기대와 다름
- INT 컬럼 비교에서 `strtol` 이후 검증 누락
- output 줄바꿈이나 쉼표 형식이 기대 출력과 다름

## 디버깅 순서 추천

1. 실패한 테스트가 unit인지 integration인지 확인
2. integration이면 대응하는 `sample_sql`과 `expected_output`을 같이 본다
3. tokenizer/parser/executor 중 어느 층 문제인지 좁힌다
4. 필요한 값만 printf로 찍는다
5. 필요하면 Docker/devcontainer 안에서 gdb 또는 valgrind를 사용한다

## 실제로 유용한 명령

전체 테스트:

```bash
make test
```

데모만 실행:

```bash
./scripts/reset_data.sh
./sql_processor sample_sql/09_full_demo.sql
```

gdb용 빌드:

```bash
make clean
make CFLAGS='-std=c11 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L -g'
```

## Docker 환경에서의 디버깅

팀 환경 기준으로 `.devcontainer/Dockerfile`에 `gdb`, `valgrind`가 들어 있습니다. 즉 컨테이너 안에서 아래 같은 디버깅 흐름을 바로 시도할 수 있습니다.

- `gdb ./sql_processor`
- `valgrind ./sql_processor sample_sql/09_full_demo.sql`

## 초보자 실수 포인트

- 정상 케이스만 보고 "다 됐다"고 생각하기
- stdout과 stderr 차이를 무시하기
- data 디렉터리를 초기화하지 않고 결과가 누적되는 이유를 놓치기

특히 통합 테스트는 실행 전 `./scripts/reset_data.sh`를 호출한다는 점을 꼭 기억하세요.

## 더 생각해볼 질문

- 현재 unit test에 더 추가하고 싶은 함수는 무엇인가?
- output format이 바뀌면 가장 먼저 어떤 테스트가 깨질까?
- REPL prompt를 stdout 대신 stderr로 둔 이유는 테스트 관점에서 어떤 장점이 있을까?
- ASan이나 UBSan 같은 도구를 붙인다면 Makefile은 어떻게 바뀔까?

## 생각 확장 방향

좋은 학습 프로젝트는 "동작하는 코드"뿐 아니라 "왜 망가졌는지 빨리 찾을 수 있는 구조"도 중요합니다. 이 저장소는 그 점을 테스트 파일 구조로 함께 보여주고 있습니다.
