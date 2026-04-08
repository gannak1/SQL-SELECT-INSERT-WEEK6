# ARCHITECTURE
## 권장 아키텍처와 데이터 흐름

이 문서는 “어떻게 나눠서 만들 것인가”를 설명합니다.

---

## 1. 큰 그림

이 프로젝트의 전체 흐름은 아래 한 줄로 요약됩니다.

```text
SQL 파일 → 파일 읽기 → 토큰화 → 파싱 → 구조화된 문장(AST) → 실행 → CSV 읽기/쓰기 → 출력
```

이 큰 그림이 코드에서도 보이게 만들어야 합니다.

---

## 2. 추천 모듈 구조

```text
main
 └─ file_reader
     └─ tokenizer
         └─ parser
             └─ executor
                 ├─ schema
                 ├─ csv_storage
                 └─ printer
```

---

## 3. 모듈별 책임

| 모듈 | 역할 |
|---|---|
| `main.c` | CLI 인자 확인, 전체 파이프라인 호출, 종료 코드 관리 |
| `file_reader.c` | SQL 파일 전체 읽기 |
| `tokenizer.c` | 문자 스트림을 토큰 배열/리스트로 변환 |
| `parser.c` | 토큰 시퀀스를 `INSERT`/`SELECT` 구조체로 변환 |
| `schema.c` | schema 파일 로딩, 컬럼/타입 정보 조회 |
| `csv_storage.c` | CSV 생성, append, row scan |
| `executor.c` | AST 실행, 타입 검사, WHERE 평가, projection 제어 |
| `printer.c` | SELECT 결과 출력 포맷 관리 |
| `error.c` | 에러 메시지 생성 또는 공통 에러 처리 유틸리티 |

---

## 4. 추천 데이터 구조

아래는 **추천**일 뿐이며, 이름은 약간 달라도 됩니다.  
중요한 것은 **구조가 분리되어 보이는 것**입니다.

### 4-1. Token
```c
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_COMMA,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SEMICOLON,
    TOKEN_OPERATOR,
    TOKEN_ASTERISK,
    TOKEN_EOF
} TokenType;
```

```c
typedef struct {
    TokenType type;
    char *lexeme;
} Token;
```

### 4-2. Value / Literal
```c
typedef enum {
    VALUE_INT,
    VALUE_TEXT
} ValueType;
```

```c
typedef struct {
    ValueType type;
    long int_value;
    char *text_value;
} LiteralValue;
```

### 4-3. Condition
```c
typedef enum {
    OP_EQ,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE
} OperatorType;
```

```c
typedef struct {
    char *column_name;
    OperatorType op;
    LiteralValue value;
} Condition;
```

### 4-4. Statement
```c
typedef enum {
    STMT_INSERT,
    STMT_SELECT
} StatementType;
```

#### INSERT
```c
typedef struct {
    char *table_name;
    LiteralValue *values;
    size_t value_count;
} InsertStatement;
```

#### SELECT
```c
typedef struct {
    int select_all;
    char **selected_columns;
    size_t selected_column_count;
    char *table_name;
    int has_where;
    Condition where;
} SelectStatement;
```

---

## 5. AST를 쓰는 이유

이 프로젝트 규모가 아주 크지는 않지만,  
토큰을 바로 실행해 버리면 아래 문제가 생깁니다.

- tokenizer / parser / executor 경계가 흐려짐
- WHERE나 projection 설명이 어려워짐
- Lessons에서 “파싱 결과”를 설명하기 힘들어짐
- 에러 위치가 애매해짐

따라서 **간단한 AST 또는 구조화된 statement struct**는 매우 유익합니다.

---

## 6. 추천 제어 흐름

### 6-1. main 흐름
1. CLI 인자 확인
2. SQL 파일 읽기
3. tokenize
4. parse program
5. statement들을 순서대로 execute
6. 성공 시 0, 실패 시 non-zero 반환

### 6-2. INSERT 실행 흐름
1. schema 로드
2. 값 개수 검사
3. 각 값 타입 검사
4. CSV 파일 생성 또는 열기
5. row append
6. 성공 메시지 출력

### 6-3. SELECT 실행 흐름
1. schema 로드
2. 선택 컬럼 유효성 검사
3. WHERE 컬럼 유효성 검사
4. CSV scan
5. 각 row에 WHERE 적용
6. projection 적용
7. 결과 출력

---

## 7. 메모리 소유권(ownership) 제안

C 프로젝트에서는 “누가 메모리를 만들고 누가 해제하는가”가 매우 중요합니다.

권장 방식은 아래와 같습니다.

- `file_reader`: SQL 전체 문자열을 할당하고 반환
- `tokenizer`: token 배열과 각 lexeme 복사본을 할당
- `parser`: AST용 문자열/배열/리터럴을 할당
- `executor`: 읽기 전용으로 AST 사용
- `main` 또는 상위 레벨 cleanup 함수: token / AST / file buffer 해제

즉, **생성 지점과 해제 지점이 문서화되어야** 합니다.

---

## 8. 문자열 처리 전략

이번 프로젝트에서 가장 버그가 나기 쉬운 부분은 문자열입니다.

대표 위험:
- buffer overflow
- null terminator 누락
- 작은따옴표 제거 과정 실수
- CSV split 과정 실수
- `strtok` 사용 시 원본 파괴 문제
- 동적 할당 후 해제 누락

가능하면:
- 입력 문자열을 복사해서 다루고
- 길이 기반 처리를 명확히 하며
- helper 함수를 작게 나누는 것이 좋습니다.

---

## 9. 에러 전파 전략

추천 전략은 단순합니다.

- 함수는 `int` 또는 `NULL`로 실패를 알림
- 상위 호출자가 에러 메시지를 출력하거나 전달
- 첫 에러에서 즉시 중단

프로젝트가 학습용 데모이므로, 지나치게 복잡한 복구 로직은 필요하지 않습니다.

---

## 10. Lessons에서 강조되어야 할 구조 포인트

Codex는 Lessons를 쓸 때 아래를 분명히 설명해야 합니다.

- tokenizer와 parser의 경계
- AST 또는 statement struct의 역할
- schema와 data file의 관계
- executor가 “DB처럼 보이는 행동”을 만드는 방식
- WHERE 때문에 생긴 비교 로직
- 포인터/배열/구조체/문자열/파일 I/O가 각각 어디서 중요한지

---

## 11. 확장 가능성을 남기는 아키텍처

이번 데모는 최소 구현이지만, 구조는 미래 확장을 암시해야 합니다.

예:
- WHERE 단일 조건 → 나중에는 expression tree로 확장 가능
- 전체 CSV scan → 나중에는 index/BST/hash로 확장 가능
- projection/selection 분리 → 관계형 연산 감각 연결 가능

즉, **코드는 작아도 생각은 확장 가능해야** 합니다.
