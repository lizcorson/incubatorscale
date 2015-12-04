This was a project for the course ENCH686 at UMBC. We created a weight scale for a low-cost neonatal incubator using an Arduino and compatible components.

These are the components we used:
*Arduino Uno
*SainSmart I2C 16x2 LCD
*Adafruit DS1307 Real Time Clock Breakout Board
*Catalex MicroSD Card Reader Board
*HX711 Board (Load Cell Amplifier/ADC)
*Phidgets 5kg load cell

A circuit diagram is included.

These are the code folders:

HX711Addon: This is the working code exactly as uploaded to the Arduino at the submission of our project

HX711Addon_commented: I went back and added additional comments to document for future users. I wanted to include both versions in case I accidentally broke something while commenting.

InProgressProject: This was a more complicated version of the code that I wrote that didn't work out but future users are welcome to play with it.

libraries: This should contain all the libraries needed to run the code. Most of the libraries also have examples included so future users can learn how they work.

rtctest: This is used to set the time on the RTC board in case the battery dies/is removed.

Thanks to makecourse.com for the excellent tutorials on I2C LCDs and Micro SD card writing. Thanks to @bodge for the HX711 library.