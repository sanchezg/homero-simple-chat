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
	int mi_socket, flag_stdin = OFF;
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

	memset(buffer, '\0', TAM);
	fgets(buffer, TAM-1, stdin);
	send(mi_socket, buffer, TAM-1, 0);

	while (strcmp(buffer,"exit\n") != 0)
	{
		if (flag_stdin == ON)
			send(mi_socket, buffer, TAM-1, MSG_DONTWAIT);
		memset(buffer, '\0', TAM-1);
		sleep(1);
		recv(mi_socket, buffer, TAM-1, MSG_DONTWAIT);

		switch (verificar_msj(buffer))
		{
			case _OK_:
				break;
			case EXITO_REG:
				printf("# Registro exitoso.\n");
				memset(buffer, '\0', TAM);
				break;
			case ERROR_REG:
				printf("# Registro rechazado. SRV: %s\n", MSJ_SALIDA);
				memset(buffer, '\0', TAM);
				break;

			case ENTRO_ALGUIEN:
				printf("# %s ha entrado.\n", MSJ_SALIDA);
  				break;

			case ERR_CL_NO:
				printf("# Cliente no registrado\n");
				break;

			case SOLIC_CHAT:
				printf("# El cliente %s quiere comunicarse, acepta? [y][n]: ", MSJ_SALIDA);
				fgets(sel, 2, stdin);
				if (sel[0] == 'y')
				{
					memset(buffer, '\0', TAM);
					strcpy(buffer, "_OK_");
					write(mi_socket, buffer, TAM);
					continue;
				}
				else
				{
					memset(buffer, '\0', TAM);
					strcpy(buffer, "_NO_");
					write(mi_socket, buffer, TAM);
					continue;
				}
				break;

			case EXITO_CHAT:
				printf("El cliente aceptó el chat.\n");
				break;
			case ERROR_CHAT:
				printf("No disponible\n");
				break;

			case CTRL_MSJ:
				write(mi_socket, buffer, TAM);
				flag_stdin = ON;
				break;

			case EXIT_CODE:
				write(mi_socket, buffer, TAM);
				flag_stdin = ON;
				continue;
				break;
			case MENU_CODE:
				printf(MENU_MSJ);
				flag_stdin = ON;
				break;

			default:
				printf("%s", buffer);
				break;
		}

		if (flag_stdin == ON)
			flag_stdin = OFF;
		memset(buffer, '\0', TAM-1);
		printf(PROMPT);
		//fgets(buffer, TAM-1, stdin);
		sleep(1);
		if (recv(fileno(stdin), buffer, TAM-1, MSG_DONTWAIT) > 0)
			flag_stdin = ON;
	}

	puts("Hasta la proxima, baby..");
	send(mi_socket, buffer, TAM-1, MSG_DONTWAIT);
	close(mi_socket);
	exit(EXIT_SUCCESS);
}

int verificar_msj(char * buffer)
{
	char *temp = malloc(sizeof(char[32]));

	/* MSJ que vienen del servidor */
	if (strstr(buffer, "CTRL QUETAL"))
		return EXITO_REG;
	if (strstr(buffer, "CTRL FUERA"))
	{
		memset(temp, '\0', 32);
		temp = strtok(buffer,"\"");
		temp = strtok(NULL,"\"");
		strcpy(MSJ_SALIDA, temp);
		return ERROR_REG;
	}

	if (strstr(buffer, "CTRL ENTRO"))
	{
		memset(temp, '\0', 32);
		temp = strtok(buffer,"\"");
		temp = strtok(NULL,"\"");
		strcpy(MSJ_SALIDA, temp);
		return ENTRO_ALGUIEN;
	}

	if (strstr(buffer, "_CHAT: "))
	{
		memset(temp, '\0', 32);
		temp = strtok(buffer,"\"");
		temp = strtok(NULL,"\"");
		strcpy(MSJ_SALIDA, temp);
		return SOLIC_CHAT;
	}

	/* MSJ del servidor que son rptas a otros */
	if (strcmp(buffer, "CTRL DALE\n"))
		return EXITO_CHAT;
	if (strstr(buffer, "CTRL_NO"))
		return ERROR_CHAT;
	if (strstr(buffer, "_CL_NO_"))
		return ERR_CL_NO;
	if (strstr(buffer, "_OK_"))
		return _OK_;

	/* MSJ que vienen de stdin */
	if(strstr(buffer, "CTRL HOLA"))
		return CTRL_MSJ;

	if(strstr(buffer, "CTRL LISTAR\n"))
		return CTRL_MSJ;

	if(strstr(buffer, "CTRL CHARLEMOS"))
		return CTRL_MSJ;

	if (strstr(buffer, "MSG"))
		return CTRL_MSJ;

	/* MSJ comandos del cliente */
	if (strstr(buffer, "exit"))
		return EXIT_CODE;
	if (strstr(buffer, "menu"))
		return MENU_CODE;

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
