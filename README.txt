Sample application using the HID, Battery and Device Information Service
 for implementing a simple mouse functionality. Alternatively it supports 
custom hid device mode that demonstrates how to read a external sensor 
over I2C and report the results as custom HID reports. Implementation also 
supports outgoing (from host to device) reports that can be used to 
command the device.

Environment:
	Built at nRF5_SDK_15.3.0_59ac345\examples\ble_peripheral
	Assumes BMD-350-EVAL board (compatible with PCA10040)

Options:

#define ENABLE_SENSOR

	Enables I2C (TWI) and communicates with si7021 Sensor. 
	Sensor humidity reading uses delayed read, client has to wait for 25 ms before 
	data is ready.

#define ENABLE_GPIO_INT

	Listens interrupts on pin 11 that toggle pin 12 and trigger temp and humidity reading from Sensor

#define CUSTOM_HID

	*** Chip must be erased and re-paired when enabling/disabling this define. ***

	When enabled exposes device as custom HID device that reports humidity and button presses as report id 1.
	Listens output reports that triggers humidity reading and report.
	
	
