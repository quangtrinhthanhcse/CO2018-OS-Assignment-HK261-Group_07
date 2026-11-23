# BÁO CÁO RÀ SOÁT KHÓA ĐỒNG BỘ HÓA & DANH SÁCH LỖI TÍCH HỢP TRÊN `main`
**Mã công việc:** `A1-6` (Hạn 22/11/2026) & `A1-7` (Hạn 23/11/2026)  
**Người thực hiện (Integration Coordinator):** Nguyễn Khắc Triệu (MSSV: 2213619)  
**Ngày kiểm tra:** 23/11/2026  
**Commit kiểm tra trên `main`:** `fb811e6` (*Merge pull request #12 from quangtrinhthanhcse/rynastermun-patch-4*)

---

## 1. Kết Quả Rà Soát Khóa Đồng Bộ Hóa Toàn Dự Án (`A1-6`)

Hệ thống mô phỏng đa CPU (`num_cpus` luồng `cpu_routine`), 1 luồng `ld_routine` và 1 luồng đồng xử lý `co_processor` chạy song song. Dưới đây là kết quả rà soát 3 khóa chính (`queue_lock`, `mmvm_lock`, `kmm_lock`) và khóa thiết bị DMA trên toàn bộ mã nguồn hiện tại:

### 1.1. Bảng tổng hợp hiện trạng các khóa

| Tên khóa | File khai báo | Phạm vi bảo vệ | Các hàm ĐÃ bọc khóa đúng | Các chỗ THIẾU khóa / Rủi ro đồng bộ cần sửa | Người phụ trách sửa |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`queue_lock`** | `src/sched.c:19` (`static pthread_mutex_t`) | Các hàng đợi lập lịch `mlq_ready_queue[0..139]`, mảng `slot[0..139]`, `ready_queue`, `run_queue`, và danh sách tiến trình đang chạy `running_list`. | - `get_mlq_proc()` (`sched.c:64-95`)<br>- `put_mlq_proc()` (`sched.c:113-116`)<br>- `add_mlq_proc()` (`sched.c:132-134`)<br>- `get_proc()`, `put_proc()`, `add_proc()` ở nhánh `#else` (`sched.c:152-202`) | 1. **Khi tiến trình kết thúc (`os.c:81-87`):** `cpu_routine` gọi `free(proc)` trực tiếp mà **chưa rút `proc` khỏi `running_list`** bằng `purgequeue(&running_list, proc)` dưới sự bảo vệ của `queue_lock` $\rightarrow$ để lại con trỏ treo (*dangling pointer*) trong `running_list` khi `__sys_memmap` duyệt `running_list`.<br>2. **Trong `__sys_memmap` (`sys_mem.c`):** Khi A2 cài đặt duyệt `krnl->running_list` để tìm `caller` theo `pid`, cần lưu ý không giữ `queue_lock` quá lâu hoặc phải đảm bảo `running_list` không chứa PCB đã `free`. | **A1 (Nguyễn Khắc Triệu)** phối hợp với **A2 (Phạm Huỳnh Trọng Trí)** |
| **`mmvm_lock`** | `src/libmem.c:25` (`static pthread_mutex_t`) | Cấu trúc vùng nhớ ảo user-space (`caller->mm`, `symrgtbl`, `vm_freerg_list`, `fifo_pgn`, bảng trang 5 cấp) và danh sách khung trang trống `mram->free_fp_list`, `active_mswp->free_fp_list`. | - `__alloc()` (`libmem.c:73-129`)<br>- `__free()` (`libmem.c:143-170`)<br>- `__write()` (`libmem.c:400-413`)<br>- `__kmalloc()` (`libmem.c:597-648`)<br>- `umem_rg_len()` (`libmem.c:754-761`)<br>- `__read_user_mem()` (`libmem.c:889-896`)<br>- `__write_user_mem()` (`libmem.c:913-920`)<br>- `free_pcb_memph()` (`libmem.c:932-952`) | 1. **`__read()` (`libmem.c:353-366`) THIẾU `mmvm_lock`:** Gọi `pg_getval() -> pg_getpage()` có thể kích hoạt swap trang và sửa `fifo_pgn` / PTE mà không có khóa bảo vệ!<br>2. **`libmem_dma_memcpy()` (`libmem.c:662-729`) THIẾU `mmvm_lock`:** Đọc `caller->mm->symrgtbl` và gọi `pg_getpage()` (có thể gây page fault/swap) nhưng chưa khóa `mmvm_lock`. | **A4 (Đặng Hồng Anh)** sửa `__read()`;<br>**A6 (Trần Khan)** sửa `libmem_dma_memcpy()` |
| **`kmm_lock`** | `src/libmem.c:26` (`static pthread_mutex_t`) | Bảng vùng nhớ nhân `ksymrgtbl[KMEM_MAX_RG]`, đỉnh cấp phát `kmem_top`, và bảng trang nhân `krnl->krnl_pgd`. | - `__kmalloc()` (`libmem.c:596-649`)<br>- `kmem_rg_len()` (`libmem.c:737-744`) | **`kmem_access()` chưa được định nghĩa trong `libmem.c`:** Hai hàm `__read_kernel_mem()` (`libmem.c:856`) và `__write_kernel_mem()` (`libmem.c:871`) đang gọi `kmem_access(...)` nhưng hàm này bị thiếu khi commit lên `main`. Khi bổ sung `kmem_access()`, bắt buộc phải bọc `pthread_mutex_lock(&kmm_lock)` và `pthread_mutex_unlock(&kmm_lock)` quanh thao tác tra cứu `kmem_find_rg()`, `kmem_va2pa()` và `MEMPHY_read/write(krnl->mram, ...)`. | **A5 (Nguyễn Hoàng Quốc Mạnh)** |
| **`dma_lock` & `dma_q_lock`** | `src/dma_c_testbench.cpp:68, 171` (`std::mutex`) | Hàng đợi yêu cầu DMA bất đồng bộ `dma_q` và bus phần cứng Verilator RTL `Vdma_top`. | - `dpi_dma_copy()` (`dma_c_testbench.cpp:69`)<br>- `dma_submit()` (`dma_c_testbench.cpp:186`)<br>- `dma_serve()` (`dma_c_testbench.cpp:199`)<br>- `dma_stop()` (`dma_c_testbench.cpp:215`) | Đã bọc khóa đầy đủ ở tầng C++ driver, không có race condition trên hàng đợi `dma_q`. | **A6 (Trần Khan)** *(Đã đạt)* |

### 1.2. Kiểm tra thứ tự lấy khóa (Lock Ordering - Phòng chống Deadlock)
- Trong `__kmalloc()` (`src/libmem.c:596-597`), thứ tự khóa là:
  ```c
  pthread_mutex_lock(&kmm_lock);
  pthread_mutex_lock(&mmvm_lock);
  ...
  pthread_mutex_unlock(&mmvm_lock);
  pthread_mutex_unlock(&kmm_lock);
  ```
- Trong `libkmem_copy_from_user()` và `libkmem_copy_to_user()` (`src/libmem.c:790-839`), các hàm `umem_rg_len()` (khóa `mmvm_lock`), `kmem_rg_len()` (khóa `kmm_lock`), `__read_user_mem()` (khóa `mmvm_lock`), `__write_kernel_mem()` (khóa `kmm_lock`) đều **lấy và nhả từng khóa độc lập (không lồng nhau)**.
- **Kết luận về Deadlock:** Không có chu trình khóa chéo (`mmvm_lock` $\rightarrow$ `kmm_lock`) nào đối nghịch với `__kmalloc` (`kmm_lock` $\rightarrow$ `mmvm_lock`), do đó không xảy ra Deadlock giữa `kmm_lock` và `mmvm_lock`.

---

## 2. Danh Sách Lỗi Kiểm Thử & Tích Hợp Trên Nhánh `main` (`A1-7`)

Khi chạy kiểm tra tích hợp toàn bộ 14 bài test trên nhánh `main` (commit `fb811e6`), các lỗi hiện tại được phân loại theo mức độ nghiêm trọng và giao cụ thể cho từng thành viên phụ trách dưới đây:

### 🔴 Nhóm 1: Lỗi Biên Dịch (Compile/Link Blocker - Chặn `make all`)

| Mã lỗi | File & Dòng | Mô tả lỗi chi tiết | Giải pháp khắc phục | Người nhận (Owner) |
| :-: | :--- | :--- | :--- | :--- |
| **BUG-01** | `src/libmem.c:856, 871, 892, 916` | **Thiếu định nghĩa 2 hàm tĩnh `kmem_access()` và `umem_region()`:**<br>Trong commit `448433b` (PR #12), các hàm `__read_kernel_mem()`, `__write_kernel_mem()` gọi `kmem_access(...)` và `__read_user_mem()`, `__write_user_mem()` gọi `umem_region(...)`, nhưng phần thân của `kmem_access` và `umem_region` chưa được dán vào `src/libmem.c`. Trình biên dịch báo lỗi `implicit declaration of function 'kmem_access' / 'umem_region'` và linker báo `undefined reference`. | Bổ sung định nghĩa 2 hàm helper `static int kmem_access(...)` (có bọc `kmm_lock`, tra `kmem_find_rg`, dịch `kmem_va2pa`, gọi `MEMPHY_read/write`) và `static struct vm_rg_struct *umem_region(...)` phía trước dòng 770 của `src/libmem.c`. | **Nguyễn Hoàng Quốc Mạnh (A5)** |

---

### 🟠 Nhóm 2: Lỗi Chức Năng Chưa Hoàn Thiện (Chặn các test Paging, Syscall & Swap)

| Mã lỗi | File & Dòng | Mô tả lỗi & Bài test bị ảnh hưởng | Giải pháp khắc phục | Người nhận (Owner) |
| :-: | :--- | :--- | :--- | :--- |
| **BUG-02** | `src/sys_mem.c:33-48` | **`__sys_memmap` vẫn dùng `caller` giả lập bằng `malloc`:**<br>Dòng 33-34 đang viết `struct pcb_t *caller = malloc(sizeof(struct pcb_t)); caller->krnl = malloc(sizeof(struct krnl_t));` thay vì tìm PCB thật có `caller->pid == pid` trong `krnl->running_list`.<br>$\rightarrow$ **Hậu quả:** Khi gọi `SYSMEM_INC_OP`, `SYSMEM_MAP_OP`, `SYSMEM_SWP_OP`, hàm thao tác trên `caller->mm` rác (chưa khởi tạo) gây **Segmentation Fault** ở tất cả các bài test paging (`os_0_mlq_paging`, `os_1_*`, `os_2_*`, `os_sc`, `os_syscall`). | Xóa 2 dòng `malloc` giả lập; duyệt `krnl->running_list` để tìm đúng `krnl->running_list->proc[i]->pid == pid` gán cho `caller`. Nếu không tìm thấy trả về `-1`. | **Phạm Huỳnh Trọng Trí (A2)** |
| **BUG-03** | `include/mm.h:102-103` | **Macro `OVERLAP` và `INCLUDE` vẫn đang là stub `(0)`:**<br>`#define OVERLAP(x1,x2,y1,y2) (0)` làm cho hàm `validate_overlap_vm_area()` không phát hiện được vùng nhớ chồng lấn. | Sửa thành:<br>`#define INCLUDE(x1,x2,y1,y2) (((y1) >= (x1)) && ((y2) <= (x2)))`<br>`#define OVERLAP(x1,x2,y1,y2) (!((x2) <= (y1) || (x1) >= (y2)))` | **Phạm Huỳnh Trọng Trí (A2)** |
| **BUG-04** | `src/mm-vm.c:63-80, 131-147` | **`get_vm_area_node_at_brk()` và `inc_vma_limit()` đang để trống (TODO):**<br>- `get_vm_area_node_at_brk()` trả về con trỏ `newrg` chưa khởi tạo.<br>- `inc_vma_limit()` chỉ `return 0;` mà không căn chỉnh kích thước trang (`PAGING64_PAGE_ALIGNSZ`), không nâng `cur_vma->sbrk` / `cur_vma->vm_end`, và không gọi `vm_map_ram()` để cấp phát frame vật lý & ánh xạ bảng trang.<br>$\rightarrow$ **Hậu quả:** `liballoc` không bao giờ ánh xạ được trang mới vào RAM, làm toàn bộ test `os_0_*`, `os_1_*`, `os_2_*` thất bại. | Hiện thực đầy đủ `get_vm_area_node_at_brk()` (lấy `cur_vma = get_vma_by_num(caller->mm, vmaid)`, cấp phát `newrg` có `rg_start = cur_vma->sbrk`, `rg_end = newrg->rg_start + alignedsz`) và `inc_vma_limit()` (kiểm tra `validate_overlap_vm_area`, nâng `sbrk`/`vm_end`, gọi `vm_map_ram`). | **Phạm Huỳnh Trọng Trí (A2)** |
| **BUG-05** | `src/mm-vm.c:114` | **Lỗi logic kiểm tra chồng lấn trong `validate_overlap_vm_area()`:**<br>Dòng 114 đang kiểm tra `OVERLAP(cur_area->vm_start, cur_area->vm_end, vma->vm_start, vma->vm_end)` thay vì kiểm tra vùng nhớ mới dự kiến `[vmastart, vmaend]` với `[vma->vm_start, vma->vm_end]`. Ngoài ra trong `get_vma_by_num()` (`mm-vm.c:43`), nếu `pvma = pvma->vm_next` bằng `NULL` thì lệnh `vmait = pvma->vm_id;` ngay dòng 43 sẽ gây **NULL pointer dereference**. | 1. Trong `get_vma_by_num()`: kiểm tra `if (pvma == NULL) return NULL;` ngay sau `pvma = pvma->vm_next;`.<br>2. Trong `validate_overlap_vm_area()`: sửa đối số `OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end)`. | **Phạm Huỳnh Trọng Trí (A2)** |
| **BUG-06** | `src/libmem.c:256-271` | **`pg_getpage()` chưa hoàn thiện bước hoán đổi trang (Swap Out / Swap In):**<br>Khi `!PAGING_PAGE_PRESENT(pte)` (trang không nằm trong RAM), hàm đã tìm `vicpgn` và `swpfpn`, nhưng phần gọi syscall `SYSMEM_SWP_OP` để chép `vicfpn -> swpfpn`, chép `tgtfpn -> vicfpn`, và cập nhật `pte_set_swap()` / `pte_set_fpn()` vẫn đang bị comment (`//pte_set_swap(...)`, `//pte_set_fpn(...)`).<br>$\rightarrow$ **Hậu quả:** Các test có kích thước RAM nhỏ phải swap (`os_1_mlq_paging_small_1K`, `os_1_mlq_paging_small_4K`) bị lỗi. | Hoàn thiện lấy `vicpte = pte_get_entry(caller->mm, vicpgn, caller)`, `vicfpn = PAGING_FPN(vicpte)`, `tgtfpn = PAGING_SWP(pte)`, gọi `_syscall(..., 17, &regs)` với `SYSMEM_SWP_OP`, cập nhật `pte_set_swap(caller, vicpgn, 0, swpfpn)` và `pte_set_fpn(caller->mm, pgn, vicfpn, caller)`. | **Đặng Hồng Anh (A4)** |
| **BUG-07** | `src/libmem.c:971-980` | **Lỗi `NULL pointer dereference` trong `find_victim_page()` khi danh sách `fifo_pgn` chỉ có 1 phần tử:**<br>Khi `mm->fifo_pgn` chỉ có đúng 1 node (`pg->pg_next == NULL`), vòng lặp `while (pg->pg_next)` không chạy nên `prev` vẫn bằng `NULL`. Dòng 978 thực hiện `prev->pg_next = NULL;` sẽ gây **Segmentation Fault** ngay lập tức! | Kiểm tra `if (prev == NULL) { mm->fifo_pgn = NULL; } else { prev->pg_next = NULL; }` trước khi `free(pg)`. | **Đặng Hồng Anh (A4)** |
| **BUG-08** | `src/libmem.c:357, 363, 401, 403, 411` | **`__read()` và `__write()` truy cập nhầm `caller->krnl->mm` thay vì `caller->mm`:**<br>- Trong `__read()`: `get_vma_by_num(caller->krnl->mm, vmaid)` và `pg_getval(caller->krnl->mm, ...)` dùng `caller->krnl->mm` (bảng trang rỗng của OS) thay vì `caller->mm` của tiến trình!<br>- Trong `__write()`: cả `get_symrg_byid(caller->krnl->mm, rgid)`, `get_vma_by_num(caller->krnl->mm, vmaid)` và `pg_setval(caller->krnl->mm, ...)` đều dùng nhầm `caller->krnl->mm`, khiến lệnh `write` luôn thất bại vì `symrgtbl` của `caller` nằm ở `caller->mm`! | Đổi toàn bộ `caller->krnl->mm` trong `__read()` và `__write()` thành `caller->mm`, đồng thời thêm `pthread_mutex_lock(&mmvm_lock)` / `pthread_mutex_unlock(&mmvm_lock)` bao quanh `__read()`, và kiểm tra `offset` nằm trong phạm vi `[0, currg->rg_end - currg->rg_start)`. | **Đặng Hồng Anh (A4)** |

---

### 🟡 Nhóm 3: Lỗi Khớp Định Dạng Output Mẫu (`diff` với `output/*.output`)

| Mã lỗi | File & Dòng | Mô tả lệch output so với `output/` | Giải pháp khắc phục | Người nhận (Owner) |
| :-: | :--- | :--- | :--- | :--- |
| **BUG-09** | `src/libmem.c:180, 211, 376` | **Vị trí/thiếu câu lệnh `printf("%s:%d\n", __func__, __LINE__);` trong `liballoc`, `libfree`, `libread`, `libwrite`:**<br>Trong `output/os_0_mlq_paging.output` (dòng 29) và `output/os_1_mlq_paging.output` (dòng 27, 80) có in `liballoc:178`, `libfree:218`... Cần đối chiếu chính xác hàm nào in dòng này và tắt các debug print thừa để khớp `diff`. | Kiểm tra lại các file `output/os_*.output` để đặt `printf("%s:%d\n", __func__, __LINE__);` đúng hàm (`liballoc`, `libfree`, `libread`, `libwrite`). | **Đặng Hồng Anh (A4)** |
| **BUG-10** | `src/mm64.c:625` | **Định dạng `print_pgtbl()` giữa `os_0`/`os_1` và `os_2`:**<br>- `output/os_0_mlq_paging.output` & `os_1_mlq_paging.output` dùng định dạng: ` PDG=%llx P4g=%llx PUD=%llx PMD=%llx\n`.<br>- `output/os_2_mlq_paging.output` có thêm `PT=...`. Hiện tại `mm64.c:625` đang in theo chuẩn của `os_0` và `os_1`. | Giữ chuẩn ` PDG=%llx P4g=%llx PUD=%llx PMD=%llx\n` cho bộ test paging chuẩn và ghi chú rõ trong báo cáo khi so sánh `os_2`. | **Trịnh Thanh Quang (A3)** |
| **BUG-11** | `src/libmem.c:680-696` | **`libmem_dma_memcpy()` hiện chỉ tra `caller->mm->symrgtbl` (user region), từ chối vùng nhớ cấp phát bởi `kmalloc`:**<br>Trong trường hợp kịch bản test gọi `dma_memcpy` có tham chiếu tới vùng nhớ kernel hoặc user, cần kiểm tra xem `input/proc/m2s` truyền `rgid` của `alloc` hay `kmalloc` (trong `m2s`: `kmalloc 1024 0`, `alloc 1024 1`, `alloc 1024 2`, `dma_memcpy 1 2 0 4` $\rightarrow$ `rgid 1` và `2` đều là vùng user `alloc`, nên `symrgtbl` xử lý đúng; tuy nhiên cần bọc `mmvm_lock` quanh `libmem_dma_memcpy`). | Bọc `pthread_mutex_lock(&mmvm_lock)` trong `libmem_dma_memcpy()` khi tra cứu và gọi `pg_getpage()`. | **Trần Khan (A6)** |

---

## 3. Bảng Phân Công Xử Lý Lỗi Sau Tích Hợp (Theo Kế Hoạch Tuần 3)

| Thành viên | Mã việc | Các mã lỗi được giao xử lý | Hạn hoàn thành |
| :--- | :---: | :--- | :---: |
| **Nguyễn Khắc Triệu** | `A1-8` | Phối hợp với A2 kiểm tra `running_list` khi tiến trình kết thúc (`os.c`); chạy lại `./run_tests.sh` sau mỗi bản vá của các thành viên. | **27/11/2026** |
| **Phạm Huỳnh Trọng Trí** | `A2-4`, `A2-5`, `A2-6` | **BUG-02** (`__sys_memmap` PID passing), **BUG-03** (`OVERLAP` macro), **BUG-04** (`get_vm_area_node_at_brk` & `inc_vma_limit`), **BUG-05** (`validate_overlap_vm_area` & `get_vma_by_num`). | **27/11/2026** |
| **Trịnh Thanh Quang** | `A3-6`, `A3-8` | **BUG-10** (Kiểm tra `os_0_mlq_paging`, `os_1_singleCPU_mlq_paging` ngay sau khi A2 push bản vá `mm-vm.c` và `sys_mem.c`). | **27/11/2026** |
| **Đặng Hồng Anh** | `A4-7` | **BUG-06** (`pg_getpage` swap in/out), **BUG-07** (`find_victim_page` 1-node NULL check), **BUG-08** (`caller->mm` & `mmvm_lock` trong `__read`/`__write`), **BUG-09** (`printf` dòng log). | **27/11/2026** |
| **Nguyễn Hoàng Quốc Mạnh** | `A5-5`, `A5-6`, `A5-7` | **BUG-01** (Bổ sung hàm `kmem_access` có bọc `kmm_lock` và hàm `umem_region` vào `src/libmem.c` để `main` biên dịch sạch lỗi). | **27/11/2026** |
| **Trần Khan** | `A6-5`, `A6-6` | **BUG-11** (Bổ sung khóa `mmvm_lock` trong `libmem_dma_memcpy` và kiểm tra `os_2_*` sau khi A2, A4, A5 vá xong). | **27/11/2026** |
