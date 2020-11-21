/*
 e conectará el microcontrolador ESP32 con:

Un sensor de humedad y temperatura DHT11.

Un reloj en tiempo real RTC DS3231.

Un diodo LED RGB con las resistencias respectivas para cada terminal.

Se debe sincronizar convenientemente el RTC a la hora actual y luego, diseñar una aplicación
que realice mediciones de temperatura con el sensor DHT11, que transmite hacia otro dispositivo
como cliente TCP el valor de la temperatura, dependiendo del rango de temperaturas obtenido en
la medición se tomarán ciertas acciones.

Temperaturas inferiores a 20 °C – se debe transmitir adicional al valor de la temperatura el
mensaje “temperatura baja.” Encendiendo el LED RGB en color AZUL.

Temperaturas entre 20 °C y 25 °C – se debe transmitir junto al valor de la temperatura el
mensaje “temperatura de confort.” Encendiendo el LED RGB en color VERDE.

Temperaturas entre 25 °C y 32 °C – adicional al valor de la temperatura se debe transmitir el
mensaje “precaución, incremento de temperatura.” Encendiendo el LED RGB con la combinación de
colores que permita el AMARILLO.

Temperaturas superiores a 32 °C – se debe transmitir adicional al valor de la temperatura el
mensaje “Peligro: alta temperatura”. Encendiendo el LED RGB en color ROJO.

Cada vez que se tenga un cambio de rango, se registra en memoria la fecha y hora en la que
ocurrió. Al escribir a través del terminal serial la palabra “reportar” y presionar la tecla
ENTER se debe visualizar en el terminal serial la lista de eventos de cambio con fecha y hora.

Para los diagramas UML existen herramientas online tanto privadas como gratuitas. Haz clic en
el siguiente enlace para conocer una y utilizarla sin mayor dificultad: Diagramas UML

https://app.diagrams.net/

No es complejo incrementar el valor de la temperatura obtenida por el sensor, por ejemplo, si
trabaja en un computador portátil al acercar el sensor a la salida de la ventilación forzada
la temperatura rápidamente se incrementará a valores cercanos a 40 Celsius.

*/


//librerias indispensables para el SO
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// manejo de colas de mensajes, intercambiar informacion entre tareas del sistema operativo
#include "freertos/queue.h"//transmitir info desde una tarea hasta otra
// el dispositivo utiliza la memoria flash para guardar configuraciones wifi (tambine bluetooh)
#include "nvs_flash.h"
//mis librerias
#include "include/ClienteHTTPS.h"//configuracion inalambrca y protocolo TCP IP
#include "include/senDHT.h"// modulo del dht11
#include "include/WiFiconfig.h"// configuracion del wifi
//Macros
#define Pila 1024 // reservar memoria de pila de un k byte
#define tamCola 1 // cola de mensajes de un solo puntero
#define tamMSN 80 // 80 bytes tendra el mensaje
//variables globales
xQueueHandle Cola_sensor;//manejador de cola
//*********************************************************************************************************************************
//seccion principal
	void app_main(void){

	   nvs_flash_init();// inicializar memoria flash
	   iniciar_wifi();//iniciar modulo wifi  como estacion
	   Cola_sensor=xQueueCreate(tamCola,tamMSN);//terminar de crear cola de mensajes q enviara informacion entre las tareas
	   xTaskCreatePinnedToCore(&TareaDHT, "Medicion_DHT", Pila*3, NULL, 3, NULL, 0);// tarea del sistema operativo
	   xTaskCreatePinnedToCore(&TareaHTTP, "Cliente_HTTP", Pila*4, NULL, 3, NULL, 1);//tarea del sistema operativo

	}
//	vTaskDelete(NULL); elimina la tarea desde la cual es llamada
// se reinicia el esp32 conectado a la consola en 115200 para ver el ip
