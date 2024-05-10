/* Edge Impulse Arduino examples
   Copyright (c) 2022 EdgeImpulse Inc.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
*  Developed by: Solomon Muhunyo Githu, 10th May 2024.
*/

/* Includes ---------------------------------------------------------------- */
#include "utils.h"
#include "centroid_tracker.h"

#include <Object_Detection_-_nuts_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

#include <WiFi.h>
#include "esp_camera.h"
/* ************************************************************************ */

/* Camera defines --------------------------------------------------------- */
// Select camera model - find more camera models in camera_pins.h file here
// https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/Camera/CameraWebServer/camera_pins.h

// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

camera_fb_t *fb_curr = NULL;

static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,

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
  .pin_pclk = PCLK_GPIO_NUM,

  //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
  .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

  .jpeg_quality = 12, //0-63 lower number means higher quality
  .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
  .fb_location = CAMERA_FB_IN_PSRAM,
  .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};
/* ************************************************************************ */

/* Wi-Fi defines ----------------------------------------------------------- */
WiFiServer server(80);
bool connected = false;
WiFiClient live_client;

const char* ssid = ""; // put your Wi-Fi SSID here
const char* password = ""; // put your Wi-Fi password here
String ip_address;
/* ************************************************************************ */

/* Centroid tracker defines ------------------------------------------------ */
CentroidTracker tracker;
// Create a vector to hold the bounding boxes
std::vector<BoundingBox> rects_init;
/* ************************************************************************ */

/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;

size_t out_len, out_width, out_height;
uint8_t *out_buf;
bool s;
uint8_t *snapshot_buf; //points to the output of the capture

/* Function definitions ------------------------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) ;

// this function serves the camera images
void liveCam(WiFiClient &client) {
  if (!fb_curr) {
    Serial.println("Frame buffer could not be acquired");
    return;
  }

  client.print("--frame\n");
  client.print("Content-Type: image/jpeg\n\n");
  client.flush();
  client.write(fb_curr->buf, fb_curr->len);
  client.flush();
  client.print("\n");
}

void http_resp() {
  WiFiClient client = server.available();
  /* check client is connected */
  if (client.connected()) {
    /* client send request? */
    /* request end with '\r' -> this is HTTP protocol format */
    String req = "";
    while (client.available()) {
      req += (char)client.read();
    }
    Serial.println("request " + req);
    /* First line of HTTP request is "GET / HTTP/1.1"
      here "GET /" is a request to get the first page at root "/"
      "HTTP/1.1" is HTTP version 1.1
    */
    /* now we parse the request to see which page the client want */
    int addr_start = req.indexOf("GET") + strlen("GET");
    int addr_end = req.indexOf("HTTP", addr_start);
    if (addr_start == -1 || addr_end == -1) {
      Serial.println("Invalid request " + req);
      return;
    }
    req = req.substring(addr_start, addr_end);
    req.trim();
    Serial.println("Request: " + req);
    client.flush();

    String s;
    /* if request is "/" then client request the first page at root "/" -> we process this by return "Hello world"*/
    if (req == "/")
    {
      s = "HTTP/1.1 200 OK\n";
      s += "Content-Type: text/html\n\n";
      s += index_html;
      s += "\n";
      client.print(s);
      client.stop();
    }
    else if (req == "/video")
    {
      live_client = client;
      live_client.print("HTTP/1.1 200 OK\n");
      live_client.print("Content-Type: multipart/x-mixed-replace; boundary=frame\n\n");
      live_client.flush();
      connected = true;
    }

    // This is a custom GET route that will publish the found objects in each void loop() run
    else if (req == "/found_objects")
    {
      s = "HTTP/1.1 200 OK\n";
      // send the bounding boxes list as JSON
      s += "Content-Type: application/json\n\n";
      s += current_found_objects_JSONData;
      s += "\n";
      client.print(s);
      client.stop();
    }
    else
    {
      /* if we can not find the page that client request then we return 404 File not found */
      s = "HTTP/1.1 404 Not Found\n\n";
      client.print(s);
      client.stop();
    }
  }
}

void serve_video_and_found_objects() {
  set_index_html();
  /*
    Serial.println("--------------------------------------------");
    Serial.print("current_found_objects_JSONData : ");
    Serial.println(current_found_objects_JSONData);
    Serial.println("--------------------------------------------");
  */

  http_resp();
  if (connected == true) {
    liveCam(live_client);
  }
}
/* ************************************************************************ */

/**
  @brief      Arduino setup function
*/
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  ip_address = WiFi.localIP().toString();
  Serial.println("IP address: " + ip_address);
  index_html.replace("server_ip", ip_address);
  server.begin();

  //comment out the below line to start inference immediately after upload
  //while (!Serial);
  Serial.println("Edge Impulse Inferencing Demo");
  if (ei_camera_init() == false) {
    ei_printf("Failed to initialize Camera!\r\n");
  }
  else {
    ei_printf("Camera initialized\r\n");
  }

  ei_printf("\nStarting continious inference in 2 seconds...\n");
  ei_sleep(2000);
}

/**
  @brief      Get data and run inferencing

  @param[in]  debug  Get debug info if true
*/
void loop()
{
  // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
  if (ei_sleep(5) != EI_IMPULSE_OK) {
    return;
  }

  snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

  // check if allocation was successful
  if (snapshot_buf == nullptr) {
    ei_printf("ERR: Failed to allocate snapshot buffer!\n");
    return;
  }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
    ei_printf("Failed to capture image\r\n");
    free(snapshot_buf);
    return;
  }

  // Run the classifier
  ei_impulse_result_t result = { 0 };

  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK) {
    ei_printf("ERR: Failed to run classifier (%d)\n", err);
    return;
  }

  // print the predictions
  ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
  bool bb_found = result.bounding_boxes[0].value > 0;

  for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {

    auto bb = result.bounding_boxes[ix];
    if (bb.value == 0) {
      continue;
    }
    ei_printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);

    // add bounding boxes to rects_init vector for the object tracking
    rects_init.emplace_back(BoundingBox{bb.x, bb.y, bb.width, bb.height});
    // add bounding boxes to JSON
    append_found_objects_to_JSON(bb.x, bb.y, bb.width, bb.height);
  }
  if (!bb_found) {
    ei_printf("    No objects found\n");
  }
#else
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    ei_printf("    %s: %.5f\n", result.classification[ix].label,
              result.classification[ix].value);
  }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

  // do the object tracking
  std::vector<int> objectIDs = tracker.update(rects_init);
  
  // Get object IDs and centroids
  std::vector<std::pair<int, int>> centroids = tracker.getCentroids();

  int number_of_tracked_objects = objectIDs.size();
  int objectIDs_and_centroids_array[number_of_tracked_objects][3];
  // Print object IDs and centroids
  Serial.println("Object IDs and their centroids....");
  for (size_t i = 0; i < objectIDs.size(); ++i) {
    Serial.print("Object ID: ");
    Serial.print(objectIDs[i]);
    Serial.print(", Centroid: [");
    Serial.print(centroids[i].first);
    Serial.print(", ");
    Serial.print(centroids[i].second);
    Serial.println("]");
    objectIDs_and_centroids_array[i][0] = objectIDs[i]; // put the object ID in array
    objectIDs_and_centroids_array[i][1] = centroids[i].first; // put the centroid X coordinate in array
    objectIDs_and_centroids_array[i][2] = centroids[i].second; // put the centroid Y coordinate in array
  }

  // Clear the vector
  rects_init.clear();

  if (bb_found) {
    end_current_bounding_boxes_JSONData();
    append_tracked_objects_to_JSOData(objectIDs_and_centroids_array, number_of_tracked_objects);
  }
  if (!bb_found) create_JSONData_for_noBBs();

  // serve the live stream and object tracking on the HTML page
  serve_video_and_found_objects();

  clear_current_found_objects_JSONData();

  // once you have finished with this image you tell it that this reserved memory can be released with the command: esp_camera_fb_return(fb);
  esp_camera_fb_return(fb_curr);

  free(snapshot_buf);
}


/* Edge Impulse's camera functions ----------------------------------------- */

/**
   @brief   Setup image sensor & start streaming
   @retval  false if initialisation failed
*/
bool ei_camera_init(void) {

  if (is_initialised) return true;

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  //initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, 0); // lower the saturation
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#elif defined(CAMERA_MODEL_ESP_EYE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_awb_gain(s, 1);
#endif

  is_initialised = true;
  return true;
}

/**
   @brief      Stop streaming of sensor data
*/
void ei_camera_deinit(void) {

  //deinitialize the camera
  esp_err_t err = esp_camera_deinit();

  if (err != ESP_OK)
  {
    ei_printf("Camera deinit failed\n");
    return;
  }

  is_initialised = false;
  return;
}


/**
   @brief      Capture, rescale and crop image

   @param[in]  img_width     width of output image
   @param[in]  img_height    height of output image
   @param[in]  out_buf       pointer to store output image, NULL may be used
                             if ei_camera_frame_buffer is to be used for capture and resize/cropping.

   @retval     false if not initialised, image captured, rescaled or cropped failed

*/
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  bool do_resize = false;

  if (!is_initialised) {
    ei_printf("ERR: Camera is not initialized\r\n");
    return false;
  }

  // esp_camera_fb_get tells it to capture a image and store it in memory, the pointer "fb" then points to where this data is
  fb_curr = esp_camera_fb_get();

  if (!fb_curr) {
    ei_printf("Camera capture failed\n");
    return false;
  }

  bool converted = fmt2rgb888(fb_curr->buf, fb_curr->len, PIXFORMAT_JPEG, snapshot_buf);

  //esp_camera_fb_return(fb_curr);

  if (!converted) {
    ei_printf("Conversion failed\n");
    return false;
  }

  if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
      || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
    do_resize = true;
  }

  if (do_resize) {
    ei::image::processing::crop_and_interpolate_rgb888(
      out_buf,
      EI_CAMERA_RAW_FRAME_BUFFER_COLS,
      EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
      out_buf,
      img_width,
      img_height);
  }


  return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
  // we already have a RGB888 buffer, so recalculate offset into pixel index
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;

  while (pixels_left != 0) {
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix + 2];

    // go to the next pixel
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  // and done!
  return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif
