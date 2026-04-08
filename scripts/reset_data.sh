#!/usr/bin/env bash
set -euo pipefail

# 간단한 data 초기화 스크립트
# Codex가 최종 프로젝트를 만들 때 이 스크립트를 유지하거나
# Makefile target으로 대체해도 된다.

rm -f data/*.csv
echo "data directory reset complete"
