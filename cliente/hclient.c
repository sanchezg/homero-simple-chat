#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "hclient.h"

int CLIENTE_ACTIVO;

int main( int argc, char *argv[] ) 
{
	int mi_socket;
	pthread_t th_stdin, th_socket;

	system("clear");

	if (argc < 2) 
	{
		fprintf( stderr, "Uso %s <host>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((mi_socket = conectar_servidor(argv[1])) < 0)
	{
		printf("Error al conectar a servidor, intente nuevamente.\n");
		exit(EXIT_FAILURE);
	}

	printf(MENU_MSJ);
	printf(PROMPT);


	if (pthread_create(&th_stdin, NULL, exec_th_stdin, (void *) &mi_socket) != 0)
	{
		printf ("ERROR PTHREAD CLIENTE\n");
		return -1;
	}

	if (pthread_create(&th_socket, NULL, exec_th_socket, (void *) &mi_socket) != 0)
	{
		printf ("ERROR PTHREAD CLIENTE\n");
		return -1;
	}

	CLIENTE_ACTIVO = ON;

	pthread_join(th_socket, NULL);
	pthread_join(th_stdin, NULL);

	puts("Hasta la proxima, baby..");
	close(mi_socket);
	exit(EXIT_SUCCESS);
}

void * exec_th_stdin(void *ptr)
{
	char buffer[TAM];
	int *iptr = (int *) ptr;
	int mi_socket = *iptr;

	while (CLIENTE_ACTIVO == ON)
	{
		memset(buffer, '\0', TAM);
		fgets(buffer, TAM, stdin);

		switch(verificar_msj(buffer))
		{
			case EXIT_CODE:
				CLIENTE_ACTIVO = OFF;
				write(mi_socket, buffer, TAM);
				continue;
			case MENU_CODE:
				system("clear");
				printf(MENU_MSJ);
				break;
			default:
				write(mi_socket, buffer, TAM);
		}
		printf(PROMPT);
	}
	return NULL;
}

void * exec_th_socket(void *ptr)
{
	char buffer[TAM];
	int *iptr = (int *) ptr;
	int mi_socket = *iptr;

	while (CLIENTE_ACTIVO == ON)
	{
		if (recv(mi_socket, buffer, TAM, MSG_DONTWAIT) > 0)
		{
			printf("MSJ recibido del server: %s\n", buffer);
		}

		usleep(100);
	}
	return NULL;
}

int verificar_msj(char *buf)
{
	char temp[TAM];
	strcpy(temp, buf);

	if (strcmp(temp, "exit\n") == 0)
		return EXIT_CODE;
	if (strcmp(temp, "menu\n") == 0)
		return MENU_CODE;
	return 0;
}

int conectar_servidor(char* hostname)
{
	int mi_socket;
	struct hostent *server;
	struct sockaddr_in serv_addr;

	if  ((mi_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	server = gethostbyname(hostname);

	if (server == NULL) 
		return -1;

	memset (&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy ((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PUERTO);

	if (connect (mi_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		return -1;
	return mi_socket;
}
