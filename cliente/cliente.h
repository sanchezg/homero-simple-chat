#ifndef __CLIENTE__
#define __CLIENTE__

/* Valores constantes */
#define TAM 256
#define PUERTO 6666

/* Valores para FLAGS */
#define ON	1
#define OFF	-1

/* Este msj significa que el enviado se recibió correctamente */
#define _MSJ_OK_ 5 

/* Estos son msj que se reciben y el cliente los utiliza para algún comportamiento específico */
#define EXITO_REG 10
#define ERROR_REG 11
#define ENTRO_ALGUIEN 20
#define EXITO_CHAT 30
#define ERROR_CHAT 31

#define ERROR_MSJ -1

#define MENU_MSJ "\t\tBienvenido a Homero-Simple-Chat.\n#### Ingrese un comando:\
			\n## <nickname> CTRL HOLA: Se registra con el servidor.\
			\n## CTRL LISTAR: Muestra los usuarios conectados.\
			\n## CTRL CHARLEMOS \"<destino>\": Trata de iniciar una conversación con usuario destino\
			\n## MSG <destino> \"<Mensaje>\": Transmite Mensaje al usuario <destino>\
			\n   (Es necesario haber iniciado la conversación antes).\
			\n## menu: muestra este menú.\
			\n## exit: sale del programa.\n"
#define PROMPT "~ "

int abrir_conexion();
int conectar_servidor(int, char*);
int verificar_msj(char *);

#endif /* __CLIENTE__ */
