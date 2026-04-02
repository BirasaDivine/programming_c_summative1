#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define MAX_PIPES 16

// Structure to hold command information
typedef struct {
  char *args[MAX_ARGS];
  int argc;
  char *input_file;
  char *output_file;
  int background;
} Command;

// Function prototypes
void print_prompt();
char *read_command();
int parse_command(char *cmd_line, Command commands[], int *num_commands);
int execute_builtin(Command *cmd);
void execute_command(Command *cmd);
void execute_pipeline(Command commands[], int num_commands);
void setup_redirection(Command *cmd);
void cleanup_command(Command *cmd);
char *trim_whitespace(char *str);

// Built-in command functions
int shell_exit(char **args);
int shell_pwd(char **args);
int shell_cd(char **args);
int shell_echo(char **args);
int shell_help(char **args);

// Array of built-in commands and their functions
char *builtin_commands[] = {"exit", "pwd", "cd", "echo", "help"};

int (*builtin_functions[])(char **) = {&shell_exit, &shell_pwd, &shell_cd,
                                       &shell_echo, &shell_help};

int num_builtins() { return sizeof(builtin_commands) / sizeof(char *); }

int main() {
  char *cmd_line;
  Command commands[MAX_PIPES];
  int num_commands;
  int status = 1;

  printf("=== Mini Shell ===\n");
  printf("Type 'help' for available commands or 'exit' to quit.\n\n");

  // Main shell loop
  while (status) {
    print_prompt();
    cmd_line = read_command();

    if (cmd_line == NULL || strlen(cmd_line) == 0) {
      free(cmd_line);
      continue;
    }

    // Parse the command line
    if (parse_command(cmd_line, commands, &num_commands) == 0) {
      if (num_commands > 0) {
        // Check if it's a built-in command (only for single commands)
        if (num_commands == 1 && execute_builtin(&commands[0])) {
          if (strcmp(commands[0].args[0], "exit") == 0) {
            status = 0;
          }
        } else {
          // Execute pipeline or single command
          execute_pipeline(commands, num_commands);
        }
      }
    }

    // Cleanup
    for (int i = 0; i < num_commands; i++) {
      cleanup_command(&commands[i]);
    }
    free(cmd_line);
  }

  printf("Shell terminated. Goodbye!\n");
  return 0;
}

void print_prompt() {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("minishell:%s$ ", cwd);
  } else {
    printf("minishell$ ");
  }
  fflush(stdout);
}

char *read_command() {
  char *line = malloc(MAX_CMD_LEN);
  if (!line) {
    fprintf(stderr, "Memory allocation error\n");
    exit(1);
  }

  if (fgets(line, MAX_CMD_LEN, stdin) == NULL) {
    free(line);
    return NULL;
  }

  // Remove trailing newline
  line[strcspn(line, "\n")] = 0;
  return line;
}

char *trim_whitespace(char *str) {
  char *end;

  // Trim leading space
  while (*str == ' ' || *str == '\t')
    str++;

  if (*str == 0)
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && (*end == ' ' || *end == '\t'))
    end--;

  end[1] = '\0';
  return str;
}

int parse_command(char *cmd_line, Command commands[], int *num_commands) {
  *num_commands = 0;

  // Split by pipes
  char *pipe_tokens[MAX_PIPES];
  int pipe_count = 0;

  char *token = strtok(cmd_line, "|");
  while (token != NULL && pipe_count < MAX_PIPES) {
    pipe_tokens[pipe_count++] = trim_whitespace(token);
    token = strtok(NULL, "|");
  }

  // Parse each command in the pipeline
  for (int i = 0; i < pipe_count; i++) {
    Command *cmd = &commands[i];
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->background = 0;

    char *cmd_copy = strdup(pipe_tokens[i]);
    if (!cmd_copy) {
      fprintf(stderr, "Memory allocation error\n");
      return -1;
    }

    // Check for output redirection
    char *output_redirect = strstr(cmd_copy, ">");
    if (output_redirect) {
      *output_redirect = '\0';
      output_redirect++;
      cmd->output_file = strdup(trim_whitespace(output_redirect));
    }

    // Check for input redirection
    char *input_redirect = strstr(cmd_copy, "<");
    if (input_redirect) {
      *input_redirect = '\0';
      input_redirect++;
      cmd->input_file = strdup(trim_whitespace(input_redirect));
    }

    // Check for background execution
    char *bg_check = strstr(cmd_copy, "&");
    if (bg_check) {
      *bg_check = '\0';
      cmd->background = 1;
    }

    // Parse arguments
    token = strtok(cmd_copy, " \t");
    while (token != NULL && cmd->argc < MAX_ARGS - 1) {
      cmd->args[cmd->argc++] = strdup(token);
      token = strtok(NULL, " \t");
    }
    cmd->args[cmd->argc] = NULL;

    free(cmd_copy);
  }

  *num_commands = pipe_count;
  return 0;
}

int execute_builtin(Command *cmd) {
  if (cmd->argc == 0)
    return 0;

  for (int i = 0; i < num_builtins(); i++) {
    if (strcmp(cmd->args[0], builtin_commands[i]) == 0) {
      return builtin_functions[i](cmd->args);
    }
  }
  return 0;
}

void setup_redirection(Command *cmd) {
  if (cmd->input_file) {
    int fd = open(cmd->input_file, O_RDONLY);
    if (fd == -1) {
      perror("Input redirection failed");
      exit(1);
    }
    dup2(fd, STDIN_FILENO);
    close(fd);
  }

  if (cmd->output_file) {
    int fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      perror("Output redirection failed");
      exit(1);
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }
}

void execute_command(Command *cmd) {
  if (cmd->argc == 0)
    return;

  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    setup_redirection(cmd);

    if (execvp(cmd->args[0], cmd->args) == -1) {
      fprintf(stderr, "minishell: %s: command not found\n", cmd->args[0]);
      exit(1);
    }
  } else if (pid < 0) {
    perror("Fork failed");
  } else {
    // Parent process
    if (!cmd->background) {
      int status;
      waitpid(pid, &status, 0);
    }
  }
}

void execute_pipeline(Command commands[], int num_commands) {
  if (num_commands == 1) {
    execute_command(&commands[0]);
    return;
  }

  int pipes[MAX_PIPES][2];
  pid_t pids[MAX_PIPES];

  // Create pipes
  for (int i = 0; i < num_commands - 1; i++) {
    if (pipe(pipes[i]) == -1) {
      perror("Pipe creation failed");
      return;
    }
  }

  // Execute commands in pipeline
  for (int i = 0; i < num_commands; i++) {
    pids[i] = fork();

    if (pids[i] == 0) {
      // Child process

      // Setup input redirection
      if (i > 0) {
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }

      // Setup output redirection
      if (i < num_commands - 1) {
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      // Close all pipe descriptors
      for (int j = 0; j < num_commands - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      // Setup file redirection
      setup_redirection(&commands[i]);

      // Execute command
      if (execvp(commands[i].args[0], commands[i].args) == -1) {
        fprintf(stderr, "minishell: %s: command not found\n",
                commands[i].args[0]);
        exit(1);
      }
    } else if (pids[i] < 0) {
      perror("Fork failed");
      return;
    }
  }

  // Close all pipe descriptors in parent
  for (int i = 0; i < num_commands - 1; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  // Wait for all children
  for (int i = 0; i < num_commands; i++) {
    int status;
    waitpid(pids[i], &status, 0);
  }
}

void cleanup_command(Command *cmd) {
  for (int i = 0; i < cmd->argc; i++) {
    free(cmd->args[i]);
  }
  if (cmd->input_file)
    free(cmd->input_file);
  if (cmd->output_file)
    free(cmd->output_file);
}

// Built-in command implementations
int shell_exit(char **args) {
  (void)args; // Suppress unused parameter warning
  printf("Exiting shell...\n");
  return 1;
}

int shell_pwd(char **args) {
  (void)args; // Suppress unused parameter warning
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("pwd failed");
  }
  return 1;
}

int shell_cd(char **args) {
  if (args[1] == NULL) {
    // No argument, go to home directory
    if (chdir(getenv("HOME")) != 0) {
      perror("cd failed");
    }
  } else {
    if (chdir(args[1]) != 0) {
      perror("cd failed");
    }
  }
  return 1;
}

int shell_echo(char **args) {
  for (int i = 1; args[i] != NULL; i++) {
    printf("%s", args[i]);
    if (args[i + 1] != NULL) {
      printf(" ");
    }
  }
  printf("\n");
  return 1;
}

int shell_help(char **args) {
  (void)args; // Suppress unused parameter warning
  printf("=== Mini Shell Help ===\n");
  printf("Built-in commands:\n");
  printf("  exit    - Exit the shell\n");
  printf("  pwd     - Print working directory\n");
  printf("  cd      - Change directory\n");
  printf("  echo    - Print arguments\n");
  printf("  help    - Show this help\n");
  printf("\nFeatures supported:\n");
  printf("  - Basic Unix commands (ls, cat, touch, etc.)\n");
  printf("  - Input redirection: command < file\n");
  printf("  - Output redirection: command > file\n");
  printf("  - Piping: command1 | command2\n");
  printf("  - Background execution: command &\n");
  printf("\nExamples:\n");
  printf("  ls -la\n");
  printf("  cat file.txt\n");
  printf("  ls | grep .txt\n");
  printf("  echo 'Hello' > output.txt\n");
  printf("  cat < input.txt\n");
  return 1;
}
