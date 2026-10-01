#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio_ext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#define BUILTIN		1
#define EXTERNAL	2
#define NO_COMMAND  3

// ANSI color codes for terminal output
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

extern char *prompt;// Global shell prompt
extern char input_string[1024];// Buffer to store user input

#define RUNNING 1// Job status: Running
#define STOPPED 0// Job status: Stopped

// Structure to store job information
typedef struct pid_node
{
    int pid;
    int status;
    char command[100];
    struct pid_node *next;
    struct pid_node *prev;
} pid_node;

// Read input from the user
void scan_input(char **prompt, char *input_string);
// Extract the command from the input string
char *get_command(char *input_string);
// Change the shell prompt
void copy_change(char **prompt, char *input_string);
// Determine the command type
int check_command_type(char *command);
// Execute built-in echo, jobs, fg, bg, and kill commands
void echo(char *input_string, int status);
// Execute internal shell commands
void execute_internal_commands(char *input_string);
// Handle signals (Ctrl+C and Ctrl+Z)
void signal_handler(int sig_num);
// Read all external commands
void extract_external_commands(char *external_commands);
// Execute commands connected by pipes
void execute_pipeline(char *commands[20][20], int cmd_count);

#endif