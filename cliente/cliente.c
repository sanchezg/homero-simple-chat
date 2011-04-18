#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>

#include "cliente.h"

char* nombre;
char* MSJ_SALIDA;

int main( int argc, char *argv[] ) 
{
	int mi_socket, flag_msj_serv = 0;
	char buffer[TAM];

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

	printf("Ingrese MSG inicial: ");
/*	printf("\t\tBienvenido a Homero-Simple-Chat.\n\tIngrese un comando:\
			\n\t<nickname> CTRL HOLA: Se registra con el servidor.\
			\n\tCTRL LISTAR: Muestra los usuarios conectados.\
			\n\tCTRL CHARLEMOS \"destino\": Trata de iniciar una conversación con usuario \"destino\"\
			\n\tMSG <destino> \"Mensaje\": Transmite \"Mensaje\" al usuario <destino>\
			(Es necesario haber iniciado la conversación antes)."\
			\n\tmenu: muestra este menú.\
			\n\texit: sale del programa.");
*/
	memset(buffer, '0', TAM);
	fgets(buffer, TAM-1, stdin);

	while (strcmp(buffer,"exit\n") != 0)
	{
		if (flag_msj_serv == 0)
		{
			if (write(mi_socket, buffer, strlen(buffer)) < 0) 
			{
				perror("write");
				exit(EXIT_FAILURE);
			}

			memset (buffer, '\0', TAM );
		
			if (read(mi_socket, buffer, TAM) < 0)
			{
				perror("read");
				exit(EXIT_FAILURE);
			}
		}

		switch (verificar_msj(buffer))
		{
			case EXITO_REG:
				printf("Registro exitoso.\n");
				memset(buffer, '0', TAM);
				break;
			case ERROR_REG:
				printf("Registro rechazado. Msj servidor: %s\n", MSJ_SALIDA);
				memset(buffer, '0', TAM);
				break;
			case ENTRO_ALGUIEN:
				printf("## servidor: %s\n", buffer);
				break;
			default:
				break;
		}

		//verificar si el servidor mandó algún msj...
/*		memset(buffer, '0', TAM);
		if (recv (mi_socket, buffer, TAM, 0) != -1)
		{
			flag_msj_serv = 1;
			continue;
		}*/

//		printf("srv: %s\n", buffer);
//		printf("usr: ");
		//sino esperar por la entrada de teclado, y seguir...
		memset(buffer, '0', TAM);
		fgets(buffer, TAM-1, stdin);
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

	if (strstr(buffer, "CTRL QUETAL\n"))
		return EXITO_REG;
	if (strstr(buffer, "CTRL FUERA"))
	{
		temp = strtok(buffer,"\"");
		while(temp != NULL)
			temp = strtok(NULL,"\"");
		MSJ_SALIDA = temp;
		return ERROR_REG;
	}
	if (strstr(buffer, "CTRL ENTRO"))
		return ENTRO_ALGUIEN;
	return ERROR_MSJ;
}

int abrir_conexion()
{
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
