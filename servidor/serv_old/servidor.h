#ifndef __SERVIDOR__
#define __SERVIDOR__

#define TAM 256
#define MAX_QUE 10

/* Valores locales */
#define EX_REGCLI 100
#define ERR_REGCLI 101

#define LISTAR_CLI 200

#define ERR_COM -1

/* Funciones locales */
int verificar_msj(char* entrada);
int registrar_usuario(char *nombre);
int archivo_agregar(char* nombre_archivo, char* texto);
int archivo_buscar(char *nombre, char *cadena);
char* archivo_listar(char* nombre);

#endif /* __SERVIDOR__ */
