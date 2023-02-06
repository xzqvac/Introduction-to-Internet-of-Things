#include <driver/i2c.h>

// Hardware configuration
#define LED_GPIO_PIN                            GPIO_NUM_1
#define I2C_PORT_NUMBER                         I2C_NUM_0
#define I2C_CLK_FREQUENCY                       100000
#define I2C_SDA_PIN                             GPIO_NUM_4
#define I2C_SCL_PIN                             GPIO_NUM_5
#define I2C_TIMEOUT                             (100 / portTICK_RATE_MS)
#define I2C_AHT20_ADDRESS                       0x38

// Sensor configuration
#define CMD_INITIALIZATION                      0xBE
#define CMD_SOFTRESET                           0xBA
#define CMD_CALIBRATE                           0x71    
#define CMD_TRIGGER_MEAS                        0xAC

static int16_t temperature;
static uint16_t humidity;

static void WaitMs(unsigned delay) {
    vTaskDelay(delay / portTICK_PERIOD_MS);
}

static void InitializeI2C(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_CLK_FREQUENCY,
    };

    i2c_param_config(I2C_PORT_NUMBER, &conf);
    i2c_driver_install(I2C_PORT_NUMBER, conf.mode, 0, 0, 0);
}

static void WriteToTheSensor(uint8_t *data, size_t length) {
    i2c_master_write_to_device(I2C_PORT_NUMBER, I2C_AHT20_ADDRESS, data, length, I2C_TIMEOUT);
}

static void ReadFromTheSensor(uint8_t *buffer, size_t length) {
    i2c_master_read_from_device(I2C_PORT_NUMBER, I2C_AHT20_ADDRESS, buffer, length, I2C_TIMEOUT);
}

static void GetData()
{
    // Reset sensor
    uint8_t softReset = CMD_SOFTRESET;
    WriteToTheSensor(&softReset, 1);
    WaitMs(20);

    // Calibration
    uint8_t calibrate = CMD_CALIBRATE;
    WriteToTheSensor(&calibrate, 3);
    WaitMs(100);

    // Trigger measurement
    uint8_t triggerMeas = CMD_TRIGGER_MEAS;
    WriteToTheSensor(&triggerMeas, 3);
    WaitMs(300);

    // Get measurment data
    uint8_t dataFrame[6];
    ReadFromTheSensor(&dataFrame, 6);

    // Evaluate values from I2C frames
    uint32_t humidityFrame = dataFrame[1];
    humidityFrame <<= 8;
    humidityFrame |= dataFrame[2];
    humidityFrame <<= 4;
    humidityFrame |= dataFrame[3] >> 4;

    humidity = ((float)humidityFrame * 100) / 1048576; // signal transformation

    uint32_t temperatureFrame = 0x0F | dataFrame[3];
    temperatureFrame <<= 8;
    temperatureFrame |= dataFrame[4];
    temperatureFrame <<=8;
    temperatureFrame |= dataFrame[5];

    temperature = ((float)temperatureFrame * 200 / 1048576) - 50; // signal transformation
    temperature *= 0.01;

    //printf("Wilgotnosc: %f\n", humidity);
    //printf("Temperatura: %f\n", temperature);
}