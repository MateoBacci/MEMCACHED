#include "stats.h"

void lock(pthread_mutex_t *mutex) {
	pthread_mutex_lock(mutex);
}

void unlock(pthread_mutex_t *mutex) {
	pthread_mutex_unlock(mutex);
}

void add_stat (Stats *stat, enum statCode cod) {
    lock(&stat->mutex);
    switch (cod) {
        case SPUT:
            stat->puts++;
            break;
        case SGET:
            stat->gets++;
            break;
        case SDEL:
            stat->dels++;
            break;
        case SKEY:
            stat->keys++;
            break;
    }
    unlock(&stat->mutex);
}

void sub_key(Stats *stat) {
    lock(&stat->mutex);
    stat->keys--;
    unlock(&stat->mutex);
}

void get_stats(int n, Stats *gstat, Stats *stats) {
	lock(&gstat->mutex);
	for (int i = 0; i < n; i++) {
	
        lock(&stats[i].mutex);
		
        /* Sumamos stats de cada hilo en gstat */
        gstat->dels += stats[i].dels;
		gstat->gets += stats[i].gets;
		gstat->keys += stats[i].keys;
		gstat->puts += stats[i].puts;
		
        /* Reiniciamos cada stat */
        stats[i].dels = 0;
        stats[i].gets = 0;
        stats[i].keys = 0;
        stats[i].puts = 0;

        unlock(&stats[i].mutex);
	}
	unlock(&gstat->mutex);
}