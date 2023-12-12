#ifndef __MEMCACHED_H__
#define __MEMCACHED_H__
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include "sock.h"
#include "parser.h"
#include "data.h"
#include "common.h"
#include "stats.h"
#include "memman.h"
#include "read.h"
#include "../structures/conc_queue.h"
#include "../structures/conc_hashtable.h"

#define CODE  0
#define KEY   1
#define VALUE 2


/* Información de cliente binario. */
typedef struct {
			int code; /* 1 solo byte */
			
			char *key;
			unsigned lenk;
			
			char *value;
			unsigned lenv;
			
			/* Buffering interno */
			unsigned olenk; /* offset lenk */
			unsigned okey; /* offset key */
			unsigned olenv;/* offset lenv */
			unsigned ovalue; /* offset value */
			int state; /* Estado del mensaje en proceso. */
			char buf[1024];
			int bo, bl;
} Cbin;

/* Información de cliente texto. */
typedef struct {
			char buf[2048];
			int blen; 
			int state; /* Estado del mensaje en proceso. */
} Ctext;

/**
 * Estructura para almacenar los clientes en 
 * la instancia epoll.
 */
typedef struct {
	int fd;
	int mode;
	void *cptr;
} Client;


/**
 * @brief Agrega el par clave-valor a la tabla hash y a una cola de desalojo.
 * 		  Si la clave existe, el valor es sobreescrito.	  
 * 		  Si no hay suficiente espacio en memoria, se desalojan los pares necesarios.
 * 
 * @param thData Estructura con información para cada hilo
 * @param key Clave
 * @param value Valor
 * @param qData Nodo de la cola para ser insertado en la cola de desalojo.
 * @return 0 si se sobreescribio el valor, 1 si se agrego como nuevo elemento y -1 si no se pudo
 * 		   insertar y -2 si supera el tamaño maximo admitido. 
 */
int add_element (struct eventloop_data *evd, QueueData *qData);

/**
 * @brief Elimina el par clave-valor de la tabla hash y de la cola de desalojo.
 * 
 * @param data Nodo de la cola para ser eliminado en la cola de desalojo y tabla hash.
 * @return -1 si no se pudo eliminar, mayor a 0 en caso contrario. 
 */
int del_element(QueueData *data);

/**
 * @brief Busca el par clave-valor en la tabla hash y lo guarda en data si es encontrado.
 * 		  Cuando encuentra dicho elemento, lo vuelve a actualizar en la cola de desalojo.
 * 
 * @param data Nodo de la cola para ser buscado en la tabla hash.
 * @return 0 si no se encuentra el elemento, 1 si la busqueda fue exitosa. 
 */
int get_element(QueueData *data);

/**
 * @brief Lee el/los mensajes en modo texto del cliente y los va consumiendo.
 * 
 * @param thData Estructura con información para cada hilo.
 * @param client Puntero de cliente
 * @return OK si se manejaron todos los mensajes con normalidad.
 * 		   EINVAL si hubo un error en la lectura o el último mensaje era 
 * 		   de longitud no aceptada. DISCONNECT si se corta la conexíon
 * 		   con el cliente. 
 */
int text_consume(struct eventloop_data *thData, Client *client);

/**
 * @brief Manejador de pedidos en modo texto.
 * 
 * @param thData Estructura con información para cada hilo. 
 * @param toks Pedido del cliente parseado.
 * @param lens Longitud de cada token.
 * @param fd File descriptor del cliente.
 * @param ntok Cantidad de tokens parseados.
 * @return OK si se manejo correctamente.
 * 		   EINVAL si el pedido es invalido.
 */
int text_handle(struct eventloop_data *thData, char *toks[], int lens[], int fd, int ntok);

/**
 * @brief Intenta leer len bytes sobre fd.
 * 
 * @param fd File descriptor del cliente.
 * @param buf Buffer a almacentar los bytes leidos.
 * @param offset Bytes leídos.
 * @param len Bytes a leer.
 * @return Mayor a 0 Lectura válida.
 * 		   INCOMPLETE Faltan datos para formar una consulta.
 * 		   DISCONNECT Cantidad de intentos de lectura superados.
 */
int try_read(Cbin *cb , int fd, void *buf, unsigned *offset, int len);

/**
 * @brief Lee datos enviados por un cliente y los guarda en su estructura, si se logra
 *        completar el pedido, se lo procesa con binary_handle.
 * 
 * @param thData Estructura con información para cada hilo.
 * @param cli Puntero de cliente.
 * @return OK Si se manejó el mensaje correctamente.
 * 		   INCOMPLETE Si se cargaron datos pero no los suficientes para procesarlos.
 * 		   EINVAL Si el pedido fue inválido.
 *		   DISCONNECT Si el cliente se desconecta o se supera la
		              cantidad de intentos de lectura.
 */			
int binary_consume(struct eventloop_data *thData, Client *cli);

/**
 * @brief Manejador de pedidos en modo binario.
 * 
 * @param thData  Estructura con información para cada hilo.
 * @param fd File descriptor del cliente.
 * @param cli Puntero del cliente binario.

 */
void binary_handle(struct eventloop_data *thData, int fd, Cbin *cli);

/**
 * @brief Manejador de pedidos de clientes.
 */
void handle_client(Client *client, struct eventloop_data *thData);

/**
 * @brief Proceso que se encarga de consumir los mensajes de los clientes
 * 		  y manejar los pedidos de estos asi como tambien atender nuevos clientes.
 * @param thData Estructura con informacion para el proceso.
 */
void *worker(struct eventloop_data *thData);

/* SERVIDOR */
void server(int text_sock, int bin_sock, size_t limitSize);



#endif