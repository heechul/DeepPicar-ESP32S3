#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_system.h"
#include <stdio.h>

#include "esp_camera.h"
#include <WiFi.h>
#include "NeuralNetwork.h"

// enable deeppicar dnn by default
int g_use_dnn = 0; // set by web server

// DNN model pointer
NeuralNetwork *g_nn;

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#include "camera_pins.h"

#include "control.h" // motor control

// ===========================
// Enter your WiFi credentials
// ===========================
#define SETUP_AP 1 // 1: setup AP mode, 0: setup Station mode
#define WAIT_SERIAL 1 // 1: wait for serial monitor, 0: don't wait

#if SETUP_AP==1
const char* ssid = "ESP32S3-DR";
const char* password = "123456789"; 
#else
const char* ssid = "";
const char* password = "";
#endif

void startCameraServer();
void setupLedFlash(int pin);
void dnn_loop();

// Function to be run on core 0
void taskCore0(void *pvParameters) {
  while (true) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    if (g_use_dnn) {
      dnn_loop();
      vTaskDelay(1);
    } else {
      BaseType_t xWasDelayd = xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000)); // 1fps
      // print core id, task name, task priority
      printf("Core%d: %s (prio=%d, delayed=%d)\n", 
        xPortGetCoreID(), pcTaskGetName(NULL), uxTaskPriorityGet(NULL), xWasDelayd);
    }
  }
}

void setup() {
  Serial.begin(115200);
#if WAIT_SERIAL==1
  while(!Serial) {
    static int retries = 0;
    delay(1000); // Wait for serial monitor to open
    if (retries++ > 5) {
      break;
    }
  } // When the serial monitor is turned on, the program starts to execute
#endif
  Serial.setDebugOutput(false);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QQVGA;
  // config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_QQVGA;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 3;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  } else {
    Serial.println("Camera init success!");
  }

  Serial.printf("Camera info: framesize=%d, pixel_format=%d\n", config.frame_size, config.pixel_format);

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

#if SETUP_AP==1
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);
  Serial.print("Use 'http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("' to connect");
#else
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
#endif

  // printf("wifi_task_core_id: %d\n", CONFIG_ESP32_WIFI_TASK_CORE_ID);

  startCameraServer();

  // Create a task pinned to core 0
  xTaskCreatePinnedToCore(
      taskCore0,   // Function to be called
      "TaskCore0", // Name of the task
      10000,       // Stack size (bytes)
      NULL,        // Parameter to pass
      2,           // Task priority
      NULL,        // Task handle
      0            // Core to run the task on (0 or 1)
  );

  // setup motor control
  setup_control();

  // register pilotnet algo
  g_nn = new NeuralNetwork();

  // end of init
  Serial.println("Ready");

}

void loop() {
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // print core id, task name, task priority
  printf("Core%d: %s (prio=%d)\n",
    xPortGetCoreID(), 
    pcTaskGetName(NULL),
    uxTaskPriorityGet(NULL));

  BaseType_t xWasDelayd = xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));

}

#define DEBUG_TFLITE 0

#if DEBUG_TFLITE==1
#include "img.h"  // Use a static image for debugging
#endif

// prepare input image tensor
#define USE_INT8 1

#include "NeuralNetwork.h"
extern NeuralNetwork *nn;


uint32_t rgb565torgb888(uint16_t color)
{
    uint8_t hb, lb;
    uint32_t r, g, b;

    lb = (color >> 8) & 0xFF;
    hb = color & 0xFF;

    r = (lb & 0x1F) << 3;
    g = ((hb & 0x07) << 5) | ((lb & 0xE0) >> 3);
    b = (hb & 0xF8);
    
    return (r << 16) | (g << 8) | b;
}

#if USE_CROP==1
#define GetImage GetImageCrop
#else
#define GetImage GetImageResize
#endif

int GetImageResize(camera_fb_t * fb, TfLiteTensor* input) 
{
    // MicroPrintf("fb: %dx%d-fmt:%d-len:%d INPUT: %dx%d", fb->width, fb->height, fb->format, fb->len, INPUT_W, INPUT_H);
    assert(fb->format == PIXFORMAT_RGB565);

    int x_scale = fb->width / INPUT_W;
    int y_scale = fb->height / INPUT_H;
    // MicroPrintf("x_scale=%d y_scale=%d\n", x_scale, y_scale);

    int post = 0;
    for (int y = 0; y < INPUT_H; y++) {
        for (int x = 0; x < INPUT_W; x++) {
            int getPos = y_scale * y * fb->width + x * x_scale;
            uint16_t color = ((uint16_t *)fb->buf)[getPos];
            uint32_t rgb = rgb565torgb888(color);
            uint8_t r = (rgb >> 16) & 0xFF; // rgb_image_data[getPos*3];
            uint8_t g = (rgb >>  8) & 0xFF; // rgb_image_data[getPos*3+1];
            uint8_t b = (rgb >>  0) & 0xFF; // rgb_image_data[getPos*3+2];
#if USE_INT8==1
            int8_t *image_data = input->data.int8;
            image_data[post * 3 + 0] = (int)r - 128;  // R
            image_data[post * 3 + 1] = (int)g - 128;  // G
            image_data[post * 3 + 2] = (int)b - 128;  // B
            // if (post < 3) printf("input[%d]: %d %d %d\n", post, image_data[post * 3 + 0] + 128, image_data[post * 3 + 1] + 128, image_data[post * 3 + 2] + 128);
#else
            float *image_data = input->data.f;
            image_data[post * 3 + 0] = (float) r / 255.0;
            image_data[post * 3 + 1] = (float) g / 255.0;
            image_data[post * 3 + 2] = (float) b / 255.0;
#endif /* USE_INT8*/
            post++;
        }
    }
    return 0;
}
int GetImageCrop(camera_fb_t * fb, TfLiteTensor* input) 
{
    // MicroPrintf("fb: %dx%d-fmt:%d-len:%d INPUT: %dx%d", fb->width, fb->height, fb->format, fb->len, INPUT_W, INPUT_H);
    assert(fb->format == PIXFORMAT_RGB565);

    // Trimming Image
    int post = 0;
    int startx = (fb->width - INPUT_W) * 0.50;
    int starty = (fb->height - INPUT_H) * 0.75;
    // printf("startx=%d starty=%d\n", startx, starty);

    for (int y = 0; y < INPUT_H; y++) {
        for (int x = 0; x < INPUT_W; x++) {
            int getPos = (starty + y) * fb->width + startx + x;
            // MicroPrintf("input[%d]: fb->buf[%d]=%d\n", post, getPos, fb->buf[getPos]);
            uint16_t color = ((uint16_t *)fb->buf)[getPos];
            uint32_t rgb = rgb565torgb888(color);
            uint8_t r = (rgb >> 16) & 0xFF; // rgb_image_data[getPos*3];
            uint8_t g = (rgb >>  8) & 0xFF; // rgb_image_data[getPos*3+1];
            uint8_t b = (rgb >>  0) & 0xFF; // rgb_image_data[getPos*3+2];
#if USE_INT8==1
            int8_t *image_data = input->data.int8;
            image_data[post * 3 + 0] = (int)r - 128;  // R
            image_data[post * 3 + 1] = (int)g - 128;  // G
            image_data[post * 3 + 2] = (int)b - 128;  // B
            // if (post < 3) printf("input[%d]: %d %d %d\n", post, image_data[post * 3 + 0] + 128, image_data[post * 3 + 1] + 128, image_data[post * 3 + 2] + 128);
#else
            float *image_data = input->data.f;
            image_data[post * 3 + 0] = (float) r / 255.0;
            image_data[post * 3 + 1] = (float) g / 255.0;
            image_data[post * 3 + 2] = (float) b / 255.0;
#endif /* USE_INT8*/
            post++;
        }
    }

    return 0;
}

inline float rad2deg(float rad) 
{
  return 180.0*rad/3.14;
}

// steering
#define CENTER 0
#define RIGHT 1
#define LEFT 2

int GetAction(float rad)
{
    int deg = (int)rad2deg(rad);
    if (deg < 10 and deg > -10)
        return CENTER;
    else if (deg >= 10)
        return RIGHT;
    else if (deg < -10)
        return LEFT;
    return -1;
}

void dnn_loop()
{
  camera_fb_t *fb = NULL;
  int64_t fr_begin, fr_cap, fr_pre, fr_dnn;
  static int64_t last_frame = 0;

  fr_begin = esp_timer_get_time();

  if (!last_frame)
    last_frame = fr_begin;

  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  fr_cap = esp_timer_get_time();

#if DEBUG_TFLITE==0
  GetImage(fb, g_nn->getInput());
#else
  // Use a static image for debugging
  memcpy(g_nn->getInput()->data.int8, img_data, sizeof(img_data));
  printf("input: %d %d %d...\n", 
      g_nn->getInput()->data.int8[0], g_nn->getInput()->data.int8[1], g_nn->getInput()->data.int8[2]);
#endif
  fr_pre = esp_timer_get_time();

  if (kTfLiteOk != g_nn->predict())
  {
      printf("Invoke failed.\n");
  }
#if USE_INT8==1
  int q = g_nn->getOutput()->data.int8[0];
  float scale = g_nn->getOutput()->params.scale;
  int zero_point = g_nn->getOutput()->params.zero_point;
  float angle = (q - zero_point) * scale;
#else
  float angle = g_nn->getOutput()->data.f[0];
#endif
  fr_dnn = esp_timer_get_time();

  int deg = (int)rad2deg(angle);
  
  // if (deg < 10 and deg > -10) 
  // {
  //   center();
  //   printf("center (%d) by CPU", deg);
  // } 
  // else if (deg >= 10) 
  // {
  //   right();
  //   printf("right (%d) by CPU", deg);
  // } 
  // else if (deg <= -10) 
  // {
  //   left();
  //   printf("left (%d) by CPU", deg);
  // }
  set_steering(deg);
  printf("deg=%d (%.3f q=%d)\n", deg, angle, q);

  if (fb)
  {
      esp_camera_fb_return(fb);
      fb = NULL;
  }
  int64_t fr_end = esp_timer_get_time();
  int64_t frame_time = (fr_end - last_frame)/1000;
  if (g_use_dnn) printf("Core%d: %s (prio=%d) %ums (%.1ffps): cap=%dms, pre=%dms, dnn=%dms\n", 
      xPortGetCoreID(), pcTaskGetName(NULL), uxTaskPriorityGet(NULL), 
      (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
      (int)((fr_cap-fr_begin)/1000), (int)((fr_pre-fr_cap)/1000), (int)((fr_dnn-fr_pre)/1000));

  last_frame = fr_end;  
}
