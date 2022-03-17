/* NAME: Lukas Hanson
 * DATE: 2020/11/20
 * CLASS: CIS 415
 * LAB: 6
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct meal_ticket {
  int number;
  char* dish;
};

struct meal_queue {
  char* name;
  struct meal_ticket** buffer;
  int head;
  int tail;
  int cur_len;
  int max_len;
};

void free_meal_ticket(struct meal_ticket* meal_ticket) {
  free(meal_ticket->dish);
  free(meal_ticket);
}

void print_meal_ticket(struct meal_ticket* meal_ticket) {
  printf("Ticket Number: %d - Dish: %s\n", meal_ticket->number, meal_ticket->dish);
}

struct meal_ticket* create_meal_ticket(int number, char* dish) {
  struct meal_ticket* meal_ticket = (struct meal_ticket*) malloc(sizeof(struct meal_ticket));
  meal_ticket->number = number;
  meal_ticket->dish = (char*) malloc(BUFSIZ * sizeof(char));
  strcpy(meal_ticket->dish, dish);

  return meal_ticket;
}

void free_meal_queue(struct meal_queue* meal_queue) {
  free(meal_queue->name);

  for (int i = 0; i < meal_queue->cur_len; i++) {
    if (meal_queue->buffer[i] != NULL) {
      free_meal_ticket(meal_queue->buffer[i]);
    }
  }

  free(meal_queue->buffer);
  free(meal_queue);
}

int is_full(struct meal_queue* meal_queue) {
  if ((meal_queue->head == meal_queue->tail + 1) || ((meal_queue->head == 0) && (meal_queue->tail == meal_queue->max_len - 1))) {
    return 1;
  }

  else {
    return 0;
  }
}

int is_empty(struct meal_queue* meal_queue) {
  if (meal_queue->head == -1) {
    return 1;
  }

  else {
    return 0;
  }
}

int dequeue(struct meal_queue* meal_queue, char* dish) {
  printf("popping: Queue %s - ", meal_queue->name);

  if (is_empty(meal_queue)) {
    printf("Queue is empty, nothing to pop\n");

    return 0;
  }

  else {
    print_meal_ticket(meal_queue->buffer[meal_queue->head]);
    strcpy(dish, (meal_queue->buffer[meal_queue->head])->dish);

    free_meal_ticket(meal_queue->buffer[meal_queue->head]);
    meal_queue->cur_len--;

    if (meal_queue->head == meal_queue->tail) {
      meal_queue->head = -1;
      meal_queue->tail = -1;
    }

    else {
      meal_queue->head = (meal_queue->head + 1) % meal_queue->max_len;
    }

    return 1;
  }
}

int enqueue(struct meal_queue* meal_queue, char* dish) {
  printf("pushing: Queue %s - ", meal_queue->name);

  if (is_full(meal_queue)) {
    printf("Error Queue is full\n");

    return 0;
  }

  else {
    if (meal_queue->head == -1) {
      meal_queue->head = 0;
    }

    meal_queue->tail = (meal_queue->tail + 1) % meal_queue->max_len;
    meal_queue->buffer[meal_queue->tail] = create_meal_ticket(meal_queue->cur_len, dish);
    meal_queue->cur_len++;

    print_meal_ticket(meal_queue->buffer[meal_queue->tail]);

    return 1;
  }
}

struct meal_queue* create_meal_queue(char* name, int max_len) {
  struct meal_queue* meal_queue = (struct meal_queue*) malloc(sizeof(struct meal_queue));
  meal_queue->name = (char*) malloc(BUFSIZ * sizeof(char));
  strcpy(meal_queue->name, name);

  meal_queue->buffer = (struct meal_ticket**) malloc(max_len * sizeof(struct meal_ticket*));

  meal_queue->head = -1;
  meal_queue->tail = -1;
  meal_queue->cur_len = 0;
  meal_queue->max_len = max_len;

  return meal_queue;
}

int main() {
  struct meal_queue** registry = (struct meal_queue**) malloc(4 * sizeof(struct meal_queue*));
  registry[0] = create_meal_queue("Breakfast", 3);
  registry[1] = create_meal_queue("Lunch", 3);
  registry[2] = create_meal_queue("Dinner", 3);
  registry[3] = create_meal_queue("Bar", 3);
  char* output = (char*) malloc(BUFSIZ * sizeof(char));

  enqueue(registry[0], "Eggs");
  enqueue(registry[0], "Bacon");
  enqueue(registry[0], "Toast");
  enqueue(registry[0], "Juice");

  enqueue(registry[1], "Sandwich");
  enqueue(registry[1], "Smoothie");
  enqueue(registry[1], "Salad");
  enqueue(registry[1], "Chips");

  enqueue(registry[2], "Steak");
  enqueue(registry[2], "Potatoes");
  enqueue(registry[2], "Asparagus");
  enqueue(registry[2], "Corn");

  enqueue(registry[3], "Mojito");
  enqueue(registry[3], "Mimosa");
  enqueue(registry[3], "Margarita");
  enqueue(registry[3], "Beer");

  int i = 0;
  int flag = 0;
  int result = 0;

  while (flag != 4) {
    result = dequeue(registry[i], output);

    if (result == 0) {
      flag++;
    }

    i++;
    i = i % 4;
  }

  free_meal_queue(registry[0]);
  free_meal_queue(registry[1]);
  free_meal_queue(registry[2]);
  free_meal_queue(registry[3]);
  free(registry);
  free(output);

  return 0;
}
