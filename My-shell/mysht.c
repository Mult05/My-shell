#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <fnmatch.h> 
#define BUFSIZE 1024
#define MAX_TOKENS 128
#define MAX_ARGS 64

int last_status = 0;
bool interactive = false;

//declaring the functions at top so we can move them as be 
void run_shell(int fd);
char **tokenize(char *line, int *num_tokens);
int execute_command(char **tokens, int num_tokens);
bool builtIn(char *cmd);
int runBuiltIn(char **args);
char *resolvePath(char *cmd);
char **expandWildcards(char *token, int *expanded_count);


//Main method to decide whether batch or interactive, and set the flag
int main(int argc, char *argv[])
{
    //using as a flag for later
    interactive = isatty(STDIN_FILENO);

    int fd = STDIN_FILENO;
    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s [batchfile]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (argc == 2)
    {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0)
        {
            perror("Error opening batch file");
            exit(EXIT_FAILURE);
        }
    }

    if (interactive)
        printf("Welcome to my shell!\n");

    run_shell(fd);

    if (interactive)
        printf("Exiting my shell.\n");

    return EXIT_SUCCESS;
}

//Running the shell - kanav look at this when you can
void run_shell(int fd)
{
    char buffer[BUFSIZE];
    int buf_pos = 0;
    char ch;

    while (1)
    {
        if (interactive)
        {
            write(STDOUT_FILENO, "mysh> ", 6);
            fflush(stdout);
        }

        int n;
        while ((n = read(fd, &ch, 1)) > 0)
        {
            if (ch == '\n')
            {
                buffer[buf_pos] = '\0';
                buf_pos = 0;
                int num_tokens;
                char **tokens = tokenize(buffer, &num_tokens);
                if (tokens != NULL && num_tokens > 0)
                {
                    last_status = execute_command(tokens, num_tokens);
                    for (int i = 0; i < num_tokens; i++)
                        free(tokens[i]);
                    free(tokens);
                }
                break;
            }
            else
            {
                if (buf_pos < BUFSIZE - 1)
                    buffer[buf_pos++] = ch;
            }
        }

        if (n <= 0)
            break;
    }
}

//doing resolvepath here since this figures out if program located here -- i moved it here
char *resolvePath(char *cmd)
{
    if (strchr(cmd, '/'))
        return strdup(cmd);
    const char *dirs[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};
    for (int i = 0; dirs[i]; i++)
    {
        char path[BUFSIZE];
        snprintf(path, sizeof(path), "%s/%s", dirs[i], cmd);
        if (access(path, X_OK) == 0)
            return strdup(path);
    }
    return NULL;
}

//handles each token and parsing, puts it in a array of strings
char **tokenize(char *line, int *num_tokens)
{
    //array of strings of tokens
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    //count for each token
    *num_tokens = 0;
    char *token = strtok(line, " \t\r\n");
    //parsing each token and figuring out what type it is
    while (token != NULL)
    {
        //if # ignore
        if (token[0] == '#')
            break;
        //if wildcard, we want to expand it, so call wildcard function
        if (strchr(token, '*'))
        {
            int expanded = 0;
            char **expanded_tokens = expandWildcards(token, &expanded);
            for (int i = 0; i < expanded; i++)
            {
                tokens[(*num_tokens)++] = strdup(expanded_tokens[i]);
            }
            free(expanded_tokens);
        }
        else
        {
            tokens[(*num_tokens)++] = strdup(token);
        }
        token = strtok(NULL, " \t\r\n");
    }
    tokens[*num_tokens] = NULL;
    return tokens;
}

char **expandWildcards(char *pattern, int *expanded_count)
{
    char **results = malloc(MAX_TOKENS * sizeof(char *));
    *expanded_count = 0;

    // Separate directory and wildcard pattern
    char *slash = strrchr(pattern, '/');
    char dir[BUFSIZE] = ".";
    char *wildcard = pattern;

    if (slash)
    {
        // Extract directory and wildcard
        strncpy(dir, pattern, slash - pattern);
        dir[slash - pattern] = '\0';
        wildcard = slash + 1;
    }

    DIR *dp = opendir(dir);
    if (!dp)
    {
        perror("opendir");
        return results;
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL)
    {
        // Skip hidden files unless we are directly told so
        if (entry->d_name[0] == '.' && wildcard[0] != '.')
            continue;

        // Match the wildcard pattern
        if (fnmatch(wildcard, entry->d_name, 0) == 0)
        {
            char full_path[BUFSIZE];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);
            results[*expanded_count] = strdup(full_path);
            (*expanded_count)++;
        }
    }

    closedir(dp);

    // If no matches, return the literal pattern, as told in the assignment description
    if (*expanded_count == 0)
    {
        results[(*expanded_count)++] = strdup(pattern);
    }

    return results;
}

//running the command got from token
int execute_command(char **tokens, int num_tokens)
{
    //all of these we are checking what type it is and dependent on what is we do a specific type
    
    //and conditional
    if (strcmp(tokens[0], "and") == 0)
    {
        if (last_status != 0)
            return last_status;
        tokens++;
        num_tokens--;
    }
    //or condtional
    else if (strcmp(tokens[0], "or") == 0)
    {
        if (last_status == 0)
            return last_status;
        tokens++;
        num_tokens--;
    }

    // pipe token check
    int pipe_index = -1;
    for (int i = 0; i < num_tokens; ++i)
    {
        if (strcmp(tokens[i], "|") == 0)
        {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index != -1)
    {
        tokens[pipe_index] = NULL; // Split into two command arrays
        char **left_cmd = tokens;
        char **right_cmd = tokens + pipe_index + 1;

        int pipefd[2];
        if (pipe(pipefd) < 0)
        {
            perror("pipe");
            return 1;
        }
        //create a child process to handle running this
        pid_t pid1 = fork();
        if (pid1 == 0)
        {
            // Left child (writes to pipe)
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            if (builtIn(left_cmd[0]))
            {
                runBuiltIn(left_cmd);
                exit(0);
            }
            //use resolve path in order to get the path and resolve it
            char *path = resolvePath(left_cmd[0]);
            if (!path)
            {
                fprintf(stderr, "%s: command not found\n", left_cmd[0]);
                exit(1);
            }
            execv(path, left_cmd);
            perror("execv (left)");
            exit(1);
        }

        //second fork
        pid_t pid2 = fork();
        if (pid2 == 0)
        {
            // Right child (reads from pipe)
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]);
            close(pipefd[0]);

            if (builtIn(right_cmd[0]))
            {
                runBuiltIn(right_cmd);
                exit(0);
            }

            char *path = resolvePath(right_cmd[0]);
            if (!path)
            {
                fprintf(stderr, "%s: command not found\n", right_cmd[0]);
                exit(1);
            }
            execv(path, right_cmd);
            perror("execv (right)");
            exit(1);
        }

        // Parent process
        close(pipefd[0]);
        close(pipefd[1]);

        int status;
        waitpid(pid1, NULL, 0);
        waitpid(pid2, &status, 0);
        // Check if right side of pipe was "exit", this handles piping with exit to make sure it exits
    if (right_cmd[0] && strcmp(right_cmd[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }

    // No pipe — regular command with redirection
    int in_redir = -1, out_redir = -1;
    char *infile = NULL, *outfile = NULL;
    char *args[MAX_ARGS];
    int argc = 0;

    for (int i = 0; i < num_tokens; ++i)
    {
        if (strcmp(tokens[i], "<") == 0 && i + 1 < num_tokens)
        {
            infile = tokens[++i];
        }
        else if (strcmp(tokens[i], ">") == 0 && i + 1 < num_tokens)
        {
            outfile = tokens[++i];
        }
        else if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0)
        {
            fprintf(stderr, "Syntax error: redirection operator with no file\n");
            return 1;
        }
        else
        {
            args[argc++] = tokens[i];
        }
    }

    args[argc] = NULL;

    if (builtIn(args[0]))
    {
        int saved_stdout = dup(STDOUT_FILENO);
        int fd_out = -1;
        if (outfile)
        {
            fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out < 0)
            {
                perror("open output");
                return 1;
            }
            dup2(fd_out, STDOUT_FILENO);
        }
        int status = runBuiltIn(args);
        if (fd_out != -1)
        {
            dup2(saved_stdout, STDOUT_FILENO);
            close(fd_out);
        }
        return status;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        if (infile)
        {
            int fd_in = open(infile, O_RDONLY);
            if (fd_in < 0)
            {
                perror("input redirection");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (outfile)
        {
            int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out < 0)
            {
                perror("output redirection");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        char *path = resolvePath(args[0]);
        if (!path)
        {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(1);
        }
        execv(path, args);
        perror("execv");
        exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    else
    {
        perror("fork");
        return 1;
    }
}

//handling the built-in commands
bool builtIn(char *cmd)
{
    return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 || strcmp(cmd, "which") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "die") == 0;
}

//running each of the built in commands, with what each is supposed to do
int runBuiltIn(char **args)
{
    if (strcmp(args[0], "cd") == 0)
    {
        if (!args[1])
        {
            fprintf(stderr, "cd: missing argument\n");
            return 1;
        }
        if (chdir(args[1]) != 0)
        {
            perror("cd");
            return 1;
        }
        return 0;
    }
    else if (strcmp(args[0], "pwd") == 0)
    {
        char cwd[BUFSIZE];
        if (getcwd(cwd, sizeof(cwd)))
        {
            printf("%s\n", cwd);
            return 0;
        }
        else
        {
            perror("pwd");
            return 1;
        }
    }
    else if (strcmp(args[0], "which") == 0)
    {
        if (!args[1])
            return 1;
        if (builtIn(args[1]))
            return 1;
        char *path = resolvePath(args[1]);
        if (path)
        {
            printf("%s\n", path);
            free(path);
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else if (strcmp(args[0], "exit") == 0)
    {
        printf("mysh: Exiting...\n");
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(args[0], "die") == 0)
    {
        for (int i = 1; args[i]; ++i)
            printf("%s ", args[i]);
        printf("\n");
        exit(EXIT_FAILURE);
    }
    return 1;
}