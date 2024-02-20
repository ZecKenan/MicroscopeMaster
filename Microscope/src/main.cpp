#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <HTTPClient.h>

#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

// MicroSD Libraries
#include "FS.h"
#include "SD_MMC.h"

// Pin definitions for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//####################################################### SERVER
const char* macServerUrl = "http://192.168.4.2:8000/upload"; // Replace with your Mac's IP and port

//######## FOLDER NAME #####
String folder1 = "/BF";
String folder2 = "/DF";

// Counter for picture number
unsigned int pictureCount = 0;

// Delay time in milliseconds (300kms = 5min)
unsigned long delayTime = 150000UL;
unsigned long previousMillis = 0;

String dirname = "/pictures";

bool isQuality = true; //Checking UXGA 
String str;
String serverSettings;
camera_config_t config;

const char* ssid = "IncuScope";
const char* password = NULL;
void startCameraServer();
void setupLedFlash(int pin);
void printImageResolution(camera_fb_t *fb);

void configESPCamera() {
  // Configure Camera parameters

  // Object to store the camera configuration parameters
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // Choices are YUV422, GRAYSCALE, RGB565, JPEG

  // Select lower framesize if the camera doesn't support PSRAM
  if (psramFound()) {
    isQuality = true;
    str = "   --Res: UXGA";
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10; // 10-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    isQuality = false;
    str = "   --Res: Annan typ";
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed with error 0x%x", err);
    return;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize the microSD card
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }

  // Disable brownout problems
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Configure ESP camera
  configESPCamera();

  //############## WiFi - Access Point ###############
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("Configuring access point...");

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP-IP address to copy: ");
  Serial.println(myIP);

  startCameraServer();

  // Create a new directory for storing images
  String dirname = "/pictures";
  if (!SD_MMC.exists(dirname)) {
    SD_MMC.mkdir(dirname);
  }

  //##############################
  // Create a new directory for storing images in folder1
  if (!SD_MMC.exists(dirname + folder1)) {
    SD_MMC.mkdir(dirname + folder1);
  }

  // Create a new directory for storing images in folder2
  if (!SD_MMC.exists(dirname + folder2)) {
    SD_MMC.mkdir(dirname + folder2);
  }
  //##############################
}

//########################################################### SERVER
void uploadImageToMac() {
  // Capture an image
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  //Debugging: Print the URL before making the POST request
  Serial.printf("Making HTTP POST request to: %s\n", macServerUrl);

  HTTPClient http;
  http.begin(macServerUrl);
  http.addHeader("Content-Type", "image/jpeg");

  // Send the image as the request body
  int httpResponseCode = http.POST(fb->buf, fb->len);

  // Debugging: Print the HTTP response code
  Serial.printf("HTTP POST Response Code: %d\n", httpResponseCode);

  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.println("Image uploaded to Mac successfully");
  } else {
    Serial.printf("HTTP POST failed with error code %d\n", httpResponseCode);
  }

  http.end();

  // Free the camera frame buffer
  esp_camera_fb_return(fb);
}
//########################################################### SERVER

void printImageResolution(camera_fb_t *fb) {
  // Calculate resolution based on format
  size_t imageSize = 0;
  if (fb->format == PIXFORMAT_JPEG) {
    imageSize = fb->len;
  } else if (fb->format == PIXFORMAT_GRAYSCALE) {
    imageSize = fb->width * fb->height;
  } else if (fb->format == PIXFORMAT_RGB888) {
    imageSize = fb->width * fb->height * 3;
  }

  // Print resolution
  Serial.printf("Resolution: %dx%d, Format: %s, Image Size: %u bytes\n",
                fb->width, fb->height, (fb->format == PIXFORMAT_JPEG) ? "JPEG" : ((fb->format == PIXFORMAT_GRAYSCALE) ? "Grayscale" : "RGB888"), imageSize);

}

void loop() {
  //#########################
  static bool isFolder1 = true;
  //##########################

  // Get the current time
  unsigned long currentMillis = millis();

  // Check if the delay time has passed
  if (currentMillis - previousMillis >= delayTime) {
    previousMillis = currentMillis;

    // Capture an image
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    uploadImageToMac();

    // Save image to microSD card
    /*
    String filename = "/pictures/pic" + String(pictureCount) + ".jpg";
    File file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
    } else {
      file.write(fb->buf, fb->len);
      Serial.println("Image saved: " + filename);
    }
    file.close();
    esp_camera_fb_return(fb);
    */
    //####################################
    String folder = isFolder1 ? folder1 : folder2;
    String filename = dirname + folder + "/pic" + String(pictureCount) + ".jpg";
    File file = SD_MMC.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
    }   else {
      file.write(fb->buf, fb->len);
      Serial.println("Image saved: " + filename);
    }
    file.close();

    // Save image to microSD card
    if (pictureCount % 2 == 0) {
      filename = dirname + "/folder1/pic" + String(pictureCount) + ".jpg";
      Serial.print("Image saved in BF-folder: ");
    } else {
      filename = dirname + "/folder2/pic" + String(pictureCount) + ".jpg";
      Serial.print("Image saved in DF-folder: ");
    }

    //####################################

    // Toggle the folder
    isFolder1 = !isFolder1;
    // Increment picture count
    pictureCount++;

    // Printing the Resolution
    printImageResolution(fb);
    esp_camera_fb_return(fb);
  }
}

// LAST UPDATE 28/7 2023
