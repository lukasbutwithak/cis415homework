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
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Function for the ls command. */
void listDir() {
  size_t n = BUFSIZ;
  char* cwd_name = (char *) malloc(n * sizeof(char));

  getcwd(cwd_name, n);
  DIR* cwd = opendir(cwd_name);
  struct dirent* entry;

  while ((entry = readdir(cwd)) != NULL) {
      write(1, entry->d_name, strlen(entry->d_name));
      write(1, " ", 1);
  }

  write(1, "\n", 1);

  free(cwd_name);
  closedir(cwd);
}

/* Function for the pwd command. */
void showCurrentDir() {
  size_t n = BUFSIZ;
  char* cwd_name = (char *) malloc(n * sizeof(char));

  getcwd(cwd_name, n);

  write(1, cwd_name, strlen(cwd_name));
  write(1, "\n", 1);

  free(cwd_name);
}

/* Function for the mkdir command. */
void makeDir(char* dirname) {
  mkdir(dirname, 0777);
}

/* Function for the cd command. */
void changeDir(char* dirname) {
  chdir(dirname);
}

/* Function for the cp command. */
void copyFile(char* filename, char* destination) {
  size_t n = BUFSIZ;
  char* line = (char *) malloc(n * sizeof(char));
  char* cwd_name = (char *) malloc(n * sizeof(char));

  getcwd(cwd_name, n);

  int sfd = open(filename, O_RDONLY, 0);

  changeDir(destination);

  filename = strrchr(filename, '/');
  filename = strtok(filename, "/");

  int dfd = creat(filename, 0777);

  if ((sfd == -1) || (dfd == -1)) {
    write(1, "Error! Invalid file descriptor(s)\n", 34);
    close(sfd);
    close(dfd);
    free(line);
    free(cwd_name);
    return;
  }

  while (read(sfd, line, n) != 0) {
    strcat(line, "\0");
    write(dfd, line, strlen(line));
  }

  changeDir(cwd_name);

  close(sfd);
  close(dfd);
  free(line);
  free(cwd_name);
}

/* Functino for the rm command. */
void deleteFile(char* filename) {
  if ((strcmp(filename, "command.c") != 0) ||
      (strcmp(filename, "main.c") != 0) ||
      (strcmp(filename, "command.h") != 0)) {
    unlink(filename);
  }
}

/* Function for the mv command. */
void moveFile(char* filename, char* destination) {
  size_t n = BUFSIZ;
  char* line = (char *) malloc(n * sizeof(char));

  int sfd = open(filename, O_RDONLY, 0);
  int dfd = creat(destination, 0777);

  if ((sfd == -1) || (dfd == -1)) {
    write(1, "Error! Invalid file descriptor(s)\n", 34);
    close(sfd);
    close(dfd);
    free(line);
    return;
  }

  while (read(sfd, line, n) != 0) {
    strcat(line, "\0");
    write(dfd, line, strlen(line));
  }

  deleteFile(filename);

  close(sfd);
  close(dfd);
  free(line);
}

/* Function for the cat command. */
void displayFile(char* filename) {
  size_t n = BUFSIZ;
  char* line = (char *) malloc(n * sizeof(char));

  int fd = open(filename, O_RDONLY, 0);

  while (read(fd, line, n) != 0) {
    strcat(line, "\0");
    write(1, line, strlen(line));
  }

  close(fd);
  free(line);
}
