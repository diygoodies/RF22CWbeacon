# RF22CWbeacon
> 70 cm CW beacon.

Based on STM32 bluepill adrduino board. It basically sends callsign and QTH in morse using SI4432 RF module. 
![](https://github.com/diygoodies/RF22CWbeacon/blob/master/Schematics/frontview.jpg)

## Usage example

After you press PTT on beacon frequency controller reply with the 3 level of power tone signal and sends "ECHO" in the end. 
Every 15 minutes Controller sends on air CALLSIGN of repeater or QTH locator by morse code.
Schematics https://github.com/diygoodies/RF22CWbeacon/blob/master/Schematics/Schematics.jpg

Progect page http://www.diygoodies.org.ua/?p=1545

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/949C4up2-Zk/0.jpg)](https://www.youtube.com/watch?v=949C4up2-Zk)
  
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/9Hn9lKGO0rA/0.jpg)](https://www.youtube.com/watch?v=9Hn9lKGO0rA)

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/iOeEjiBlF5k/0.jpg)](https://www.youtube.com/watch?v=iOeEjiBlF5k)

USART comamands list: https://github.com/diygoodies/RF22CWbeacon/Commandslist.txt

## Development setup
Code based on BluePill-RTClock-test example of
https://github.com/rogerclarkmelbourne/Arduino_STM32
package
