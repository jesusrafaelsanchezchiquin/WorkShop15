// Archivos----------------
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include <string.h>
//podria hacerse un #include ds3231 pero se utilizan las macros de ams abajo
// macros y definiciones
#define DATA_PIR GPIO_NUM_4
#define BUZZER GPIO_NUM_22
#define PULSADOR GPIO_NUM_0
//Estructura y variables (si estan fuera de la funcion son variables globales
uint8_t cont_alarma=0;// conteo de cada q se detecta movimientos
// despues de 20 movimientos vuelve a cero
struct ALARMA{
	uint8_t anio;
	uint8_t mes;
	uint8_t fecha;
	uint8_t seg;
	uint8_t min;
	uint8_t hora;
	uint8_t formato_hora;
	char dia[10]; //miercoles es la palabra mas grande
	char ampm[2];
}Registros_alarma[20];//matriz
//la siguiente funcion fue escrita en ds3231.c y aqui la llamamos
// Con una declaracion de variable externa con la palabra extern y se
// escribe el mismo codigo que en su archivo
extern char* leer_ds3231(uint8_t* seg, uint8_t* min, uint8_t* hor,
						uint8_t* fec , uint8_t* mes, uint8_t* ani, uint8_t* ampm);
void TareaPir(void* P){
	gpio_set_direction(DATA_PIR|PULSADOR,GPIO_MODE_INPUT);
	gpio_set_direction(BUZZER,GPIO_MODE_OUTPUT);
	leer_ds3231(&Registros_alarma[0].seg,
				&Registros_alarma[0].min,
				&Registros_alarma[0].hora,
				&Registros_alarma[0].fecha,
				&Registros_alarma[0].mes,
				&Registros_alarma[0].anio,
				&Registros_alarma[0].formato_hora);
	for(;;){// bucle infinito que debe existir en toda tarea de sistema operativo
		if (gpio_get_level(DATA_PIR)){// se recive senal desde el PIR?
			//obtener lectura del RTC------------
			strcpy(Registros_alarma[cont_alarma].dia,// funcion para copiar cadena de carateres, en el argumento dia
				leer_ds3231(&Registros_alarma[cont_alarma].seg,// debe estar en el mismo orden que en la "cabecera"
				   &Registros_alarma[cont_alarma].min,
				   &Registros_alarma[cont_alarma].hora,
				   &Registros_alarma[cont_alarma].fecha,
				   &Registros_alarma[cont_alarma].mes,
				   &Registros_alarma[cont_alarma].anio,
				   &Registros_alarma[cont_alarma].formato_hora));
			// Obtener siglas formato hora----------
			if (Registros_alarma[cont_alarma].formato_hora)//hora AM o hora PM?
				strcpy(Registros_alarma[cont_alarma].ampm,"PM");
			else
				strcpy(Registros_alarma[cont_alarma].ampm,"AM");
			//encender buzzer por dos segundos -----
			gpio_set_level(BUZZER,1);
			vTaskDelay(2000/portTICK_PERIOD_MS);
			gpio_set_level(BUZZER,0);
			//**********************************//
			cont_alarma++;
			if (cont_alarma>=20) cont_alarma=0;
		}
		vTaskDelay(100/portTICK_PERIOD_MS);
	}
}
void Pulsador (void* P){// dicta acciones a tomar cada vez q se toque el pulsador EN que esta  conectado en el GPIO0
	//este termial es normalmente 1 mientras no se toque el pulsador se esta en alto
	for(;;){
		if(gpio_get_level(PULSADOR)==0){
			vTaskDelay(750/portTICK_PERIOD_MS);// rutina antirebote para el pulsador
			//Reportar alarmas en consola serial
			//por defecto para print f se usa 115200 en la consola
			for (int i=0;i<cont_alarma;i++){// for para imprimir en la pantalla todos los registros
				printf("*************************ALARMAS %d*************************\n\r",i);
				printf("Fecha: %x-%x-20%x\n\r",Registros_alarma[i].fecha,
											   Registros_alarma[i].mes,
											   Registros_alarma[i].anio);
				printf("Fecha: %x:%x:%x %c%c\n\r",Registros_alarma[i].hora,
												  Registros_alarma[i].min,
												  Registros_alarma[i].seg,
												  Registros_alarma[i].ampm[0],
												  Registros_alarma[i].ampm[1]);
				printf("**********************************************************\n\r");
			}
		}
		vTaskDelay(15/portTICK_PERIOD_MS);// si no se toca nunca el pulsador, la tarea se bloquea 15 ms
	}
}
