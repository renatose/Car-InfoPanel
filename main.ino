#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS1307new.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <dht11.h>

dht11 DHT11;
#define DHT11PIN 4
#define ONE_WIRE_BUS 7
#define BUTTON1 12
#define BUTTON2 11
#define ROW_DATA 0
#define ROW_TIME 1
#define HOLD_MS 500
#define WHEEL_RADIUS 1791
#define SAMPLING_MS 2000
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

byte cedilla[8] = {
  0b00000,
  0b01110,
  0b10000,
  0b10000,
  0b10001,
  0b01110,
  0b00100,
  0b01100
};
byte acute_a[8] = {
  0b00010,
  0b00100,
  0b01110,
  0b00001,
  0b01111,
  0b10001,
  0b01111,
  0b00000
};
byte degC[8] = {
  0b01000,
  0b10100,
  0b01000,
  0b00110,
  0b01001,
  0b01000,
  0b01001,
  0b00110
};
const char degree = (char)0b11011111;

LiquidCrystal_I2C lcd(0x20, 16, 2); // set the LCD address to 0x20 for a 16 chars and 2 line display

bool upd_time = true;
bool prev_state[2] = {HIGH, HIGH};
bool curr_state[2] = {HIGH, HIGH};
char button_status = 0;
int count = 0;
unsigned long button_timer;
byte adjust_time = 0;
bool type0 = true;
bool type1 = false;
bool double_hold = false;
void setup() {
  DHT11.read(DHT11PIN);

  sensors.begin();
  sensors.getAddress(insideThermometer, 0);
  sensors.setResolution(insideThermometer, 12);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  //  Serial.begin(57600);
  attachInterrupt(0, set_irq_flag0, CHANGE);
  pinMode(2, INPUT_PULLUP);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  RTC.ctrl = 0x10;                 // 0x00=disable SQW pin, 0x10=1Hz,
  RTC.setCTRL();                   // 0x11=4096Hz, 0x12=8192Hz, 0x13=32768Hz

  lcd.init();                      // initialize the lcd
  lcd.createChar(1, degC);
  lcd.createChar(2, acute_a);
  lcd.createChar(6, cedilla);
  lcd.home();
  lcd.setBacklight(1);
}

void loop() {
  if (adjust_time == 0) {
    switch (button_status) {
      case -1:
        detachInterrupt(0);
        RTC.ctrl = 0x0;                 // 0x00=disable SQW pin, 0x10=1Hz,
        RTC.setCTRL();
        adjust_time = 1;
        lcd.setCursor(0, ROW_DATA);
        lcd.print("DD/MM/AAAA hh:mm");
        lcd.blink_on();
        upd_time = true;
        break;
      case 1:
        // Change time format displayed
        if (!type0 && !type1)
          type0 = true;
        else {
          type1 = type0;
          type0 = false;
        }
        upd_time = true;
        break;
      case 2:
        // Change data displayed
        break;
      case -2:
        // Race mode
        break;
      case 3:
        //auto check mode
        break;
      case 0:
        if (upd_time) {
          RTC.getTime();
          print_it();
          upd_time = false;
        }
    }
  } else {
    switch (button_status) {
      case 3:
        if (RTC.day <= last_dom()) {
          RTC.fillByYMD(RTC.year, RTC.month, RTC.day);
          RTC.fillByHMS(RTC.hour, RTC.minute, 0);
          RTC.setTime();
          RTC.startClock();
        }
      case -1:
        RTC.ctrl = 0x10;                 // 0x00=disable SQW pin, 0x10=1Hz,
        RTC.setCTRL();
        lcd.blink_off();
        adjust_time = 0;
        lcd.clear();
        upd_time = true;
        attachInterrupt(0, set_irq_flag0, CHANGE);
        break;
      case 1:
        adjust_time++;
        if (adjust_time == 6)
          adjust_time = 1;
        break;
      case 2:
        switch (adjust_time) {
          case 1:
            RTC.year += 2;
            break;
          case 2:
            RTC.month += 2;
            break;
          case 3:
            RTC.day += 2;
            break;
          case 4:
            RTC.hour += 2;
            break;
          case 5:
            RTC.minute += 2;
            break;
        }
        // Edit time section
      case -2:
        switch (adjust_time) {
          case 1:
            RTC.year--;
            break;
          case 2:
            RTC.month--;
            break;
          case 3:
            RTC.day--;
            break;
          case 4:
            RTC.hour--;
            break;
          case 5:
            RTC.minute--;
            break;
        }
        upd_time = true;
      default:
        switch (adjust_time) {
          case 1:
            if (RTC.year == 2100) {
              RTC.year = 2014; upd_time = true;
            } else if (RTC.year == 2013) {
              RTC.year = 2099; upd_time = true;
            }
            break;
          case 2:
            if (RTC.month == 13) {
              RTC.month = 1; upd_time = true;
            } else if (RTC.month == 0) {
              RTC.month = 12; upd_time = true;
            }
            break;
          case 3:
            if (RTC.day == last_dom() + 1) {
              RTC.day = 1; upd_time = true;
            } else if (RTC.day == 0) {
              RTC.day = last_dom(); upd_time = true;
            }
            break;
          case 4:
            if (RTC.hour == 24) {
              RTC.hour = 0; upd_time = true;
            } else if (RTC.hour > 24) {
              RTC.hour = 23; upd_time = true;
            }
            break;
          case 5:
            if (RTC.minute == 60) {
              RTC.minute = 0; upd_time = true;
            } else if (RTC.minute > 60) {
              RTC.minute = 59; upd_time = true;
            }
            break;
        }
        // Edit time section
        break;
        //  default:
        // cenas
    }
    if (upd_time) {
      print_it();
      upd_time = false;
    }
    switch (adjust_time) {
      case 1:
        lcd.setCursor(9, ROW_TIME);
        break;
      case 2:
        lcd.setCursor(4, ROW_TIME);
        break;
      case 3:
        lcd.setCursor(1, ROW_TIME);
        break;
      case 4:
        lcd.setCursor(12, ROW_TIME);
        break;
      case 5:
        lcd.setCursor(15, ROW_TIME);
        break;
    }
  }
  curr_state[0] = digitalRead(BUTTON1);
  curr_state[1] = digitalRead(BUTTON2);
  button_status = 0;
  if (curr_state[0] * curr_state[1] * prev_state[0] * prev_state[1] == 0) {
    if (prev_state[0] != curr_state[0]) {
      if (curr_state[0] == HIGH) {
        button_status = 1;
        if (button_timer + HOLD_MS < millis()) {
          button_status = -1;
          if (double_hold) {
            button_status = 3;
            double_hold = false;
          }
        }
        if (!curr_state[1]) {
          button_status = 0;
          double_hold = true;
        }
      }
      else {
        button_timer = millis();
      }
      prev_state[0] = curr_state[0];
    }
    if (prev_state[1] != curr_state[1]) {
      if (curr_state[1] == HIGH) {
        button_status = 2;
        if (button_timer + HOLD_MS < millis()) {
          button_status = -2;
          if (double_hold) {
            button_status = 3;
            double_hold = false;
          }
        }
        if (!curr_state[0]) {
          button_status = 0;
          double_hold = true;
        }
      }
      else {
        button_timer = millis();
      }
      prev_state[1] = curr_state[1];
    }
  }
}

void set_irq_flag0()
{
  upd_time = true; // set flag
}
byte last_dom() {
  switch (RTC.month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    case 2:
      return (28 + is_leap_year(RTC.year));
  }
}
byte is_leap_year(word y)
{
  //uint16_t y = year;
  if (
    ((y % 4 == 0) && (y % 100 != 0)) ||
    (y % 400 == 0)
  )
    return 1;
  return 0;
}
void print_date()
{
  //static char date[10];
  if (RTC.day < 10)
    lcd.print('0');
  lcd.print(RTC.day, DEC);
  lcd.print('/');
  if (type0 || adjust_time != 0) {
    if (RTC.month < 10)
      lcd.print('0');
    lcd.print(RTC.month, DEC);
    lcd.print('/');
    lcd.print(RTC.year, DEC);
  }
  else {
    char moy[12][4] = {
      "Jan", "Fev", "Mar", "Abr", "Mai", "Jun", "Jul", "Ago", "Set", "Out", "Nov", "Dez"
    };
    lcd.print(moy[RTC.month - 1]);
    lcd.print(' ');
    if (!type1) {
      char wd[7][4] = {
        "Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"
      };
      //  wd[2][3] = (char)6;
      wd[6][1] = (char)2;
      lcd.print(wd[RTC.dow]);
    }
    //    sprintf(date, "%3s %3s", print_month, print_weekday());
  }
  /*if (type0 || adjust_time!=0) {
    sprintf(date, "%02u/%02u/%04u ", RTC.day, RTC.month, RTC.year);
    }
    else {
    sprintf(date, "%02u/%3s %3s ", RTC.day, print_month, print_weekday());
    }
  return &date[0];*/
}
void print_it() {
  lcd.setCursor(0, ROW_TIME);
  if (RTC.day == 165) {
    lcd.print("ERRO NO RELOGIO");
  }
  else {
    print_date();
    lcd.print(' ');
    //lcd.setCursor(11, ROW_TIME);
    print_time();
  }
}
void print_time()
{
  bool temp = digitalRead(2);
  if (RTC.hour < 10)
    lcd.print('0');
  lcd.print(RTC.hour);
  if (temp)
    lcd.print(' ');
  else
    lcd.print(':');
  if (RTC.minute < 10)
    lcd.print('0');
  lcd.print(RTC.minute);
  if (temp)
    lcd.print(' ');
  else
    lcd.print(':');
  if (RTC.second < 10)
    lcd.print('0');
  lcd.print(RTC.second);
}
/*
void print_time()
{
  char s;
  if (digitalRead(2))
    s = ' ';
  else
    s = ':';
  char time[9];
  sprintf(time,"%02i%c%02i%c%02i ", RTC.hour,s, RTC.minute,s, RTC.second);
  lcd.print(time);
}*/
char* print_temp(float temp_raw)
{
  int inte = int(temp_raw);
  int deci = temp_raw * 10 - inte * 10;
  if (deci == 10) {
    inte++;
    deci = 0;
  }
  static char temp[7];
  sprintf(temp, "%2i.%1u%c", inte, deci, (char)1);
  return &temp[0];
}
char* print_percent(byte numb_raw)
{
  static char percent[5];
  sprintf(percent, "HR%2u%% ", numb_raw);
  return &percent[0];
}
// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm
double dewPoint(double celsius, double humidity)
{
  double A0 = 373.15 / (273.15 + celsius);
  double SUM = -7.90298 * (A0 - 1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1) ;
  SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM - 3) * humidity;
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}
