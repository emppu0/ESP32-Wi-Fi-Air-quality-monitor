#include <Arduino.h>
#include <WiFi.h>
//#include <esp_wifi.h>
#include <HTTPClient.h>
#include "SensirionI2CScd4x.h"
//#include "SparkFun_SGP30_Arduino_Library.h"

#include <Wire.h>
#include "password.h"
//#include "password_example.h" //uncomment this and comment the above line
#include <OneWire.h>
#include <DallasTemperature.h>

#define READINGS 10                // how many sensor readings
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60           // Sleep time

uint8_t index1 = 0;
float temp;
float temp2;
float temp_readings_arr[READINGS];
float temp_avg = 0;
float temp_total;

float humi;
float humi_readings_arr[READINGS];
float humi_avg = 0;
float humi_total;

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
#define SENSOR_RESOLUTION 12 // sensor resolution

// SGP30 SGP30_1; // create an object of the SGP30 class
SensirionI2CScd4x scd4x;

void error_handler(void) // implement -> inform server about issue
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
  uint8_t concounter = 0;
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && concounter < 8)
  {
    delay(500);
    Serial.println("...");
    concounter++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect(true, true);
    delay(1000);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.println("...");
    }
    // Serial.println("Restarting, cannot connect to WiFI");
    // ESP.restart();
  }

  Serial.print("Connected to:");
  Serial.println(WiFi.localIP());
}

void setup()
{
  pinMode(25, OUTPUT); // status led blue
  pinMode(26, OUTPUT); // status led red
  pinMode(27, OUTPUT); // status led green

  
  digitalWrite(27, HIGH);
  delay(1000);
  digitalWrite(27, LOW);

  Serial.begin(9600); // serial for debug
  Serial.println("Im here");

  Wire.begin();
  Network();

  if (sensor_begin())
  {
    error_handler();
    // implement reset for SCD4x
  }

  error = scd4x.readMeasurement(co2, temp2, humi);

  if (error || co2 == 0) // read again if reading is invalid or error is returned
  {
    delay(31000);
    error = scd4x.readMeasurement(co2, temp, humi);
    if (error)
    {
      error_handler();
    }
  }

  sensors.begin();
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);

  /*
  if (SGP30_1.begin())
  {
    SGP30_1.initAirQuality();
    tvoc_total = SGP30_1.TVOC * READINGS;
    SGP30_ok = true;
  }
  */

  temp_total = temp * READINGS;
  humi_total = humi * READINGS;
  co2_total = co2 * READINGS;

  for (int i = 0; i < READINGS; i++)
  { // initialize arrays with current sensor values
    temp_readings_arr[i] = temp;
    humi_readings_arr[i] = humi;
    co2_readings_arr[i] = co2;
    // tvoc_readings_arr[i] = 0;
  }

  digitalWrite(27, HIGH);
  delay(500);
  digitalWrite(27, LOW);
  delay(500);
  digitalWrite(27, HIGH);
  delay(500);
  digitalWrite(27, LOW);
}

void loop()
{
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.flush();
  delay(100);
  esp_light_sleep_start();

  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  error = scd4x.readMeasurement(co2, temp2, humi);

  if (error) // read again if reading is invalid or error is returned
  {
    delay(31000);
    error = scd4x.readMeasurement(co2, temp2, humi);
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

  temp_total = temp_total - temp_readings_arr[index1];
  temp_readings_arr[index1] = temp;
  temp_total = temp_total + temp_readings_arr[index1];

  humi_total = humi_total - humi_readings_arr[index1];
  humi_readings_arr[index1] = humi;
  humi_total = humi_total + humi_readings_arr[index1];

  co2_total = co2_total - co2_readings_arr[index1];
  co2_readings_arr[index1] = co2;
  co2_total = co2_total + co2_readings_arr[index1];

  index1++;

  // calculate avg:
  temp_avg = temp_total / READINGS;
  humi_avg = humi_total / READINGS;
  co2_avg = co2_total / READINGS;

  /*
  Serial.println("........................");
  Serial.print("Curr temp:");
  Serial.println(temp);
  Serial.print("Avg temp:");
  Serial.println(temp_avg);
  Serial.println("........................");
  Serial.print("Curr humi:");
  Serial.println(humi);
  Serial.print("Avg humi:");
  Serial.println(humi_avg);
  Serial.println("........................");
  Serial.print("Curr co2:");
  Serial.println(co2);
  Serial.print("Avg co2:");
  Serial.println(co2_avg);
  Serial.println("........................");
  */

  if (index1 >= READINGS)
  {
    Network(); // connect to wifi after sleep
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {

      WiFiClient client;
      HTTPClient http;
      digitalWrite(25, HIGH);

      // Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Prepare HTTP POST data

      // String httpRequestData = "api_key=" + apiKeyValue + "temperature=" + String(temp_avg) + "&humidity=" + String(humi_avg) + "&co2=" + String(co2_avg) + "&tvoc=" + String(tvoc_avg) + "";
      String httpRequestData = "api_key=" + apiKeyValue + "&temperature=" + String(temp_avg) + "&humidity=" + String(humi_avg) + "&co2=" + String(co2_avg) + "&tvoc=" + String(co2) + "";

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
      digitalWrite(25, LOW);
    }
    index1 = 0;
  }
}