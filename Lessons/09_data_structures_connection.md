# Lesson 09. 자료구조 연결

## 이 문서의 목적

이 저장소의 실제 구현을 기준으로, 배열/트리/스택/큐/연결 리스트/BST 같은 자료구조 사고가 어디에서 연결되는지 설명합니다.

## 현재 코드에서 실제로 가장 많이 쓰는 자료구조

가장 많이 쓰는 것은 동적 배열입니다.

대표 예시:

- `TokenArray.items`
- `Program.statements`
- `InsertStatement.values`
- `SelectStatement.selected_columns`
- `TableSchema.columns`
- `CsvTable.rows`

이 프로젝트가 배열 중심인 이유는 현재 범위가 작고, 선형 스캔으로도 충분히 설명 가능하기 때문입니다.

## 트리와 연결되는 지점

현재 코드에는 진짜 트리 구조체가 없습니다. 하지만 트리적 사고는 이미 들어 있습니다.

- `Statement`
  INSERT와 SELECT로 갈라지는 분기
- `SelectStatement.where`
  앞으로 확장되면 expression tree가 될 자리
- `parse_program`
  토큰을 구조화된 문장으로 바꾸는 과정

지금 WHERE는 `column operator value` 하나뿐이어서 `Condition` 구조체 하나로 충분합니다. 하지만 `AND`, `OR`, 괄호가 들어오면 거의 바로 트리 구조가 필요해집니다.

## 스택과 연결되는 지점

명시적 stack 자료구조는 쓰지 않았지만, call stack은 계속 사용합니다.

예:

- `main -> tokenize_sql -> push_token`
- `main -> parse_program -> parse_select_statement -> parse_literal`
- `execute_program -> execute_select_statement -> row_matches_where`

또한 나중에 괄호가 있는 수식이나 중첩 SELECT를 지원한다면 parser는 명시적 stack 또는 재귀 구조를 더 적극적으로 쓰게 됩니다.

## 큐와 연결될 수 있는 지점

현재 구현에는 queue가 직접 필요하지 않습니다. 하지만 확장 방향으로는 연결됩니다.

- 작업 파이프라인 버퍼링
- 비동기 파일 읽기
- 대량 row 처리에서 batch 작업 스케줄링
- BFS 스타일 문법 트리 순회

즉 현재는 필요 없지만, 처리량이 커지면 "들어온 작업을 순서대로 처리한다"는 의미에서 큐 사고가 등장할 수 있습니다.

## 연결 리스트와 연결될 수 있는 지점

현재는 배열을 썼지만, 아래는 linked list 후보입니다.

- token stream
- statement list
- CSV row list

왜 지금은 안 썼을까?

- 인덱스 접근이 불편해집니다.
- 학습 초반에는 배열이 더 직관적입니다.
- 데이터 크기가 작아 `realloc` 기반 동적 배열로 충분합니다.

하지만 아주 큰 입력을 streaming으로 처리한다면 linked list나 chunked buffer 구조도 생각할 수 있습니다.

## BST와 연결될 수 있는 지점

현재 `find_column_index`는 선형 탐색입니다. 작은 schema에서는 충분하지만, 컬럼이 많아지면 BST나 hash map을 떠올릴 수 있습니다.

BST가 어울릴 수 있는 지점:

- schema column lookup
- table name lookup
- future index structure
- WHERE 최적화

즉, 이 저장소는 아직 BST를 직접 쓰지 않지만 "어디에 쓰고 싶은지"는 매우 분명합니다.

## 왜 지금은 선형 배열로 두었는가

이 프로젝트 목표는 성능보다 구조 선명도입니다.

- 배열은 설명이 쉽습니다.
- 메모리 배치가 단순합니다.
- 디버깅이 쉽습니다.
- 초보자가 흐름을 따라가기 좋습니다.

그래서 자료구조 선택도 의도적으로 보수적입니다.

## 실제 파일과 연결해서 다시 보기

- `src/tokenizer.c`
  token 동적 배열
- `src/parser.c`
  statement, literal, selected column 배열
- `src/schema.c`
  column 배열과 선형 검색
- `src/csv_storage.c`
  row 배열
- `src/executor.c`
  projection 인덱스 배열

## 초보자 실수 포인트

- "트리를 안 썼으니 자료구조와 상관없다"라고 생각하기
- 배열이 단순해서 설계 의미도 없다고 생각하기

실제로는 "지금은 왜 배열로 충분한가"를 설명하는 것 자체가 자료구조 사고입니다.

## 더 생각해볼 질문

- WHERE를 expression tree로 바꾼다면 `Condition` 대신 어떤 구조체가 필요할까?
- `find_column_index`를 hash map으로 바꾸면 어떤 코드가 단순해지고 어떤 문서가 바뀔까?
- CSV row를 모두 메모리에 안 올리고 streaming 처리하면 어떤 자료구조가 더 적합할까?

## 생각 확장 방향

이 저장소는 자료구조를 일부러 과하게 쓰지 않았습니다. 그 덕분에 "왜 더 강한 자료구조가 아직 필요 없고, 언제 필요해질지"를 비교하며 배우기 좋습니다.
