#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#define TAM 256

int main( int argc, char *argv[] ) 
{
	int sockfd, puerto, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[TAM];//, usuario[32];
	if ( argc < 2 ) 
	{
		fprintf( stderr, "Uso %s <host>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	puerto = 6666;
	
	/*if(strcpy(usuario,argv[2]) < 0)
	{
		perror("strcpy");
		exit(EXIT_FAILURE);
	}*/
 
	sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sockfd < 0 ) 
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	server = gethostbyname( argv[1] );
	if (server == NULL) 
	{
		fprintf( stderr,"Error, no existe el host\n" );
		exit(EXIT_FAILURE);
	}
	memset( &serv_addr, '0', sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length );
	serv_addr.sin_port = htons(puerto);
	if (connect( sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}

	printf("ingrese msj inicial: ");
	memset( buffer, '0', TAM );
	fgets( buffer, TAM-1, stdin );

	while (strcmp(buffer,"CTRL exit\n") != 0)
	{
		if (write(sockfd, buffer, strlen(buffer)) < 0) 
		{
			perror("write");
			exit(EXIT_FAILURE);
		}

		memset( buffer, '\0', TAM );
		
		if (read(sockfd, buffer, TAM) < 0)
		{
			perror("read");
			exit(EXIT_FAILURE);
		}
		printf("srv dice: %s\n", buffer);
		printf("usr: ");
		memset(buffer, '0', TAM);
		fgets(buffer, TAM-1, stdin);
	}
	puts("Hasta la proxima, baby..");
	exit(EXIT_SUCCESS);
} 
