// COP4610
// Project 1
// Joshua Rosenfield, Barrett McKinney, Ryan Kenney
#include <signal.h>  //kill
#include <stdbool.h> //bool
#include <stdio.h>
#include <stdlib.h>   //getenv
#include <string.h>   //strcmp
#include <sys/stat.h> //check for files
#include <unistd.h>   //getlogin_r, gethostname, getcwd

typedef struct {
  char **tokens;
  int numTokens;
  int exitTotal; // exit command
} instruction;

typedef struct {
  char command[256][256];
  int pids[256];
  int numProcess; // number of processes running
  int bg;         // background process bool
} bg_struct;

struct alias {
  char *shortcut;
  char *aliasCommand;
  char *option;
};

void addToken(instruction *instr_ptr, char *tok);
void executeTokens(instruction *instr_ptr, bg_struct *bg_ptr);
void clearInstruction(instruction *instr_ptr);
void addNull(instruction *instr_ptr);
char *resolveShortcut(char *path);
char *pathResolution(char *cmd);

int main() {
  char *token = NULL;
  char *temp = NULL;

  int aliasCount = 0;
  struct alias commands[20];

  instruction instr;
  instr.tokens = NULL;
  instr.numTokens = 0;
  instr.exitTotal = 0; // initial exit

  bg_struct bgprocesses;
  bgprocesses.numProcess = 0;
  bgprocesses.bg = 0;

  // user login
  char loginarr[256];
  char hostarr[256];
  char cwdarr[256];
  getlogin_r(loginarr, 256);
  gethostname(hostarr, 256);
  getcwd(cwdarr, 256);

  while (1) {
    getcwd(cwdarr, 256);
    printf("%s@%s : %s>", loginarr, hostarr, cwdarr);
    int invalid_path = 0; // Set if resolveShortcut finds a bad path
    // printf("Please enter an instruction: ");

    // loop reads character sequences separated by whitespace
    do {
      // scans for next token and allocates token var to size of scanned token
      scanf("%ms", &token);

      // first cmd = & skip
      if (strcmp(token, "&") == 0 && instr.numTokens == 0)
        continue;

      temp = (char *)malloc((strlen(token) + 1) * sizeof(char));

      int i;
      int start = 0;
      for (i = 0; i < strlen(token); i++) {
        // pull out special characters and make them into a separate token in
        // the instruction
        if (token[i] == '|' || token[i] == '>' || token[i] == '<' ||
            token[i] == '&') {
          if (i - start > 0) {
            memcpy(temp, token + start, i - start);
            temp[i - start] = '\0';
            addToken(&instr, temp);
          }

          char specialChar[2];
          specialChar[0] = token[i];
          specialChar[1] = '\0';

          addToken(&instr, specialChar);

          start = i + 1;
        }
      }

      if (start < strlen(token)) {
        memcpy(temp, token + start, strlen(token) - start);
        temp[i - start] = '\0';
        addToken(&instr, temp);
      }

      // free and reset variables
      free(token);
      free(temp);
      token = NULL;
      temp = NULL;
    } while ('\n' != getchar()); // until end of line is reached

    // allocate space for the c-strings
    commands[aliasCount].shortcut = malloc(20);
    commands[aliasCount].aliasCommand = malloc(20);
    commands[aliasCount].option = malloc(10);

    // alias check and assignment
    if (strcmp(instr.tokens[0], "alias") == 0) {
      strcpy(commands[aliasCount].shortcut, instr.tokens[1]);
      strcpy(commands[aliasCount].aliasCommand, instr.tokens[3]);
      if (instr.numTokens == 5) {
        strcpy(commands[aliasCount].option, instr.tokens[4]);
      }

      clearInstruction(&instr);
      aliasCount++;
      instr.exitTotal++;
    } else if (strcmp(instr.tokens[0], "unalias") == 0) {
      int commandCount;
      for (commandCount = 0; commandCount <= aliasCount; commandCount++) {
        if (strcmp(commands[commandCount].shortcut, instr.tokens[1]) == 0) {
          // Deleting the commands and shortcuts
          strcpy(commands[commandCount].shortcut, "");
          strcpy(commands[commandCount].aliasCommand, "");
          strcpy(commands[commandCount].option, "");
        }
      }
      clearInstruction(&instr);
      instr.exitTotal++;
    } else {
      // check for alias commands
      int commandCount;
      for (commandCount = 0; commandCount <= aliasCount; commandCount++) {
        if (strcmp(commands[commandCount].shortcut, instr.tokens[0]) == 0) {
          // Replacing the given command with the stored command
          strcpy(instr.tokens[0], commands[commandCount].aliasCommand);
          // add the option token
          if (commands[commandCount].option != NULL) {
            addToken(&instr, commands[commandCount].option);
          }
        }
      }

      int j = 0;
      int i = 0;
      // if background process add to amount executing
      if (instr.numTokens > 1 && instr.tokens[instr.numTokens - 1][0] == '&') {
        instr.tokens[instr.numTokens - 1] = NULL;
        instr.numTokens--;
        bgprocesses.bg = 1;
        // Copy command and arguments
        memset(bgprocesses.command[bgprocesses.numProcess], '\0', 256);
        strcat(bgprocesses.command[bgprocesses.numProcess], instr.tokens[0]);
        for (j = 1; j < instr.numTokens; ++j) {
          strcat(bgprocesses.command[bgprocesses.numProcess], " ");
          strcat(bgprocesses.command[bgprocesses.numProcess], instr.tokens[j]);
        }
      }
      // check for . and / in each token to determine if path; if so expand it
      if (strcmp(instr.tokens[0], "echo\0") != 0) {
        for (i = 0; i < instr.numTokens; ++i) {
          for (j = 0; j < strlen(instr.tokens[i]); ++j) {
            if (instr.tokens[i][j] == '/' || instr.tokens[i][j] == '.' ||
                instr.tokens[i][j] == '~') {
              instr.tokens[i] = resolveShortcut(instr.tokens[i]);
              if (strcmp(instr.tokens[i], " \0") == 0)
                invalid_path = 1;
              break;
            }
          }
        }
      }

      addNull(&instr);
      // Only execute instruction if it has a valid path
      if (invalid_path == 0)
        executeTokens(&instr, &bgprocesses);
      clearInstruction(&instr);
      instr.exitTotal += 1;
    }
  }
  return 0;
}

// reallocates instruction array to hold another token
// allocates for new token within instruction array
void addToken(instruction *instr_ptr, char *tok) {
  // extend token array to accomodate an additional token
  if (instr_ptr->numTokens == 0)
    instr_ptr->tokens = (char **)malloc(sizeof(char *));
  else {
    instr_ptr->tokens = (char **)realloc(
        instr_ptr->tokens, (instr_ptr->numTokens + 1) * sizeof(char *));
  }
  // allocate char array for new token in new slot
  instr_ptr->tokens[instr_ptr->numTokens] =
      (char *)malloc((strlen(tok) + 1) * sizeof(char));
  strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

  instr_ptr->numTokens++;
}

void addNull(instruction *instr_ptr) {
  // extend token array to accomodate an additional token
  if (instr_ptr->numTokens == 0)
    instr_ptr->tokens = (char **)malloc(sizeof(char *));
  else
    instr_ptr->tokens = (char **)realloc(
        instr_ptr->tokens, (instr_ptr->numTokens + 1) * sizeof(char *));

  instr_ptr->tokens[instr_ptr->numTokens] = (char *)NULL;
  instr_ptr->numTokens++;
}

void executeTokens(instruction *instr_ptr, bg_struct *bg_ptr) {
  int i;
  bool isRedirect, inputRedirect, outputRedirect, pipeRedirect, bgProcess;
  int numPipes = 0;
  isRedirect = false;
  inputRedirect = false;
  outputRedirect = false;
  pipeRedirect = false;
  int status;
  pid_t pid, pipe_pid_1, pipe_pid_2;

  // char * redirect_path = NULL;
  char *input_path = NULL;
  char *output_path = NULL;

  // execution for <> -- cat PATH NULL > output.txt
  // 6 tokens
  if (instr_ptr->numTokens == 6 && (instr_ptr->tokens[1][0] == '<') &&
      (instr_ptr->tokens[3][0] == '>')) {
    inputRedirect = true;
    outputRedirect = true;

    input_path = instr_ptr->tokens[2];
    if (input_path == '\0')
      return;
    instr_ptr->tokens[1] = input_path;
    instr_ptr->tokens[2] = NULL;

    output_path = instr_ptr->tokens[4];
    if (output_path == '\0')
      return;
    instr_ptr->tokens[3] = NULL;
    instr_ptr->tokens[4] = NULL;

    instr_ptr->numTokens = instr_ptr->numTokens - 3;
  }
  // execution for ><
  else if (instr_ptr->numTokens == 6 && (instr_ptr->tokens[1][0] == '>') &&
           (instr_ptr->tokens[3][0] == '<')) {
    inputRedirect = true;
    outputRedirect = true;

    output_path = instr_ptr->tokens[2];
    if (output_path == '\0')
      return;
    instr_ptr->tokens[3] = NULL;
    instr_ptr->tokens[2] = NULL;

    input_path = instr_ptr->tokens[4];
    if (input_path == '\0')
      return;
    instr_ptr->tokens[1] = input_path;
    instr_ptr->tokens[4] = NULL;

    instr_ptr->numTokens = instr_ptr->numTokens - 3;
  }

  // check for Missing name for redirect
  // execute simple < or > command
  for (i = 0; i < instr_ptr->numTokens - 1; i++) {
    if (instr_ptr->tokens[i][0] == '>' || instr_ptr->tokens[i][0] == '<') {
      isRedirect = true;
      // error checking
      if (isRedirect == true && instr_ptr->numTokens < 4) {
        printf("Missing name for redirect.\n");
        return;
      }
      if (instr_ptr->tokens[i][0] == '<') {
        inputRedirect = true;
        input_path = instr_ptr->tokens[i + 1];
        // if file for redirection doesnt exit
        if (input_path == '\0')
          return;
        instr_ptr->tokens[i] = input_path;
        instr_ptr->tokens[i + 1] = NULL;
        instr_ptr->numTokens = instr_ptr->numTokens - 1;
      } else if (instr_ptr->tokens[i][0] == '>') {
        outputRedirect = true;
        output_path = instr_ptr->tokens[i + 1];
        // if file for redirection doesnt exit
        if (output_path == '\0')
          return;
        instr_ptr->tokens[i] = NULL;
        instr_ptr->tokens[i + 1] = NULL;
        instr_ptr->numTokens = instr_ptr->numTokens - 2;
      }
    }

    // Determine if pipe command
    if (instr_ptr->tokens[i][0] == '|') {
      if (i == 0 || i == instr_ptr->numTokens - 2 ||
          instr_ptr->tokens[i + 1][0] == '|') {
        printf("Invalid pipe syntax\n");
        return;
      }
      pipeRedirect = true;
      ++numPipes;
    }
  }

  // start of env implementation
  for (i = 0; i < instr_ptr->numTokens - 1; i++) {
    if (instr_ptr->tokens[i][0] == '$') {
      if (getenv(instr_ptr->tokens[i] + 1) == NULL) {
        printf("%s: Undefined variable.\n", instr_ptr->tokens[i]);
        return; // end this command
      } else { // copys $*** to its token
        strcpy(instr_ptr->tokens[i], getenv(instr_ptr->tokens[i] + 1));
      }
    }
  }

  // built ins
  if (strcmp(instr_ptr->tokens[0], "exit") == 0 && instr_ptr->numTokens == 2) {
    if (bg_ptr->numProcess != 0) {
      printf("Waiting for processes to finish...\n");
      wait(0);
    }
    printf("Exiting...\n");
    printf("\tCommands executed: %d\n", instr_ptr->exitTotal);
    exit(0);
  }
  // cd not finished
  else if (strcmp(instr_ptr->tokens[0], "cd") == 0) {

    if (instr_ptr->numTokens > 3)
      printf("Too many arguments.\n");
    else if (instr_ptr->numTokens == 2) {
      chdir(getenv("HOME"));
    } else if (chdir(instr_ptr->tokens[1]) != 0)
      printf("%s: No such file or directory.\n", instr_ptr->tokens[1]);
  }
  // Echo
  else if (strcmp(instr_ptr->tokens[0], "echo") == 0) {
    for (i = 1; i < instr_ptr->numTokens; i++) {
      if (instr_ptr->tokens[i] != NULL && instr_ptr->tokens[i][0] == '$') {
        // Check if valid env variable
        if (strcmp(instr_ptr->tokens[i], getenv(instr_ptr->tokens[i])) == 0)
          printf("%s ", getenv(instr_ptr->tokens[i]));
        else
          printf("Variable not found.\n");
      } else if (instr_ptr->tokens[i] != NULL)
        printf("%s ", (instr_ptr->tokens)[i]);
    }
    printf("\n");
  }

  // forks and execv
  else {
    strcpy(instr_ptr->tokens[0], pathResolution(instr_ptr->tokens[0]));
    int fd0, fd1;
    int **fdpipe;
    pid = fork();

    // Create and populate pipe array
    if (pipeRedirect) {
      fdpipe = (int **)malloc((numPipes) * sizeof(int *));
      for (i = 0; i < numPipes; ++i) {
        fdpipe[i] = (int *)malloc(2 * sizeof(int));
        if (pipe(fdpipe[i]) == -1) {
          printf("Pipe error\n");
          kill(getpid(), SIGKILL);
        }
      }
    }
    if (inputRedirect) {
      fd0 = open(input_path);
    }
    if (outputRedirect) {
      fd1 = creat(output_path);
    }
    if (pid == -1) {
      perror("fork error");
    } else if (pid == 0) {
      if (pipeRedirect) {
        if (numPipes > 2) {
          fprintf(stderr, "More than 2 pipe\n");
          kill(getpid(), SIGKILL);
        }
        int j = 0;
        // Create instruction array for individual pipe commands
        instruction *cmds =
            (instruction *)malloc((numPipes + 2) * sizeof(instruction));
        cmds->numTokens = numPipes + 1;
        // Populate instruction array
        for (i = 0; i < numPipes + 1; ++i) {
          int k;
          cmds[i].numTokens = 0;
          for (k = 0;
               j < instr_ptr->numTokens - 1 && instr_ptr->tokens[j][0] != '|';
               ++k, ++j) {
            addToken(&cmds[i], instr_ptr->tokens[j]);
          }
          addNull(&cmds[i]);
          strcpy(cmds[i].tokens[0], pathResolution(cmds[i].tokens[0]));
          // Make sure a valid command is found
          if (cmds[i].tokens[0][0] != '/') {
            fprintf(stderr, "%s: Command not found\n", cmds[i].tokens[0]);
            kill(getpid(), SIGKILL);
          }
          if (instr_ptr->tokens[j - 1][0] == '&') {
            fprintf(stderr, "Invalid syntax\n");
            kill(getpid(), SIGKILL);
          }
          ++j;
        }
        // Fork to begin pipe
        pid_t temp_pid_1 = fork();
        if (temp_pid_1 == -1) {
          fprintf(stderr, "Fork error.\n");
          kill(getpid(), SIGKILL);
        } else if (temp_pid_1 == 0) {
          close(STDOUT_FILENO);
          dup(fdpipe[0][1]);
          close(fdpipe[0][1]);
          // close(fdpipe[0][0]);

          execv(cmds[0].tokens[0], cmds[0].tokens);
          kill(getpid(), SIGKILL);
        } else {
          pipe_pid_1 = temp_pid_1;
          // For more than one pipe
          for (i = 1; i < numPipes; ++i) {
            pid_t temp_pid_2 = fork();
            if (temp_pid_2 == -1) {
              fprintf(stderr, "Forker error.\n");
              kill(getpid(), SIGKILL);
            } else if (temp_pid_2 == 0) {
              close(STDIN_FILENO);
              dup(fdpipe[i - 1][0]);
              // close(fdpipe[i-1][0]);
              close(fdpipe[i - 1][1]);

              close(STDOUT_FILENO);
              dup(fdpipe[i][1]);

              execv(cmds[i].tokens[0], cmds[i].tokens);
              kill(getpid(), SIGKILL);
            } else {
              pipe_pid_2 = temp_pid_2;
            }
          }
          // Initial print for pipe background process
          if (bg_ptr->bg == 1) {
            if (numPipes == 1)
              printf("[%d]    [%d][%d]\n", bg_ptr->numProcess, getpid(),
                     pipe_pid_1);
            else if (numPipes == 2)
              printf("[%d]    [%d][%d][%d]\n", bg_ptr->numProcess, getpid(),
                     pipe_pid_1, pipe_pid_2);
          }

          wait(NULL);
          close(STDIN_FILENO);
          dup(fdpipe[numPipes - 1][0]);
          for (i = 0; i < numPipes; ++i) {
            close(fdpipe[i][0]);
            close(fdpipe[i][1]);
          }
          execv(cmds[i].tokens[0], cmds[i].tokens);
          kill(getpid(), SIGKILL);
        }
      }
      if (inputRedirect && outputRedirect) {
        close(STDOUT_FILENO);
        dup(fd1);
        close(fd1);

        close(STDIN_FILENO);
        dup(fd0);
        close(fd0);
      } else if (inputRedirect) {
        close(STDIN_FILENO);
        dup(fd0);
        close(fd0);

      } else if (outputRedirect) {
        close(STDOUT_FILENO);
        dup(fd1);
        close(fd1);
      }
      execv(instr_ptr->tokens[0], instr_ptr->tokens);
      printf("%s: Command not found\n", instr_ptr->tokens[0]);
      kill(getpid(), SIGKILL);

    } else {
      if (pipeRedirect) {
        for (i = 0; i < numPipes; ++i) {
          close(fdpipe[i][0]);
          close(fdpipe[i][1]);
          free(fdpipe[i]);
        }
        free(fdpipe);
      }
      if (inputRedirect)
        close(fd0);
      if (outputRedirect)
        close(fd1);

      if (bg_ptr->bg == 1) {
        bg_ptr->pids[bg_ptr->numProcess] = pid;
        if (!pipeRedirect) {
          printf("[%d]    [%d]\n", bg_ptr->numProcess,
                 bg_ptr->pids[bg_ptr->numProcess]);
        }
        usleep(1000); // Fixes output for instant commands; does it matter?
        bg_ptr->numProcess++;
      } else {
        while (wait(NULL) != pid)
          ;
      }

      bg_ptr->bg = 0;

      if (bg_ptr->numProcess > 0) {
        int processes = bg_ptr->numProcess;
        // Check each process to see if finished
        for (i = 0; i < processes; ++i) {
          if (waitpid(bg_ptr->pids[i], &status, WNOHANG) < 0) {
            printf("[%d]+    [%s]\n", i, bg_ptr->command[i]);
            bg_ptr->pids[i] = -1;
            memset(bg_ptr->command[i], '\0', 256);
            bg_ptr->numProcess--;
          }
        }
        // Move processes down queue
        for (i = 0; i < processes; ++i) {
          if (bg_ptr->pids[i] == -1) {
            int j;
            for (j = i; j < processes - 1; ++j) {
              bg_ptr->pids[j] = bg_ptr->pids[j + 1];
              strcpy(bg_ptr->command[j], bg_ptr->command[j + 1]);
              memset(bg_ptr->command[j + 1], '\0', 256);
            }
          }
        }
      }
    }
  }
}

void clearInstruction(instruction *instr_ptr) {
  int i;
  for (i = 0; i < instr_ptr->numTokens; i++)
    free(instr_ptr->tokens[i]);
  free(instr_ptr->tokens);
  instr_ptr->tokens = NULL;
  instr_ptr->numTokens = 0;
}

char *resolveShortcut(char *path) {
  int i;
  instruction instr;
  instr.tokens = NULL;
  instr.numTokens = 0;
  int start = 0; // saves location of / in path
  // skip first / in absolute path
  if (path[0] == '/')
    start = 1;

  for (i = start; i <= strlen(path); ++i) {
    // stop at every / found in path; save word between start and i as a token
    if (path[i] == '/' || i == strlen(path)) {
      char *temp = (char *)malloc((i - start + 1) * sizeof(char));
      int j;
      // extract letters into string
      for (j = 0; j < i - start; ++j)
        temp[j] = path[start + j];
      temp[i - start] = '\0';
      start = i + 1;
      if (strcmp(temp, "\0") != 0) {
        addToken(&instr, temp);
      }
      free(temp);
    }
  }
  addNull(&instr);

  int absolute = 0;
  char *cwd = (char *)malloc(256 * sizeof(char));
  if (path[0] == '/') {
    absolute = 1;
    cwd[0] = '/';
  } else {
    getcwd(cwd, 255);
  }
  // go token by token
  for (i = 0; i < instr.numTokens - 1; ++i) {
    // return to $HOME if ~ found as first token, else error
    if (strcmp(instr.tokens[i], "~\0") == 0) {
      if (i == 0) {
        memset(cwd, 0, 256);
        strcpy(cwd, getenv("HOME"));
      } else {
        fprintf(stderr,
                "Error: '~' can only be used at the start of the path\n");
        strcpy(cwd, " \0");
        return cwd;
      }
    }
    // move up a directory if possible when .. is found, else error
    else if (strcmp(instr.tokens[i], "..\0") == 0) {
      int j = strlen(cwd);
      if (j <= 1) {
        fprintf(stderr, "Error: Cannot go past root directory\n");
        strcpy(cwd, " \0");
        return cwd;
      }
      while (cwd[j] != '/') {
        cwd[j] = '\0';
        --j;
      }
      if (j != 0)
        cwd[strlen(cwd) - 1] = '\0';
    }
    // check if word is a file / directory
    else if (strcmp(instr.tokens[i], ".\0") != 0) {
      struct stat buffer;
      char *temp = (char *)malloc(256 * sizeof(char));
      strcpy(temp, cwd);
      if (absolute == 0) {
        strcat(temp, "/");
      } else {
        absolute = 0;
      }
      strcat(temp, instr.tokens[i]); // build path from cwd + / + token
      stat(temp, &buffer);
      if (S_ISDIR(buffer.st_mode)) {
        strcpy(cwd, temp);
      }
      // check if is file and if filename is last token
      else if (S_ISREG(buffer.st_mode)) {
        if (i == instr.numTokens - 2) {
          strcpy(cwd, temp);
        } else {
          fprintf(stderr,
                  "Error: Files can only appear at the end of a path\n");
          strcpy(cwd, " \0");
          return cwd;
        }
      }
      // misc token / unknown name
      else {
        fprintf(stderr, "Error: File / directory '%s' not found\n",
                instr.tokens[i]);
        strcpy(cwd, " \0");
        return cwd;
      }
      free(temp);
      buffer.st_mode = 0;
    }
    // printf("%s\n",cwd);
  }
  return cwd;
}

char *pathResolution(char *cmd) {
  char **paths_array; // array for possible paths
  int total_colon, path_length, counter, i;
  total_colon = 0;
  counter = 0;

  char *test_path = getenv("PATH");
  int test_path_length = strlen(test_path);
  char env_path[test_path_length];
  strcpy(env_path, test_path);

  path_length = strlen(env_path);
  // add up amount of : (paths)
  for (i = 0; i < path_length; i++) {
    if (env_path[i] == ':') {
      env_path[i] = '\0';
      total_colon++;
    }
  }
  // env_path = path with \0 instead of :
  if (total_colon == 0)
    return cmd;
  paths_array = malloc((total_colon + 1) * sizeof(*paths_array));
  // fill paths array
  paths_array[0] = env_path;
  for (i = 0; i < path_length; i++) {
    if (env_path[i] == '\0') {
      paths_array[++counter] = env_path + i + 1;
    }
  }

  // need to add cmd to the end of every path then check them if it exist: else
  // error what do i need instead of 400?
  char temp[2056];
  for (i = 0; i < total_colon; i++) {
    strcpy(temp, paths_array[i]);
    strcat(temp, "/");
    strcat(temp, cmd);
    if (access(temp, F_OK) != -1) {
      cmd = temp;
      free(paths_array);
      return cmd;
    }
  }
  free(paths_array);
  return cmd;
}
