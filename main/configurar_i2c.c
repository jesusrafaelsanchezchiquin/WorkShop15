//archivos Espressif API
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//las dos anteriores no son necesarias
#include "driver/i2c.h"// definiciones para configurar el I2c
#include "driver/gpio.h"// manipulacion de terminales digitales
// Macros y definiciones
#define pinSDA 19
#define pinSCL 18
#define i2c_clock 100000// frecuencia del protocolo se usara 100 kHZ (el mas comun)

void iniciar_i2c(){// debe estar declaradad en el archivo cabecera porq sera llamada
	i2c_config_t configuracion;// especificacion de la configuracion
	configuracion.mode =I2C_MODE_MASTER;//modo de configuracion el ESP32 sera el mestro y el ds3231 el esclavo
	configuracion.sda_io_num = pinSDA;
	configuracion.scl_io_num = pinSCL;
	configuracion.sda_pullup_en =GPIO_PULLUP_ENABLE;// habilitamos resistencias de PUllup interanas del micro
	configuracion.scl_pullup_en =GPIO_PULLUP_ENABLE;
	configuracion.master.clk_speed = i2c_clock; //definimos cual sera el clock
	i2c_param_config(I2C_NUM_0, &configuracion);//cargamos datos
	i2c_driver_install(I2C_NUM_0,I2C_MODE_MASTER,0,0,0);// instalamos el controlador
}// los dos primeros ceros son la asignacion de tama;o de buffer y solo se usan cuando se configura como esclavo
// el tercer cero es la bandera de interupciones (que no se esta usando)
