# Bản nháp B5: Hoạt động bất đồng bộ và Định địa chỉ Bus

Phần báo cáo của A5/B5. Bám theo mục 2.3 của đề (Hình 8, 9, 10, Bảng 3) và code trong `hk261_ossim_demacOprx/`.
Sơ đồ luồng chi tiết xem [dma_flow.md](dma_flow.md).

---

## 1. Hoạt động bất đồng bộ (Asynchronous operation)

### 1.1 Vì sao cần DMA

Nếu không có DMA (Programmed I/O), muốn chép `len` byte từ vùng A sang vùng B thì CPU phải tự làm từng bước:
đọc 1 đơn vị dữ liệu từ A vào thanh ghi, ghi nó ra B, rồi lặp lại `len` lần.
Suốt thời gian đó CPU không làm được việc gì khác, chu kỳ CPU bị phí vào việc chép dữ liệu.

Với DMA, CPU chỉ **giao việc** (nguồn, đích, độ dài) rồi quay lại chạy tiến trình khác.
Bộ đồng xử lý DMA tự nắm bus hệ thống để chép dữ liệu, xong thì **báo lại** cho CPU qua ngắt hoặc callback.
Hai bên chạy song song, không bên nào phải đứng chờ bên kia: đó là hoạt động bất đồng bộ (Hình 8 của đề).

### 1.2 Ba giai đoạn và tín hiệu tương ứng

| Giai đoạn | CPU / phần mềm làm gì | Tín hiệu của chip DMA (Bảng 3) | Chỗ trong code |
|---|---|---|---|
| **1. Giao việc** | Reset kênh, ghi descriptor gồm nguồn, đích, độ dài, rồi bật kênh | `rst_n`, `desc_data_0` = nguồn, `desc_data_1` = đích, `desc_data_2` = độ dài, `desc_data_3` dự phòng, `desc_valid = 1`, `ch_enable = 1` | `dpi_rtl_dma_copy`, `src/dpi_rtl.cpp:80` |
| **2. DMA tự chạy** | Không cần can thiệp (chỉ cấp xung clock cho mô phỏng) | Theo từng nhịp `clk`, chip đọc 4 byte ở nguồn, ghi 4 byte ra đích, rồi đọc lại để kiểm tra; `state` cho biết trạng thái hiện tại, `ch_bytes_transferred` đếm số byte đã chép | `src/Vdma_top___024root__DepSet_hbdce0907__0.cpp:140, 144` |
| **3. Báo xong** | Chạy hàm xử lý khi xong (callback), nhận kết quả rồi chạy tiếp | `ch_done` lên 1; ở **cạnh lên** của `ch_done` chip gọi `dma_done_callback()`; `ch_error` báo lỗi nếu có | `src/sysbus.c:149`, `src/Vdma_top___024root__DepSet_hbdce0907__0.cpp:334` |

Tên chân: Bảng 3 của đề gọi là `ch_en`, `dat_valid`, `dat_0`..`dat_3`, `ch_xfer_count`; trong `include/Vdma_top.h` tên tương ứng là `ch_enable`, `desc_valid`, `desc_data_0`..`desc_data_3`, `ch_bytes_transferred`.

Ghi chú về ngắt: chip có chân `irq = ch_done AND intr_en` (`src/Vdma_top___024root__DepSet_hbdce0907__0.cpp:367`),
nhưng trong bản RTL được cung cấp, `intr_en` luôn bằng 0. Vì vậy tín hiệu báo xong dùng được trong mô phỏng là **callback** `dma_done_callback()`,
đóng vai trò giống "trình xử lý ngắt" ở Bước 4 của Hình 8.

### 1.3 Hiện trạng code và hướng làm cho bất đồng bộ thật

**Hiện trạng:** luồng hiện tại vẫn **đồng bộ**.
`dpi_rtl_dma_copy` tự quay vòng `while (cycle < max_cycles && !top->ch_done)` (`src/dpi_rtl.cpp:118`),
nên luồng CPU gọi `do_dma` phải đứng chờ tới khi chép xong (tối đa 100 000 nhịp).
Phần bất đồng bộ đang là TODO trong `co_processor` (`src/os.c:121`), `do_dma` (`src/dma_c_testbench.cpp:136`) và `libmem_dma_memcpy` (`src/libmem.c:467`).

**Hướng hiện thực (đề xuất, chưa code):**

1. Hệ điều hành đã có sẵn một luồng riêng cho bộ đồng xử lý: `pthread_create(dma, NULL, co_processor, NULL)` (`src/os.c:317`).
   Luồng này đóng vai "phần cứng DMA chạy song song với các CPU".
2. Khi tiến trình chạy lệnh `dma_memcpy`, `libmem_dma_memcpy` chỉ **ghi một yêu cầu** (nguồn, đích, độ dài, tiến trình gọi) vào hàng đợi yêu cầu DMA, đánh thức luồng `co_processor`, rồi trả về ngay. CPU tiếp tục chạy lệnh khác.
3. Luồng `co_processor` lấy yêu cầu ra, gọi `do_dma`, chip chạy.
4. Khi xong, `dma_done_callback()` đánh dấu yêu cầu đã hoàn tất (và đánh thức tiến trình đang chờ dữ liệu, nếu có).
5. Vì nhiều CPU có thể cùng gửi yêu cầu mà chip chỉ có một kênh, hàng đợi yêu cầu phải được bảo vệ bằng mutex
   (đề mục 2.4 nhắc phần cứng DMA cũng là tài nguyên dùng chung cần đồng bộ hóa).

---

## 2. Định địa chỉ Bus (Bus addressing)

Ý chính: **CPU và DMA cùng đọc ghi một RAM vật lý (`mram`), nhưng đi bằng hai đường khác nhau.**
CPU dùng địa chỉ ảo và phải qua MMU; DMA dùng địa chỉ vật lý (địa chỉ bus) và đi thẳng tới RAM.

### 2.1 CPU truy cập bộ nhớ qua MMU

Khi tiến trình chạy lệnh `read`/`write`, CPU chỉ biết **địa chỉ ảo** của tiến trình đó. Đường đi:

1. Địa chỉ ảo 64-bit được tách thành các chỉ số 5 cấp: PGD (bit 56..48), P4D (47..39), PUD (38..30), PMD (29..21), PT (20..12) và offset trong trang (11..0) (`include/mm64.h`).
2. MMU đi qua 5 cấp bảng trang của tiến trình để lấy PTE. Nếu bit Present = 0 (trang đang nằm ở SWAP) thì `pg_getpage` phải swap trang vào RAM trước.
3. Từ PTE lấy được số khung trang `fpn`, tính địa chỉ vật lý: `phyaddr = fpn * PAGESZ + offset` (PAGESZ = 4096 ở chế độ 64-bit).
4. Đọc/ghi 1 byte ở `mram->storage[phyaddr]` qua SYSCALL 17 (`SYSMEM_IO_READ` / `SYSMEM_IO_WRITE`) (`pg_getval` / `pg_setval`, `src/libmem.c:276, 302`).

Nhờ vậy mỗi tiến trình có không gian địa chỉ riêng, không đọc nhầm được bộ nhớ của tiến trình khác hay của kernel.

### 2.2 DMA truy cập bộ nhớ trực tiếp

DMA **không có MMU** và không biết bảng trang của tiến trình nào. Nó chỉ nhận 3 con số trong descriptor:
`desc_data_0` = địa chỉ nguồn, `desc_data_1` = địa chỉ đích, `desc_data_2` = độ dài, đều là **offset vật lý tính từ đầu RAM**.

- Theo đề: `ADDR_WIDTH` = 64 bit, `DATA_WIDTH` = 32 bit, nên mỗi lần đọc/ghi trên bus chip chuyển 4 byte.
- Chip đọc ghi RAM qua bus hệ thống ở `src/sysbus.c`: `dpi_bank_read32_legacy(offset)` (dòng 118) và `dpi_bank_write32_legacy(offset, value)` (dòng 135).
  Hai hàm này đổi offset thành con trỏ bằng `memory_bank_get_ptr(offset)` (dòng 97), tức là `địa chỉ bắt đầu của RAM + offset`, không tra bảng trang.
- Đề khuyến khích phần mềm xử lý dữ liệu theo từng byte nếu cần, vì gói 4 byte có thể bị đảo thứ tự byte (endianness).

**Hệ quả quan trọng:** trước khi giao việc cho DMA, phần mềm phải **tự dịch địa chỉ ảo ra địa chỉ vật lý**.
Đó là việc của `libmem_dma_memcpy`: từ số vùng nhớ (ví dụ lệnh `dma_memcpy 1 2 0 4` trong `input/proc/m2s` chép từ vùng 1 sang vùng 2), tìm địa chỉ bắt đầu của vùng, tra bảng trang để ra `fpn`, rồi tính `fpn * PAGESZ + offset` để đưa vào descriptor.

**Vì sao kernel memory phải liên tục về mặt vật lý:** descriptor chỉ chứa *một* địa chỉ bắt đầu và *một* độ dài, nên DMA hiểu vùng cần chép là một khúc liên tục trên RAM vật lý.
Đề yêu cầu `kmalloc` cấp vùng **liên tục về mặt vật lý** (mục 1.4, lệnh `kmalloc`); một lợi ích trực tiếp là vùng kernel dùng được ngay làm nguồn/đích DMA bằng một descriptor duy nhất (đây là nhóm tự suy luận, đề không nói thẳng).
Ngược lại, vùng nhớ user liên tục về địa chỉ ảo nhưng các trang có thể nằm rải rác trên RAM vật lý,
nên nếu vùng user vượt qua ranh giới trang thì phải chia thành nhiều lần chép, mỗi lần nằm gọn trong một trang (đề xuất, cần kiểm tra lại khi hiện thực).

### 2.3 So sánh hai đường truy cập

| | CPU (qua MMU) | DMA (trực tiếp qua bus) |
|---|---|---|
| Loại địa chỉ | Địa chỉ ảo của tiến trình | Địa chỉ vật lý (offset trong RAM) |
| Có dịch địa chỉ không | Có, qua bảng trang 5 cấp | Không |
| Ai dịch địa chỉ | MMU / `pg_getpage`, mỗi lần truy cập | Phần mềm, một lần trước khi giao việc (`libmem_dma_memcpy`) |
| Đơn vị mỗi lần truy cập | 1 byte (`MEMPHY_read` / `MEMPHY_write`) | 4 byte (`read32` / `write32`) |
| Trang đang ở SWAP | Tự swap vào RAM khi gặp | Không xử lý được, trang phải có sẵn trong RAM trước khi giao việc |
| Bảo vệ bộ nhớ | Có, tiến trình chỉ thấy vùng của mình | Không, chép được bất kỳ đâu trong RAM, nên chỉ kernel mới được giao việc cho DMA |

### 2.4 Lưu ý khi hiện thực (cho A5)

- Trong `./dma_testbench`, "RAM" là một `memory_bank` 1 MB (`memory_bank_init`, `src/sysbus.c:34`).
  Khi chạy trong OS (`./os`), RAM thật là `caller->krnl->mram->storage`, và nhánh OS trong `dpi_dma_copy` (`src/dma_c_testbench.cpp:55`) vẫn đang là TODO.
- Không chỉ `dpi_dma_copy`: các hàm bus mà chip gọi (`dpi_bank_read32_legacy` / `dpi_bank_write32_legacy`) cũng đi qua `memory_bank_get_ptr`, hàm này chỉ biết `g_banks[0]`.
  Nếu không có memory bank nào được khởi tạo thì `memory_bank_get_ptr` trả về `NULL` và hai hàm trên sẽ `memcpy` vào `NULL`.
  Vì vậy khi tích hợp vào OS, bus phải được nối tới `mram->storage` (ví dụ cho bank 0 trỏ vào `mram->storage`), không chỉ sửa `dpi_dma_copy`.
