/* NAME: Lukas Hanson
 * DATE: 2020/11/12
 * CLASS: CIS 415
 * PROJECT: Project 2 Part 2
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "MCP.h"

void script_print (pid_t* pid_ary, int size) {
  FILE* fout;
  fout = fopen("top_script.sh", "w");

  fprintf(fout, "#!/bin/bash\ntop");

  for (int i = 0; i < size; i++) {
    fprintf(fout, " -p %d", (int)(pid_ary[i]));
  }

  fprintf(fout, "\n");
  fclose(fout);
}

void signaller(pid_t* pid_array, int size, int signal) {
  sleep(2);
  for (int i = 0; i < size; i++) {
    printf("Parent process: %d - Sending signal: %d to child process: %d\n", getpid(), signal, pid_array[i]);
    kill(pid_array[i], signal);
  }
}

void scheduler(struct clist* clist) {
  int i, signal;
  pid_t* pid_array = (pid_t*) malloc(clist->cnum * sizeof(pid_t));
  pid_t child_id = 0;

  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGUSR1);
  sigprocmask(SIG_BLOCK, &sigset, NULL);

  for (i = 0; i < clist->cnum; i++) {
    child_id = fork();

    if (child_id < 0) {
      perror("Error creating child");
      free_clist(clist);
      free(pid_array);
      exit(EXIT_FAILURE);
    }

    else if (child_id == 0) {
      printf("Child process: %d - Waiting for %d...\n", getpid(), SIGUSR1);
      sigwait(&sigset, &signal);
      printf("Child process: %d - Recieved signal: %d - Running %s\n", getpid(), SIGUSR1, (clist->commands[i])->argv[0]);
      execvp((clist->commands[i])->argv[0], (clist->commands[i])->argv);
      perror("Error starting program");
      free_clist(clist);
      free(pid_array);
      exit(EXIT_FAILURE);
    }

    else {
      pid_array[i] = child_id;
    }
  }

  script_print(pid_array, clist->cnum);

  signaller(pid_array, clist->cnum, SIGUSR1);
  sleep(3);
  signaller(pid_array, clist->cnum, SIGSTOP);
  sleep(3);
  signaller(pid_array, clist->cnum, SIGCONT);
  sleep(3);
  signaller(pid_array, clist->cnum, SIGINT);

  for (i = 0; i < clist->cnum; i++) {
    wait(0);
  }

  free_clist(clist);
  free(pid_array);
}

struct clist* read_file(FILE* file) {
  size_t buffer = BUFSIZ;
  char* line = (char*) malloc(buffer * sizeof(char));
  char* token;

  char* filler = "filler";
  struct clist* clist = create_clist(filler, strlen(filler) + 1);

  int file_flag = 1;
  int line_flag;
  int counter = 0;

  while (getline(&line, &buffer, file) != -1) {
    line_flag = 1;
    token = strtok(line, " ");

    while (token != NULL) {
      token[strcspn(token, "\n")] = '\0';

      if (file_flag) {
        replace_command(clist, counter, token, strlen(token) + 1);
        file_flag = 0;
        line_flag = 0;
      }

      else {
        if (line_flag) {
          add_command(clist, token, strlen(token) + 1);
          line_flag = 0;
        }

        else {
          update_command(clist, counter, token, strlen(token) + 1);
        }
      }

      token = strtok(NULL, " ");
    }

    update_command(clist, counter, NULL, sizeof(NULL));
    counter++;
  }

  free(line);
  return clist;
}

int main(int argc, char** argv) {
  if ((argc != 2)) {
    printf("Invalid use of %s\n", argv[0]);
    return 0;
  }

  FILE* file = fopen(argv[1], "r");

  if (file == NULL) {
    printf("Invalid file %s\n", argv[1]);
    return 0;
  }

  struct clist* clist = read_file(file);
  fclose(file);

  scheduler(clist);

  return 0;
}
