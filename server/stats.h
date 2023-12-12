#ifndef __STATS_H
#define __STATS_H
#define N_PROC 50
#include <pthread.h>

enum statCode {
	SPUT = 0,
	SGET = 1,
	SDEL = 2,
	SKEY = 3,
};

/* Estrucuta para manejo de stats */
typedef struct _Stats {
	long puts;
	long gets;
	long dels;
	long keys;
    pthread_mutex_t mutex; // Lock para cada stat de cada hilo
} Stats;

void lock(pthread_mutex_t *mutex);

void unlock(pthread_mutex_t *mutex);

/**
 * Función que incrementa en 1 el contador representado por cod en la 
 * estructura stat.
 */
void add_stat(Stats *stat, enum statCode cod);

/**
 * Función que decrementa en 1 el contador de keys en la 
 * estructura stat.
 */
void sub_key(Stats *stat);

/**
 * Función que suma todos los stats de los hilos en
 * gstat. 
 */
void get_stats(int n, Stats *gstat, Stats *stats);


#endif