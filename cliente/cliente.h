#ifndef __CLIENTE__
#define __CLIENTE__

#define TAM 256
#define PUERTO 6666

#define EXITO_REG 10
#define ERROR_REG 11
#define ENTRO_ALGUIEN 20

int abrir_conexion();
int conectar_servidor(int, char*);
int verificar_msj(char *);

#endif /* __CLIENTE__ */
