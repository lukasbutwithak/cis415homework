#ifndef STORE_H
#define STORE_H

// Structure declarations
typedef struct entry {
  int number;
  struct timeval* timestamp;
  char* url;
  char* caption;
} entry;

typedef struct queue {
  int id;
  char* name;
  struct entry** entries;
  int* initialized;
  pthread_mutex_t* lock;
  int head;
  int tail;
  int total;
  int length;
} queue;

typedef struct store {
  struct queue** queues;
  int* initialized;
  int index;
  int length;
  int delta;
} store;

// Methods for the entry structure
entry* create_entry (char* url, char* caption, int url_size, int cap_size);

void print_entry (entry* entry);

void file_entry (entry* entry, char* name, FILE* file);

void copy_entry (entry* entry_dest, entry* entry_source);

void destroy_entry (entry* entry);

// Methods for the queue structure
queue* create_queue (int id, char* name, int name_size, int length);

void print_queue (queue* queue);

void destroy_queue (queue* queue);

int is_full(queue* queue);

int is_empty(queue* queue);

int dequeue(queue* queue, entry* entry);

int enqueue(queue* queue, entry* entry);

int get_entry(queue* queue, entry* entry, int last_entry);

// Methods for the store structure
store* create_store (int length);

void print_store (store* store);

void destroy_store (store* store);

void change_delta (store* store, int delta);

int add_queue (store* store, int id, char* name, int name_size, int length);

int find_queue (store* store, int id);

#endif
