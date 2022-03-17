/* NAME: Lukas Hanson
 *
 * CLASS: CIS 415
 *
 * DATE: 2020/10/22
 *
 * PROJECT: 1
 *
 * DESCRIPTION: Implementing a pseudo-shell that can handle eight commands
 *              by using system calls.
 */

/* Including the necessary libraries. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "command.h"

/* Creating a global list of valid commands to accept. */
const char* commands[10] = {"ls", "pwd", "mkdir",
                          "cd", "cp", "mv", "rm",
                          "cat", "exit", ";"};

/* Defining a command structure for future use. */
struct command {
  int command;
  char* parameter1;
  char* parameter2;
};

/* Creating a function to run commands. */
void run_command(struct command** action) {
  if ((*action)->parameter1[strlen((*action)->parameter1) - 1] == '\n') {
    (*action)->parameter1[strlen((*action)->parameter1) - 1] = '\0';
  }

  if ((*action)->parameter2[strlen((*action)->parameter2) - 1] == '\n') {
    (*action)->parameter2[strlen((*action)->parameter2) - 1] = '\0';
  }

  if ((*action)->command == 0) {
    listDir();
  }

  else if ((*action)->command == 1) {
    showCurrentDir();
  }

  else if ((*action)->command == 2) {
    makeDir((*action)->parameter1);
  }

  else if ((*action)->command == 3) {
    changeDir((*action)->parameter1);
  }

  else if ((*action)->command == 4) {
    copyFile((*action)->parameter1, (*action)->parameter2);
  }

  else if ((*action)->command == 5) {
    moveFile((*action)->parameter1, (*action)->parameter2);
  }

  else if ((*action)->command == 6) {
    deleteFile((*action)->parameter1);
  }

  else if ((*action)->command == 7) {
    displayFile((*action)->parameter1);
  }
}

/* Creating a function to reset the action after running a command. */
void reset_command(struct command* action) {
  strcpy(action->parameter1, "empty");
  strcpy(action->parameter2, "empty");
  action->command = -1;
}

/* Creating a function to free the commmand struct. */
void free_command(struct command* action) {
  if (action != NULL) {
    free(action->parameter1);
    free(action->parameter2);
    free(action);
  }
}

/* Create a function to built a command. */
struct command* malloc_command(size_t n) {
  struct command* action = (struct command *) malloc(sizeof(struct command));
  if (action != NULL) {
    action->parameter1 = (char *) malloc(n * sizeof(char));
    action->parameter2 = (char *) malloc(n * sizeof(char));

    if ((action->parameter1 != NULL) && (action->parameter2 != NULL)) {
      action->command = -1;
      strcpy(action->parameter1, "empty");
      strcpy(action->parameter2, "empty");

      return action;
    }

    else {
      free(action);
    }
  }

  return NULL;
}

/* Creating a function to return the number of parameters a command needs. */
int get_parameters(int command) {
  if ((command == 4) || (command == 5)) {
    return 2;
  }

  else if ((command == 2) || (command == 3) ||
          (command == 6) || (command == 7)) {
    return 1;
    }

  else {
    return 0;
  }
}

/* Creating a function to check which command is in the token. */
int check_command(char* token) {
  /* If the command is found and matches the same length, return the command's
     index in the global string array. */
  for (int i = 0; i < 10; i++) {
    if (strncmp(token, commands[i], strlen(commands[i])) == 0) {
      return i;
    }
  }

  return -1;
}

void parse_tokens(FILE* infile, FILE* outfile, int mode) {
  /* Prepare variables and allocate memory for the line buffer. */
  size_t n = BUFSIZ;
  char* line = (char *) malloc(n * sizeof(char));
  char* token = NULL;

  /* Exit flag determines whether the loop breaks. Command is used to store
     the index of the command found in a token. Search is a flag used to
     determine what the parser is looking for. Zero is looking for a command.
     One is looking for parameters. Two is looking for a semicolon. Parameters
     tracks how many parameters are needed. */
  int exit = 0;
  int command, search, parameters;

  /* Instantiate a command structure as an action to be taken. */
  struct command* action = malloc_command(n);

  /* Loop until end of file or exit command. */
  while(1) {
    /* If in interactive mode, print command prompt. */
    if (mode) {
      fprintf(outfile, ">>> ");
    }

    /* If end of file is reached, break the loop. */
    if (getline(&line, &n, infile) == -1) {
      break;
    }

    /* Always start each line looking for a command. */
    search = 0;

    /* Break up the line by spaces and grab the first token. */
    token = strtok(line, " ");

    while (token != NULL) {
      /* If the token is empty, just skip it. */
      if (strncmp(token, "\n", 1) == 0) {
        token = strtok(NULL, " ");
        continue;
      }

      /* Check for which command was in the token. */
      command = check_command(token);

      /* Since the exit command will always be on its own, just quit. */
      if (command == 8) {
        exit = 1;
        break;
      }

      /* If the parser is searching for a command, make sure it's valid.
         If the command isn't valid, throw an error. Otherwise move to
         parameter search mode and store the command in action-> */
      if (search == 0) {
        if ((command == -1) || (command == 9)) {
          fprintf(outfile, "Error! Invalid command: %s\n", token);
          break;
        }

        else {
          action->command = command;
          parameters = get_parameters(command);
          if (parameters == 0) {
            search = 2;
          }

          else {
            search = 1;
          }
        }
      }

      /* If the parser is searching for parameters, behavior depends on the
         number of parametrs being sought. */
      else if (search == 1) {
        if (command == 9) {
          fprintf(outfile, "Error! Missing parameter(s) for command: %s\n",
                  commands[action->command]);
          reset_command(action);
          break;
        }

        else if (command != -1) {
          fprintf(outfile, "Error! Incorrect syntax. No control code found\n");
          reset_command(action);
          break;
        }

        else {
          if (strncmp(action->parameter1, "empty", 5) == 0) {
            if (parameters == 1) {
              search = 2;
            }

            strcpy(action->parameter1, token);
          }

          else {
            if (parameters == 2) {
              strcpy(action->parameter2, token);
              search = 2;
            }
          }
        }
      }

      /* If the parser is looking for a semiclon, then any other token will
         trigger an error. Otherwise, pass action to run command. */
      else {
        if (command == 9) {
          run_command(&action);
          reset_command(action);
          search = 0;
        }

        else {
          fprintf(outfile, "Error! Incorrect syntax. No control code found\n");
          reset_command(action);
          break;
        }
      }

      /* Grab the next token in the line. */
      token = strtok(NULL, " ");
    }

    /* If the exit command was recieved, break the loop. */
    if (exit) {
      break;
    }

    /* The only instance in which a command doesn't need the semicolon is when
       it is the last file in a line. After token is NULL, check to see if there
       is a valid command in action. If so, run it. */
    if (action->command != -1) {
      if (((parameters == 2) &&
          (strncmp(action->parameter1, "empty", 5) != 0) &&
          (strncmp(action->parameter2, "empty", 5) != 0)) ||
          ((parameters == 1) &&
          (strncmp(action->parameter1, "empty", 5) != 0)) ||
          (parameters == 0)) {
        run_command(&action);
        reset_command(action);
      }

      else {
        fprintf(outfile, "Error! Missing paramter(s) for %s\n",
                commands[action->command]);
        reset_command(action);
        break;
      }
    }
  }

  /* Free the allocated line buffers. */
  free(line);
  free_command(action);
}

/* Determines if the function was called correctly and calls parse_tokens(3). */
int main(int argc, char** argv) {
  /* Default state is interactive mode. Initialize variables as such. */
  FILE* infile = stdin;
  FILE* outfile = stdout;
  int mode = 1;

  /* If the pseudo-shell has been called with a number of parameters that isn't
     one or three, then it cannot possible be used correctly. */
  if ((argc >= 4) || (argc == 2)) {
    fprintf(outfile, "Invalid use of %s\n", argv[0]);
    return 0;
  }

  /* If the pseudo-shell has been called with three parameters, check to see if
     they're correct. If they are, switch to file mode. Return an error
     otherwise. */
  else if (argc == 3) {
    infile = fopen(argv[2], "r");

    if ((strncmp(argv[1], "-f", 2) == 0) && (infile != NULL)) {
      /* If all the requirements are met for file mode, change outfile and the
         mode so that the parser doesn't wait for the exit commmand. */
      freopen("output.txt", "w", outfile);
      mode = 0;
    }

    else {
      fprintf(outfile, "Invalid use of %s\n", argv[0]);
      return 0;
    }
  }

  /* After determining which mode and files to use, parse lines from input. */
  parse_tokens(infile, outfile, mode);

  /* If in file mode, then close the files used. */
  if (!mode) {
    fclose(infile);
    fclose(outfile);
  }

  return 1;
}
