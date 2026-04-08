# 파일 목적:
#   Ubuntu + gcc + make 환경에서 프로젝트를 빌드하고 테스트하기 위한 규칙을 제공한다.
#
# 전체 흐름에서 위치:
#   사용자는 make, make test, make run-demo 같은 명령으로
#   실행 파일 생성과 검증을 한 번에 수행할 수 있다.
#
# 주요 내용:
#   - sql_processor 빌드
#   - unit_tests 빌드
#   - 통합 테스트 실행
#   - data 초기화와 clean

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L
INCLUDES = -Iinclude -Itests

COMMON_SOURCES = \
	src/common.c \
	src/error.c \
	src/file_reader.c \
	src/token.c \
	src/ast.c \
	src/tokenizer.c \
	src/parser.c \
	src/schema.c \
	src/csv_storage.c \
	src/printer.c \
	src/executor.c \
	src/sql_runner.c

APP_SOURCES = $(COMMON_SOURCES) src/main.c
TEST_SOURCES = $(COMMON_SOURCES) tests/test_main.c tests/test_tokenizer.c tests/test_parser.c tests/test_executor.c

APP_BINARY = sql_processor
TEST_BINARY = unit_tests

.PHONY: all clean test run-demo

all: $(APP_BINARY)

$(APP_BINARY): $(APP_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(APP_SOURCES) -o $(APP_BINARY)

$(TEST_BINARY): $(TEST_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_SOURCES) -o $(TEST_BINARY)

test: $(APP_BINARY) $(TEST_BINARY)
	./$(TEST_BINARY)
	./tests/integration_test.sh

run-demo: $(APP_BINARY)
	./scripts/reset_data.sh
	./$(APP_BINARY) sample_sql/09_full_demo.sql

clean:
	rm -f $(APP_BINARY) $(TEST_BINARY)
	rm -f data/*.csv
	rm -rf tests/tmp
