#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define TAM 256
#define MAX_QUE 10
#define EX_REGCLI 0
#define ERR_REGCLI -1
#define ERR_COM 1

int verificar_msj(char* entrada);
int registrar_usuario(char *nombre);
int archivo_agregar(char* nombre_archivo, char* texto);
int buscar(char *nombre, char *cadena);

int main( int argc, char *argv[] ) 
{
	int sockfd, newsockfd, puerto, clilen, pid;
	char buffer[TAM];
	char ipstr[INET_ADDRSTRLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, port;

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

	while (1)
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
			//port = ntohs(s->sin_port);
			//inet_ntop (AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

			while (strcmp(buffer,"CTRL exit\n") != 0)
			{
/*
				printf( "usr: %s", buffer);
				n = write( newsockfd, "OK", 18 );
				if ( n < 0 ) {
					perror("write");
					exit(EXIT_FAILURE);
				}
*/
				printf( "usr: %s:%d dice:%s", inet_ntop (AF_INET, &s->sin_addr, ipstr, sizeof ipstr), ntohs(s->sin_port), buffer);
				switch (verificar_msj(buffer))
				{
					case EX_REGCLI:
						write (newsockfd, "OK", 18);
						break;
					case ERR_REGCLI:
						write (newsockfd, "ERROR", 18);
						break;
					case ERR_COM:
						write (newsockfd, "ERROR COMANDO", 18);
						break;
				}
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
			close(newsockfd);
	}
	exit(EXIT_SUCCESS); 
}

int verificar_msj(char* entrada)
{
	char* ptr;
	if (strstr(entrada,"CTRL HOLA\n")!=NULL)
	{
		if (registrar_usuario(strtok(entrada," ")) < 0)
			return ERR_REGCLI;
		else
			return EX_REGCLI;
	}
/*	if (strstr(entrada,"CTRL LISTAR")!=NULL)
	{
		
	}
	if (strstr(entrada,"CTRL CHARLEMOS")!=NULL)
	{
		
	}
	if (strstr(entrada,"CTRL NO")!=NULL)
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
	if (buscar("clientes",nombre) == 0)
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

	fputs(texto,pf);
	fclose(pf);
	return 0;
}

int buscar(char *nombre, char *cadena)
{
	FILE *pf;
	char * buffer;
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

	if (strstr(buffer,cadena) == NULL)
	{
		fclose(pf);
		return 0;
	}
	else
	{
		fclose(pf); 
		return 1;
	}
	return -1;
}
