#include <Arduino.h>
#include "icons.h" // Custom header containing icons in xbitmap form
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <Adafruit_BME280.h>
#include <RevEng_PAJ7620.h>

//=============PINs=================//
#define I2C_SDA   21
#define I2C_SCL   22
#define PHOTOPIN  33 // Photoresistor pin

#define SAMPLING_TIME 5000

// Coordinates for top-left corner of the icon
#define X_LOC 0
#define Y_LOC 10

// Network configuration
const char* ssid     = "ASUS";
const char* password = "Jeremias777";

// NTP Server Details
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

// Setup for the display
TFT_eSPI tft = TFT_eSPI();

// BME280 sensor
Adafruit_BME280 bme;

// Gesture sensor
RevEng_PAJ7620 gesture = RevEng_PAJ7620();

uint8_t state; //Global variable
unsigned long timer;

void displayIndicator(uint8_t displayNumber);
void displayTemperature();
void displayHumidity();
void displayPressure();
void displayLight();

void displaySensor();
void incrementState();
void decrementState();

void setup()
{
  tft.begin();               // Initialise the display
  tft.fillScreen(TFT_BLACK); // Black screen fill
  
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
  
  // Initialize BME280
  // return value of 0 == successv
  if (!bme.begin(0x77, &Wire)) {
    Serial.println("BME280 I2C error - halting");
    while (1);
  }
  else{
    Serial.println("BME280 init: OK");
  }

  // Initialize PAJ7620
  if( !gesture.begin(&Wire) )           
  {
    Serial.print("PAJ7620 I2C error - halting");
    while(true) { }
  }

  Serial.println("PAJ7620 init: OK");

  tft.setCursor(20, 40, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.println("Sensors: OK.");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi: OK.");

  tft.setCursor(20, 80, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.println("Wifi: OK.");

  delay(2000);  
  
  tft.setCursor(40, 140, 1);
  tft.setTextColor(TFT_PINK);
  tft.setTextSize(4);
  tft.println("Welcome");

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  delay(3000);
  
  //Configurations
  state = 0;
  timer = millis();
  
}

void loop()
{
  Gesture data;
  data = gesture.readGesture();
  
  bool state_change = false;

  switch (data)
    {
    case GES_RIGHT:
      incrementState();
      state_change=true;
      timer=millis();
      break;

    case GES_LEFT:
      decrementState();
      state_change=true;
      timer=millis();
      break;
    
    default:
      unsigned long now = millis();
      bool done=false;
      //Wait longer if the state is at home
      if (state == 0) {
        done = (now>timer) &&((now - timer) > (3 * SAMPLING_TIME));
      }
      else {
        done = (now>timer) &&((now - timer) > (SAMPLING_TIME));
      }

      if(done)
      {
        incrementState();
        state_change=true;
        timer = millis();
      }
      break;
    }
    
    if(state_change){
      displaySensor();
      state_change = false;
    }
}

// Create display marker for each screen
void displayIndicator(int displayNumber) {
  int xCoordinates[5] = {80, 100, 120, 140, 160};
  for (int i =0; i<5; i++) {
    if (i == displayNumber) {
      tft.fillCircle(xCoordinates[i], 215, 2, TFT_WHITE);
    }
    else {
      tft.drawCircle(xCoordinates[i], 215, 2, TFT_WHITE);
    }
  }
}

void displayLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  //Get full weekday name
  char weekDay[10];
  strftime(weekDay, sizeof(weekDay), "%a", &timeinfo);
  //Get day of month
  char dayMonth[4];
  strftime(dayMonth, sizeof(dayMonth), "%d", &timeinfo);
  //Get abbreviated month name
  char monthName[5];
  strftime(monthName, sizeof(monthName), "%b", &timeinfo);
  //Get year
  char year[6];
  strftime(year, sizeof(year), "%Y", &timeinfo);

  //GET TIME
  //Get hour (12 hour format)
  /*char hour[4];
  strftime(hour, sizeof(hour), "%I", &timeinfo);*/
  
  //Get hour (24 hour format)
  char hour[4];
  strftime(hour, sizeof(hour), "%H", &timeinfo);
  //Get minute
  char minute[4];
  strftime(minute, sizeof(minute), "%M", &timeinfo);
  
  tft.fillScreen(TFT_BLACK);
  delay(200);
  //              x          y  xbm          xbm width  xbm height  color
  tft.drawXBitmap(X_LOC+50, 0, home, iconWidth, iconHeight, TFT_PINK);
  //Display Date and Time on OLED display
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(40, iconHeight + 10, 1);
  tft.print(hour);
  tft.print(":");
  tft.print(minute);

  tft.setTextSize(2);
  tft.setCursor(20, iconHeight + 40, 1);
  tft.print(weekDay);
  tft.print(", ");
  tft.print(dayMonth);
  tft.print(" ");
  tft.print(monthName);
  tft.print(" ");
  tft.print(year);

  displayIndicator(0);
}

void displayTemperature(){
  tft.fillScreen(TFT_BLACK);
  delay(200);

  //              x          y  xbm          xbm width  xbm height  color
  tft.drawXBitmap(X_LOC, Y_LOC, thermometer, iconWidth, iconHeight, TFT_RED);
  
  //Get the sensor values
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure()/100.0f;

  //Main readings
  tft.setCursor(iconWidth-20, iconHeight/2, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.print(temperature, 1);
  tft.write(167);
  tft.println("C");
  
  //Other readings
  tft.setCursor(20, iconHeight + Y_LOC, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("Humidity: ");
  tft.print(humidity, 1);
  tft.println("%");

  tft.setCursor(20, iconHeight + Y_LOC + 35, 1);
  tft.print("Pressure: ");
  tft.print(pressure, 1);
  tft.setTextSize(1);
  tft.println("hPa");

  //Draw indicator circles
  displayIndicator(1);
};

void displayHumidity(){
  tft.fillScreen(TFT_BLACK);  
  delay(200);

  //              x          y  xbm       xbm width  xbm height  color
  tft.drawXBitmap(X_LOC, Y_LOC, humidity, iconWidth, iconHeight, TFT_CYAN);

  //Get the sensor values
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure()/100.0f;
  
  //Main readings
  tft.setCursor(iconWidth-10, iconHeight/2, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.print(humidity);
  tft.println("%");
  
  //Other readings
  tft.setCursor(10, iconHeight + Y_LOC, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("Temperature: ");
  tft.print(temperature, 1);
  tft.write(167);
  tft.println("C");
  

  tft.setCursor(10, iconHeight + Y_LOC + 35, 1);
  tft.print("Pressure: ");
  tft.print(pressure, 1);
  tft.setTextSize(1);
  tft.println("hPa");

  displayIndicator(2);
};

void displayPressure(){
  tft.fillScreen(TFT_BLACK);  
  delay(200);

  //              x          y  xbm       xbm width  xbm height  color
  tft.drawXBitmap(X_LOC, 0, pressure, iconWidth, iconHeight, TFT_BROWN);
  
  //Get the sensor values
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure()/100.0f;

  //Main readings
  tft.setCursor(iconWidth-30, iconHeight/2, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.print(pressure, 1);
  tft.setTextSize(1);
  tft.print("hPa");

  //Other readings
  tft.setCursor(10, iconHeight + Y_LOC, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("Temperature: ");
  tft.print(temperature, 1);
  tft.write(167);
  tft.println("C");
  

  tft.setCursor(10, iconHeight + Y_LOC + 35, 1);
  tft.print("Humidity: ");
  tft.print(humidity, 1);
  tft.println("%");

  displayIndicator(3);
  
};

void displayLight(){
  tft.fillScreen(TFT_BLACK);  
  delay(200);

  //              x          y     xbm    xbm width  xbm height  color
  tft.drawXBitmap(X_LOC+10, Y_LOC, sunny, iconWidth, iconHeight, TFT_YELLOW);
  
  //Get the sensor values
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure()/100.0f;
  int   light = map(analogRead(PHOTOPIN), 0, 4095, 0, 100);

  //Main readings
  tft.setCursor(iconWidth+30, iconHeight/2, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.print(light); tft.println("%");

  //Other readings
  tft.setCursor(10, iconHeight + Y_LOC, 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.print("Temperature: ");
  tft.print(temperature, 1);
  tft.write(167);
  tft.println("C");
  

  tft.setCursor(10, iconHeight + Y_LOC + 35, 1);
  tft.print("Humidity: ");
  tft.print(humidity, 1);
  tft.println("%");
  
  displayIndicator(4);
};

void displaySensor(){
  switch (state)
  {
    case 0:
      displayLocalTime();
      break;
  
    case 1:
      displayTemperature();
      break;

    case 2:
      displayHumidity();
      break;

    case 3:
      displayPressure();
      break;

    case 4:
      displayLight();
      break;

    default:
      break;
  }
}

void incrementState(){
  if(state<4)
    state++;
  else
    state=0;
}

void decrementState(){
  if(state>0)
    state--;
  else
    state=4;
}