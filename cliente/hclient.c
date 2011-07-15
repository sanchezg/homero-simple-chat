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
char* MSJ_RETORNO;
char* NICKNAME;

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

	MSJ_RETORNO = malloc(sizeof(char)*TAM);
	NICKNAME = malloc(sizeof(char)*32);

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
			case EXITO_CHAT:
				write(mi_socket, MSJ_RETORNO, TAM);
				break;
			case ERROR_CHAT:
				write(mi_socket, "CHAT_NO\n", TAM);
				break;
			default:
				write(mi_socket, buffer, TAM);
				break;
		}
		printf(PROMPT);
		usleep(100);
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
			switch(verificar_msj(buffer))
			{
				case CHAT_CODE:
					break;
				default:
					printf("MSJ recibido del server: %s", buffer);
					break;
			}
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

	if (strstr(temp, "CTRL HOLA") != NULL)
	{
		memset(NICKNAME, '\0', 32);
		strcpy(NICKNAME, strtok(temp, " "));
		return 0;
	}

	/* Se devuelve al servidor el msj: <DESTINO>|CHAT_OK|<ORIGEN>*/
	if (strcmp(temp, "SI\n") == 0)
	{
		memset(temp, '\0', TAM);
		strcpy(temp, MSJ_RETORNO);
		strcat(temp, "|CHAT_OK|");
		strcat(temp, NICKNAME);
		strcat(temp, "\n");
		memset(MSJ_RETORNO, '\0', TAM);
		strcpy(MSJ_RETORNO, temp);
		return EXITO_CHAT;
	}
	if (strcmp(temp, "NO\n") == 0)
		return ERROR_CHAT;

	if (strstr(temp, "_CHAT|") != NULL)
	{
		memset(MSJ_RETORNO, '\0', TAM);
		strtok(temp, "|");
		strcpy(temp, strtok(NULL, "\n"));
		strcpy(MSJ_RETORNO, temp);
		printf("El usuario %s quiere iniciar chat, acepta? [SI|NO]\n", temp);
		return CHAT_CODE;
	}
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
