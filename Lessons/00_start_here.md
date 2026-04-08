# Lesson 00. 시작 가이드

## 이 문서의 목적

이 저장소를 처음 읽는 사람이 어디서부터 보면 좋은지, 그리고 무엇을 질문하며 읽으면 좋은지 안내합니다.

## 이 저장소의 한 줄 요약

`src/main.c`에서 파일 모드, `-e` 모드, REPL 모드를 선택한 뒤 `run_sql_text`, `tokenize_sql`, `parse_program`, `execute_program`으로 이어지는 교육용 최소 SQL 처리기입니다. 데이터는 `schema/*.schema`와 `data/*.csv`를 사용합니다.

## 가장 먼저 읽을 파일

1. `README.md`
2. `src/main.c`
3. `src/sql_runner.c`
4. `src/tokenizer.c`
5. `src/parser.c`
6. `src/executor.c`
7. `src/schema.c`
8. `src/csv_storage.c`
9. `tests/`

이 순서가 좋은 이유는 실행 흐름과 거의 같기 때문입니다.

## 폴더 맵

- `include/`
  `Token`, `Program`, `TableSchema`, `CsvTable`, `SqlError` 선언
- `src/`
  실제 구현
- `tests/`
  단위 테스트와 통합 테스트 스크립트
- `schema/`
  테이블 정의 파일
- `sample_sql/`
  시연용 SQL
- `expected_output/`
  기대 출력
- `data/`
  실행 중 생성되는 CSV
- `.devcontainer/`, `.vscode/`
  팀 Docker/devcontainer 실행 기준

## 처음 붙잡아야 할 핵심 질문

- SQL 파일은 어디서 읽히는가
- `-e` 문자열과 REPL 입력은 파일 모드와 어디서 합쳐지는가
- 문자열이 언제 토큰으로 쪼개지는가
- 토큰이 언제 `Statement`와 `Program` 구조체가 되는가
- schema는 언제 읽고 어디서 타입 검사를 하는가
- WHERE는 어디서 평가되는가
- CSV는 어느 함수가 만들고 어느 함수가 읽는가

## 실제 코드 흐름 따라가기

1. `main`
   `src/main.c`의 `main`은 파일 모드, `-e`, REPL 중 입력 방식을 고릅니다.
2. `sql_runner`
   `src/sql_runner.c`의 `run_sql_text`, `run_sql_file`이 공통 실행 경로를 제공합니다.
2. tokenizer
   `src/tokenizer.c`의 `tokenize_sql`이 `TokenArray`를 만듭니다.
3. parser
   `src/parser.c`의 `parse_program`이 `Program`과 `Statement` 구조를 만듭니다.
4. executor
   `src/executor.c`의 `execute_program`이 각 문장을 실행합니다.
5. storage
   `load_schema`, `csv_append_row`, `csv_load_table`이 실제 파일 I/O를 담당합니다.
6. printer
   `src/printer.c`가 표준 출력 형식을 맞춥니다.

## 초보자에게 추천하는 학습 순서

- 먼저 `main`에서 전체 흐름을 본다.
- 그다음 `Token`, `LiteralValue`, `Statement`, `TableSchema`, `CsvTable` 구조체를 본다.
- tokenizer와 parser를 보고 "문자열이 구조화되는 과정"을 이해한다.
- executor와 storage를 보며 "구조가 실제 파일 작업으로 이어지는 과정"을 본다.
- 마지막에 `tests/`와 이 Lessons 폴더를 읽는다.

## 초보자 실수 포인트

- `-e`와 REPL이 tokenizer/parser를 별도로 가진다고 착각하기
- parser가 직접 파일을 읽는다고 착각하기
- executor가 토큰을 직접 읽는다고 착각하기
- `schema`와 `data`가 같은 역할이라고 느끼기

## 디버깅 포인트

- 토큰화 문제면 `tests/test_tokenizer.c`
- 문법 문제면 `tests/test_parser.c`
- 타입 검사나 WHERE 문제면 `tests/test_executor.c`
- 전체 출력 문제면 `tests/integration_test.sh`

## 더 생각해볼 질문

- 왜 입력 모드는 늘어나도 실행 파이프라인은 하나로 유지하는 것이 좋을까?
- tokenizer와 parser를 하나로 합치면 어떤 부분이 더 복잡해질까?
- 지금 구조에서 `ORDER BY`를 넣으려면 어느 파일을 먼저 바꾸게 될까?

## 생각 확장 방향

지금은 아주 작은 SQL 처리기지만, 파일 경계가 분리돼 있어서 앞으로 expression tree, index, binary page format 같은 확장을 어디에 붙일지 감을 잡기 좋습니다.
