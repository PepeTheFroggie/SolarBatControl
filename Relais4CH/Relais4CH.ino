#include <Arduino.h>

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <time.h>                  

#define CH4

// 4ch
// rel1 16, rel2 14, rel3 12, rel4 13
// led1 02, led2 5
// analog 17
// R3 = WR direct
// R4 = WR über 10 ohm

// 2ch
// rel1 5, rel2 4
// led1 02, led2 16
// analog 17

const int analogInPin = A0;

/* Configuration of NTP */
#define MY_NTP_SERVER "ch.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/03"
time_t now;  // this is the epoch
tm tm;       // the structure tm holds time information 

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

const char *ssid = "FSM";
const char *password = "0101010101";
String Shelly_IP = "192.168.178.47";

bool WRstate  = false; // positive = on
bool CHGstate = false; // positive = on

boolean IsActive = true;
boolean ChgOnly = false;
boolean ChgNow = false;
int16_t HIlimit =  200;  // switch on WR above
int16_t LOlimit =  10;   // switch OFF WR below
int16_t CHGlimitLO = -200; // switch ON DC charging below
int16_t CHGlimitHI =  -10; // switch OFF DC charging above
// 13.3 90%, 12.8 17% 
float HIBat    = 27.2; // 30.5; // above this no charge
float ChgLimit = 27.4; // 29.8; // Limit for force charge
float LOBat    = 25.3; // 23.5; // below this no discharge
float ReChgBat = 26.6;
float HalfChgBat = 26.4;
bool lowbatinhibit = false;
bool highbatinhibit = false;

unsigned long actTimems;
#define timeintervalms 5000 // 5 sec 
unsigned long LoopInterval = timeintervalms;
unsigned long nextTimems;
#define shellytimems 15000 // 15 sec 
unsigned long ShellyInterval = shellytimems;
unsigned long nextShellyms;
unsigned int HttpRefreshInterval= 15;


const int LED = LED_BUILTIN;

uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 ()
{
  Serial.println("sntp_update_delay_MS");
  return 12 * 60 * 60 * 1000UL; // 12 hours
}

void wificonnect()
{ 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected to WiFi");
  Serial.println(WiFi.localIP());
}

void setup() 
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  Serial.begin(115200);  
  Serial.println("Start");
  Serial.flush();
  delay(500);
  switchinit();
  
  WiFi.mode(WIFI_STA);

  wificonnect();
  wifi_set_sleep_type(NONE_SLEEP_T);

  httpUpdater.setup(&server,"/up"); 
  
  server.on("/", getGraph);
  server.on("/pwr.svg", drawGraph);
  server.on("/cmd", handleCmd);
  server.on("/timing", timeIntervals);
  server.on("/pwr", getPwr);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  clearwp();

  configTime(MY_TZ, MY_NTP_SERVER); 
  digitalWrite(LED, HIGH);
}

float readanalog()
{
  int sensorValue; 
  sensorValue = analogRead(analogInPin);
  return (float)sensorValue * 0.0296;
}

int pwrsum;
int pwr1,pwr2,pwr3;

int http_get(String getstr)
{
  StaticJsonDocument<200> doc;
  WiFiClient client;
  HTTPClient http;
  int pwr = 12345;

  if (http.begin(client, getstr)) 
  {  
    int httpCode = http.GET();
    if (httpCode > 0) 
    {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) 
      {
        String payload = http.getString();
        //Serial.println(payload);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) 
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
        }
        else pwr = doc["power"];
      }
    } 
    else Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
  } 
  else Serial.printf("[HTTP} Unable to connect\n");
  
  return pwr;
}

float batvolt;
int8_t state;

void loop() 
{
  actTimems = millis();

  if (nextShellyms <= actTimems)
  {
    boolean fail = false;
    nextShellyms += ShellyInterval; 
    if (WiFi.status() == WL_CONNECTED) 
    {
      digitalWrite(LED, false);
      pwr1 = http_get("http://" + Shelly_IP + "/emeter/0");
      if (pwr1 == 12345) fail = true;
      pwr2 = http_get("http://" + Shelly_IP + "/emeter/1");
      if (pwr2 == 12345) fail = true;
      pwr3 = http_get("http://" + Shelly_IP + "/emeter/2");
      if (pwr3 == 12345) fail = true;
      digitalWrite(LED, true);
    }
    else fail = true;
    
    if (fail) Serial.println("get pwr failed");
    else
    {
      pwrsum = pwr1 + pwr2 + pwr3;
      //Serial.print(pwrsum); Serial.print("  ");
    }
  }

  batvolt = readanalog();
  if (IsActive)
  {
    if      (batvolt > HIBat) // hi battery voltage
    {
      chg_on(false);
      highbatinhibit = true;
    }
    else if (batvolt < LOBat) // low battery
    {
      wr_on(false); // off if bat < 24.4 V   
      lowbatinhibit = true;
    }
    else
    {  
      // HIBat LOBat hysteresis
      if ((highbatinhibit)& (batvolt < ReChgBat))   highbatinhibit = false;
      if ((lowbatinhibit) & (batvolt > HalfChgBat)) lowbatinhibit = false;

      // WR switching
      if      (ChgOnly) wr_on(false);       // no discharge becuz disabled
      else if (lowbatinhibit) wr_on(false); // no discharge becuz low bat
      else if (pwrsum >= HIlimit)           // more consumption than solar
      {
        if ((!lowbatinhibit) && (!ChgOnly) && (!ChgNow)) wr_on(true);
      }
      else if (pwrsum <= LOlimit) wr_on(false); // more solar than consumption

      // DC charger switching
      if (highbatinhibit) chg_on(false);
      else if (ChgNow) // manual force charge
      {
        if (batvolt <= ChgLimit) chg_on(true);
        else ChgNow = false;  
      }
      else if (pwrsum <= CHGlimitLO) // way too much solar
      {
        if (!CHGstate) chg_on(true);
      }
      else if (pwrsum >= CHGlimitHI)
      {
        if (CHGstate) chg_on(false);
      }
    }
    
  } // is active
  else 
  {
    wr_on(false);
    chg_on(false);
  }

  // 720 points, every min -> 12h.
  time(&now);
  localtime_r(&now, &tm);
  
  state = 0;
  if (WRstate) state += 1;   
  if (CHGstate) state += 2;
   
  storewp(constrain(pwrsum,-1000,1000), batvolt, state, tm.tm_hour);
  Serial.println();
    
  while (millis() <= nextTimems)
  {
    delay(20);
    server.handleClient();
  }
  nextTimems = LoopInterval + millis();
}
