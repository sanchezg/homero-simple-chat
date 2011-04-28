#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <fcntl.h>

#include "cliente.h"

//char* nombre;
//char* MSJ_SALIDA;


int main( int argc, char *argv[] ) 
{
	int mi_socket, fl_recv = OFF;
	char buffer[TAM];
	int CLIENTE_ACTIVO = ON;

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
/*
	memset(buffer, '\0', TAM);
	fgets(buffer, TAM, stdin);
*/
	while(CLIENTE_ACTIVO == ON)
	{
		if (fl_recv == OFF)
		{
			memset(buffer, '\0', TAM);
			fgets(buffer, TAM, stdin);
		}

		switch(verificar_msj(buffer))
		{
			case EXIT_CODE:
				CLIENTE_ACTIVO = OFF;
				continue;
			case MENU_CODE:
				system("clear");
				printf(MENU_MSJ);
				break;
			default:
				if (fl_recv == ON)
				{
					printf("MSJ recibido del server: %s", buffer);
					fl_recv = OFF;
				}
				else
					write(mi_socket, buffer, TAM);
				break;
		}
		printf(PROMPT);
		if (recv(mi_socket, buffer, TAM, MSG_DONTWAIT) > 0)
			fl_recv = ON;
	}

	puts("Hasta la proxima, baby..");
	close(mi_socket);
	exit(EXIT_SUCCESS);
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
