# 사람용 시작 안내

이 문서는 완성된 저장소를 팀이 빠르게 읽고 시연하기 위한 안내서입니다.

## 먼저 해볼 것

빌드:

```bash
make
```

테스트:

```bash
make test
```

데모:

```bash
./scripts/reset_data.sh
./sql_processor sample_sql/09_full_demo.sql
```

## 이 저장소를 한 줄로 설명하면

`schema`와 `CSV`를 사용하는 아주 작은 SQL 처리기를 C로 직접 구현하고, 그 구현을 학습 문서까지 연결한 저장소입니다.

## 사람 기준 추천 읽기 순서

1. `README.md`
2. `src/main.c`
3. `src/sql_runner.c`
4. `src/tokenizer.c`
5. `src/parser.c`
6. `src/executor.c`
7. `src/schema.c`, `src/csv_storage.c`
8. `tests/`
9. `Lessons/`

## 팀이 바로 확인할 핵심 포인트

- `INSERT`, `SELECT`, `WHERE(단일 조건)`가 실제로 동작하는가
- 파일 모드, `-e`, REPL이 같은 실행 파이프라인을 공유하는가
- 출력 형식이 `expected_output/`과 일치하는가
- tokenizer / parser / executor / storage 경계가 코드에서 선명한가
- 주석이 "무엇을 하나"뿐 아니라 "왜 필요한가"를 설명하는가
- Lessons가 실제 함수명과 파일명을 기준으로 쓰였는가

## Docker / Dev Container와의 관계

이 프로젝트는 `.devcontainer/`와 `.vscode/` 설정이 포함돼 있고, 최종 실행 기준도 그 Ubuntu 기반 Docker 환경입니다.

- `.devcontainer/Dockerfile`
  `gcc`, `make`, `gdb`, `valgrind`가 설치됩니다.
- `.devcontainer/devcontainer.json`
  팀 컨테이너 안에서 바로 C 개발을 시작할 수 있게 설정돼 있습니다.
- `.vscode/tasks.json`, `.vscode/launch.json`
  컨테이너 안에서 빌드와 디버깅을 연결합니다.

즉, 로컬 환경이 달라도 `make`, `make test`, `./sql_processor ...` 동작 기준은 이 Docker 환경에 맞춰 두었습니다.

## 읽으면서 던져볼 질문

- 파일 모드, `-e`, REPL은 어디서 갈라지고 어디서 다시 하나로 합쳐질까?
- `run_sql_text -> tokenize_sql -> parse_program -> execute_program` 흐름이 왜 분리되어 있을까?
- `WHERE`가 들어오자 executor가 어떤 책임을 새로 가지게 되었을까?
- `load_schema`가 없다면 타입 검사는 어디에서 할 수 있을까?
- `csv_load_table`의 선형 스캔은 언제 병목이 될까?
- `AND`, `OR`, `ORDER BY`를 넣으려면 어느 파일부터 바뀌어야 할까?

## 팀 토론에 바로 쓰기 좋은 주제

- 지금 구조에서 expression tree를 도입하면 parser가 어떻게 바뀌는가
- `find_column_index`의 선형 탐색을 다른 자료구조로 바꾸면 무엇이 좋아지는가
- CSV 대신 binary page format으로 가면 `csv_storage.c` 자리에 무엇이 들어가야 하는가
- `make test` 외에 valgrind를 Docker에서 어떻게 붙일 것인가
