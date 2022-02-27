#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "password.h"
//#include "password_example.h" //uncomment this and comment the above line
#include "SparkFun_SGP30_Arduino_Library.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

unsigned long httppost_update_interval = 600000;
unsigned long sensors_update_interval = 60000;
unsigned long time_now = 0;
unsigned long time_now2 = 0;

#define READINGS 10 // how many sensor readings

float temp;
float temp_readings_arr[READINGS]; // the readings from temperature sensor
uint16_t temp_read_index = 0;      // index of the current reading
float temp_avg = 0;                // temp avg
float temp_total;                  // running total

float tvoc = 0;                    // total volatile organic compounds
float tvoc_readings_arr[READINGS]; // the readings from gas sensor
int tvoc_read_index = 0;           // index of the current reading
float tvoc_avg = 0;                // tvoc avg
float tvoc_total;                  // running total
bool SGP30_ok = false;

const int oneWireBus = 18; // input pin for ds18b20, tempsens1
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#define SENSOR_RESOLUTION 12 // sensor resolution

SGP30 mySensor; // create an object of the SGP30 class

void setup()
{
  pinMode(25, OUTPUT); // status led blue
  pinMode(26, OUTPUT); // status led red
  pinMode(27, OUTPUT); // status led green
  Wire.begin();

  Serial.begin(9600); // serial for debug

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  delay(1000);
  if (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("Wrong ssid or password");
  }

  Serial.print("Connected to WiFi network with IP Address: ");
  digitalWrite(27, HIGH);
  delay(4000);
  digitalWrite(27, LOW);
  Serial.println(WiFi.localIP());

  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  temp_total = temp * READINGS;

  for (int i = 0; i < READINGS; i++)
  { // initialize array with current sensor value
    temp_readings_arr[i] = temp;
  }

  if (mySensor.begin())
  {
    mySensor.initAirQuality();
    tvoc = mySensor.TVOC;

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

  if ((unsigned long)(millis() - time_now) > sensors_update_interval)
  {

    time_now = millis();

    sensors.requestTemperatures();
    temp = sensors.getTempCByIndex(0);
    temp_total = temp_total - temp_readings_arr[temp_read_index];
    temp_readings_arr[temp_read_index] = temp;
    temp_total = temp_total + temp_readings_arr[temp_read_index];
    temp_read_index = temp_read_index + 1;

    if (temp_read_index >= READINGS)
    {
      temp_read_index = 0;
    }

    // calculate temperature avg:
    temp_avg = temp_total / READINGS;

    if (SGP30_ok)
    {
      mySensor.measureAirQuality();
      tvoc = mySensor.TVOC;
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
    delay(1000);
    
  }

  if ((unsigned long)(millis() - time_now2) > httppost_update_interval)
  {
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

      // Prepare HTTP POST request data
      String httpRequestData = "api_key=" + apiKeyValue + "&value1=" + String(temp_avg) + "&value2=" + String(tvoc_avg) + "";

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
    }
    else
    {
      Serial.println("WiFi Disconnected");
    }

    time_now2 = millis();
    digitalWrite(25, LOW);
  }

  delay(10);
}