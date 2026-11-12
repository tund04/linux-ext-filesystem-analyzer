
#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
TMP_DIR="$(mktemp -d)"

trap 'rm -rf "$TMP_DIR"' EXIT

mkdir -p "$BUILD_DIR"

cd "$ROOT_DIR"

# Compiler flags
CFLAGS=(
    -Wall
    -Wextra
    -Wpedantic
    -Wformat=2
    -Werror
    -std=c11
    -I"$ROOT_DIR/include"
)

echo "===== BUILD METADATA MODULE ====="

# Biên dịch chương trình kiểm thử CLI
gcc "${CFLAGS[@]}" \
    src/metadata.c \
    tests/test_metadata.c \
    -o "$BUILD_DIR/test_metadata"

# Biên dịch chương trình kiểm thử API
gcc "${CFLAGS[@]}" \
    src/metadata.c \
    tests/test_metadata_api.c \
    -o "$BUILD_DIR/test_metadata_api"

echo "[PASS] Compilation"

# Chạy kiểm thử API
echo
echo "===== METADATA API TEST ====="

"$BUILD_DIR/test_metadata_api"

# Chạy kiểm thử CLI
echo
echo "===== METADATA CLI TEST ====="

BIN="$BUILD_DIR/test_metadata"

# Test 1: File thông thường
printf "Linux filesystem analyzer\n" > "$TMP_DIR/sample.txt"

chmod 640 "$TMP_DIR/sample.txt"

"$BIN" "$TMP_DIR/sample.txt" > "$TMP_DIR/output.txt"

grep -q "File type: Regular file" "$TMP_DIR/output.txt"
grep -q "Permission bits: 0640" "$TMP_DIR/output.txt"

EXPECTED_INODE=$(stat -c '%i' "$TMP_DIR/sample.txt")
EXPECTED_SIZE=$(stat -c '%s' "$TMP_DIR/sample.txt")

ACTUAL_INODE=$(awk -F': ' '/^Inode number:/ {print $2}' "$TMP_DIR/output.txt")

ACTUAL_SIZE=$(awk -F': ' '/^File size:/ {print $2}' "$TMP_DIR/output.txt" | awk '{print $1}')

[[ "$EXPECTED_INODE" == "$ACTUAL_INODE" ]]
[[ "$EXPECTED_SIZE" == "$ACTUAL_SIZE" ]]

echo "[PASS] Regular file"

# Test 2: Hard link
ln "$TMP_DIR/sample.txt" "$TMP_DIR/hard_link.txt"

"$BIN" "$TMP_DIR/sample.txt" > "$TMP_DIR/output.txt"

EXPECTED_LINKS=$(stat -c '%h' "$TMP_DIR/sample.txt")

ACTUAL_LINKS=$(awk -F': ' '/^Link count:/ {print $2}' "$TMP_DIR/output.txt")

[[ "$EXPECTED_LINKS" == "$ACTUAL_LINKS" ]]

echo "[PASS] Hard link count"

# Test 3: File rỗng
: > "$TMP_DIR/empty.txt"

"$BIN" "$TMP_DIR/empty.txt" > "$TMP_DIR/output.txt"

grep -q "File size: 0 bytes" "$TMP_DIR/output.txt"

echo "[PASS] Empty file"

# Test 4: Directory
"$BIN" "$TMP_DIR" > "$TMP_DIR/output.txt"

grep -q "File type: Directory" "$TMP_DIR/output.txt"

echo "[PASS] Directory"

# Test 5: FIFO / Pipe
mkfifo "$TMP_DIR/sample_fifo"

"$BIN" "$TMP_DIR/sample_fifo" > "$TMP_DIR/output.txt"

grep -q "File type: FIFO / Pipe" "$TMP_DIR/output.txt"

echo "[PASS] FIFO"

# Test 6: Character device
"$BIN" /dev/null > "$TMP_DIR/output.txt"

grep -q "File type: Character device" "$TMP_DIR/output.txt"

echo "[PASS] Character device"

# Test 7: Symbolic link
ln -s sample.txt "$TMP_DIR/sample_link"

"$BIN" "$TMP_DIR/sample_link" > "$TMP_DIR/output.txt"

grep -q "File type: Regular file" "$TMP_DIR/output.txt"

echo "[PASS] Symbolic link follows target"

# Test 8: Broken symbolic link
ln -s "$TMP_DIR/no_target" "$TMP_DIR/broken_link"

if "$BIN" "$TMP_DIR/broken_link" > /dev/null 2>&1; then
    echo "[FAIL] Broken symbolic link"
    exit 1
fi

echo "[PASS] Broken symbolic link"

# Test 9: File không tồn tại
if "$BIN" "$TMP_DIR/not_found.txt" > /dev/null 2>&1; then
    echo "[FAIL] Missing file"
    exit 1
fi

echo "[PASS] Missing file"

# Test 10: Không có tham số
if "$BIN" > /dev/null 2>&1; then
    echo "[FAIL] Missing argument"
    exit 1
fi

echo "[PASS] Missing argument"

# Test 11: Thừa tham số
if "$BIN" "$TMP_DIR/sample.txt" extra > /dev/null 2>&1; then
    echo "[FAIL] Extra argument"
    exit 1
fi

echo "[PASS] Extra argument"

echo
echo "================================"
echo "ALL METADATA TESTS PASSED"
echo "================================"
