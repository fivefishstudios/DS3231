/* 
  Demo program combining Multiplexed 7-segment LED Display and LCD Display.
  Push-button Switch input via Interrupt subroutines.
  + playing with RGB discrete LEDs
  + displaying RTC clock
  + using DS3231 RTC module
*/
#include "mbed.h"
#include "LCD_DISCO_F429ZI.h"
#include "clock.h"
#include "ds3231.h"
using namespace std;

// serial comms for debugging
Serial pc(USBTX, USBRX);

// LCD library
LCD_DISCO_F429ZI lcd;

// Interrupt for User PushButton switch
InterruptIn Button(PA_0);

// User LED
DigitalOut led(PG_13);
PwmOut RGBLED_red(PE_9);
PwmOut RGBLED_grn(PE_11);
PwmOut RGBLED_blu(PA_5);

// instantiate RTC object
Ds3231 rtc(PB_11, PB_10);   // (sda, scl)  -- consult datasheet for I2C channel pins we can use 

// Our Interrupt Handler Routine, for Button(PA_0)
void PBIntHandler()
{
  led = !led; // toggle LED
  if (led)
  {
    lcd.DisplayStringAt(0, 150, (uint8_t *)"  Interrupt!  ", CENTER_MODE); // will this work?
  }
  else
  {
    lcd.DisplayStringAt(0, 150, (uint8_t *)" Another IRQ! ", CENTER_MODE); // will this work?
  }
}

/* intensity in % percentage */
void SetLEDBrightness(PwmOut led, float intensity)
{
  float period = 0.000009f;
  led.period(period);
  led.pulsewidth(period * (intensity / 100));
  wait(0.001);
}

#define DISPLAY_DELAY 0.001f

static const int Digits[] = {
    //abcdefgp    // 7-segment display + decimal point
    0b11111100, // 0
    0b01100000, // 1
    0b11011010, // 2
    0b11110010, // 3
    0b01100110, // 4
    0b10110110, // 5
    0b10111110, // 6
    0b11100000, // 7
    0b11111110, // 8
    0b11110110, // 9
    0b11101110, // A (10)
    0b00111110, // B (11)
    0b10011100, // C (12)
    0b01111010, // D (13)
    0b10011110, // E (14)
    0b10001110  // F (15)
};

/*
  NOTE: The weird ass and seemingly random pin assignments below is necessary because 
  we have USB Serial and LCD display enabled. Consult user manual for available pins. 
  */

DigitalOut Digit1(PC_11); // anode for Digit1 (ones)
DigitalOut Digit2(PC_12); // anode for Digit2 (tens)
DigitalOut Digit3(PC_13); // anode for Digit3 (hundreds)

DigitalOut SegmentA(PA_7); // clockwise, starting from top segment
DigitalOut SegmentB(PA_8);
DigitalOut SegmentC(PA_15);
DigitalOut SegmentD(PB_4);
DigitalOut SegmentE(PB_7);
DigitalOut SegmentF(PB_12);
DigitalOut SegmentG(PC_3); // middle segment
DigitalOut SegmentP(PC_8); // decimal point

void Display_Clear()
{
  // reset all pins, clear display
  // common anode, so logic 1 for OFF
  SegmentA = SegmentB = SegmentC = SegmentD = SegmentE = SegmentF = SegmentG = SegmentP = !0;
  Digit3 = Digit2 = Digit1 = 0;
}

void Display_Digit(int DigitPosition, int Number)
{
  // common anode display, so invert logic to light up each segment
  SegmentA = !((Digits[Number] & 0b10000000) >> 7);
  SegmentB = !((Digits[Number] & 0b01000000) >> 6);
  SegmentC = !((Digits[Number] & 0b00100000) >> 5);
  SegmentD = !((Digits[Number] & 0b00010000) >> 4);
  SegmentE = !((Digits[Number] & 0b00001000) >> 3);
  SegmentF = !((Digits[Number] & 0b00000100) >> 2);
  SegmentG = !((Digits[Number] & 0b00000010) >> 1);
  SegmentP = !((Digits[Number] & 0b00000001) >> 0);

  // we need to clear out the other digits before displaying the new digit
  // otherwise, the same number will be displayed in all the digits.
  // common anode display, so logic 1 to light up the digit.
  switch (DigitPosition)
  {
  case 1:
    Digit1 = 1; // ones
    Digit2 = 0;
    Digit3 = 0;
    break;
  case 2:
    Digit1 = 0; // tens
    Digit2 = 1;
    Digit3 = 0;
    break;
  case 3:
    Digit1 = 0; // hundreds
    Digit2 = 0;
    Digit3 = 1;
    break;
  }
  wait(DISPLAY_DELAY);
}

void Display_Number(int Number, uint32_t Duration_ms)
{
  int hundreds, tens, ones;
  uint32_t start_time_ms, elapsed_time_ms = 0;
  Timer t;

  // breakdown our Number into hundreds, tens and ones
  hundreds = Number / 100;
  tens = (Number % 100) / 10;
  ones = (Number % 100) % 10;

  t.start(); // start timer, we'll use this to setup elapsed display time
  start_time_ms = t.read_ms();
  while (elapsed_time_ms < Duration_ms)
  {
    Display_Digit(3, hundreds);
    Display_Digit(2, tens);
    Display_Digit(1, ones);
    elapsed_time_ms = t.read_ms() - start_time_ms;
  }
  t.stop(); // stop timer
}

// clock data
uint32_t year = 2018;
uint32_t month = 12;
uint32_t day_of_week = 6;
uint32_t day = 15;
uint32_t hh = 10;
uint32_t mm = 54;
uint32_t ss = 00;
uint32_t rtn_val;

/* 
  Start of Main Program 
  */
int main()
{
  // set usb serial
  pc.baud(115200);

  SetDateTime(year, month, day, hh, mm, ss);
  // time = 12:00:00 AM 12hr mode
  ds3231_time_t rtctime = {ss, mm, hh, 1, 1};   // seconds, min, hours, am_pm (true=pm), mode (true=12hour format)
  rtn_val = rtc.set_time(rtctime);

  ds3231_calendar_t calendar = {day_of_week, day, month, year};    // dayofweek, day, month, year
  rtn_val = rtc.set_calendar(calendar);

  // setup Interrupt Handler
  Button.rise(&PBIntHandler);

  // setup LCD Display
  lcd.Clear(0xFF000011);
  lcd.SetFont(&Font24);
  lcd.SetBackColor(0xFF000011);      // text background color
  lcd.SetTextColor(LCD_COLOR_WHITE); // text foreground color
  char buf[50];                      // buffer for integer to text conversion

  // setup 7-segment LED Display
  Display_Clear();
  lcd.DisplayStringAt(0, 200, (uint8_t *)" by owel.codes ", CENTER_MODE); 

  int r, g, b;
  long ctr = 1;
  // start of main loop
  while (true)
  {
    for (r = 0; r < 100; r += 10)
    {
      for (g = 5; g < 100; g += 10)
      {
        for (b = 0; b < 100; b += 15)
        {

          ctr++; // increment counter for display by 7-segment
          if (ctr > 999)
          {
            ctr = 0;
          }

          SetLEDBrightness(RGBLED_red, r);
          SetLEDBrightness(RGBLED_grn, g);
          SetLEDBrightness(RGBLED_blu, b);

          sprintf(buf, "Red %03d ", r);
          lcd.SetTextColor(LCD_COLOR_RED);
          lcd.DisplayStringAt(10, 50, (uint8_t *)buf, CENTER_MODE);

          sprintf(buf, "Green %03d ", g);
          lcd.SetTextColor(LCD_COLOR_GREEN);
          lcd.DisplayStringAt(10, 70, (uint8_t *)buf, CENTER_MODE);

          sprintf(buf, "Blue %03d ", b);
          lcd.SetTextColor(LCD_COLOR_WHITE);
          lcd.DisplayStringAt(10, 90, (uint8_t *)buf, CENTER_MODE);

          // display time only hh:mm:ss pp
          // time_t seconds = time(NULL);
          // strftime(buf, sizeof(buf), "%I:%M:%S %p", localtime(&seconds));
          
          // rtn_val = rtc.get_time(&rtctime);    // RTC DS3231
          time_t epoch_time; 
          epoch_time = rtc.get_epoch();
          strftime(buf, sizeof(buf), "%I:%M:%S %p", localtime(&epoch_time));
          lcd.DisplayStringAt(10, 240, (uint8_t *)buf, CENTER_MODE);          

          Display_Number(ctr, 50); // Number to display on 7-segment LED, Duration_ms
        }
      }
    }
  }

  return 0;
}