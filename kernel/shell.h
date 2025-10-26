#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

// Shell constants
#define MAX_COMMAND_LEN 128
#define MAX_ARGS 8
#define MAX_ARG_LEN 32

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
    uint32_t history_pos;
    uint8_t cursor_pos;
    uint8_t echo_enabled;
} shell_state_t;

// Global shell state
extern shell_state_t g_shell;

// Function declarations
void shell_init(void);
void shell_process_char(char c);
void shell_execute_command(const char* line);
void shell_prompt(void);
void shell_clear_line(void);

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

// Utility functions
void shell_print_prompt(void);
int shell_parse_args(const char* line, char* argv[], int max_args);

#endif // SHELL_H
