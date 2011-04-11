#ifndef __SERV_HILOS__
#define __SERV_HILOS__

#define MAX_QUE 5
#define TAM 256
#define PUERTO 6666

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

#endif /* __SERV_HILOS__ */
