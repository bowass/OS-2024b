#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
typedef struct
{
    const char **start;
    size_t len;
    size_t capacity;
} dynamic_array;
void dynamic_array_init(dynamic_array *const d, const size_t capacity);
void dynamic_array_push(dynamic_array *const d, const char *str);
void dynamic_array_free(dynamic_array *const d);
void dynamic_array_clear(dynamic_array *const d);
const char *dynamic_array_last(const dynamic_array *const d);
char *strdup(const char *);

int main(void)
{
    close(2);
    dup(1);
    char command[BUFFER_SIZE];
    char *token;
    int background = 0;
    dynamic_array tokens;
    dynamic_array history;
    dynamic_array_init(&tokens, 1);
    dynamic_array_init(&history, 1);
    for (;;)
    {
        fprintf(stdout, "my-shell> ");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
        if (strncmp(command, "exit", 4) == 0)
        {
            break;
        }
        dynamic_array_push(&history, command);

        token = strtok(command, " \n");
        if (!token)
            continue;
        if (strncmp(token, "history", 7) == 0)
        {
            size_t i = history.len;
            while (i--)
            {
                fprintf(stdout, "%d %s", i + 1, history.start[i]);
            }
            continue;
        }
        while (token != NULL)
        {
            dynamic_array_push(&tokens, token);
            token = strtok(NULL, " \n");
        }
        if (background = (strncmp(tokens.start[tokens.len - 1], "&", 1) == 0))
        {
            free((char *)tokens.start[--tokens.len]);
            tokens.start[tokens.len] = NULL;
        }
        pid_t pid = fork();
        if (pid == 0)
        {
            if (execvp(tokens.start[0], (char **)tokens.start) == -1)
            {
                perror("error");
                dynamic_array_clear(&tokens);
                exit(1);
            }
        }
        else if (pid < 0)
        {
            perror("error");
            exit(1);
        }
        else if (pid > 0 && !background)
        {
            int status;
            waitpid(pid, &status, 0);
            if (!WIFEXITED(status))
            {
                perror("error");
                exit(1);
            }
        }

        dynamic_array_clear(&tokens);
    }
    dynamic_array_free(&tokens);
    return 0;
}
void dynamic_array_init(dynamic_array *const d, const size_t capacity)
{
    const char **start = malloc(capacity * sizeof(char *));
    if (!start)
    {
        perror("error");
        exit(1);
    }
    memset(start, 0, capacity * sizeof(char *));
    d->start = start;
    d->len = 0;
    d->capacity = capacity;
}

void dynamic_array_push(dynamic_array *const d, const char *str)
{
    if (d->len == d->capacity)
    {
        d->capacity *= 2;
        const char **tmp = realloc(d->start, d->capacity * sizeof(char *));
        if (!tmp)
        {
            perror("error");
            exit(1);
        }
        d->start = tmp;
    }
    const char *tmp_dup = strdup(str);
    if (!tmp_dup)
    {
        perror("error");
        exit(1);
    }
    d->start[d->len++] = tmp_dup;
}
void dynamic_array_free(dynamic_array *const d)
{
    for (char **str = (char **)d->start; str < (char **)d->start + d->len; ++str)
    {
        free(*str);
    }
    free(d->start);
}
void dynamic_array_clear(dynamic_array *const d)
{
    for (char **str = (char **)d->start; str < (char **)d->start + d->len; ++str)
    {
        free(*str);
        *str = NULL;
    }
    d->len = 0;
}
