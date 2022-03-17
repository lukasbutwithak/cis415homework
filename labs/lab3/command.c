/*
* Description: Using Lab 1 and 2 to implement the lfcat function
*
* Author: Lukas Hanson
*
* Date: 2020/10/16
*
* Notes: None
*/

/*-------------------------Preprocessor Directives---------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
/*---------------------------------------------------------------------------*/
/*-----------------------------Program Main----------------------------------*/
void lfcat() {
  size_t n = BUFSIZ;
  char* line = (char *) malloc(n * sizeof(char));
  char* cwd_name = (char *) malloc(n * sizeof(char));
  FILE* file;

  getcwd(cwd_name, n);

	DIR* cwd = opendir(cwd_name);
	struct dirent* entry;

	while ((entry = readdir(cwd)) != NULL) {
    if (strncmp(entry->d_name, "output.txt", 10) == 0 ||
        strncmp(entry->d_name, "a.out", 5) == 0 ||
        strncmp(entry->d_name, "main.c", 6) == 0 ||
        strncmp(entry->d_name, "command.c", 9) == 0 ||
        strncmp(entry->d_name, "command.h", 9) == 0 ||
        strncmp(entry->d_name, "makefile", 8) == 0 ||
        strncmp(entry->d_name, ".", 1) == 0 ||
        strncmp(entry->d_name, "..", 2) == 0) {
      continue;
    }

    write(1, "File: ", 6);
    write(1, entry->d_name, strlen(entry->d_name));
    write(1, "\n", 1);

    file = fopen(entry->d_name, "r");

    while (getline(&line, &n, file) != -1) {
      write(1, line, strlen(line));
    }

    write(1, "\n", 1);

    fclose(file);

    for (int i = 0; i < 80; i++) {
      write(1, "-", 1);
    }

    write(1, "\n", 1);
	}

  closedir(cwd);
  free(cwd_name);
  free(line);
}
/*---------------------------------------------------------------------------*/
