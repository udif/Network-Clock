# Network-Clock
A network-synchronized clock based on the DP Flash Destroyer platform + ESP8266

After having the Dangerous Prototypes Flash Destroyer kit lying dormant in a drawer, I've finally decided it's time to give it some good use.
This is a regular clock that gets date/time synchronization from the internet via a simple ESP8266 module.

Necessary board changes
-----------------------
1. The CC7 output is connected to the RC7/RX Pin, and it may be necessary to change it to the RC6/TX pin. This will allow the ESP8266 to send the date/time info via the UART.
Since the ESP8266 is a 3.3V device and the PIC18F2550 is a 5V device, the ESP8266 UART TX pin must be protected from software errors where the PIC18F2550 RX pin is turned into an output by mistake.
This can be done by driving the RX pin from the ESP8266 through a low Vf schottkey diode. Since the Vih threshold voltage of the PIC18F2550 is 2V in TTL mode, and the diode can drop as little as 0.6v, the ESP8266 output is sufficiently high to drive the 5V PIC.
2. Another required change is to add a 3.3V power supply for the ESP8266.
3. Another optional change, if we find a free PIC I/O pin, is power down control for the ESP8266, so we can enable it only when we want a new date/time update, increasing security and reducing power consumption.

Software changes
----------------
The original software has been ported to the modern XC8 compiler. The required packages are:
* XC8 compiler (Currently 1.35)  
http://www.microchip.com/mplabxc8windows
* The Legacy peripheral library  
http://ww1.microchip.com/downloads/en/DeviceDoc/peripheral-libraries-for-pic18-v2.00rc3-windows-installer.exe
(Reached from http://www.microchip.com/pagehandler/en_us/devtools/mplabxc/).

The last revision that should behave as as the original "Flash Destroyer" is:  
https://github.com/udif/Network-Clock/commit/2b5f688b798429381d05f851c033b18a248ddd0a  
It contains all the changes required for XC8.  
The actual commit that did all the work is:  
https://github.com/udif/Network-Clock/commit/c658de67bf5fdc220bea03da099797981bd075c8

Compile the code using the simple `compile.bat` script, and download it into the Flash Destroyer via the `prog.bat` script.
Linux users can convert these one-line script simply by transforming all \'s to /'s

ESP8266 code
------------
Additional ESP8266 code will be added here. It will be written using the ESP8266 Arduino port (https://github.com/esp8266/Arduino), or via the Open ESP8266 SDK (https://github.com/pfalcon/esp-open-sdk).  
Date and time will be downloaded via one of the following resources:  
http://www.worldweatheronline.com/api/docs/time-zone-api.aspx  
http://www.ipinfodb.com/ip_location_api_json.php  
https://freegeoip.net/  - no API key needed, TZ string returned, not UTC offset
http://timezonedb.com/api - require API Key, returns local time
http://www.tzdata.org/api

These have one advantage over plain NTP - They do GeoIP location to determine the timezone.
By using one of these services, the clock will work anywhere in the world.  
Once the correct date and time are calculated, it will be sent via the UART to the PIC for display.
The ESP8266 will update the PIC periodically (update frequency to be determined)

Use:
http://ip-api.com/json
Get field from "timezone:"
Paste into http://www.tzdata.org/lookup?tzid=
