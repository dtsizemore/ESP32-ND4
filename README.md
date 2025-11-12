# ESP32-ND4
ESP32 Sketch for driving a Symmetricom ND-4 clock. Replaces the Rabbit2000 MCU.

20251111 - DTSizemore

Replacement for the original Rabbit2000 processor in the Symmetricom ND-4 clock.

Remove the existing Rabbit2000 controller board, and utilize the existing pin sockets for connection.

+5v VCC is present so no additional regulation needed.

Below are pins on the ND-4 Driver board and their respective connections

| LINE | ESP32   | ND-4 |
|------|---------|------|
| VCC  | VCC     | VCC  |
| MOSI | GPIO 23 | PA2  |
| SCLK | GPIO 18 | PA0  |
| /CS  | GPIO 5  | PA1  |


Dependencies:
- ESP-WiFiSettings 3.9.2
- LedController 2.0.2 https://github.com/noah1510/LedController
