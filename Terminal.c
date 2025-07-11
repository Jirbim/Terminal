#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <ctype.h>

#define INITIAL_BUFFER_SIZE 1024
#define INITIAL_COMMANDS_SIZE 10
#define INITIAL_ARGS_SIZE 10

// Function to check input syntax
int check_syntax(char *input)
{
    int single_quotes = 0, double_quotes = 0;
    int i = 0;
    int length = strlen(input);
    while (input[i])
    {
        if (input[i] == '\'') single_quotes++;
        if (input[i] == '\"') double_quotes++;
        i++;
    }
    if (single_quotes % 2 != 0 || double_quotes % 2 != 0)
    {
        return -1;
    }

    for (i = 0; i < length; i++)
    {
        while (i < length && isspace(input[i])) i++;
        if (input[i] == '|')
        {
            if (i == 0 || i == length - 1 || input[i + 1] == '|')
            {
                return -1;
            }
        }

        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                if (i == 0 || i == length - 1 || input[i + 1] == '|')
                {
                    return -1;
                }
                i++;
            }
            else
            {
                if (i == 0 || i == length - 1 || input[i + 1] == '|' || input[i + 1] == '>' || input[i + 1] == '<')
                {
                    return -1;
                }
            }

            int j = i - 1;
            while (j >= 0 && isspace(input[j])) j--;
            if (j < 0 || input[j] == '|')
            {
                return -1;
            }

            j = i + 1;
            while (j < length && (input[j] == ' ' || input[j] == '\t')) j++;
            if (input[j] == '\0'|| input[j] == '|')
            {
                return -1;
            }
            if (input[j] == '>')
            {
                return -1;
            }
        }
        if (input[i] == '<')
        {
            if (i == 0 || i == length - 1 || input[i + 1] == '|' || input[i + 1] == '>' || input[i + 1] == '<')
            {
                return -1;
            }

            int j = i + 1;
            while (j < length && (input[j] == ' ' || input[j] == '\t')) j++;
            if (input[j] == '\0')
            {
                return -1;
            }
        }
        if (input[i] == '2' && input[i + 1] == '>')
        {
            if (i + 1 < length && isspace(input[i + 1]))
            {
                return -1;
            }
            if (i == 0 || input[i - 1] == '|' || input[i - 1] == '>' || input[i - 1] == '<')
            {
                return -1;
            }

            int j = i - 1;
            while (j >= 0 && isspace(input[j])) j--;
            if (j < 0 || input[j] == '|')
            {
                return -1;
            }

            j = i + 2;
            while (j < length && (input[j] == ' ' || input[j] == '\t')) j++;
            if (input[j] == '\0' || input[j] == '|')
            {
                return -1;
            }
            i++;
        }
    }
    char *input_copy = strdup(input);
    if (input_copy == NULL)
    {
        return -1;
    }

    char *token = strtok(input_copy, "|");
    while (token != NULL)
    {
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') end--;
        end[1] = '\0';

        if (strlen(token) == 0)
        {
            free(input_copy);
            return -1;
        }
        token = strtok(NULL, "|");
    }

    free(input_copy);
    return 0;
}

// Function for handling redirects
int handle_redirection(char **args, int *input_fd, int *output_fd, int *error_fd)
{
    int i = 0;
    while (args[i] != NULL)
    {
        if (strcmp(args[i], "<") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            *input_fd = open(args[i + 1], O_RDONLY);
            if (*input_fd < 0)
            {
                fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            args[i] = NULL;
            i += 2;
            continue;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            else if (args[i + 2] != NULL && (strcmp(args[i + 2], ">") != 0 || strcmp(args[i + 2], ">>") != 0 || strcmp(args[i + 2], "2>") != 0))
            {
                fprintf(stderr, "Syntax error\n");
                return -1;
            }
            *output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*output_fd < 0)
            {
                fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            args[i] = NULL;
            i += 2;
            continue;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            else if (args[i + 2] != NULL && (strcmp(args[i + 2], ">") != 0 || strcmp(args[i + 2], ">>") != 0 || strcmp(args[i + 2], "2>") != 0))
            {
                fprintf(stderr, "Syntax error\n");
                return -1;
            }
            *output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (*output_fd < 0) {
                 fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            args[i] = NULL;
            i += 2;
            continue;
        }
        else if (strcmp(args[i], "2>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            else if (args[i + 2] != NULL && (strcmp(args[i + 2], ">") != 0 || strcmp(args[i + 2], ">>") != 0 || strcmp(args[i + 2], "2>") != 0))
            {
                fprintf(stderr, "Syntax error\n");
                return -1;
            }
            *error_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*error_fd < 0)
            {
                 fprintf(stderr, "File %s not found\n", args[i + 1]);
                return -1;
            }
            args[i] = NULL;
            i += 2;
            continue;
        }
        else
        {
            i++;
        }
    }
    return 0;
}

// Function for splitting parameters
char **split_arguments(char *command, size_t *arg_count)
{
    size_t args_size = INITIAL_ARGS_SIZE;
    char **args = malloc(args_size * sizeof(char*));
    if (args == NULL)
    {
        free(command);
        exit(EXIT_FAILURE);
    }
    *arg_count = 0;

    char *ptr = command;
    char *start = NULL;
    int in_quotes = 0;

    while (*ptr)
    {
        if (*ptr == '\"' || *ptr == '\'')
        {
            in_quotes = !in_quotes;
            if (in_quotes)
                start = ptr + 1;
            else {
                size_t length = ptr - start;
                if (*arg_count >= args_size)
                {
                    args_size *= 2;
                    char **temp = realloc(args, args_size * sizeof(char*));
                    if (temp == NULL)
                    {
                        for (size_t i = 0; i < *arg_count; i++)
                            free(args[i]);
                        free(args);
                        free(command);
                        exit(EXIT_FAILURE);
                    }
                    args = temp;
                }
                args[(*arg_count)] = strndup(start, length);
                if (args[(*arg_count)] == NULL)
                {
                    for (size_t i = 0; i < *arg_count; i++)
                        free(args[i]);
                    free(args);
                    free(command);
                    exit(EXIT_FAILURE);
                }
                (*arg_count)++;
                start = NULL;
            }
        }
        else if (!in_quotes && isspace(*ptr))
        {
            if (start)
            {
                size_t length = ptr - start;
                if (*arg_count >= args_size)
                {
                    args_size *= 2;
                    char **temp = realloc(args, args_size * sizeof(char*));
                    if (temp == NULL) {
                        for (size_t i = 0; i < *arg_count; i++)
                            free(args[i]);
                        free(args);
                        free(command);
                        exit(EXIT_FAILURE);
                    }
                    args = temp;
                }
                args[(*arg_count)] = strndup(start, length);
                if (args[(*arg_count)] == NULL)
                {
                    for (size_t i = 0; i < *arg_count; i++)
                        free(args[i]);
                    free(args);
                    free(command);
                    exit(EXIT_FAILURE);
                }
                (*arg_count)++;
                start = NULL;
            }
        }
        else if (!in_quotes && (*ptr == '>' || *ptr == '<' || *ptr == '|' || (*ptr == '2' && *(ptr + 1) == '>')))
        {
            if (start)
            {
                size_t length = ptr - start;
                if (*arg_count >= args_size)
                {
                    args_size *= 2;
                    char **temp = realloc(args, args_size * sizeof(char*));
                    if (temp == NULL)
                    {
                        for (size_t i = 0; i < *arg_count; i++)
                            free(args[i]);
                        free(args);
                        free(command);
                        exit(EXIT_FAILURE);
                    }
                    args = temp;
                }
                args[(*arg_count)] = strndup(start, length);
                if (args[(*arg_count)] == NULL)
                {
                    for (size_t i = 0; i < *arg_count; i++)
                        free(args[i]);
                    free(args);
                    free(command);
                    exit(EXIT_FAILURE);
                }
                (*arg_count)++;
                start = NULL;
            }
            if (*ptr == '>' && *(ptr + 1) == '>')
            {
                size_t length = 2;
                if (*arg_count >= args_size)
                {
                    args_size *= 2;
                    char **temp = realloc(args, args_size * sizeof(char*));
                    if (temp == NULL)
                    {
                        for (size_t i = 0; i < *arg_count; i++)
                            free(args[i]);
                        free(args);
                        free(command);
                        exit(EXIT_FAILURE);
                    }
                    args = temp;
                }
                args[(*arg_count)] = strndup(ptr, length);
                if (args[(*arg_count)] == NULL)
                {
                    for (size_t i = 0; i < *arg_count; i++)
                        free(args[i]);
                    free(args);
                    free(command);
                    exit(EXIT_FAILURE);
                }
                (*arg_count)++;
                ptr++;
            }
            else if (*ptr == '2' && *(ptr + 1) == '>')
            {
                size_t length = 2;
                if (*arg_count >= args_size)
                {
                    args_size *= 2;
                    char **temp = realloc(args, args_size * sizeof(char*));
                    if (temp == NULL)
                    {
                        for (size_t i = 0; i < *arg_count; i++)
                            free(args[i]);
                        free(args);
                        free(command);
                        exit(EXIT_FAILURE);
                    }
                    args = temp;
                }
                args[(*arg_count)] = strndup(ptr, length);
                if (args[(*arg_count)] == NULL)
                {
                    for (size_t i = 0; i < *arg_count; i++)
                        free(args[i]);
                    free(args);
                    free(command);
                    exit(EXIT_FAILURE);
                }
                (*arg_count)++;
                ptr++;
            }
            else
            {
                size_t length = 1;
                if (*arg_count >= args_size)
                {
                    args_size *= 2;
                    char **temp = realloc(args, args_size * sizeof(char*));
                    if (temp == NULL)
                    {
                        for (size_t i = 0; i < *arg_count; i++)
                            free(args[i]);
                        free(args);
                        free(command);
                        exit(EXIT_FAILURE);
                    }
                    args = temp;
                }
                args[(*arg_count)] = strndup(ptr, length);
                if (args[(*arg_count)] == NULL)
                {
                    for (size_t i = 0; i < *arg_count; i++)
                        free(args[i]);
                    free(args);
                    free(command);
                    exit(EXIT_FAILURE);
                }
                (*arg_count)++;
            }
        }
        else if (!start)
            start = ptr;

        ptr++;
    }

    if (start)
    {
        size_t length = ptr - start;
        if (*arg_count >= args_size)
        {
            args_size *= 2;
            char **temp = realloc(args, args_size * sizeof(char*));
            if (temp == NULL)
            {
                for (size_t i = 0; i < *arg_count; i++)
                    free(args[i]);
                free(args);
                free(command);
                exit(EXIT_FAILURE);
            }
            args = temp;
        }
        args[(*arg_count)] = strndup(start, length);
        if (args[(*arg_count)] == NULL)
        {
            for (size_t i = 0; i < *arg_count; i++)
                free(args[i]);
            free(args);
            free(command);
            exit(EXIT_FAILURE);
        }
        (*arg_count)++;
    }

    args[*arg_count] = NULL;
    return args;
}



// Function for executing commands
int execute_command(char *command, int input_fd, int output_fd, int error_fd)
{
    size_t arg_count;
    char **args = split_arguments(command, &arg_count);
    if (args == NULL)
    {
        return -1;
    }

    int in_fd = input_fd;
    int out_fd = output_fd;
    int err_fd = error_fd;

    if (handle_redirection(args, &in_fd, &out_fd, &err_fd) == -1)
    {
        for (size_t i = 0; i < arg_count; i++)
            free(args[i]);
        free(args);
        return -1;
    }

    if (strcmp(args[0], "cd") == 0)
    {
        if (arg_count > 1)
            if (chdir(args[1]) != 0)
            {
                fprintf(stderr, "Directory %s not found\n", args[1]);
            }
        for (size_t i = 0; i < arg_count; i++)
            free(args[i]);
        free(args);
        return 0;
    }

    if (strcmp(args[0], "pwd") == 0)
    {
        char *cwd = malloc(INITIAL_BUFFER_SIZE);
        if (cwd == NULL)
        {
            for (size_t i = 0; i < arg_count; i++)
                free(args[i]);
            free(args);
            return -1;
        }
        if (getcwd(cwd, INITIAL_BUFFER_SIZE) != NULL)
            printf("%s\n", cwd);
        free(cwd);
        for (size_t i = 0; i < arg_count; i++)
            free(args[i]);
        free(args);
        return 0;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        for (size_t i = 0; i < arg_count; i++)
            free(args[i]);
        free(args);
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        if (in_fd != 0)
        {
            dup2(in_fd, 0);
            close(in_fd);
        }
        if (out_fd != 1)
        {
            dup2(out_fd, 1);
            close(out_fd);
        }
        if (err_fd != 2)
        {
            dup2(err_fd, 2);
            close(err_fd);
        }
        execvp(args[0], args);
        fprintf(stderr, "Command %s not found\n", args[0]);
        for (size_t i = 0; i < arg_count; i++)
            free(args[i]);
        free(args);
        exit(EXIT_FAILURE);
    }

    int status;
    waitpid(pid, &status, 0);

    for (size_t i = 0; i < arg_count; i++)
        free(args[i]);
    free(args);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        return -1;
    return 0;
}

// Function for executing a command pipeline
void execute_pipeline(char **commands, int command_count)
{
    int *pipe_fds = malloc(sizeof(int) * 2 * (command_count - 1));
    if (pipe_fds == NULL) exit(EXIT_FAILURE);
    for (int i = 0; i < command_count - 1; i++)
    {
        if (pipe(pipe_fds + i * 2) < 0)
        {
            exit(EXIT_FAILURE);
        }
    }

    int input_fd = 0;
    int error_fd = 2;
    int command_failed = 0;

    for (int i = 0; i < command_count; i++)
    {
        if (strlen(commands[i]) == 0)
            continue;

        size_t arg_count;
        char **args = split_arguments(commands[i], &arg_count);
        if (args == NULL) exit(EXIT_FAILURE);

        if (strcmp(args[0], "exit") == 0)
        {
            printf("Shell is closing, bye\n");
            for (size_t j = 0; j < arg_count; j++)
                free(args[j]);
            free(args);
            exit(EXIT_SUCCESS);
        }

        int output_fd = (i < command_count - 1) ? pipe_fds[i * 2 + 1] : 1; // If not the last command, use pipe
        if (execute_command(commands[i], input_fd, output_fd, error_fd) == -1)
        {
            if (!command_failed)
                command_failed = 1;
        }

        if (input_fd != 0)
            close(input_fd);

        if (i < command_count - 1)
            close(pipe_fds[i * 2 + 1]);

        input_fd = pipe_fds[i * 2];

        // Freeing up memory for tokens
        for (size_t j = 0; j < arg_count; j++)
            free(args[j]);
        free(args);
    }

    for (int i = 0; i < command_count - 1; i++)
        close(pipe_fds[i * 2]);

    for (int i = 0; i < command_count; i++)
    {
        int status;
        wait(&status);
    }
}


char *read_input()
{
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(buffer_size);
    if (buffer == NULL)
    {
        exit(EXIT_FAILURE);
    }
    size_t position = 0;
    int c;

    while (1)
    {
        c = getchar();
        if (c == '\n' || c == EOF)
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            if (position >= buffer_size - 1)
            {
                buffer_size *= 2;
                char *new_buffer = realloc(buffer, buffer_size);
                if (new_buffer == NULL)
                {
                    free(buffer);
                    exit(EXIT_FAILURE);
                }
                buffer = new_buffer;
            }
            buffer[position++] = c;
        }
    }
}


int main()
{
    while (1)
    {
        //printf("> ");
        char *input = read_input();

        if (input == NULL || (strlen(input) == 0 && feof(stdin)))
        {
            free(input);
            break;
        }

        if (strlen(input) == 0)
        {
            free(input);
            continue;
        }

        if (check_syntax(input) == -1)
        {
            fprintf(stderr, "Syntax error\n");
            fflush(stderr);
            free(input);
            continue;
        }

        char **commands = malloc(INITIAL_COMMANDS_SIZE * sizeof(char*));
        if (commands == NULL)
        {
            free(input);
            return 1;
        }

        int command_count = 0;
        size_t commands_size = INITIAL_COMMANDS_SIZE;

        char *token = strtok(input, "|");
        while (token != NULL)
        {
            if (command_count >= commands_size)
            {
                commands_size *= 2;
                char **temp = realloc(commands, commands_size * sizeof(char*));
                if (temp == NULL)
                {
                    for (int i = 0; i < command_count; i++)
                        free(commands[i]);
                    free(commands);
                    free(input);
                    return 1;
                }
                commands = temp;
            }

            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }
        commands[command_count] = NULL;

        execute_pipeline(commands, command_count);

        free(commands);
        free(input);
    }
    return 0;
}
