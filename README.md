# 미니SQL 프로젝트

SQL 형태의 쿼리를 입력받아 CSV 파일로 데이터를 저장하고 조회하는 C 언어 기반 미니 데이터베이스 프로젝트.

---

## 실행 방법

```text
miniSQL> SELECT name FROM table;
miniSQL> SELECT * FROM table WHERE id > 1;
miniSQL> INSERT 1, 'hong123', 'Hong', 'pass1234', '2026-04-08 18:30:00';
miniSQL> EXIT
```

- 쿼리 마지막에 반드시 `;` 을 붙여야 함
- 여러 줄 입력 가능 (`->` 프롬프트로 이어짐)
- `EXIT` 또는 `QUIT` 입력 시 종료

---

## 지원 쿼리

| 쿼리 | 형식 | 설명 |
|------|------|------|
| SELECT | `SELECT 컬럼 FROM 테이블;` | 데이터 조회 |
| SELECT * | `SELECT * FROM 테이블;` | 전체 컬럼 조회 |
| SELECT WHERE | `SELECT 컬럼 FROM 테이블 WHERE 컬럼 = 값;` | 조건 조회 |
| INSERT | `INSERT 값1, 값2, 값3, 값4, 값5;` | 데이터 삽입 |

### WHERE 비교 연산자

| 연산자 | 의미 |
|--------|------|
| `=` | 같음 |
| `>` | 초과 |
| `<` | 미만 |

---

## 스키마

테이블은 고정 스키마 5컬럼으로 구성됨. (`result/table.csv`)

| 순서 | 컬럼명 | 타입 | 예시 |
|------|--------|------|------|
| 1 | id | INTEGER | `1` |
| 2 | login_id | STRING | `'hong123'` |
| 3 | name | STRING | `'Hong'` |
| 4 | password | STRING | `'pass1234'` |
| 5 | created_at | STRING | `'2026-04-08 18:30:00'` |

### INSERT 값 타입 규칙

| 타입 | 형식 | 예시 |
|------|------|------|
| STRING | 작은따옴표로 감쌈 | `'hong123'` |
| INTEGER | 따옴표 없이 숫자만 | `1` |
| BOOLEAN | 따옴표 없이 TRUE/FALSE | `TRUE` |

---

## 프로젝트 구조

```text
├── main.c        입력 루프, 쿼리 라우팅
├── parser.c      문자열 파싱, 쿼리 구조체 생성
├── parser.h
├── storage.c     CSV 파일 읽기/쓰기
├── storage.h
├── printer.c     콘솔 출력
├── printer.h
├── utils.c       문자열 유틸 함수
├── utils.h
└── types.h       구조체 및 상수 정의
```

---

## 실행 흐름

```mermaid
flowchart TD
    A["사용자 입력"] --> B["main.c\nread_query_input()\n여러 줄 입력 합치기, ; 감지"]
    B --> C["parser.c\nparse_query()\n; 제거, 커맨드 추출, 페이로드 분리"]
    C --> D["SELECT\nparse_select_payload()\nFROM / WHERE 파싱"]
    C --> E["INSERT\nsplit_items()\n콤마 기준 값 분리"]
    E --> F["parse_insert_command()\n타입 판별\nSTRING / INTEGER / BOOLEAN"]
    D --> G["storage.c\nexecute_select_query()\nCSV 읽기, WHERE 필터, 결과 출력"]
    F --> H["storage.c\nappend_insert_to_csv()\nCSV 파일에 한 줄 추가"]
    G --> I["printer.c\nprint_query()\n파싱 결과 콘솔 출력"]
    H --> I
```

---

## 주요 함수 목록

### parser.c

| 함수 | 역할 |
|------|------|
| `parse_query()` | 입력 문자열을 Query 구조체로 파싱 |
| `parse_select_payload()` | SELECT 페이로드에서 컬럼/테이블/WHERE 분리 |
| `parse_insert_command()` | INSERT 값들을 타입 판별 후 InsertCommand에 저장 |
| `is_exit_command()` | EXIT/QUIT 여부 확인 |

### storage.c

| 함수 | 역할 |
|------|------|
| `execute_select_query()` | CSV 읽기, WHERE 필터링, 결과 출력 |
| `append_insert_to_csv()` | CSV 파일에 INSERT 데이터 추가 |
| `parse_csv_line()` | CSV 한 줄을 필드 배열로 분리 |
| `evaluate_where_condition()` | WHERE 조건 평가 |

### utils.c

| 함수 | 역할 |
|------|------|
| `trim_in_place()` | 문자열 앞뒤 공백 제거 |
| `to_upper_copy()` | 대문자 복사 |
| `is_integer_text()` | 정수 문자열 여부 확인 |
| `is_wrapped_with_single_quotes()` | 작은따옴표 감싸기 여부 확인 |
| `equals_ignore_case()` | 대소문자 무시 문자열 비교 |

---

## 제한 사항

- 테이블은 `table` 하나만 지원
- 스키마 고정 (컬럼 추가/변경 불가)
- WHERE 조건은 단일 조건만 지원 (AND/OR 미지원)
- Windows 전용 (`_wfopen_s`, `strncpy_s` 등 MSVC 함수 사용)
