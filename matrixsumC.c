#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#define MAXSIZE 10000
#define MAXWORKERS 10

pthread_mutex_t barrier;

pthread_cond_t go;

pthread_mutex_t minlock;

pthread_mutex_t maxlock;

pthread_mutex_t rowlock;

pthread_mutex_t totalsumlock;

int numWorkers;

int numArrived = 0;

void Barrier() {

    pthread_mutex_lock(&barrier);

    numArrived++;

    if (numArrived == numWorkers) {

        numArrived = 0;

        pthread_cond_broadcast(&go);
    }
    else pthread_cond_wait(&go, &barrier);

    pthread_mutex_unlock(&barrier);
}


double read_timer() {

    static bool initialized = false;

    static struct timeval start;

    struct timeval end;

    if( !initialized ){

        gettimeofday( &start, NULL );

        initialized = true;
    }
    gettimeofday( &end, NULL );

    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time;

int size, stripSize;

int sums[MAXWORKERS];

int min, minR, minC, max, maxR, maxC,final, nextRow;

int matrix[MAXSIZE][MAXSIZE];

void *Worker(void *);

int main(int argc, char *argv[]) {

    int i, j;

    long l;

    pthread_attr_t attr;

    pthread_t workerid[MAXWORKERS];

    pthread_attr_init(&attr);

    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    pthread_mutex_init(&barrier, NULL);

    pthread_cond_init(&go, NULL);

    size = (argc > 1)? atoi(argv[1]) : MAXSIZE;

    numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;

    if (size > MAXSIZE) size = MAXSIZE;

    if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;

    stripSize = size/numWorkers;

    for (i = 0; i < size; i++)for (j = 0; j < size; j++)matrix[i][j] = rand()%99;

#ifdef DEBUG
    for (i = 0; i < size; i++) {
	  printf("[ ");
	  for (j = 0; j < size; j++) {
	    printf(" %d", matrix[i][j]);
	  }
	  printf(" ]\n");
  }
#endif

    start_time = read_timer();

    for (l = 0; l < numWorkers; l++) pthread_create(&workerid[l], &attr, Worker, (void *) l);

    for(l = 0; l < numWorkers; l++) pthread_join(workerid[l],NULL);

    end_time = read_timer();

    printf("SUM : %d\n", final);

    printf("MIN : %d at (%d,%d)\n", min,minR,minC);

    printf("MAX : %d at (%d,%d)\n", max,maxR,maxC);

    printf("The execution time is %g sec\n", end_time - start_time);

    pthread_exit(NULL);

}

void *Worker(void *arg) {

    long myid = (long) arg;

    int total, i, j, first, last, row;

#ifdef DEBUG
    printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif

    first = myid*stripSize;

    last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);

    total = 0;

    while(nextRow < size){

        pthread_mutex_lock(&rowlock);

        row = nextRow++;

        pthread_mutex_unlock(&rowlock);

        for (j = 0; j < size; j++){
            if(matrix[row][j] > max){
                pthread_mutex_lock(&maxlock);
                if(matrix[row][j] > max){
                    max = matrix[row][j];
                    maxR = nextRow;
                    maxC = j;

                }
                pthread_mutex_unlock(&maxlock);
            }
            if(matrix[row][j] < min){
                pthread_mutex_lock(&minlock);
                if(matrix[row][j] < min){
                    min = matrix[row][j];
                    minR = nextRow;
                    minC= j;
                }
                pthread_mutex_unlock(&minlock);
            }
            total += matrix[row][j];
        }
    }
    pthread_mutex_lock(&totalsumlock);

    final += total;

    pthread_mutex_unlock(&totalsumlock);
}