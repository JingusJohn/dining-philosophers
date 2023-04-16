#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// CONSTANTS

const int DEBUG = 0;

// STRUCTURES

struct diner_t {
  sem_t *forkBuffer; // array of semaphores to post and wait for. Each
                     // represents the status of a fork
  int philosophers; // number of philosophers dining
  int startingApetite; // total amount of times each philosopher will eat
};

struct philosopherCtx_t {
  struct diner_t *ctx;
  int apetite; // amount of times the individual philosopher has left to eat
  int identity;  // identifies the philosopher
  int leftFork;  // index of the left fork in the forkBuffer
  int rightFork; // index of the right fork in the forkBuffer
};

// FUNCTIONS

/* Initialize the diner context for the application */
void init(struct diner_t *diner, int philosophers, int apetite);
/* Initialize an individual philosopher's context */
void initPCtx(struct philosopherCtx_t *pCtx, struct diner_t *diner, int p);
/* Calculates the index of the fork to the left of the philosopher */
int getLeftFork(int p, int philosophers);
/* Calculates the index of the fork to the right of the philosopher */
int getRightFork(int p, int philosophers);
/* logic for a philosopher thread to execute */
void *philosopher(void *param);
/* Philosopher eats (uses sleep to simulate complexity) */
void eat(int philosopher);
/* Philosopher thinks (uses sleep to simulate complexity) */
void think(int philosopher);
/* Cleanup for the diner */
void clean(struct diner_t *diner);


int main(int argc, char *argv[]) {

  // user needs to provide the number of philosophers and the amount of times
  //  each philosopher will eat before they are full

  // if they don't provide the required arguments, notify the user and exit
  if (argc != 3) {
    printf("Usage: %s [number of philosophers] [how many times each "
           "philosopher eats]\n",
           argv[0]);
    exit(1);
  }

  // parse the arguments as integers
  int numPhilosophers = atoi(argv[1]);
  int philosopherApetite = atoi(argv[2]);

  if (DEBUG) {
    // just prints some info about the number of philosopers and their apetites
    printf("%d philosophers, each eats %d times total.\n", numPhilosophers,
           philosopherApetite);
  }

  // initialize the diner
  struct diner_t diner;
  init(&diner, numPhilosophers, philosopherApetite);

  // use an array to track the threads created for each philosopher
  pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * diner.philosophers);
  // create an array of structs tracking the context of each philosopher
  // these contexts will be passed as arguments into the thread function
  struct philosopherCtx_t *pContexts = (struct philosopherCtx_t *)malloc(sizeof(struct philosopherCtx_t) * diner.philosophers);

  // create threads and context for every philosopher
  for (int p = 0; p < numPhilosophers; p++) {
    // configure philosopherCtx
    initPCtx(&pContexts[p], &diner, p);

    // create a thread for each philosopher (executes philosopher function) with context as parameter
    pthread_create(&threads[p], NULL, philosopher, &pContexts[p]);

  }

  // wait for all the threads we created
  for (int p = 0; p < numPhilosophers; p++) {
    pthread_join(threads[p], NULL);
  }

  // cleanup (deallocating dynamically allocated memory)
  clean(&diner);
  free(threads);
  free(pContexts);
  return 0;
}

/* Initialize the diner context for the application */
void init(struct diner_t *diner, int philosophers, int apetite) {
  diner->philosophers = philosophers;
  diner->startingApetite = apetite;
  // the number of forks is equal to the number of philosophers assuming they're
  // at a circular table
  diner->forkBuffer = (sem_t *)malloc(sizeof(sem_t) * philosophers);

  // initialize a semaphore for every fork in the forkBuffer
  for (int p = 0; p < philosophers; p++) {
    sem_init(&diner->forkBuffer[p], 0, 1);
  }
}

/* Initialize an individual philosopher's context */
void initPCtx(struct philosopherCtx_t *pCtx, struct diner_t *diner, int p) {
  // attach the diner context to the philosopher object to allow for easy access
  pCtx->ctx = diner;
  pCtx->apetite = diner->startingApetite;
  pCtx->identity = p;
  // these getFork functions assume the table is circular and there is a fork between each philosopher
  // this assumption is safe because it's exactly what's specified in the assignment PDF
  pCtx->leftFork = getLeftFork(p, diner->philosophers);
  pCtx->rightFork = getRightFork(p, diner->philosophers);
}

/* Calculates the index of the fork to the left of the philosopher */
int getLeftFork(int p, int philosophers) {
  int result = p - 1;
  if (result == -1) {
    result = philosophers - 1;
  }
  return result;
}

/* Calculates the index of the fork to the right of the philosopher */
int getRightFork(int p, int philosophers) {
  return p;
}

/* logic for a philosopher thread to execute */
void *philosopher(void *param) {
  // parse philosopher context from the void *param
  struct philosopherCtx_t *pCtx = (struct philosopherCtx_t *)param;

  // used only for debugging
  int timesEaten = 0;

  while (pCtx->apetite > 0) {
    think(pCtx->identity);

    // even philosophers grab the right fork first
    //  odd philosophers grab the left fork first
    // sem_wait to simulate this action

    if (pCtx->identity % 2 == 0) {
      if (DEBUG) {
        printf("philosopher %d is waiting for right [%d] then left [%d] fork\n", pCtx->identity, pCtx->rightFork, pCtx->leftFork);
      }
      sem_wait(&pCtx->ctx->forkBuffer[pCtx->rightFork]);
      sem_wait(&pCtx->ctx->forkBuffer[pCtx->leftFork]);
    } else {
      if (DEBUG) {
        printf("philosopher %d is waiting for left [%d] then right [%d] fork\n", pCtx->identity, pCtx->leftFork, pCtx->rightFork);
      }
      sem_wait(&pCtx->ctx->forkBuffer[pCtx->leftFork]);
      sem_wait(&pCtx->ctx->forkBuffer[pCtx->rightFork]);
    }

    eat(pCtx->identity);
    if (DEBUG) {
      printf("philosopher %d has eaten %d times.\n", pCtx->identity + 1, timesEaten++ + 1);
    }
    // decrement apetite since the philosopher has eaten
    pCtx->apetite--;
    // philosophers have no preference for order in which they place forks back down
    // sem_post to simulate this action
    sem_post(&pCtx->ctx->forkBuffer[pCtx->rightFork]);
    sem_post(&pCtx->ctx->forkBuffer[pCtx->leftFork]);
  }
}

/* Philosopher eats (uses sleep to simulate complexity) */
void eat(int philosopher) {
  printf("Philosopher %d is eating.\n", philosopher + 1);
  // sleep one second (I don't know how to sleep in milliseconds)
  sleep(1);
}

/* Philosopher thinks (uses sleep to simulate complexity) */
void think(int philosopher) {
  printf("Philosopher %d is thinking.\n", philosopher + 1);
  // sleep one second (I don't know how to sleep in milliseconds)
  sleep(1);
}

/* Cleanup for the diner */
void clean(struct diner_t *diner) {
  // deallocate the memory we allocated in the init function
  free(diner->forkBuffer);
}
