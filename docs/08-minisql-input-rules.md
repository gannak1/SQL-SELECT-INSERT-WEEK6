# MiniSQL 입력 규칙

## 목적
이 문서는 인터랙티브 CLI에서 입력받는 MiniSQL 문장의 해석 규칙을 고정한다.
파서 구현과 사용자 입력 처리 로직은 이 문서를 기준으로 한다.

## 입력 방식
- 프로그램은 `MiniSQL> ` 프롬프트를 출력한다.
- 사용자는 MiniSQL 문장을 한 줄씩 입력할 수 있다.
- 세미콜론 `;`이 나올 때까지 여러 줄을 누적해서 하나의 MiniSQL 문장으로 처리한다.
- 문장이 아직 끝나지 않았으면 다음 줄에서는 `...> ` 프롬프트를 출력한다.
- 빈 줄은 조용히 무시하고 다음 프롬프트를 출력한다.
- EOF(`Ctrl + D`) 입력 시 프로그램은 조용히 종료한다.

## 종료 명령
- `exit`
- `quit`

위 두 입력은 MiniSQL 문장이 아니라 CLI 제어 명령으로 취급한다.
사용자가 정확히 `exit` 또는 `quit`를 입력하면 프로그램은 종료한다.
입력 누적 중이더라도, 한 줄에 `exit` 또는 `quit`만 입력하면 누적 버퍼를 더 처리하지 않고 바로 종료한다.

## 키워드 규칙
- 키워드는 대소문자를 구분하지 않는다.

예:
- `select * from users;`
- `SELECT * FROM users;`
- `SeLeCt * FrOm users;`

위 입력은 모두 같은 의미로 처리한다.

## 세미콜론 규칙
- `INSERT`, `SELECT` 같은 MiniSQL 문장 끝에는 세미콜론 `;`이 반드시 있어야 한다.
- 세미콜론이 아직 없으면 REPL은 문장이 끝나지 않았다고 보고 다음 줄 입력을 계속 받는다.
- `exit`, `quit`는 세미콜론 없이 입력한다.

예:
- 정상: `SELECT * FROM users;`
- 오류: `SELECT * FROM users`

## 공백 규칙
- 공백은 유연하게 허용한다.
- 키워드 사이에 공백이 여러 개 있어도 허용한다.
- 문장 앞뒤 공백도 허용한다.

예:
- `SELECT * FROM users;`
- `  SELECT   *   FROM   users ;`

## 값 규칙
### 숫자
- 숫자는 따옴표 없이 입력한다.

예:
- `1`
- `25`

### 문자열
- 문자열은 작은따옴표 `'` 또는 큰따옴표 `"`로 감쌀 수 있다.
- 문자열의 시작과 끝은 같은 종류의 따옴표여야 한다.

예:
- `'kim01'`
- `"kim01"`
- `'Kim'`
- `"Kim"`
- `'010-1234-5678'`
- `'kim@example.com'`
- `""`

### 문자열 내부 특수문자
- 전화번호 하이픈 `-`, 이메일의 `@`, 점 `.` 같은 특수문자는 일반 문자열의 일부로 취급한다.
- 공백과 쉼표도 따옴표 안에 있으면 별도 의미 없이 그대로 문자열 값으로 취급한다.

예:
- `'010-1234-5678'`
- `'kim@example.com'`
- `'Kim, Min'`
- `"Kim Min"`

### 빈 문자열
- 빈 문자열을 허용한다.

예:
- `''`
- `""`

### 문자열 내부 같은 종류의 따옴표
- 1차 구현에서는 문자열을 감싸는 것과 같은 종류의 따옴표를 문자열 내부에 넣는 것을 지원하지 않는다.

예:
- 미지원: `'O'Brien'`
- 미지원: `"He said "hello""`

### 줄바꿈
- 문자열 내부 줄바꿈은 1차 구현에서 지원하지 않는다.

## 파싱 오류 처리 규칙
- 파싱 오류가 발생하면 오류 메시지를 출력한다.
- 현재 입력 문장은 실행하지 않는다.
- 프로그램은 종료하지 않고 다음 프롬프트를 유지한다.

예:
```text
MiniSQL> SELECT FROM users;
Error: invalid SELECT syntax
MiniSQL>
```

## 1차 오류 메시지 정책
1차 구현에서는 오류 메시지를 아래 범주로 구분한다.

### 문법 오류
- `Error: missing semicolon`
- `Error: unsupported command`
- `Error: invalid INSERT syntax`
- `Error: invalid SELECT syntax`
- `Error: invalid WHERE syntax`
- `Error: unterminated string literal`
- `Error: unsupported quoted string format`
- `Error: out of memory`

### 지원 범위 오류
- `Error: unsupported table`
- `Error: unsupported SELECT columns`
- `Error: unsupported WHERE condition`

### 값/타입 오류
- `Error: INSERT expects 5 values`
- `Error: invalid numeric value for id`
- `Error: invalid numeric value for age`

### 파일 입출력 오류
- `Error: data file not found`
- `Error: failed to read data file`
- `Error: failed to write data file`

## 정상 메시지 정책
- INSERT 성공 시: `Inserted 1 row`
- SELECT 결과가 0건일 때: `No rows found`
- SELECT 결과가 있으면 헤더와 행을 출력하고, 필요하면 마지막에 선택된 행 수를 출력할 수 있다.

## 1차 구현에서 지원하는 MiniSQL 범위
### INSERT
```sql
INSERT INTO users VALUES ('kim01', 'Kim', 25, '010-1234-5678', 'kim@example.com');
```

설명:
- 사용자는 `username`, `name`, `age`, `phone`, `email`의 5개 값만 입력한다.
- `id`는 storage 단계에서 자동 증가로 생성된다.

### SELECT ALL
```sql
SELECT * FROM users;
```

### SELECT WHERE
```sql
SELECT * FROM users WHERE id = 1;
SELECT * FROM users WHERE name = 'Kim';
SELECT * FROM users WHERE age = 25;
```

## 1차 구현에서 아직 지원하지 않는 것
- 문자열 내부 같은 종류의 따옴표
- 문자열 내부 줄바꿈
- 복합 조건 `AND`, `OR`
- 비교 연산자 `>`, `<`, `>=`, `<=`
- `LIKE`
- 컬럼 목록 선택 `SELECT id, name FROM users;`
