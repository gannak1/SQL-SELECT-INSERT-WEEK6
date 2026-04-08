# INSERT와 SELECT가 실제로 어떻게 동작하는가

## 1. SELECT가 실행되는 흐름
예:

```sql
SELECT * FROM users WHERE id = 1;
```

흐름:
1. [repl.c](../src/repl.c) 가 입력을 받는다.
2. `process_input_line()`가 [parser.c](../src/parser.c)를 호출한다.
3. `parse_select()`가 이 문장을 `Command` 구조체로 바꾼다.
4. [executor.c](../src/executor.c)가 `execute_select()`를 호출한다.
5. 조건이 `id`면 [storage.c](../src/storage.c)가 B-tree 인덱스로 파일 위치를 찾고, 다른 컬럼이면 전체 행을 읽는다.
6. 필요한 행을 찾아서 조건과 비교한다.
7. 맞는 행을 [printer.c](../src/printer.c)로 출력한다.

즉:

`입력 -> 파싱 -> 실행 -> 파일 읽기 -> 조건 검사 -> 출력`

## 2. INSERT가 실행되는 흐름
예:

```sql
INSERT INTO users VALUES ("hong13", "Hong", 26, "010-1313-1414", "hong@example.com");
```

흐름:
1. [repl.c](../src/repl.c) 가 입력을 받는다.
2. [parser.c](../src/parser.c) 가 `INSERT` 문장을 해석한다.
3. 값 5개를 `InsertCommand` 구조체에 저장한다.
4. [executor.c](../src/executor.c) 가 값 개수와 숫자 타입을 검사한다.
5. [storage.c](../src/storage.c) 가 새 `id`를 만든 뒤 CSV 한 줄을 추가한다.
6. [printer.c](../src/printer.c) 가 `Inserted 1 row`를 출력한다.

즉:

`입력 -> 파싱 -> 값 검사 -> 파일에 저장 -> 성공 메시지 출력`

## 3. 왜 parser와 executor를 나눴는가
처음 보면 한 파일에서 다 해도 될 것 같지만, 역할이 다르다.

- `parser`
  - 문장을 읽어서 의미를 해석
- `executor`
  - 해석된 결과를 실제 행동으로 연결

예:
- `parser`는 "`SELECT`구나"를 알아내는 역할
- `executor`는 "그럼 CSV를 읽어야겠네"를 결정하는 역할

## 4. 공부할 때 가장 중요한 포인트
이 프로젝트를 이해할 때는 모든 코드를 한 번에 외우려 하지 말고 아래 4가지만 먼저 잡으면 된다.

1. 입력은 어디서 들어오는가
2. 문자열은 어디서 구조체로 바뀌는가
3. 파일은 어디서 읽고 쓰는가
4. 결과는 어디서 출력되는가

이 4개가 잡히면 전체 구조가 훨씬 덜 어렵게 보인다.
