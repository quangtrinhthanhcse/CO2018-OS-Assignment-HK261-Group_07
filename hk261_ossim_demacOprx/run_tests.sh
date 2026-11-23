#!/usr/bin/env bash
# ==============================================================================
# Script tự động biên dịch, chạy toàn bộ bộ test và đối chiếu với output/ mẫu
# Phụ trách: Nguyễn Khắc Triệu (MSSV: 2213619) - Nhiệm vụ A1-5 & A1-7
# Cách dùng:
#   cd hk261_ossim_demacOprx && ./run_tests.sh [--normalize]
#   Hoặc chạy từ thư mục gốc repo: ./hk261_ossim_demacOprx/run_tests.sh
# ==============================================================================

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

NORMALIZE=0
if [[ "${1:-}" == "--normalize" || "${1:-}" == "-n" ]]; then
    NORMALIZE=1
fi

OUT_ACTUAL_DIR="output_actual"
DIFF_DIR="output_diff"
mkdir -p "$OUT_ACTUAL_DIR" "$DIFF_DIR"

TESTS=(
    "sched"
    "sched_0"
    "sched_1"
    "os_1_singleCPU_mlq"
    "os_sc"
    "os_syscall"
    "os_syscall_list"
    "os_0_mlq_paging"
    "os_1_singleCPU_mlq_paging"
    "os_1_mlq_paging"
    "os_1_mlq_paging_small_1K"
    "os_1_mlq_paging_small_4K"
    "os_2_singleCPU_mlq_paging"
    "os_2_mlq_paging"
)

echo "================================================================"
echo " [1/3] Biên dịch toàn bộ hệ thống (make clean && make all)"
echo "================================================================"
make clean > /dev/null 2>&1
if ! make all; then
    echo ""
    echo "[LỖI BIÊN DỊCH] 'make all' thất bại! Vui lòng kiểm tra danh sách lỗi tích hợp (docs/integration_review_A1.md)."
    exit 2
fi

echo ""
echo "================================================================"
echo " [2/3] Chạy ${#TESTS[@]} kịch bản kiểm thử và so sánh với output/"
echo "================================================================"

 normalize_log() {
    # Chuẩn hóa địa chỉ con trỏ bảng trang động (ASLR) để so sánh cấu trúc log
    sed -E \
        -e 's/PDG=[0-9a-fA-Fx ]+/PDG=<ADDR>/g' \
        -e 's/P4[gD]=[0-9a-fA-Fx ]+/P4D=<ADDR>/g' \
        -e 's/PUD=[0-9a-fA-Fx ]+/PUD=<ADDR>/g' \
        -e 's/PMD=[0-9a-fA-Fx ]+/PMD=<ADDR>/g' \
        -e 's/PT=[0-9a-fA-Fx ]+/PT=<ADDR>/g' \
        "$1"
}

PASS_COUNT=0
FAIL_COUNT=0
FAILED_TESTS=()

printf "%-28s | %-10s | %s\n" "Kịch bản (Test Case)" "Kết quả" "Chi tiết"
echo "-----------------------------+------------+-------------------------------------"

for t in "${TESTS[@]}"; do
    EXPECTED="output/${t}.output"
    ACTUAL="${OUT_ACTUAL_DIR}/${t}.output"
    DIFF_FILE="${DIFF_DIR}/${t}.diff"

    if [[ ! -f "input/${t}" ]]; then
        printf "%-28s | %-10s | Không tìm thấy input/%s\n" "$t" "SKIP" "$t"
        continue
    fi

    # Chạy mô phỏng với timeout 15 giây để tránh treo vô hạn (deadlock/infinite loop)
    if command -v timeout > /dev/null 2>&1; then
        timeout 15s ./os "$t" > "$ACTUAL" 2>&1
        RET=$?
    else
        ./os "$t" > "$ACTUAL" 2>&1
        RET=$?
    fi

    if [[ $RET -eq 124 ]]; then
        printf "%-28s | %-10s | TIMEOUT (>15s - Nghi ngờ Deadlock)\n" "$t" "FAIL"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_TESTS+=("$t (TIMEOUT)")
        continue
    elif [[ $RET -ne 0 ]]; then
        printf "%-28s | %-10s | Exit code %d (Crash / Segfault)\n" "$t" "FAIL" "$RET"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_TESTS+=("$t (EXIT $RET)")
        continue
    fi

    if [[ $NORMALIZE -eq 1 ]]; then
        diff -u <(normalize_log "$EXPECTED") <(normalize_log "$ACTUAL") > "$DIFF_FILE"
        DIFF_RET=$?
    else
        diff -u "$EXPECTED" "$ACTUAL" > "$DIFF_FILE"
        DIFF_RET=$?
    fi

    if [[ $DIFF_RET -eq 0 ]]; then
        rm -f "$DIFF_FILE"
        printf "%-28s | %-10s | Khớp với %s\n" "$t" "PASS" "$EXPECTED"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        DIFF_LINES=$(wc -l < "$DIFF_FILE" | tr -d ' ')
        printf "%-28s | %-10s | Lệch (%s dòng diff -> %s)\n" "$t" "DIFF" "$DIFF_LINES" "$DIFF_FILE"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_TESTS+=("$t")
    fi
done

echo "================================================================"
echo " [3/3] Tổng kết: PASS = ${PASS_COUNT}/${#TESTS[@]} | LỆCH/LỖI = ${FAIL_COUNT}/${#TESTS[@]}"
if [[ $FAIL_COUNT -gt 0 ]]; then
    echo " Danh sách test lệch hoặc lỗi: ${FAILED_TESTS[*]}"
    echo " Gợi ý: Chạy './run_tests.sh --normalize' để bỏ qua khác biệt địa chỉ con trỏ ASLR."
    echo "================================================================"
    exit 1
fi
echo " Tất cả bài test đều khớp hoàn toàn!"
echo "================================================================"
exit 0
