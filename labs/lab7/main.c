/*=============================================================================
 * Program Name: lab7
 * Author: Lukas Hanson
 * Date: 11/27/2020
 * Description:
 *     A simple program that implements a thread-safe queue of meal tickets
 *
 *===========================================================================*/

//========================== Preprocessor Directives ==========================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
//=============================================================================

//================================= Constants =================================
#define MAXNAME 15
#define MAXQUEUES 4
#define MAXTICKETS 3
#define MAXDISH 20
#define MAXPUBS 3
#define MAXSUBS 4
//=============================================================================

//============================ Structs and Macros =============================
typedef struct meal_ticket{
	int number;
	char* dish;
} meal_ticket;

typedef struct MTQ {
	char name[MAXNAME];
	struct meal_ticket* buffer;
	pthread_mutex_t lock;
	int head;
	int tail;
	int max_len;
	int cur_len;
} MTQ;

typedef struct pargs {
	struct meal_ticket* tickets;
	char** MTQ_IDs;
	pthread_t thread;
} pargs;

typedef struct sargs {
	char* MTQ_ID;
	pthread_t thread;
	struct meal_ticket ticket;
} sargs;

MTQ* registry[MAXQUEUES];

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t global_cond = PTHREAD_COND_INITIALIZER;
//=============================================================================

//================================= Functions =================================
void initMTQ(int pos, char* MTQ_ID) {
    registry[pos] = (MTQ *) malloc(sizeof(struct MTQ));
		strcpy(registry[pos]->name, MTQ_ID);
		registry[pos]->buffer = (meal_ticket*) malloc(MAXTICKETS * sizeof(struct meal_ticket));
		registry[pos]->head = -1;
		registry[pos]->tail = -1;
		registry[pos]->max_len = MAXTICKETS;
		registry[pos]->cur_len = 0;
}

void freeMTQ(int pos) {
	free(registry[pos]->buffer);
	free(registry[pos]);
}

void print_meal_ticket(meal_ticket* meal_ticket) {
  printf("Ticket Number: %d - Dish: %s\n", meal_ticket->number, meal_ticket->dish);
}

int findMTQ(char* MTQ_ID) {
	for (int i = 0; i < MAXQUEUES; i++) {
		if (strcmp(registry[i]->name, MTQ_ID) == 0) {
			return i;
		}
	}

	return -1;
}

void printMTQ(char* MTQ_ID) {
	int index = findMTQ(MTQ_ID);

	printf("Name: %s\n", registry[index]->name);

	for (int i = 0; i < registry[index]->cur_len; i++) {
		print_meal_ticket(&registry[index]->buffer[i]);
	}
}

int is_full(MTQ* meal_queue) {
  if ((meal_queue->head == meal_queue->tail + 1) || ((meal_queue->head == 0) && (meal_queue->tail == meal_queue->max_len - 1))) {
    return 1;
  }

  else {
    return 0;
  }
}

int is_empty(MTQ* meal_queue) {
  if (meal_queue->head == -1) {
    return 1;
  }

  else {
    return 0;
  }
}

int enqueue(char* MTQ_ID, meal_ticket* MT) {
	int index = findMTQ(MTQ_ID);

	pthread_mutex_lock(&global_lock);

	printf("pushing: Queue %s - ", registry[index]->name);

  if (is_full(registry[index])) {
    printf("Error Queue is full\n");

		pthread_mutex_unlock(&global_lock);
    return 0;
  }

  else {
    if (registry[index]->head == -1) {
      registry[index]->head = 0;
    }

    registry[index]->tail = (registry[index]->tail + 1) % registry[index]->max_len;
		MT->number = registry[index]->cur_len;
		registry[index]->buffer[registry[index]->tail] = *MT;
    registry[index]->cur_len++;

    print_meal_ticket(&registry[index]->buffer[registry[index]->tail]);

		pthread_mutex_unlock(&global_lock);
		return 1;
	}
}

int dequeue(char* MTQ_ID, meal_ticket* MT) {
	int index = findMTQ(MTQ_ID);

	pthread_mutex_lock(&global_lock);

	printf("popping: Queue %s - ", registry[index]->name);

  if (is_empty(registry[index])) {
    printf("Queue is empty, nothing to pop\n");

		pthread_mutex_unlock(&global_lock);
    return 0;
  }

  else {
    print_meal_ticket(&registry[index]->buffer[registry[index]->head]);
    MT = &registry[index]->buffer[registry[index]->head];

    registry[index]->cur_len--;

    if (registry[index]->head == registry[index]->tail) {
      registry[index]->head = -1;
      registry[index]->tail = -1;
    }

    else {
      registry[index]->head = (registry[index]->head + 1) % registry[index]->max_len;
    }

		pthread_mutex_unlock(&global_lock);
    return 1;
  }
}

void* publisher(void* args) {
	pthread_mutex_lock(&global_lock);
	printf("Publisher thread %ld waiting for signal...\n", pthread_self());
	pthread_cond_wait(&global_cond, &global_lock);
	pthread_mutex_unlock(&global_lock);

	for (int i = 0; i < MAXPUBS; i++) {
		int index = findMTQ(((struct pargs*)args)->MTQ_IDs[i]);
		printf("Publisher thread %ld enqueuing...\n", pthread_self());
		enqueue(registry[index]->name, &((struct pargs*)args)->tickets[i]);
		sleep(1);
	}
}

void* subscriber(void* args) {
	pthread_mutex_lock(&global_lock);
	printf("Subscriber thread %ld waiting for signal...\n", pthread_self());
	pthread_cond_wait(&global_cond, &global_lock);
	pthread_mutex_unlock(&global_lock);

	for (int i = 0; i < MAXSUBS; i++) {
		int index = findMTQ(((struct sargs*)args)->MTQ_ID);
		printf("Subscriber thread %ld dequeuing...\n", pthread_self());
		int result = dequeue(registry[index]->name, &((struct sargs*)args)->ticket);

		if (result == 0) {
			printf("Queue was empty when accessed by subscriber thread %ld...\n", pthread_self());
			sleep(1);
		}
	}
}
//=============================================================================

//=============================== Program Main ================================
int main(int argc, char argv[]) {
	//Variables Declarations
	char* qNames[] = {"Breakfast", "Lunch", "Dinner", "Bar"};
	char* bonly[] = {"Breakfast", "Breakfast", "Breakfast"};
	char* lonly[] = {"Lunch", "Lunch", "Lunch"};
	char* donly[] = {"Dinner", "Dinner", "Dinner"};
	char* bronly[] = {"Bar", "Bar", "Bar"};
	char* bFood[] = {"Eggs", "Bacon", "Steak"};
	char* lFood[] = {"Burger", "Fries", "Coke"};
	char* dFood[] = {"Steak", "Greens", "Pie"};
	char* brFood[] = {"Whiskey", "Sake", "Wine"};
	int i, j, t = 1;
	int test[4];
	char dsh[] = "Empty";
	meal_ticket bfast[3] = {[0].dish = bFood[0], [1].dish = bFood[1], [2].dish = bFood[2]};
	meal_ticket lnch[3] = {[0].dish = lFood[0], [1].dish = lFood[1], [2].dish = lFood[2]};
	meal_ticket dnr[3] = {[0].dish = dFood[0], [1].dish = dFood[1], [2].dish = dFood[2]};
	meal_ticket br[3] = {[0].dish = brFood[0], [1].dish = brFood[1], [2].dish = brFood[2]};
	meal_ticket ticket = {.number=0, .dish=dsh};
	pthread_t p1, p2, p3, p4, p5, p6, p7, p8;

	for (i = 0; i < MAXQUEUES; i++) {
		initMTQ(i, qNames[i]);
	}

	struct pargs args1 = {bfast, bonly, p1};
	struct pargs args2 = {lnch, lonly, p2};
	struct pargs args3 = {dnr, donly, p3};
	struct pargs args4 = {br, bronly, p4};
	struct sargs args5 = {qNames[0], p5, ticket};
	struct sargs args6 = {qNames[1], p6, ticket};
	struct sargs args7 = {qNames[2], p7, ticket};
	struct sargs args8 = {qNames[3], p8, ticket};

	pthread_create(&p1, NULL, publisher, (void *)&args1);
	pthread_create(&p2, NULL, publisher, (void *)&args2);
	pthread_create(&p3, NULL, publisher, (void *)&args3);
	pthread_create(&p4, NULL, publisher, (void *)&args4);
	pthread_create(&p5, NULL, subscriber, (void *)&args5);
	pthread_create(&p6, NULL, subscriber, (void *)&args6);
	pthread_create(&p7, NULL, subscriber, (void *)&args7);
	pthread_create(&p8, NULL, subscriber, (void *)&args8);

	sleep(3);
	pthread_mutex_lock(&global_lock);
	printf("Broadcasting to all threads from MAIN...\n");
	pthread_cond_broadcast(&global_cond);
	pthread_mutex_unlock(&global_lock);

	pthread_join(p1, NULL);
	pthread_join(p2, NULL);
	pthread_join(p3, NULL);
	pthread_join(p4, NULL);
	pthread_join(p5, NULL);
	pthread_join(p6, NULL);
	pthread_join(p7, NULL);
	pthread_join(p8, NULL);

	for (i = 0; i < MAXQUEUES; i++) {
		freeMTQ(i);
	}

	return EXIT_SUCCESS;
}
//=============================================================================
