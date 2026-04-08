# SAMPLE_RUNS
## 사람이 바로 돌려볼 수 있는 예시 시나리오

이 문서는 Codex가 데모를 만들 때 참고할 수 있는 실행 예시입니다.

---

## 1. 빌드

```bash
make
```

---

## 2. data 초기화

```bash
rm -f data/*.csv
```

---

## 3. users bootstrap

```bash
./sql_processor sample_sql/01_users_bootstrap.sql
```

기대 출력:

```text
OK: 1 row inserted into users
OK: 1 row inserted into users
OK: 1 row inserted into users
RESULT: users
id,name,age
1,Alice,30
2,Bob,24
3,Charlie,29
ROWS: 3
```

---

## 4. projection 확인

```bash
./sql_processor sample_sql/02_users_projection.sql
```

기대 출력:

```text
RESULT: users
id,name
1,Alice
2,Bob
3,Charlie
ROWS: 3
```

---

## 5. WHERE INT 확인

```bash
./sql_processor sample_sql/03_users_where_int.sql
```

기대 출력:

```text
RESULT: users
id,name
1,Alice
3,Charlie
ROWS: 2
```

---

## 6. WHERE TEXT 확인

```bash
./sql_processor sample_sql/04_users_where_text.sql
```

기대 출력:

```text
RESULT: users
id,name,age
2,Bob,24
ROWS: 1
```

---

## 7. books 테이블 확인

```bash
./sql_processor sample_sql/05_books_demo.sql
```

기대 출력:

```text
OK: 1 row inserted into books
OK: 1 row inserted into books
RESULT: books
title,author
CSAPP,Bryant
ROWS: 1
```

---

## 8. 에러 케이스: 없는 컬럼

```bash
./sql_processor sample_sql/06_error_unknown_column.sql
```

기대 출력 예:

```text
ERROR: column not found: nickname
```

종료 코드는 non-zero

---

## 9. 에러 케이스: 타입 불일치

```bash
./sql_processor sample_sql/07_error_type_mismatch.sql
```

기대 출력 예:

```text
ERROR: type mismatch in WHERE for column age
```

---

## 10. 에러 케이스: TEXT 비교 미지원

```bash
./sql_processor sample_sql/08_error_unsupported_text_compare.sql
```

기대 출력 예:

```text
ERROR: unsupported TEXT comparison operator: >
```

---

## 11. 전체 데모

```bash
rm -f data/*.csv
./sql_processor sample_sql/09_full_demo.sql
```

Codex는 이 데모가 **설명 가능한 범위 안에서 안정적으로 동작하도록** 구현해야 합니다.
