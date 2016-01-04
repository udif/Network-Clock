/**
   GetLocalTime.ino

    Created on: 24.05.2015

*/

__extension__ typedef int __guard __attribute__((mode (__DI__)));

extern "C" int __cxa_guard_acquire(__guard *);
extern "C" void __cxa_guard_release (__guard *);
int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
void __cxa_guard_release (__guard *g) {*(char *)g = 1;};

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

#include "my_ssid.h"

//
// This class holds the WiFi connection state
// It is a singleton so that both web and ntp share it
//
class wifi {
    // Adapted from http://stackoverflow.com/questions/1008019/c-singleton-design-pattern
  public:
    static wifi& get_instance(void)
    {
      static wifi instance;
      return instance;
    }
  private:
    wifi(void);
    //wifi(wifi const&);
    //void operator=(wifi const &);

  public:
    ESP8266WiFiMulti WiFiMulti;
    wifi(wifi const&) = delete;
    void operator=(wifi const &) = delete;
};

wifi::wifi(void)
{
  USE_SERIAL.begin(115200);
  // USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }
  WiFiMulti.addAP(MY_SSID, MY_PASSWORD);
}

class net_time {
  protected:
    wifi& w;
    net_time(void);
  public:
    virtual void send_time_req(void) = 0;
    virtual const char *get_time(void) = 0;
    bool ready(void);
};

net_time::net_time(void) : w(wifi::get_instance())
{
}

//
// web_time - get local time from web services
//
class web_time: virtual public net_time {
  private:
    uint8_t tzdata_state;
    char datetime[64];
    uint8_t datetime_index;
    char tzdata[100];
    uint8_t comma_cnt, tz_cnt;
    void process_ip_api(uint8_t *buff, int len);
    void process_tzdata_org_json(uint8_t *buff, int len);
    void process_tzdata_org_xml(uint8_t *buff, int len);
    void process_http(const char *host, const char *path, void (web_time::*process_result)(uint8_t *buff, int len));

  public:
    void send_time_req(void);
    const char *get_time(void);
    web_time(void);

};

void web_time::process_ip_api(uint8_t *buff, int len)
{
  for (int i=0; i < len;) {
    char c = buff[i++];
    if (comma_cnt == 9 && (c != ',') && (tz_cnt < sizeof(tzdata) - 1)) {
      tzdata[tz_cnt++] = c;
      tzdata[tz_cnt] = 0;
    }
    if (c == ',')
      comma_cnt++;
  }
}

void web_time::process_tzdata_org_xml(uint8_t *buff, int len)
{
  static const char tzdata_xml_tag[] = "LocalTime>";
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

void web_time::process_tzdata_org_json(uint8_t *buff, int len)
{
  static const char tzdata_json_tag[] = "localTime\": \"";
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

void web_time::process_http(const char *host, const char *path, void (web_time::*process_result)(uint8_t *buff, int len))
{

  HTTPClient http;

  http.begin(host, 80, path);

  //USE_SERIAL.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();
  if (httpCode) {
    // HTTP header has been send and Server response header has been handled


    // file found at server
    if (httpCode == 200) {

      // get lenght of document (is -1 when Server sends no Content-Length header)
      int len = http.getSize();

      // create buffer for read
      uint8_t buff[128] = { 0 };

      // get tcp stream
      WiFiClient *stream = http.getStreamPtr();

      // read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();

        if (size) {
          // read up to 128 byte
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          (this->*process_result)(buff, c);

          // write it to Serial
          //USE_SERIAL.write(buff, c);

          if (len > 0) {
            len -= c;
          }
        }
        delay(1);
      }
      http.end();
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, code: %d\n", httpCode);
    }
  } else {
    USE_SERIAL.print("[HTTP] GET... failed, no connection or no HTTP server\n");
  }
}

web_time::web_time(void)
{
  //USE_SERIAL.print("[HTTP] begin...\n");
  // configure traged server and url
  comma_cnt = 0;
  strcpy(tzdata, "/lookup?tzid=");
  tz_cnt = strlen(tzdata);
  process_http("ip-api.com", "/csv", &web_time::process_ip_api);
  //USE_SERIAL.println(tzdata);
  //USE_SERIAL.println();

}

void web_time::send_time_req(void)
{
  tzdata_state = 255;
  comma_cnt = 0;
}

const char *web_time::get_time(void)
{
  datetime_index = 0;
  tzdata_state = 255;
  process_http("www.tzdata.org", &tzdata[0], &web_time::process_tzdata_org_json);
  return &datetime[0];
}

//
//  NTP code
//
class ntp_time : virtual public net_time {
    unsigned int localPort = 2390;      // local port to listen for UDP packets
    IPAddress timeServerIP; // time.nist.gov NTP server address
    const char *ntpServerName = "pool.ntp.org";
    static const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
    byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
    WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
  public:
    ntp_time(void);
    void send_time_req(void);
    const char *get_time(void);
};

// send an NTP request to the time server at the given address
void ntp_time::send_time_req(void)
{
  //Serial.println("sending NTP packet...");
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
  udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

ntp_time::ntp_time(void)
{
  udp.begin(localPort);
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
}

const char *ntp_time::get_time(void)
{

  send_time_req(); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  } else {
    //Serial.print("packet received, length=");
    //Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    //Serial.print("Seconds since Jan 1 1900 = " );
    //Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    //Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    //Serial.println(epoch);


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
  return "";
}

bool net_time::ready(void)
{
  return (w.WiFiMulti.run() == WL_CONNECTED);
}

//
// main program starts here
//
ntp_time *ntp;
web_time *web;

void display_time(void)
{
  // wait for WiFi connection
  if (web->ready()) {
    USE_SERIAL.print("T");
    USE_SERIAL.println(web->get_time());
    //USE_SERIAL.println(ntp->get_time());
  }
}

// Nothing to do here, everything done by constructors
void setup ()
{
  web = new web_time;
  //ntp = new ntp_time;
}

void loop() {
  display_time();
  delay(10000);
}

