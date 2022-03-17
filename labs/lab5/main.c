/* NAME: Lukas Hanson
 *
 * CLASS: CIS 415
 *
 * DATE: 2020/11/06
 *
 * PROJECT: Lab 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void signaller(pid_t* pid_array, int size, int signal) {
  sleep(2);
  for (int i = 0; i < size; i++) {
    printf("Parent process: %d - Sending signal: %d to child process: %d\n",
          getpid(), signal, pid_array[i]);
    kill(pid_array[i], signal);
  }
}

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

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Error! Invalid parameters(s)\n");
    return 0;
  }

  int i, n, status, signal;
  char* ptr;

  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGUSR1);
  sigprocmask(SIG_BLOCK, &sigset, NULL);

  char* command[4];
  command[0] = "./iobound";
  command[1] = "-seconds";
  command[2] = "5";
  command[3] = NULL;

  n = strtol(argv[1], &ptr, 10);
  pid_t* pid_array = (pid_t*) malloc(n * sizeof(pid_t));
  pid_t child_id = 0;

  for (i = 0; i < n; i++) {
    child_id = fork();
    if (child_id == 0) {
      printf("Child process: %d - Waiting for SIGUSR1...\n", getpid());
      sigwait(&sigset, &signal);
      printf("Child process: %d - Recieved signal: SIGUSR1 - Calling exec().\n", getpid());
      execvp(command[0], command);
    }

    else {
      pid_array[i] = child_id;
    }
  }

  script_print(pid_array, n);

  signaller(pid_array, n, SIGUSR1);
  sleep(3);
  signaller(pid_array, n, SIGSTOP);
  sleep(3);
  signaller(pid_array, n, SIGCONT);
  sleep(3);
  signaller(pid_array, n, SIGINT);

  for (i = 0; i < n; i++) {
    wait(0);
  }

  free(pid_array);
  return 1;
}
