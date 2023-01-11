#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <HTTPClient.h>
#include "SensirionI2CScd4x.h"
//#include "SparkFun_SGP30_Arduino_Library.h"

#include <Wire.h>

//#include "password_example.h" //uncomment this and comment the above line
#include <OneWire.h>
#include <DallasTemperature.h>

#define BLUE_LED_PIN 25
#define RED_LED_PIN 26
#define GREEN_LED_PIN 27

#define READINGS 10               // how many sensor readings
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60           // Sleep time

uint8_t index1 = 0;
float temperature;
float temperature2;
float temperature_readings_arr[READINGS];
float temperature_avg = 0;
float temperature_total;

float humidity;
float humidity_readings_arr[READINGS];
float humidity_avg = 0;
float humidity_total;

uint16_t co2 = 0;
uint16_t co2_readings_arr[READINGS];
uint16_t co2_avg = 0;
uint16_t co2_total;

/*
uint16_t tvoc_readings_arr[READINGS];
uint16_t tvoc_read_index = 0;
uint16_t tvoc_avg = 0;
uint16_t tvoc_total;
bool SGP30_ok = false;
*/

uint16_t error;

const int oneWireBus = 5; // input pin for ds18b20, tempsens1
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#define SENSOR_RESOLUTION 12 // sensor resolution in bits

// SGP30 SGP30_1; // create an object of the SGP30 class
SensirionI2CScd4x scd4x;

void blinkOnBoardLed(uint8_t pin, uint8_t howManyBlinks, uint16_t blinkDelay)
{
  for (int i = 0; i < howManyBlinks; i++)
  {
    digitalWrite(pin, HIGH);
    delay(blinkDelay);
    digitalWrite(pin, LOW);
  }
}

void error_handler(void) // implement better error handler :)
{
  Serial.flush();
  // ESP.restart();
  esp_deep_sleep_start(); // sleep forever
}

uint16_t sensor_begin(void)
{
  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  uint16_t value;

  scd4x.begin(Wire);

  error = scd4x.stopPeriodicMeasurement();
  if (error)
  {
    return error;
  }

  error = scd4x.performSelfTest(value);
  if (error)
  {
    return error;
  }
  if (value != 0)
  {
    return 1;
  }

  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error)
  {
    return error;
  }
  // print serialNumber

  error = scd4x.startLowPowerPeriodicMeasurement();
  if (error)
  {
    return error;
  }

  delay(31000); // wait for first measurement
  return 0;
}

//
void Network()
{
  uint8_t connectionCounter = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && connectionCounter < 8)
  {
    delay(500);
    Serial.println("...");
    connectionCounter++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    //try connecting again
    WiFi.disconnect(true, true);
    delay(1000);
    WiFi.begin(ssid, password);
    delay(1000);
    if (WiFi.status() != WL_CONNECTED)
    {
      ESP.restart();
    }
  }

  Serial.print("got IP:");
  Serial.println(WiFi.localIP());
}

void setup()
{
  //set pinouts for rgb led
  pinMode(BLUE_LED_PIN, OUTPUT); 
  pinMode(RED_LED_PIN, OUTPUT); 
  pinMode(GREEN_LED_PIN, OUTPUT); 

  Serial.begin(9600); // serial for debug

  Wire.begin();
  Network();

  if (sensor_begin())
  {
    error_handler();
    // implement reset for SCD4x
  }

  error = scd4x.readMeasurement(co2, temperature2, humidity);

  if (error || co2 == 0) // read again if reading is invalid or error is returned
  {
    delay(31000);
    error = scd4x.readMeasurement(co2, temperature2, humidity);
    if (error)
    {
      error_handler();
    }
  }

  sensors.begin();
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0); //read temperature from ds18b20

  /*
  if (SGP30_1.begin())
  {
    SGP30_1.initAirQuality();
    tvoc_total = SGP30_1.TVOC * READINGS;
    SGP30_ok = true;
  }
  */

  temperature_total = temperature * READINGS;
  humidity_total = humidity * READINGS;
  co2_total = co2 * READINGS;

  for (int i = 0; i < READINGS; i++)
  { // initialize arrays with current sensor values
    temperature_readings_arr[i] = temperature;
    humidity_readings_arr[i] = humidity;
    co2_readings_arr[i] = co2;
    // tvoc_readings_arr[i] = 0;
  }
  blinkOnBoardLed(BLUE_LED_PIN, 2, 500);
}

void loop()
{
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.flush();
  delay(50);
  esp_light_sleep_start();

  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  error = scd4x.readMeasurement(co2, temperature2, humidity);

  if (error) // read again if reading is invalid or error is returned
  {
    delay(31000);
    error = scd4x.readMeasurement(co2, temperature2, humidity); //temperature from scd4x not used, too inaccurate
    if (error)
    {
      error_handler();
    }
  }

  /*
  if (SGP30_ok)
  {
    SGP30_1.measureAirQuality();
    tvoc_total = tvoc_total - tvoc_readings_arr[index1]; // substract the last reading
    tvoc_readings_arr[index1] = SGP30_1.TVOC;            // read sensor
    tvoc_total = tvoc_total + tvoc_readings_arr[index1]; // add the reading to the total

    // calculate humidity avg:
    tvoc_avg = tvoc_total / READINGS;
  }
  */
  
  temperature_total = temperature_total - temperature_readings_arr[index1];
  temperature_readings_arr[index1] = temperature;
  temperature_total = temperature_total + temperature_readings_arr[index1];

  humidity_total = humidity_total - humidity_readings_arr[index1];
  humidity_readings_arr[index1] = humidity;
  humidity_total = humidity_total + humidity_readings_arr[index1];

  co2_total = co2_total - co2_readings_arr[index1];
  co2_readings_arr[index1] = co2;
  co2_total = co2_total + co2_readings_arr[index1];

  index1++;

  // calculate avg:
  temperature_avg = temperature_total / READINGS;
  humidity_avg = humidity_total / READINGS;
  co2_avg = co2_total / READINGS;
  

  if (index1 >= READINGS)
  {
    Network(); // connect to wifi after sleep

    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      WiFiClientSecure client;
      HTTPClient http;

      // Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Prepare HTTP POST data
      String httpRequestData = "api_key=" + apiKeyValue + "&temperature=" + String(temperature_avg) + "&humidity=" + String(humidity_avg) + "&co2=" + String(co2_avg) + "&tvoc=" + String(temperature2) + "";

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0)
      {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      }
      else
      {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
      WiFi.disconnect(true, true); //disconnect wifi after posting data
    }
    index1 = 0;
  }
}