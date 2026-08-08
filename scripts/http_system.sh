#!/usr/bin/env bash
# http_system.sh - fetch URLs, block disallowed links, and rewrite HTML so CSS loads before JS.
# Usage:
#   ./scripts/http_system.sh https://example.com
#   cat urls.txt | ./scripts/http_system.sh

set -euo pipefail
IFS=$'\n\t'

OUTPUT_DIR="."
BLOCK_LIST="tracker.example.com,bad.com"
URLS=()

function usage() {
  cat <<EOF
Usage: $0 [--output-dir DIR] [--block-list PATTERN1,PATTERN2] [URL ...]
Read URLs from stdin if no URL args are provided.
EOF
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --output-dir)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    --block-list)
      BLOCK_LIST="$2"
      shift 2
      ;;
    --help|-h)
      usage
      ;;
    *)
      URLS+=("$1")
      shift
      ;;
  esac
done

if [[ ${#URLS[@]} -eq 0 ]]; then
  while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    URLS+=("$line")
  done
fi

if [[ ${#URLS[@]} -eq 0 ]]; then
  usage
fi

mkdir -p "$OUTPUT_DIR"

function is_blocked_url() {
  local url="$1"
  IFS=',' read -r -a patterns <<< "$BLOCK_LIST"
  for pattern in "${patterns[@]}"; do
    if [[ "$url" == *"$pattern"* ]]; then
      return 0
    fi
  done
  return 1
}

function rewrite_html() {
  local html="$1"
  local css_tags
  local script_tags

  css_tags=$(grep -oP '(?i)<link[^>]+rel=["\'\"]?stylesheet["\'\"]?[^>]*>' <<< "$html" || true)
  script_tags=$(grep -oP '(?si)<script[^>]*>.*?</script>' <<< "$html" || true)

  html=$(sed -E 's@(?i)<link[^>]+rel=["\'\"]?stylesheet["\'\"]?[^>]*>@@g' <<< "$html")
  html=$(sed -E 's@(?si)<script[^>]*>.*?</script>@@g' <<< "$html")

  html=$(perl -pe 's{(<head[^>]*>)}$1\n'