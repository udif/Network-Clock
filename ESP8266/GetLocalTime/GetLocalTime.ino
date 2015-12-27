/**
   GetLocalTime.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

#include "my_ssid.h"

ESP8266WiFiMulti WiFiMulti;

void setup_ntp(void);

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

void setup_web(void)
{
    USE_SERIAL.print("[HTTP] begin...\n");
    // configure traged server and url
    process_http("ip-api.com", "/csv", &process_ip_api);
    USE_SERIAL.println(tzdata);
    USE_SERIAL.println();

}
void setup() {

  USE_SERIAL.begin(115200);
  // USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP(MY_SSID, MY_PASSWORD);
  setup_web();
  setup_ntp();

}

//
//  NTP code
//
static unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
//const char* ntpServerName = "time.nist.gov";
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void setup_ntp(void)
{
  udp.begin(localPort);
}

void get_ntp(void)
{
    //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
}

void get_time()
{
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;

    datetime_index = 0;
    tzdata_state = 255;
    process_http("www.tzdata.org", &tzdata[0], &process_tzdata_org_json);
    USE_SERIAL.println(datetime);
    USE_SERIAL.println();
    get_ntp();
  }
}

void loop() {
  get_time();
  delay(10000);
}

