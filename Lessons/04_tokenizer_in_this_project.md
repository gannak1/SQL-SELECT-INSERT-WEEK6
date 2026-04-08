# Lesson 04. 이 프로젝트의 tokenizer

## 이 문서의 목적

`src/tokenizer.c`의 `tokenize_sql`이 실제로 어떤 토큰을 만들고 어떤 규칙으로 문자열을 나누는지 설명합니다.

## 실제 코드 위치

- `include/token.h`
  `TokenType`, `Token`, `TokenArray`
- `include/tokenizer.h`
  `tokenize_sql` 선언
- `src/tokenizer.c`
  실제 tokenizer 구현
- `src/token.c`
  `token_type_name`, `free_token_array`

## 어떤 토큰들이 있는가

현재 지원 토큰은 아래와 같습니다.

- `TOKEN_KEYWORD`
- `TOKEN_IDENTIFIER`
- `TOKEN_INT_LITERAL`
- `TOKEN_STRING_LITERAL`
- `TOKEN_COMMA`
- `TOKEN_LPAREN`
- `TOKEN_RPAREN`
- `TOKEN_SEMICOLON`
- `TOKEN_OPERATOR`
- `TOKEN_ASTERISK`
- `TOKEN_EOF`

이 작은 집합만으로도 현재 프로젝트 범위는 충분히 표현됩니다.

## tokenize 흐름

`tokenize_sql`은 SQL 문자열을 왼쪽에서 오른쪽으로 한 글자씩 읽습니다.

큰 분기는 아래와 같습니다.

1. 공백이면 건너뜀
2. identifier 시작 문자면 단어를 끝까지 읽음
3. 작은따옴표면 string literal 끝까지 읽음
4. 숫자 또는 `-` 뒤 숫자면 int literal 읽음
5. `,`, `(`, `)`, `;`, `*` 같은 단일 문자 토큰 처리
6. `=`, `!=`, `<`, `>`, `<=`, `>=` 처리
7. 그 외 문자는 에러

마지막에는 반드시 `TOKEN_EOF`를 하나 넣습니다.

## keyword와 identifier 구분

`src/tokenizer.c`의 `is_keyword`는 아래 여섯 keyword를 인식합니다.

- `INSERT`
- `INTO`
- `VALUES`
- `SELECT`
- `FROM`
- `WHERE`

구현에서 중요한 점:

- keyword는 대문자로 정규화합니다.
- identifier는 소문자로 정규화합니다.

그래서 parser는 `strcmp(token->lexeme, "SELECT")` 같은 단순 비교를 쓸 수 있습니다.

## 문자열 리터럴 처리

작은따옴표를 만나면 tokenizer는 다음 작은따옴표까지 읽고, 따옴표를 제외한 내부 문자열만 `TOKEN_STRING_LITERAL.lexeme`에 저장합니다.

예:

```sql
'Alice'
```

토큰 내부 문자열:

```text
Alice
```

이 프로젝트에서는 TEXT를 단순화했기 때문에 tokenizer 단계에서 아래도 같이 막습니다.

- 줄바꿈 포함 금지
- 쉼표 포함 금지

## 공백과 줄바꿈 처리

공백은 무시하지만, `line`과 `column`은 계속 갱신합니다.

이 정보는 나중에 parser 오류 메시지에 사용됩니다.

예를 들어 토큰 자체에는 아래 정보가 같이 들어갑니다.

- `type`
- `lexeme`
- `line`
- `column`

## 메모리 관점에서 중요한 함수

- `push_token`
  동적 토큰 배열을 늘립니다.
- `sql_strndup`
  토큰 lexeme 일부를 복사합니다.
- `free_token_array`
  각 토큰 문자열까지 함께 해제합니다.

여기서 가장 중요한 것은 lexeme이 모두 heap 복사본이라는 점입니다.

## 에러 처리

대표 에러는 아래와 같습니다.

- 알 수 없는 문자
- 닫히지 않은 문자열 literal
- 문자열 literal 안의 줄바꿈
- 문자열 literal 안의 쉼표
- `!` 단독 사용

토큰화 실패 시 `SqlError`에 메시지를 남기고 즉시 중단합니다.

## 초보자 실수 포인트

- tokenizer가 SQL 의미까지 이해한다고 착각하기
- 음수 `-7`을 두 토큰으로 쪼개야 한다고 생각하기
- string literal 안의 따옴표 제거를 parser가 한다고 생각하기
- EOF 토큰의 필요성을 과소평가하기

## 디버깅 포인트

- 토큰 개수가 이상하면 `tests/test_tokenizer.c`
- keyword/identifier 대소문자가 이상하면 정규화 helper 확인
- 문자열 literal이 깨지면 `sql_strndup` 길이 계산 확인
- 줄 번호가 이상하면 공백 처리 구간의 `line`, `column` 증가 로직 확인

## 더 생각해볼 질문

- 토큰 타입을 더 늘리면 parser는 어떻게 바뀔까?
- 나중에 `AND`, `OR`, `ORDER BY`를 넣으려면 어떤 keyword와 토큰이 추가될까?
- 지금은 keyword를 tokenizer에서 구분하는데, parser에서 구분하도록 미루면 어떤 장단점이 있을까?

## 생각 확장 방향

현재 tokenizer는 hand-written 방식입니다. 규모가 커지면 더 체계적인 scanner가 필요할 수 있지만, 지금 단계에서는 직접 구현이 구조를 이해하기 가장 좋습니다.
