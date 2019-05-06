#ifndef _SI7021_H
#define _SI7021_H

#include <stdint.h>

#define SI7021_SLAVEADDR 0x40

#define SI7021_REG_READ_HUM 0xE5
#define SI7021_REG_HUM_TEMP 0xE0
#define SI7021_REG_READ_TEMP 0xE3
#define SI7021_READ_USER_REG 0xE7
#define SI7021_REG_RESET 0xFE

typedef union {
    struct {
        uint8_t res0 : 1;  // resolution bit0
        uint8_t : 1;       // reserved
        uint8_t htre : 1;  // heater
        uint8_t : 3;       // reserved
        uint8_t vdds : 1;  // Vdd status
        uint8_t res1 : 1;  // resolution bit1
    } r;
    uint8_t b8;
} si7021_userreg;

ret_code_t si7021_init(const nrf_twi_mngr_t* twi);
ret_code_t si7021_read_user_reg(const nrf_twi_mngr_t* twi, si7021_userreg* userreg);
ret_code_t si7021_read_device_id(const nrf_twi_mngr_t* twi, uint8_t* deviceid);

typedef void (*si7021_read_sensor_cb)(float value, uint16_t rawvalue);

ret_code_t si7021_read_humidity(const nrf_twi_mngr_t* twi, si7021_read_sensor_cb cb);
ret_code_t si7021_read_temperature(const nrf_twi_mngr_t* twi, si7021_read_sensor_cb cb);

#endif  // _SI7021_H