# Dino-Mochi-ESP32 🦖🤖
Open-source Arduino IDE code for the esp32 c3 supermini, featuring smooth animations, touch reactivity, sound effects, and a hidden Google Dino game.
## Features
* Smooth face animations (total 12 faces) generated from https://huykhong.com/IOT/gif2cpp/
* Touch reactivity - Switches to next animation on single tap, opens game on long press.
* Sound effects- Startup sound, beeps on touch, and sound effects in the dino game.
## Hardware required
* ESP32 C3 SuperMini
* 0.96 inch monochrome OLED display (SSD1306)  (Will later add support for 1.3 inch SSH1306 as well)
* TTP223 touch sensor
* Audio amplifier (based on 8002D IC)
* Small lipo cell(s) (I used two 40 mah cells in parallel)
* TP4056 charging module (Smaller, Type-C version) , **WARNING!!** Replace 1k resistor on tp4056 module with 10k or else you will fry your tiny battery!!
* Tiny speaker, you can use passive buzzer, I used old speaker salvaged from dead phone.
* Tiny sliding switch, perfboard, male and female pin headers, etc.
* I haven't used any 5 volt boost module, connecting TP4056 board's output directly to esp32 5 volt pin through a switch works, but DONT PLUG IN THE ESP32 TO A COMPUTER OR CHARGER WHEN THE SWITCH IS ON, OR ELSE 5 VOLTS WILL GO INTO YOUT BATTERY AND DESTROY IT!!
## Wiring the modules
You can wire the modules however you want, just put your pin configuration into my code before uploading it to the esp32, or, you could just wire it exactly the way I wired it, so that you don't need to modify the code at all.
* Display SDA - Pin 20
* Display SCL - Pin 21
* Touch Sensor I/O - Pin 1
* Buzzer/Audio Signal- Pin 2
* Note: Display and touch sensor VCC is connected to 3.3 volt pin of esp32, Audio amplifier VCC is connected to 5 volt pin.
## How to install
* Click <>Code and Download as zip from the main github repository page.
* Extract the zip to your computer, do not rename any file inside or else it wont work.
* Open dinomochiesp32.ino in arduino IDE
* Make sure you have all the required libraries installed. All libraries can be installed from Arduino IDE.
* Select your board and COM port, open device manager if you don't know the COM port.
* Click the upload button.
## Credits
*Some animations and the animation code have been taken from https://github.com/huykhoong/esp32_dasai_mochi_clone_and_how_to
*Some gifs taken from https://github.com/pham-tuan-binh/watcher-mochi/tree/main/sd_content and converted to esp32 compatible c++ code using this website: https://huykhong.com/IOT/gif2cpp/
