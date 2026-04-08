# Lessons 안내

이 폴더는 일반 SQL 입문서가 아니라 이 저장소 해설서입니다. 각 문서는 실제 파일명, 함수명, 구조체명을 기준으로 현재 구현을 따라가도록 작성했습니다.

추천 읽기 순서:

1. `00_start_here.md`
2. `01_big_picture_and_repo_map.md`
3. `03_cli_file_io_and_program_entry.md`
4. `04_tokenizer_in_this_project.md`
5. `05_parser_and_ast_in_this_project.md`
6. `06_execution_and_where_in_this_project.md`
7. `07_csv_schema_and_table_storage.md`
8. `08_memory_pointers_structs_and_strings.md`
9. `11_testing_debugging_and_common_bugs.md`

문서별 성격:

- `00` ~ `03`
  저장소를 어디서부터 읽을지와 CLI 진입 흐름을 잡아줍니다.
- `04` ~ `07`
  tokenizer, parser, executor, storage를 실제 코드 순서대로 설명합니다.
- `08` ~ `10`
  C 메모리, 자료구조, CSAPP 관점에서 구현을 다시 해석합니다.
- `11`, `12`
  테스트, 디버깅, 확장 아이디어를 정리합니다.

학습 팁:

- README와 `src/main.c`를 먼저 읽고 Lessons를 보면 훨씬 덜 추상적으로 느껴집니다.
- 각 Lesson에서 언급하는 함수 이름을 실제 코드에서 바로 검색해 함께 보세요.
