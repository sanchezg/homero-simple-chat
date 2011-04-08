#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_QUE 5
#define TAM 256

typedef struct ptr 
{
	pthread_t data1;
	int data2;
	struct ptr *next;
} lista_pt;

void list_print(lista_pt *); 
lista_pt **list_search_d2(lista_pt **, int);
lista_pt **list_search_d1(lista_pt **, pthread_t);
void list_remove(lista_pt **);
lista_pt *list_add(lista_pt **, pthread_t, int);
void *ejec_cliente(void *);
int nuevo_cliente(lista_pt **, int);
int aceptar_conexion(int);
int iniciar_servidor (int);
int list_len(lista_pt *);

int SERVER_ACTIVO;

int main(int argc, char *argv[]) 
{
	int des_servidor, des_cliente, clilen;
	
	struct sockaddr_in cli_addr;
	lista_pt *thread = NULL;

	des_servidor = iniciar_servidor(atoi(argv[1]));

	//clilen = sizeof(cli_addr);

	//supuestamente si no hay clientes en la cola, se queda esperando...
	while(SERVER_ACTIVO)
	{
		des_cliente=aceptar_conexion(des_servidor);
		if (nuevo_cliente(&thread, des_cliente) < 0)
			exit(EXIT_FAILURE);

		
		//Si no hay mas conexiones o el adm cerro el server,
		//SERVER_ACTIVO == 0;

	//No hace falta hacer join, porque los threads de seguro terminan antes que el server...
	}

	exit(EXIT_SUCCESS);
}


int iniciar_servidor (int puerto)
{
	int sockfd;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{ 
		perror("socket");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons (puerto);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("bind");
		return -1;
	}

	if(listen (sockfd, MAX_QUE) < 0)
	{
		perror("listen");
		return -1;
	}

	SERVER_ACTIVO = 1;
	printf ("## srv disponible, puerto %d\n", ntohs(serv_addr.sin_port));
	return sockfd;
}

int aceptar_conexion(int socket)
{
	int newsockfd;
	size_t clilen;
	struct sockaddr_in cli_addr;

	clilen = sizeof(cli_addr);
	newsockfd = accept(socket, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0) 
	{
		perror("accept");
		return -1;
	}
	return newsockfd;
}

int nuevo_cliente(lista_pt **n, int dsc)
{
	pthread_t new;

	if (pthread_create(&new, NULL, ejec_cliente, (void *) &dsc) != 0)
	{
		printf ("ERROR PTHREAD\n");
		return -1;
	}
	list_add(n, new, dsc);
	return 0;
}

void *ejec_cliente(void *ptr)
{
	int *iptr, isc;
	size_t clilen;
	struct sockaddr_in cli_addr;
	struct sockaddr_in* s;
	char ipstr[INET_ADDRSTRLEN], buffer[TAM];

	iptr = (int *) ptr;
	isc = *iptr;

	clilen = sizeof(cli_addr);
	s = (struct sockaddr_in *) &cli_addr;
	getpeername(isc, (struct sockaddr*) &cli_addr, &clilen);
	
	printf("hilo %lu del cliente %s\n", pthread_self(), inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr));

	memset(buffer, 0, TAM);
	if (read(des_cliente, buffer, TAM-1) < 0) 
	{
		perror("read");
		exit(EXIT_FAILURE);
	}

	return NULL;
}


lista_pt *list_add(lista_pt **p, pthread_t i, int d) 
{
    lista_pt *n = (lista_pt *)malloc(sizeof(lista_pt));
    if (n == NULL)
        return NULL;
    n->next = *p;                                                                            
    *p = n;
    n->data1 = i;
	n->data2 = d;
    return n;
}

/* borrar cabeza*/ 
void list_remove(lista_pt **p) 
{ 
    if (*p != NULL) 
	{
        lista_pt *n = *p;
        *p = (*p)->next;
        free(n);
    }
}
 
lista_pt **list_search_d1(lista_pt **n, pthread_t i) 
{
    while (*n != NULL) 
	{
        if ((*n)->data1 == i) 
			return n;
        n = &(*n)->next;
    }
    return NULL;
}

lista_pt **list_search_d2(lista_pt **n, int i) 
{
    while (*n != NULL) 
	{
        if ((*n)->data2 == i) 
			return n;
        n = &(*n)->next;
    }
    return NULL;
}

void list_print(lista_pt *n) 
{
    if (n == NULL) {
        printf("lista esta vacía\n");
    }
    while (n != NULL) {
        printf("print %p %p %lu\n", n, n->next, n->data1);
        n = n->next;
    }
}

int list_len(lista_pt *s)
{
	int cont = 0;

	lista_pt *n = s;
	while (s != NULL)
	{
		n = n->next;
		cont+=1;
	}
	return cont;
}

