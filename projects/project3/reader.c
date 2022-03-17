// PROJECT: File Interpreter
//
// AUTHOR: Lukas Hanson
//
// DATE: 2020/11/05
//
// CLASS: CIS 415
//
// DESCRIPTION:
// Definition and implementation of the clist and command structures for use
// in Project 3, as well as their requisite methods.

// Necessary libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "reader.h"

// Methods for the command structure
void destroy_command(command* command) {
  for (int i = 0; i < command->argc; i++) {
    free(command->argv[i]);
  }

  free(command->argv);
  free(command);
}

void print_command(command* command) {
  printf("Argument Count: %d |", command->argc);
  for (int i = 0; i < command->argc; i++) {
    printf(" %d: %s", i, command->argv[i]);
  }
  printf("\n");
}

void store_argv(command* command, int index, char* string, size_t size) {
  if (string == NULL) {
    command->argv[index] = NULL;
  }

  else {
    command->argv[index] = (char*) malloc(size * sizeof(char));
    strcpy(command->argv[index], string);
  }
}

void replace_argv(command* command, int index, char* string, size_t size) {
  store_argv(command, index, string, size);
}

void add_argv(command* command, char* string, size_t size) {
  command->argc++;
  command->argv = (char**) realloc(command->argv, command->argc * sizeof(char*));
  store_argv(command, command->argc - 1, string, size);
}

command* create_command(char* string, size_t size) {
  command* command = (struct command*) malloc(sizeof(struct command));
  command->argc = 1;
  command->argv = (char**) malloc(command->argc * sizeof(char*));
  store_argv(command, command->argc - 1, string, size);

  return command;
}

// Methods for the clist structure
void destroy_clist(clist* clist) {
  for (int i = 0; i < clist->cnum; i++) {
    destroy_command(clist->commands[i]);
  }

  free(clist->commands);
  free(clist);
}

void print_clist(clist* clist) {
  printf("Command Count: %d\n", clist->cnum);

  for (int i = 0; i < clist->cnum; i++) {
    printf("Command %d: | ", i);
    print_command(clist->commands[i]);
  }
}

void update_command(clist* clist, int index, char* string, size_t size) {
  add_argv(clist->commands[index], string, size);
}

void delete_command(clist* clist, int index) {
  destroy_command(clist->commands[index]);
  clist->cnum--;
}

void replace_command(clist* clist, int index, char* string, size_t size) {
  destroy_command(clist->commands[index]);
  clist->commands[index] = create_command(string, size);
}

void add_command(clist* clist, char* string, size_t size) {
  clist->cnum++;
  clist->commands = realloc(clist->commands, clist->cnum * sizeof(struct command*));
  clist->commands[clist->cnum - 1] = create_command(string, size);
}

clist* create_clist(char* string, size_t size) {
  clist* clist = (struct clist*) malloc(sizeof(struct clist));
  clist->cnum = 1;
  clist->commands = (struct command**) malloc(clist->cnum * sizeof(struct command*));
  clist->commands[clist->cnum - 1] = create_command(string, size);

  return clist;
}

// Methods for the job structure
job* create_job (clist* clist, int type) {
  job* job = (struct job*) malloc(sizeof(struct job));
  job->lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(job->lock, NULL);
  job->clist = clist;
  job->type = type;
  job->completed = 0;
  job->taken = 0;

  return job;
}

void print_job (job* job) {
  printf("JOB TYPE: %d | STATUS: %d\n", job->type, job->completed);
  print_clist(job->clist);
}

void destroy_job (job* job) {
  destroy_clist(job->clist);
  free(job->lock);
  free(job);
}

// Methods for the jlist structure
jlist* create_jlist (job* job) {
  jlist* jlist = (struct jlist*) malloc(sizeof(struct jlist));
  jlist->size = 1;
  jlist->jobs = (struct job**) malloc(jlist->size * sizeof(struct job*));
  jlist->jobs[0] = job;

  return jlist;
}

void print_jlist (jlist* jlist) {
  for (int i = 0; i < jlist->size; i++) {
    print_job(jlist->jobs[i]);
  }
}

void destroy_jlist (jlist* jlist) {
  for (int i = 0; i < jlist->size; i++) {
    destroy_job(jlist->jobs[i]);
  }

  free(jlist->jobs);
  free(jlist);
}

void add_job (jlist* jlist, job* job) {
  jlist->size++;
  jlist->jobs = (struct job**) realloc(jlist->jobs, jlist->size * sizeof(struct job*));
  jlist->jobs[jlist->size - 1] = job;
}

int jobs_left (jlist* jlist) {
  int jobs_left = jlist->size;
  for (int i = 0; i < jlist->size; i++) {
    pthread_mutex_lock(jlist->jobs[i]->lock);
    jobs_left -= jlist->jobs[i]->completed;
    pthread_mutex_unlock(jlist->jobs[i]->lock);
  }

  return jobs_left;
}

// Miscellaneous methods
clist* read_file (FILE* file) {
  size_t buffer = BUFSIZ;
  char* line = (char*) malloc(buffer * sizeof(char));
  char* token;

  char* filler = "filler";
  clist* clist = create_clist(filler, strlen(filler) + 1);

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

void remove_quotes (char* dst, char* src) {
  char c;

  while ((c = *src++) != '\0') {
    if (c != '"') {
      *dst++ = c;
    }
  }

  *dst = '\0';
}

char* trim_quotes (char* string) {
  char* new_string = (char*) malloc((strlen(string) + 1) * sizeof(char));

  remove_quotes(new_string, string);
  strcpy(string, new_string);

  free(new_string);

  return string;
}

void convert_html (char* input_filename, int num_queues) {
  FILE* input_file = fopen(input_filename, "r");

  char* output_filename = (char*) malloc(BUFSIZ * sizeof(char));
  sprintf(output_filename, "SUB:%ld.html", pthread_self());

  FILE* output_file = fopen(output_filename, "a");

  fprintf(output_file, "<!DOCTYPE html>\n<html>\n<head>\n<title>\n%s</title>\n\n<style>\n", output_filename);
  fprintf(output_file, "table, th, td {\n  border: 1px solid black;\n  border-collapse: collapse;\n}\n);th, td {\n  padding: 5px;\n}\nth {\n  text-align: left;\n}\n</style>\n</head>\n<body>\n");
  fprintf(output_file, "<h1>Subscriber: %ld </h1>\n", pthread_self());

  clist* clist = read_file(input_file);
  fclose(input_file);

  char** queues = (char**) malloc(num_queues * sizeof(char*));

  for (int i = 0; i < num_queues; i++) {
    queues[i] = (char*) malloc(BUFSIZ * sizeof(char));
    strcpy(queues[i], "FILLER");
  }

  int index = 0;

  for (int i = 0; i < clist->cnum; i++) {
    if (if_in(queues, num_queues, clist->commands[i]->argv[0]) == 0) {
      strcpy(queues[index], clist->commands[i]->argv[0]);
      index++;
    }
  }

  for (int i = 0; i < num_queues; i++) {
    if (strcmp(queues[i], "FILLER") != 0) {
      fprintf(output_file, "<h2>Topic Name: %s</h2>\n\n<table style=\"width:100%%\" align=\"middle\">\n  <tr>\n    <th>CAPTION</th>\n    <th>PHOTO-URL</th>\n  </tr>", queues[i]);

      for (int j = 0; j < clist->cnum; j++) {
        if (strcmp(queues[i], clist->commands[j]->argv[0]) == 0) {
          fprintf(output_file, "<tr><th>");

          for (int k = 2; k < clist->commands[j]->argc - 1; k++) {
            fprintf(output_file, "%s ", clist->commands[j]->argv[k]);
          }

          fprintf(output_file, "</th>\n");

          fprintf(output_file, "<th>%s</th>\n</tr>\n\n", clist->commands[j]->argv[1]);
        }
      }

      fprintf(output_file, "</table>\n");
    }
  }

  fprintf(output_file, "</body>\n</html>");

  for (int i = 0; i < num_queues; i++) {
    free(queues[i]);
  }

  free(queues);
  free(output_filename);
  destroy_clist(clist);
  fclose(output_file);
}

int if_in (char** strings, int num_strings, char* string) {
  for (int i = 0; i < num_strings; i++) {
    if (strcmp(strings[i], string) == 0) {
      return 1;
    }
  }

  return 0;
}
