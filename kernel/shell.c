#include "shell.h"
#include "ramdisk.h"
#include <stddef.h>

// External functions from kernel.c
extern void term_print(const char* str);
extern void term_putc(char c);
extern void term_clear(void);

// String utility functions (freestanding environment)
static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
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
    {"status", "Show system status", cmd_status}
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

// Initialize the shell
void shell_init(void) {
    g_shell.line_pos = 0;
    g_shell.history_pos = 0;
    g_shell.cursor_pos = 0;
    g_shell.echo_enabled = 1;
    g_shell.current_line[0] = '\0';
    
    term_print("SimpliOS Shell initialized.\n");
    shell_print_prompt();
}

// Print shell prompt
void shell_print_prompt(void) {
    term_print("simplios> ");
}

// Clear current line
void shell_clear_line(void) {
    g_shell.line_pos = 0;
    g_shell.current_line[0] = '\0';
    term_print("\n");
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
            shell_execute_command(g_shell.current_line);
            break;
            
        case '\b':
            if (g_shell.line_pos > 0) {
                g_shell.line_pos--;
                g_shell.current_line[g_shell.line_pos] = '\0';
                term_putc('\b');
                term_putc(' ');
                term_putc('\b');
            }
            break;
            
        case '\t':
            // Tab completion (simplified - just add spaces)
            if (g_shell.line_pos < MAX_COMMAND_LEN - 1) {
                g_shell.current_line[g_shell.line_pos++] = ' ';
                g_shell.current_line[g_shell.line_pos] = '\0';
                term_putc(' ');
            }
            break;
            
        default:
            if (g_shell.line_pos < MAX_COMMAND_LEN - 1 && c >= 32 && c <= 126) {
                g_shell.current_line[g_shell.line_pos++] = c;
                g_shell.current_line[g_shell.line_pos] = '\0';
                term_putc(c);
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
        term_print("Command not found: ");
        term_print(argv[0]);
        term_print("\nType 'help' for available commands.\n");
    }
    
    g_shell.line_pos = 0;
    g_shell.current_line[0] = '\0';
    shell_print_prompt();
}

// Command implementations

int cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv; // Suppress unused parameter warnings
    
    term_print("Available commands:\n");
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
        term_print("Files in ramdisk:\n");
        term_print(buffer);
    } else if (result == 0) {
        term_print("No files in ramdisk.\n");
    } else {
        term_print("Error listing files.\n");
    }
    return 0;
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        term_print("Usage: cat <filename>\n");
        return -1;
    }
    
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramdisk_read_file(argv[1], buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        term_print("Contents of ");
        term_print(argv[1]);
        term_print(":\n");
        term_print(buffer);
        term_print("\n");
    } else if (bytes_read == 0) {
        term_print("File is empty.\n");
    } else {
        term_print("Error reading file: ");
        term_print(argv[1]);
        term_print("\n");
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
        term_print("Usage: create <filename>\n");
        return -1;
    }
    
    int result = ramdisk_create_file(argv[1], FILE_TYPE_REGULAR);
    
    if (result == 0) {
        term_print("File created: ");
        term_print(argv[1]);
        term_print("\n");
    } else if (result == -1) {
        term_print("Invalid filename.\n");
    } else if (result == -2) {
        term_print("No free file slots available.\n");
    } else if (result == -3) {
        term_print("File already exists.\n");
    } else {
        term_print("Error creating file.\n");
    }
    return result;
}

int cmd_delete(int argc, char* argv[]) {
    if (argc < 2) {
        term_print("Usage: delete <filename>\n");
        return -1;
    }
    
    int result = ramdisk_delete_file(argv[1]);
    
    if (result == 0) {
        term_print("File deleted: ");
        term_print(argv[1]);
        term_print("\n");
    } else {
        term_print("File not found: ");
        term_print(argv[1]);
        term_print("\n");
    }
    return result;
}

int cmd_write(int argc, char* argv[]) {
    if (argc < 3) {
        term_print("Usage: write <filename> <text>\n");
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
    } else {
        term_print("Error writing to file: ");
        term_print(argv[1]);
        term_print("\n");
    }
    return result;
}

int cmd_read(int argc, char* argv[]) {
    if (argc < 2) {
        term_print("Usage: read <filename>\n");
        return -1;
    }
    
    char buffer[MAX_FILE_SIZE];
    int bytes_read = ramdisk_read_file(argv[1], buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        term_print(buffer);
        term_print("\n");
    } else if (bytes_read == 0) {
        term_print("File is empty.\n");
    } else {
        term_print("Error reading file: ");
        term_print(argv[1]);
        term_print("\n");
    }
    return bytes_read;
}

int cmd_status(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    term_print("=== System Status ===\n");
    ramdisk_print_status();
    
    // Show scheduler status if available
    extern void scheduler_print_status(void);
    scheduler_print_status();
    
    return 0;
}
