# Lesson 01. 큰 그림과 저장소 맵

## 이 문서의 목적

이 저장소의 전체 데이터 흐름과 폴더 구성을 한 번에 잡도록 돕습니다.

## 이 저장소의 전체 흐름

이 프로젝트의 핵심 파이프라인은 아래 한 줄로 요약됩니다.

```text
파일 경로 / -e 문자열 / REPL 입력
    -> run_sql_text
    -> tokenize_sql
    -> parse_program
    -> execute_program
    -> load_schema / csv_append_row / csv_load_table -> printer
```

실제 파일로 다시 쓰면 다음과 같습니다.

```text
src/main.c
  -> src/sql_runner.c
       -> src/file_reader.c (파일 모드일 때만)
       -> src/tokenizer.c
       -> src/parser.c
       -> src/executor.c
            -> src/schema.c
            -> src/csv_storage.c
            -> src/printer.c
```

## 실제 폴더별 역할

- `include/`
  `Token`, `Program`, `TableSchema`, `CsvTable`, `SqlError` 같은 핵심 선언을 모읍니다.
- `src/`
  실행 로직이 들어 있습니다.
- `tests/`
  단위 테스트와 통합 테스트 스크립트가 있습니다.
- `schema/`
  테이블 정의를 보관합니다.
- `sample_sql/`
  정상 케이스와 에러 케이스 SQL 예제가 있습니다.
- `expected_output/`
  통합 테스트가 비교하는 결과 파일이 있습니다.
- `data/`
  실제 CSV 데이터가 만들어지는 위치입니다.
- `Lessons/`
  구현 해설 문서입니다.

## 실제 코드 읽기 출발점

가장 좋은 출발점은 `src/main.c`입니다. `main` 함수는 길지 않고, 이 저장소 전체를 대표하는 함수 호출 순서를 보여줍니다.

그다음 아래 순서가 자연스럽습니다.

1. `run_sql_text`
2. `tokenize_sql`
3. `parse_program`
4. `execute_program`
5. `load_schema`
6. `csv_append_row`, `csv_load_table`

## 왜 이런 분리가 중요한가

- tokenizer와 parser를 분리하면 글자 처리와 문법 처리를 분리할 수 있습니다.
- 입력 모드와 실행 파이프라인을 분리하면 기능을 확장해도 핵심 로직을 덜 건드리게 됩니다.
- parser와 executor를 분리하면 "구조화"와 "실행"을 분리할 수 있습니다.
- schema와 storage를 분리하면 "정답표"와 "실제 데이터"를 분리할 수 있습니다.
- printer를 분리하면 출력 형식을 바꿀 때 executor를 덜 건드리게 됩니다.

이 분리 덕분에 테스트도 계층별로 나눌 수 있습니다.

## 이 저장소에서 중요한 구조체

- `Token`, `TokenArray`
  tokenizer 결과
- `LiteralValue`, `Condition`
  parser가 만든 작은 의미 단위
- `Statement`, `Program`
  parser 최종 결과
- `ColumnSchema`, `TableSchema`
  schema 파일 해석 결과
- `CsvRow`, `CsvTable`
  CSV 파일을 메모리로 올린 결과
- `SqlError`
  공통 에러 메시지 전달 구조

## 초보자 실수 포인트

- `schema`가 있으면 DB가 자동으로 생긴다고 생각하기
- `data/*.csv`가 없으면 SELECT가 무조건 실패한다고 생각하기
- `Program`이 "복잡한 AST"라고만 느껴질 수 있습니다

하지만 현재 구현은 일부러 단순합니다.

- schema는 타입 검사 기준입니다.
- data 파일이 없으면 빈 테이블로 취급합니다.
- `Program`은 작은 AST 비슷한 구조체 모음입니다.

## 디버깅 포인트

- 프로그램이 시작하자마자 실패하면 `main`, `run_sql_file`, `run_repl`
- 문법 오류가 이상하면 `tokenize_sql`, `parse_program`
- 실행은 되는데 결과가 이상하면 `execute_program`, `csv_load_table`
- 출력 줄바꿈이나 쉼표가 틀리면 `src/printer.c`

## 더 생각해볼 질문

- tokenizer와 parser를 합치면 어떤 함수가 너무 많은 책임을 갖게 될까?
- executor와 storage를 합치면 테스트가 왜 어려워질까?
- 나중에 index를 넣는다면 어느 폴더에 어떤 모듈이 추가될까?

## 생각 확장 방향

지금은 선형 스캔과 단순 배열 기반 구조지만, 이 큰 그림을 유지한 채 내부 자료구조만 더 강하게 바꾸는 방향으로 확장할 수 있습니다.
