# SimpliOS Kernel API Reference

This document provides a reference for the internal C APIs available in the SimpliOS kernel. These functions are used by the kernel itself, drivers, and built-in applications.

## 1. VGA Console (`kernel/kernel.c`)

The VGA console API allows printing text to the screen.

### `void term_print(const char* str)`
Prints a null-terminated string to the console at the current cursor position. Handles newlines (`\n`) by moving to the next line and scrolling if necessary.

### `void term_putc(char c)`
Prints a single character to the console.
- **`\n`**: Moves cursor to the start of the next line.
- **`\b`**: Moves cursor back one position (non-destructive).
- **Other**: Prints the character and advances the cursor.

### `void term_clear(void)`
Clears the entire screen (fills with spaces) and resets the cursor to (0, 0).

### `void term_set_color(uint8_t color)`
Sets the current text color.
- **`color`**: A byte where the high nibble is the background color and the low nibble is the foreground color.
- **Example**: `term_set_color(0x02)` sets green text on a black background.

### `void term_set_col(int col)` / `void term_set_row(int row)`
Sets the cursor's column (0-79) or row (0-24) position.

## 2. Ramdisk Filesystem (`kernel/ramdisk.c`)

The Ramdisk API manages files stored in memory.

### `int ramdisk_create_file(const char* filename, file_type_t type)`
Creates a new file.
- **Returns**: `0` on success, negative error code on failure.
- **Errors**: `-1` (Invalid name), `-2` (Full), `-3` (Exists).

### `int ramdisk_write_file(const char* filename, const void* data, uint32_t size)`
Writes data to an existing file.
- **Returns**: Number of bytes written, or negative error code.

### `int ramdisk_read_file(const char* filename, void* buffer, uint32_t max_size)`
Reads data from a file into a buffer.
- **Returns**: Number of bytes read, or negative error code.

### `int ramdisk_delete_file(const char* filename)`
Deletes a file and frees its slot.
- **Returns**: `0` on success, `-1` if not found.

### `int ramdisk_list_files(char* buffer, uint32_t buffer_size)`
Writes a newline-separated list of filenames into the provided buffer.

## 3. Scheduler (`kernel/scheduler.c`)

The scheduler API allows managing processes.

### `void scheduler_add_process(const char* name, uint32_t priority)`
Creates a new process (PCB) and adds it to the ready queue.
- **`name`**: A human-readable name for the process.
- **`priority`**: Priority level (currently unused by the round-robin logic).

### `void scheduler_yield(void)`
Voluntarily gives up the CPU to the next process in the ready queue.

## 4. Game Loop Hook (`kernel/kernel.c`)

### `extern volatile void (*g_main_loop_hook)(void)`
A function pointer that, if set, is called repeatedly from the kernel's idle loop.
- **Usage**: Set this pointer to your game's update/render function to run it at maximum speed (or throttled by the PIT) without blocking interrupts.
- **Example**:
  ```c
  void my_game_loop() {
      // Update and render
  }
  
  void start_game() {
      g_main_loop_hook = my_game_loop;
  }
  ```
