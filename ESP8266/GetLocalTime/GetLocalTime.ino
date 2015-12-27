/**
   GetLocalTime.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

#include "my_ssid.h"

ESP8266WiFiMulti WiFiMulti;

static char tzdata[100] = "/lookup?tzid=";
static uint8_t comma_cnt = 0, tz_cnt = 13; // tzdata length
void process_ip_api(uint8_t *buff, int len)
{
  int i = 0;
  char c;

  while (i < len) {
    c = buff[i++];
    if (comma_cnt == 9 && (c != ',') && (tz_cnt < sizeof(tzdata)-1)) {
      tzdata[tz_cnt++] = c;
      tzdata[tz_cnt] = 0;
    }
    if (c == ',')
      comma_cnt++;
  }
}

uint8_t tzdata_state = 255;
static const char tzdata_xml_tag[] = "LocalTime>";
static char datetime[64];
static uint8_t datetime_index;
void process_tzdata_org_xml(uint8_t *buff, int len)
{
  int i = 0;
  char c;
  
  while (i < len) {
    c = buff[i++];
    USE_SERIAL.printf("tzdata_state: %d, c=%c\n", tzdata_state, c);
    if (tzdata_state == 254) { // "found <localtime>" state
      if (c == '<') return;
      if (datetime_index < 63)
        datetime[datetime_index++] = c;
      datetime[datetime_index] = 0;
    } else if (tzdata_state == 255) { // initial state
      if (c == '<')
        tzdata_state = 0;
      continue;
    } else {
      if (c == tzdata_xml_tag[tzdata_state++]) {
        if (tzdata_state == 10) { // stylen("LocalTime>")
          tzdata_state = 254; // "copy" state
          datetime_index = 0;
        }
        continue;
      } else {
        tzdata_state = 255; // string match fail, wait for next
        continue;
      }
    }
  }
}

static const char tzdata_json_tag[] = "localTime\": \"";
void process_tzdata_org_json(uint8_t *buff, int len)
{
  int i = 0;
  char c;

  if (tzdata_state == 253)
    return;
  while (i < len) {
    c = buff[i++];
    //USE_SERIAL.printf("tzdata_state: %d, c=%c\n", tzdata_state, c);
    if (tzdata_state == 254) { // "found <localtime>" state
      if (c == '\"') {
        tzdata_state = 253; // "end" state
        return;
      }
      if (datetime_index < 63)
        datetime[datetime_index++] = c;
      datetime[datetime_index] = 0;
    } else if (tzdata_state == 255) { // initial state
      if (c == '\"')
        tzdata_state = 0;
      continue;
    } else {
      if (c == tzdata_json_tag[tzdata_state++]) {
        if (tzdata_state == 13) { // stylen("LocalTime>")
          tzdata_state = 254; // "copy" state
          datetime_index = 0;
        }
        continue;
      } else {
        tzdata_state = 255; // string match fail, wait for next
        continue;
      }
    }
  }
}

void process_http(char *host, char *path, void (*process_result)(uint8_t *buff, int len)) {

    HTTPClient http;

    http.begin(host, 80, path);

    USE_SERIAL.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    if (httpCode) {
      // HTTP header has been send and Server response header has been handled

      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == 200) {

        // get lenght of document (is -1 when Server sends no Content-Length header)
        int len = http.getSize();

        // create buffer for read
        uint8_t buff[128] = { 0 };

        // get tcp stream
        WiFiClient * stream = http.getStreamPtr();

        // read all data from server
        while (http.connected() && (len > 0 || len == -1)) {
          // get available data size
          size_t size = stream->available();

          if (size) {
            // read up to 128 byte
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            (*process_result)(buff, c);

            // write it to Serial
            //USE_SERIAL.write(buff, c);

            if (len > 0) {
              len -= c;
            }
          }
          delay(1);
        }
        http.end();
      }
    } else {
      USE_SERIAL.print("[HTTP] GET... failed, no connection or no HTTP server\n");
    }
}
void setup() {

  USE_SERIAL.begin(115200);
  // USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP(MY_SSID, MY_PASSWORD);

}

void get_time()
{
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;

    USE_SERIAL.print("[HTTP] begin...\n");
    // configure traged server and url
    process_http("ip-api.com", "/csv", &process_ip_api);
    USE_SERIAL.println(tzdata);
    USE_SERIAL.println();
    datetime_index = 0;
    tzdata_state = 255;
    process_http("www.tzdata.org", &tzdata[0], &process_tzdata_org_json);
    USE_SERIAL.println(datetime);
    USE_SERIAL.println();
  }
}

void loop() {
  get_time();
  delay(10000);
}

