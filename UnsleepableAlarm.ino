/*
 * Main code driver for the Unsleepable Alarm Clock.
 * Creating and Making UG1070 - Fulbright University Vietnam.
 *
 * @author Phan Canh Minh Phuoc
 * @version v0.8
 *
 * @date 2018-12-19
*/


// -----------------------------------------------------------------------
// 1. Importing libraries

#include <OneButton.h>
#include <DHT.h>
#include <DS3231.h>
#include <LiquidCrystal_I2C.h>

// Testing purpose
#define LED_port 13

// Sensor Defining
#define DHTPIN 6    
#define DHTTYPE DHT11
#define PIR 5

// Motor Defining
#define motor_sound_1 11
#define motor_sound_2 12
#define motor_sound_enB 10

#define motor_sensor_1 7
#define motor_sensor_2 8
#define motor_sensor_enA 9


// -----------------------------------------------------------------------
// 2. Defining global variables


// Key-pad Defining

OneButton button1(3, true);
OneButton button2(2, true);
OneButton button3(1, true);
OneButton button4(0, true);

OneButton button_stop(4, false); // this button must be specified as false


// Clock-only mode defining

int current_Hour;
int current_Min;

unsigned long current_duration;

byte degree[8] = { // the (Celcius) degree symbol
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

// Alarm-mode defining

boolean setting_alarm = false;
boolean alarm_set = false;

boolean alarm_ringing = false;
boolean pressed_stop = false;
boolean scanning_mode = false;
boolean alarm_confirmed = false;

int hour_alarm;
int min_alarm;
int PIR_value;

unsigned long start_ring_mark, press_stop_mark, start_scan_mark, prev_PIR;

// long PIR_interval = 11700; -- for testing only
long PIR_interval = 12500;
long time_snooze = 30000;
long wait_press_stop = 30000;
long time_scanning = 900000; // 15 minutes


// Defining external devices

LiquidCrystal_I2C lcd (0x27, 16, 2);
DS3231 rtc(SDA,SCL);
DHT dht(DHTPIN, DHTTYPE);





// -----------------------------------------------------------------------
// 3. Start the program

// -----------------------------------------------------------------------
// Start setting up the connected devices

void setup() {

  // Testing purposes
  Serial.begin(9600);
  pinMode(LED_port, OUTPUT);


  pinMode(PIR, INPUT);

  analogWrite(motor_sensor_enA, 90);
  analogWrite(motor_sound_enB, 100);
  


  button1.attachClick(press_key_1);
  button2.attachClick(press_key_2);
  button3.attachClick(press_key_3);
  button4.attachClick(press_key_4);

  button_stop.attachClick(press_key_stop);
    
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, degree);

  rtc.begin();
  
  // rtc.setDOW(SUNDAY);     // Set Day-of-Week to SUNDAY
  // rtc.setTime(4, 20, 5);     // Set the time to 12:00:00 (24hr format)
  // rtc.setDate(15, 12, 2019);   // Set the date to January 1st, 2014

  dht.begin();

  // Showing hello messages
  hello_message();
  

}


// -----------------------------------------------------------------------
// Start the looping execution

void loop() {


  // --------------------------------------------------------------------
  // Getting time info and key_pressed
  
  current_Hour = get_Hour();
  current_Min = get_Min();

  button4.tick();
  button1.tick(); 
  button2.tick();  
  button3.tick();  
     
  button_stop.tick();

  current_duration = millis();



  // --------------------------------------------------------------------
  // Clock screen - Displaying Time Info Only 
  
  if ((setting_alarm == false) && (alarm_ringing == false)) display_clock_info();


  // --------------------------------------------------------------------
  // Alarm goes off

  if ((alarm_confirmed == true) && (alarm_set == true)) {
    
    if ((current_Hour == hour_alarm) and (current_Min == min_alarm) and (alarm_ringing == false)) {
      
      alarm_go_off(); // Start ringing the alarm
      
    }

    if (alarm_ringing == true) {

      if ((pressed_stop == true) && (current_duration - press_stop_mark > time_snooze) && (scanning_mode == false)) {

        // Start Scanning Mode
        
        lcd.setCursor(9,0);
        lcd.print("Mode");
        scanning_mode = true;
        start_scan();
        start_scan_mark = current_duration;
        
      }

      if ((pressed_stop == false) && (current_duration - start_ring_mark > wait_press_stop)) {
        
        // if user do not press stop 30sec after ringing        
        press_key_stop();
        
      }

      if (scanning_mode == true) {

        if (current_duration - start_scan_mark > time_scanning) {

          end_alarm(); // End the Alarm after xxx minutes of Scanning

        } else {

          // Scanning Mode Execution

          if (current_duration - prev_PIR > PIR_interval) {
        
            // the value must be between 11000 and 12500
            // or between 16500 and 18000
        
            // sensor detect for 1500 ms
            // stop for 4000 ms then re-detect
            
            PIR_value = digitalRead(PIR);
        
            if (PIR_value == HIGH) {
              
              
              start_sound();
              prev_PIR = current_duration;
              
            } else {
              
              stop_sound();   
              
            }
        
          }

        }

      }

      } // end alarm ringing

      
    }


} // end loop 






// -----------------------------------------------------------------------
// 4. Helper functions





// -----------------------------------------------------------------------
// Key-pressing functions:

void press_key_1() { // change alarm hour
  
  if ((setting_alarm == true) && (alarm_set == true)) {
    if (hour_alarm == 23)
      hour_alarm = 0;
      
    else hour_alarm += 1;
    
    lcd.setCursor(5,1);
    if (hour_alarm < 10) lcd.print(" ");
    lcd.print(hour_alarm);
  }
}

void press_key_2() { // change alarm minute
 
  if ((setting_alarm == true) && (alarm_set == true)) {
    if (min_alarm == 59)
      min_alarm = 0;
    else min_alarm += 1;
  
    lcd.setCursor(10,1);
    if (min_alarm < 10) lcd.print("0");
    lcd.print(min_alarm);
  }
  
}

void press_key_3() {

  if (setting_alarm == true) {
    
    setting_alarm = false;
    alarm_confirmed = true;

    if ((hour_alarm == current_Hour) && (min_alarm == current_Min)) {
      
      if (min_alarm == 59) {
        
        if (hour_alarm == 23) {
          hour_alarm = 0;
          min_alarm = 0;
        } else {
          hour_alarm += 1;
          min_alarm = 0;
        }
        
      } else {
  
        min_alarm += 1;
        
      }
    }
    
  }
  
}
    
void press_key_4() {

  if (alarm_set == false) {

    hour_alarm = current_Hour;
    min_alarm = current_Min;
 
  } // done suggesting

  
  if (alarm_ringing == false) { 

    if (setting_alarm == false) { // transform from clock mode

      alarm_confirmed = false;

      setting_alarm = true;

      lcd.setCursor(0,0);
      lcd.print("Alarm Mode:  ");

      if (alarm_set == true) {

          lcd.setCursor(13,0);
          lcd.print(" ON");
          lcd.setCursor(0,1);
          lcd.print("    ");
          lcd.setCursor(5,1);
          if (hour_alarm < 10) lcd.print(" ");
          lcd.print(hour_alarm);
          lcd.print(" : ");
          if (min_alarm < 10) lcd.print("0");
          lcd.print(min_alarm);
          lcd.print("     ");
          
      } else {

          lcd.setCursor(13,0);
          lcd.print("OFF");
          lcd.setCursor(0,1);
          lcd.print("                ");
      }
        
    } else { // already in setting alarm mode

      alarm_confirmed = false;

      alarm_set = !alarm_set;

      if (alarm_set == false) {
        
          lcd.setCursor(13,0);
          lcd.print("OFF");
          lcd.setCursor(0,1);
          lcd.print("                ");
          
      } else {

          lcd.setCursor(13,0);
          lcd.print(" ON");
          lcd.setCursor(5,1);
          if (hour_alarm < 10) lcd.print(" ");
          lcd.print(hour_alarm);
          lcd.print(" : ");
          if (min_alarm < 10) lcd.print("0");
          lcd.print(min_alarm);
        
      }
        
    }

  }

  
}


void press_key_stop() {

  if ((alarm_ringing == true) && (pressed_stop == false)) {

    lcd.setCursor(0,0);
    lcd.print("Scanning Soon...");
    lcd.setCursor(0,1);
    lcd.print("Leave your bed !");

    press_stop_mark = current_duration;

    prev_PIR = current_duration;
    pressed_stop = true;
    stop_sound();
    
  }
  
}


// -----------------------------------------------------------------------
// Clock-only mode helper functions:

int get_Hour() {

  char *char_hour = rtc.getHourStr();
  int current_hour = String (char_hour[1]).toInt() + String (char_hour[0]).toInt()*10;
  return current_hour;
  
}

int get_Min() {

  char *char_min = rtc.getMinStr();
  int current_Min = String (char_min[1]).toInt() + String (char_min[0]).toInt()*10;
  return current_Min;
  
}


void display_clock_info() {

  
  float temp = dht.readTemperature();

  // lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print(rtc.getDOWStr());
  lcd.print("   ");
  lcd.print(rtc.getDateStr());


  lcd.setCursor(0,1);
  lcd.print(round(temp));
  lcd.write(1);
  lcd.print("C");
  lcd.print("       "); 
  
  if (current_Hour == 0) lcd.print(" ");
  else if (current_Hour < 10) lcd.print("0");
  lcd.print(current_Hour);
  lcd.print(":");
  
  if (current_Min < 10) lcd.print("0");
  lcd.print(current_Min);

  
}


// -----------------------------------------------------------------------
// Alarm-mode helper functions:

void alarm_go_off() {
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Alarm Ringing...");
  lcd.setCursor(4,1);
  lcd.print("Wake Up !");

  alarm_ringing = true;
  start_ring_mark = current_duration;
  
  start_sound();
  
  
}

void end_alarm() {
  
    lcd.clear();
    stop_scan();
    stop_sound();
    alarm_set = false;
    alarm_ringing = false;
    scanning_mode = false;
    pressed_stop = false;
    
}


// -----------------------------------------------------------------------
// Motors driving helper functions 

void start_sound() {
  
  digitalWrite(motor_sound_1, HIGH);
  digitalWrite(motor_sound_2, LOW);

  digitalWrite(LED_port, HIGH);

}

void stop_sound() {
  
  digitalWrite(motor_sound_1, LOW);
  digitalWrite(motor_sound_2, LOW);

  digitalWrite(LED_port, LOW);
  
}


void start_scan() {
  digitalWrite(motor_sensor_1, HIGH);
  digitalWrite(motor_sensor_2, LOW);
}


void stop_scan() {
  digitalWrite(motor_sensor_1, LOW);
  digitalWrite(motor_sensor_2, LOW);
}


// -----------------------------------------------------------------------
// Start-up messages

void hello_message() {
  
  lcd.setCursor(3,0);
  lcd.print("Loading.");
  lcd.setCursor(0,1);
  lcd.print("UnstoppableAlarm");
  delay(2000);

  lcd.setCursor(3,0);
  lcd.print("Loading..");
  delay(2000);

  lcd.setCursor(3,0);
  lcd.print("Loading...");
  delay(2000);

  lcd.setCursor(3,0);
  lcd.print("SickName's");
  delay(3000);
  
}


// -----------------------------------------------------------------------
// For testing-only helper functions:

void alert_led(int times) {
  int i;
  
  for (i = 0; i < times; i++) {
    digitalWrite(LED_port, HIGH);
    delay(250);  
    digitalWrite(LED_port, LOW); 
    delay(250); 
  }
}
