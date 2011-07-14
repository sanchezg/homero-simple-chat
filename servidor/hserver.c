#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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
#include "hserver.h"

int SERVER_ACTIVO, ID_COLA;
char * ERROR_MSJ;
key_t K_COLA;
//elem_cola COLA_GRAL;

lista_pt *thread = NULL;

pthread_mutex_t mutex_listapt = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_archivo_clientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cola_msj = PTHREAD_MUTEX_INITIALIZER;

void sigint_handler(int sig)
{
	/* Primero liberar la cola de msj */
	msgctl(ID_COLA, IPC_RMID, NULL);

	/* Eliminar archivos */
	//remove("clientes");

	printf("Hasta la próxima... :)\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) 
{
	int des_servidor;
	pthread_t th_conex;

	system("clear");

	if (signal(SIGINT, sigint_handler) == SIG_ERR)
	{
		printf("Error en sigint_handler\n");
		exit(EXIT_FAILURE);
	}

	if ((des_servidor = iniciar_servidor(PUERTO)) == -1)
		exit(EXIT_FAILURE);

	if (pthread_create(&th_conex, NULL, ejec_servidor, (void*) &des_servidor) != 0)
	{
		printf ("ERROR PTHREAD SERVIDOR\n");
		return -1;
	}

	/* Esperar a que termine el thread de ejecución. */
	pthread_join(th_conex, NULL);	
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

	SERVER_ACTIVO = ON;
	printf ("## Homero-Server disponible, puerto %d\n", ntohs(serv_addr.sin_port));

//	printf ("## creando logs...\n");
/*	if ((archivo_crear("clientes") != EXITO) || (archivo_crear("serv.log") != EXITO) || (archivo_crear("chat.log") != EXITO))
	{
		printf("ERROR creando logs!!!\n");
		return -1;
	}*/
	return sockfd;
}

void* ejec_servidor(void *ptr)
{
	int *iptr, des_servidor, des_cliente;

	iptr = (int *) ptr;
	des_servidor = *iptr;

	if(iniciar_cola() == ERROR)
	{
		printf("Error al abrir cola de mensajes. Abortado\n");
		exit(EXIT_FAILURE);
	}

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

void *ejec_cliente(void *ptr)
{
	int *iptr, mi_descriptor;//, fl_recv = OFF;
	int CLIENTE_ACTIVO = ON;
	size_t clilen;
	struct sockaddr_in cli_addr;
	struct sockaddr_in* s;
	char ipstr[INET_ADDRSTRLEN], buffer[TAM], resp_servidor[TAM];
	elem_cola buffer_cola;

	iptr = (int *) ptr;
	mi_descriptor = *iptr;

	clilen = sizeof(cli_addr);
	s = (struct sockaddr_in *) &cli_addr;
	getpeername(mi_descriptor, (struct sockaddr*) &cli_addr, &clilen);

	printf("hilo %lu del cliente %s\n", pthread_self(), inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr));

	//Hilo en ejecucion y listo para laburar y escuchar al cliente.
	//Bucle principal...
	while ((CLIENTE_ACTIVO == ON) && (getpeername(mi_descriptor, (struct sockaddr*) &cli_addr, &clilen) == 0))
	{
		//Esperar por el primer msj.
		memset(buffer, '\0', TAM);
		if (recv(mi_descriptor, buffer, TAM, MSG_DONTWAIT) > 0)
		{
			switch (verificar_msj(buffer, mi_descriptor))
			{
				case EXIT_CODE:
					CLIENTE_ACTIVO = OFF;
					continue;

				case EXITO_REG_CLIENTE:
					if (write(mi_descriptor, "CTRL QUETAL\n", TAM) < 0) 
					{
						perror("write");
						exit(EXIT_FAILURE);
					}
					break;

				case ERROR_REG_CLIENTE:
					if (write(mi_descriptor, "CTRL FUERA\n", TAM) < 0) 
					{
						perror("write");
						exit(EXIT_FAILURE);
					}
					break;

				case LISTAR_CODE:
					strcpy(resp_servidor, "# Lista de usuarios: \n");
					strcat(resp_servidor, listar_clientes(mi_descriptor));
					if (write(mi_descriptor, resp_servidor, TAM) < 0) 
					{
						perror("write");
						exit(EXIT_FAILURE);
					}
					break;

				case EXITO_CL_CHARL:
					puts("pregunta de chat enviada\n");
					break;

				case ERROR_CL_CHARL:
					puts("error de chat\n");
					send(mi_descriptor, ERROR_MSJ, TAM, MSG_DONTWAIT);
					break;

				case EXITO_CHAT:
					puts("Chat aceptado\n");
					break;
				case ERROR_CHAT:
					puts("Chat rechazado\n");
					break;

				default:
					printf("Msj recibido: %s",buffer);
					break;
			}
		}
		/* acá debería revisar si hay msj pendientes en la cola */
		/* Mensajes que preguntan chat */
		pthread_mutex_lock(&mutex_cola_msj);
		if(msgrcv(ID_COLA, (struct msgbuf *) &buffer_cola, sizeof(buffer_cola.msj), mi_descriptor, IPC_NOWAIT) > 0)
		{
			pthread_mutex_unlock(&mutex_cola_msj);
			//printf("Hay un msj para mí: %s \n", buffer_cola.msj);
			send(mi_descriptor, buffer_cola.msj, TAM, MSG_DONTWAIT);
		}
		pthread_mutex_unlock(&mutex_cola_msj);

		/* Mensajes de chat, notar el tipo varía!! */
		pthread_mutex_lock(&mutex_cola_msj);
		if(msgrcv(ID_COLA, (struct msgbuf *) &buffer_cola, sizeof(buffer_cola.msj), mi_descriptor*100, IPC_NOWAIT) > 0)
		{
			pthread_mutex_unlock(&mutex_cola_msj);
			//printf("Hay un msj para mí: %s \n", buffer_cola.msj);
			send(mi_descriptor, buffer_cola.msj, TAM, MSG_DONTWAIT);
		}
		pthread_mutex_unlock(&mutex_cola_msj);

	}
	printf("hilo %lu finalizado\n", pthread_self());
	//deregistrar_usuario(mi_descriptor);
	list_remove(list_search_d2(&thread, mi_descriptor));
	return NULL;
}

/* esta verifica el msj recibido en el buffer y llama a la función correspondiente */
int verificar_msj(char * buffer_entrada, int sock_id)
{
	char temp[TAM];
	strcpy(temp, buffer_entrada);

	if (strcmp(temp, "exit\n") == 0)
		return EXIT_CODE;

	if (strcmp(temp, "CTRL LISTAR\n") == 0)
		return LISTAR_CODE;

	if (strstr(buffer_entrada,"CTRL HOLA") != NULL)
	{
		if (registrar_usuario(sock_id, strtok(temp," ")) == EXITO)
			return EXITO_REG_CLIENTE;
		else
			return ERROR_REG_CLIENTE;
	}

	if (strstr(temp, "CTRL CHARLEMOS") != NULL)
	{
		strtok(temp, "\"");
		strcpy(temp, strtok(NULL, "\""));
		printf("nombre a buscar: %s\n", temp);
		pthread_mutex_lock(&mutex_archivo_clientes);
		if (archivo_buscar("clientes", temp) == EXITO)
		{
			pthread_mutex_unlock(&mutex_archivo_clientes);
			if (preguntar_chat(sock_id, temp) == EXITO)
				return EXITO_CL_CHARL;
			else
			{
				strcpy(ERROR_MSJ, "Error desconocido\n");
				return ERROR_CL_CHARL;
			}
		}
		else
		{
			pthread_mutex_unlock(&mutex_archivo_clientes);
			ERROR_MSJ = "No se encontro al cliente\n";
			return ERROR_CL_CHARL;
		}
	}

	if (strstr(temp, "CHAT_OK") != NULL)
	{
		char temp1[32], temp2[32];
		strcpy(temp1, strtok(temp, "|")); //solicitante
		strtok(NULL, "|");
		strcpy(temp2, strtok(NULL, "\n"); //solicitado
		
		return EXITO_CHAT;
	}

	if (strstr(temp, "CHAT_NO") != NULL)
		return ERROR_CHAT;
	return 0;
}

/* si el nombre como argumento es válido, registra al usuario */
int registrar_usuario(int id, char *nombre)
{
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

/* manda msj a usuario solicitando chat */
int preguntar_chat(int socket_origen, char* nombre_destino)
{
	int socket_destino;
	char* nombre_origen;
	elem_cola buffer_cola;

	pthread_mutex_lock(&mutex_listapt);
	socket_destino = obtener_id_nombre(nombre_destino);
	nombre_origen = obtener_nombre_id(socket_origen);
	nombre_origen = strtok(nombre_origen, "\n");
	pthread_mutex_unlock(&mutex_listapt);

	printf("desde %d %s hacia %d %s\n", socket_origen, nombre_origen, socket_destino, nombre_destino);

	buffer_cola.tipo = socket_destino;
	strcpy(buffer_cola.msj, "_CHAT|");
	strcat(buffer_cola.msj, nombre_origen);
	strcat(buffer_cola.msj, "\n");

	pthread_mutex_lock(&mutex_cola_msj);
	if(msgsnd(ID_COLA, (struct msgbuf *) &buffer_cola, sizeof(buffer_cola.msj), IPC_NOWAIT) != 0)
	{
		pthread_mutex_unlock(&mutex_cola_msj);
		return ERROR;
	}
	pthread_mutex_unlock(&mutex_cola_msj);

	return EXITO;
}

/* borra el registro de un usuario para que otro se pueda conectar con ese nombre */
void deregistrar_usuario(int descriptor)
{
	pthread_mutex_lock(&mutex_archivo_clientes);
	archivo_borrar("clientes", obtener_nombre_id(descriptor));
	pthread_mutex_unlock(&mutex_archivo_clientes);
	return;
}

/*envía un msj a todos los clientes */
int broadcast(int origen, char *msj)
{
	lista_pt *n = (lista_pt *)malloc(sizeof(lista_pt));
	if (n == NULL)
		return ERROR;
	n = thread;
    while (n != NULL)
	{
        send(n->_id_socket_, msj, TAM, MSG_DONTWAIT);
        n = n->_next_;
    }
	return EXITO;
}

/* Devuelve una lista de clientes registrados */
char * listar_clientes(int socket)
{
	char* lista = (char*) malloc (sizeof(char)*TAM);

	lista_pt *n = malloc(sizeof(lista_pt));
	pthread_mutex_lock(&mutex_listapt);
	n = thread;
	pthread_mutex_unlock(&mutex_listapt);

    while (n != NULL) {
        strcat(lista, n->_nombre_);
        n = n->_next_;
    }
	free(n);

	return lista;
}

/* Obtienen el sock_id o nombre de un cliente de la lista */
int obtener_id_nombre(char* nombre)
{
	lista_pt **n = malloc(sizeof(lista_pt*));

	if ((n = list_search_d3(&thread, nombre)) == NULL)
		return -1;
	else
		return ((*n)->_id_socket_);
}

char* obtener_nombre_id(int id)
{
	lista_pt **n = malloc(sizeof(lista_pt));

	if ((n = list_search_d2(&thread, id)) == NULL)
		return NULL;
	else
		return ((*n)->_nombre_);
}

/********** Funciones que tratan con archivos *********************/

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

int archivo_borrar(char* nombre_archivo, char* texto)
{
	char *temp1, *str, *lista_nombres;
	FILE *pf;
	if ((pf=fopen(nombre_archivo, "w+r")) == NULL)
	{
		fclose(pf);
		return ERROR;
	}

	lista_nombres = malloc(sizeof(char)*TAMNOM*TAMNOM);
	strcpy(lista_nombres, archivo_listar(nombre_archivo));

	temp1 = malloc(sizeof(char)*TAMNOM*TAMNOM);
	str = malloc(sizeof(char)*TAMNOM);

	//primero ir hasta que encuentre el nombre...
	while (strcmp(texto, fgets(str, TAMNOM, pf)) != 0)
		strcpy(temp1, str);

	//ahora ir hasta el final...
	while (fgets(str, TAMNOM, pf) != NULL)
		strcpy(temp1, str);		//en temp1 me quedan todos los nombres

	fclose(pf);		//Primero, borrar el archivo...
	if(remove(nombre_archivo) != 0 )
	{
		perror("remove");
		fclose(pf);
		return ERROR;
	}
	//... abrirlo para escritura
	if ((pf=fopen(nombre_archivo, "w+r")) == NULL)
	{
		fclose(pf);
		return ERROR;
	}

	fputs(temp1, pf);	//y ponerle todo de nuevo!
	fclose(pf);
	return EXITO;
}


/********** Funciones que tratan con la lista de threads **********/

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
