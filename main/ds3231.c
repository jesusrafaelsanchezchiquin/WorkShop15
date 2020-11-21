/// leer o escribir hora en en ds3231
// Archivos----------------
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include <string.h>
//Macros y definiciones
#define ds3231 0x68// direccion del i2c para el ds3231 en la hoja del fabricante
//con un cero se escribe y con un 1 se escribe sobre la memoria
// la hora zero del dispositivo que el micro escribira en el RTC
#define SecZ 0x00
#define MinZ 0x00
#define HorZ 0x51 // formanto para 11 AM
#define DiaZ 0x03 // dia martes
#define FecZ 0x17 // fecha 17
#define MesZ 0x03 // marzo
#define AniZ 0x20 // 2020
//toda esta configuracion se ve en la hoja del fabricante del DS3231

static void enviar_ds3231(){//envia la hora hacia el ds3231
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();// variable q funciona como manejador del protocolo (cmd)
	//creamos variable q funciona como manejador del protocolo (cmd) del tipo i2c_cmd_handle_t cmd con
	//la funcion i2c_cmd_link_create()
	//ahora las funciones del protocolo de la api de espressif
	i2c_master_start(cmd);// iniciar el maestro
	//ahora inicio el proceso de escritura
	//primero iniciar el maestro ultilizando el manejador, y luego comoenzar el proceso de escritura
	//el primer byte que se envia es el que contiene la direccion del ds3231 y es necesario
	//desplazarlo una posicion hacia izquierda, para liberar el bit menos significativo
	// ya que este es el que se especifica para indicar al esclavo si se esta leyendo o escribiendo
	// | significa OR
	// el macro I2C_MASTER_WRITE  es basicamente el nuemero cero, cerramos con 1
	i2c_master_write_byte(cmd, (ds3231<<1)|I2C_MASTER_WRITE,1);
	i2c_master_write_byte(cmd,0x0, 1);// posicion de memoria en la que inicia el proceso de escritura
	i2c_master_write_byte(cmd,SecZ, 1);//automanticamente se cambia el 0x00 por 0x01 y asi susecivamente
	i2c_master_write_byte(cmd,MinZ, 1);
	i2c_master_write_byte(cmd,HorZ, 1);
	i2c_master_write_byte(cmd,DiaZ, 1);
	i2c_master_write_byte(cmd,FecZ, 1);
	i2c_master_write_byte(cmd,MesZ, 1);
	i2c_master_write_byte(cmd,AniZ, 1);
	i2c_master_stop(cmd);// cerramos el proceso de comunicacion
	i2c_master_cmd_begin(I2C_NUM_0,cmd, 1000/portTICK_PERIOD_MS);//proceso de reinicio
	i2c_cmd_link_delete(cmd);
}
// esta instruccion no se pierde nuca porq el ds3231 tiene su bateria
//
// retorna puntero char y una serie de punteros de valores de entrada
char* leer_ds3231(uint8_t* seg, uint8_t* min, uint8_t* hor,
		uint8_t* fec , uint8_t* mes, uint8_t* ani, uint8_t* ampm){
// primera variable arreglo de cadena caracteres
	//el ds3231 reporta el domingo como 0 por tanto mas adelante se le restara uno para que quede como ene ste arreglo
	char*  diax []={"Domingo","Lunes","Martes","Miercoles","Jueves","Viernes","Sabado"};
	uint8_t segundos, minutos, horas, dia, fecha, meses, anios;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();//creamos manejador i2c
	i2c_master_start(cmd);//iniciamos
	i2c_master_write_byte(cmd, (ds3231<<1)|I2C_MASTER_WRITE,1);// realizamos un desplazamiento hacia la izquierda que deseamos escribir
	i2c_master_write_byte(cmd, 0x0, 1);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (ds3231<<1)|I2C_MASTER_READ,1);
	//la funcion de escritura sirve para indicarle al esclavo q todas las funciones en adelante seran de lectura
	i2c_master_read(cmd, &segundos, 1, 0);//& va previo a la variable que llega del ds3231
	//estas lecturas estan en orden segun el el orden de llegada del ds3231
	i2c_master_read(cmd, &minutos, 1, 0);
	i2c_master_read(cmd, &horas, 1, 0);
	i2c_master_read(cmd, &dia, 1, 0);
	i2c_master_read(cmd, &fecha, 1, 0);
	i2c_master_read(cmd, &meses, 1, 0);
	i2c_master_read(cmd, &anios, 1, 1);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0,cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	*seg=segundos;
	*min=minutos;
	*hor=0x1F & horas;// es necesario enmascarar solo nos quedamos con los bits 0 1 2 3 4 en binario seria 0b0001111
	*mes=meses;
	*ani=anios;
	if (horas & 0x20) *ampm = 1; else *ampm =0;// & operacion AND solo deseamos saber si un bit esta en 1 0b00 1 00000
	return diax[dia-1];// aqui retornamos el dia -1 para arreglar lo q se dice antes
}
// hasta aqui se tiene todo lo necesario, en adelante es todo opcional


//%x valor hexadecimal
//%C caracter

/*
void tareaDS3231(void* P){
	uint8_t seg,min,hor,fec,mes,ani,ampm;
	char diax[10]=pmam[2];
	//enviar_ds3231 graba la fecha cuando le quito el comentario
	for(;;){// leer la fecha del ds3231
		strcpy(diax,leer_ds3231(&seg,&min,&hor,&fec,&mes,&ani,&ampm));
		if (ampm)
			memcpy(pmam,"PM",2);
		else
			memcpy(pmam,"AM",2);
		printf("Fecha: %s %x-%x-20%x\n\r",diax,fec,mes,ani);
		printf("Hora: %x:%x:%x %C%C\n\r",hor,min,seg,pmam[0],pmam[1]);
		vTaskDelay(1000/portTICK_PERIOD_MS)
	}
}
*/
//dentro de esa tarea lo q hago es escribir en consola
