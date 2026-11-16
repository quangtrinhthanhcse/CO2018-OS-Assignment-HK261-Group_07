# BÁO CÁO BÀI TẬP LỚN MÔN HỆ ĐIỀU HÀNH (CO2018)
## Đề tài: Mô phỏng Hệ điều hành Đơn giản với Bộ đồng xử lý DMA (`demacOprx`)

**Học kỳ:** HK261  
**Nhóm:** Group 07  
**Giảng viên hướng dẫn:** PGS.TS. Nguyễn Tiến Dũng (pdnguyen)

---

## Danh sách Thành viên & Phân công Nhiệm vụ

| STT | Họ và Tên | MSSV | Email | Mã việc | Nhiệm vụ chính (Code & Báo cáo) |
| :-: | :--- | :---: | :--- | :---: | :--- |
| 1 | **Nguyễn Khắc Triệu** | 2213619 | trieu.nguyenkhac@hcmut.edu.vn | **A1, B1** | Bộ lập lịch MLQ & Queue (`queue.c`, `sched.c`); Biểu đồ Gantt, Câu hỏi 3, Khung báo cáo chung |
| 2 | **Phạm Huỳnh Trọng Trí** | 2213657 | tri.phamhuynhtrong@hcmut.edu.vn | **A2, B2** | Syscall `memmap` & MM-VM (`sys_mem.c`, `mm-vm.c`, `mm.h`); Cơ chế PID Passing, Câu hỏi 1 & 2 |
| 3 | **Trịnh Thanh Quang** | 2412862 | quang.trinhthanhcse@hcmut.edu.vn | **A3, B3** | Bộ nhớ vật lý & Phân trang 64-bit 5 cấp (`mm-memphy.c`, `mm64.c`); Sơ đồ 5 cấp, Câu hỏi 4, 5 & 6 |
| 4 | **Đặng Hồng Anh** | 2410060 | anh.dang2502@hcmut.edu.vn | **A4, B4** | Thư viện `libmem` User-space & Thay trang FIFO (`libmem.c`); Sơ đồ Data Segment, Swap, Câu hỏi 7 |
| 5 | **Nguyễn Hoàng Quốc Mạnh** | 2111721 | manh.nguyenrynaster01@hcmut.edu.vn | **A5, B5** | Bộ nhớ nhân (`kmalloc`) & Copy User/Kernel (`libmem.c`, `os.c`); Tách không gian User/Kernel, Đồng bộ `kmem` |
| 6 | **Trần Khan** | 2510102 | khan.tran1507@hcmut.edu.vn | **A6, B6** | Bộ đồng xử lý DMA & Bus (`os.c`, `dma_c_testbench.cpp`, `sysbus.c`); Bất đồng bộ DMA, Bus Addressing |

---

## Mục lục
1. [Phần 1 (B1): Bộ lập lịch Đa hàng đợi (MLQ Scheduler)](#phần-1-b1-bộ-lập-lịch-đa-hàng-đợi-mlq-scheduler)
   - [1.1. Thiết kế và Hiện thực Hàng đợi (`queue.c`) và Bộ lập lịch (`sched.c`)](#11-thiết-kế-và-hiện-thực-hàng-đợi-queuec-và-bộ-lập-lịch-schedc)
   - [1.2. Biểu đồ Gantt và Phân tích Kết quả Kiểm thử (`sched`, `sched_0`, `sched_1`)](#12-biểu-đồ-gantt-và-phân-tích-kết-quả-kiểm-thử-sched-sched_0-sched_1)
   - [1.3. Trả lời Câu hỏi 3 (Mục 2.1): Lợi ích chính sách MLQ và Chống đói tài nguyên](#13-trả-lời-câu-hỏi-3-mục-21-lợi-ích-chính-sách-mlq-và-chống-đói-tài-nguyên)
2. [Phần 2 (B2): Giao diện Lời gọi Hệ thống (System Call) & Vùng nhớ ảo (MM-VM)](#phần-2-b2-giao-diện-lời-gọi-hệ-thống-system-call--vùng-nhớ-ảo-mm-vm)
   - [2.1. Sơ đồ Luồng System Call và Cơ chế Bảo mật PID Passing](#21-sơ-đồ-luồng-system-call-và-cơ-chế-bảo-mật-pid-passing)
   - [2.2. Trả lời Câu hỏi 1 (Trang 9): Giao diện System Call thống nhất](#22-trả-lời-câu-hỏi-1-trang-9-giao-diện-system-call-thống-nhất)
   - [2.3. Trả lời Câu hỏi 2 (Trang 11): Xử lý System Call thực thi quá lâu](#23-trả-lời-câu-hỏi-2-trang-11-xử-lý-system-call-thực-thi-quá-lâu)
3. [Phần 3 (B3): Bộ nhớ Vật lý & Phân trang Đa cấp 64-bit (5-level Paging)](#phần-3-b3-bộ-nhớ-vật-lý--phân-trang-đa-cấp-64-bit-5-level-paging)
   - [3.1. Sơ đồ Dịch địa chỉ Phân trang 5 cấp (57-bit Virtual Address)](#31-sơ-đồ-dịch-địa-chỉ-phân-trang-5-cấp-57-bit-virtual-address)
   - [3.2. Thống kê Truy cập Bộ nhớ và Dung lượng Bảng trang](#32-thống-kê-truy-cập-bộ-nhớ-và-dung-lượng-bảng-trang)
   - [3.3. Trả lời Câu hỏi 4, 5, 6](#33-trả-lời-câu-hỏi-4-5-6)
4. [Phần 4 (B4): Quản lý Vùng nhớ User-space, Hoán đổi trang (Swapping) & Đồng bộ hóa](#phần-4-b4-quản-lý-vùng-nhớ-user-space-hoán-đổi-trang-swapping--đồng-bộ-hóa)
   - [4.1. Sơ đồ Trạng thái Cấp phát Bộ nhớ trong Data Segment](#41-sơ-đồ-trạng-thái-cấp-phát-bộ-nhớ-trong-data-segment)
   - [4.2. Cơ chế Thay thế trang FIFO và Hoán đổi RAM/SWAP](#42-cơ-chế-thay-thế-trang-fifo-và-hoán-đổi-ramswap)
   - [4.3. Trả lời Câu hỏi 7 (Trang 25): Đồng bộ hóa và Điều kiện Tranh chấp (Race Condition)](#43-trả-lời-câu-hỏi-7-trang-25-đồng-bộ-hóa-và-điều-kiện-tranh-chấp-race-condition)
5. [Phần 5 (B5): Phân tách Không gian Nhớ User/Kernel & Quản lý Bộ nhớ Nhân (`kmem`)](#phần-5-b5-phân-tách-không-gian-nhớ-userkernel--quản-lý-bộ-nhớ-nhân-kmem)
   - [5.1. Cơ chế Phân tách Địa chỉ Chuẩn tắc (Canonical Address) và Bảo vệ Bộ nhớ Nhân](#51-cơ-chế-phân-tách-địa-chỉ-chuẩn-tắc-canonical-address-và-bảo-vệ-bộ-nhớ-nhân)
   - [5.2. Cấp phát Bộ nhớ Nhân liên tục (`__kmalloc`) và Sao chép `copy_from_user` / `copy_to_user`](#52-cấp-phát-bộ-nhớ-nhân-liên-tục-__kmalloc-và-sao-chép-copy_from_user--copy_to_user)
6. [Phần 6 (B6): Bộ đồng xử lý DMA (Verilator RTL) & Định địa chỉ Bus](#phần-6-b6-bộ-đồng-xử-lý-dma-verilator-rtl--định-địa-chỉ-bus)
   - [6.1. Cơ chế Hoạt động Bất đồng bộ (Asynchronous Operation) giữa CPU và DMA](#61-cơ-chế-hoạt-động-bất-đồng-bộ-asynchronous-operation-giữa-cpu-và-dma)
   - [6.2. Định địa chỉ Bus (Bus Addressing): CPU qua MMU vs. DMA truy cập trực tiếp RAM](#62-định-địa-chỉ-bus-bus-addressing-cpu-qua-mmu-vs-dma-truy-cập-trực-tiếp-ram)

---

## Phần 1 (B1): Bộ lập lịch Đa hàng đợi (MLQ Scheduler)
*(Phụ trách: Nguyễn Khắc Triệu - MSSV: 2213619)*

### 1.1. Thiết kế và Hiện thực Hàng đợi (`queue.c`) và Bộ lập lịch (`sched.c`)

#### a) Hiện thực cấu trúc hàng đợi FIFO (`src/queue.c`)
Mỗi hàng đợi `struct queue_t` quản lý một mảng tĩnh tối đa `MAX_QUEUE_SIZE = 50` con trỏ `struct pcb_t *` và biến `size` lưu số lượng tiến trình hiện tại. Vì trong kiến trúc MLQ, mỗi mức độ ưu tiên `prio` (từ `0` đến `MAX_PRIO - 1`) đã có một hàng đợi riêng biệt `mlq_ready_queue[prio]`, nên các tiến trình nằm trong cùng một hàng đợi có cùng độ ưu tiên và được phục vụ theo nguyên tắc **FIFO (First-In, First-Out)**:
- **`enqueue(struct queue_t *q, struct pcb_t *proc)`**: Kiểm tra tính hợp lệ (`q != NULL`, `proc != NULL`) và nếu `q->size < MAX_QUEUE_SIZE` thì chèn `proc` vào cuối mảng tại vị trí `q->proc[q->size]`, sau đó tăng `q->size++`.
- **`dequeue(struct queue_t *q)`**: Kiểm tra hàng đợi khác rỗng (`q->size > 0`), lấy phần tử đầu tiên `q->proc[0]` ra để trả về, sau đó dùng vòng lặp `for` dịch chuyển toàn bộ các phần tử từ vị trí `1 .. q->size - 1` sang trái 1 ô và giảm `q->size--`.
- **`purgequeue(struct queue_t *q, struct pcb_t *proc)`**: Duyệt mảng tìm phần tử khớp với con trỏ `proc`. Khi tìm thấy tại chỉ số `i`, lưu lại con trỏ, dịch các phần tử phía sau lên 1 vị trí để lấp chỗ trống, giảm `q->size--` và trả về `proc` vừa xóa. Hàm này được dùng để rút tiến trình ra khỏi `running_list` khi tiến trình hết lượt chạy (time slice) hoặc kết thúc.

#### b) Hiện thực Bộ lập lịch Đa hàng đợi MLQ (`src/sched.c`)
Hệ thống hỗ trợ `MAX_PRIO = 140` mức ưu tiên (từ `0` đến `139`, giá trị số càng nhỏ độ ưu tiên càng cao):
- **`init_scheduler()`**: Khởi tạo toàn bộ 140 hàng đợi `mlq_ready_queue[i].size = 0`, khởi tạo định mức khe thời gian (slot) cho từng mức ưu tiên theo công thức `slot[i] = MAX_PRIO - i`, làm rỗng `running_list` và khởi tạo khóa `pthread_mutex_init(&queue_lock, NULL)`.
- **`get_mlq_proc()`**:
  1. Khóa `queue_lock` để đảm bảo an toàn đa luồng khi nhiều CPU cùng lấy tiến trình.
  2. Quét từ mức ưu tiên cao nhất `i = 0` đến `MAX_PRIO - 1`. Nếu hàng đợi `mlq_ready_queue[i]` không rỗng (`!empty(...)`) thì ghi nhận `has_proc = 1`. Nếu đồng thời `slot[i] > 0`, gọi `dequeue(&mlq_ready_queue[i])`, giảm `slot[i]--` và thoát vòng lặp.
  3. Nếu duyệt hết cả 140 mức ưu tiên mà chưa lấy được tiến trình (`proc == NULL`) nhưng vẫn còn tiến trình đang chờ (`has_proc == 1`, tức các hàng đợi có tiến trình đều đã cạn `slot[i] == 0`), bộ lập lịch thực hiện **cấp phát lại (reset) toàn bộ mảng `slot[i] = MAX_PRIO - i`** rồi quét lại từ đầu để lấy tiến trình.
  4. Nếu lấy được `proc != NULL`, đưa tiến trình vào `running_list` bằng `enqueue(&running_list, proc)`, mở khóa `queue_lock` và trả về `proc`.
- **`put_mlq_proc()` và `add_mlq_proc()`**: Bảo vệ bằng `queue_lock`, gỡ tiến trình khỏi `running_list` (khi `put_mlq_proc`) và đưa tiến trình vào cuối hàng đợi `mlq_ready_queue[proc->prio]`.

---

### 1.2. Biểu đồ Gantt và Phân tích Kết quả Kiểm thử (`sched`, `sched_0`, `sched_1`)

#### 1.2.1. Kịch bản kiểm thử 1: `input/sched` (Đa CPU: 2 CPU, Time slice = 4)

**Cấu hình đầu vào (`input/sched`):**
- `time_slice = 4`, `num_cpus = 2`, `num_processes = 4`
- Danh sách tiến trình nạp vào:

| PID | Chương trình | Thời điểm nạp (`start_time`) | Số lệnh (`size` - toàn bộ `calc`) | Độ ưu tiên (`prio`) | Định mức `slot` ban đầu |
| :-: | :---: | :-: | :-: | :-: | :-: |
| **P1** | `p2s` | Slot 0 | 12 | 0 | 140 |
| **P2** | `p1s` | Slot 1 | 10 | 1 | 139 |
| **P3** | `p2s` | Slot 3 | 12 | 1 | 139 |
| **P4** | `p3s` | Slot 4 | 11 | 0 | 140 |

**Bảng trung gian theo dõi trạng thái lập lịch (`output/sched.output`):**

| Time Slot | Sự kiện Loader (`ld_routine`) | Hoạt động trên **CPU 1** | Hoạt động trên **CPU 0** | Hàng đợi `prio = 0` | Hàng đợi `prio = 1` |
| :-: | :--- | :--- | :--- | :---: | :---: |
| **0** | Load **P1** (`prio=0`) | *(Chưa lấy được proc)* | *(Chưa lấy được proc)* | `[P1]` | `[]` |
| **1** | Load **P2** (`prio=1`) | **Dispatch P1** (chạy 1/12) | *(Chờ tới slot sau)* | `[]` | `[P2]` |
| **2** | - | Chạy P1 (2/12) | **Dispatch P2** (chạy 1/10) | `[]` | `[]` |
| **3** | Load **P3** (`prio=1`) | Chạy P1 (3/12) | Chạy P2 (2/10) | `[]` | `[P3]` |
| **4** | Load **P4** (`prio=0`) | Chạy P1 (4/12) | Chạy P2 (3/10) | `[P4]` | `[P3]` |
| **5** | *(Done)* | Put P1 $\rightarrow$ **Dispatch P4** (1/11) | Chạy P2 (4/10) | `[P1]` | `[P3]` |
| **6..8** | - | Chạy P4 (2..4/11) | Slot 6: Put P2 $\rightarrow$ **Dispatch P1** (5..7/12) | `[]` | `[P3, P2]` |
| **9** | - | Put P4 $\rightarrow$ **Dispatch P4** (5/11) | Chạy P1 (8/12) | `[]` | `[P3, P2]` |
| **10..12** | - | Chạy P4 (6..8/11) | Slot 10: Put P1 $\rightarrow$ **Dispatch P1** (9..11/12) | `[]` | `[P3, P2]` |
| **13** | - | Put P4 $\rightarrow$ **Dispatch P4** (9/11) | Chạy P1 (12/12) | `[]` | `[P3, P2]` |
| **14..16** | - | Chạy P4 (10..11/11) | Slot 14: **P1 Finished** $\rightarrow$ **Dispatch P3** (1..3/12) | `[]` | `[P2]` |
| **17** | - | Put P4 $\rightarrow$ **Dispatch P4** *(pc==size)* | Chạy P3 (4/12) | `[]` | `[P2]` |
| **18..20** | - | Chờ hết time_slice của P4 | Slot 18: Put P3 $\rightarrow$ **Dispatch P2** (5..7/10) | `[]` | `[P3]` |
| **21** | - | Put P4 $\rightarrow$ **Dispatch P4** | Chạy P2 (8/10) | `[]` | `[P3]` |
| **22..25** | - | Slot 22: **P4 Finished** $\rightarrow$ **Dispatch P3** (5..8/12) | Slot 22: Put P2 $\rightarrow$ **Dispatch P2** (9..10/10) | `[]` | `[]` |
| **26..29** | - | Slot 26: Put P3 $\rightarrow$ **Dispatch P3** (9..12/12) | Slot 26: Put P2 $\rightarrow$ **Dispatch P2** | `[]` | `[]` |
| **30..32** | - | Slot 30: **P3 Finished**, **CPU 1 Stopped** | Slot 30: Put P2 $\rightarrow$ **Dispatch P2** | `[]` | `[]` |
| **33** | - | *(Stopped)* | **P2 Finished**, **CPU 0 Stopped** | `[]` | `[]` |

**Biểu đồ Gantt (Kịch bản `sched` - 2 CPU):**

```text
Time Slot:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33
           +--+-----------+-----------+-----------+-----------+-----------+--+-----------+-----------+
CPU 1:     |  |    P1     |    P4     |    P4     |    P4     |    P4     |P4|    P3     |    P3     | (Stop at 30)
           +--+-----------+-----------+-----------+-----------+-----------+--+-----------+-----------+
           +-----+-----------+-----------+-----------+-----------+-----------+-----------+-----------+--------+
CPU 0:     |     |    P2     |    P1     |    P1     |    P3     |    P2     |    P2     |    P2     |   P2   | (Stop at 33)
           +-----+-----------+-----------+-----------+-----------+-----------+-----------+-----------+--------+
```

**Giải thích thứ tự chạy:**
- Tại Slot 1 và 2: P1 (`prio=0`) được cấp cho CPU 1, P2 (`prio=1`) được cấp cho CPU 0 vì lúc đó hàng đợi `prio=0` đã trống.
- Tại Slot 5: P1 hết `time_slice = 4`, trả về hàng đợi `prio=0` sau P4 (vừa vào lúc Slot 4). Do đó CPU 1 lấy P4 ra chạy.
- Tại Slot 6: P2 hết `time_slice = 4`, trả về hàng đợi `prio=1`. Khi CPU 0 gọi `get_proc()`, vì hàng đợi `prio=0` đang có P1 (ưu tiên cao hơn `prio=1`), CPU 0 lập tức lấy P1 ra chạy thay vì chạy tiếp P3 hay P2.
- Chỉ khi cả P1 và P4 (thuộc `prio=0`) đã được lấy hết hoặc hoàn thành (từ Slot 14 trở đi), CPU 0 và CPU 1 mới quay xuống phục vụ P3 và P2 ở hàng đợi `prio=1`.

---

#### 1.2.2. Kịch bản kiểm thử 2: `input/sched_0` (1 CPU, Time slice = 2)

**Cấu hình đầu vào (`input/sched_0`):**
- `time_slice = 2`, `num_cpus = 1`, `num_processes = 4`
- P1 (`s0`, 15 lệnh, nạp Slot 0, `prio=4`), P2 (`s0`, 15 lệnh, nạp Slot 1, `prio=4`), P3 (`s1`, 7 lệnh, nạp Slot 4, `prio=0`), P4 (`s0`, 15 lệnh, nạp Slot 5, `prio=0`).

**Biểu đồ Gantt (Kịch bản `sched_0` - 1 CPU):**

```text
Slot:   0  1  3  5  7  9 11 13 15 17 19 21 22 24 26 28 29 31 33 35 37 39 41 43 45 47 49 51 53 54 55
       +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
CPU 0: |  |P1|P2|P3|P4|P3|P4|P3|P4|P3|P4|P3|P4|P4|P4|P4|P1|P2|P1|P2|P1|P2|P1|P2|P1|P2|P1|P2|P1|P2|Stop
       +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

**Giải thích:**
- Từ Slot 1 đến Slot 5: Chỉ có P1 và P2 (`prio=4`), CPU 0 chạy lần lượt P1 (Slot 1-2) và P2 (Slot 3-4).
- Tại Slot 4 và 5: P3 và P4 xuất hiện với độ ưu tiên cao nhất (`prio=0`). Do đó từ Slot 5 đến Slot 29, P3 và P4 chiếm quyền ưu tiên trên CPU 0, luân phiên nhau theo Round-Robin (`time_slice = 2`) cho đến khi P3 kết thúc ở Slot 22 và P4 kết thúc ở Slot 29.
- Từ Slot 29 đến Slot 55: Hàng đợi `prio=0` đã trống hoàn toàn, P1 và P2 ở `prio=4` mới tiếp tục luân phiên nhau chạy cho đến khi kết thúc ở Slot 54 (P1) và Slot 55 (P2).

---

#### 1.2.3. Kịch bản kiểm thử 3: `input/sched_1` (1 CPU, Time slice = 2)

**Cấu hình đầu vào (`input/sched_1`):**
- `time_slice = 2`, `num_cpus = 1`, `num_processes = 5`
- P1 (`s0`, 15 lệnh, Slot 0, `prio=4`), P2 (`s3`, 11 lệnh, Slot 2, `prio=0`), P3 (`s2`, 12 lệnh, Slot 3, `prio=0`), P4 (`s1`, 7 lệnh, Slot 5, `prio=0`), P5 (`s3`, 11 lệnh, Slot 6, `prio=0`).

**Biểu đồ Gantt (Kịch bản `sched_1` - 1 CPU):**

```text
Slot:   0  1  3  5  7  9 11 13 15 17 19 21 23 25 27 29 31 33 35 37 39 41 42 44 46 48 50 52 54 56 .. 69
       +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+-----+--+
CPU 0: |  |P1|P2|P3|P2|P4|P5|P3|P2|P4|P5|P3|P2|P4|P5|P3|P2|P4|P5|P3|P2|P4|P5|P3|P2|P5|P2|P5|P5| P1  |Stop
       +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+-----+--+
```

**Giải thích:**
- P1 (`prio=4`) chạy được 1 lượt (Slot 1-2). Ngay sau đó P2, P3, P4, P5 (`prio=0`) lần lượt vào hàng đợi ưu tiên 0 và xoay vòng Round-Robin `(P2 -> P4 -> P5 -> P3)` cho tới khi hoàn tất hết ở Slot 56 (P4 xong ở 42, P3 xong ở 46, P2 xong ở 52, P5 xong ở 56).
- Từ Slot 56 đến 69: P1 (`prio=4`) được lấy ra chạy nốt các lệnh còn lại cho đến khi dừng ở Slot 69.

---

### 1.3. Trả lời Câu hỏi 3 (Mục 2.1): Lợi ích chính sách MLQ và Chống đói tài nguyên

> **Câu hỏi 3 (Trang 13 đề gốc):** *Xét tác động của các chính sách chi tiết trong MLQ, lợi ích của từng chính sách là gì?*

**Trả lời:**

Trong bộ lập lịch Hàng đợi Đa mức (Multi-Level Queue - MLQ) của hệ thống Simple OS, có **3 chính sách cốt lõi** phối hợp với nhau, mỗi chính sách mang lại lợi ích thiết thực sau:

1. **Chính sách phân tầng ưu tiên cố định (`MAX_PRIO = 140` mức hàng đợi riêng biệt):**
   - **Cơ chế:** Mỗi tiến trình khi nạp vào hệ thống được gán vào hàng đợi `mlq_ready_queue[prio]` tương ứng với độ ưu tiên `prio` của nó (`0` là cao nhất, `139` là thấp nhất). Bộ lập lịch luôn ưu tiên kiểm tra các hàng đợi có chỉ số `prio` nhỏ trước.
   - **Lợi ích:**
     - **Đảm bảo tính phản hồi (Responsiveness) cho tác vụ quan trọng:** Các tiến trình hệ thống hoặc tiến trình thời gian thực (có `prio` nhỏ) luôn được phục vụ ngay lập tức khi có CPU rảnh mà không phải xếp hàng chờ sau các tiến trình nền kém quan trọng.
     - **Độ phức tạp tìm kiếm thấp và ổn định $O(\text{MAX\_PRIO})$:** Thay vì phải sắp xếp (sort) toàn bộ tiến trình mỗi khi thêm/lấy hoặc tìm kiếm tuyến tính trên một danh sách khổng lồ, việc tách thành mảng các hàng đợi FIFO giúp thao tác `enqueue` và `dequeue` tại mỗi mức đạt $O(1)$.

2. **Chính sách lập lịch Xoay vòng (Round-Robin với định mức `time_slice`) trong từng hàng đợi:**
   - **Cơ chế:** Mỗi tiến trình khi được cấp CPU chỉ được phép chạy tối đa `time_slice` đơn vị thời gian. Nếu hết `time_slice` mà chưa hoàn thành (`pc < code->size`), tiến trình bị thu hồi CPU (`put_proc`) và đưa về cuối hàng đợi cùng mức `prio`.
   - **Lợi ích:**
     - **Công bằng giữa các tiến trình cùng độ ưu tiên (Fairness within same priority):** Ngăn chặn tình trạng một tiến trình có thời gian chạy dài (CPU-bound) độc chiếm CPU, làm các tiến trình ngắn hơn có cùng độ ưu tiên phải chờ đợi lâu (hiệu ứng đoàn tàu - *convoy effect*).
     - **Tăng tính tương tác đa nhiệm (Multitasking):** Mọi tiến trình cùng mức ưu tiên đều được chia sẻ CPU luân phiên đều đặn.

3. **Chính sách định mức khe thời gian (`slot[prio] = MAX_PRIO - prio`) cho mỗi vòng duyệt:**
   - **Cơ chế:** Mỗi hàng đợi ưu tiên `prio` chỉ được cấp tối đa `MAX_PRIO - prio` lần lấy tiến trình (`get_mlq_proc`) trong một chu kỳ lập lịch. Mỗi lần một tiến trình ở hàng đợi `prio` được xuất ra cho CPU, biến `slot[prio]` giảm đi 1. Khi `slot[prio] == 0`, ngay cả khi hàng đợi `mlq_ready_queue[prio]` vẫn còn tiến trình đang chờ, bộ lập lịch **bắt buộc phải bỏ qua** hàng đợi này để chuyển xuống phục vụ hàng đợi `prio + 1` kế tiếp. Khi tất cả các hàng đợi đang có tiến trình đều dùng hết `slot`, hệ thống mới khôi phục (reset) lại mảng `slot` cho vòng mới.
   - **Lợi ích quan trọng nhất — Chống đói tài nguyên (Anti-Starvation):**
     - Trong thuật toán MLQ cổ điển (Strict Priority Scheduling), nếu các tiến trình ở hàng đợi ưu tiên cao (`prio = 0`) liên tục được nạp vào hoặc có thời gian chạy rất dài, các tiến trình ở hàng đợi ưu tiên thấp (`prio > 0`) sẽ không bao giờ nhận được CPU (hiện tượng **đói CPU - Starvation**).
     - Với cơ chế định mức `slot = MAX_PRIO - prio`, hàng đợi ưu tiên cao vẫn nhận được **tỷ trọng CPU lớn hơn** (ví dụ `prio = 0` được 140 lượt, `prio = 1` được 139 lượt, ..., `prio = 139` được 1 lượt), nhưng **hữu hạn**. Sau tối đa 140 lần cấp phát cho `prio = 0`, CPU chắc chắn được nhường xuống cho các mức ưu tiên thấp hơn. Nhờ đó, hệ thống vừa duy trì được sự phân hóa mức độ ưu tiên (Proportional Share), vừa triệt tiêu hoàn toàn nguy cơ đói tài nguyên mà không cần tốn chi phí tính toán lại độ ưu tiên động (Aging/Feedback) phức tạp.

---

## Phần 2 (B2): Giao diện Lời gọi Hệ thống (System Call) & Vùng nhớ ảo (MM-VM)
*(Phụ trách: Phạm Huỳnh Trọng Trí - MSSV: 2213657)*

### 2.1. Sơ đồ Luồng System Call và Cơ chế Bảo mật PID Passing
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B2 cập nhật)*

### 2.2. Trả lời Câu hỏi 1 (Trang 9): Giao diện System Call thống nhất
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B2 cập nhật)*

### 2.3. Trả lời Câu hỏi 2 (Trang 11): Xử lý System Call thực thi quá lâu
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B2 cập nhật)*

---

## Phần 3 (B3): Bộ nhớ Vật lý & Phân trang Đa cấp 64-bit (5-level Paging)
*(Phụ trách: Trịnh Thanh Quang - MSSV: 2412862)*

### 3.1. Sơ đồ Dịch địa chỉ Phân trang 5 cấp (57-bit Virtual Address)
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B3 cập nhật)*

### 3.2. Thống kê Truy cập Bộ nhớ và Dung lượng Bảng trang
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B3 cập nhật)*

### 3.3. Trả lời Câu hỏi 4, 5, 6
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B3 cập nhật)*

---

## Phần 4 (B4): Quản lý Vùng nhớ User-space, Hoán đổi trang (Swapping) & Đồng bộ hóa
*(Phụ trách: Đặng Hồng Anh - MSSV: 2410060)*

### 4.1. Sơ đồ Trạng thái Cấp phát Bộ nhớ trong Data Segment
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B4 cập nhật)*

### 4.2. Cơ chế Thay thế trang FIFO và Hoán đổi RAM/SWAP
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B4 cập nhật)*

### 4.3. Trả lời Câu hỏi 7 (Trang 25): Đồng bộ hóa và Điều kiện Tranh chấp (Race Condition)
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B4 cập nhật)*

---

## Phần 5 (B5): Phân tách Không gian Nhớ User/Kernel & Quản lý Bộ nhớ Nhân (`kmem`)
*(Phụ trách: Nguyễn Hoàng Quốc Mạnh - MSSV: 2111721)*

### 5.1. Cơ chế Phân tách Địa chỉ Chuẩn tắc (Canonical Address) và Bảo vệ Bộ nhớ Nhân
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B5 cập nhật)*

### 5.2. Cấp phát Bộ nhớ Nhân liên tục (`__kmalloc`) và Sao chép `copy_from_user` / `copy_to_user`
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B5 cập nhật)*

---

## Phần 6 (B6): Bộ đồng xử lý DMA (Verilator RTL) & Định địa chỉ Bus
*(Phụ trách: Trần Khan - MSSV: 2510102)*

### 6.1. Cơ chế Hoạt động Bất đồng bộ (Asynchronous Operation) giữa CPU và DMA
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B6 cập nhật)*

### 6.2. Định địa chỉ Bus (Bus Addressing): CPU qua MMU vs. DMA truy cập trực tiếp RAM
*(Phần nội dung chi tiết sẽ do thành viên phụ trách B6 cập nhật)*
