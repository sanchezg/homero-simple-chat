#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "serv_hilos.h"

int SERVER_ACTIVO;

int main(int argc, char *argv[]) 
{
	int des_servidor;
	
	pthread_t th_conex;
//	lista_pt *thread = NULL;

	des_servidor = iniciar_servidor(PUERTO);

	if (pthread_create(&th_conex, NULL, ejec_servidor, (void*) &des_servidor) != 0)
	{
		printf ("ERROR PTHREAD SERVIDOR\n");
		return -1;
	}

	/* Esperar a que termine el thread de ejecución. */
	pthread_join(th_conex, NULL);	

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

/*
	ejec_servidor se queda a la espera de conexiones
	mientras el server esté activo. Por cada conexión
	recibida, lanza un hilo que se encarga de ejecutar
	la rutina común al cliente.
*/

void* ejec_servidor(void *ptr)
{
	int *iptr, des_servidor, des_cliente;
	lista_pt *thread = NULL;

	iptr = (int *) ptr;
	des_servidor = *iptr;

	while(SERVER_ACTIVO)
	{
		des_cliente=aceptar_conexion(des_servidor);
		if (nuevo_cliente(&thread, des_cliente) < 0)
			exit(EXIT_FAILURE);
	}
	return NULL;
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
		printf ("ERROR PTHREAD CLIENTE\n");
		return -1;
	}
	list_add(n, new, dsc);
	return 0;
}

/*
	ejec_cliente es la rutina que realiza el hilo
	que se encarga de cada cliente. Tiene que revisar
	continuamente los msj que envía el cliente, y
	mandarlos al hilo del servidor.
*/

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
	if (read(isc, buffer, TAM-1) < 0) 
	{
		perror("read");
		exit(EXIT_FAILURE);
	}

	while ((strstr(buffer,"CTRL exit\n") == NULL) && (getpeername(isc, (struct sockaddr*) &cli_addr, &clilen) == 0))
	{
		printf("%s:%d say:%s \n", inet_ntop (AF_INET, &s->sin_addr, ipstr, sizeof ipstr), ntohs(s->sin_port), buffer);
		write(isc, "QUETAL", 6);
		memset(buffer, 0, TAM);
		if (read(isc, buffer, TAM-1) < 0) 
		{
			perror("read");
			exit(EXIT_FAILURE);
		}
	}

	printf("hilo %lu finalizado\n", pthread_self());
	return NULL;
}


/*
	Funciones de control de la lista de clientes.
*/

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

