#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "password.h"
//#include "password_example.h" //uncomment this and comment the above line
#include "SparkFun_SGP30_Arduino_Library.h"
#include "SparkFun_SCD4x_Arduino_Library.h"
#include <Wire.h>
//#include <OneWire.h>
//#include <DallasTemperature.h>

#define READINGS 2               // how many sensor readings
#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 1         // Sleep time 

uint8_t index1 = 0;
float temp;
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

int16_t tvoc = -100;                  
uint16_t tvoc_readings_arr[READINGS]; 
uint16_t tvoc_read_index = 0;         
uint16_t tvoc_avg = 0;                
uint16_t tvoc_total;                  
bool SGP30_ok = false;

//const int oneWireBus = 18; // input pin for ds18b20, tempsens1
//OneWire oneWire(oneWireBus);
//DallasTemperature sensors(&oneWire);
//#define SENSOR_RESOLUTION 12 // sensor resolution

SGP30 SGP30_1; // create an object of the SGP30 class
SCD4x SCD40;   // create an object of the SCD4x class

void Network()
{
    WiFi.disconnect(true, true); //delete AP before connecting again
    delay(1000);
    WiFi.begin(ssid, password);

    //WiFi.onEvent(Wifi_connected,SYSTEM_EVENT_STA_CONNECTED);
    //WiFi.onEvent(Get_IPAddress, SYSTEM_EVENT_STA_GOT_IP);
    //WiFi.onEvent(Wifi_disconnected, SYSTEM_EVENT_STA_DISCONNECTED); 
 
    delay(1000);
    while(WiFi.status() != WL_CONNECTED)
    {
      Serial.print("...");
      delay(1000);
    }
    Serial.print("Connected to:");
    Serial.println(WiFi.localIP());
    delay(1000);
}

void setup()
{
  pinMode(25, OUTPUT); // status led blue
  pinMode(26, OUTPUT); // status led red
  pinMode(27, OUTPUT); // status led green
  Wire.begin();
  Network();

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

  SCD40.stopPeriodicMeasurement(); //stpo periodic measurements
  SCD40.startLowPowerPeriodicMeasurement(); //Enable low power periodic measurements
  delay(310);

  SCD40.readMeasurement();
  {
    if (SGP30_1.begin())
    {
      SGP30_1.initAirQuality();
      tvoc = SGP30_1.TVOC;
      tvoc_total = tvoc * READINGS;
      SGP30_ok = true;
    }

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
      tvoc_readings_arr[i] = tvoc;
    }
  }



}

void loop()
{

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("I go to sleep now");
  Serial.flush();
  delay(100);
  delay(2000);
  esp_light_sleep_start();
  

  Serial.println("You woke me up");

  // sensors.requestTemperatures();
  // temp = sensors.getTempCByIndex(0);
  SCD40.readMeasurement();
  {

    temp = SCD40.getTemperature();
    humi = SCD40.getHumidity();
    co2 = SCD40.getCO2();

    if (SGP30_ok)
    {
      SGP30_1.measureAirQuality();
      tvoc = SGP30_1.TVOC;
      tvoc_total = tvoc_total - tvoc_readings_arr[index1];
      tvoc_readings_arr[index1] = tvoc;
      tvoc_total = tvoc_total + tvoc_readings_arr[index1];

      // calculate humidity avg:
      tvoc_avg = tvoc_total / READINGS;
    }
  }

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


  // debug
  /*
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
  */

  if (index1 >= READINGS)
  {
    Network(); //does not work
    //Serial.print("Connected to:");
    //Serial.println(WiFi.localIP());
    if (WiFi.status() == WL_CONNECTED)
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
      
      digitalWrite(25, LOW);
    }
    index1 = 0;
    
  }
}