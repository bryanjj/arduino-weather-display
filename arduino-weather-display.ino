/*
 * Arduino app which requests the weather from a weather API
 * and displays the result on the Arduino LED matrix
 */
#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"

#include "arduino_secrets.h" 
#include "digits.h"

#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

#define ARROW_WIDTH 3
#define FRAME_WIDTH 12
#define FRAME_HEIGHT DIGIT_HEIGHT
#define HEX_WIDTH 32

#define REFRESH_SECS 60
#define DISPLAY_DEGREE 0 
#define MAX_API_RESPONSE_LEN 1000

#define DEBUG 1

ArduinoLEDMatrix matrix;
uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH];

float lastTemp = 0;
uint8_t lastArrowUp = 0;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;
char* server = "api.open-meteo.com";

String lat = LATITUDE;
String lon = LONGITUDE;
String tz = "America%2FLos_Angeles";
String unit = "fahrenheit";

String request = String("/v1/forecast")
  + String("?latitude=") + lat 
  + String("&longitude=") + lon 
  + String("&timezone=") + tz 
  + String("&temperature_unit=") + unit
  + String("&daily=temperature_2m_max,temperature_2m_min&current_weather=true&forecast_days=1");

/* -------------------------------------------------------------------------- */
void setup() {
/* -------------------------------------------------------------------------- */  
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }  

  matrix.begin();

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
     
    // wait 10 seconds for connection:
    delay(10000);
  }
  
  printWifiStatus();
}

/**
 * make the weather API http request and return the response
 * if there was an error then resposne will be empty
 */
String make_request() {

    // make the actually weather API http request
  WiFiClient client;
  HttpClient http = HttpClient(client, server, 80);

  http.get(request);

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  http.stop();
  
  if (DEBUG) {
    Serial.println(request);
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);
  }

  return response;
}

/**
 * make a request to weather api and return the min, current, and max temps
 */
void get_temps(float& minTemp, float& curTemp, float& maxTemp) {
  
  String response = make_request();

  StaticJsonDocument<MAX_API_RESPONSE_LEN> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    minTemp = 0;
    curTemp = 0;
    maxTemp = 0;
  }

  minTemp = (float)doc["daily"]["temperature_2m_min"][0];
  curTemp = (float)doc["current_weather"]["temperature"];
  maxTemp = (float)doc["daily"]["temperature_2m_max"][0];
}

/**
 * convert a digit into a bitmap frame
 */
void load_digit(uint8_t digit_frame[DIGIT_HEIGHT][DIGIT_WIDTH], int digit) {
  int size = sizeof(uint8_t) * DIGIT_HEIGHT * DIGIT_WIDTH;

  switch (digit) {
    case 0: memcpy(digit_frame, zerot, size); return;
    case 1: memcpy(digit_frame, onet, size); return;
    case 2: memcpy(digit_frame, twot, size); return;
    case 3: memcpy(digit_frame, threet, size); return;
    case 4: memcpy(digit_frame, fourt, size); return;
    case 5: memcpy(digit_frame, fivet, size); return;
    case 6: memcpy(digit_frame, sixt, size); return;
    case 7: memcpy(digit_frame, sevent, size); return;
    case 8: memcpy(digit_frame, eightt, size); return;
    case 9: memcpy(digit_frame, ninet, size); return;
    case 10: memcpy(digit_frame, tent, size); return; // for 100-109 degrees
    case 11: memcpy(digit_frame, elevent, size); return; // for 110-119 degrees
    default: memcpy(digit_frame, xt, size); return; // X for something else
  }
}

/**
 * print out a display frame for debugging
 */
void print_frame(uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH]) {
  for (int i=0; i < FRAME_HEIGHT; i++) {
    for (int j=0; j < FRAME_WIDTH; j++) {
      if (frame[i][j]) {
        Serial.print("X");
      } else {
        Serial.print(" ");
      }
    }
    Serial.println();
  }
}

/**
 * load a display for the temperature arrow
 * for the current temp relative to high and low temps.
 * top of the frame means high temp of the day
 * bottom of the frame means low temp of the day.
 * arrow pointing up means temp is rising, down temp is falling
 */
void load_arrow(uint8_t arrow_frame[FRAME_HEIGHT][ARROW_WIDTH], float minTemp, float curTemp, float maxTemp) {

  // curTemp == minTemp -> ratio = 0
  // curTemp == maxTemp -> ratio = 1
  float ratio = ((curTemp - minTemp)*100/(maxTemp - minTemp))/100;
  uint8_t arrow_row = (FRAME_HEIGHT-1) * (1-ratio);
  if (arrow_row > FRAME_HEIGHT-2) {
    arrow_row = FRAME_HEIGHT-2;
  }

  Serial.print("arrow_row: ");
  Serial.println(arrow_row);

  // clear out arrow frame
  for (int i=0; i<FRAME_HEIGHT; i++) {
    for (int j=0; j<ARROW_WIDTH; j++) {
      arrow_frame[i][j] = 0;
    }
  }

  // temp unchanged, keep the last arrow
  uint8_t unchanged = curTemp == lastTemp;

  // up arrow
  if (curTemp > lastTemp || (unchanged && lastArrowUp)) {
    lastArrowUp = 1;
    Serial.println("UP");
    arrow_frame[arrow_row+1][0] = 1;
    arrow_frame[arrow_row][1] = 1;
    arrow_frame[arrow_row+1][2] = 1;
  }

  // down arrow
  else {
    lastArrowUp = 0;
    Serial.println("DOWN");
    arrow_frame[arrow_row][0] = 1;
    arrow_frame[arrow_row+1][1] = 1;
    arrow_frame[arrow_row][2] = 1;
  }

  lastTemp = curTemp;
}

/**
 * builds the matrix led bitmap frame from the min, current, and max temps
 */
void load_frame(uint8_t frame[FRAME_HEIGHT][FRAME_WIDTH], float minTemp, float curTemp, float maxTemp) {
  uint8_t ten_frame[FRAME_HEIGHT][DIGIT_WIDTH];
  uint8_t one_frame[FRAME_HEIGHT][DIGIT_WIDTH];
  uint8_t arrow_frame[FRAME_HEIGHT][ARROW_WIDTH];

  int roundCurTemp = (int)(curTemp + 0.5);

  load_digit(ten_frame, roundCurTemp / 10);
  load_digit(one_frame, roundCurTemp % 10);
  load_arrow(arrow_frame, minTemp, curTemp, maxTemp);

  // concat all frames
  for(int i=0; i<FRAME_HEIGHT; i++) {
    for(int j=0; j<DIGIT_WIDTH; j++) {
      frame[i][j] = ten_frame[i][j];
    }
  }

  // space
  for(int i=0; i<FRAME_HEIGHT; i++) {
    frame[i][DIGIT_WIDTH] = 0;
  }

    
  for(int i=0; i<FRAME_HEIGHT; i++) {
    for(int j=0; j<DIGIT_WIDTH; j++) {
      frame[i][j+DIGIT_WIDTH+1] = one_frame[i][j];
    }
  }

  for(int i=0; i<FRAME_HEIGHT; i++) {
    for(int j=0; j<ARROW_WIDTH; j++) {
      frame[i][j+2*DIGIT_WIDTH+1] = arrow_frame[i][j];
    }
  }
}


/* -------------------------------------------------------------------------- */
/* main loop: get the temp and render the display every REFRESH_SECS */
void loop() {
/* -------------------------------------------------------------------------- */  
  float minTemp, curTemp, maxTemp;
  get_temps(minTemp, curTemp, maxTemp);
  
  if (DEBUG) {
    Serial.print("min:");
    Serial.println(minTemp);
    Serial.print("cur:");
    Serial.println(curTemp);
    Serial.print("max:");
    Serial.println(maxTemp);
    Serial.print("last:");
    Serial.println(lastTemp);
  }

  if (curTemp > 0) {
    // build the frame from the temp values
    load_frame(frame, minTemp, curTemp, maxTemp);  
  } else {
    // error case: add error squre in lower-left corner
    frame[FRAME_HEIGHT-2][0] = 1;
    frame[FRAME_HEIGHT-2][1] = 1;
    frame[FRAME_HEIGHT-1][0] = 1;
    frame[FRAME_HEIGHT-1][1] = 1;
  }

  if (DISPLAY_DEGREE) {
    // add dot for degreee symbol
    frame[0][2*DIGIT_WIDTH+1] = 1;
  }
  
  if (DEBUG) {
    print_frame(frame);
  }
  
  // render frame on the led matrix
  matrix.renderBitmap(frame, FRAME_WIDTH, FRAME_HEIGHT);

  delay(REFRESH_SECS*1000);
}

/* -------------------------------------------------------------------------- */
void printWifiStatus() {
/* -------------------------------------------------------------------------- */  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
