#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#include "servidor.h"

int main(int argc, char *argv[]) 
{
	int sockfd, newsockfd, puerto, pid;
	size_t clilen;
	char buffer[TAM], buffer_compartido[TAM];//, com_control[32];
	char ipstr[INET_ADDRSTRLEN];
	struct sockaddr_in serv_addr, cli_addr;

	time_t rawtime;
	struct tm * fechahora;

	//int n, port;

	/* if ( argc < 2 ) {
        	fprintf( stderr, "Uso: %s <puerto>\n", argv[0] );
		exit( 1 );
	} */

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{ 
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset( &serv_addr, 0, sizeof(serv_addr));
	puerto = 6666;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons( puerto );

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}
	printf ("srv disponible, puerto %d\n", ntohs(serv_addr.sin_port));

	if(listen (sockfd, MAX_QUE) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	clilen = sizeof(cli_addr);

//	while(strcmp(com_control,"exit\n") != 0)
	while(1)
	{
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if ( newsockfd < 0 ) 
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}

		pid = fork();
		if (pid < 0)
		{
			perror("fork");
			exit(EXIT_FAILURE);
		}

		if (pid == 0) 
		{
			// Proceso hijo, esto se encarga del socket cliente!

			close(sockfd);	//no se porque el close()

			memset(buffer, 0, TAM);
			if (read(newsockfd, buffer, TAM-1) < 0) 
			{
				perror("read");
				exit(EXIT_FAILURE);
			}


			getpeername(newsockfd, (struct sockaddr*) &cli_addr, &clilen);

			struct sockaddr_in *s = (struct sockaddr_in *) &cli_addr;

			while (strcmp(buffer,"CTRL exit\n") != 0)
			{
				printf("%s:%d say:%s", inet_ntop (AF_INET, &s->sin_addr, ipstr, sizeof ipstr), ntohs(s->sin_port), buffer);
				switch (verificar_msj(buffer))
				{
					case EX_REGCLI:
						write (newsockfd, "QUETAL", 18);
						memset(buffer_compartido, 0, TAM);
						time(&rawtime);
						fechahora=localtime(&rawtime);
						strcat(buffer_compartido,(strcat(strcat(strcat(buffer_compartido,"CLIREG|"),ipstr),asctime(fechahora))));
//						write(newsockfd,strcat(strcat(strcat(buffer_compartido,"CLIREG|"),ipstr),asctime(fechahora)),32);
						break;
					case LISTAR_CLI:
						write(newsockfd, archivo_listar("clientes"), TAM);
						break;
					case ERR_REGCLI:
						write (newsockfd, "ERROR", 18);
						break;
					case ERR_COM:
						write (newsockfd, "ERROR COMANDO", 18);
						break;
				}

				memset(buffer, 0, TAM);
				if (read(newsockfd, buffer, TAM-1) < 0) 
				{
					perror("read");
					exit(EXIT_FAILURE);
				}
			}
			// Finalizo la ejecucion del cliente..
			printf("El cliente ha dejado la conexion\n");
			exit(EXIT_SUCCESS);
		}
		else
		{
			//Proceso servidor, compartir recursos..
			close(newsockfd);

			printf(buffer_compartido);

/*			//memset(buffer_compartido, 0, TAM);
			if (read(newsockfd, buffer_compartido, TAM-1) < 0) 
			{
				perror("read");
				exit(EXIT_FAILURE);
			}
			printf(buffer_compartido);
			close(newsockfd);
*/		}
	}
	exit(EXIT_SUCCESS); 
}

int verificar_msj(char* entrada)
{
	//char* ptr;
	if (strstr(entrada,"CTRL HOLA\n")!=NULL)
	{
		if (registrar_usuario(strtok(entrada," ")) < 0)
			return ERR_REGCLI;
		else
			return EX_REGCLI;
	}
	if (strstr(entrada,"CTRL LISTAR\n")!=NULL)
		return LISTAR_CLI;
/*	if (strstr(entrada,"CTRL CHARLEMOS")!=NULL)
	{
		
	}
*/
/*	if (strstr(entrada,"CTRL NO")!=NULL)
	{
		
	}
	if (strstr(entrada,"MSG")!=NULL)
	{
		
	}
	if (strstr(entrada,"CTRL SILENCIO")!=NULL)
	{
		
	}
*/
	else
		return ERR_COM;
}

int registrar_usuario(char *nombre)
{
	if (archivo_buscar("clientes",nombre) == 0)
	{
		if(archivo_agregar("clientes", nombre) < 0)
			return -1;
		return 0;
	}
	else
		return -1;
}

int archivo_agregar(char* nombre_archivo, char* texto)
{
	FILE *pf;

	if ((pf=fopen(nombre_archivo, "a")) == NULL) 
	{
		fclose(pf);
		return -1;		
	}

	fputs(strcat(texto,"\n"),pf);
	fclose(pf);
	return 0;
}

int archivo_buscar(char *nombre, char *cadena)
{
//	FILE *pf;
	char * buffer;
/*
	long lsize;
	size_t result;

	pf = fopen(nombre, "r");
	if (pf==NULL) 
	{
		fclose(pf);
		return -1;		
	}

	fseek (pf, 0, SEEK_END);
  	lsize = ftell (pf);
	rewind (pf);

	buffer = (char*) malloc (sizeof(char)*lsize);
	if (buffer == NULL)
	{
		fclose(pf);
		return -1;
	}

	result = fread (buffer,1,lsize,pf);
	if (result != lsize)
	{
		fclose(pf);
		return -1;
	}
*/
	buffer = archivo_listar(nombre);

	if (strstr(buffer,cadena) == NULL)
	{
		//fclose(pf);
		return 0;
	}
	else
	{
		//fclose(pf); 
		return 1;
	}
	return -1;
}

char* archivo_listar(char* nombre)
{
	FILE *pf;
	char * buffer;
	long lsize;
	size_t result;

	pf = fopen(nombre, "r");
	if (pf==NULL) 
	{
		fclose(pf);
		return NULL;		
	}

	fseek (pf, 0, SEEK_END);
  	lsize = ftell (pf);
	rewind (pf);

	buffer = (char*) malloc (sizeof(char)*lsize);
	if (buffer == NULL)
	{
		fclose(pf);
		return NULL;
	}

	result = fread (buffer,1,lsize,pf);
	if (result != lsize)
	{
		fclose(pf);
		return NULL;
	}
	
	return buffer;
}
