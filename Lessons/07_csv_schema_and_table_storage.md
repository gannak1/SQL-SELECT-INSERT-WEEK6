# Lesson 07. CSV, schema, 테이블 저장

## 이 문서의 목적

이 프로젝트가 왜 `schema/*.schema`와 `data/*.csv`를 분리했는지, 그리고 그 둘이 실제 코드에서 어떻게 연결되는지 설명합니다.

## 실제 코드 위치

- `src/schema.c`
  `load_schema`, `find_column_index`, `free_schema`
- `src/csv_storage.c`
  `csv_append_row`, `csv_load_table`, `free_csv_table`
- `schema/users.schema`, `schema/books.schema`
  실제 schema 예시
- `data/*.csv`
  실행 중 생성되는 실제 데이터

## schema 파일의 역할

schema는 "정답표"입니다.

예:

```text
table=users
columns=id:INT,name:TEXT,age:INT
```

`load_schema`는 이 파일을 읽어 `TableSchema`를 만듭니다.

그 결과 executor는 아래를 알 수 있습니다.

- 테이블 이름
- 컬럼 순서
- 컬럼 이름
- 컬럼 타입

## 왜 schema와 CSV를 분리했는가

CSV는 값은 저장하지만 타입 정보는 없습니다.

예:

```csv
1,Alice,30
```

이 줄만 보면:

- 첫 번째 값이 id인지 모릅니다
- 세 번째 값이 숫자인지 문자열인지 모릅니다

그래서 schema가 필요합니다. schema는 의미를 주고, CSV는 실제 row 데이터를 보관합니다.

## load_schema가 하는 일

`src/schema.c`의 `load_schema`는 아래를 수행합니다.

1. `schema/<table>.schema` 파일 읽기
2. `table=` 줄 찾기
3. `columns=` 줄 찾기
4. `name:TYPE` 목록 파싱
5. `ColumnSchema` 배열 생성

helper 함수:

- `append_column`
- `parse_type_text`

## csv_append_row가 하는 일

INSERT가 성공하면 `csv_append_row`가 호출됩니다.

동작:

1. `data/<table>.csv` 경로 계산
2. 파일 존재 여부 확인
3. 없으면 header row 먼저 작성
4. 새 row append

예를 들어 `users` 테이블에 첫 INSERT가 들어오면:

```csv
id,name,age
1,Alice,30
```

이후 두 번째 INSERT부터는 header를 유지하고 데이터만 추가합니다.

## csv_load_table가 하는 일

SELECT 시에는 `csv_load_table`가 호출됩니다.

동작:

1. schema 기반 header 복사
2. `data/<table>.csv` 열기
3. 파일이 없으면 빈 테이블로 성공 반환
4. 첫 줄 header 검증
5. 이후 모든 row를 `CsvRow` 배열로 메모리에 적재

중요한 점:

- data 파일이 없더라도 에러가 아닙니다.
- schema만 있으면 빈 테이블로 취급합니다.

그래서 아래 출력이 가능합니다.

```text
RESULT: users
id,name,age
ROWS: 0
```

## 왜 CSV split을 직접 구현했는가

`split_csv_line`은 쉼표를 직접 세고 잘라서 셀 문자열 배열을 만듭니다.

이유:

- 이번 프로젝트는 CSV full escaping이 범위 밖입니다.
- `strtok`는 원본 문자열을 파괴합니다.
- 빈 셀 처리 규칙을 직접 통제하기 쉽습니다.

즉, "작은 범위에 맞는 가장 설명 가능한 구현"을 택한 것입니다.

## TableSchema와 CsvTable의 관계

- `TableSchema`
  컬럼 정의, 즉 구조
- `CsvTable`
  실제 row 값들

executor는 둘 다 필요합니다.

- schema는 타입과 컬럼 위치를 알려줍니다.
- csv table은 실제 비교 대상 row를 줍니다.

## 초보자 실수 포인트

- CSV header만 있으면 schema가 필요 없다고 생각하기
- data 파일이 없으면 SELECT가 실패해야 한다고 생각하기
- CSV가 "그냥 텍스트 파일"이라서 타입 검사가 필요 없다고 생각하기

현재 구현에서는 schema가 항상 기준이고, CSV는 schema를 따르는 저장소일 뿐입니다.

## 디버깅 포인트

- 테이블이 안 읽히면 `schema/<table>.schema` 경로 먼저 확인
- INSERT는 되는데 SELECT가 이상하면 header mismatch 여부 확인
- row 개수가 이상하면 `csv_load_table`의 줄 읽기와 blank line 처리 확인
- 쉼표 구분이 깨지면 `split_csv_line` 확인

## 더 생각해볼 질문

- CSV에 쉼표가 들어간 TEXT까지 지원하려면 split 로직이 얼마나 복잡해질까?
- schema lookup을 빠르게 하려면 `find_column_index`를 무엇으로 바꾸고 싶을까?
- CSV 대신 binary page format으로 바꾸면 `csv_storage.c`는 어떤 인터페이스를 유지하고 어떤 구현만 바뀌게 할 수 있을까?

## 생각 확장 방향

지금 구조는 "schema와 data 분리"라는 중요한 데이터베이스 사고를 보여줍니다. 앞으로 저장 포맷이 바뀌어도 이 경계는 계속 중요할 가능성이 큽니다.
