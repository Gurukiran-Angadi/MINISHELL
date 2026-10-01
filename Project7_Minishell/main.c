/* Name        : Gurukiran Angadi
   Date        : 02/08/2026
   Description : MiniShell is a simple Linux command-line interpreter developed in C.
                 It executes built-in and external commands using fork() and execvp(),
                 supports piping between commands, signal handling (Ctrl+C, Ctrl+Z),
                 custom shell prompts, and basic background process management.
*/
#include "minishell.h"

char *prompt;// Global variable to store the shell prompt
char input_string[1024];// Buffer to store user input

int main()
{
    prompt = strdup("Minishell");// Allocate memory and initialize the default shell prompt

    scan_input(&prompt, input_string); // Start the shell by reading and executing user commands
    free(prompt);
}