
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

#include "esp_camera.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"

#include "camera.h"
#include "chip_info.h"
#include "imu.h"
#include "srf.h"

#define WIFI_SSID "NVT"
#define WIFI_PASS "12345678910"
#define UDP_SERVER_IP "192.168.18.195"
#define UDP_PORT 1234
#define LOCAL_PORT 12346
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_UDP_IP_CORE_ID 0
#define MAIN_CORE 1

static const char *MAIN_TAG = "MAIN";
static const char *WIFI_TAG = "UDP_WIFI";

static EventGroupHandle_t wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
void wifi_init_sta(void);
void udp_stream_task(void *pvParameters);
static void udp_receive_task(void *pvParameters);

void app_main(void) {
  ESP_LOGI(MAIN_TAG, "Start Program!");
  ESP_LOGI(MAIN_TAG, "Chip Info Start ---------------------------");
  chip_info();
  ESP_LOGI(MAIN_TAG, "Chip Info End -----------------------------");

  // Init
  nvs_flash_init();
  wifi_init_sta();
  camera_init();

  srf_gpio_config();

  /*while (1) {*/
  /*  // Gửi xung TRIG*/
  /*  trigger_pulse();*/
  /**/
  /*  // Đo thời gian xung ECHO*/
  /*  int64_t duration = measure_echo_pulse();*/
  /**/
  /*  // Tính khoảng cách*/
  /*  float distance = calculate_distance(duration);*/
  /**/
  /*  // In kết quả*/
  /*  printf("Khoang cach: %.2f cm\n", distance);*/
  /**/
  /*  // Đợi 100ms trước khi đo lại*/
  /*  vTaskDelay(100 / portTICK_PERIOD_MS);*/
  /*}*/

  // MPU6050 init
  // imu_init(&bus_handle);

  // FreeRTOS task
  // xTaskCreate(capture_video, "capture_video", 4096, NULL, 5, NULL);
  xTaskCreatePinnedToCore(&udp_stream_task, "udp_stream_task", 8192, NULL, 5,
                          NULL, WIFI_UDP_IP_CORE_ID);
  xTaskCreatePinnedToCore(&udp_receive_task, "udp_receive_task", 8192, NULL, 5,
                          NULL, MAIN_CORE);
  // Read imu data
  // xTaskCreate(mpu6050_read_task, "mpu6050_read_task", 4096, (void
  // *)bus_handle,
  //             5, NULL);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void wifi_init_sta(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  wifi_event_group = xEventGroupCreate();

  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &wifi_event_handler, NULL,
                                      &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &wifi_event_handler, NULL,
                                      &instance_got_ip);

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = WIFI_SSID,     // Wifi SSID
              .password = WIFI_PASS, // Wifi password
          },
  };
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();

  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE,
                      portMAX_DELAY);
  ESP_LOGI(WIFI_TAG, "WiFi connected, starting UDP...");
}

// Task nhận dữ liệu UDP (chạy trên lõi 1)
static void udp_receive_task(void *pvParameters) {
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    ESP_LOGE(WIFI_TAG, "Failed to create socket");
    vTaskDelete(NULL);
  }

  struct sockaddr_in local_addr = {
      .sin_addr.s_addr = INADDR_ANY,
      .sin_family = AF_INET,
      .sin_port = htons(LOCAL_PORT),
  };

  if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
    ESP_LOGE(WIFI_TAG, "Failed to bind socket");
    close(sock);
    vTaskDelete(NULL);
  }

  char rx_buffer[1024];
  struct sockaddr_in source_addr;
  socklen_t socklen = sizeof(source_addr);

  while (1) {
    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                       (struct sockaddr *)&source_addr, &socklen);
    if (len < 0) {
      ESP_LOGE(WIFI_TAG, "recvfrom failed: %d", errno);
    } else {
      rx_buffer[len] = 0; // Null-terminate
      ESP_LOGI(WIFI_TAG, "Received %d bytes from %s:%d: %s", len,
               inet_ntoa(source_addr.sin_addr), ntohs(source_addr.sin_port),
               rx_buffer);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  close(sock);
  vTaskDelete(NULL);
}

void udp_stream_task(void *pvParameters) {
  struct sockaddr_in dest_addr;
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    ESP_LOGE(CAMERA_TAG, "Unable to create socket: errno %d", errno);
    return;
  }

  dest_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(UDP_PORT);

  TickType_t last_time = xTaskGetTickCount();
  while (1) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      ESP_LOGE(CAMERA_TAG, "Camera capture failed");
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    int sent = sendto(sock, fb->buf, fb->len, 0, (struct sockaddr *)&dest_addr,
                      sizeof(dest_addr));
    if (sent < 0) {
      ESP_LOGE(CAMERA_TAG, "Error sending UDP: errno %d", errno);
    } else {
      ESP_LOGI(CAMERA_TAG, "Sent frame: %d bytes", sent);

      // Tính FPS
      TickType_t current_time = xTaskGetTickCount();
      float fps = 1000.0 / ((current_time - last_time) * portTICK_PERIOD_MS);
      ESP_LOGI(CAMERA_TAG, "FPS: %.2f", fps);
      last_time = current_time;
    }

    esp_camera_fb_return(fb);
    vTaskDelay(50 / portTICK_PERIOD_MS); // ~20 FPS
  }

  close(sock);
}
