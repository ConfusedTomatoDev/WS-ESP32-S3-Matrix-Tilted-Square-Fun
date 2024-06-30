// Developer: ConfusedTomatoDev
// Created 06/30/2024
//
// Instructions:
// Load file in to Arduino IDE 2.3.2 or one of your choice.
// For full details setting up the matrix, follow WaveShare guide:
// https://www.waveshare.com/wiki/ESP32-S3-Matrix
// Note the board name to use is "ESP32S3 Dev Module".
// 
// Using the Arduino IDE, Verify the code, and Upload to the "ESP32S3 Dev Module".  
// The matrix will show a square that expands and contracts cahnging it's color
// Red, Green, Blue, as it expands and contracts across the matrix.
// Holding the matrix flat, the square will expand and contract slowely.
// Tilting the matrix in any direction will result in the expaning and contracting square to accellerate.
// Have fun... I'm sure improvements can be made, my C+ is rough...


#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "SensorQMI8658.hpp"

// Define pins and constants
#define I2C_SDA 11
#define I2C_SCL 12
#define LED_PIN 14
#define NUMPIXELS 64 // Number of LEDs in the matrix (e.g., 8x8 matrix)
#define MATRIX_WIDTH 8 // Width of the LED matrix
#define MATRIX_HEIGHT 8 // Height of the LED matrix
#define BRIGHTNESS 2 // Define the brightness (0-255)

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Colors array for animation
uint32_t colors[] = {pixels.Color(0, 255, 0), pixels.Color(0, 0, 255), pixels.Color(255, 0, 0)};
int colorIndex = 0; // Index for cycling through colors

// Configurable speed range variables
int slowSpeed = 200; // Slowest speed in milliseconds
int fastSpeed = 5;   // Fastest speed in milliseconds

// Configurable angle range variables
int minAngle = 0;    // Minimum tilt angle
int maxAngle = 90;   // Maximum tilt angle

// IMU sensor objects
SensorQMI8658 QMI;
IMUdata Accel;
IMUdata Gyro;

// Function to initialize the QMI8658 sensor
void QMI8658_Init() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("I2C initialized.");

  if (!QMI.begin(Wire, 0x6b, I2C_SDA, I2C_SCL)) {
    Serial.println("Failed to find QMI8658 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }
  Serial.printf("I2C Device ID: %x\n", QMI.getChipID());    // Get chip id

  QMI.configAccelerometer(
      SensorQMI8658::ACC_RANGE_4G,      // ACC_RANGE_2G / ACC_RANGE_4G / ACC_RANGE_8G / ACC_RANGE_16G
      SensorQMI8658::ACC_ODR_1000Hz,    // ACC_ODR_1000H / ACC_ODR_500Hz / ACC_ODR_250Hz / ACC_ODR_125Hz / ACC_ODR_62_5Hz / ACC_ODR_31_25Hz / ACC_ODR_LOWPOWER_128Hz / ACC_ODR_LOWPOWER_21Hz / ACC_ODR_LOWPOWER_11Hz / ACC_ODR_LOWPOWER_3Hz    
      SensorQMI8658::LPF_MODE_0,        //LPF_MODE_0 (2.66% of ODR) / LPF_MODE_1 (3.63% of ODR) / LPF_MODE_2 (5.39% of ODR) / LPF_MODE_3 (13.37% of ODR)
      true);                            // selfTest enable

  QMI.configGyroscope(
      SensorQMI8658::GYR_RANGE_64DPS,   // GYR_RANGE_16DPS / GYR_RANGE_32DPS / GYR_RANGE_64DPS / GYR_RANGE_128DPS / GYR_RANGE_256DPS / GYR_RANGE_512DPS / GYR_RANGE_1024DPS
      SensorQMI8658::GYR_ODR_896_8Hz,   // GYR_ODR_7174_4Hz / GYR_ODR_3587_2Hz / GYR_ODR_1793_6Hz / GYR_ODR_896_8Hz / GYR_ODR_448_4Hz / GYR_ODR_224_2Hz / GYR_ODR_112_1Hz / GYR_ODR_56_05Hz / GYR_ODR_28_025H
      SensorQMI8658::LPF_MODE_3,        // LPF_MODE_0 (2.66% of ODR) / LPF_MODE_1 (3.63% of ODR) / LPF_MODE_2 (5.39% of ODR) / LPF_MODE_3 (13.37% of ODR)
      true);                            // selfTest enable

  QMI.enableGyroscope();
  QMI.enableAccelerometer();

  QMI.dumpCtrlRegister();               // printf register configuration information
  Serial.println("Read data now...");
}

// Function to get the tilt angle from the QMI8658 sensor
int getTiltAngle() {
  printf("Getting tilt angle...\n");

  if (QMI.getDataReady()) {
    printf("I2C Device ID: %x\n", QMI.getChipID());
    printf("Data ready\n");

    if (QMI.getAccelerometer(Accel.x, Accel.y, Accel.z)) {
      float ax = Accel.x;
      float ay = Accel.y;
      float az = Accel.z;

      float pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180 / PI;
      float roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;

      // Calculate the combined tilt angle for a 360-degree awareness
      float combinedTilt = sqrt(pitch * pitch + roll * roll);

      // Print accelerometer data
      printf("ACCEL: %f, %f, %f\n", ax, ay, az);

      // Print tilt angles
      printf("Pitch: %f, Roll: %f, Combined Tilt: %f\n", pitch, roll, combinedTilt);

      // Print temperature data
      float tempC = QMI.getTemperature_C();
      float tempF = tempC * 9.0 / 5.0 + 32.0;
      printf("Timestamp: %lu, Temp: %.2f °C / %.2f °F\n\r\n\r", QMI.getTimestamp(), tempC, tempF);

      return combinedTilt; // Return the combined tilt angle for animation speed
    } else {
      printf("Failed to read accelerometer data\n");
    }
  } else {
    printf("Data not ready\n");
  }

  return 0;
}

// Function to draw a square on the LED matrix
void drawSquare(int offset, uint32_t color) {
  pixels.clear(); // Clear previous drawing

  // Top and bottom rows
  for (int x = offset; x < MATRIX_WIDTH - offset; x++) {
    pixels.setPixelColor(x + offset * MATRIX_WIDTH, color); // Top row
    pixels.setPixelColor(x + (MATRIX_HEIGHT - 1 - offset) * MATRIX_WIDTH, color); // Bottom row
  }

  // Left and right columns
  for (int y = offset; y < MATRIX_HEIGHT - offset; y++) {
    pixels.setPixelColor(offset + y * MATRIX_WIDTH, color); // Left column
    pixels.setPixelColor((MATRIX_WIDTH - 1 - offset) + y * MATRIX_WIDTH, color); // Right column
  }

  pixels.show(); // Update the LEDs
}

void setup() {
  Serial.begin(115200);
  QMI8658_Init(); // Initialize the QMI8658 sensor
  pixels.begin(); // Initialize the NeoPixel library
  pixels.setBrightness(BRIGHTNESS); // Set the brightness level
  pixels.show(); // Turn off all LEDs initially
}

void loop() {
  int animationDelay = map(getTiltAngle(), minAngle, maxAngle, slowSpeed, fastSpeed); // Use configurable speed ranges and angles

  // Shrink the pixel square
  // printf("Animation Shrink Delay: %d\n", animationDelay);
  for (int i = 0; i <= MATRIX_WIDTH / 2; i++) {
    drawSquare(i, colors[colorIndex]);
    colorIndex = (colorIndex + 1) % 3; // Cycle through colors
    delay(animationDelay);
  }

  // Expand the pixel square
  // printf("Animation Expand Delay: %d\n", animationDelay);
  for (int i = MATRIX_WIDTH / 2; i >= 0; i--) {
    drawSquare(i, colors[colorIndex]);
    colorIndex = (colorIndex + 1) % 3; // Cycle through colors
    delay(animationDelay);
  }
}
