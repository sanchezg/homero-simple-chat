#ifndef __SERV_HILOS__
#define __SERV_HILOS__

#define MAX_QUE 5
#define TAM 256
#define PUERTO 6666

#define EXITO 0
#define ERROR 1

#define EXITO_REG_CLIENTE 10
#define ERROR_REG_CLIENTE 11


typedef struct ptr 
{
	pthread_t data1;
	int data2;
	struct ptr *next;
} lista_pt;

void list_print(lista_pt *); 
lista_pt **list_search_d2(lista_pt **, int);
lista_pt **list_search_d1(lista_pt **, pthread_t);
void list_remove(lista_pt **);
lista_pt *list_add(lista_pt **, pthread_t, int);
void *ejec_cliente(void *);
int nuevo_cliente(lista_pt **, int);
int aceptar_conexion(int);
int iniciar_servidor (int);
int list_len(lista_pt *);
void* ejec_servidor(void *);
int verificar_msj(char *);

int archivo_agregar(char*, char*);
char* archivo_listar(char*);
int archivo_buscar(char *, char *);
int registrar_usuario(char *);

void broadcast_clientes(char *);

#endif /* __SERV_HILOS__ */
