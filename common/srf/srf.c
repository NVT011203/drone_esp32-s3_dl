#include "srf.h"

void srf_gpio_config() {
  // Cấu hình GPIO
  gpio_config_t io_conf = {};

  // Cấu hình TRIG là OUTPUT
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << TRIG_PIN);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  // Cấu hình ECHO là INPUT
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
  gpio_config(&io_conf);
}

// Hàm gửi xung TRIG
void trigger_pulse() {
  gpio_set_level(TRIG_PIN, 0);
  esp_rom_delay_us(2); // Đợi 2us
  gpio_set_level(TRIG_PIN, 1);
  esp_rom_delay_us(10); // Gửi xung 10us
  gpio_set_level(TRIG_PIN, 0);
}

// Hàm đo thời gian xung ECHO
int64_t measure_echo_pulse() {
  // Chờ ECHO chuyển sang HIGH
  while (gpio_get_level(ECHO_PIN) == 0) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  // Ghi lại thời gian bắt đầu
  int64_t start_time = esp_timer_get_time();

  // Chờ ECHO chuyển sang LOW
  while (gpio_get_level(ECHO_PIN) == 1) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  // Ghi lại thời gian kết thúc
  int64_t end_time = esp_timer_get_time();

  return end_time - start_time; // Trả về thời gian (micro giây)
}

// Hàm tính khoảng cách
float calculate_distance(int64_t duration) {
  // Tốc độ âm thanh: 343 m/s = 0.0343 cm/us
  // Khoảng cách = (thời gian * tốc độ) / 2
  return (duration * 0.0343) / 2.0;
}
