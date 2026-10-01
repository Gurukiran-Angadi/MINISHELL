#include "minishell.h"

//Global flags and counters
int flag = 0;
int count_node = 0;
int echo_flag = 1;
int pid;

//doubly linked list for job control
pid_node *head = NULL;
pid_node *tail = NULL;

//Insert a new job node at the beginnog of the list
void insert_first(pid_node **head, pid_node **tail, char *command, int pid, int status)
{
    if (count_node == 10)//Limit job list to 10
    {
        printf("Error: Job list is full.\n");
        return;
    }

    //Assign memory through malloc
    pid_node *new_node = (pid_node *)malloc(sizeof(pid_node));
    if (new_node == NULL)
    {
        perror("malloc");
        return;
    }

    //Initialize node fields
    new_node->pid = pid;
    new_node->status = status;

    strncpy(new_node->command, command, sizeof(new_node->command) - 1);
    new_node->command[sizeof(new_node->command) - 1] = '\0';

    new_node->next = NULL;//update next with null
    new_node->prev = NULL;//update prev with null

    //If list empty, set head/tail
    if (*head == NULL)
    {
        *head = new_node;
        *tail = new_node;
    }
    else
    {
        /* Insert at beginning */
        new_node->next = *head;
        (*head)->prev = new_node;
        *head = new_node;
    }

    count_node++;
}

/* Remove a job (by pid) once it has been resumed with fg/bg,
 * so the "list is full" counter reflects reality. */
void remove_job(int target_pid)
{
    //Start from the first node in the job list
    pid_node *curr = head;

    //Traverse the linked list
    while (curr != NULL)
    {
        //Check if the current node matches the target PID
        if (curr->pid == target_pid)
        {
            //Update previous node's next pointer
            if (curr->prev)
                curr->prev->next = curr->next;
            else
                head = curr->next;//Update head if removing the first node

            if (curr->next) //Update next node's previous pointer
                curr->next->prev = curr->prev;
            else
                tail = curr->prev;//Update tail if removing the last node

            free(curr);
            count_node--;
            return;
        }
        curr = curr->next;//Move to the next node
    }
}

//Handle fg, bg, jobs, kil commands
void echo(char *input_string, int status)
{
    //Check if the command is "fg"
    if (strcmp(input_string, "fg") == 0)
    {
        //Check if there are any jobs
        if (head == NULL)
        {
            printf("-bash: fg: current: no such job\n");
            return;
        }

        //Start searching from the first job
        pid_node *job = head;

        //Find the first stopped job
        while (job != NULL && job->status != STOPPED)
        {
            job = job->next;
        }

        //If no stopped job is found
        if (job == NULL)
        {
            printf("fg: no stopped job\n");
            return;
        }

        printf("[%d] %d %s &\n",count_node,head->pid,head->command);//Display the foreground job details

        //Resume the stopped process
        if (kill(job->pid, SIGCONT) == -1)
        {
            perror("kill");
            return;
        }

        job->status = RUNNING; //Mark the job as running

        int child_status;

        waitpid(job->pid, &child_status, WUNTRACED); // Wait until the process exits or stops again

        // Check if the process was stopped
        if (WIFSTOPPED(child_status))
        {
            job->status = STOPPED;// Update the job status to stopped
        }
        else
        {
            remove_job(job->pid);// Remove the completed job from the list
        }
    }

    // Check if the command is "bg"
    else if (strcmp(input_string, "bg") == 0)
    {
        // Check if there are any jobs
        if (head == NULL)
        {
            printf("-bash: bg: current: no such job\n");
            return;
        }

        // Resume the background process
        if (kill(head->pid, SIGCONT) == -1)
        {
            perror("kill");
            return;
        }

        head->status = RUNNING;// Mark the job as running

        printf("[%d] %d %s &\n",count_node,head->pid,head->command); // Display the resumed background job
    }

    // Check if the command is "jobs"
    else if (strcmp(input_string, "jobs") == 0)
    {
        // Check if there are any jobs
        if (head == NULL)
        {
            printf("No jobs\n");
            return;
        }

        pid_node *temp = tail;// Start displaying jobs from the last node
        int job = 1;

        while (temp != NULL)// Traverse the job list
        {
            printf("[%d]   Pid -> %d    %s    %s\n", job, temp->pid, (temp->status == RUNNING) ? "Running" : "Stopped",
                   temp->command);

            temp = temp->prev;// Move to the previous job
            job++; // Increment the job number
        }
    }

    // Check if the command starts with "kill "
    else if (strncmp(input_string, "kill ", 5) == 0)
    {
        int pid = atoi(input_string + 5);// Extract the PID from the command

         // Send SIGKILL to the specified process
        if (kill(pid, SIGKILL) == -1)
        {
            perror("kill");
        }
        else
        {
            printf("Killed process %d\n", pid);
            remove_job(pid); // Remove the job from the job list
        }
    }
}

//Signal handler for clt + c and clt + z
void signal_handler(int sig_num)
{
    // Check if Ctrl + C (SIGINT) is received
    if (sig_num == SIGINT)
    {
        // Display the shell prompt if no foreground process is running
        if (flag == 1)
        {
            char *path = getcwd(NULL, 0);// Get the current working directory
            printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET ":" ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "$ ", prompt, path);
            free(path);
        }
    }

    // Check if Ctrl + Z (SIGTSTP) is received
    if (sig_num == SIGTSTP)
    {
        // Add the stopped process to the job list only once
        if (echo_flag == 0)
        {
            insert_first(&head, &tail, input_string, pid, STOPPED);
            echo_flag = 1;
        }

        // Display the shell prompt after stopping the process
        if (flag == 1)
        {
            char *path = getcwd(NULL, 0);// Get the current working directory
            printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET ":" ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "$ ", prompt, path);
            free(path);
        }
    }
}

//Change shell prompt (PS1)
void copy_change(char **prompt, char *input_string)
{
    // Find the '=' character in the input
    char *str = strchr(input_string, '=');
    if (!str)
        return;
    str++;

    // Allocate memory for the new prompt
    char *new_prompt = (char *)malloc(strlen(str) + 1);
    if (!new_prompt)
    {
        perror("malloc\n");
        return;
    }

    strcpy(new_prompt, str);// Copy the new prompt string
    free(*prompt);

    *prompt = new_prompt;// Update the prompt pointer
}

//Main input loop
void scan_input(char **prompt, char *input_string)
{
    system("clear");
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);
    while (1)
    {
        /*when  you pass NULL as the buffer and 0 as the size, glibc's getcwd() will automatically malloc() a
        buffer just large enough to hold the path.*/
        char *path = getcwd(NULL, 0);

        // print prompt
        printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET ":" ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET "$ ", *prompt, path);
        fflush(stdout);
        // read input_string

        if (fgets(input_string, 1023, stdin) == NULL)
        {
            free(path);
            clearerr(stdin);
            continue;
        }

        input_string[strcspn(input_string, "\n")] = '\0';

        if (input_string[0] == '\0')
        {
            free(path);
            continue;
        }

        // if input_string new "ps1="
        if (strncmp(input_string, "PS1=", 4) == 0)
        {

            copy_change(prompt, input_string);
            free(path);
            continue;
        }

        if (strcmp(input_string, "fg") == 0 || strcmp(input_string, "bg") == 0 ||
            strcmp(input_string, "jobs") == 0 || strncmp(input_string, "kill ", 5) == 0)
        {
            echo(input_string, echo_flag);
            free(path);
            continue;
        }

        // by this we can get first of the input.
        char *cmd = get_command(input_string);
        // printf("%s\n", cmd);

        // it gives which command is that..
        int ret = check_command_type(cmd);

        if (ret == BUILTIN)
        {
            // builtin execution
            execute_internal_commands(input_string);
        }
        else if (ret == EXTERNAL)
        {
            // external execution
            extract_external_commands(input_string);
        }
        else
        {
            printf("%s: command not found\n", input_string);
        }

        free(path);
    }
}

//Extract first word of command
char *get_command(char *input_string)
{
    // logic
    // why this means the orignal string will not change the strcpy will do the deepcopy..
    static char temp[100];

    strcpy(temp, input_string);
    return strtok(temp, " ");
}

//check if command external or built-in
int check_command_type(char *command)
{
    // builtin commands
    char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval",
                        "set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source",
                        "exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help", NULL};

    // external commands
    char *externals[] = {
        "bash", "bunzip2", "busybox", "bzcat", "bzcmp", "bzdiff", "bzegrep",
        "bzexe", "bzfgrep", "bzgrep", "bzip2", "bzip2recover", "bzless", "bzmore",
        "cat", "chacl", "chgrp", "chmod", "chown", "chvt", "cp", "cpio", "dash",
        "date", "dbus-cleanup-sockets", "dbus-daemon", "dbus-uuidgen", "dd", "df",
        "dir", "dmesg", "dnsdomainname", "domainname", "dumpkeys", "echo", "ed",
        "egrep", "false", "fgconsole", "fgrep", "findmnt", "fuser", "fusermount",
        "getfacl", "grep", "gunzip", "gzexe", "gzip", "hostname", "ip", "kbd_mode",
        "kill", "kmod", "less", "lessecho", "lessfile", "lesskey", "lesspipe", "ln",
        "loadkeys", "login", "loginctl", "lowntfs-3g", "ls", "lsblk", "lsmod",
        "mkdir", "mknod", "mktemp", "more", "mount", "mountpoint", "mt", "mt-gnu",
        "mv", "nano", "nc", "nc.openbsd", "netcat", "netstat", "nisdomainname",
        "ntfs-3g", "ntfs-3g.probe", "ntfs-3g.secaudit", "ntfs-3g.usermap", "ntfscat",
        "ntfsck", "ntfscluster", "ntfscmp", "ntfsdump_logfile", "ntfsfix", "ntfsinfo",
        "ntfsls", "ntfsmftalloc", "ntfsmove", "ntfstruncate", "ntfswipe", "open",
        "openvt", "pidof", "ping", "ping6", "plymouth", "plymouth-upstart-bridge",
        "ps", "pwd", "rbash", "readlink", "red", "rm", "rmdir", "rnano",
        "running-in-container", "run-parts", "sed", "setfacl", "setfont", "setupcon",
        "sh", "sh.distrib", "sleep", "ss", "static-sh", "stty", "su", "sync",
        "tailf", "tar", "tempfile", "touch", "true", "udevadm", "ulockmgr_server",
        "umount", "uname", "uncompress", "unicode_start", "vdir", "vmmouse_detect",
        "which", "whiptail", "ypdomainname", "zcat", "zcmp", "zdiff", "zegrep",
        "zfgrep", "zforce", "zgrep", "zless", "zmore", "znew", "wc", NULL};

    //comparison for built-in commnds
    for (int i = 0; builtins[i] != NULL; i++)
    {
        if (strcmp(command, builtins[i]) == 0)
        {
            return 1;
        }
    }

    //comparison for externals commnds
    for (int i = 0; externals[i] != NULL; i++)
    {
        if (strcmp(command, externals[i]) == 0)
        {
            return 2;
        }
    }

    return 0;
}

//Parse and execute external commands with pipes
void extract_external_commands(char *external_commands)
{
    char *commands[20][20];
    char *cmd;
    int cmd_count = 0;

    char *outer_save, *inner_save;

    /*why the strtok insted of the strtok
    for example the input is "ls -l | wc -l"
    strtok -> ls -l in here the remaning it will lose on strtok_r(cmd, " ", &inner_save); this line in loop.
    strok_r -> ls -l it will save the | + 1 address in pointer..*/
    cmd = strtok_r(external_commands, "|", &outer_save);

    // Process each command separated by '|'
    while (cmd != NULL)
    {
        while (*cmd == ' ')// Skip leading spaces in the command
        {
            cmd++;
        }

        int i = 0;
        inner_save = NULL;// Reset the inner tokenizer pointer
        commands[cmd_count][i] = strtok_r(cmd, " ", &inner_save);// Get the first argument of the command

        // Tokenize all arguments of the command
        while (commands[cmd_count][i] != NULL)
        {
            i++;
            commands[cmd_count][i] = strtok_r(NULL, " ", &inner_save);
        }
        commands[cmd_count][i] = NULL;// Mark the end of the argument list
        cmd_count++;
        // it will return the address of the null+1..
        cmd = strtok_r(NULL, "|", &outer_save);
    }
    execute_pipeline(commands, cmd_count);//function call
}

// Execute multiple commands connected by pipes
void execute_pipeline(char *commands[20][20], int cmd_count)
{
    int fd[2];
    int saved_stdin = dup(STDIN_FILENO);// Save the original standard input
    int input_fd = STDIN_FILENO;// Store the current input file descriptor

    pid_t pids[20];

    int background = 0;

    /* Check if last argument is '&' */
    int j = 0;

    // Find the last argument of the final command
    while (commands[cmd_count - 1][j] != NULL)
    {
        j++;
    }

    // Check if the command should run in the background
    if (j > 0 && strcmp(commands[cmd_count - 1][j - 1], "&") == 0)
    {
        background = 1;
        commands[cmd_count - 1][j - 1] = NULL;// Remove '&' before execution
    }

    // Execute each command in the pipeline
    for (int i = 0; i < cmd_count; i++)
    {
        // Create a pipe except for the last command
        if (i != cmd_count - 1)
        {
            if (pipe(fd) == -1)
            {
                perror("pipe");
                return;
            }
        }

        pids[i] = fork();// Create a child process

        if (pids[i] == -1)
        {
            perror("fork");
            return;
        }

        if (pids[i] > 0)// Parent process
        {
            pid = pids[i];// Store the child PID
            flag = 0;
            echo_flag = 0;

            // Prepare input for the next command
            if (i != cmd_count - 1)
            {
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
            }
        }
        if (pids[i] == 0)// Child process
        {
            flag = 1;
            // Restore default signal handling
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            // Redirect output to the next command
            if (i != cmd_count - 1)
            {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }

            execvp(commands[i][0], commands[i]);// Execute the command

            perror("execvp");
            exit(1);
        }
    }
    // Restore the original standard input
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdin);

    // Wait for all child processes if running in foreground
    if (!background)
    {
        int status;

        for (int i = 0; i < cmd_count; i++)
        {
            waitpid(pids[i], &status, WUNTRACED);
        }
    }
    else
    {
        printf("[Background] PID = %d\n", pids[cmd_count - 1]); // Display background process information
        // Add the background job to the job list
        insert_first(&head, &tail, commands[cmd_count - 1][0], pids[cmd_count - 1], RUNNING);
    }
}

// Execute built-in shell commands
void execute_internal_commands(char *input_string)
{
    char *argv[1024];// Array to store command arguments
    int i = 0;

    // Split the input into words
    argv[i] = strtok(input_string, " ");

    while (argv[i] != NULL)
    {
        i++;
        argv[i] = strtok(NULL, " ");
    }

    // Handle the echo command
    if (strcmp(argv[0], "echo") == 0)
    {
        if (argv[1] == NULL)// Print a blank line if no arguments are given
        {
            printf("\n");
        }
        else if (strcmp(argv[1], "$$") == 0)// Print the current process ID
        {
            printf("%d\n", getpid());
        }
        else if (strcmp(argv[1], "$SHELL") == 0)// Print the SHELL environment variable
        {
            char *shell = getenv("SHELL");

            if (shell != NULL)
            {
                printf("%s\n", shell);
            }
        }
        else if (argv[1][0] == '$')// Handle environment variables beginning with '$'
        {
            /* Only '$' */
            if (argv[1][1] == '\0')
            {
                printf("$\n");
            }
            else
            {
                char *env = getenv(argv[1] + 1);// Get the environment variable value

                if (env != NULL)
                {
                    printf("%s\n", env);
                }
                else
                {
                    /* Variable not found */
                    printf("%s\n", argv[1]);
                }
            }
        }
        else
        {
            // Print all remaining arguments
            for (int j = 1; argv[j] != NULL; j++)
            {
                printf("%s ", argv[j]);
            }
            printf("\n");
        }
    }
    // Handle the exit command
    else if (strcmp(argv[0], "exit") == 0)
    {
        exit(0);
    }
    // Handle the cd command
    else if (strcmp(argv[0], "cd") == 0)
    {
        if (argv[1] == NULL)// Check if directory is provided
        {
            printf("cd: missing operand\n");
        }
        else
        {
            // Change the current directory
            if (chdir(argv[1]) == -1)
            {
                perror("cd");
            }
        }
    }
    // Handle the pwd command
    else if (strcmp(argv[0], "pwd") == 0)
    {
        // Get the current working directory
        char buff[1024];
        if (getcwd(buff, sizeof(buff)) != NULL)
        {
            printf("%s\n", buff);
        }
        else
        {
            perror("pwd\n");
        }
    }
    else// Handle unsupported built-in commands
    {
        printf("you have entered %s built-in command\n", input_string);
    }
}