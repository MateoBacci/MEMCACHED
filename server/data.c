#include "data.h"
#include <stdlib.h>

void free_data(Data *d) {
    if (d->data != NULL)
        free(d->data);
    free(d);
}