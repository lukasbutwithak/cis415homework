#ifndef READER_H
#define READER_H

// Structure declarations
typedef struct command {
  int argc;
  char** argv;
} command;

typedef struct clist {
  int cnum;
  struct command** commands;
} clist;

typedef struct job {
  struct clist* clist;
  pthread_mutex_t* lock;
  int type;
  int completed;
  int taken;
} job;

typedef struct jlist {
  struct job** jobs;
  int size;
} jlist;

// Methods for the command structure
void destroy_command (command* command);

void print_command (command* command);

void store_argv (command* command, int index, char* string, size_t size);

void replace_argv (command* command, int index, char* string, size_t size);

void add_argv (command* command, char* string, size_t size);

command* create_command (char* string, size_t size);

// Methods for the clist structure
void destroy_clist (clist* clist);

void print_clist (clist* clist);

void update_command (clist* clist, int index, char* string, size_t size);

void delete_command (clist* clist, int index);

void replace_command (clist* clist, int index, char* string, size_t size);

void add_command (clist* clist, char* string, size_t size);

clist* create_clist (char* string, size_t size);

// Methods for the job structure
job* create_job (clist* clist, int type);

void print_job (job* job);

void destroy_job (job* job);

// Methods for the jlist structures
jlist* create_jlist (job* job);

void print_jlist (jlist* jlist);

void destroy_jlist (jlist* jlist);

void add_job (jlist* jlist, job* job);

int jobs_left (jlist* jlist);

// Miscellaneous methods
clist* read_file (FILE* file);

void remove_quotes (char* dst, char* src);

char* trim_quotes (char* string);

void convert_html (char* input_filename, int num_queues);

int if_in (char** strings, int num_strings, char* string);

#endif
