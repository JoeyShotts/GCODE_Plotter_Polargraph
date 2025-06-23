/*
Displays what function will run. Does not have to be included if project is to be used on arduino uno.
*/

#ifdef OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

//defaults to mega pins 21 and 22

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

void SETUP_OLED(){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Display the splash screen (we're legally required to do so)
  display.display();
  delay(1000); // Pause for 1 second

  // Clear the buffer
  display.clearDisplay();

  display.setTextSize(3);             
  display.setTextColor(SSD1306_WHITE);       
  display.setCursor(0,5);             // Start at top-left corner
  display.println(F("HOME"));
  display.display();
  delay(300);

}

// void setTextSize(int sz){
//   display.setTextSize(sz);
// }

void writeText(String str){
  // setTextSize(3);
  display.clearDisplay();
  display.setCursor(0, 5);
  display.println(str);
  display.display();
  Serial.println(str);
}
#endif