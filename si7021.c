
#include "app_timer.h"
#include "nrf_drv_twi.h"
#include "nrf_twi_sensor.h"
#include "sdk_macros.h"

#include "si7021.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

static const uint8_t regReadHum = SI7021_REG_READ_HUM;
static const uint8_t regReadTemp = SI7021_REG_HUM_TEMP;

#define HUMIDITY_REG_READ_DELAY_MS 25

APP_TIMER_DEF(m_delay_timer_id);
static uint8_t m_buffer[4];  // for sensor data

// http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
// Chapter. 4.3
static uint8_t updateCRC(uint8_t crc, uint8_t b)
{
    //  8   5   4
    // x + x + x + 1
    // const unsigned int pol = (1 << 8) | (1 << 5) | (1 << 4) | (1 << 0);
    const unsigned int pol = 0x131;

    crc ^= b;

    for (int i = 8; i > 0; i--) {
        if (crc & 0x80)
            crc = (crc << 1) ^ pol;
        else
            crc = (crc << 1);
    }
    return crc;
}

static uint8_t calcCRC(uint16_t val)
{
    uint8_t crc = 0;
    crc = updateCRC(crc, val >> 8);
    crc = updateCRC(crc, val & 0xFF);
    return crc;
}

ret_code_t si7021_read_user_reg(const nrf_twi_mngr_t* twi_mngr, si7021_userreg* userreg)
{
    ret_code_t err_code;
    uint8_t reg = SI7021_READ_USER_REG;
    nrf_twi_mngr_transfer_t transfers[] = {                                   //
        NRF_TWI_MNGR_WRITE(SI7021_SLAVEADDR, &reg, 1, NRF_TWI_MNGR_NO_STOP),  //
        NRF_TWI_MNGR_READ(SI7021_SLAVEADDR, (uint8_t*)userreg, 1, 0)};
    err_code = nrf_twi_mngr_perform(twi_mngr, NULL, transfers, ARRAYSIZE(transfers), NULL);
    return err_code;
}

static void humid_wait_timeout(void* p_context);

ret_code_t si7021_init(const nrf_twi_mngr_t* twi_mngr)
{
    // Create delay timer.
    ret_code_t err_code;
    err_code = app_timer_create(&m_delay_timer_id, APP_TIMER_MODE_SINGLE_SHOT, humid_wait_timeout);
    return err_code;
}

ret_code_t si7021_read_device_id(const nrf_twi_mngr_t* twi_mngr, uint8_t* deviceid)
{
    ret_code_t err_code;

    uint8_t cmd[2] = {0xFA, 0x0F};
    uint8_t buffer[8];
    nrf_twi_mngr_transfer_t transfers[] = {                                               //
        NRF_TWI_MNGR_WRITE(SI7021_SLAVEADDR, cmd, ARRAYSIZE(cmd), NRF_TWI_MNGR_NO_STOP),  //
        NRF_TWI_MNGR_READ(SI7021_SLAVEADDR, buffer, 8, 0)};
    err_code = nrf_twi_mngr_perform(twi_mngr, NULL, transfers, ARRAYSIZE(transfers), NULL);
    VERIFY_SUCCESS(err_code);
    uint32_t ser1 = 0;
    uint8_t crc = 0;

    for (int i = 0; i < 4; i++) {
        uint8_t b = buffer[i * 2];
        uint8_t csum = buffer[i * 2 + 1];
        ser1 <<= 8;
        ser1 |= b;

        crc = updateCRC(crc, b);

        // Check for data corruption
        if(crc != csum) return NRF_ERROR_INVALID_DATA;
    }

    cmd[0] = 0xFC;
    cmd[1] = 0xC9;
    nrf_twi_mngr_transfer_t transfers2[] = {                                              //
        NRF_TWI_MNGR_WRITE(SI7021_SLAVEADDR, cmd, ARRAYSIZE(cmd), NRF_TWI_MNGR_NO_STOP),  //
        NRF_TWI_MNGR_READ(SI7021_SLAVEADDR, buffer, 6, 0)};
    err_code = nrf_twi_mngr_perform(twi_mngr, NULL, transfers2, ARRAYSIZE(transfers2), NULL);
    VERIFY_SUCCESS(err_code);

    uint32_t ser2 = 0;
    crc = 0;

    for (int i = 0; i < 2; i++) {
        uint8_t b = buffer[i * 3];
        crc = updateCRC(crc, b);
        ser2 <<= 8;
        ser2 |= b;
        b = buffer[i * 3 + 1];
        crc = updateCRC(crc, b);
        ser2 <<= 8;
        ser2 |= b;
        uint8_t csum = buffer[i * 3 + 2];
        
        // Check for data corruption
        if(crc != csum) return NRF_ERROR_INVALID_DATA;
    }
    *deviceid = (ser2 >> 24) & 0x0FF;

    return NRF_SUCCESS;
}

typedef struct {
    nrf_twi_mngr_t const* p_nrf_twi_mngr;
    si7021_read_sensor_cb cb;
} si7021_read_humidity_ctx_t;

static void si7021_read_humidity_cb1(ret_code_t result, void* p_user_data)
{
    si7021_read_humidity_ctx_t* ctx = (si7021_read_humidity_ctx_t*)p_user_data;
    if (result == NRF_SUCCESS) {
        // request for humidity has been sent to device, wait 25 ms before attempting reading the data
        ret_code_t err_code;
        err_code = app_timer_start(m_delay_timer_id, APP_TIMER_TICKS(HUMIDITY_REG_READ_DELAY_MS), ctx);
        APP_ERROR_CHECK(err_code);
    } else {
        ctx->cb(-1, 0xFFFF);
    }
}

static void si7021_read_humidity_cb2(ret_code_t result, void* p_user_data);

static void humid_wait_timeout(void* p_context)
{
    // wait time has passed, now read the bytes
    si7021_read_humidity_ctx_t* ctx = (si7021_read_humidity_ctx_t*)p_context;
    ret_code_t err_code;
    static nrf_twi_mngr_transfer_t const transfers[] = {NRF_TWI_MNGR_READ(SI7021_SLAVEADDR, m_buffer, 3, 0)};

    static nrf_twi_mngr_transaction_t NRF_TWI_MNGR_BUFFER_LOC_IND transaction = {//
        .callback = si7021_read_humidity_cb2,                                    //
        .p_user_data = NULL,                                                     //
        .p_transfers = transfers,                                                //
        .number_of_transfers = ARRAYSIZE(transfers)};
    transaction.p_user_data = ctx;

    err_code = nrf_twi_mngr_schedule(ctx->p_nrf_twi_mngr, &transaction);

    APP_ERROR_CHECK(err_code);
}

static void si7021_read_humidity_cb2(ret_code_t result, void* p_user_data)
{
    float humf = -1;
    unsigned int hum = 0xFFFF;
    si7021_read_humidity_ctx_t* ctx = (si7021_read_humidity_ctx_t*)p_user_data;
    if (result == NRF_SUCCESS) {
        hum = m_buffer[0];         // MSB
        hum = hum << 8 | (m_buffer[1] & 0xFF);  // LSB
        uint8_t checksum = m_buffer[2];
        bool checksumOk = calcCRC(hum) == checksum;
        if (checksumOk) {
            hum &= 0xFFFC;  // lsb lowest bits are always 01
            humf = (125.0f * hum) / 65536 - 6;
        }
    }
    ctx->cb(humf, hum);
}

ret_code_t si7021_read_humidity(const nrf_twi_mngr_t* twi_mngr, si7021_read_sensor_cb user_cb)
{
    // Humidity reading is bit hacky. First write the register, then wait for 25ms
    // and then read the response.

    ret_code_t err_code;
    static nrf_twi_mngr_transfer_t const transfers[] = {NRF_TWI_MNGR_WRITE(SI7021_SLAVEADDR, &regReadHum, 1, 0)};

    static si7021_read_humidity_ctx_t ctx;
    ctx.p_nrf_twi_mngr = twi_mngr;
    ctx.cb = user_cb;

    static nrf_twi_mngr_transaction_t NRF_TWI_MNGR_BUFFER_LOC_IND transaction = {//
        .callback = si7021_read_humidity_cb1,                                    //
        .p_user_data = NULL,                                                     //
        .p_transfers = transfers,                                                //
        .number_of_transfers = ARRAYSIZE(transfers)};
    transaction.p_user_data = &ctx;

    err_code = nrf_twi_mngr_schedule(twi_mngr, &transaction);

    return err_code;
}

static void si7021_read_temperature_cb(ret_code_t result, void* p_user_data)
{
    float tempf = -1.0f;
    unsigned int temp = 0;
    if (result == NRF_SUCCESS) {
        int temp = m_buffer[0];                   // MSB
        temp = temp << 8 | (m_buffer[1] & 0xFF);  // LSB
        tempf = (175.72f * temp) / 65536 - 46.85f;
    }
    si7021_read_sensor_cb cb = (si7021_read_sensor_cb)p_user_data;
    cb(tempf, temp);
}

ret_code_t si7021_read_temperature(const nrf_twi_mngr_t* twi_mngr, si7021_read_sensor_cb user_cb)
{
    ret_code_t err_code;
    static nrf_twi_mngr_transfer_t const transfers[] = {
        NRF_TWI_MNGR_WRITE(SI7021_SLAVEADDR, &regReadTemp, 1, NRF_TWI_MNGR_NO_STOP),
        NRF_TWI_MNGR_READ(SI7021_SLAVEADDR, m_buffer, 2, 0)};

    static nrf_twi_mngr_transaction_t NRF_TWI_MNGR_BUFFER_LOC_IND transaction = {//
        .callback = si7021_read_temperature_cb,                                  //
        .p_user_data = NULL,                                                     //
        .p_transfers = transfers,                                                //
        .number_of_transfers = ARRAYSIZE(transfers)};
    transaction.p_user_data = user_cb;

    err_code = nrf_twi_mngr_schedule(twi_mngr, &transaction);

    return err_code;
}