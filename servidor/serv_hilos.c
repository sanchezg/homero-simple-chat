#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <regex.h>
#include <sys/msg.h>
#include "serv_hilos.h"

int SERVER_ACTIVO, ID_COLA;
char * ERROR_MSJ;
key_t K_COLA;
elem_cola COLA_GRAL;

pthread_mutex_t mutex_archivo_clientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_write_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listapt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_archivo_log = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cola_msj = PTHREAD_MUTEX_INITIALIZER;

lista_pt *thread = NULL;

int main(int argc, char *argv[]) 
{
	int des_servidor;
	
	pthread_t th_conex;
//	lista_pt *thread = NULL;

	system("clear"); 

	des_servidor = iniciar_servidor(PUERTO);

	if (pthread_create(&th_conex, NULL, ejec_servidor, (void*) &des_servidor) != 0)
	{
		printf ("ERROR PTHREAD SERVIDOR\n");
		return -1;
	}

	/* Esperar a que termine el thread de ejecución. */
	pthread_join(th_conex, NULL);	
/*
	archivo_borrar("clientes");
	archivo_borrar("serv.log");
	archivo_borrar("chat.log");
*/
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

	/*printf ("## creando logs...\n");
	if ((archivo_crear("clientes") != EXITO) || (archivo_crear("serv.log") != EXITO) || (archivo_crear("chat.log") != EXITO))
	{
		printf("ERROR creando logs!!!\n");
		return -1;
	}*/

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
	//	lista_pt *thread = NULL;

	if(iniciar_cola() == ERROR)
	{
		printf("Error al abrir cola de mensajes. Abortado\n");
		exit(EXIT_FAILURE);
	}

	iptr = (int *) ptr;
	des_servidor = *iptr;

	while(SERVER_ACTIVO)
	{
		des_cliente=aceptar_conexion(des_servidor);
		if (nuevo_cliente(&thread, des_cliente) < 0)
			exit(EXIT_FAILURE);
		list_print(thread);
	}
	return NULL;
}

int iniciar_cola()
{
	K_COLA = ftok ("/bin/ls", 33);		//Primero obtengo un KEY para crear la Q
	if (K_COLA == (key_t)-1)
		return ERROR;

	ID_COLA = msgget (K_COLA, 0600 | IPC_CREAT);	//Segundo obtengo el ID de la Q creada
	if (ID_COLA == -1)
		return ERROR;

	return EXITO;
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
	pthread_mutex_lock(&mutex_listapt);
	list_add(n, new, dsc);
	pthread_mutex_unlock(&mutex_listapt);
	return 0;
}

/*
	ejec_cliente es la rutina que realiza el hilo que se encarga de cada cliente.
	Tiene que revisar continuamente los msj que envía el cliente, y tratarlos según sea.
*/

void *ejec_cliente(void *ptr)
{
	int *iptr, mi_descriptor, flag_pendiente=OFF;
	size_t clilen;
	struct sockaddr_in cli_addr;
	struct sockaddr_in* s;
	char ipstr[INET_ADDRSTRLEN], buffer[TAM], resp_servidor[TAM], buffer_envio[TAM];
	char* tmp;

	iptr = (int *) ptr;
	mi_descriptor = *iptr;

	clilen = sizeof(cli_addr);
	s = (struct sockaddr_in *) &cli_addr;
	getpeername(mi_descriptor, (struct sockaddr*) &cli_addr, &clilen);
	
	printf("hilo %lu del cliente %s\n", pthread_self(), inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr));

	memset(buffer, '\0', TAM);
	if (read(mi_descriptor, buffer, TAM-1) < 0) 
	{
		perror("read");
		exit(EXIT_FAILURE);
	}
	
	do
	{
		printf("%s:%d say:%s", inet_ntop (AF_INET, &s->sin_addr, ipstr, sizeof ipstr), ntohs(s->sin_port), buffer);

		memset(resp_servidor, '\0', TAM);

		switch (verificar_msj(buffer, mi_descriptor))
		{
			case EXITO_REG_CLIENTE:
				strcat(resp_servidor, "CTRL QUETAL");
				//broadcast_clientes("CTRL ENTRO\n");
				break;
			case ERROR_REG_CLIENTE:
				strcat(resp_servidor, "CTRL FUERA ");
				strcat(resp_servidor, ERROR_MSJ);
				break;

			case EXITO_CL_CHARL:
				tmp = strtok(buffer,"\"");
				tmp = strtok(NULL,"\"");						

				strcat(buffer_envio, (char*) mi_descriptor);
				strcat(buffer_envio, " - ");
				strcat(buffer_envio, (char*) obtener_id_nombre(tmp));
				/*if(loguear("chat.log", buffer_envio) == ERROR)
				{
					printf("Error logueando: %s", ERROR_LOG);
					strcat(resp_servidor,"ERROR");
					break;
				}*/

				strcat(resp_servidor,"CTRL DALE \" "); 
				strcat(resp_servidor, tmp);
				strcat(resp_servidor, " \"");
				break;
			case ERROR_CL_CHARL:
				tmp = strtok(buffer,"\"");
				tmp = strtok(NULL,"\"");

				strcat(resp_servidor, "CTRL NO \" ");
				strcat(resp_servidor, tmp);
				strcat(resp_servidor, " \"");
				//loguear(ERROR_MSJ);
				break;
			case ERROR_CLIENTE_INC:
				strcat(resp_servidor, "_CL_NO_");
				//loguear(ERROR_MSJ);
				break;

			case EXITO_VER_CONV:
				tmp = strtok(buffer,"\"");
				tmp = strtok(NULL,"\"");
				strcat(buffer_envio, tmp);
				if(mandar_msj(mi_descriptor, obtener_id_nombre(tmp), buffer_envio) == EXITO)
					strcat(resp_servidor,"_MSG_OK_");
				else
					strcat(resp_servidor,"Error al enviar msj. Intente nuevamente.");
			case ERROR_VER_CONV:
				strcat(resp_servidor, ERROR_MSJ);

			default:
				strcat(resp_servidor, "ERROR desconocido");
				break;
		}
		ERROR_MSJ = " ";

		/* Verificar si hay msj en la cola para mi. */
		pthread_mutex_lock(&mutex_cola_msj);
		if (msgrcv (ID_COLA, (struct msgbuf *)&COLA_GRAL, sizeof(COLA_GRAL.msj), mi_descriptor, IPC_NOWAIT) > 0)
		{
			pthread_mutex_unlock(&mutex_cola_msj);
			flag_pendiente = ON;
			strcpy(buffer, COLA_GRAL.msj);
			printf("Msj pendiente de tipo 1\n");
			continue;
		}

		if (msgrcv (ID_COLA, (struct msgbuf *)&COLA_GRAL, sizeof(COLA_GRAL.msj), mi_descriptor*10, IPC_NOWAIT) > 0)
		{
			pthread_mutex_unlock(&mutex_cola_msj);
			flag_pendiente = ON;
			strcpy(buffer, COLA_GRAL.msj);
			printf("Msj pendiente de tipo 10\n");
			continue;
		}
		pthread_mutex_unlock(&mutex_cola_msj);

		printf("No hay msj pendientes\n");

		if(flag_pendiente == ON)
		{
			flag_pendiente = OFF;
			strcat(buffer, "\r\n");
			write(mi_descriptor, buffer, TAM);			
		}
		else
		{
			strcat(resp_servidor, "\r\n");
			write(mi_descriptor, resp_servidor, TAM);
			printf("Hizo el write de %s \n",resp_servidor);
		}

		memset(buffer, '\0', TAM);
		if (read(mi_descriptor, buffer, TAM) < 0) 
		{
			perror("read");
			exit(EXIT_FAILURE);
		}

	}
	while ((strcmp(buffer,"exit\n") != 0) && (getpeername(mi_descriptor, (struct sockaddr*) &cli_addr, &clilen) == 0));

	printf("hilo %lu finalizado\n", pthread_self());
	list_remove(list_search_d2(&thread, mi_descriptor));
	return NULL;
}

/* Aca se utiliza source sock fd para cuestiones que relacionan dos clientes (conversación) */
int verificar_msj(char * buffer_entrada, int ssock)
{
	char* temp;
	char msjtemp[16];
	int i, dsock;

	/* Primero verificar por registro */
	if (strstr(buffer_entrada,"CTRL HOLA") != NULL)
	{
		if (registrar_usuario(ssock, strtok(buffer_entrada," ")) == EXITO)
			return EXITO_REG_CLIENTE;
		else
			return ERROR_REG_CLIENTE;
	}

	/* Verificar por inicio chat */
	if (strstr(buffer_entrada,"CTRL CHARLEMOS") != NULL)
	{
		temp = strtok(buffer_entrada,"\"");
		temp = strtok(NULL,"\"");
		strcpy(msjtemp, temp);

		pthread_mutex_lock(&mutex_archivo_clientes);

		if (archivo_buscar("clientes", msjtemp) == EXITO)
		{
			pthread_mutex_unlock(&mutex_archivo_clientes);
			dsock = obtener_id_nombre(msjtemp);
			if(cliente_charlemos(ssock, dsock) == EXITO)
				return EXITO_CL_CHARL;
			else
				return ERROR_CL_CHARL;
		}
		else
		{
			pthread_mutex_unlock(&mutex_archivo_clientes);
			ERROR_MSJ = "No se encuentra el cliente\n";
			return ERROR_CLIENTE_INC;
		}
	}

	/* Verificar por envío de msg */
	if (strstr(buffer_entrada, "MSG") != NULL)
	{
		i = 0;
		temp = strtok(buffer_entrada," ");
		while(temp != NULL && i<2)
		{
			temp = strtok(NULL," ");
			i++;
		}
		if (verificar_conversacion(ssock, obtener_id_nombre(temp)) == EXITO)
			return EXITO_VER_CONV;
		else
		{
			ERROR_MSJ = "Error al tratar de conversar";
			return ERROR_VER_CONV;
		}
	}
	return 0;
}

// busco en el log si esta habilitada la conversacion
int verificar_conversacion(int s_origen, int s_dest)
{
    char* buffer;
	char buscar[TAM];
    FILE *pf;
    
    if ((pf = fopen("chat.log", "r")) == NULL)
        return ERROR;

    buffer = archivo_listar("chat.log");
    if (buffer == NULL)
        return ERROR;

	memset(buscar, '\0', TAM);
	strcat(buscar, (char*) s_origen);
	strcat(buscar, " - ");
	strcat(buscar, (char*) s_dest);

	if (strstr(buffer, buscar) == NULL)
		return ERROR_VER_CONV;
	else
		return EXITO_VER_CONV;

	return ERROR;
}

//poner el msj en la cola, con tipo==cliente_destino
int mandar_msj(int origen, int destino, char* mensaje)
{
	char buffer[TAM];

	memset(buffer, '\0', TAM);
	strcat(buffer, mensaje);

	pthread_mutex_lock(&mutex_cola_msj);
	COLA_GRAL.tipo = destino*10;		//destino*10 indica que el msj es de chat
	strcpy(COLA_GRAL.msj, buffer);
	if(msgsnd(ID_COLA, (struct msgbuf *) &COLA_GRAL, sizeof(COLA_GRAL.msj), IPC_NOWAIT) != 0)
	{
		pthread_mutex_unlock(&mutex_cola_msj);
		return ERROR;
	}
	pthread_mutex_unlock(&mutex_cola_msj);
	return EXITO;
}

//Estas dos siguientes no se si andan!!
int obtener_id_nombre(char* nombre)
{
	lista_pt **n = malloc(sizeof(lista_pt*));

	if ((n = list_search_d3(&thread, nombre)) == NULL)
		return -1;
	else
	{
//		printf("dentro del obtener_id, ret: %d\n", (*n)->_id_socket_);
		return ((*n)->_id_socket_);
	}
}

char* obtener_nombre_id(int id)
{
	lista_pt **n = malloc(sizeof(lista_pt));

	if ((n = list_search_d2(&thread, id)) == NULL)
		return NULL;
	else
		return ((char*) &(*n)->_nombre_);
}

/*	
	Preguntar al cliente destino si quiere charlar. 
	Envia un msj a la que con la pregunta, y espera por la rpta. 
*/
int cliente_charlemos(int cl_orig, int cl_dest)
{
	char *nombre_origen;
	char buffer[TAM];

	memset(buffer, '\0', TAM);
	nombre_origen = obtener_nombre_id(cl_orig);
	strcpy(buffer, "_CHAT: ");
	strcat(buffer, nombre_origen);
	strcat(buffer, "\n");

	pthread_mutex_lock(&mutex_cola_msj);
	COLA_GRAL.tipo = cl_dest;
	strcpy(COLA_GRAL.msj, buffer);
	msgsnd(ID_COLA, (struct msgbuf *) &COLA_GRAL, sizeof(COLA_GRAL.msj), IPC_NOWAIT);		//Primero enviar la preugnta si quiere charlar ...
	pthread_mutex_unlock(&mutex_cola_msj);

	usleep(100000); //dormir cien miliseg
	pthread_mutex_lock(&mutex_cola_msj);
	while (msgrcv(ID_COLA, (struct msgbuf *) &COLA_GRAL, sizeof(COLA_GRAL.msj), cl_orig, IPC_NOWAIT) < 0)
	{
		//es porque no hay msg todavia
		pthread_mutex_unlock(&mutex_cola_msj);
		usleep(100000); //esperar cien miliseg
		pthread_mutex_lock(&mutex_cola_msj);
	}

	//salió es porque ya recibió el msj...
	memset(buffer, '\0', TAM);
	strcpy(buffer, COLA_GRAL.msj);
	pthread_mutex_unlock(&mutex_cola_msj);

	if (strstr(buffer, "_OK_"))
		return EXITO;
	ERROR_MSJ = "El cliente rechazó la conversación\n";
	return ERROR;
}

int registrar_usuario(int id, char *nombre)
{
/*	regex_t regex;
    int reti;
	char msgbuf[100]; */
/*
	reti = regcomp(&regex, "[[:alnum:]]", 0);
    if (reti)
	{
		ERROR_MSJ = "\"Error desconocido\"";
		return ERROR;
	}
	reti = regexec(&regex, nombre, 0, NULL, 0);
	if(reti == REG_NOMATCH)
	{
		ERROR_MSJ = "\"Error, sólo se aceptan caracteres alfanuméricos.\"";
		return ERROR;
	}
	else 
	{
		regerror(reti, &regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "Regex match failed: %s\n", msgbuf);
		return ERROR;
	}
	regfree(&regex);
*/
	//Si llego hasta acá es porque el nombre está bien...
	//Hay que bloquear el archivo para saber que no lo va a usar otro thread...
	pthread_mutex_lock(&mutex_archivo_clientes);
	if (archivo_buscar("clientes",nombre) == ERROR) //pregunto por error porque no quiero que este en el archivo...
	{
		if(archivo_agregar("clientes", nombre) == EXITO)
		{
			pthread_mutex_unlock(&mutex_archivo_clientes);
			pthread_mutex_lock(&mutex_listapt);
			if(lista_add_nombre(id, nombre) == ERROR)
				return ERROR;
			pthread_mutex_unlock(&mutex_listapt);
			return EXITO;
		}
		else
		{
			ERROR_MSJ = "archivo_agregar()";
			pthread_mutex_unlock(&mutex_archivo_clientes);
			return ERROR;
		}
	}
	pthread_mutex_unlock(&mutex_archivo_clientes);
	ERROR_MSJ = "\"Error registrando usuario, el nombre ya está siendo usado\"";
	return ERROR;
}

/* Funciones que tratan con archivos explícitamente */

int archivo_buscar(char *nombre, char *cadena)
{
	char * buffer;

	buffer = archivo_listar(nombre);

	if (strstr(buffer,cadena) != NULL)
		return EXITO;
	else
		return ERROR;
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
	fclose(pf);
	return buffer;
}

int archivo_agregar(char* nombre_archivo, char* texto)
{
	FILE *pf;

	if ((pf=fopen(nombre_archivo, "a")) == NULL) 
	{
		fclose(pf);
		return ERROR;		
	}

	fputs(strcat(texto,"\n"),pf);
	fclose(pf);
	return EXITO;
}

void broadcast_clientes(char *msj)
{	
	lista_pt *n = (lista_pt *) malloc(sizeof(lista_pt));
	n = thread;
	while (n != NULL)
	{
		write(n->_id_socket_, msj, TAM);
		n = n->_next_;
	}
}

/*
	Funciones de control de la lista de clientes.
*/

lista_pt *list_add(lista_pt **p, pthread_t i, int d) 
{
    lista_pt *n = (lista_pt *)malloc(sizeof(lista_pt));
    if (n == NULL)
        return NULL;
    n->_next_ = *p;
    *p = n;
    n->_id_thread_ = i;
	n->_id_socket_ = d;
    return n;
}

void list_remove(lista_pt **p) 
{ 
    if (*p != NULL) 
	{
        lista_pt *n = *p;
        *p = (*p)->_next_;
        free(n);
    }
}
 
lista_pt **list_search_d1(lista_pt **n, pthread_t i) 
{
    while (*n != NULL) 
	{
        if ((*n)->_id_thread_ == i) 
			return n;
        n = &(*n)->_next_;
    }
    return NULL;
}

lista_pt **list_search_d2(lista_pt **n, int i) 
{
    while (*n != NULL) 
	{
        if ((*n)->_id_socket_ == i) 
			return n;
        n = &(*n)->_next_;
    }
    return NULL;
}

lista_pt **list_search_d3(lista_pt **n, char* nom) 
{
    while (*n != NULL) 
	{
        if (strstr((*n)->_nombre_, nom) != NULL) 
			return n;
        n = &(*n)->_next_;
    }
    return NULL;
}

int lista_add_nombre(int id, char* nombre)
{
	lista_pt *n = (lista_pt *)malloc(sizeof(lista_pt));

	if ((n = *list_search_d2(&thread, id)) == NULL)
		return ERROR;
	n->_nombre_ = malloc(sizeof(char[16]));
	strcpy(n->_nombre_, nombre);
	return EXITO;
}

void list_print(lista_pt *n) 
{
    if (n == NULL) {
        printf("lista esta vacía\n");
    }
    while (n != NULL) {
        printf("print %p %p %lu %d\n", n, n->_next_, n->_id_thread_, n->_id_socket_);
        n = n->_next_;
    }
}

int list_len(lista_pt *s)
{
	int cont = 0;

	lista_pt *n = s;
	while (s != NULL)
	{
		n = n->_next_;
		cont+=1;
	}
	return cont;
}

int log_evento(char *ev, char *msj)
{
	time_t rawtime;
	struct tm * timeinfo;
	char log_msj[256];
	FILE *pf;

	time(&rawtime);
	timeinfo = localtime (&rawtime);

	strcat(log_msj, asctime(timeinfo));
	strcat(log_msj, "\t");
	strcat(log_msj, ev);
	strcat(log_msj, "\t");
	strcat(log_msj, msj);
	strcat(log_msj, "\n");

	pthread_mutex_lock(&mutex_archivo_log);
	if ((pf=fopen("serv.log", "a")) == NULL) 
	{
		fclose(pf);
		pthread_mutex_unlock(&mutex_archivo_log);
		return ERROR;		
	}

	fputs(log_msj, pf);
	fclose(pf);
	pthread_mutex_unlock(&mutex_archivo_log);
	return EXITO;
}
