# ALLInOneHub

## build command 

west build -p always -b raytac_mdbt50q_db_40_nrf52840

## pin match
### LED
* power 0.13
* reserve 0.14

### Button
* power 0.11
* reserve up 0.12
* reserve confirm 0.24
* neopixel mode 25

### FND
* DIO 0.19
* CLK 0.20

### Neopixel
* Data pin 0.21

### Uart to HID
* RX 1.01
* TX 1.02

### Log
* RX 0.08
* TX 0.06