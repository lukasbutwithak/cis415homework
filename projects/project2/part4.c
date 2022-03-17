/* NAME: Lukas Hanson
 * DATE: 2020/11/12
 * CLASS: CIS 415
 * PROJECT: Project 2 Part 3
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
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

void signal_all(pid_t* pid_array, int size, int signal) {
  for (int i = 0; i < size; i++) {
    printf("Parent process: %d - Sending signal: %d to child process: %d\n", getpid(), signal, pid_array[i]);
    kill(pid_array[i], signal);
  }
}

void signal_one(pid_t* pid_array, int index, int signal) {
  printf("Parent process: %d - Sending signal: %d to child process: %d\n", getpid(), signal, pid_array[index]);
  kill(pid_array[index], signal);
}

void print_process(pid_t process) {
  size_t buffer = BUFSIZ;
  char* filename = (char*) malloc(buffer * sizeof(char));

  sprintf(filename, "/proc/%d/stat", process);

  FILE* file = fopen(filename, "r");

  int unused;
  unsigned int unusued2;
  unsigned long int unused3;
  unsigned long int uruntime;
  unsigned long int kruntime;
  char* exec = (char*) malloc(buffer * sizeof(char));
  char state;
  pid_t pid;
  pid_t ppid;

  fscanf(file, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu", &pid, exec, &state, &ppid, &unused, &unused, &unused, &unused, &unusued2, &unused3, &unused3, &unused3, &unused3, &uruntime, &kruntime);

  printf("Process Information: PID: %d | Parent PID: %d | Executing: %s\n                     State: %c | User Runtime: %lu | Kernel Runtime: %lu\n", pid, ppid, exec, state, uruntime, kruntime);

  fclose(file);
  free(filename);
  free(exec);
}

void scheduler(struct clist* clist) {
  int i, j, signalhold, status, flag, sum;
  pid_t* pid_array = (pid_t*) malloc(clist->cnum * sizeof(pid_t));
  int* death_array = (int*) malloc(clist->cnum *sizeof(int));
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
      free(death_array);
      exit(EXIT_FAILURE);
    }

    else if (child_id == 0) {
      /*printf("Child process: %d - Waiting for %d...\n", getpid(), SIGUSR1);*/
      sigwait(&sigset, &signalhold);
      printf("Child process: %d - Recieved signal: %d - Starting %s\n", getpid(), SIGUSR1, (clist->commands[i])->argv[0]);
      execvp((clist->commands[i])->argv[0], (clist->commands[i])->argv);
      perror("Error starting program");
      free_clist(clist);
      free(pid_array);
      free(death_array);
      exit(EXIT_FAILURE);
    }

    else {
      pid_array[i] = child_id;
      death_array[i] = 0;
    }
  }

  script_print(pid_array, clist->cnum);

  signal_all(pid_array, clist->cnum, SIGUSR1);
  signal_all(pid_array, clist->cnum, SIGSTOP);

  i = 0;
  flag = 1;

  while (flag) {
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    if (death_array[i] == 0) {
      printf("================================================================================\n");
      printf("BEGIN TIME QUANTUM\n");
      alarm(2);
      signal_one(pid_array, i, SIGCONT);

      print_process(pid_array[i]);

      if ((kill(pid_array[i], 0) == -1) || errno == ESRCH) {
        death_array[i] = 1;
      }

      sigwait(&sigset, &signalhold);
      printf("END TIME QUANTUM\n");
      signal_one(pid_array, i, SIGSTOP);

      waitpid(pid_array[i], &status, WUNTRACED);

      if ((WIFEXITED(status) != 0) || (WIFSTOPPED(status) == 0)) {
        death_array[i] = 1;
      }
    }

    else {
      sum = 0;

      for (j = 0; j < clist->cnum; j++) {
        sum += death_array[j];
      }

      if (sum == clist->cnum) {
        flag = 0;
      }
    }

    i++;
    i = i % clist->cnum;
  }

  free_clist(clist);
  free(pid_array);
  free(death_array);
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
