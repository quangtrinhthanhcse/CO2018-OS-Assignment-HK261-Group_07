# BẢNG PHÂN CÔNG NHIỆM VỤ & HƯỚNG DẪN DỰ ÁN
## Bài tập lớn: Hệ Điều Hành (CO2018) - Học kỳ HK261
**Đề tài:** Simple Operating System Simulation with DMA (`demacOprx`)  
**Nhóm:** Group 07  
**Kho mã nguồn (GitHub):** [CO2018-OS-Assignment-HK261-Group_07](https://github.com/quangtrinhthanhcse/CO2018-OS-Assignment-HK261-Group_07)

---

## 1. Bảng Phân Công Nhiệm Vụ Thành Viên

> *Nguyên tắc làm việc:* 
> 1. Mỗi thành viên chọn 1 việc **Phần A** (code mã nguồn trên một nhánh `feat/*`) và đúng việc **Phần B** cùng số (viết báo cáo và chuẩn bị vấn đáp tương ứng).
> 2. Mỗi module đều có một **Buddy** (người hỗ trợ/dự phòng). Buddy phải đọc hiểu code của owner để đảm bảo không ai là điểm nghẽn duy nhất và cả hai đều tự tin trả lời vấn đáp.

| STT | Họ và Tên | MSSV | Email | Nhiệm vụ Code (Phần A) | Buddy (Dự phòng) | Hạn Merge vào `main` | Trách nhiệm Báo cáo (Phần B) & Tích hợp |
| :-: | :--- | :---: | :--- | :--- | :--- | :---: | :--- |
| **1** | **Nguyễn Khắc Triệu** | 2213619 | `trieu.nguyenkhac@hcmut.edu.vn` | **A1:** `feat/scheduler`<br>(MLQ Scheduler & Queue) | Trần Khan<br>(A6) | **15/11/2026** | **B1:** Vẽ Gantt, trả lời Q3; **Khởi tạo khung Báo cáo nhóm**; Điều phối tích hợp & script auto-test. |
| **2** | **Phạm Huỳnh Trọng Trí** | 2213657 | `tri.phamhuynhtrong@hcmut.edu.vn` | **A2:** `feat/syscall-mmvm`<br>(Syscall memmap & MM-VM) | Trịnh Thanh Quang<br>(A3) | **18/11/2026** | **B2:** Sơ đồ luồng syscall, cơ chế PID Passing, trả lời Q1 & Q2. |
| **3** | **Trịnh Thanh Quang** | 2412862 | `quang.trinhthanhcse@hcmut.edu.vn` | **A3:** `feat/mm64-memphy`<br>(RAM/SWAP & Phân trang 5 cấp) | Phạm Huỳnh Trọng Trí<br>(A2) | **17/11/2026** | **B3:** Sơ đồ phân trang 5 cấp, thống kê truy cập/dung lượng bảng trang, trả lời Q4, Q5 & Q6. |
| **4** | **Đặng Hồng Anh** | 2410060 | `anh.dang2502@hcmut.edu.vn` | **A4:** `feat/libmem`<br>(libmem user-space & FIFO swap) | Nguyễn Hoàng Quốc Mạnh<br>(A5) | **20/11/2026** | **B4:** Sơ đồ trạng thái Data Segment, ví dụ swap thực tế, trả lời Q7 (Đồng bộ hóa). |
| **5** | **Nguyễn Hoàng Quốc Mạnh** | 2111721 | `manh.nguyenrynaster01@hcmut.edu.vn` | **A5:** `feat/kmem`<br>(Kernel mem & copy user/kernel) | Đặng Hồng Anh<br>(A4) | **21/11/2026** | **B5:** Phân tách không gian user/kernel, cấp phát kmem, copy user/kernel. |
| **6** | **Trần Khan** | 2510102 | `khan.tran1507@hcmut.edu.vn` | **A6:** `feat/dma-coproc`<br>(DMA Verilator RTL & Bus) | Nguyễn Khắc Triệu<br>(A1) | **22/11/2026** | **B6:** Giải thích cơ chế bất đồng bộ DMA, định địa chỉ Bus; **Đóng gói file nộp bài**. |

---

## 2. Chi Tiết Nhiệm Vụ Từng Feature Branch

### 🌟 Feature 1: `feat/scheduler` — Bộ Lập Lịch Đa Hàng Đợi (MLQ Scheduler) & Queue
* **Người phụ trách (Owner):** Nguyễn Khắc Triệu · **Buddy:** Trần Khan
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/queue.c`
  * `hk261_ossim_demacOprx/src/sched.c`
  * `hk261_ossim_demacOprx/include/os-cfg.h`
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. `queue.c`:
     * `enqueue(struct queue_t *q, struct pcb_t *proc)`: Đưa con trỏ PCB vào cuối hàng đợi `q->proc[q->size]` và tăng `q->size`. Kiểm tra hàng đợi đầy hoặc con trỏ `NULL`.
     * `dequeue(struct queue_t *q)`: Lấy PCB ra khỏi đầu hàng đợi (FIFO), dồn các phần tử còn lại lên trước và giảm `q->size`. Trả về `NULL` khi hàng đợi rỗng.
     * `purgequeue(struct queue_t *q, struct pcb_t *proc)`: Tìm và xóa một `proc` cụ thể ra khỏi hàng đợi (dùng để gỡ tiến trình khỏi `running_list` khi đã kết thúc).
  2. `sched.c`:
     * `init_scheduler()`: Khởi tạo mảng định mức `slot[i] = MAX_PRIO - i` cho $i = 0 \dots 139$ (độ ưu tiên càng cao thì slot càng lớn) và các hàng đợi rỗng.
     * `get_mlq_proc()`: Duyệt qua các hàng đợi ưu tiên từ `0` đến `MAX_PRIO - 1`. Lấy tiến trình từ hàng đợi đầu tiên có `size > 0` và `slot > 0`, giảm `slot--`. Nếu duyệt hết vòng mà các hàng đợi còn tiến trình nhưng hết slot, tiến hành hồi phục lại toàn bộ `slot` để bắt đầu vòng mới.
     * `put_mlq_proc()` / `add_mlq_proc()`: Đưa tiến trình trở lại đúng hàng đợi `mlq_ready_queue[proc->prio]`.
     * Đồng bộ hóa: Dùng `pthread_mutex_lock(&queue_lock)` và `pthread_mutex_unlock(&queue_lock)` bao bọc mọi thao tác lấy/thêm tiến trình.
* **Vai trò tích hợp (Integration Coordinator):**
  * Sau khi hoàn thành phần của mình, viết script tự động chạy tất cả bài test và `diff` với thư mục `output/`.
  * Điều phối thứ tự merge nhánh vào `main`.
  * Rà soát các khóa đồng bộ hóa (`queue_lock`, `mmvm_lock`, `kmm_lock`) trên toàn dự án.
* **Các bài test kiểm tra:**
  * `sched`, `sched_0`, `sched_1` (So sánh trực tiếp với `output/sched*.output`).
* **Nhiệm vụ trong Báo cáo (Report - B1):**
  * Khởi tạo file tài liệu Báo cáo chung (Template) cho cả nhóm.
  * Vẽ **biểu đồ Gantt** thể hiện thứ tự các tiến trình được CPU thực thi theo từng time slot (khớp với log chạy thực tế).
  * Trả lời **Câu hỏi 3 (Trang 13 đề bài)**: Phân tích lợi ích của chính sách định mức slot trong hàng đợi MLQ đối với việc chống đói tài nguyên (anti-starvation).
* **Lưu ý:** Hàng đợi `run_queue` và `processed_queue` là mã nguồn cũ đã lỗi thời (obsolete), **không sử dụng** trong thuật toán MLQ.

---

### 🌟 Feature 2: `feat/syscall-mmvm` — Giao Diện Lời Gọi Hệ Thống & Tầng Vùng Nhớ Ảo (MM-VM)
* **Người phụ trách (Owner):** Phạm Huỳnh Trọng Trí · **Buddy:** Trịnh Thanh Quang
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/sys_mem.c`
  * `hk261_ossim_demacOprx/src/mm-vm.c`
  * `hk261_ossim_demacOprx/include/mm.h`
  * `hk261_ossim_demacOprx/src/syscall.tbl`, `Makefile` (khi kiểm thử thêm syscall mới)
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. `sys_mem.c` (`__sys_memmap`):
     * **Xóa bỏ đoạn code giả lập tạo `caller` bằng `malloc` (dòng 33--34).**
     * Hiện thực tìm kiếm con trỏ `pcb_t *caller` thật sự từ nhân bằng cách duyệt danh sách `krnl->running_list` (hoặc các hàng đợi sẵn sàng) sao cho `caller->pid == pid`. Đảm bảo chuẩn an toàn **PID Passing**.
     * Xử lý điều phối 5 thao tác bộ nhớ của syscall 17: `SYSMEM_MAP_OP`, `SYSMEM_INC_OP`, `SYSMEM_SWP_OP`, `SYSMEM_IO_READ`, `SYSMEM_IO_WRITE`.
  2. `include/mm.h`:
     * Hiện thực macro kiểm tra chồng lấn:
       ```c
       #define OVERLAP(x1, x2, y1, y2) (!((x2) <= (y1) || (x1) >= (y2)))
       ```
  3. `mm-vm.c`:
     * `get_vm_area_node_at_brk()`: Tạo cấu trúc `vm_rg_struct` mới xuất phát từ đỉnh `sbrk` hiện tại của VMA với kích thước căn chỉnh trang (`alignedsz`).
     * `validate_overlap_vm_area()`: Kiểm tra vùng nhớ mới dự kiến cấp phát có bị đè lên các VMA khác hay không.
     * `inc_vma_limit()`: Nâng giới hạn `sbrk` và `vm_end` khi vùng nhớ trống không đủ, gọi `vm_map_ram()` để cấp phát thêm frame vật lý.
* **Các bài test kiểm tra:**
  * `os_sc`, `os_syscall`, `os_syscall_list`.
* **Nhiệm vụ trong Báo cáo (Report - B2):**
  * Giải thích sơ đồ luồng một system call đi từ User space $\rightarrow$ `syscall_tbl` $\rightarrow$ `__sys_memmap` $\rightarrow$ Kernel.
  * Phân tích cơ chế bảo mật PID Passing (vì sao user chỉ truyền PID, kernel tự tra cứu PCB).
  * Trả lời **Câu hỏi 1 (Trang 9)** về ưu/nhược điểm của giao diện system call thống nhất và **Câu hỏi 2 (Trang 11)** về cách HĐH phát hiện/xử lý khi một syscall chạy quá lâu.

---

### 🌟 Feature 3: `feat/mm64-memphy` — Bộ Nhớ Vật Lý (RAM/SWAP) & Phân Trang 64-bit 5 Cấp
* **Người phụ trách (Owner):** Trịnh Thanh Quang · **Buddy:** Phạm Huỳnh Trọng Trí
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/mm-memphy.c`
  * `hk261_ossim_demacOprx/src/mm64.c`
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. `mm-memphy.c`:
     * `MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *retfpn)`: Lấy khung trang vật lý trống ở đầu `free_fp_list`, gán `*retfpn = fp->fpn`, cập nhật con trỏ danh sách và giải phóng node. Trả về `0` nếu thành công, `-1` nếu hết RAM.
     * `MEMPHY_dump(struct memphy_struct *mp)`: In nội dung bộ nhớ vật lý `mp->storage` phục vụ kiểm tra và gỡ lỗi.
  2. `mm64.c`:
     * `init_mm()`: Cấp phát động 5 mảng bảng trang đại diện cho 5 cấp (`pgd`, `p4d`, `pud`, `pmd`, `pt`), mỗi mảng gồm 512 phần tử ($512 \times 8 = 4096\text{ bytes} = 1\text{ trang}$). Khởi tạo VMA0 (`vm_id = 0`).
     * `pte_get_entry()`, `pte_set_entry()`: Đọc và ghi phần tử bảng trang (PTE).
     * `pte_set_fpn()`, `pte_set_swap()`: Thiết lập các cờ bit `Present`, `Swapped`, `FPN`, `SWPTYP`, `SWPOFF` vào PTE theo đúng định dạng 64-bit đề bài quy định.
     * `alloc_pages_range()`, `vmap_page_range()`, `vmap_pgd_memset()`: Xin các khung trang từ RAM và ghi liên kết vào bảng trang + danh sách FIFO `fifo_pgn`.
     * `print_pgtbl()`: In địa chỉ các cấp bảng trang theo đúng định dạng mẫu:  
       ` PDG=... P4g=... PUD=... PMD=...`.
* **Các bài test kiểm tra:**
  * `os_0_mlq_paging`, `os_1_singleCPU_mlq_paging`.
* **Nhiệm vụ trong Báo cáo (Report - B3):**
  * Vẽ và giải thích sơ đồ phân trang 5 cấp (chia bit: PGD 56..48, P4D 47..39, PUD 38..30, PMD 29..21, PT 20..12, OFFSET 11..0).
  * Tính toán số lần truy cập bộ nhớ khi tra cứu bảng trang và dung lượng bộ nhớ dành cho bảng trang.
  * Trả lời **Câu hỏi 4 (Trang 17)** về động lực kết hợp Segmentation với Paging, **Câu hỏi 5 (Trang 18)** về lợi ích của phân trang $N$ cấp, và **Câu hỏi 6 (Trang 19)** so sánh phân trang với cấp phát liên tục.

---

### 🌟 Feature 4: `feat/libmem` — Thư Viện Quản Lý Bộ Nhớ User-Space & Thay Trang FIFO
* **Người phụ trách (Owner):** Đặng Hồng Anh · **Buddy:** Nguyễn Hoàng Quốc Mạnh
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/libmem.c` (phần user-space)
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. Quản lý vùng nhớ ảo & Data Segment:
     * `get_free_vmrg_area()`: Tìm kiếm vùng trống trong `vm_freerg_list` vừa vặn với kích thước yêu cầu.
     * `__alloc()` / `liballoc()`: Cấp phát vùng nhớ biến từ `vm_freerg_list` hoặc yêu cầu mở rộng qua SYSCALL 17 (`SYSMEM_INC_OP`). Cập nhật bảng `symrgtbl[rgid]`.
     * `__free()` / `libfree()`: Thu hồi vùng nhớ biến và đưa vào `vm_freerg_list`.
  2. Quản lý trang ảo & Hoán đổi (Swapping):
     * `find_victim_page()`: Hoàn thiện thuật toán thay trang FIFO dựa trên danh sách `caller->mm->fifo_pgn` (sửa lỗi kiểm tra `prev == NULL` khi chỉ có 1 node).
     * `pg_getpage()`: Kiểm tra bit `PAGING_PAGE_PRESENT`. Nếu trang không online, kích hoạt quy trình swap: tìm trang nạn nhân, gọi `SYSCALL 17 (SYSMEM_SWP_OP)` để chuyển trang ra SWAP, nạp trang cần dùng vào RAM và cập nhật lại PTE.
     * `pg_getval()` / `pg_setval()`: Tính địa chỉ vật lý `phyaddr = fpn * PAGESZ + offset` và gọi `SYSCALL 17` (`SYSMEM_IO_READ` / `SYSMEM_IO_WRITE`) để đọc/ghi 1 byte.
     * `__read()` / `__write()` / `libread()` / `libwrite()`: Xác thực vùng nhớ dựa trên `symrgtbl[rgid]` và gọi đọc/ghi (sử dụng đúng `caller->mm`).
  3. Đồng bộ hóa:
     * Đảm bảo mọi thao tác trên cấu trúc nhớ ảo người dùng đều được bảo vệ bằng `mmvm_lock`.
* **Các bài test kiểm tra:**
  * `os_1_mlq_paging`, `os_1_mlq_paging_small_1K`, `os_1_mlq_paging_small_4K`.
* **Nhiệm vụ trong Báo cáo (Report - B4):**
  * Thống kê và vẽ sơ đồ trạng thái cấp phát bộ nhớ trong Data Segment qua các lệnh `alloc`, `free`, `read`, `write`. Đưa ra ví dụ hoán đổi trang (swap) thực tế từ log chạy.
  * Trả lời **Câu hỏi 7 (Trang 25)**: Phân tích các lỗi nghiêm trọng xảy ra nếu thiếu đồng bộ hóa (Race condition, Deadlock, trùng lặp khung trang vật lý giữa các tiến trình).

---

### 🌟 Feature 5: `feat/kmem` — Phân Tách Không Gian Nhớ User/Kernel & Quản Lý Bộ Nhớ Nhân
* **Người phụ trách (Owner):** Nguyễn Hoàng Quốc Mạnh · **Buddy:** Đặng Hồng Anh
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/libmem.c` (phần kernel memory)
  * `hk261_ossim_demacOprx/src/os.c` (hàm `ld_routine`)
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. Cấp phát bộ nhớ nhân liên tục:
     * `ld_routine()` trong `os.c`: Cấp phát bảng trang kernel `krnl_pgd` (và khởi tạo `os.mm`).
     * `libkmem_malloc()` & `__kmalloc()`: Cấp phát vùng nhớ trong không gian Kernel liên tục về mặt vật lý từ `krnl->mram` và cập nhật ánh xạ vào `krnl->krnl_pgd`.
  2. Phân tách và truyền dữ liệu an toàn giữa User và Kernel:
     * `libkmem_copy_from_user()`: Đọc an toàn từ User space và ghi sang Kernel space.
     * `libkmem_copy_to_user()`: Đọc từ Kernel space và ghi sang User space.
     * `__read_kernel_mem()`, `__write_kernel_mem()`, `__read_user_mem()`, `__write_user_mem()`.
  3. Đồng bộ hóa & Bảo vệ bộ nhớ:
     * Dùng khóa `kmm_lock` bao bọc mọi thao tác trên không gian nhớ nhân.
     * Kiểm tra ngăn chặn chương trình người dùng truy cập trực tiếp địa chỉ kernel space trái phép.
* **Các bài test kiểm tra:**
  * `os_2_singleCPU_mlq_paging`, `os_2_mlq_paging` (phần `kmalloc`, `copy_from_user`, `copy_to_user`).
* **Nhiệm vụ trong Báo cáo (Report - B5):**
  * Trình bày cơ chế phân tách địa chỉ chuẩn tắc (Canonical address) giữa User space và Kernel space.
  * Giải thích cơ chế cấp phát bộ nhớ liên tục trong nhân và cách sao chép an toàn qua `copy_from_user` / `copy_to_user`.

---

### 🌟 Feature 6: `feat/dma-coproc` — Bộ Đồng Xử Lý DMA (Verilator RTL) & Bus Hệ Thống
* **Người phụ trách (Owner):** Trần Khan · **Buddy:** Nguyễn Khắc Triệu
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/os.c` (hàm `co_processor`)
  * `hk261_ossim_demacOprx/src/dma_c_testbench.cpp`
  * `hk261_ossim_demacOprx/src/sysbus.c`
  * `hk261_ossim_demacOprx/src/libmem.c` (riêng hàm `libmem_dma_memcpy`)
* **Đặc điểm kỹ thuật (Không cần lo lắng về độ phức tạp):**
  * Bộ máy phần cứng Verilator (`Vdma_top*`) đã được giảng viên biên dịch sẵn 100%, sinh viên **hoàn toàn không cần sửa mã Verilog/RTL**.
  * Nhiệm vụ chỉ là gọi hàm khởi tạo và nối con trỏ RAM của HĐH vào bus hệ thống.
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. Kiểm thử phần cứng DMA độc lập:
     * Chạy `make dma` và kiểm tra `./dma_testbench` trên máy ảo Ubuntu để làm quen với luồng hoạt động của DMA.
  2. Khởi tạo luồng Co-processor:
     * `os.c`: Khởi tạo mô phỏng RTL bằng cách gọi `rtl_dma_init_simulation()` trong hàm luồng `co_processor()`.
  3. Giao tiếp Bus & Sao chép dữ liệu DMA:
     * `dma_c_testbench.cpp`: Trong `dpi_dma_copy()`, gán địa chỉ nguồn/đích trỏ về vùng nhớ `mram->storage` của OS khi hệ thống chạy thực tế.
     * `sysbus.c`: Đảm bảo hàm callback `dma_done_callback()` in ra thông báo hoàn tất truyền dữ liệu.
     * `libmem.c` (`libmem_dma_memcpy`): Gọi `do_dma()` thực hiện truyền dữ liệu DMA (tính toán địa chỉ vật lý từ vùng nhớ nguồn và đích).
* **Các bài test kiểm tra:**
  * Chạy độc lập: `./dma_testbench`
  * Chạy tích hợp toàn hệ thống: `os_2_singleCPU_mlq_paging`, `os_2_mlq_paging` (phần `dma_memcpy`).
* **Nhiệm vụ trong Báo cáo (Report - B6):**
  * Trình bày cơ chế hoạt động bất đồng bộ của DMA và cách định địa chỉ Bus (CPU truy cập qua MMU vs. DMA truy cập trực tiếp bộ nhớ).
  * Rà soát kiểm tra lần cuối toàn bộ mã nguồn theo chuẩn GNU C và nén file zip nộp bài `assignment_STUDENTID.zip`.
