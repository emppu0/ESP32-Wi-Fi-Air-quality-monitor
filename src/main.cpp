#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "password.h"
//#include "password_example.h" //uncomment this and comment the above line
#include "SparkFun_SGP30_Arduino_Library.h"
#include "SparkFun_SCD4x_Arduino_Library.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

unsigned long httppost_update_interval = 600000; // should be 600000
unsigned long sensors_update_interval = 60000;   // should be 60000
unsigned long time_now = 0;
unsigned long time_now2 = 0;

#define READINGS 10               // how many sensor readings
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 3           /* Time ESP32 will go to sleep (in seconds) */

uint16_t counter = 0;
uint16_t index1 = 0; // korvaa kaikki yhdellä muuttujalla
float temp;
float temp_readings_arr[READINGS]; // the readings from temperature sensor
float temp_avg = 0;                // temp avg
float temp_total;                  // running total

float humi;
float humi_readings_arr[READINGS]; // the readings from temperature sensor
float humi_avg = 0;                // temp avg
float humi_total;                  // running total

uint16_t co2 = 0;                    // total volatile organic compounds
uint16_t co2_readings_arr[READINGS]; // the readings from gas sensor
uint16_t co2_avg = 0;                // tvoc avg
uint16_t co2_total;                  // running total

uint16_t tvoc = 0;                    // total volatile organic compounds
uint16_t tvoc_readings_arr[READINGS]; // the readings from gas sensor
uint16_t tvoc_read_index = 0;         // index of the current reading
uint16_t tvoc_avg = 0;                // tvoc avg
uint16_t tvoc_total;                  // running total
bool SGP30_ok = false;

const int oneWireBus = 18; // input pin for ds18b20, tempsens1
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#define SENSOR_RESOLUTION 12 // sensor resolution

SGP30 SGP30_1;  // create an object of the SGP30 class
SCD4x SCD40; // create an object of the SCD4x class

void setup()
{
  pinMode(25, OUTPUT); // status led blue
  pinMode(26, OUTPUT); // status led red
  pinMode(27, OUTPUT); // status led green
  Wire.begin();

  Serial.begin(9600); // serial for debug

  digitalWrite(27, HIGH);
  delay(4000);
  digitalWrite(27, LOW);

  // sensors.requestTemperatures();
  // temp = sensors.getTempCByIndex(0);
  if (SCD40.begin() == false)
  {
    while (1)
      ;
  }
  delay(5000);

  SCD40.readMeasurement();
  {
    temp = SCD40.getTemperature();
    temp_total = temp * READINGS;
    humi = SCD40.getHumidity();
    humi_total = humi * READINGS;
    co2 = SCD40.getCO2();
    co2_total = co2 * READINGS;
    co2 = SCD40.getCO2();
    co2_total = co2 * READINGS;

    for (int i = 0; i < READINGS; i++)
    { // initialize arrays with current sensor values
      temp_readings_arr[i] = temp;
      humi_readings_arr[i] = humi;
      co2_readings_arr[i] = co2;
    }
  }

  if (SGP30_1.begin())
  {
    SGP30_1.initAirQuality();
    tvoc = SGP30_1.TVOC;

    tvoc_total = tvoc * READINGS;

    for (int i = 0; i < READINGS; i++)
    {
      tvoc_readings_arr[i] = tvoc;
    }
    SGP30_ok = true;
  }
}

void loop()
{

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("I go to sleep now");
  Serial.flush();
  esp_light_sleep_start();
  Serial.println("You woke my up");

  // sensors.requestTemperatures();
  // temp = sensors.getTempCByIndex(0);
  SCD40.readMeasurement();
  {

    temp = SCD40.getTemperature();
    humi = SCD40.getHumidity();
    co2 = SCD40.getCO2();

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

    // calculate temperature avg:
    temp_avg = temp_total / READINGS;
    humi_avg = humi_total / READINGS;
    co2_avg = co2_total / READINGS;
  }

  if (SGP30_ok)
  {
    SGP30_1.measureAirQuality();
    tvoc = SGP30_1.TVOC;
    tvoc_total = tvoc_total - tvoc_readings_arr[tvoc_read_index];
    tvoc_readings_arr[tvoc_read_index] = tvoc;
    tvoc_total = tvoc_total + tvoc_readings_arr[tvoc_read_index];
    tvoc_read_index = tvoc_read_index + 1;

    if (tvoc_read_index >= READINGS)
    {
      tvoc_read_index = 0;
    }

    // calculate humidity avg:
    tvoc_avg = tvoc_total / READINGS;
  }

  // debug
  Serial.print("Current Temperature: ");
  Serial.println(temp);
  Serial.print("AVG Temperature: ");
  Serial.println(temp_avg);

  Serial.print("Current Tvoc: ");
  Serial.println(tvoc);
  Serial.print("AVG Tvoc: ");
  Serial.println(tvoc_avg);

  Serial.print("Current Humidity: ");
  Serial.println(humi);
  Serial.print("AVG humidity: ");
  Serial.println(humi_avg);

  Serial.print("Current co2: ");
  Serial.println(co2);
  Serial.print("AVG co2: ");
  Serial.println(co2_avg);
  delay(1000);

  if (index1 >= READINGS) //(unsigned long)(millis() - time_now2) > httppost_update_interval
  {
    WiFi.begin(ssid, password);
    delay(5000);
    if (WL_CONNECTED)
    {

      // Check WiFi connection status

      WiFiClient client;
      HTTPClient http;
      digitalWrite(25, HIGH);

      // Domain name with URL path or IP address with path
      http.begin(client, serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Prepare HTTP POST data

      String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + String(temp_avg) + "&value2=" + String(humi_avg) + "&value3=" + String(co2_avg) + "&value4=" + String(tvoc_avg) + "";

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
      // WiFi.disconnect();

      time_now2 = millis();
      digitalWrite(25, LOW);
      index1 = 0;
    }
  }

  delay(10);
}