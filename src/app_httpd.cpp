// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"

#define COLOR_GREEN 0x0000FF00
#define COLOR_RED 0x00FF0000
#define COLOR_BLUE 0x000000FF

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t cmd_httpd = NULL;

#include <Arduino.h>

#include "control.h"

extern int g_use_dnn; // defined in src/main.cpp

// httpd handler for the stream
// Core1, priority=5, stack=4096
static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[128];
        
    static int64_t last_frame = 0;
    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }
    int64_t fr_cap, fr_pre, fr_dnn, fr_enc;

    sensor_t *s = esp_camera_sensor_get();
    if (s->status.framesize > 0) {
        Serial.printf("Camera info: framesize=%d, pixel_format=%d\n", s->status.framesize, s->pixformat);
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    while (true)
    {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        fr_pre = esp_timer_get_time();

        fb = esp_camera_fb_get();
        
        fr_cap = esp_timer_get_time();

        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;
            Serial.printf("fb: %dx%d, format: %d, len: %d\n", fb->width, fb->height, fb->format, fb->len);

            if (fb->format != PIXFORMAT_JPEG) {
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!jpeg_converted) {
                    Serial.println("JPEG compression failed");
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
            fr_enc = esp_timer_get_time();
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }        
        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK) {
            Serial.println("Send frame failed");
            break;
        }
        int64_t fr_end = esp_timer_get_time();

        int64_t frame_time = (fr_end - last_frame)/1000;
    
        last_frame = fr_end;

        // sleep 
        BaseType_t xWasDelayd = pdTRUE;
        if (g_use_dnn)
            xWasDelayd = xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000)); // 1fps
        else
            xWasDelayd = xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000 / 20)); // 20fps
            
        if (xWasDelayd == pdFALSE) {
            log_w("Task was blocked for longer than the set period");       
        }
        printf("Core%d: %s (prio=%d): %u ms (%.1ffps): enc: %d ms\n",
            xPortGetCoreID(), pcTaskGetName(NULL), uxTaskPriorityGet(NULL),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time, (uint32_t)((fr_enc - fr_cap)/1000));            
    }
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char *buf = NULL;
    size_t buf_len = 0;
    char variable[32];
    char value[32];

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            }
            Serial.printf("Found URL query parameter => var=%s, val=%s\n", variable, value);
        }
        free(buf);
    }

    int val = atoi(value);
    int res = 0;

    if(!strcmp(variable, "auto")) {
        Serial.println("Autonomous mode");
        g_use_dnn = 1;
        sensor_t *s = esp_camera_sensor_get();
        if (s->pixformat != PIXFORMAT_RGB565) {
            s->set_pixformat(s, PIXFORMAT_RGB565);
        }
    } else if(!strcmp(variable, "manual")) {
        Serial.println("Manual mode");
        g_use_dnn = 0;
        sensor_t *s = esp_camera_sensor_get();
        if (s->pixformat != PIXFORMAT_JPEG) {
            s->set_pixformat(s, PIXFORMAT_JPEG);
        }
    } else if(!strcmp(variable, "throttle_pct")) {
        // printf("Core%d: %s (prio=%d): updated throttle %d\n",
        //     xPortGetCoreID(), pcTaskGetName(NULL), uxTaskPriorityGet(NULL), val);
        set_throttle(val);
    } else if(!strcmp(variable, "steering_deg")) {
        // printf("Core%d: %s (prio=%d): updated steering %d deg\n",
        //     xPortGetCoreID(), pcTaskGetName(NULL), uxTaskPriorityGet(NULL), val);
        set_steering(val);
    } else if (!strcmp(variable, "framesize")) {
        sensor_t *s = esp_camera_sensor_get();
        res = s->set_framesize(s, (framesize_t)val);
        printf("Camera info: framesize=%d, pixel_format=%d\n", s->status.framesize, s->pixformat);
    } else {
        log_i("Unknown command: %s", variable);
        res = -1;
    }

    if (res < 0) {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t cmd_uri = {
    .uri = "/control",
    .method = HTTP_GET,
    .handler = cmd_handler,
    .user_ctx = NULL,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
  };

  Serial.printf("Starting control server on port: '%d'\n", config.server_port);
  if (httpd_start(&cmd_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(cmd_httpd, &cmd_uri);
  }  
  config.server_port += 1;  // 81
  config.ctrl_port += 1;    // 32769
  Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

