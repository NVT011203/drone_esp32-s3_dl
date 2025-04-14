#include "imu.h"

void imu_init(i2c_master_bus_handle_t *bus_handle) {
  // Init I2C bus
  i2c_master_bus_config_t bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = false,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));
  ESP_LOGI(IMU_TAG, "I2C bus initialized at %d Hz", I2C_MASTER_FREQ_HZ);

  // Add MPU6050 in bus
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = MPU6050_ADDR,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };
  ESP_ERROR_CHECK(
      i2c_master_bus_add_device(*bus_handle, &dev_config, &mpu6050_handle));
  if (mpu6050_init(*bus_handle) != ESP_OK) {
    ESP_LOGE(IMU_TAG, "MPU6050 initialization failed. Stopping.");
    return;
  }
}

// MPU6050 write byte
static esp_err_t mpu6050_write_register(i2c_master_bus_handle_t bus_handle,
                                        uint8_t reg_addr, uint8_t data) {
  uint8_t write_buf[2] = {reg_addr, data};
  return i2c_master_transmit(mpu6050_handle, write_buf, 2, I2C_TIMEOUT_MS);
}

// MPU6050 read byte
static esp_err_t mpu6050_read_registers(i2c_master_bus_handle_t bus_handle,
                                        uint8_t reg_addr, uint8_t *data,
                                        size_t len) {
  return i2c_master_transmit_receive(mpu6050_handle, &reg_addr, 1, data, len,
                                     I2C_TIMEOUT_MS);
}

// MPU6050 init
static esp_err_t mpu6050_init(i2c_master_bus_handle_t bus_handle) {
  // Đánh thức MPU6050 (tắt sleep mode)
  esp_err_t ret = mpu6050_write_register(bus_handle, MPU6050_PWR_MGMT_1, 0x00);
  if (ret != ESP_OK) {
    ESP_LOGE(IMU_TAG, "Failed to wake up MPU6050: %s", esp_err_to_name(ret));
    return ret;
  }

  // Config accel gain: ±4g (0x08)
  ret = mpu6050_write_register(bus_handle, MPU6050_ACCEL_CONFIG,
                               MPU6050_ACCEL_4G);
  if (ret != ESP_OK) {
    ESP_LOGE(IMU_TAG, "Failed to configure accel: %s", esp_err_to_name(ret));
    return ret;
  }

  // Config gyro gain: ±1000°/s (0x10)
  ret = mpu6050_write_register(bus_handle, MPU6050_GYRO_CONFIG,
                               MPU6050_GYRO_1000);
  if (ret != ESP_OK) {
    ESP_LOGE(IMU_TAG, "Failed to configure gyro: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(IMU_TAG, "MPU6050 initialized successfully");
  return ESP_OK;
}

void mpu6050_read_task(void *pvParameters) {
  i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)pvParameters;
  uint8_t data[14]; // Store 14 byte: 6 (accel) + 2 (temp) + 6 (gyro)

  while (1) {
    // Read from ACCEL_XOUT_H to GYRO_ZOUT_L (14 byte)
    esp_err_t ret =
        mpu6050_read_registers(bus_handle, MPU6050_ACCEL_XOUT_H, data, 14);
    if (ret != ESP_OK) {
      ESP_LOGE(IMU_TAG, "Failed to read MPU6050 data: %s",
               esp_err_to_name(ret));
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    // Concatenate bit (16-bit)
    int16_t accel_x = (data[0] << 8) | data[1];
    int16_t accel_y = (data[2] << 8) | data[3];
    int16_t accel_z = (data[4] << 8) | data[5];
    int16_t gyro_x = (data[8] << 8) | data[9];
    int16_t gyro_y = (data[10] << 8) | data[11];
    int16_t gyro_z = (data[12] << 8) | data[13];

    // Convert to physical unit
    float accel_x_g = accel_x / ACCEL_SCALE_FACTOR;
    float accel_y_g = accel_y / ACCEL_SCALE_FACTOR;
    float accel_z_g = accel_z / ACCEL_SCALE_FACTOR;
    float gyro_x_dps = gyro_x / GYRO_SCALE_FACTOR;
    float gyro_y_dps = gyro_y / GYRO_SCALE_FACTOR;
    float gyro_z_dps = gyro_z / GYRO_SCALE_FACTOR;
    // Calc roll, pitch
    float roll = atan2(accel_y_g, accel_z_g) * DEG_PER_RAD;
    float pitch =
        atan2(-accel_x_g, sqrt(accel_y_g * accel_y_g + accel_z_g * accel_z_g)) *
        DEG_PER_RAD;

    // Print data to console
    ESP_LOGI(IMU_TAG, "Accel (g): X=%.2f, Y=%.2f, Z=%.2f", accel_x_g, accel_y_g,
             accel_z_g);
    ESP_LOGI(IMU_TAG, "Gyro (°/s): X=%.2f, Y=%.2f, Z=%.2f", gyro_x_dps,
             gyro_y_dps, gyro_z_dps);
    ESP_LOGI(IMU_TAG, "Angles (°): Roll=%.2f°, Pitch=%.2f°", roll, pitch);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
