// PROJECT: Topic Store
//
// AUTHOR: Lukas Hanson
//
// DATE: 2020/11/05
//
// CLASS: CIS 415
//
// DESCRIPTION:
// Definition and implementation of the entry, queue, and
// store structures for use in Project 3, as well as their requisite
// methods.

// Necessary libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "store.h"

// Methods for the entry structure
entry* create_entry (char* url, char* caption, int url_size, int cap_size) {
  entry* entry = (struct entry*) malloc(sizeof(struct entry));
  entry->timestamp = (struct timeval*) malloc(sizeof(struct timeval));
  entry->url = (char*) malloc(url_size * sizeof(char));
  entry->caption = (char*) malloc(cap_size * sizeof(char));

  entry->number = -1;
  gettimeofday(entry->timestamp, NULL);
  strcpy(entry->url, url);
  strcpy(entry->caption, caption);

  return entry;
}

void print_entry (entry* entry) {
  char time_string[64];
  struct tm* time = localtime(&entry->timestamp->tv_sec);

  strftime(time_string, sizeof(time_string), "%Y/%m/%d %H:%M:%S", time);

  printf("--------------------------------------------------------------------------------\n");
  printf("Entry #%03d | Published on %s.%06ld\n\nURL: %s\n\nCaption: %s\n", entry->number, time_string, entry->timestamp->tv_usec, entry->url, entry->caption);
  printf("--------------------------------------------------------------------------------\n\n");
}

void file_entry (entry* entry, char* name, FILE* file) {
  fprintf(file, "%s %s %s\n", name, entry->url, entry->caption);
}

void copy_entry (entry* entry_dest, entry* entry_source) {
  entry_dest->number = entry_source->number;
  entry_dest->timestamp->tv_sec = entry_source->timestamp->tv_sec;
  entry_dest->timestamp->tv_usec = entry_source->timestamp->tv_usec;

  strcpy(entry_dest->url, entry_source->url);
  strcpy(entry_dest->caption, entry_source->caption);
}

void destroy_entry (entry* entry) {
  free(entry->timestamp);
  free(entry->url);
  free(entry->caption);
  free(entry);
}

// Methods for the queue structure
queue* create_queue (int id, char* name, int name_size, int length) {
  queue* queue = (struct queue*) malloc(sizeof(struct queue));
  queue->name = (char*) malloc(name_size * sizeof(char));
  queue->entries = (struct entry**) malloc(length * sizeof(struct entry*));
  queue->initialized = (int*) malloc(length * sizeof(int));
  queue->lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));

  queue->id = id;
  strcpy(queue->name, name);

  for (int i = 0; i < length; i++) {
    queue->initialized[i] = 0;
  }

  pthread_mutex_init(queue->lock, NULL);
  queue->head = -1;
  queue->tail = -1;
  queue->total = 0;
  queue->length = length;

  return queue;
}

void print_queue (queue* queue) {
  printf("================================================================================\n");
  printf("Queue #%03d | %s\n\n", queue->id, queue->name);

  for (int i = 0; i < queue->length; i++) {
    if (queue->initialized[i]) {
      print_entry(queue->entries[i]);
    }
  }

  printf("================================================================================\n\n");
}

void destroy_queue (queue* queue) {
  free(queue->name);

  for (int i = 0; i < queue->length; i++) {
    if (queue->initialized[i]) {
      destroy_entry(queue->entries[i]);
    }
  }

  free(queue->entries);
  free(queue->initialized);
  pthread_mutex_destroy(queue->lock);
  free(queue->lock);
  free(queue);
}

int is_full(queue* queue) {
  pthread_mutex_lock(queue->lock);
  if ((queue->head == queue->tail + 1) || ((queue->head == 0) && (queue->tail == queue->length - 1))) {
    pthread_mutex_unlock(queue->lock);
    return 1;
  }

  else {
    pthread_mutex_unlock(queue->lock);
    return 0;
  }
}

int is_empty(queue* queue) {
  pthread_mutex_lock(queue->lock);
  if (queue->head == -1) {
    pthread_mutex_unlock(queue->lock);
    return 1;
  }

  else {
    pthread_mutex_unlock(queue->lock);
    return 0;
  }
}

int dequeue(queue* queue, entry* entry) {
  if (is_empty(queue)) {
    return 0;
  }

  else {
    pthread_mutex_lock(queue->lock);

    copy_entry(entry, queue->entries[queue->head]);
    destroy_entry(queue->entries[queue->head]);
    queue->initialized[queue->head] = 0;

    if (queue->head == queue->tail) {
      queue->head = -1;
      queue->tail = -1;
    }

    else {
      queue->head = (queue->head + 1) % queue->length;
    }

    pthread_mutex_unlock(queue->lock);

    return 1;
  }
}

int enqueue(queue* queue, entry* entry) {
  if (is_full(queue)) {
    return 0;
  }

  else {
    pthread_mutex_lock(queue->lock);

    if (queue->head == -1) {
      queue->head = 0;
    }

    struct entry* new_entry = create_entry(entry->url, entry->caption, strlen(entry->url) + 1, strlen(entry->caption) + 1);
    queue->tail = (queue->tail + 1) % queue->length;
    new_entry->number = queue->total;
    gettimeofday(new_entry->timestamp, NULL);
    queue->entries[queue->tail] = new_entry;
    queue->initialized[queue->tail] = 1;
    queue->total++;

    pthread_mutex_unlock(queue->lock);

    return 1;
  }
}

int get_entry(queue* queue, entry* entry, int last_entry) {
  int all_less = 1;
  int oldest = -1;
  int oldest_index = -1;

  if (is_empty(queue)) {
    return 0;
  }

  else {
    pthread_mutex_lock(queue->lock);

    for (int i = 0; i < queue->length; i++) {
      if (queue->initialized[i]) {
        if ((queue->entries[i]->number < oldest) || oldest == -1) {
          oldest = queue->entries[i]->number;
          oldest_index = i;
        }

        if (queue->entries[i]->number == last_entry + 1) {
          copy_entry(entry, queue->entries[i]);

          pthread_mutex_unlock(queue->lock);

          return 1;
        }

        else if (queue->entries[i]->number > last_entry + 1) {
          all_less = 0;
        }
      }
    }

    if (all_less) {
      pthread_mutex_unlock(queue->lock);

      return 0;
    }

    if (oldest > last_entry + 1) {
      copy_entry(entry, queue->entries[oldest_index]);

      pthread_mutex_unlock(queue->lock);

      return oldest;
    }

    pthread_mutex_unlock(queue->lock);

    return 0;
  }
}

// Methods for the store structure
store* create_store (int length) {
  store* store = (struct store*) malloc(sizeof(struct store));
  store->queues = (struct queue**) malloc(length * sizeof(struct queue*));
  store->initialized = (int*) malloc(length * sizeof(int));

  for (int i = 0; i < length; i++) {
    store->initialized[i] = 0;
  }

  store->index = 0;
  store->length = length;
  store->delta = 25;

  return store;
}

void print_store (store* store) {
  printf("≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡\n");
  printf("STORE DATA: DELTA: %03d | MAXIMUM LENGTH: %03d\n\n", store->delta, store->length);
  for (int i = 0; i < store->length; i++) {
    if (store->initialized[i]) {
      print_queue(store->queues[i]);
    }
  }

  printf("≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡≡\n");
}

void destroy_store (store* store) {
  for (int i = 0; i < store->length; i++) {
    if (store->initialized[i]) {
      destroy_queue(store->queues[i]);
    }
  }

  free(store->queues);
  free(store->initialized);
  free(store);
}

void change_delta (store* store, int delta) {
  store->delta = delta;
}

int add_queue (store* store, int id, char* name, int name_size, int length) {
  if (store->index > store->length - 1) {
    return 0;
  }

  else {
    store->queues[store->index] = create_queue(id, name, name_size, length);
    store->initialized[store->index] = 1;
    store->index++;

    return 1;
  }
}

int find_queue (store* store, int id) {
  for (int i = 0; i < store->length; i++) {
    if (store->initialized[i]) {
      if (store->queues[i]->id == id) {
        return i;
      }
    }
  }

  return -1;
}
