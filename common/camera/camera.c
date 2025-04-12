#include "camera.h"

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GOIO_NUM,
    .xclk_freq_hz = 8000000, // 20MHz clock
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG, // Định dạng JPEG cho video
    .frame_size = FRAMESIZE_QQVGA,  // 320x240
    .jpeg_quality = 25, // Chất lượng JPEG (15 để cân bằng kích thước và tốc độ)
    .fb_count = 3,      // 2 frame buffer để tăng frame rate
    .grab_mode = CAMERA_GRAB_LATEST,  // Lấy frame mới nhất
    .fb_location = CAMERA_FB_IN_PSRAM // Lưu frame trong PSRAM
};

void camera_init() {
  if (camera_config.pin_reset != -1) {
    gpio_set_direction(camera_config.pin_reset, GPIO_MODE_OUTPUT);
    gpio_set_level(camera_config.pin_reset, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(camera_config.pin_reset, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    ESP_LOGE(CAMERA_TAG, "Camera init failed with error 0x%x", err);
    return;
  }
  ESP_LOGI(CAMERA_TAG, "Camera initialized successfully");

  // Tinh chỉnh camera để tăng frame rate
  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QQVGA); // Xác nhận lại 320x240
  s->set_quality(s, 25);                // Chất lượng JPEG
  s->set_vflip(s, 0);                   // Không lật dọc
  s->set_hmirror(s, 0);                 // Không lật ngang
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
}
