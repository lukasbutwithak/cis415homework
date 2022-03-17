/* NAME: Lukas Hanson
 * DATE: 2020/11/12
 * CLASS: CIS 415
 * PROJECT: Project 2 Header
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct clist {
  int cnum;
  struct command** commands;
};

struct command {
  int argc;
  char** argv;
};

/* Commands for creating, freeing, and managing the command struct. */
void free_command(struct command* command) {
  for (int i = 0; i < command->argc; i++) {
    free(command->argv[i]);
  }

  free(command->argv);
  free(command);
}

void print_command(struct command* command) {
  printf("Argument Count: %d |", command->argc);
  for (int i = 0; i < command->argc; i++) {
    printf(" %d: %s", i, command->argv[i]);
  }
  printf("\n");
}

void store_argv(struct command* command, int index, char* string, size_t size) {
  if (string == NULL) {
    command->argv[index] = NULL;
  }

  else {
    command->argv[index] = (char*) malloc(size * sizeof(char));
    strcpy(command->argv[index], string);
  }
}

void replace_argv(struct command* command, int index, char* string, size_t size) {
  store_argv(command, index, string, size);
}

void add_argv(struct command* command, char* string, size_t size) {
  command->argc++;
  command->argv = (char**) realloc(command->argv, command->argc * sizeof(char*));
  store_argv(command, command->argc - 1, string, size);
}

struct command* create_command(char* string, size_t size) {
  struct command* command = (struct command*) malloc(sizeof(struct command));
  command->argc = 1;
  command->argv = (char**) malloc(command->argc * sizeof(char*));
  store_argv(command, command->argc - 1, string, size);

  return command;
}

/* Commands for creating, freeing and managing the clist struct. */
void free_clist(struct clist* clist) {
  for (int i = 0; i < clist->cnum; i++) {
    free_command(clist->commands[i]);
  }

  free(clist->commands);
  free(clist);
}

void print_clist(struct clist* clist) {
  printf("Command Count: %d\n", clist->cnum);
  for (int i = 0; i < clist->cnum; i++) {
    printf("Command %d: | ", i);
    print_command(clist->commands[i]);
  }
}

void update_command(struct clist* clist, int index, char* string, size_t size) {
  add_argv(clist->commands[index], string, size);
}

void delete_command(struct clist* clist, int index) {
  free_command(clist->commands[index]);
  clist->cnum--;
}

void replace_command(struct clist* clist, int index, char* string, size_t size) {
  free_command(clist->commands[index]);
  clist->commands[index] = create_command(string, size);
}

void add_command(struct clist* clist, char* string, size_t size) {
  clist->cnum++;
  clist->commands = realloc(clist->commands, clist->cnum * sizeof(struct command*));
  clist->commands[clist->cnum - 1] = create_command(string, size);
}

struct clist* create_clist(char* string, size_t size) {
  struct clist* clist = (struct clist*) malloc(sizeof(struct clist));
  clist->cnum = 1;
  clist->commands = (struct command**) malloc(clist->cnum * sizeof(struct command*));
  clist->commands[clist->cnum - 1] = create_command(string, size);

  return clist;
}
