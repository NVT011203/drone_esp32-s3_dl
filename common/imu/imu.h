#ifndef UAV_IMU
#define UAV_IMU

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <stdio.h>

#define I2C_MASTER_SCL_IO 40      // SCL
#define I2C_MASTER_SDA_IO 41      // SDA
#define I2C_MASTER_FREQ_HZ 400000 // I2C Freequency 400kHz
#define I2C_PORT I2C_NUM_0        // I2C port 0
#define I2C_TIMEOUT_MS 1000       // Timeout 1 second
#define MPU6050_ADDR 0x68         // Address MPU6050 (AD0 = GND)

// Thanh ghi MPU6050
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H 0x43

#define MPU6050_ACCEL_4G 0x08
#define MPU6050_GYRO_1000 0x10

// Thang đo
#define ACCEL_SCALE_FACTOR 8192.0f  // ±2g: 16384 LSB/g
#define GYRO_SCALE_FACTOR 32.8f     // ±250°/s: 131 LSB/(°/s)
#define DEG_PER_RAD (180.0f / M_PI) // Radian to degree

static const char *IMU_TAG = "IMU";

static i2c_master_dev_handle_t mpu6050_handle;
static i2c_master_bus_handle_t bus_handle;
// MPU6050 write byte
static esp_err_t mpu6050_write_register(i2c_master_bus_handle_t bus_handle,
                                        uint8_t reg_addr, uint8_t data);
// MPU6050 read byte
static esp_err_t mpu6050_read_registers(i2c_master_bus_handle_t bus_handle,
                                        uint8_t reg_addr, uint8_t *data,
                                        size_t len);
// Init MPU6050
static esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle);
void mpu6050_read_task(void *pvParameters);
void imu_init(i2c_master_bus_handle_t *bus_handle);

#endif
