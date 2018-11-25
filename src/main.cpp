// Read two separate temperatures from DS18B20 sensors,
// show them on an LCD keypad shield and a WS2812B LED strip
// LED strip shows whether hot wine punch is at correct temperature

#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_PIN A3
#define DS18B20_RESOLUTION 12 // 9-12
#define DS18B20_WAIT 750 / (1 << (12 - DS18B20_RESOLUTION))

DeviceAddress tempSensor1 = {0x28, 0xFF, 0xD2, 0xA5, 0x24, 0x17, 0x03, 0xD7};
DeviceAddress tempSensor2 = {0x28, 0xFF, 0xC9, 0xFD, 0x24, 0x17, 0x03, 0x1F};

OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensors(&oneWire);

// LED strips
#include <Adafruit_NeoPixel.h>
#define PIXEL_PIN A2
#define PIXEL_COUNT 7
Adafruit_NeoPixel ledStrip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
const uint32_t COLOR_RED = ledStrip.Color(255, 0, 0);
const uint32_t COLOR_GREEN = ledStrip.Color(0, 255, 0);
const uint32_t COLOR_BLUE = ledStrip.Color(0, 0, 255);

// display
#include <Wire.h>
#include <LiquidCrystal.h>

int lcd_key = 0;
int adc_key_in1 = 0, adc_key_in2 = 0;
#define btnRIGHT 0
#define btnUP 1
#define btnDOWN 2
#define btnLEFT 3
#define btnSELECT 4
#define btnNONE 5

int nLastButton = btnNONE;

int read_LCD_buttons()
{
    // get current button state
    // with simple debouncing workaround
    adc_key_in1 = 0;
    adc_key_in2 = 1023;

    while (abs(adc_key_in1 - adc_key_in2) > 10)
    {
        // read again until value stays the same within range
        adc_key_in1 = analogRead(0);
        delay(5);
        adc_key_in2 = analogRead(0);
    }

    if (adc_key_in1 > 1000)
        return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result

    if (adc_key_in1 < 50)
        return btnRIGHT;
    if (adc_key_in1 < 195)
        return btnUP;
    if (adc_key_in1 < 380)
        return btnDOWN;
    if (adc_key_in1 < 555)
        return btnLEFT;
    if (adc_key_in1 < 790)
        return btnSELECT;

    return btnNONE; // when all others fail, return this...
}

LiquidCrystal lcd = LiquidCrystal(8, 9, 4, 5, 6, 7);


void showShortMsg(const char *msg1, const char *msg2, uint16_t delayTime = 1000)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(msg1);
    lcd.setCursor(0, 1);
    lcd.write(msg2);
    lcd.display();
    delay(delayTime);
}

unsigned long nLastTempRequestTime = 0;


void message_on_boot()
{
    unsigned long nStartTime = millis();
    while(millis() < nStartTime+3000)
    {
        if (read_LCD_buttons() == btnRIGHT)
        {
            // skip init process if btnRIGHT is pressed
            break;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("GLUEHWODISP v1.0"));
        lcd.setCursor(7, 1);
        lcd.print((int)((nStartTime+3000-millis())/1000)+1);
        delay(200); // so display does not blink
    }
}

void display_degrees_celsius()
{
  lcd.print(" ");
  lcd.print((char)223); // degree symbol
  lcd.print("C");
}

void display_temps(float t1, float t2)
 {
   lcd.clear();

   lcd.setCursor(0, 0);
   lcd.print("T1: ");
   lcd.print(t1);
   display_degrees_celsius();

   lcd.setCursor(0, 1);
   lcd.print("T2: ");
   lcd.print(t2);
   display_degrees_celsius();
 }

void setup(void) {
  Serial.begin(115200);

  tempSensors.begin();
  tempSensors.setResolution(DS18B20_RESOLUTION);
  tempSensors.setWaitForConversion(false);

  tempSensors.requestTemperatures();
  nLastTempRequestTime = millis();

  ledStrip.begin();
  ledStrip.setBrightness(128); // 50% brightness
  ledStrip.show();

  lcd.begin(16, 2);

  message_on_boot();
}

void set_single_temp_display(const float t, const int led_idx_offset)
{
    // led_idx_offset: idx of first LED to use

    if(t < 57)
    {
        // too cold
        ledStrip.setPixelColor(led_idx_offset+0, COLOR_BLUE);
    }
    else if(t < 63)
    {
        // slightly too cold
        ledStrip.setPixelColor(led_idx_offset+0, COLOR_BLUE);
        ledStrip.setPixelColor(led_idx_offset+1, COLOR_GREEN);
    }
    else if(t < 67)
    {
        // optimal
        ledStrip.setPixelColor(led_idx_offset+1, COLOR_GREEN);
    }
    else if(t < 70)
    {
        // slightly too hot
        ledStrip.setPixelColor(led_idx_offset+1, COLOR_GREEN);
        ledStrip.setPixelColor(led_idx_offset+2, COLOR_RED);
    }
    else
    {
        // too hot
        ledStrip.setPixelColor(led_idx_offset+2, COLOR_RED);
    }

}

void set_leds_temp_display(const float t1, const float t2)
{
    set_single_temp_display(t1, 0);
    set_single_temp_display(t2, 4);

    ledStrip.show();
}


void loop(void) {
  unsigned long nNow = millis();
  if (nNow - nLastTempRequestTime > DS18B20_WAIT)
  {
      float t1 = tempSensors.getTempC(tempSensor1);
      float t2 = tempSensors.getTempC(tempSensor2);

      display_temps(t1, t2);
      set_leds_temp_display(t1, t2);

      tempSensors.requestTemperatures();
      nLastTempRequestTime = millis(); // request again
  }

}