#ifndef UAV_SRF
#define UAV_SRF

#include "driver/gpio.h"
#include "esp_rom_sys.h" // Thêm header cho esp_rom_delay_us
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TRIG_PIN GPIO_NUM_40
#define ECHO_PIN GPIO_NUM_41

void trigger_pulse();
int64_t measure_echo_pulse();
float calculate_distance(int64_t duration);
void srf_gpio_config();

#endif
