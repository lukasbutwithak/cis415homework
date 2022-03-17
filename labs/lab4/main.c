/* NAME: Lukas Hanson
 *
 * CLASS: CIS 415
 *
 * DATE: 2020/10/30
 *
 * PROJECT: Lab 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

  int i, n, status;
  char* ptr;

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
      execvp(command[0], command);
    }

    else {
      pid_array[i] = child_id;
    }
  }

  script_print(pid_array, n);

  for (i = 0; i < n; i++) {
    wait(0);
  }

  free(pid_array);
  return 1;
}
