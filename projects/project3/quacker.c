// PROJECT: Quacker Server
//
// AUTHOR: Lukas Hanson
//
// DATE: 2020/11/05
//
// CLASS: CIS 415
//
// DESCRIPTION:
// The main function that implements the InstaQuack server for Proejct 3.

// Necessary libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "reader.h"
#include "store.h"

// Global initializations
pthread_mutex_t GLOBAL_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t GLOBAL_COND = PTHREAD_COND_INITIALIZER;
pthread_mutex_t CLEANUP_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CLEANUP_COND = PTHREAD_COND_INITIALIZER;
int START = 1;
int MAXTOPICS = 5;
int NUMPROXIES = 8;
int NUMATTEMPTS = 5;

// Structure declarations
typedef struct thread {
  pthread_t id;
  pthread_mutex_t* lock;
  int type;
  int free;
} thread;

typedef struct pool {
  struct thread** threads;
  int size;
} pool;

typedef struct args {
  struct pool* pool;
  struct thread* thread;
  struct store* store;
  struct job* job;
} args;

// Methods for the args structs
args* create_args (pool* pool, thread* thread, store* store, job* job) {
  args* args = (struct args*) malloc(sizeof(struct args));
  args->pool = pool;
  args->thread = thread;
  args->store = store;
  args->job = job;

  return args;
}

void destroy_args (args* args) {
  free(args);
}

// Methods for the thread struct
thread* create_thread (int type) {
  thread* thread = (struct thread*) malloc(sizeof(struct thread));
  thread->id = -1;
  thread->lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(thread->lock, NULL);
  thread->type = type;
  thread->free = 1;

  return thread;
}

void print_thread (thread* thread) {
  printf("ID: %ld | TYPE: %d | STATUS: %d\n", thread->id, thread->type, thread->free);
}

void destroy_thread (thread* thread) {
  free(thread->lock);
  free(thread);
}

// Methods for the pool struct
pool* create_pool (int proxies) {
  pool* pool = (struct pool*) malloc(sizeof(struct pool));
  pool->threads = (struct thread**) malloc((proxies + 1) * sizeof(struct thread*));
  pool->size = proxies + 1;

  for (int i = 0; i < pool->size; i++) {
    if (i == 0) {
      pool->threads[i] = create_thread(0);
    }

    else if ((i - 1) < (proxies / 2)) {
      pool->threads[i] = create_thread(1);
    }

    else {
      pool->threads[i] = create_thread(2);
    }
  }

  return pool;
}

void print_pool (pool* pool) {
  for (int i = 0; i < pool->size; i++) {
    print_thread(pool->threads[i]);
  }
}

void destroy_pool (pool* pool) {
  for (int i = 0; i < pool->size; i++) {
    destroy_thread(pool->threads[i]);
  }

  free(pool->threads);
  free(pool);
}

int threads_running (pool* pool) {
  int running = pool->size;

  for (int i = 0; i < pool->size; i++) {
    pthread_mutex_lock(pool->threads[i]->lock);
    running -= pool->threads[i]->free;
    pthread_mutex_unlock(pool->threads[i]->lock);
  }

  return running;
}

// Method to handle thread instructions
void interpret_job (store* store, thread* thread, job* job) {
  int* last_entries = (int*) malloc(store->length * sizeof(int));

  for (int i = 0; i < store->length; i++) {
    last_entries[i] = -1;
  }

  for (int i = 0; i < job->clist->cnum; i++) {
    char* argument = job->clist->commands[i]->argv[0];

    if (thread->type == 1) {
      printf("Proxy thread %ld - Type: Publisher  - Read command: %s\n", thread->id, argument);
    }

    else {
      printf("Proxy thread %ld - Type: Subscriber - Read command: %s\n", thread->id, argument);
    }

    if (strcmp(argument, "put") == 0) {
      int attempts = NUMATTEMPTS;
      int id = strtol(job->clist->commands[i]->argv[1], NULL, 10);
      int index = find_queue(store, id);
      char* url = job->clist->commands[i]->argv[2];
      char* caption = (char*) malloc(BUFSIZ * sizeof(char));

      strcpy(caption, "");

      for (int j = 3; j < job->clist->commands[i]->argc - 1; j++) {
        strcat(caption, job->clist->commands[i]->argv[j]);
        strcat(caption, " ");
      }

      trim_quotes(caption);
      trim_quotes(url);

      entry* new_entry = create_entry(url, caption, strlen(url) + 1, strlen(caption) + 1);

      while (attempts > 0) {
        int result = enqueue(store->queues[index], new_entry);

        if (result == 0) {
          printf("Proxy thread %ld - Type: Publisher  - Failed to push to queue: %s\n", thread->id, store->queues[index]->name);

          attempts--;

          sched_yield();
        }

        else {
          printf("Proxy thread %ld - Type: Publisher  - Pushed to queue: %s\n", thread->id, store->queues[index]->name);
          break;
        }
      }

      destroy_entry(new_entry);
      free(caption);
    }

    else if (strcmp(argument, "get") == 0) {
      int attempts = NUMATTEMPTS;
      int id = strtol(job->clist->commands[i]->argv[1], NULL, 10);
      int index = find_queue(store, id);
      char* filler_text = (char*) malloc(BUFSIZ * sizeof(char));
      char* filename = (char*) malloc(BUFSIZ * sizeof(char));

      strcpy(filler_text, "FILLER");
      sprintf(filename, "SUB:%ld.txt", pthread_self());

      entry* filler = create_entry(filler_text, filler_text, BUFSIZ, BUFSIZ);

      while (attempts > 0) {
        int result = get_entry(store->queues[index], filler, last_entries[index]);

        if (result == 0) {
          printf("Proxy thread %ld - Type: Subscriber - Failed to pull entry %d from queue: %s\n", thread->id, last_entries[index] + 1, store->queues[index]->name);

          attempts--;

          sched_yield();
        }

        else {
          if ((result != 1) && (result > last_entries[index])) {
            last_entries[index] = result;
          }

          printf("Proxy thread %ld - Type: Subscriber - Pulled entry %d from queue: %s\n", thread->id, last_entries[index] + 1, store->queues[index]->name);
          FILE* file = fopen(filename, "a");
          file_entry(filler, store->queues[index]->name, file);
          fclose(file);

          last_entries[index]++;
        }
      }

      destroy_entry(filler);
      free(filler_text);
      free(filename);
    }

    else if (strcmp(argument, "sleep") == 0) {
      int milliseconds = strtol(job->clist->commands[i]->argv[1], NULL, 10);

      sched_yield();

      usleep(milliseconds * 1000);
    }

    else if (strcmp(argument, "stop") == 0) {
      pthread_mutex_lock(job->lock);
      job->completed = 1;
      job->taken = 0;
      pthread_mutex_unlock(job->lock);

      free(last_entries);

      return;
    }
  }
}

// Functions to pass to threads
void* modifier (void* args) {
  if (START) {
    pthread_mutex_lock(&GLOBAL_LOCK);

    if (((struct args*)args)->thread->type == 1) {
      printf("Proxy thread %ld - Type: Publisher  - Waiting for a signal from main thread\n", ((struct args*)args)->thread->id);
    }

    else {
      printf("Proxy thread %ld - Type: Subscriber - Waiting for a signal from main thread\n", ((struct args*)args)->thread->id);
    }

    pthread_cond_wait(&GLOBAL_COND, &GLOBAL_LOCK);
    pthread_mutex_unlock(&GLOBAL_LOCK);
  }

  if (((struct args*)args)->thread->type == 1) {
    printf("Proxy thread %ld - Type: Publisher  - Started\n", ((struct args*)args)->thread->id);
  }

  else {
    printf("Proxy thread %ld - Type: Subscriber - Started\n", ((struct args*)args)->thread->id);
  }

  interpret_job(((struct args*)args)->store, ((struct args*)args)->thread, ((struct args*)args)->job);

  pthread_mutex_lock(((struct args*)args)->thread->lock);
  ((struct args*)args)->thread->free = 1;
  pthread_mutex_unlock(((struct args*)args)->thread->lock);

  if (((struct args*)args)->thread->type == 1) {
    printf("Proxy thread %ld - Type: Publisher  - Ended\n", ((struct args*)args)->thread->id);
  }

  else {
    printf("Proxy thread %ld - Type: Subscriber - Ended\n", ((struct args*)args)->thread->id);
    char* filename = (char*) malloc(BUFSIZ * sizeof(char));
    sprintf(filename, "SUB:%ld.txt", pthread_self());
    convert_html(filename, ((struct args*)args)->store->length);
    remove(filename);
    free(filename);
  }

  destroy_args(((struct args*)args));

  return NULL;
}

void* cleanup (void* args) {
  if (START) {
    pthread_mutex_lock(&CLEANUP_LOCK);
    printf("Proxy thread %ld - Type: Cleanup    - Waiting for a signal from main thread\n", ((struct args*)args)->thread->id);
    pthread_cond_wait(&CLEANUP_COND, &CLEANUP_LOCK);
    pthread_mutex_unlock(&CLEANUP_LOCK);
  }

  printf("Proxy thread %ld - Type: Cleanup    - Started\n", ((struct args*)args)->thread->id);

  sleep(((struct args*)args)->store->delta);

  while (threads_running(((struct args*)args)->pool) > 1) {
    for (int i = 0; i < ((struct args*)args)->store->length; i++) {
      if (((struct args*)args)->store->initialized[i]) {
        if (is_empty(((struct args*)args)->store->queues[i]) == 0) {
          struct timeval* current_time = (struct timeval*) malloc(sizeof(struct timeval));
          gettimeofday(current_time, NULL);

          int difference = current_time->tv_sec - ((struct args*)args)->store->queues[i]->entries[((struct args*)args)->store->queues[i]->tail]->timestamp->tv_sec;
          free(current_time);

          if (difference >= ((struct args*)args)->store->delta) {
            char* filler_text = (char*) malloc(BUFSIZ * sizeof(char));
            strcpy(filler_text, "FILLER");
            entry* filler = create_entry(filler_text, filler_text, BUFSIZ, BUFSIZ);

            dequeue(((struct args*)args)->store->queues[i], filler);

            printf("Proxy thread %ld - Type: Cleanup    - Dequeued entry %d from queue %s because %d seconds passed\n", ((struct args*)args)->thread->id, filler->number, ((struct args*)args)->store->queues[i]->name, difference);

            free(filler_text);
            destroy_entry(filler);
          }
        }
      }
    }

    sched_yield();
  }

  pthread_mutex_lock(((struct args*)args)->thread->lock);
  ((struct args*)args)->thread->free = 1;
  pthread_mutex_unlock(((struct args*)args)->thread->lock);

  printf("Proxy thread %ld - Type: Cleanup    - Ended\n", ((struct args*)args)->thread->id);

  destroy_args(((struct args*)args));

  return NULL;
}

// Method to assign jobs to threads
void assign_jobs (store* store, pool* pool, jlist* jlist) {
  for (int i = 0; i < pool->size; i++) {
    pthread_mutex_lock(pool->threads[i]->lock);
    int free = pool->threads[i]->free;
    int thread_type = pool->threads[i]->type;
    pthread_mutex_unlock(pool->threads[i]->lock);

    if (free) {
      if ((thread_type == 0) && START) {
        args* args = create_args(pool, pool->threads[i], store, NULL);

        pthread_mutex_lock(pool->threads[i]->lock);
        pool->threads[i]->free = 0;
        pthread_mutex_unlock(pool->threads[i]->lock);

        pthread_create(&pool->threads[i]->id, NULL, cleanup, args);

        continue;
      }

      for (int j = 0; j < jlist->size; j++) {
        pthread_mutex_lock(jlist->jobs[j]->lock);
        int completed = jlist->jobs[j]->completed;
        int taken = jlist->jobs[j]->taken;
        int job_type = jlist->jobs[j]->type;
        pthread_mutex_unlock(jlist->jobs[j]->lock);

        if ((completed == 0) && (taken == 0) && (thread_type == job_type)) {
          pthread_mutex_lock(pool->threads[i]->lock);
          pool->threads[i]->free = 0;
          pthread_mutex_unlock(pool->threads[i]->lock);

          pthread_mutex_lock(jlist->jobs[j]->lock);
          jlist->jobs[j]->taken = 1;
          pthread_mutex_unlock(jlist->jobs[j]->lock);

          args* args = create_args(pool, pool->threads[i], store, jlist->jobs[j]);

          pthread_create(&pool->threads[i]->id, NULL, modifier, args);

          break;
        }
      }
    }
  }
}

// Method to handle initial instructions
void interpret_input (store* store, pool* pool, jlist* jlist) {
  jlist->jobs[0]->taken = 1;

  for (int i = 0; i < jlist->jobs[0]->clist->cnum; i++) {
    char* argument = jlist->jobs[0]->clist->commands[i]->argv[0];

    printf("MAIN THREAD - READ COMMAND: %s\n", argument);

    if (strcmp(argument, "create") == 0) {
      int id = strtol(jlist->jobs[0]->clist->commands[i]->argv[2], NULL, 10);
      char* name = jlist->jobs[0]->clist->commands[i]->argv[3];
      trim_quotes(name);
      int name_length = strlen(jlist->jobs[0]->clist->commands[i]->argv[3]) + 1;
      int max_length = strtol(jlist->jobs[0]->clist->commands[i]->argv[4], NULL, 10);

      add_queue(store, id, name, name_length, max_length);
    }

    else if (strcmp(argument, "delta") == 0) {
      int delta = strtol(jlist->jobs[0]->clist->commands[i]->argv[1], NULL, 10);

      change_delta(store, delta);
    }

    else if (strcmp(argument, "add") == 0) {
      char* filename = jlist->jobs[0]->clist->commands[i]->argv[2];
      trim_quotes(filename);

      FILE* input_file = fopen(filename, "r");

      if (input_file == NULL) {
        printf("Invalid file %s\n", filename);
        continue;
      }

      struct clist* clist = read_file(input_file);
      fclose(input_file);

      char* subargument = jlist->jobs[0]->clist->commands[i]->argv[1];

      if (strcmp(subargument, "publisher") == 0) {
        struct job* new_job = create_job(clist, 1);
        add_job(jlist, new_job);
      }

      else if (strcmp(subargument, "subscriber") == 0) {
        struct job* new_job = create_job(clist, 2);
        add_job(jlist, new_job);
      }
    }

    else if (strcmp(argument, "start") == 0) {
      jlist->jobs[0]->completed = 1;
      assign_jobs(store, pool, jlist);
      sleep(1);
      printf("MAIN THREAD - SIGNALLING CLEANUP THREAD TO START FIRST\n");
      pthread_cond_broadcast(&CLEANUP_COND);
      sleep(1);
      printf("MAIN THREAD - SIGNALLING OTHER PROXY THREADS TO START\n");
      pthread_cond_broadcast(&GLOBAL_COND);
      START = 0;
    }
  }
}

// Main function
int main (int argc, char** argv) {
  // Grabbing the input file and converting it to instructions
  if ((argc != 2)) {
    printf("Invalid use of %s\n", argv[0]);
    return 0;
  }

  FILE* input_file = fopen(argv[1], "r");

  if (input_file == NULL) {
    printf("Invalid file %s\n", argv[1]);
    return 0;
  }

  clist* instructions = read_file(input_file);
  fclose(input_file);
  job* startup = create_job(instructions, -1);

  // Initializing the necessary structs
  jlist* jlist = create_jlist(startup);
  store* store = create_store(MAXTOPICS);
  pool* pool = create_pool(NUMPROXIES);

  interpret_input(store, pool, jlist);

  // If jobs are left, assign them once threads are freed
  while (jobs_left(jlist) > 0) {
    assign_jobs(store, pool, jlist);
  }

  // Wait for all the threads to finish
  for (int i = 0; i < pool->size; i++) {
    pthread_mutex_lock(pool->threads[i]->lock);
    long int id = pool->threads[i]->id;
    pthread_mutex_unlock(pool->threads[i]->lock);

    if (id != -1) {
      printf("MAIN THREAD - WAITING FOR PROXY THREAD %ld\n", pool->threads[i]->id);
      pthread_join(pool->threads[i]->id, NULL);
    }
  }

  // Freeing the memory used
  destroy_jlist(jlist);
  destroy_store(store);
  destroy_pool(pool);

  return 0;
}
