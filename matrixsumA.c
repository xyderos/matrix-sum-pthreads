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

#undef DEBUG

pthread_mutex_t barrier;

pthread_cond_t go;

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

int matrix[MAXSIZE][MAXSIZE];

int minvals[MAXWORKERS][3];

int maxVals[MAXWORKERS][3];

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

    for (i = 0; i < size; i++) for (j = 0; j < size; j++)matrix[i][j] = rand()%99;

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

    pthread_exit(NULL);
}

void *Worker(void *arg) {

    long myid = (long) arg;

    int total, i, j, first, last, min, minRow, minCol, max, maxRow, maxCol;

#ifdef DEBUG
    printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif

    first = myid*stripSize;

    last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);

    total = 0;

    max = matrix[first][0];

    min = matrix[first][0];

    for (i = first; i <= last; i++){
        for (j = 0; j < size; j++){
            total += matrix[i][j];

            if(matrix[i][j] >= max){
                max = matrix[i][j];
                maxRow = i;
                maxCol = j;
            }
            if(matrix[i][j] <= min){
                min = matrix[i][j];
                minRow = i;
                minCol= j;
            }
        }
    }

    minvals[myid][0] = min;

    minvals[myid][1] = minRow;

    minvals[myid][2]= minCol;

    maxVals[myid][0] = max;

    maxVals[myid][1] = maxRow;

    maxVals[myid][2]= maxCol;

    sums[myid] = total;

    Barrier();

    if (myid == 0) {
        total = 0;
        max = 0;
        min = 100;
        for (i = 0; i < numWorkers; i++){
            total += sums[i];

            if(maxVals[i][0] >= max){
                max = maxVals[i][0];
                maxRow = maxVals[i][1];
                maxCol = maxVals[i][2];
            }
            if(minvals[i][0] <= min){
                min = minvals[i][0];
                minRow = minvals[i][1];
                minCol = minvals[i][2];
            }
        }

        end_time = read_timer();

        printf("SUM : %d\n", total);

        printf("MIN : %d at (%d,%d)\n", min, minRow, minCol);

        printf("MAX : %d at (%d,%d)\n", max, maxRow, maxCol);

        printf("The execution time is %g sec\n", end_time - start_time);
    }
}