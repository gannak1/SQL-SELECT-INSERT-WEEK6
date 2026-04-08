#!/usr/bin/env bash
set -euo pipefail

# 파일 목적:
#   실제 CLI 바이너리를 sample_sql 입력과 expected_output 비교로 검증한다.
#
# 전체 흐름에서 위치:
#   make test의 두 번째 단계로 실행되며,
#   tokenizer부터 executor까지 전체 파이프라인을 end-to-end로 확인한다.
#
# 주요 내용:
#   - 정상 케이스 stdout 비교
#   - 에러 케이스 non-zero 종료 코드와 메시지 비교
#   - 파일 모드, -e 모드, REPL 모드 회귀 확인

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run_success_case() {
    local sql_file="$1"
    local expected_file="$2"
    local output_file

    output_file="$(mktemp)"
    "${ROOT_DIR}/sql_processor" "${ROOT_DIR}/${sql_file}" >"${output_file}"
    diff -u "${ROOT_DIR}/${expected_file}" "${output_file}"
    rm -f "${output_file}"
}

run_string_success_case() {
    local sql_text="$1"
    local expected_file="$2"
    local output_file

    output_file="$(mktemp)"
    "${ROOT_DIR}/sql_processor" -e "${sql_text}" >"${output_file}"
    diff -u "${ROOT_DIR}/${expected_file}" "${output_file}"
    rm -f "${output_file}"
}

run_string_error_case() {
    local sql_text="$1"
    local expected_file="$2"
    local output_file

    output_file="$(mktemp)"
    if "${ROOT_DIR}/sql_processor" -e "${sql_text}" >"${output_file}" 2>&1; then
        echo "Expected failure for -e SQL input, but command succeeded." >&2
        cat "${output_file}" >&2
        rm -f "${output_file}"
        exit 1
    fi

    diff -u "${ROOT_DIR}/${expected_file}" "${output_file}"
    rm -f "${output_file}"
}

run_repl_success_case() {
    local repl_input="$1"
    local expected_file="$2"
    local output_file

    output_file="$(mktemp)"
    printf "%s" "${repl_input}" | "${ROOT_DIR}/sql_processor" --repl >"${output_file}" 2>/dev/null
    diff -u "${ROOT_DIR}/${expected_file}" "${output_file}"
    rm -f "${output_file}"
}

run_error_case() {
    local sql_file="$1"
    local expected_file="$2"
    local output_file

    output_file="$(mktemp)"
    if "${ROOT_DIR}/sql_processor" "${ROOT_DIR}/${sql_file}" >"${output_file}" 2>&1; then
        echo "Expected failure for ${sql_file}, but command succeeded." >&2
        cat "${output_file}" >&2
        rm -f "${output_file}"
        exit 1
    fi

    diff -u "${ROOT_DIR}/${expected_file}" "${output_file}"
    rm -f "${output_file}"
}

cd "${ROOT_DIR}"

./scripts/reset_data.sh >/dev/null
run_success_case "sample_sql/01_users_bootstrap.sql" "expected_output/01_users_bootstrap.txt"
run_success_case "sample_sql/02_users_projection.sql" "expected_output/02_users_projection.txt"
run_success_case "sample_sql/03_users_where_int.sql" "expected_output/03_users_where_int.txt"
run_repl_success_case $'SELECT id, name\nFROM users\nWHERE age >= 29;\nquit\n' "expected_output/03_users_where_int.txt"
run_success_case "sample_sql/04_users_where_text.sql" "expected_output/04_users_where_text.txt"
run_success_case "sample_sql/05_books_demo.sql" "expected_output/05_books_demo.txt"
run_error_case "sample_sql/06_error_unknown_column.sql" "expected_output/06_error_unknown_column.txt"
run_error_case "sample_sql/07_error_type_mismatch.sql" "expected_output/07_error_type_mismatch.txt"
run_error_case "sample_sql/08_error_unsupported_text_compare.sql" "expected_output/08_error_unsupported_text_compare.txt"

./scripts/reset_data.sh >/dev/null
run_string_success_case "$(cat "${ROOT_DIR}/sample_sql/09_full_demo.sql")" "expected_output/09_full_demo.txt"
run_string_error_case "$(cat "${ROOT_DIR}/sample_sql/06_error_unknown_column.sql")" "expected_output/06_error_unknown_column.txt"

./scripts/reset_data.sh >/dev/null
run_success_case "sample_sql/09_full_demo.sql" "expected_output/09_full_demo.txt"

echo "Integration tests passed."
