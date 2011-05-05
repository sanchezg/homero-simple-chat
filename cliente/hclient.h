#ifndef __HCLIENT__
#define __HCLIENT__

/* Valores constantes */
#define TAM 256
#define PUERTO 6666

/* Valores para FLAGS */
#define ON	1
#define OFF	-1

#define MENU_CODE 3
#define EXIT_CODE 4

/* Este msj significa que el enviado se recibió correctamente */
/* El cliente no muestra msj en pantalla. */
#define _OK_ 5

/* Este msj indica que hay una solicitud de chat. */
#define SOLIC_CHAT 6

/* Este msj indica que no existe el cliente. */
#define ERR_CL_NO 7

/* Este msj indica que el msj es de control y se tiene que hacer asi el write */
#define CTRL_MSJ 8

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
			\n## CTRL CHARLEMOS \"<destino>\": Trata de iniciar una conversación con usuario <destino>\
			\n## MSG <destino> \"<mensaje>\": Transmite <mensaje> al usuario <destino>\
			\n   (Es necesario haber iniciado la conversación antes).\
			\n## menu: muestra este menú.\
			\n## exit: sale del programa.\n"
#define PROMPT "~ "

int abrir_conexion();
int conectar_servidor(char*);
int verificar_msj(char *);

#endif /* __HCLIENT__ */
