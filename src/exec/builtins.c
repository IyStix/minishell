#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "builtins.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int is_builtin(const char *cmd)
{
    return cmd && (
        strcmp(cmd, "echo") == 0 ||
        strcmp(cmd, "cd") == 0 ||
        strcmp(cmd, "exit") == 0 ||
        strcmp(cmd, "kill") == 0
    );
}

int builtin_echo(char **args, int arg_count, struct exec_state *state __attribute__((unused)))
{
    for (int i = 1; i < arg_count; i++)
    {
        printf("%s", args[i]);
        if (i < arg_count - 1)
            printf(" ");
    }
    printf("\n");
    fflush(stdout);
    return 0;
}

int builtin_cd(char **args, int arg_count, struct exec_state *state __attribute__((unused)))
{
    char *path;
    char cwd[PATH_MAX];
    int ret;
    
    if (arg_count == 1)
    {
        path = getenv("HOME");
        if (!path)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    }
    else
        path = args[1];

    if (getcwd(cwd, sizeof(cwd)) != NULL)
        setenv("OLDPWD", cwd, 1);

    ret = chdir(path);
    if (ret != 0)
    {
        perror("cd");
        return 1;
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL)
        setenv("PWD", cwd, 1);
    
    return 0;
}

int builtin_exit(char **args, int arg_count, struct exec_state *state)
{
    state->should_exit = 1;
    if (arg_count > 1)
    {
        char *endptr;
        long exit_code = strtol(args[1], &endptr, 10);
        if (*endptr != '\0')
        {
            fprintf(stderr, "exit: numeric argument required\n");
            state->exit_code = 2;
            return 2;
        }
        state->exit_code = exit_code & 0xFF;
        return state->exit_code;
    }
    state->exit_code = state->last_return;
    return state->exit_code;
}

int builtin_kill(char **args, int arg_count, struct exec_state *state __attribute__((unused)))
{
    if (arg_count < 2)
    {
        fprintf(stderr, "kill: usage: kill [-s sigspec | -n signum | -sigspec] pid\n");
        return 1;
    }
    
    int signum = SIGTERM;
    int arg_index = 1;
    
    if (args[1][0] == '-' && arg_count >= 3)
    {
        signum = atoi(args[1] + 1);
        arg_index = 2;
    }
    
    for (; arg_index < arg_count; arg_index++)
    {
        pid_t pid = atoi(args[arg_index]);
        if (kill(pid, signum) == -1)
        {
            perror("kill");
            return 1;
        }
    }
    
    return 0;
}
