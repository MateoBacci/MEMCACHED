#include "memcached.h"
#include <unistd.h>

#define MB 1 << 20
#define GB 1 << 30

#define MAX_EPOLL_EVENTS 1000		  /* Cantidad de eventos en la instancia epoll. */
#define HASH_TABLE_SIZE 1000000		  /* Cantidad de entradas de la tabla hash. */
#define LIMIT_SIZE (unsigned)3 * (GB) /* Espacio limite que va a usar el servidor si no se especifica. */
#define RESERVED_SIZE 40 * (MB)		  /* Espacio requerido para el uso interno del servidor. */
#define MAX_TEXT_LINE 2048			  /* Longitud maxima de un pedido en modo texto. */
#define TEXT_MODE 1
#define BINARY_MODE 0
#define BYTES_TO_READ_DATA 4 /* Cantidad de bytes a leer en modo binario para \
								determinar la longitud del dato siguiente. */
#define READING_ATTEMPS 10	 /* Máximos intentos de lectura invalida. */
#define DISCONNECT -1
#define INCOMPLETE -2

Stats gStats;		 /* Stat general para enviarle al cliente que lo pida. */
Stats stats[N_PROC]; /* Stat para cada hilo trabajador. Cuando un cliente los pida, los sumamos en gStats. */

ConcHashTable table; /* Tabla hash de pares clave-valor */
ConcQueue queue;	 /* Cola para manejar el desalojo de memoria */

/* Inicializo estructura para cliente binario. */
Cbin *init_cbin()
{
	Cbin *cb = memalloc(table, queue, sizeof(Cbin), &gStats);
	cb->bl = 0;
	cb->bo = 0;
	cb->olenk = 0;
	cb->okey = 0;
	cb->olenv = 0;
	cb->ovalue = 0;
	cb->lenv = 0;
	cb->lenk = 0;
	cb->code = 0;
	cb->key = NULL;
	cb->value = NULL;
	cb->state = CODE;
	return cb;
}

/* Inicializo estructura para cliente de texto. */
Ctext *init_ctext()
{
	Ctext *ct = memalloc(table, queue, sizeof(Ctext), &gStats);
	ct->blen = 0;
	ct->state = 0;
	return ct;
}

/* Destruccion de cliente. */
void destroy_client(Client *client)
{
	if(client->mode == BINARY_MODE) {
		Cbin *cb = (Cbin *)client->cptr;
		free(cb->key);
		free(cb->value);
	}
	free(client->cptr);
	free(client);
}


int add_element(struct eventloop_data *thData, QueueData *qData)
{

	/* Si el par clave-valor supera el tamaño máximo admitido no insertamos. */
	if (queue_data_key(qData, NULL)->len +
			queue_data_value(qData, NULL)->len >
		thData->size_max)
		return -2;

	/* Obtenemos posicion en tabla hash. */
	unsigned idx = hash(queue_data_key(qData, NULL), conc_hashtable_size(table));

	/* Preparamos el nodo (idx, key, value) para insertar en la tabla hash. */
	QueueNode *newNodeTable = memalloc(table, queue, queue_node_size(), &gStats);
	queue_node_init(newNodeTable);
	queue_data_ihash(qData, idx);
	queue_node_data(newNodeTable, qData);

	/* Preparamos el nodo (idx, NULL, NULL) para insertar en la cola. */
	QueueNode *newQueueNode = memalloc(table, queue, queue_node_size(), &gStats);
	QueueData *newQueueData = memalloc(table, queue, queue_data_size(), &gStats);
	queue_node_init(newQueueNode);
	queue_data_init(newQueueData);
	queue_data_ihash(newQueueData, idx);
	queue_node_data(newQueueNode, newQueueData);

	/* Lockeamos tabla hash y cola de desalojo */
	conc_hashtable_lock(table, idx);
	conc_queue_lock(queue);

	/* Insertamos en tabla hash y cola de desalojo */
  int pos = hashtable_insert(conc_hashtable_table(table), newNodeTable); /* Insertamos en table. */

	/* Si la clave es existente eliminamos el elemento anterior de la cola. */
	if (pos >= 0)
		queue_delete(conc_queue_queue(queue), newQueueData, pos);

	/* Pusheamos el nodo en la cola. */
	queue_push(conc_queue_queue(queue), newQueueNode);
	/* Unlockeamos tabla hash y cola de desalojo */
	conc_queue_unlock(queue);
	conc_hashtable_unlock(table, idx);

	return pos;
}

int del_element(QueueData *data)
{
	/* Obtenemos posicion en tabla hash. */
	unsigned idx = queue_data_ihash(data, -1);

	/* Lockeamos tabla hash y cola de desalojo */
	conc_hashtable_lock(table, idx);
	conc_queue_lock(queue);

	/* Eliminamos elemento de la tabla hash. */
	int pos = hashtable_delete(conc_hashtable_table(table), data);

	/* Eliminamos el elemento de la cola de desalojo. */
	if (pos >= 0)
		queue_delete(conc_queue_queue(queue), data, pos);

	/* Unlockeamos tabla hash y cola de desalojo */
	conc_queue_unlock(queue);
	conc_hashtable_unlock(table, idx);

	return pos;
}

int get_element(QueueData *data)
{

	/* Obtenemos posicion en tabla hash. */
	unsigned idx = queue_data_ihash(data, -1);

	/* Preparamos el nodo (idx, key, value) para copiar el elemento buscado. */
	QueueNode *newNodeTable = memalloc(table, queue, queue_node_size(), &gStats);
	queue_node_init(newNodeTable);
	queue_node_data(newNodeTable, data);

	/* Preparamos el nodo (idx, NULL, NULL) para reemplazar el elemento en la cola. */
	QueueNode *newQueueNode = memalloc(table, queue, queue_node_size(), &gStats);
	QueueData *newQueueData = memalloc(table, queue, queue_data_size(), &gStats);
	queue_node_init(newQueueNode);
	queue_data_init(newQueueData);
	queue_data_ihash(newQueueData, idx);
	queue_node_data(newQueueNode, newQueueData);

	/* Lockeamos tabla hash y cola de desalojo */
	conc_hashtable_lock(table, idx);
	conc_queue_lock(queue);

	/* Buscamos el elemento en la tabla hash y guardamos una copia en data. */
	int len = hashtable_search(conc_hashtable_table(table), data);
	Data *value = queue_data_value(data, NULL);
	if (value == NULL || value->data == NULL) { /* Si el elemento no se encuentra eliminamos lo previamente creado. */
    free_queue_node(newQueueNode);
		free_queue_node(newNodeTable);
		conc_queue_unlock(queue);
		conc_hashtable_unlock(table, idx);
    if (len != -1)
      return EBIG;
		return ENOTFOUND;
	}
	/* Reinsertamos en tabla hash y cola de desalojo */
	int pos = hashtable_insert(conc_hashtable_table(table), newNodeTable);
	queue_delete(conc_queue_queue(queue), newQueueData, pos);
	queue_push(conc_queue_queue(queue), newQueueNode);

	/* Unlockeamos tabla hash y cola de desalojo */
	conc_queue_unlock(queue);
	conc_hashtable_unlock(table, idx);

	return OK;
}

/* Inicializa un dato con el tamaño especificado. */
Data *data_init(char *data, unsigned len)
{
	Data *d = memalloc(table, queue, sizeof(Data), &gStats);
	d->data = data;
	d->len = len;
	return d;
}

int text_consume(struct eventloop_data *thData, Client *client)
{
	Ctext *cli = (Ctext *)client->cptr;
	int blen = cli->blen, fd = client->fd, r = 0, att = 0;
	char *buf = cli->buf;
	while (1) {
		if (att >= READING_ATTEMPS) {
			cli->blen = blen;
			return INCOMPLETE;
		}

		int rem = MAX_TEXT_LINE - blen;
		assert(rem >= 0);

		/* Buffer lleno, no hay comandos */
		if (rem == 0) {
			blen = 0;
			rem = MAX_TEXT_LINE;
			cli->state = -1;
    }
    int nread = READ(fd, buf + blen, rem);
		if (nread == 0) { // Si no se logra leer nada
			att++;
			continue;
		}
		else if (nread < 0)
			return DISCONNECT;

		att = 0;
		blen += nread;
		char *p, *p0 = buf;
		int nlen = blen;

		/* Para cada \n, procesar, y avanzar punteros */
		while ((p = memchr(p0, '\n', nlen)) != NULL) {
			if (cli->state == -1) { /* Si los datos son parte de un mensaje largo */
				write(fd, code_to_str(EINVALID), 7);
				nlen -= p - p0 + 1;
				p0 = p + 1;
				cli->state = 0;
				continue;
			}
			/* Mensaje completo */
			int len = p - p0;
			*p++ = 0;
			char *toks[5] = {NULL};
			int lens[5] = {0};
			int ntok;
			ntok = text_parser(p0, toks, lens);
			r = text_handle(thData, toks, lens, fd, ntok);
			if (r == EINVALID)
				write(fd, code_to_str(EINVALID), 7);
			if (r == ENOTFOUND)
				write(fd, code_to_str(ENOTFOUND), 11);

			nlen -= len + 1;
			p0 = p;
		}

		/* Si consumimos algo, mover */
		if (p0 != buf) {
			memmove(buf, p0, nlen);
			blen = nlen;
		}
	}
}

int text_handle(struct eventloop_data *thData, char *toks[], int lens[], int fd, int ntok)
{
	int id = thData->id;

	/* Cantidad de palabras inválidas. */
	if (ntok > 3)
		return EINVALID;

	/* Se ingresó PUT */
	if (!strcmp(toks[0], code_to_str(PUT))) {
		if (toks[1] == NULL || toks[2] == NULL) /* Parametros de PUT mal ingresados. */
			return EINVALID;

		/* Verificamos que las palabras son alfanumericas. */
		if (!(is_word(toks[1]) && is_word(toks[2])))
			return EINVALID;

		/* Asignamos los valores a key y a value respectivamente. */
		char *key = memalloc(table, queue, sizeof(char) * lens[1] + 1, &gStats);
		key[lens[1]] = 0;
		memcpy(key, toks[1], lens[1]);
		Data *dkey = data_init(key, lens[1]);

		char *value = memalloc(table, queue, sizeof(char) * lens[2] + 1, &gStats);
		value[lens[2]] = 0;
		memcpy(value, toks[2], lens[2]);
		Data *dvalue = data_init(value, lens[2]);

		QueueData *qData = memalloc(table, queue, queue_data_size(), &gStats);
		queue_data_init(qData);
		queue_data_key(qData, dkey);
		queue_data_value(qData, dvalue);

		/* Insertamos par key-value en la tabla. Si no es posible EINVALID. */
		int newKey = add_element(thData, qData);

    if (newKey == -2) {
      return EINVALID;
    }

		/* Modificamos contador de PUT y KEYS. */
		add_stat(&stats[id], SPUT);
		if (newKey == -1)
			add_stat(&stats[id], SKEY);

		/* Si todo sale bien enviamos OK al cliente.*/
		write(fd, code_to_str(OK), 3);
		return OK;
	}

	/* Se ingresó GET */
	else if (!(strcmp(toks[0], code_to_str(GET)))) {
		if (toks[1] == NULL || toks[2] != NULL) /* Parametros de GET mal ingresados. */
			return EINVALID;

		/* Verificamos que las palabras son alfanumericas. */
		if (!is_word(toks[1]))
			return EINVALID;

		/* Asignamos los valores a key y a value respectivamente. */
		char *key = memalloc(table, queue, sizeof(char) * lens[1] + 1, &gStats);
		memcpy(key, toks[1], lens[1]);
		key[lens[1]] = 0;
		Data *dkey = data_init(key, lens[1]);

		QueueData *qData = memalloc(table, queue, queue_data_size(), &gStats);
		queue_data_init(qData);
		queue_data_key(qData, dkey);
		queue_data_ihash(qData, hash(dkey, conc_hashtable_size(table)));

		/* Buscamos según el valor asociado a la clave en la tabla hash. */
		int encontrado = get_element(qData);
    if (encontrado == EBIG) {
			write(fd, code_to_str(EBIG), 5);
			return EBIG;
    } else if (encontrado == ENOTFOUND)
			return ENOTFOUND;

		Data *value = queue_data_value(qData, NULL);

		/* Modificamos contador de GETS. */
		add_stat(&stats[id], SGET);

		/* Si todo sale bien le respondemos al cliente con el valor obtenido. */
		write(fd, "OK ", 4);
		write(fd, value->data, value->len);
		write(fd, "\n", 1);
		return OK;
	}

	/* Se ingresó DEL */
	else if (strcmp(toks[0], code_to_str(DEL)) == 0) {
		if (toks[1] == NULL || toks[2] != NULL) /* Parametros de GET mal ingresados. */
			return EINVALID;

		/* Verificamos que las palabras son alfanumericas. */
		if (!is_word(toks[1]))
			return EINVALID;

		/* Asignamos los valores a key */
		char *key = memalloc(table, queue, sizeof(char) * lens[1] + 1, &gStats);
		memcpy(key, toks[1], lens[1]);
		key[lens[1]] = 0;
		Data *dkey = data_init(key, lens[1]);

		QueueData *qData = memalloc(table, queue, queue_data_size(), &gStats);
		queue_data_init(qData);
		queue_data_key(qData, dkey);
		queue_data_ihash(qData, hash(dkey, conc_hashtable_size(table)));

		/* Buscamos según el valor asociado a la clave en la tabla hash. */
		int deleted = del_element(qData);

		free_queue_data(qData);
		if (deleted == -1)
			return ENOTFOUND;

		/* Modificamos contador de DELS y KEYS. */
		add_stat(&stats[id], SDEL);
		sub_key(&stats[id]);

		/* Si todo sale bien enviamos OK. */
		write(fd, code_to_str(OK), 3);
		return OK;
	}

	/* Se ingresó STATS */
	else if (strcmp(toks[0], code_to_str(STATS)) == 0) {
		char reply[100];
		get_stats(thData->n_proc, &gStats, stats); /* Obtenemos gStats a partir de los stats de los hilos. */
		sprintf(reply, "OK PUTS=%ld DELS=%ld GETS=%ld KEYS=%ld\n", gStats.puts, gStats.dels, gStats.gets, gStats.keys);
		write(fd, reply, strlen(reply));
		return 1;
	}

	/* No se ingreso un pedido valido */
	return EINVALID;
}

int try_read(Cbin *cb, int fd, void *buf, unsigned *offset, int len)
{
	int nread = 0;
	void *tbuf = buf + *offset;
	for (int i = 0, rc = 0; i < 10 && len > 0; i++, len -= rc, nread += rc) {
    rc = READB(cb, fd, tbuf + nread, len);
		if (rc < 0)
			return DISCONNECT;
	}
	*offset += nread;
	if (len == 0)
		return nread;
	else
		return INCOMPLETE;
}
void restart_client_data (Cbin *cli) {
  	/* Actualizamos las condiciones iniciales */
	cli->key = cli->value = NULL;
  cli->state = CODE; 
  cli->lenk = cli->lenv = 0;
  cli->olenk = cli->olenv = 0;
  cli->okey = cli->ovalue = 0;
}


int binary_consume(struct eventloop_data *thData, Client *client)
{
	Cbin *cli = (Cbin *)client->cptr;
	int fd = client->fd, r, n, ocode = 0;
	unsigned *offset;

	if (cli->state == CODE) {

		/* Si el pedido comienza a cargarse desde el principio. */
		offset = (unsigned *)&ocode;
		cli->code = 0;

		/* Leo la orden del cliente (PUT/GET/DEL/STATS). */
		if ((r = try_read(cli, fd, &cli->code, offset, 1)) != 1)
			return r;

		if (cli->code != PUT && cli->code != GET && cli->code != DEL) {
			if (cli->code != STATS)
				return EINVALID; /* Orden inválida. */
			binary_handle(thData, fd, cli);
      /* Actualizamos las condiciones iniciales */
      restart_client_data(cli);
			return OK;
		}

		cli->state = KEY; /* Actualizamos estado actual del pedido. */
	}

	if (cli->state == KEY) {
		
    n = BYTES_TO_READ_DATA - cli->olenk; /* bytes a leer */
		if (n > 0) {
			
      offset = (unsigned *)&cli->olenk;
			
      /* Leemos la longitud de la clave. */
			if ((r = try_read(cli, fd, &cli->lenk, offset, n)) != n)
				return r;

			cli->lenk = ntohl(cli->lenk); /* Paso longitud de clave de big-endian a little-endian. */

			cli->key = memalloc(table, queue, cli->lenk + 1, &gStats);
			if (!cli->key)
      	return DISCONNECT;
		}

		n = cli->lenk - cli->okey;
		if (n > 0) {
			
      offset = (unsigned *)&cli->okey;
			
      /* Leemos la clave a partir de la longitud anterior. */
			if ((r = try_read(cli, fd, cli->key, offset, n)) != n)
				return r;
			cli->key[cli->lenk] = 0;
    }

		if (cli->code == GET || cli->code == DEL) // GET o DEL
			binary_handle(thData, fd, cli);
		else
			cli->state = VALUE; /* Actualizamos estado actual del pedido. */
	}
	if (cli->state == VALUE) {

		n = BYTES_TO_READ_DATA - cli->olenv; /* bytes a leer */
		if (n > 0) {
			offset = (unsigned *)&cli->olenv;

			/* Leemos la longitud del valor. */
			if ((r = try_read(cli, fd, &cli->lenv, offset, n)) != n)
				return r;

			cli->lenv = ntohl(cli->lenv); /* Paso longitud de clave de big-endian a little-endian. */
      cli->value = memalloc(table, queue, cli->lenv + 1, &gStats);
      if (!cli->value)
        return DISCONNECT;
		}

		n = cli->lenv - cli->ovalue;
		if (n > 0) {
			offset = (unsigned *)&cli->ovalue;
			/* Leemos el valor a partir de la longitud anterior. */
			if ((r = try_read(cli, fd, cli->value, offset, n)) != n)
				return r;

			cli->value[cli->lenv] = 0;
		}
		binary_handle(thData, fd, cli);
	}

	/* Actualizamos las condiciones iniciales */
  restart_client_data(cli);
	return OK;
}

void binary_handle(struct eventloop_data *thData, int fd, Cbin *cli)
{
	int len, reply;
	int id = thData->id;
	int ord = cli->code;

	/* Se ingresó PUT*/
	if (ord == PUT) {
		Data *dkey, *dvalue;

		dkey = data_init(cli->key, cli->lenk);

		dvalue = data_init(cli->value, cli->lenv);

		QueueData *qData = memalloc(table, queue, queue_data_size(), &gStats);
		queue_data_key(qData, dkey);
		queue_data_value(qData, dvalue);

		/* Insertamos par key-value en la tabla. */
    int newKey = add_element(thData, qData);

    if (newKey == -2) {
      reply = EINVALID;
      write(fd, &reply, 1);
      return;
    }

		/* Modificamos contador de PUT y KEYS. */
		add_stat(&stats[id], SPUT);
		if (newKey == -1)
			add_stat(&stats[id], SKEY);

		/* Si todo sale bien enviamos OK al cliente.*/
		reply = OK;
		write(fd, &reply, 1);
		return;
	}

	/* Se ordenó GET */
	else if (ord == GET) {
		Data *dkey;

		dkey = data_init(cli->key, cli->lenk);

		QueueData *qData = memalloc(table, queue, queue_data_size(), &gStats);
		queue_data_init(qData);
		queue_data_key(qData, dkey);
		queue_data_ihash(qData, hash(dkey, conc_hashtable_size(table)));

		/* Buscamos según el valor asociado a la clave en la tabla hash. */
		int encontrado = get_element(qData);
    if (encontrado == EBIG) {
      reply = EBIG;
			write(fd, &reply, 1);
      return;
    }
		if (encontrado == ENOTFOUND) {
			reply = ENOTFOUND;
			write(fd, &reply, 1);
			return;
		}

		/* Modificamos contador de GETS. */
		add_stat(&stats[id], SGET);
		len = htonl(queue_data_value(qData, NULL)->len); /* Pasamos la longitud del valor a formato big-endian. */

		/* Si todo sale bien le respondemos al cliente con el valor obtenido. */
		reply = OK;
		write(fd, &reply, 1);
		write(fd, &len, 4);
		write(fd, queue_data_value(qData, NULL)->data, queue_data_value(qData, NULL)->len);
		return;
	}

	/* Se ingresó DEL */
	else if (ord == DEL) {
		Data *dkey;

		dkey = data_init(cli->key, cli->lenk);

		QueueData *qData = memalloc(table, queue, queue_data_size(), &gStats);
		queue_data_init(qData);
		queue_data_key(qData, dkey);
		queue_data_ihash(qData, hash(dkey, conc_hashtable_size(table)));

		/* Buscamos según el valor asociado a la clave en la tabla hash y lo eliminamos. */
		int deleted = del_element(qData);
		free_queue_data(qData);
		if (deleted == -1) {
			reply = ENOTFOUND;
			write(fd, &reply, 1);
			return;
		}

		/* Modificamos contador de DELS y KEYS. */
		add_stat(&stats[id], SDEL);
		sub_key(&stats[id]);

		/* Si todo sale bien enviamos OK. */
		reply = OK;
		write(fd, &reply, 1);
		return;
	}

	/* Se ingresó STATS */
	else if (ord == STATS) {
		char ans[500];
		get_stats(thData->n_proc, &gStats, stats); /* Obtenemos gStats a partir de los stats de los hilos. */
		long puts = gStats.puts,
			 gets = gStats.gets,
			 dels = gStats.dels,
			 keys = gStats.keys;

		/* Vamos enviando el mensaje armado de a partes */

		sprintf(ans, "PUTS=%ld DELS=%ld GETS=%ld KEYS=%ld", puts, dels, gets, keys);
		reply = OK;
		len = htonl(strlen(ans)); /* Enviamos longitud del mensaje en formato big-endian. */

		write(fd, &reply, 1);
		write(fd, &len, 4);
		len = ntohl(len); /* Pasamos longitud del mensaje a formato little-endian. */
		write(fd, ans, len);
		return;
	}

	/* Si el mensaje no matchea con ningun condicional enviamos EINVALID */
	reply = EINVALID;
	write(fd, &reply, 1);
	return;
}

void handler_ignore()
{
	/* Ignoramos la señal */
}

void handler_quit(int signum)
{
	if (signum == SIGINT) {
		puts("\nEliminando servidor...");
		conc_hashtable_destroy(table);
		puts("Tabla hash destruida");
		conc_queue_destroy(queue);
		puts("Cola de elementos destruida");
	}
	exit(1);
}

void handle_signals()
{
	signal(SIGINT, handler_quit);
	signal(SIGPIPE, handler_ignore);
  //signal(SIGSEGV, handler_ignore);
}

void handle_client(Client *client, struct eventloop_data *thData)
{
	int r = 0;
	struct epoll_event clievent;
	clievent.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
	if (client->mode) { /*Modo TEXTO */
		r = text_consume(thData, client);
		if (r == EINVALID)
			write(client->fd, code_to_str(EINVALID), 7);
	}
	else { /*Modo BINARIO */
		r = binary_consume(thData, client);
		if (r == EINVALID || r == EUNK)
			write(client->fd, &r, 1);
	}
	if (r == DISCONNECT) {
		puts("Cliente desconectado");
		close(client->fd);
		destroy_client(client);
	}
	else {
		/* Si el cliente no se desconecta lo volvemos a agregar a la instancia
		   epoll para futuros mensajes.*/
		clievent.data.ptr = client;
		if (epoll_ctl(thData->epfd, EPOLL_CTL_MOD, client->fd, &clievent) == -1)
			quit("epoll_ctl() - epollworker");
	}
}

void *worker(struct eventloop_data *thData)
{

	struct epoll_event events[MAX_EPOLL_EVENTS];
	int epollfd = thData->epfd,
		num_ready = 0,
		text_sock = thData->tsock,
		bin_sock = thData->bsock,
		csock;
	struct epoll_event clievent;
	Client *cli;

	/* Aceptamos nuevos clientes y procesamos pedidos. */
	while (1) {
		num_ready = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1); // Esperamos la aparicion de eventos
		if (num_ready == -1)
			quit("epoll_wait() - main process");

		for (int i = 0; i < num_ready; i++) {
			if (events[i].data.fd == text_sock || events[i].data.fd == bin_sock) {
				/* Agregamos cliente a la instancia epoll */
				cli = memalloc(table, queue, sizeof(Client), &gStats);
				if (events[i].data.fd == text_sock) {
					if ((csock = accept(text_sock, NULL, NULL)) == -1)
						quit("accept(0)");
					cli->mode = TEXT_MODE;
					cli->cptr = init_ctext();
				}
				else {
					if ((csock = accept(bin_sock, NULL, NULL)) == -1)
						quit("accept(1)");
					cli->mode = BINARY_MODE;
					cli->cptr = init_cbin();
				}
				fcntl(csock, F_SETFL, O_NONBLOCK);
				cli->fd = csock;

				if (cli->mode)
					printf("SERVER: Cliente registrado en modo texto\n");
				else
					printf("SERVER: Cliente registrado en modo binario\n");
				clievent.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
				clievent.data.ptr = cli;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, csock, &clievent) == -1)
					quit("epoll_ctl() - epollworker");
			}
			else {
				handle_client(events[i].data.ptr, thData);
			}
		}
	}
}

/* Estructura de datos para cada proceso. */
struct eventloop_data *thread_data(int id, int epollfd, int n_workers, unsigned limitSize, int tsock, int bsock)
{
	struct eventloop_data *thData = memalloc(table, queue, sizeof(struct eventloop_data), &gStats);
	thData->epfd = epollfd;
	thData->id = id;
	thData->n_proc = n_workers;
	thData->size_max = limitSize;
	thData->tsock = tsock;
	thData->bsock = bsock;
	pthread_mutex_init(&stats[id].mutex, NULL);
	stats[id].puts = 0;
	stats[id].gets = 0;
	stats[id].dels = 0;
	stats[id].keys = 0;
	return thData;
}

void server(int text_sock, int bin_sock, size_t limitSize)
{

	/* Creamos instancia epoll */
	int epollfd = epoll_create1(0);
	if (epollfd == -1)
		quit("epoll_create()");

	/* Numeros de cores del procesador. */
	int n_proc = sysconf(_SC_NPROCESSORS_ONLN);

	/* Numero de trabajadores que van a encargarse de procesar las instrucciones. */
	int n_workers = n_proc;

	gStats.dels = 0;
	gStats.gets = 0;
	gStats.keys = 0;
	gStats.puts = 0;

	/* Agregamos los sockets para modo texto(8888) y binario(8889) respectivamente a la instancia epoll */
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = text_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, text_sock, &event) == -1)
		quit("epoll_ctl()");

	event.data.fd = bin_sock;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, bin_sock, &event) == -1)
		quit("epoll_ctl()");

	/* Creamos los hilos trabajadores */
	pthread_t pthread_id[n_workers - 1];
	struct eventloop_data *thData[n_workers];
	for (int i = 0; i < n_workers - 1; i++) {
		/* Inicializamos la estructura arg y stat de cada hilo. */
		thData[i] = thread_data(i, epollfd, n_workers, limitSize, text_sock, bin_sock);
		pthread_create(&pthread_id[i], NULL, (void *(*)(void *))worker, (void *)thData[i]);
	}

	puts("SERVIDOR INICIADO");
	printf("Limite de memoria: %ld en bytes / %ld en KB / %ld en MB / %ld en GB\n", limitSize,
		   limitSize / 1024,
		   (limitSize / 1024) / 1024,
		   ((limitSize / 1024) / 1024) / 1024);

	thData[n_workers - 1] = thread_data(n_workers - 1, epollfd, n_workers, limitSize, text_sock, bin_sock);
	worker(thData[n_workers - 1]);
}

int main(int argc, char **argv)
{
	size_t limitSize = LIMIT_SIZE;
	if (argc == 2) { /* Si no se ingreso argumento se toma por defecto LIMIT_SIZE */
		long input = strtol(argv[1], NULL, 10);
		if (input > 0)
			limitSize = input + RESERVED_SIZE;
	}

	handle_signals();

	int text_sock, bin_sock;

	/* Limitamos la memoria, una parte para el almacenamiento de datos
	   y la otra para el uso interno del servidor. */
	limitSize = limit_mem(limitSize);

	/* Socket vinculado al puerto de modo texto */
	text_sock = mk_tcp_sock(mc_lport_text);
	if (text_sock < 0)
		quit("mk_tcp_sock.text");

	/* Socket vinculado a puerto de modo binario */
	bin_sock = mk_tcp_sock(mc_lport_bin);
	if (bin_sock < 0)
		quit("mk_tcp_sock.bin");

	/* Inicializamos la tabla hash y creamos la cola de desalojo. */
	table = conc_hashtable_init(HASH_TABLE_SIZE);
	queue = conc_queue_create();

	/*Iniciar el servidor*/
	server(text_sock, bin_sock, limitSize);

	return 0;
}
