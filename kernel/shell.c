#include "shell.h"
#include "ramdisk.h"
#include "scheduler.h"
#include "pcb.h"
#include "irq.h"
#include <stddef.h>

// VGA color codes
#define VGA_COLOR_BLACK      0x0
#define VGA_COLOR_BLUE       0x1
#define VGA_COLOR_GREEN      0x2
#define VGA_COLOR_CYAN       0x3
#define VGA_COLOR_RED        0x4
#define VGA_COLOR_MAGENTA    0x5
#define VGA_COLOR_BROWN      0x6
#define VGA_COLOR_LIGHTGRAY  0x7
#define VGA_COLOR_DARKGRAY   0x8
#define VGA_COLOR_LIGHTBLUE  0x9
#define VGA_COLOR_LIGHTGREEN 0xA
#define VGA_COLOR_LIGHTCYAN  0xB
#define VGA_COLOR_LIGHTRED   0xC
#define VGA_COLOR_LIGHTMAG   0xD
#define VGA_COLOR_YELLOW     0xE
#define VGA_COLOR_WHITE      0xF

#define SHELL_COLOR_PROMPT  VGA_COLOR_LIGHTCYAN
#define SHELL_COLOR_ERROR   VGA_COLOR_LIGHTRED
#define SHELL_COLOR_INFO    VGA_COLOR_LIGHTGREEN
#define SHELL_COLOR_WARNING VGA_COLOR_YELLOW

// External functions from kernel.c
extern void term_print(const char* str);
extern void term_putc(char c);
extern void term_clear(void);
extern void term_move_left(void);
extern void term_move_right(void);
extern void term_clear_to_eol(void);
extern int term_get_col(void);
extern void term_set_col(int col);
extern int term_get_row(void);
extern void term_set_row(int row);
extern uint8_t term_get_color(void);
extern void term_set_color(uint8_t color);
extern uint8_t term_get_default_color(void);
extern void term_set_default_color(uint8_t color);
extern void term_reset_color(void);
extern volatile unsigned short* vga_buffer;
extern const int VGA_COLS;
extern const int VGA_ROWS;

// String utility functions (freestanding environment)
static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static char tolower_char(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

static int strieq(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = tolower_char(*s1++);
        char c2 = tolower_char(*s2++);
        if (c1 != c2) {
            return 0;
        }
    }
    return (*s1 == '\0' && *s2 == '\0');
}

static void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Global shell state
shell_state_t g_shell;

// Color helpers
static uint8_t shell_color_push(uint8_t color) {
    uint8_t previous = term_get_color();
    term_set_color(color);
    return previous;
}

static void shell_color_pop(uint8_t previous) {
    term_set_color(previous);
}

static void shell_print_colored(const char* text, uint8_t color) {
    uint8_t prev = shell_color_push(color);
    term_print(text);
    shell_color_pop(prev);
}

static void shell_print_info(const char* text) {
    shell_print_colored(text, SHELL_COLOR_INFO);
}

static void shell_print_warning(const char* text) {
    shell_print_colored(text, SHELL_COLOR_WARNING);
}

static void shell_print_error(const char* text) {
    shell_print_colored(text, SHELL_COLOR_ERROR);
}

typedef struct {
    const char* name;
    uint8_t value;
} color_entry_t;

static const color_entry_t color_table[] = {
    {"black", VGA_COLOR_BLACK},
    {"blue", VGA_COLOR_BLUE},
    {"green", VGA_COLOR_GREEN},
    {"cyan", VGA_COLOR_CYAN},
    {"red", VGA_COLOR_RED},
    {"magenta", VGA_COLOR_MAGENTA},
    {"brown", VGA_COLOR_BROWN},
    {"lightgray", VGA_COLOR_LIGHTGRAY},
    {"darkgray", VGA_COLOR_DARKGRAY},
    {"lightblue", VGA_COLOR_LIGHTBLUE},
    {"lightgreen", VGA_COLOR_LIGHTGREEN},
    {"lightcyan", VGA_COLOR_LIGHTCYAN},
    {"lightred", VGA_COLOR_LIGHTRED},
    {"pink", VGA_COLOR_LIGHTMAG},
    {"yellow", VGA_COLOR_YELLOW},
    {"white", VGA_COLOR_WHITE}
};

static int shell_lookup_color(const char* name, uint8_t* out_value) {
    if (!name || !out_value) return 0;
    for (size_t i = 0; i < sizeof(color_table) / sizeof(color_table[0]); i++) {
        if (strieq(name, color_table[i].name)) {
            *out_value = color_table[i].value;
            return 1;
        }
    }
    return 0;
}

static void shell_print_color_options(void) {
    uint8_t prev = shell_color_push(SHELL_COLOR_WARNING);
    term_print("Available colors: ");
    for (size_t i = 0; i < sizeof(color_table) / sizeof(color_table[0]); i++) {
        term_print(color_table[i].name);
        if (i < (sizeof(color_table) / sizeof(color_table[0])) - 1) {
            term_print(", ");
        }
    }
    term_print("\n");
    shell_color_pop(prev);
}

// Available commands
static const shell_command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear the screen", cmd_clear},
    {"ls", "List files in ramdisk", cmd_ls},
    {"cat", "Display file contents", cmd_cat},
    {"echo", "Echo arguments to console", cmd_echo},
    {"create", "Create a new file", cmd_create},
    {"delete", "Delete a file", cmd_delete},
    {"write", "Write text to a file", cmd_write},
    {"read", "Read file contents", cmd_read},
    {"status", "Show system status", cmd_status},
    {"ps", "List processes (PID | Name | State | CPU ticks)", cmd_ps},
    {"uptime", "Show system uptime", cmd_uptime},
    {"setcolor", "Set default console colors", cmd_setcolor}
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

// Initialize the shell
void shell_init(void) {
    g_shell.line_pos = 0;
    g_shell.cursor_pos = 0;
    g_shell.history_pos = 0;
    g_shell.history_count = 0;
    g_shell.echo_enabled = 1;
    g_shell.current_line[0] = '\0';
    
    // Clear history
    for (int i = 0; i < MAX_HISTORY; i++) {
        g_shell.history[i][0] = '\0';
    }
    
    term_print("SimpliOS Shell initialized.\n");
    shell_print_prompt();
}

// Print shell prompt
void shell_print_prompt(void) {
    uint8_t prev = shell_color_push(SHELL_COLOR_PROMPT);
    term_print("simplios> ");
    shell_color_pop(prev);
    term_reset_color();
}

// Redraw the current line (used after history navigation or cursor movement)
static void shell_redraw_line(void) {
    // Clear from cursor to end of line
    term_clear_to_eol();
    
    // Print the rest of the line from cursor position
    for (uint32_t i = g_shell.cursor_pos; i < g_shell.line_pos; i++) {
        term_putc(g_shell.current_line[i]);
    }
    
    // Move cursor back to original position
    // We printed (line_pos - cursor_pos) characters
    for (uint32_t i = 0; i < (g_shell.line_pos - g_shell.cursor_pos); i++) {
        term_move_left();
    }
}

// Insert character at cursor position
static void shell_insert_char(char c) {
    if (g_shell.line_pos >= MAX_COMMAND_LEN - 1) return;
    if (g_shell.cursor_pos > g_shell.line_pos) g_shell.cursor_pos = g_shell.line_pos;
    
    // If inserting at end, just append
    if (g_shell.cursor_pos == g_shell.line_pos) {
        g_shell.current_line[g_shell.line_pos++] = c;
        g_shell.current_line[g_shell.line_pos] = '\0';
        g_shell.cursor_pos++;
        term_putc(c);
        return;
    }
    
    // Shift characters to the right
    for (uint32_t i = g_shell.line_pos; i > g_shell.cursor_pos; i--) {
        g_shell.current_line[i] = g_shell.current_line[i - 1];
    }
    
    // Insert new character
    g_shell.current_line[g_shell.cursor_pos] = c;
    g_shell.line_pos++;
    g_shell.current_line[g_shell.line_pos] = '\0';
    
    // Redraw from cursor position
    shell_redraw_line();
    g_shell.cursor_pos++;
}

// Delete character at cursor position (backspace)
static void shell_delete_char(void) {
    if (g_shell.cursor_pos == 0) return;
    if (g_shell.line_pos == 0) return;
    
    // Move cursor back first (visually)
    g_shell.cursor_pos--;
    term_move_left();
    
    // Shift characters to the left (delete the character before cursor)
    for (uint32_t i = g_shell.cursor_pos; i < g_shell.line_pos - 1; i++) {
        g_shell.current_line[i] = g_shell.current_line[i + 1];
    }
    
    g_shell.line_pos--;
    g_shell.current_line[g_shell.line_pos] = '\0';
    
    // Redraw from cursor position to show updated line
    shell_redraw_line();
}

// Clear current line
void shell_clear_line(void) {
    g_shell.line_pos = 0;
    g_shell.current_line[0] = '\0';
    term_print("\n");
}

// Process arrow key input
void shell_process_arrow_key(uint8_t arrow_type) {
    // 0=Up, 1=Down, 2=Left, 3=Right
    
    if (arrow_type == 0) {  // Up arrow - go to previous history
        if (g_shell.history_count == 0) return;
        
        // Navigate backwards in history
        if (g_shell.history_pos < g_shell.history_count) {
            g_shell.history_pos++;
            uint32_t hist_idx = g_shell.history_count - g_shell.history_pos;
            
            // Load history entry
            strcpy(g_shell.current_line, g_shell.history[hist_idx]);
            g_shell.line_pos = strlen(g_shell.current_line);
            g_shell.cursor_pos = g_shell.line_pos;
            
            // Clear and redraw
            term_clear_to_eol();
            term_print(g_shell.current_line);
        }
    } else if (arrow_type == 1) {  // Down arrow - go to next history
        if (g_shell.history_pos == 0) return;
        
        g_shell.history_pos--;
        
        if (g_shell.history_pos == 0) {
            // Back to empty line
            g_shell.current_line[0] = '\0';
            g_shell.line_pos = 0;
            g_shell.cursor_pos = 0;
            term_clear_to_eol();
        } else {
            uint32_t hist_idx = g_shell.history_count - g_shell.history_pos;
            strcpy(g_shell.current_line, g_shell.history[hist_idx]);
            g_shell.line_pos = strlen(g_shell.current_line);
            g_shell.cursor_pos = g_shell.line_pos;
            term_clear_to_eol();
            term_print(g_shell.current_line);
        }
    } else if (arrow_type == 2) {  // Left arrow - move cursor left
        if (g_shell.cursor_pos > 0) {
            g_shell.cursor_pos--;
            term_move_left();
        }
    } else if (arrow_type == 3) {  // Right arrow - move cursor right
        if (g_shell.cursor_pos < g_shell.line_pos) {
            g_shell.cursor_pos++;
            term_move_right();
        }
    }
}

// Process a character input
void shell_process_char(char c) {
    if (!g_shell.echo_enabled) {
        // Handle special characters without echo
        if (c == '\n') {
            shell_execute_command(g_shell.current_line);
            return;
        }
        return;
    }
    
    switch (c) {
        case '\n':
        case '\r':
            term_print("\n");
            // Add to history if not empty
            if (g_shell.line_pos > 0) {
                // Shift history down if full
                if (g_shell.history_count >= MAX_HISTORY) {
                    for (int i = 0; i < MAX_HISTORY - 1; i++) {
                        strcpy(g_shell.history[i], g_shell.history[i + 1]);
                    }
                    g_shell.history_count = MAX_HISTORY - 1;
                }
                // Add new command to history
                strcpy(g_shell.history[g_shell.history_count], g_shell.current_line);
                g_shell.history_count++;
            }
            g_shell.history_pos = 0;  // Reset history position
            shell_execute_command(g_shell.current_line);
            break;
            
        case '\b':
            shell_delete_char();
            break;
            
        case '\t':
            // Tab completion (simplified - just add spaces)
            if (g_shell.line_pos < MAX_COMMAND_LEN - 1) {
                shell_insert_char(' ');
            }
            break;
            
        default:
            if (c >= 32 && c <= 126) {
                shell_insert_char(c);
            }
            break;
    }
}

// Parse command line arguments
int shell_parse_args(const char* line, char* argv[], int max_args) {
    if (!line || !argv || max_args < 1) return 0;
    
    int argc = 0;
    int i = 0;
    int in_word = 0;
    int arg_start = 0;
    
    while (line[i] && argc < max_args - 1) {
        if (line[i] == ' ' || line[i] == '\t') {
            if (in_word) {
                // End of word
                int len = i - arg_start;
                if (len < MAX_ARG_LEN) {
                    strcpy(argv[argc], &line[arg_start]);
                    argv[argc][len] = '\0';
                    argc++;
                }
                in_word = 0;
            }
        } else {
            if (!in_word) {
                // Start of word
                arg_start = i;
                in_word = 1;
            }
        }
        i++;
    }
    
    // Handle last argument
    if (in_word && argc < max_args - 1) {
        int len = i - arg_start;
        if (len < MAX_ARG_LEN) {
            strcpy(argv[argc], &line[arg_start]);
            argv[argc][len] = '\0';
            argc++;
        }
    }
    
    argv[argc] = NULL; // Null terminate
    return argc;
}

// Execute a command
void shell_execute_command(const char* line) {
    if (!line || strlen(line) == 0) {
        g_shell.line_pos = 0;
        g_shell.current_line[0] = '\0';
        shell_print_prompt();
        return;
    }
    
    char argv[MAX_ARGS][MAX_ARG_LEN];
    char* argv_ptrs[MAX_ARGS];
    
    for (int i = 0; i < MAX_ARGS; i++) {
        argv_ptrs[i] = argv[i];
    }
    
    int argc = shell_parse_args(line, argv_ptrs, MAX_ARGS);
    
    if (argc == 0) {
        shell_print_prompt();
        return;
    }
    
    // Find and execute command
    int found = 0;
    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(commands[i].name, argv[0]) == 0) {
            commands[i].handler(argc, argv_ptrs);
            found = 1;
            break;
        }
    }
    
    if (!found) {
        uint8_t prev = shell_color_push(SHELL_COLOR_ERROR);
        term_print("Command not found: ");
        term_print(argv[0]);
        term_print("\nType 'help' for available commands.\n");
        shell_color_pop(prev);
    }
    
    g_shell.line_pos = 0;
    g_shell.cursor_pos = 0;
    g_shell.current_line[0] = '\0';
    term_reset_color();
    shell_print_prompt();
}

// Command implementations

int cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv; // Suppress unused parameter warnings
    
    shell_print_warning("Available commands:\n");
    for (int i = 0; i < NUM_COMMANDS; i++) {
        term_print("  ");
        term_print(commands[i].name);
        term_print(" - ");
        term_print(commands[i].description);
        term_print("\n");
    }
    return 0;
}

int cmd_clear(int argc, char* argv[]) {
    (void)argc; (void)argv;
    term_clear();
    return 0;
}

int cmd_ls(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    char buffer[1024];
    int result = ramdisk_list_files(buffer, sizeof(buffer));
    
    if (result > 0) {
        shell_print_info("Files in ramdisk:\n");
        term_print(buffer);
    } else if (result == 0) {
        shell_print_warning("No files in ramdisk.\n");
    } else {
        shell_print_error("Error listing files.\n");
    }
    return 0;
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        shell_print_warning("Usage: cat <filename>\n");
        return -1;
    }
    
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramdisk_read_file(argv[1], buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        uint8_t prev = shell_color_push(SHELL_COLOR_INFO);
        term_print("Contents of ");
        term_print(argv[1]);
        term_print(":\n");
        shell_color_pop(prev);
        term_print(buffer);
        term_print("\n");
    } else if (bytes_read == 0) {
        shell_print_warning("File is empty.\n");
    } else {
        uint8_t prev = shell_color_push(SHELL_COLOR_ERROR);
        term_print("Error reading file: ");
        term_print(argv[1]);
        term_print("\n");
        shell_color_pop(prev);
    }
    return 0;
}

int cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        term_print(argv[i]);
        if (i < argc - 1) {
            term_print(" ");
        }
    }
    term_print("\n");
    return 0;
}

int cmd_create(int argc, char* argv[]) {
    if (argc < 2) {
        shell_print_warning("Usage: create <filename>\n");
        return -1;
    }
    
    int result = ramdisk_create_file(argv[1], FILE_TYPE_REGULAR);
    
    if (result == 0) {
        uint8_t prev = shell_color_push(SHELL_COLOR_INFO);
        term_print("File created: ");
        term_print(argv[1]);
        term_print("\n");
        shell_color_pop(prev);
    } else if (result == -1) {
        shell_print_error("Invalid filename.\n");
    } else if (result == -2) {
        shell_print_error("No free file slots available.\n");
    } else if (result == -3) {
        shell_print_error("File already exists.\n");
    } else {
        shell_print_error("Error creating file.\n");
    }
    return result;
}

int cmd_delete(int argc, char* argv[]) {
    if (argc < 2) {
        shell_print_warning("Usage: delete <filename>\n");
        return -1;
    }
    
    int result = ramdisk_delete_file(argv[1]);
    
    if (result == 0) {
        uint8_t prev = shell_color_push(SHELL_COLOR_INFO);
        term_print("File deleted: ");
        term_print(argv[1]);
        term_print("\n");
        shell_color_pop(prev);
    } else {
        uint8_t prev = shell_color_push(SHELL_COLOR_ERROR);
        term_print("File not found: ");
        term_print(argv[1]);
        term_print("\n");
        shell_color_pop(prev);
    }
    return result;
}

int cmd_write(int argc, char* argv[]) {
    if (argc < 3) {
        shell_print_warning("Usage: write <filename> <text>\n");
        return -1;
    }
    
    // Concatenate all arguments after filename
    char text[MAX_FILE_SIZE];
    text[0] = '\0';
    
    for (int i = 2; i < argc; i++) {
        if (i > 2) {
            strcpy(text + strlen(text), " ");
        }
        strcpy(text + strlen(text), argv[i]);
    }
    
    int result = ramdisk_write_file(argv[1], text, strlen(text));
    
    if (result > 0) {
        uint8_t prev = shell_color_push(SHELL_COLOR_INFO);
        term_print("Wrote ");
        // Print number of bytes
        char num_str[12];
        int num = result;
        int pos = 0;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            while (num > 0) {
                num_str[pos++] = '0' + (num % 10);
                num /= 10;
            }
        }
        for (int i = pos - 1; i >= 0; i--) {
            term_putc(num_str[i]);
        }
        term_print(" bytes to ");
        term_print(argv[1]);
        term_print("\n");
        shell_color_pop(prev);
    } else {
        uint8_t prev = shell_color_push(SHELL_COLOR_ERROR);
        term_print("Error writing to file: ");
        term_print(argv[1]);
        term_print("\n");
        shell_color_pop(prev);
    }
    return result;
}

int cmd_read(int argc, char* argv[]) {
    if (argc < 2) {
        shell_print_warning("Usage: read <filename>\n");
        return -1;
    }
    
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramdisk_read_file(argv[1], buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        uint8_t prev = shell_color_push(SHELL_COLOR_INFO);
        term_print(buffer);
        term_print("\n");
        shell_color_pop(prev);
    } else if (bytes_read == 0) {
        shell_print_warning("File is empty.\n");
    } else {
        uint8_t prev = shell_color_push(SHELL_COLOR_ERROR);
        term_print("Error reading file: ");
        term_print(argv[1]);
        term_print("\n");
        shell_color_pop(prev);
    }
    return bytes_read;
}

int cmd_status(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    shell_print_warning("=== System Status ===\n");
    ramdisk_print_status();
    
    // Show scheduler status if available
    extern void scheduler_print_status(void);
    scheduler_print_status();
    
    return 0;
}

int cmd_ps(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    extern scheduler_t g_scheduler;
    
    // Helper to convert state to string
    static const char* state_strs[] = {
        "Ready",
        "Running",
        "Blocked",
        "Terminated"
    };
    
    shell_print_warning("PID | Name     | State    | CPU ticks\n");
    shell_print_warning("-------------------------------------\n");
    
    // Iterate through all processes in scheduler
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_scheduler.processes[i].pid != 0) {
            pcb_t* pcb = &g_scheduler.processes[i];
            
            // Print PID
            char num_str[12];
            int num = pcb->pid;
            int pos = 0;
            if (num == 0) {
                num_str[pos++] = '0';
            } else {
                while (num > 0) {
                    num_str[pos++] = '0' + (num % 10);
                    num /= 10;
                }
            }
            for (int j = pos - 1; j >= 0; j--) {
                term_putc(num_str[j]);
            }
            
            term_print(" | ");
            
            // Print name (pad to 9 chars)
            term_print(pcb->name);
            int name_len = strlen(pcb->name);
            for (int j = name_len; j < 9; j++) {
                term_putc(' ');
            }
            
            term_print(" | ");
            
            // Print state
            if (pcb->state < 4) {
                term_print(state_strs[pcb->state]);
            } else {
                term_print("Unknown");
            }
            int state_len = strlen(state_strs[pcb->state]);
            for (int j = state_len; j < 8; j++) {
                term_putc(' ');
            }
            
            term_print(" | ");
            
            // Print CPU ticks (total_runtime)
            num = pcb->total_runtime;
            pos = 0;
            if (num == 0) {
                num_str[pos++] = '0';
            } else {
                while (num > 0) {
                    num_str[pos++] = '0' + (num % 10);
                    num /= 10;
                }
            }
            for (int j = pos - 1; j >= 0; j--) {
                term_putc(num_str[j]);
            }
            
            term_print("\n");
        }
    }
    
    return 0;
}

int cmd_uptime(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    uint32_t ticks = pit_get_ticks();
    
    // PIT is at 100 Hz, so:
    // ticks / 100 = seconds
    // (ticks % 100) / 100 = centiseconds (0.00 to 0.99)
    
    uint32_t total_seconds = ticks / 100;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    uint32_t centiseconds = ticks % 100;
    
    uint8_t prev = shell_color_push(SHELL_COLOR_INFO);
    term_print("Uptime: ");
    
    // Print minutes (MM)
    if (minutes < 10) term_putc('0');
    char num_str[12];
    int num = minutes;
    int pos = 0;
    if (num == 0) {
        num_str[pos++] = '0';
    } else {
        while (num > 0) {
            num_str[pos++] = '0' + (num % 10);
            num /= 10;
        }
    }
    for (int j = pos - 1; j >= 0; j--) {
        term_putc(num_str[j]);
    }
    
    term_putc(':');
    
    // Print seconds (SS)
    if (seconds < 10) term_putc('0');
    num = seconds;
    pos = 0;
    if (num == 0) {
        num_str[pos++] = '0';
    } else {
        while (num > 0) {
            num_str[pos++] = '0' + (num % 10);
            num /= 10;
        }
    }
    for (int j = pos - 1; j >= 0; j--) {
        term_putc(num_str[j]);
    }
    
    term_putc('.');
    
    // Print centiseconds (.mm)
    if (centiseconds < 10) term_putc('0');
    num = centiseconds;
    pos = 0;
    if (num == 0) {
        num_str[pos++] = '0';
    } else {
        while (num > 0) {
            num_str[pos++] = '0' + (num % 10);
            num /= 10;
        }
    }
    for (int j = pos - 1; j >= 0; j--) {
        term_putc(num_str[j]);
    }
    
    term_print("\n");
    shell_color_pop(prev);
    
    return 0;
}

int cmd_setcolor(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        shell_print_warning("Usage: setcolor <foreground> [background]\n");
        shell_print_color_options();
        return -1;
    }
    
    uint8_t fg;
    if (!shell_lookup_color(argv[1], &fg)) {
        shell_print_error("Unknown foreground color.\n");
        shell_print_color_options();
        return -1;
    }
    
    uint8_t bg = (term_get_default_color() >> 4) & 0x0F;
    if (argc == 3) {
        if (!shell_lookup_color(argv[2], &bg)) {
            shell_print_error("Unknown background color.\n");
            shell_print_color_options();
            return -1;
        }
    }
    
    uint8_t new_color = (bg << 4) | (fg & 0x0F);
    uint8_t old_color = term_get_default_color();
    term_set_default_color(new_color);
    
    // Immediately update all visible characters on screen with new colors
    // Update background for all cells, and foreground if it was using old default
    uint8_t old_fg = old_color & 0x0F;
    uint8_t old_bg = (old_color >> 4) & 0x0F;
    uint8_t new_fg = new_color & 0x0F;
    uint8_t new_bg = (new_color >> 4) & 0x0F;
    
    for (int r = 0; r < VGA_ROWS; r++) {
        for (int c = 0; c < VGA_COLS; c++) {
            int index = r * VGA_COLS + c;
            unsigned short cell = vga_buffer[index];
            unsigned char ch = cell & 0xFF;
            unsigned char cell_color = (cell >> 8) & 0xFF;
            unsigned char cell_fg = cell_color & 0x0F;
            unsigned char cell_bg = (cell_color >> 4) & 0x0F;
            
            // Update background to new background
            cell_bg = new_bg;
            
            // If foreground was using old default, update to new default
            if (cell_fg == old_fg) {
                cell_fg = new_fg;
            }
            
            // Rewrite cell with updated colors
            unsigned char new_cell_color = (cell_bg << 4) | (cell_fg & 0x0F);
            vga_buffer[index] = ((unsigned short)new_cell_color << 8) | ch;
        }
    }
    
    // Save current position
    int saved_row = term_get_row();
    int saved_col = term_get_col();
    
    // Print success message
    shell_print_info("Default console color updated.\n");
    
    // Restore cursor position (message may have moved it)
    term_set_row(saved_row + 1); // Message printed on next line
    term_set_col(0);
    term_reset_color();
    
    return 0;
}
