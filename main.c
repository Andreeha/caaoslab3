#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#define THREAD_CNT 4

typedef struct queue {
  int id;
  struct queue* next;
} queue;

typedef struct {
  int id;
  const char* name;
} data;

queue *freeq = NULL;
queue *freeb = NULL;
sem_t slen;
pthread_mutex_t mfreeq = PTHREAD_MUTEX_INITIALIZER;

void push(int id) {
  queue *q = malloc(sizeof(queue));
  q->id = id;
  q->next = NULL;
  pthread_mutex_lock(&mfreeq);
  if (freeq) {
    freeb->next = q;
    freeb = q;
  } else {
    freeq = freeb = q;
  }
  pthread_mutex_unlock(&mfreeq);
  sem_post(&slen);
}

int pop() {
  sem_wait(&slen);
  pthread_mutex_lock(&mfreeq);
  queue* tmp = freeq;
  int res = freeq->id;
  
  freeq = freeq->next;

  pthread_mutex_unlock(&mfreeq);
  free(tmp);
  return res;
}

int counter[256];
pthread_mutex_t mcounter;

void* runner(void* val) {
  data* d = val;
  int incounter[256] = { 0 };
  FILE* in = fopen(d->name, "r");
  char c;
  while ((c = getc(in)) != EOF) {
    incounter[c]++;
  }
  pthread_mutex_lock(&mcounter);
  for (int i = 0; i < 256; i++) {
    counter[i] += incounter[i];
  }
  pthread_mutex_unlock(&mcounter);
  fclose(in);
  int id = d->id;
  free(d);
  push(id);
  return NULL;
}

pthread_t threads[THREAD_CNT];

int get_thread() {
  return pop();
}


int main (int argc, const char** argv) {
  int v = 0;
  sem_init(&slen, 0, 0);
  if (argc == 1) {
    printf("No arguments provided\n");
    return 0;
  }

  for (int i = 0; i < THREAD_CNT; i++) {
    push(i);
  }

  for (int i = 1; i < argc; i++) {
    int id = get_thread();
    data* s = malloc(sizeof(data));// + strlen(argv[i]) + 1);
    s->id = id;
    s->name = argv[i];
    pthread_create(&threads[id], NULL, runner, s); //s);
  }

  do {
    sem_getvalue(&slen, &v);
  } while (v < 4);

  while (freeq) pop();

  for (int i = 0; i < 256; i++) {
    printf("%d ", counter[i]);
  }
  printf("\n");
  
  return 0;
}
