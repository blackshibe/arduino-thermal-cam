
#include "printer.h"
#include "Adafruit_Thermal.h"
#include "SPI.h"

const int printerBaudrate = 9600; // or 19200
const byte rxPin = -1;			  // useless after printer initialization, does not need to be connected
const byte txPin = 3;

HardwareSerial serial(1);
Adafruit_Thermal printer(&serial);

printer_controller printer_controller::instance;

esp_err_t printer_controller::init() {
	pinMode(txPin, OUTPUT);

	serial.begin(printerBaudrate, SERIAL_8N1, rxPin, txPin); // must be 8N1 mode
	printer.begin();

	this->printer = printer;

	return ESP_OK;
}
