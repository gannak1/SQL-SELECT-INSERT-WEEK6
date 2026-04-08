# Lesson 12. 확장 질문

## 이 문서의 목적

현재 구현을 바탕으로 다음 단계에서 어떤 기능과 설계 확장을 고민할 수 있는지 정리합니다.

## 질문 1. AND / OR를 넣으려면?

현재 WHERE는 `Condition` 하나로 표현됩니다.

```text
column operator value
```

`AND`, `OR`를 넣으려면 보통 아래가 바뀝니다.

- `Condition` 구조체를 expression tree로 확장
- parser에서 우선순위와 괄호 처리 추가
- executor의 `row_matches_where`를 재귀 평가 또는 tree 순회 방식으로 변경

즉 가장 먼저 바뀌는 파일은 `include/ast.h`, `src/parser.c`, `src/executor.c`입니다.

## 질문 2. CREATE TABLE을 넣으려면?

현재 schema는 이미 존재한다고 가정합니다. 그래서 `CREATE TABLE`을 넣으려면 아래가 필요합니다.

- 새 Statement 타입 추가
- parser에 CREATE TABLE 문법 추가
- executor에서 schema 파일 생성 로직 추가
- README와 Lessons 범위 업데이트

그러면 저장소는 "읽기만 하는 schema"에서 "schema를 생성하는 시스템"으로 한 단계 더 올라갑니다.

## 질문 3. ORDER BY를 넣으려면?

현재 SELECT는 row를 읽는 즉시 출력해도 됩니다. 하지만 ORDER BY가 들어오면:

- 먼저 모든 결과 row를 모아야 하고
- 정렬 기준 컬럼을 해석해야 하고
- 비교 함수를 만들어야 합니다

즉 `execute_select_statement`의 흐름이 "scan하면서 바로 출력"에서 "scan -> collect -> sort -> print"로 바뀝니다.

## 질문 4. CSV 대신 binary format으로 바꾸려면?

가장 크게 바뀌는 파일은 `src/csv_storage.c`입니다. 하지만 인터페이스를 잘 유지하면 나머지 계층은 적게 바꿀 수도 있습니다.

예:

- `csv_append_row` 자리에 page write 함수
- `csv_load_table` 자리에 page scan 함수

중요한 것은 executor가 "storage 세부 구현"을 덜 알게 만드는 것입니다.

## 질문 5. 성능을 올리려면 어디부터 바꿔야 할까?

현재 병목 후보:

- `find_column_index` 선형 탐색
- `csv_load_table` 전체 row 메모리 적재
- WHERE 선형 스캔

확장 후보:

- schema lookup용 hash map 또는 BST
- streaming row scan
- 인덱스 파일
- 정렬/집계용 별도 메모리 구조

## 질문 6. 에러 메시지를 더 좋게 만들려면?

지금도 `SqlError`와 token `line`, `column` 덕분에 기본 위치 정보는 있습니다. 더 나아가고 싶다면:

- parser에서 예상 토큰 집합을 더 자세히 출력
- schema 오류에 파일 경로 포함
- integration test에 더 다양한 에러 케이스 추가

## 질문 7. REPL로 바꾸려면?

현재는 SQL 파일 배치 실행기입니다. REPL로 바꾸려면:

- `main`이 stdin 루프를 가져야 하고
- 한 줄 또는 여러 줄 입력 누적 로직이 필요하고
- 문장 종료 세미콜론 처리 방식을 다시 생각해야 합니다

즉 `src/main.c`와 입력 계층이 가장 먼저 바뀝니다.

## 질문 8. Lessons도 같이 어떻게 바뀌어야 할까?

기능을 추가하면 코드만 바꾸는 것으로 끝나면 안 됩니다.

- README 지원 범위
- 테스트
- `Lessons/04`, `05`, `06`, `07`

같이 업데이트해야 문서와 구현이 다시 맞습니다.

## 더 생각해볼 질문

- 지금 구조에서 가장 확장 친화적인 부분은 어디인가?
- 가장 확장에 취약한 부분은 어디인가?
- 성능, 설명 가능성, 코드 길이 중 무엇을 우선할 것인가?

## 생각 확장 방향

이 프로젝트는 "작지만 분리된 구조"를 목표로 만들었습니다. 그래서 지금은 작아도, 다음 단계에서 무엇을 바꿔야 할지 토론하기 좋은 출발점이 됩니다.
