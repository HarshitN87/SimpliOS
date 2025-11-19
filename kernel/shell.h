#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

// Shell constants
#define MAX_COMMAND_LEN 128
#define MAX_ARGS 8
#define MAX_ARG_LEN 32
#define MAX_HISTORY 32

// Command structure
typedef struct {
    char name[16];
    char description[64];
    int (*handler)(int argc, char* argv[]);
} shell_command_t;

// Shell state
typedef struct {
    char current_line[MAX_COMMAND_LEN];
    uint32_t line_pos;
    uint32_t cursor_pos;  // Cursor position in current line
    uint32_t history_pos;  // Current position in history (0 = no history, 1 = most recent)
    uint32_t history_count;  // Number of commands in history
    char history[MAX_HISTORY][MAX_COMMAND_LEN];  // Command history
    uint8_t echo_enabled;
} shell_state_t;

// Global shell state
extern shell_state_t g_shell;

// Function declarations
void shell_init(void);
void shell_process_char(char c);
void shell_process_arrow_key(uint8_t arrow_type);  // 0=Up, 1=Down, 2=Left, 3=Right
void shell_execute_command(const char* line);
void shell_prompt(void);
void shell_clear_line(void);

// Input redirection
typedef void (*input_handler_t)(char c, uint8_t arrow);
void shell_set_input_handler(input_handler_t handler);

// Command handlers
int cmd_help(int argc, char* argv[]);
int cmd_clear(int argc, char* argv[]);
int cmd_ls(int argc, char* argv[]);
int cmd_cat(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_create(int argc, char* argv[]);
int cmd_delete(int argc, char* argv[]);
int cmd_write(int argc, char* argv[]);
int cmd_read(int argc, char* argv[]);
int cmd_status(int argc, char* argv[]);
int cmd_ps(int argc, char* argv[]);
int cmd_uptime(int argc, char* argv[]);
int cmd_setcolor(int argc, char* argv[]);
int cmd_calc(int argc, char* argv[]);

// Utility functions
void shell_print_prompt(void);
int shell_parse_args(const char* line, char* argv[], int max_args);

#endif // SHELL_H
