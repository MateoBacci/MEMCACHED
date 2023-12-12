#ifndef __DATA_H__
#define __DATA_H__

/*
 * Estructura para almacenar un string y su longitud.
 */
typedef struct _Data {
    char *data;
    unsigned len;
} Data;


void free_data(Data *d);

#endif