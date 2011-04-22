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

char* nombre;
char* MSJ_SALIDA;

int main( int argc, char *argv[] ) 
{
	int mi_socket;//, flag_msj_serv = OFF;
	char buffer[TAM], sel[2];

	system("clear");

	if (argc < 2) 
	{
		fprintf( stderr, "Uso %s <host>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((mi_socket = abrir_conexion()) < 0)
	{
		printf("Error al abrir socket, intente nuevamente.\n");
		exit(EXIT_FAILURE);
	}

	if (conectar_servidor(mi_socket, argv[1]) < 0)
	{
		printf("Error al conectar a servidor, intente nuevamente.\n");
		exit(EXIT_FAILURE);
	}

	printf(MENU_MSJ);
	printf(PROMPT);

	memset(buffer, ' ', TAM);
	fgets(buffer, TAM, stdin);

	while (strcmp(buffer,"exit\n") != 0)
	{
		if (write(mi_socket, buffer, strlen(buffer)) < 0) 
		{
			perror("write");
			exit(EXIT_FAILURE);
		}
	
		memset(buffer, ' ', TAM );
		if (read(mi_socket, buffer, TAM) < 0)
		{
			perror("read");
			exit(EXIT_FAILURE);
		}

		switch (verificar_msj(buffer))
		{
			case _MSJ_OK_:
				break;

			case EXITO_REG:
				printf("Registro exitoso.\n");
				memset(buffer, ' ', TAM);
				break;
			case ERROR_REG:
				printf("Registro rechazado. Msj servidor: %s\n", MSJ_SALIDA);
				memset(buffer, ' ', TAM);
				break;

			case ENTRO_ALGUIEN:
				printf("##: %s\n", buffer);
				break;

			case _CHAT_:
				printf("##: El cliente %s quiere comunicarse, acepta? [y][n]: ", MSJ_SALIDA);
				fgets(sel, 2, stdin);
				if (strcmp(sel,"y\n") == 0)
				{
					memset(buffer, ' ', TAM);
					strcpy(buffer, "_OK_");
					continue;
				}
				else
				{
					memset(buffer, ' ', TAM);
					strcpy(buffer, "_NO_");
					continue;
				}
				break;

			case EXITO_CHAT:
				printf("El cliente aceptó el chat.\n");
			case ERROR_CHAT:
				printf("No disponible\n");

			default:
				break;
		}

		memset(buffer, ' ', TAM);
		fgets(buffer, TAM, stdin);
	}

	puts("Hasta la proxima, baby..");
	if (write(mi_socket, buffer, strlen(buffer)) < 0) 
	{
		perror("write");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

int verificar_msj(char * buffer)
{
	char *temp;

	if (strstr(buffer, "_MSJ_OK_"))
		return _MSJ_OK_;

	if (strstr(buffer, "CTRL QUETAL"))
		return EXITO_REG;
	if (strstr(buffer, "CTRL FUERA"))
	{
		temp = strtok(buffer,"\"");
		//while(temp != NULL)
		temp = strtok(NULL,"\"");
		MSJ_SALIDA = temp;
		return ERROR_REG;
	}
	if (strstr(buffer, "CTRL DALE"))
		return EXITO_CHAT;
	if (strstr(buffer, "CTRL NO"))
		return ERROR_CHAT;
	
	if (strstr(buffer, "CTRL ENTRO"))
		return ENTRO_ALGUIEN;

	if (strstr(buffer, "_CHAT: "))
	{
		MSJ_SALIDA = strtok(buffer, " \n");
		MSJ_SALIDA = strtok(NULL, " \n");
		return _CHAT_;
	}

	return ERROR_MSJ;
}

int abrir_conexion()
{
/*	int mi_socket, sock_opt;

	mi_socket = socket (AF_INET, SOCK_STREAM, 0);
	sock_opt = fcntl(mi_socket, F_GETFL);
	if (fcntl(mi_socket, F_SETFL, O_NONBLOCK) )
		return -1;

	return mi_socket;
*/
	return socket (AF_INET, SOCK_STREAM, 0);
}

int conectar_servidor(int socket, char* hostname)
{
	struct hostent *server;
	struct sockaddr_in serv_addr;

	server = gethostbyname(hostname);

	if (server == NULL) 
		return -1;

	memset (&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy ((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PUERTO);

	if (connect (socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		return -1;
	return 0;
}
