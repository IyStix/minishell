#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include "exec.h"
#include "builtins.h"

extern char **environ;

static int exec_external_command(struct command *cmd, struct exec_state *state);
void restore_redirections(int saved_fds[3]);

int handle_redirections(struct command *cmd)
{
    int saved_fds[3];
    
    saved_fds[0] = dup(STDIN_FILENO);
    saved_fds[1] = dup(STDOUT_FILENO);
    saved_fds[2] = dup(STDERR_FILENO);

    for (int i = 0; i < cmd->redirections_count; i++)
    {
        struct redirection *redir = cmd->redirections[i];
        int fd;
        int target_fd = (redir->ionumber == -1) ? 
            (strcmp(redir->operator, "<") == 0 ? STDIN_FILENO : STDOUT_FILENO) : 
            redir->ionumber;
            
        if (strcmp(redir->operator, "<") == 0)
        {
            fd = open(redir->word, O_RDONLY);
            if (fd == -1)
            {
                fprintf(stderr, "minishell: %s: No such file or directory\n", redir->word);
                restore_redirections(saved_fds);
                return 1;
            }
        }
        else if (strcmp(redir->operator, ">") == 0)
        {
            fd = open(redir->word, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1)
            {
                perror("minishell");
                restore_redirections(saved_fds);
                return 1;
            }
        }
        else if (strcmp(redir->operator, ">>") == 0)
        {
            fd = open(redir->word, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1)
            {
                perror("minishell");
                restore_redirections(saved_fds);
                return 1;
            }
        }
        else
        {
            restore_redirections(saved_fds);
            return 1;
        }

        if (dup2(fd, target_fd) == -1)
        {
            perror("minishell");
            close(fd);
            restore_redirections(saved_fds);
            return 1;
        }
        close(fd);
    }
    return 0;
}

void restore_redirections(int saved_fds[3])
{
    dup2(saved_fds[0], STDIN_FILENO);
    dup2(saved_fds[1], STDOUT_FILENO);
    dup2(saved_fds[2], STDERR_FILENO);
    close(saved_fds[0]);
    close(saved_fds[1]);
    close(saved_fds[2]);
}

static char *safe_strdup(const char *str)
{
    if (!str)
        return NULL;
    return strdup(str);
}

static int exec_external_command(struct command *cmd, struct exec_state *state)
{
    if (!cmd->name)
        return 0;

    char *full_path = NULL;
    
    if (cmd->name[0] == '/' || 
        (cmd->name[0] == '.' && cmd->name[1] == '/') ||
        (cmd->name[0] == '.' && cmd->name[1] == '.' && cmd->name[2] == '/'))
    {
        full_path = safe_strdup(cmd->name);
    }
    else
    {
        char *path = getenv("PATH");
        if (!path)
            path = "/bin:/usr/bin";

        char *path_copy = safe_strdup(path);
        char *dir = strtok(path_copy, ":");
        
        while (dir)
        {
            char tmp[4096];
            snprintf(tmp, sizeof(tmp), "%s/%s", dir, cmd->name);
            if (access(tmp, X_OK) == 0)
            {
                full_path = safe_strdup(tmp);
                break;
            }
            dir = strtok(NULL, ":");
        }
        free(path_copy);
    }
    
    if (!full_path)
    {
        fprintf(stderr, "minishell: %s: command not found\n", cmd->name);
        state->last_return = 127;
        return 127;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("minishell: fork");
        free(full_path);
        state->last_return = 1;
        return 1;
    }
        
    if (pid == 0)
    {
        if (handle_redirections(cmd) != 0)
            exit(1);
            
        execv(full_path, cmd->args);
        
        if (errno == EACCES)
        {
            fprintf(stderr, "minishell: %s: Permission denied\n", cmd->name);
            exit(126);
        }
        fprintf(stderr, "minishell: %s: command not found\n", cmd->name);
        exit(127);
    }
    
    free(full_path);
    int status;
    waitpid(pid, &status, 0);
    state->last_return = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    return state->last_return;
}

static int exec_command_with_env(struct command *cmd, struct exec_state *state)
{
    char **old_env = environ;
    int ret;

    if (!cmd->name && cmd->assignments_count > 0)
    {
        for (int i = 0; i < cmd->assignments_count; i++)
        {
            char *assignment = safe_strdup(cmd->assignments[i]);
            putenv(assignment);
        }
        return 0;
    }

    if (cmd->assignments_count > 0)
    {
        char **new_env = malloc(sizeof(char *) * (cmd->assignments_count + 1));
        for (int i = 0; i < cmd->assignments_count; i++)
        {
            new_env[i] = safe_strdup(cmd->assignments[i]);
        }
        new_env[cmd->assignments_count] = NULL;
        environ = new_env;
    }

    ret = exec_external_command(cmd, state);

    if (cmd->assignments_count > 0)
    {
        for (int i = 0; i < cmd->assignments_count; i++)
        {
            free(environ[i]);
        }
        free(environ);
        environ = old_env;
    }

    return ret;
}

struct exec_state *exec_init(char **env)
{
    struct exec_state *state = malloc(sizeof(struct exec_state));
    if (!state)
        return NULL;
        
    state->env = env;
    state->last_return = 0;
    state->should_exit = 0;
    state->exit_code = 0;
    
    return state;
}

void exec_free(struct exec_state *state)
{
    if (state)
        free(state);
}

int exec_pipeline(struct ast_node *node, struct exec_state *state)
{
    if (node->type != NODE_PIPELINE)
        return exec_command(node->data.command, state);
        
    int pipefd[2];
    if (pipe(pipefd) == -1)
        return 1;
        
    pid_t pid1 = fork();
    if (pid1 == -1)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }
    
    if (pid1 == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        exit(exec_ast(node->data.binary.left, state));
    }
    
    pid_t pid2 = fork();
    if (pid2 == -1)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }
    
    if (pid2 == 0)
    {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        exit(exec_ast(node->data.binary.right, state));
    }
    
    close(pipefd[0]);
    close(pipefd[1]);
    
    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    
    return WEXITSTATUS(status2);
}

int exec_and_or(struct ast_node *node, struct exec_state *state)
{
    if (node->type != NODE_AND_OR)
        return exec_pipeline(node, state);
        
    int left_status = exec_ast(node->data.binary.left, state);
    state->last_return = left_status;
    
    if (strcmp(node->data.binary.operator, "&&") == 0)
    {
        if (left_status == 0)
            return exec_ast(node->data.binary.right, state);
        return left_status;
    }
    else
    {
        if (left_status != 0)
            return exec_ast(node->data.binary.right, state);
        return left_status;
    }
}

int exec_command(struct command *cmd, struct exec_state *state)
{
    int ret;

    if (is_builtin(cmd->name))
    {
        if (strcmp(cmd->name, "echo") == 0)
            ret = builtin_echo(cmd->args, cmd->args_count, state);
        else if (strcmp(cmd->name, "cd") == 0)
            ret = builtin_cd(cmd->args, cmd->args_count, state);
        else if (strcmp(cmd->name, "exit") == 0)
            ret = builtin_exit(cmd->args, cmd->args_count, state);
        else if (strcmp(cmd->name, "kill") == 0)
            ret = builtin_kill(cmd->args, cmd->args_count, state);
        else
            ret = 1;
    }
    else
    {
        ret = exec_command_with_env(cmd, state);
    }

    state->last_return = ret;
    return ret;
}

int exec_ast(struct ast_node *node, struct exec_state *state)
{
    if (!node)
        return 0;

    int ret;
        
    switch (node->type)
    {
        case NODE_COMMAND:
            if (is_builtin(node->data.command->name))
            {
                if (strcmp(node->data.command->name, "echo") == 0)
                    ret = builtin_echo(node->data.command->args, node->data.command->args_count, state);
                else if (strcmp(node->data.command->name, "cd") == 0)
                    ret = builtin_cd(node->data.command->args, node->data.command->args_count, state);
                else if (strcmp(node->data.command->name, "exit") == 0)
                    ret = builtin_exit(node->data.command->args, node->data.command->args_count, state);
                else if (strcmp(node->data.command->name, "kill") == 0)
                    ret = builtin_kill(node->data.command->args, node->data.command->args_count, state);
                else
                    ret = 1;
            }
            else
                ret = exec_command_with_env(node->data.command, state);
            state->last_return = ret;
            return ret;
        case NODE_PIPELINE:
            ret = exec_pipeline(node, state);
            state->last_return = ret;
            return ret;
        case NODE_AND_OR:
            ret = exec_and_or(node, state);
            state->last_return = ret;
            return ret;
        case NODE_SEQUENCE:
            exec_ast(node->data.binary.left, state);
            ret = exec_ast(node->data.binary.right, state);
            state->last_return = ret;
            return ret;
        default:
            state->last_return = 1;
            return 1;
    }
}
