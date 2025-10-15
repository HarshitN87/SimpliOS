# Memory Management

## Physical Memory Manager (PMM)
- Current implementation is a stub that acknowledges Multiboot handoff.
- Future: parse Multiboot memory map, build a frame bitmap, and expose `alloc_frame`/`free_frame`.

## Paging (Virtual Memory)
- Identity maps first 4 MiB using a single page table and page directory entry.
- Loads CR3 with the page directory and sets PG bit in CR0 to enable paging.
- All pages are present and read/write; no user/supervisor separation yet.

## Plans
- Switch to higher-half kernel mapping.
- Add per-process address spaces and user mode segmentation.
- Demand paging and page fault handler.
