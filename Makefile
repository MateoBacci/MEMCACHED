FILES.O = common.o data.o parser.o sock.o conc_hashtable.o hashtable.o memcached.o conc_queue.o queue.o stats.o memman.o

FLAGS = -g -Wall -Wextra -std=c99

SERVER = server/

STRUCT = structures/

OBJ = obj/

CC = gcc

EXE.NAME = memcached



program: $(OBJ) $(FILES.O)
	$(CC)  $(addprefix $(OBJ),$(FILES.O)) $(FLAGS) -pthread -o $(EXE.NAME)

$(OBJ):
	mkdir -p $(OBJ)
	
common.o: $(SERVER)common.c
	$(CC) $(FLAGS) -c $(SERVER)common.c -o $(OBJ)common.o 

data.o: $(SERVER)data.c
	$(CC) $(FLAGS) -c $(SERVER)data.c -o $(OBJ)data.o 

stats.o: $(SERVER)stats.c
	$(CC) $(FLAGS) -pthread -c $(SERVER)stats.c -o $(OBJ)stats.o

parser.o: $(SERVER)parser.c
	$(CC) $(FLAGS) -c $(SERVER)parser.c -o $(OBJ)parser.o

sock.o: $(SERVER)sock.c
	$(CC) $(FLAGS) -c $(SERVER)sock.c -o $(OBJ)sock.o

memman.o: $(SERVER)memman.c
	$(CC) $(FLAGS) -c $(SERVER)memman.c -o $(OBJ)memman.o

memcached.o: $(SERVER)memcached.c
	$(CC) $(FLAGS) -pthread -c $(SERVER)memcached.c -o $(OBJ)memcached.o

hashtable.o: $(STRUCT)hashtable.c 
	$(CC) $(FLAGS) -c $(STRUCT)hashtable.c -o $(OBJ)hashtable.o

conc_hashtable.o: $(STRUCT)conc_hashtable.c 
	$(CC) $(FLAGS) -c $(STRUCT)conc_hashtable.c -o $(OBJ)conc_hashtable.o

queue.o: $(STRUCT)queue.c
	$(CC) $(FLAGS) -c $(STRUCT)queue.c -o $(OBJ)queue.o

conc_queue.o: $(STRUCT)conc_queue.c
	$(CC) $(FLAGS) -c $(STRUCT)conc_queue.c -o $(OBJ)conc_queue.o

clean:
	rm $(OBJ)*