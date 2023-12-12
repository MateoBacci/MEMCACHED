#ifndef __MEM_MAM_H__
#define __MEM_MAM_H__

#include "common.h"
#include "../structures/conc_hashtable.h" 
#include "../structures/conc_queue.h"
#include "stats.h"

/**
 * Función para limitar memoria.
*/
size_t limit_mem(size_t size);

/**
 * Función para reemplazar la operacion malloc. 
 * Si no se puede realizar se desaloja memoria de la tabla hash
 * hasta que sea posible. Si igualmente no es posible retorna NULL.
 */
void *memalloc(ConcHashTable table, ConcQueue queue, size_t size, Stats *k);

#endif /* __MEM_MAM_H__ */