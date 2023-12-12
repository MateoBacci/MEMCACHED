#include <stdlib.h>
#include <stdio.h>
#include "memman.h"
#include <limits.h> 
#include <sys/time.h> 
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include "../structures/conc_hashtable.h"
#include "../structures/conc_queue.h"

size_t min (size_t a, size_t b) { return a < b ? a : b; }

size_t limit_mem(size_t size) {
	struct rlimit limit;
    struct sysinfo mem;
    sysinfo(&mem);
    size_t lim = min(size, mem.freeram); /* Tomamos como límite máximo el espacio de memoria ram disponible. */
    limit.rlim_cur = lim;
    limit.rlim_max = lim;

    if (setrlimit(RLIMIT_DATA, &limit) != 0) {
        quit("setrlimit");
    }

    return lim;
}

void *memalloc(ConcHashTable table, ConcQueue queue, size_t size, Stats *s) {
    void *buf = malloc(size);
    unsigned pos;
    conc_queue_lock(queue);
    while(!queue_is_empty(conc_queue_queue(queue)) && buf == NULL) {
        pos = queue_pop(conc_queue_queue(queue));
        if (conc_hashtable_trylock(table, pos) != 0) { // Si no toma el mutex
            // Creamos de nuevo el nodo recien eliminado.
            QueueNode *qn = malloc(queue_node_size());
            QueueData *qd = malloc(queue_data_size());
            queue_node_init(qn);
            queue_data_init(qd);
            queue_data_ihash(qd, pos);
            queue_node_data(qn, qd);
            // Lo reinsertamos en la cola.
            queue_push(conc_queue_queue(queue), qn);
        } else { // Si podemos tomar el mutex, eliminamos el nodo correspondiente.
            hashtable_pop(conc_hashtable_table(table), pos);
            conc_hashtable_unlock(table, pos);
            sub_key(s); /* Decrementados contador de keys. */
            buf = malloc(size);
        }
    }
    conc_queue_unlock(queue);
    return buf;
}


