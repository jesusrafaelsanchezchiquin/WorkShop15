#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*

Archivos FreeRTOS.h y task.h Se requieren estos archivos para el funcionamiento íntegro
 del freeRTOS y sus tareas.

*/
#include "esp_log.h"// mensajes de depuracion

/*
 Archivo esp_log.h Para mostrar mensajes relativos al funcionamiento de un programa en la
 * consola del computador desde donde se realiza el código, se pueden escribir mensajes
 * estratégicos para indicar al programador el momento en el que se ejecutan (o dejan de
 * ejecutarse) ciertas instrucciones, procedimiento conocido como depuración. La función
 * “printf” es una forma de escribir estos mensajes indicadores, otra alternativa para realizar
 *  este procedimiento es con las funciones “ESP_LOGI” , “ESP_LOGW” y “ESP_LOGE”.

Estas funciones en orden respectivo se utilizan para indicar información (letra I) que en la
consola se visualiza en color verde, las operaciones con precauciones (letra W) que se visualiza
 en color amarillo y alertas de error (letra E) que se visualiza en la consola en color rojo.
 Otra ventaja que presenta el uso de ESP_LOG al comparar con “printf” es que adicional al mensaje,
 también escribe el número del contador de pulsos de reloj (timestamp) que han transcurrido desde
 el último reinicio del µC. Esta transmisión de mensajes siempre ocurre a través del puerto
 configurado por defecto (UART0) a una tasa de 115200 bps.
 */

#include "freertos/queue.h"

/*

 Archivo queue.h Para este programa se capturarán los datos de humedad y temperatura en la tarea
 correspondiente al DHT11, pero esa data capturada se transmitirá a través de la red WiFi en otra
  tarea, es necesario transferir los datos de una tarea a la otra y para ello se utilizarán las colas,
  cuyas funciones y macros necesarios se definen en este archivo “queue.h”.

 */

#include "driver/gpio.h"
/*

 Archivo queue.h Para este programa se capturarán los datos de humedad y temperatura en la tarea
  correspondiente al DHT11, pero esa data capturada se transmitirá a través de la red WiFi en otra tarea,
   es necesario transferir los datos de una tarea a la otra y para ello se utilizarán las colas, cuyas
    funciones y macros necesarios se definen en este archivo “queue.h”.

*/

#include <string.h>

/*

Archivo string.h Como será necesaria la manipulación de mensajes en cadenas de caracteres ya que la
información de la humedad y la temperatura se transmiten en formato de caracteres ASCII, es conveniente
 utilizar las funciones contenidas en la librería estándar de C para ello.

 */

#define us_retardo 1 //retardos medidos en micro segundos
#define numDHT_bits 40
/*

numDHT_bits: indicar la cantidad de bits por recibir, como se mencionó con anterioridad, el DHT11 entrega 40 bits
con la información de humedad y temperatura.

 */
#define numDHT_bytes 5
#define DHTpin 4 // terminal de entrada y salida no se puede utilizar 0 2 5 12 15, son usados para flashear el micro

/*

Para el funcionamiento de la cola de mensajes es necesario declarar una variable global, la funcionalidad de las
colas entre tareas necesitan una variable para la asignación de la misma. Dicha variable es declarada en el archivo
 donde se encuentra el código de la aplicación principal, para poder utilizarla en este archivo fuente se declara
 con la palabra reservada “extern.” También es necesario definir una segunda variable global que será utilizada en
 una función que requiere detener temporalmente el salto automático entre tareas del sistema operativo.

*/

extern xQueueHandle Cola_sensor;// declarada en modulo principal
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; //mux sera utilizada para procesos del SO
// se necesita una medida precisa en us

/*

Como el protocolo para la comunicación con el sensor de humedad y temperatura requiere medir el tiempo en
microsegundos, los cambios entre tareas del sistema operativo pueden interferir con este proceso de medición de
 tiempo provocando fallas en la lectura del sensor, por tanto resulta necesario que en los instantes donde se
 requiera realizar la medición, detener el proceso de salto entre tareas del RTOS.

Luego de la definición de las variables globales, se escribe el código uno a uno de las funciones requeridas.
Previamente se realizó la declaración de una función prototipo en el archivo cabecera, la tarea RTOS llamada
 “TareaDHT”. Desde esta función “TareaDHT” se ejecutan varias instrucciones de forma repetitiva con cierta
 complejidad, por tanto se agrupan como funciones adicionales cuyo código se debe escribir primero, esta
 funcionalidad repetitiva que solo será utilizada en este archivo fuente se declaran como funciones estáticas.

*/
/*

Resumiendo, en el lenguaje C el orden en que se escribe el código de las funciones es importante, las funciones
estáticas no se declaran en los archivos cabecera, por tanto si una función A necesita llamar otra función B, el
código de la función B debe estar escrito (o declarado) antes que el código de la función A. Las que serán utilizadas
 en este archivo fuente serán:

*/
/*

1)TiempoDeEspera Una función del tipo estática que se encargará de medir los microsegundos que transcurren en determinado
 estado lógico del terminal GPIO. Solamente es llamada desde el interior de una función llamada “CapturarDatos.”

*/
static esp_err_t TiempoDeEspera(gpio_num_t pin,
								uint32_t timeout,
								int valor_esperado,
								uint32_t *contador_us){
	//numero de pin, us que se ejecuta la instruccion(si no se cumple, error), que esperamos q cambie de 1 a o o al revez, puntero(numero de repeticiones)

	gpio_set_direction(pin, GPIO_MODE_INPUT); //cambiar a entrada digital
	for (uint32_t i = 0;i< timeout;i+= us_retardo){// datos desde el dht11
		ets_delay_us(us_retardo);// vTaskDelay solo genera retardos de un minimo de 10ms, esta hace retardos en us
		if(gpio_get_level(pin) == valor_esperado){// si alcanza el valor esperado entonces retorna esp_ok
			if(contador_us) *contador_us=i;// esta linea no importa cuando el ultimo argumento de  tiempo de espera es null
			return ESP_OK;
		}
	}
	return ESP_ERR_TIMEOUT;
}
/*

2)CapturarDatos Otra función del tipo estática, esta se encargará de realizar la solicitud de datos al sensor DHT11,
procediendo a capturar los 5 bytes de datos que el dispositivo entrega. La función “CapturarDatos” solamente es llamada
desde la función “leerDHT.”

*/
static esp_err_t CapturarDatos (gpio_num_t pin,// requiere de comunicacion precisa con el DHT11, los saltos entre tareas son de 10us esta funcion tarda 25ms
								uint8_t datos[numDHT_bytes]){// al ser una funcion estatica retiene los valores
	uint32_t tiempo_low;
	uint32_t tiempo_high;
	gpio_set_direction(pin,GPIO_MODE_OUTPUT_OD);
	gpio_set_level(pin,0);
	ets_delay_us(20000);// cero logico durante 20 ms (inicio de comunicacion)
	gpio_set_level(pin,1);
	if(TiempoDeEspera(pin,40,0,NULL)!=ESP_OK)return ESP_ERR_TIMEOUT;// medir cuanto tarda el dht11 en responder en este caso 40us
	// si desppues de 40 us el pin no pasa de 1 a cero, entonces hubo un error
	if(TiempoDeEspera(pin,90,1,NULL)!=ESP_OK)return ESP_ERR_TIMEOUT;
	if(TiempoDeEspera(pin,90,0,NULL)!=ESP_OK)return ESP_ERR_TIMEOUT;
	for(int i = 0 ; i < numDHT_bits; i++ ){//ahora vendran los 5 bytes de comunicaion el ciclo se repite 40 veces
		if(TiempoDeEspera(pin,60,1,&tiempo_low)!=ESP_OK)return ESP_ERR_TIMEOUT;// en tiempo de espera se almacenara la cantidad de repeticiones, cada una de 1us
		//se mide hasta 60 us hasta q cambie a 1
		if(TiempoDeEspera(pin,75,0,&tiempo_high)!=ESP_OK)return ESP_ERR_TIMEOUT;
		//tiempo hig mayor a low entonces se tiene un 1 logico
		uint8_t b=i/8,m=i%8;// rutina para ordenar datos, b para enumerar bytes y m para enumerar bits
		if(!m)datos[b]=0;
		datos[b] |= (tiempo_high > tiempo_low) << (7-m);
	}
	return ESP_OK;
}
/*

3)leerDHT En esta función se activará la secuencia de lectura de datos desde el sensor de humedad y temperatura,
luego de recibir estos bytes de datos del sensor, se procederá a una operación matemática para verificar el Checksum y
verificr que la misma data fue obtenida sin errores.

*/
esp_err_t leerDHT(gpio_num_t pin, uint8_t *humedad, uint8_t *temperatura,// ESP_OK es algo q esta dentro de esp_err_t
				uint8_t *decimal){
	uint8_t datos[numDHT_bytes]= {0,0,0,0,0};// almancen de datos que llegan del DHT11
	gpio_set_direction(pin,GPIO_MODE_OUTPUT_OD);//necesario para realizar medicion del dispositivo
	gpio_set_level(pin,1);//necesario para realizar medicion del dispositivo
	portENTER_CRITICAL(&mux); //si una de las interrupciones del funcionamiento natural del SO entra justo en el momento de la medida
	// se tendra una medida erronea, con esto detenemos el flujo tipico del so pero si si se de tienen por mas de 2 s el watch dog actua
	//y da falla en el sistema
	esp_err_t resultado = CapturarDatos(pin, datos);// se espera q capturar datos retorne esp_ok, si es diferente  mostramos el numero de error q dio
	portEXIT_CRITICAL(&mux);// se pone en marcha el sistema operativo nuevamente
	gpio_set_direction(pin,GPIO_MODE_OUTPUT_OD);
	gpio_set_level(pin,1);
	if (resultado != ESP_OK)return resultado;
	if (datos[4] != ((datos[0]+datos[1]+datos[2]+datos[3]) & 0xFF)){// verificacion, 0xFF preguntar
		ESP_LOGE("sensor_DHT11","Error en verificacion de checksum");
		return ESP_ERR_INVALID_CRC;
	}
	*humedad=datos[0];
	*temperatura=datos[2];
	*decimal=datos[3];// copia de datos en estas 3 lineas
	return ESP_OK;
}

/*

4)TareaDHT Esta es la tarea del sistema operativo que se encarga de todo el proceso relacionado con el DHT11, luego de
obtener los datos del sensor de humedad y temperatura, se transforma la información de formato hexadecimal a caracteres
ASCII y se realiza una copia que se envía hacia otra tarea mediante el sistema de colas del freeRTOS.

*/
void TareaDHT(void *P){
	uint8_t temperatura=0,decimal=0;//uint8_t variable de tipo entero sin signo de 8 bits
	uint8_t humedad=0;
	char datos_sensor[]={"https://api.thingspeak.com/update?api_key=AWC0P0T8N76H06CO&field1=00.0&field2=00"};// mensaje q va de esta tarea a la tarea cleinte HTTP 80 bites
	//se modificaran las posiciones 19 20 38 39 40
	for(;;){
		vTaskDelay(3000/portTICK_PERIOD_MS);// se debe medir con una tasa mayor a 2 segundos, de no ser asi se puede calentar y perder presicicon
		if (leerDHT(DHTpin,&humedad,&temperatura,&decimal) == ESP_OK){// si todo sale bien retorna 0 que es ESP_OK
			ESP_LOGI("Sensor_DHT11","Humedad %d%% Temperatura %d.%dC",humedad,temperatura,decimal);// %% es para imprimir %
			datos_sensor[78] = (humedad / 10)+'0';//comillas simples alt+39, si es 42 esto da 4.2 y no se almacena el decimal e para la representacion  assci
			datos_sensor[79] = (humedad % 10)+'0';// al codigo decimal se le suma 30(hexadecimal) o el caracter '0', para encontrar la unidad se utiliza la operacion modulo  que retorna el residuo
			datos_sensor[66] = (temperatura / 10)+'0';
			datos_sensor[67] = (temperatura % 10)+'0';
			datos_sensor[69] = (decimal % 10)+'0';
			xQueueSend(Cola_sensor,&datos_sensor,0/portTICK_PERIOD_MS);// cola de mensaje en la cual se enviara el mensaje, mensaje que sera copiado, tiempo q permanecera copiada la tarea del sistema operativo mientras se realiza la copia
		}else
			ESP_LOGE("Sensor_DHT11","No fue posible leer datos del DHT11");// representacion de error
	}
}


