# BẢNG PHÂN CÔNG NHIỆM VỤ & HƯỚNG DẪN DỰ ÁN
## Bài tập lớn: Hệ Điều Hành (CO2018) - Học kỳ HK261
**Đề tài:** Simple Operating System Simulation with DMA (`demacOprx`)  
**Nhóm:** Group 07  
**Kho mã nguồn (GitHub):** [CO2018-OS-Assignment-HK261-Group_07](https://github.com/quangtrinhthanhcse/CO2018-OS-Assignment-HK261-Group_07)

---

## 1. Bảng Phân Công Nhiệm Vụ & Đóng Góp Thành Viên

> *Nguyên tắc phân công:* Mỗi thành viên chịu trách nhiệm độc lập một module mã nguồn (tương ứng với một nhánh `feat/*` trên Git) và tự biên soạn phần Báo cáo (Report) tương ứng với module đó. Tỷ lệ đóng góp thực tế sẽ được nhóm đánh giá lại sau khi hoàn thành dự án.

| STT | Họ và Tên | MSSV | Email | Nhiệm vụ chính (Feature Branch) | Trách nhiệm Báo cáo & Phối hợp | Đóng góp (%) |
| :-: | :--- | :---: | :--- | :--- | :--- | :-: |
| 1 | [Điền Tên TV1] | [MSSV 1] | [Email 1] | **Feat 1:** `feat/scheduler` (Lập lịch MLQ & Queue) | Vẽ biểu đồ Gantt, trả lời Câu hỏi 3; **Khởi tạo khung Báo cáo nhóm** | |
| 2 | [Điền Tên TV2] | [MSSV 2] | [Email 2] | **Feat 2:** `feat/syscall-mmvm` (System Call & Vùng nhớ ảo) | Báo cáo cơ chế PID Passing, trả lời Câu hỏi 1 & 2 | |
| 3 | [Điền Tên TV3] | [MSSV 3] | [Email 3] | **Feat 3:** `feat/mm64-memphy` (Phân trang 64-bit 5 cấp & RAM/SWAP) | Sơ đồ phân trang 5 cấp, trả lời Câu hỏi 4, 5 & 6 | |
| 4 | [Điền Tên TV4] | [MSSV 4] | [Email 4] | **Feat 4:** `feat/libmem` (Thư viện bộ nhớ User/Kernel & Đồng bộ hóa) | Trạng thái vùng nhớ Data Segment, trả lời Câu hỏi 7 | |
| 5 | [Điền Tên TV5] | [MSSV 5] | [Email 5] | **Feat 5:** `feat/dma-coproc` (Đồng xử lý DMA RTL Verilator & Bus) | Giải thích cơ chế bất đồng bộ DMA, định địa chỉ Bus; **Đóng gói file nộp bài** | |

---

## 2. Chi Tiết Nhiệm Vụ Từng Feature Branch

### 🌟 Feature 1: `feat/scheduler` — Bộ Lập Lịch Đa Hàng Đợi (MLQ Scheduler) & Queue
* **Người phụ trách:** Thành viên 1
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/queue.c`
  * `hk261_ossim_demacOprx/src/sched.c`
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. `queue.c`:
     * `enqueue(struct queue_t *q, struct pcb_t *proc)`: Đưa con trỏ PCB vào cuối hàng đợi `q->proc[q->size]` và tăng `q->size`.
     * `dequeue(struct queue_t *q)`: Lấy PCB ra khỏi đầu hàng đợi (FIFO), dồn các phần tử còn lại lên trước và giảm `q->size`.
     * `purgequeue(struct queue_t *q, struct pcb_t *proc)`: Tìm và xóa một `proc` cụ thể ra khỏi hàng đợi (dùng để gỡ tiến trình khỏi `running_list` khi đã kết thúc).
  2. `sched.c`:
     * `init_scheduler()`: Khởi tạo mảng định mức `slot[i] = MAX_PRIO - i` cho $i = 0 \dots 139$ và các hàng đợi rỗng.
     * `get_mlq_proc()`: Duyệt qua các hàng đợi ưu tiên từ `0` đến `MAX_PRIO - 1`. Lấy tiến trình từ hàng đợi có `size > 0` và `slot > 0`, giảm `slot--`. Nếu duyệt hết vòng mà các hàng đợi còn tiến trình nhưng hết slot, tiến hành hồi phục lại toàn bộ `slot` để bắt đầu vòng mới.
     * `put_mlq_proc()` / `add_mlq_proc()`: Đưa tiến trình trở lại đúng hàng đợi `mlq_ready_queue[proc->prio]`.
     * Đồng bộ hóa: Dùng `pthread_mutex_lock(&queue_lock)` và `pthread_mutex_unlock(&queue_lock)` bao bọc mọi thao tác lấy/thêm tiến trình.
* **Các bài test kiểm tra:**
  * `sched`, `sched_0`, `sched_1` (So sánh trực tiếp với `output/sched*.output`).
* **Nhiệm vụ trong Báo cáo (Report):**
  * Khởi tạo file tài liệu Báo cáo chung (Template) cho cả nhóm.
  * Vẽ **biểu đồ Gantt** thể hiện thứ tự các tiến trình được CPU thực thi theo từng time slot.
  * Trả lời **Câu hỏi 3 (Trang 13 đề bài)**: Phân tích lợi ích của chính sách định mức slot trong hàng đợi MLQ đối với việc chống đói tài nguyên (anti-starvation).
* **Lưu ý:** Hàng đợi `run_queue` và `processed_queue` là mã nguồn cũ đã lỗi thời (obsolete), **không sử dụng** trong thuật toán MLQ.

---

### 🌟 Feature 2: `feat/syscall-mmvm` — Giao Diện Lời Gọi Hệ Thống & Tầng Vùng Nhớ Ảo (MM-VM)
* **Người phụ trách:** Thành viên 2
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/sys_mem.c`
  * `hk261_ossim_demacOprx/src/mm-vm.c`
  * `hk261_ossim_demacOprx/include/mm.h` (macro `OVERLAP`, `INCLUDE`)
  * `hk261_ossim_demacOprx/src/syscall.tbl`, `Makefile` (khi test thêm syscall mới)
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. `sys_mem.c` (`__sys_memmap`):
     * **Xóa bỏ đoạn code giả lập tạo `caller` bằng `malloc` (dòng 33--34).**
     * Hiện thực tìm kiếm con trỏ `pcb_t *caller` thật sự từ nhân bằng cách duyệt danh sách `krnl->running_list` (hoặc các hàng đợi sẵn sàng) sao cho `caller->pid == pid`. Đảm bảo chuẩn bảo mật **PID Passing**.
     * Xử lý điều phối 5 thao tác bộ nhớ: `SYSMEM_MAP_OP`, `SYSMEM_INC_OP`, `SYSMEM_SWP_OP`, `SYSMEM_IO_READ`, `SYSMEM_IO_WRITE`.
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
* **Nhiệm vụ trong Báo cáo (Report):**
  * Giải thích cơ chế giao tiếp an toàn qua System Call (chỉ truyền `PID`, không truyền trực tiếp `PCB` từ user space).
  * Trả lời **Câu hỏi 1 (Trang 9)** về ưu/nhược điểm của giao diện system call thống nhất và **Câu hỏi 2 (Trang 11)** về cách HĐH phát hiện/xử lý khi một syscall chạy quá lâu.

---

### 🌟 Feature 3: `feat/mm64-memphy` — Bộ Nhớ Vật Lý (RAM/SWAP) & Phân Trang 64-bit 5 Cấp
* **Người phụ trách:** Thành viên 3
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/mm-memphy.c`
  * `hk261_ossim_demacOprx/src/mm64.c`
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. `mm-memphy.c`:
     * `MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *retfpn)`: Lấy khung trang vật lý trống ở đầu `free_fp_list`, gán `*retfpn = fp->fpn`, cập nhật con trỏ danh sách và giải phóng node.
     * `MEMPHY_dump(struct memphy_struct *mp)`: In nội dung bộ nhớ vật lý `mp->storage` phục vụ kiểm tra và gỡ lỗi.
  2. `mm64.c`:
     * `init_mm()`: Cấp phát động 5 mảng bảng trang đại diện cho 5 cấp (`pgd`, `p4d`, `pud`, `pmd`, `pt`), mỗi mảng gồm 512 phần tử ($512 \times 8 = 4096\text{ bytes} = 1\text{ trang}$). Khởi tạo VMA0 (`vm_id = 0`).
     * `pte_get_entry()`, `pte_set_entry()`: Đọc và ghi phần tử bảng trang (PTE).
     * `pte_set_fpn()`, `pte_set_swap()`: Thiết lập các cờ bit `Present`, `Swapped`, `FPN`, `SWPTYP`, `SWPOFF` vào PTE theo chuẩn Hình 6 của đề bài.
     * `alloc_pages_range()`, `vmap_page_range()`: Xin các khung trang từ RAM và ghi liên kết vào bảng trang + danh sách FIFO `fifo_pgn`.
     * `print_pgtbl()`: In địa chỉ các cấp bảng trang theo đúng định dạng mẫu:  
       `PDG=... P4D=... PUD=... PMD=... PT=...`.
* **Các bài test kiểm tra:**
  * `os_0_mlq_paging`, `os_1_singleCPU_mlq_paging`.
* **Nhiệm vụ trong Báo cáo (Report):**
  * Vẽ và giải thích sơ đồ phân trang đa cấp 5 cấp (bit 56..48, 47..39, 38..30, 29..21, 20..12, 11..0).
  * Trả lời **Câu hỏi 4 (Trang 17)** về động lực kết hợp Segmentation với Paging, **Câu hỏi 5 (Trang 18)** về lợi ích của phân trang $N$ cấp, và **Câu hỏi 6 (Trang 19)** so sánh phân trang với cấp phát liên tục.

---

### 🌟 Feature 4: `feat/libmem` — Thư Viện Quản Lý Bộ Nhớ User/Kernel & Đồng Bộ Hóa
* **Người phụ trách:** Thành viên 4
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/libmem.c` (ngoại trừ hàm `libmem_dma_memcpy`)
* **Nhiệm vụ cụ thể cần hiện thực:**
  1. Quản lý trang ảo & Hoán đổi (Swapping):
     * `find_victim_page()`: Hoàn thiện thuật toán thay trang FIFO dựa trên danh sách `caller->mm->fifo_pgn` (sửa lỗi kiểm tra `prev == NULL` khi chỉ có 1 node).
     * `pg_getpage()`: Kiểm tra bit `PAGING_PAGE_PRESENT`. Nếu trang không online, kích hoạt quy trình swap: tìm trang nạn nhân, gọi `SYSCALL 17 (SYSMEM_SWP_OP)` để chuyển trang ra SWAP, nạp trang cần dùng vào RAM và cập nhật lại PTE.
     * `pg_getval()` / `pg_setval()`: Tính địa chỉ vật lý `phyaddr = fpn * PAGESZ + offset` và gọi `SYSCALL 17` (`SYSMEM_IO_READ` / `SYSMEM_IO_WRITE`) để đọc/ghi 1 byte.
     * `__read()` / `__write()`: Xác thực vùng nhớ dựa trên `symrgtbl[rgid]` và gọi đọc/ghi.
  2. Phân tách không gian nhớ User và Kernel:
     * `libkmem_malloc()` & `__kmalloc()`: Cấp phát vùng nhớ trong không gian Kernel liên tục.
     * `libkmem_copy_from_user()`: Đọc từ User space và ghi sang Kernel space.
     * `libkmem_copy_to_user()`: Đọc từ Kernel space và ghi sang User space.
     * `__read_kernel_mem()`, `__write_kernel_mem()`, `__read_user_mem()`, `__write_user_mem()`.
  3. Đồng bộ hóa:
     * Đảm bảo mọi thao tác trên cấu trúc nhớ ảo và bộ nhớ nhân đều được bảo vệ bằng `mmvm_lock` và `kmm_lock`.
* **Các bài test kiểm tra:**
  * `os_1_mlq_paging`, `os_1_mlq_paging_small_1K`, `os_1_mlq_paging_small_4K`.
* **Nhiệm vụ trong Báo cáo (Report):**
  * Thống kê và vẽ sơ đồ trạng thái cấp phát bộ nhớ trong Data Segment qua các lệnh `alloc`, `free`, `read`, `write`.
  * Trả lời **Câu hỏi 7 (Trang 25)**: Phân tích các lỗi nghiêm trọng xảy ra nếu thiếu đồng bộ hóa (Race condition, Deadlock, trùng lặp khung trang vật lý).

---

### 🌟 Feature 5: `feat/dma-coproc` — Bộ Đồng Xử Lý DMA (Verilator RTL) & Bus
* **Người phụ trách:** Thành viên 5
* **Các file code cần chỉnh sửa:**
  * `hk261_ossim_demacOprx/src/os.c` (hàm `co_processor`, `ld_routine`)
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
     * Khởi tạo mảng `krnl_pgd` trong `ld_routine()`.
  3. Giao tiếp Bus & Sao chép dữ liệu DMA:
     * `dma_c_testbench.cpp`: Trong `dpi_dma_copy()`, gán địa chỉ nguồn/đích trỏ về vùng nhớ `mram->storage` của OS khi hệ thống chạy thực tế.
     * `sysbus.c`: Đảm bảo hàm callback `dma_done_callback()` in ra thông báo hoàn tất truyền dữ liệu.
     * `libmem.c` (`libmem_dma_memcpy`): Gọi `do_dma()` thực hiện truyền dữ liệu DMA.
* **Các bài test kiểm tra:**
  * Chạy độc lập: `./dma_testbench`
  * Chạy tích hợp toàn hệ thống: `os_2_singleCPU_mlq_paging`, `os_2_mlq_paging` (các test có lệnh `kmalloc`, `dma_memcpy`, `copy_from_user`).
* **Nhiệm vụ trong Báo cáo (Report):**
  * Trình bày cơ chế hoạt động bất đồng bộ của DMA và cách định địa chỉ Bus (CPU truy cập qua MMU vs. DMA truy cập trực tiếp bộ nhớ).
  * Rà soát kiểm tra lần cuối toàn bộ mã nguồn theo chuẩn GNU C và nén file zip nộp bài `assignment_STUDENTID.zip`.

---

## 3. Quy Trình Phối Hợp Làm Việc Trên Git (Git Collaboration Rules)

1. **Tuyệt đối không push code trực tiếp lên nhánh `main`.** Nhánh `main` chỉ chứa code đã chạy ổn định và được nhóm kiểm duyệt.
2. **Quy trình làm việc cá nhân:**
   * Trước khi làm việc: Kéo code mới nhất từ `main`:
     ```bash
     git checkout main
     git pull origin main
     ```
   * Tạo nhánh tính năng cá nhân:
     ```bash
     git checkout -b feat/<ten-tinh-nang>
     ```
   * Code và biên dịch thử nghiệm trên máy ảo Ubuntu (`make all`).
   * Commit rõ ràng theo chuẩn:
     ```bash
     git add .
     git commit -m "feat(module): Mo ta cong viec da hoan thanh"
     git push -u origin feat/<ten-tinh-nang>
     ```
3. **Thứ tự ghép code (Merge Order) vào `main`:**
   1. Ghép `feat/scheduler` (Chạy các bài test độc lập về CPU/Scheduler).
   2. Ghép `feat/mm64-memphy` và `feat/syscall-mmvm`.
   3. Ghép `feat/libmem` (Chạy các bài test Paging và bộ nhớ User/Kernel).
   4. Ghép `feat/dma-coproc` (Chạy các bài test tổng hợp hoàn chỉnh có DMA).

---

## 4. Cơ Cấu Điểm Đánh Giá Môn Học (Theo mục 3.4 của Đề bài)

* **Vấn đáp / Thực hành Demo (7.0 điểm):**
  * **Lập lịch (Scheduling):** 2.0 điểm
  * **MMU và không gian User/Kernel:** 2.0 điểm
  * **Bộ đồng xử lý (Co-processor / DMA):** 3.0 điểm
    * *1.0 điểm:* Cơ chế hoạt động bất đồng bộ (Asynchronous operation).
    * *2.0 điểm:* Định địa chỉ Bus (1.0 điểm CPU truy cập qua MMU, 1.0 điểm Co-processor truy cập bộ nhớ).
* **Báo cáo (Report - 3.0 điểm):** Đánh giá chung toàn bộ biểu đồ Gantt, sơ đồ dịch địa chỉ, bảng phân bổ bộ nhớ và câu trả lời cho 7 câu hỏi lý thuyết.
