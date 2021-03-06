#include <Servo.h>
#include<SoftwareSerial.h>
#include <ArduinoJson.h>
#include <Nextion.h>
#include "NexButton.h"

//====== Serial COnnection with NODEMCU =====
SoftwareSerial SUART(2, 3); //SRX=Dpin-2; STX-DPin-3

Servo servoright;
int pos = 0;   

// ==== Analog Pins =====
float potpinIE_ratio = 0;  
int potpinTidVol = 1;  
int potpinBPM = 5;
int pinguage_mask = 4;
int pinguage_expiration = 3;
int pinguage_diff = 2;

// ==== Analog Pins ====
//int inPin = 4;

// Declare your Nextion objects - Example (page id = 0, component id = 1, component name = "b0") 
NexText t_mode = NexText(0, 9, "t_mode");
NexButton b0 = NexButton(0, 2, "b0");
NexButton b1 = NexButton(0, 3, "b1");
NexButton b2 = NexButton(0, 4, "b2");
NexText t3 = NexText(1, 5, "t3");
NexText t4 = NexText(1, 6, "t4");
NexText t5 = NexText(1, 7, "t5");
NexText t_ie_ratio = NexText(0, 10, "t_ie_Ratio");
NexText t_bpm = NexText(0, 11, "t_bpm");
NexText t_tidvol = NexText(0, 12, "t_tidvol");
String set_mode = "";

NexTouch *nex_listen_list[] = {&b0,&b1,&b2,NULL};

void b0PopCallback(void *ptr) {
  t_mode.setText("MODE : ACV");
  set_mode = "acv";
  Serial.println(set_mode);
  //acv_mode();
  
}

void b1PopCallback(void *ptr) {
  t_mode.setText("MODE : SIMV");
  //simv_mode();
  set_mode = "simv"; 
  
}

void b2PopCallback(void *ptr) {
  t_mode.setText("MODE : None");
  //no_mode();
  set_mode = "None"; 
  Serial.println(set_mode);
}



// ==== Sensor Offset =====
const float guageSensorOffset = 41;
const float pressureDiffSensorOffset = 41;

// ===== Other vars ======
float check_value;
float IE_ratio;  
float TidVol;  
float BPM;
float separation;  
float sensorvalue;
float pressure_mask;
float pressure_expiration;
float pressure_diff;

int display_IE_ratio;
int display_BPM;
int display_TidVol;
int display_sensorvalue_cmh2o;
int display_sensorvalue_gp_cmh2o;


void setup()
{
  servoright.attach(9);  // attaches the servo on pin 9 to the servo object
  Serial.begin(9600);
  SUART.begin(9600); //enable SUART Port for communication with NODEMCU
  nexInit();

  // Register the pop event callback function of the components
  b0.attachPop(b0PopCallback, &b0);
  b1.attachPop(b1PopCallback, &b1);
  b2.attachPop(b2PopCallback, &b2);
  
}



void loop()
 
{ nexLoop(nex_listen_list);
  
/*   if (set_mode == "simv")
  {
      while(set_mode=="simv")
      {
        simv_mode();
      }
  }
  else 
  {   while(set_mode == "acv")
      {
        acv_mode();  
      }
      
  } */
}


// =======================
// ACV MODE Function
// =======================

void acv_mode()
{
  float IE_ratio;
  float TidVol;
  float BPM;
  float per_breath_time;
  float per_inspiration_time;
  float per_expiration_time;
  float human_effort;
  float separation;
  uint32_t cycleEndTime;
  bool firstRun = true;

  while(set_mode == "acv")
  {
      // Fetch all potentiometer values
      IE_ratio = map(analogRead(potpinIE_ratio), 0, 1023, 1.00, 4.00);    
      TidVol = map(analogRead(potpinTidVol), 0, 1023, 40.00, 90.00);     
      BPM = map(analogRead(potpinBPM), 0, 1023, 8.00, 30.00);     
      separation = (60/BPM - (1+IE_ratio));
      if (separation < 0)
      { IE_ratio = 60/BPM - 1;
        separation = 60/BPM - (1+IE_ratio);
       }

      // Fetch pressure sensor values
      pressure_mask = (map(analogRead(pinguage_mask), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92;    
      pressure_diff = (map(analogRead(pinguage_diff), 0, 1023, 0, 1023) - pressureDiffSensorOffset)*10.1972/92;
      pressure_expiration = (map(analogRead(pinguage_expiration), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92;
      // Initiate the cycle
      if(firstRun)
      {
        inspiration(TidVol);
        delay(15);
        cycleEndTime = expiration(TidVol, IE_ratio);
        firstRun = false;
      }

      // ========= Identify trigger and initiate the cycle =============
      if(millis() - cycleEndTime >= (uint32_t)separation*1000 || pressure_mask < -1)
      {
        inspiration(TidVol);
        delay(15);
        cycleEndTime = expiration(TidVol, IE_ratio);
      }
      print_to_screen(BPM, IE_ratio, pressure_mask, pressure_expiration, TidVol, pressure_diff);
  }
  
}

// =======================
// SIMV Logic Function
// =======================

uint32_t simv_logic(float pressure_mask, float TidVol, float IE_ratio)
{
    uint32_t cycleEndTime;
    pressure_mask = int(floor(pressure_mask));
    switch(int(pressure_mask)){
        case -6:
        case -5:
        case -4:
            TidVol = TidVol*0.25;
            inspiration(TidVol);
            delay(15);
            cycleEndTime = expiration(TidVol, IE_ratio);
            break;
        case -3:
            TidVol = TidVol*0.50;
            inspiration(TidVol);
            delay(15);
            cycleEndTime = expiration(TidVol, IE_ratio);
            break;
        case -2:
            TidVol = TidVol*0.75;
            inspiration(TidVol);
            delay(15);
            cycleEndTime = expiration(TidVol, IE_ratio);
            break;
        case -1:
            TidVol = TidVol*0.75;
            inspiration(TidVol);
            delay(15);
            cycleEndTime = expiration(TidVol, IE_ratio);
            break;
        default:
            inspiration(TidVol);
            delay(15);
            cycleEndTime = expiration(TidVol, IE_ratio);
            break;
    }
    return cycleEndTime;
}

// =======================
// SIMV MODE Function
// =======================

void simv_mode()
{
  uint32_t cycleEndTime;
  bool firstRun = true;

  while(set_mode == "simv")
  {
      // Fetch all potentiometer values
      IE_ratio = map(analogRead(potpinIE_ratio), 0, 1023, 1.00, 4.00);    
      TidVol = map(analogRead(potpinTidVol), 0, 1023, 40.00, 90.00);     
      BPM = map(analogRead(potpinBPM), 0, 1023, 8.00, 30.00);     
      separation = (60/BPM - (1+IE_ratio));
      if (separation < 0)
    { IE_ratio = 60/BPM - 1;
        separation = 60/BPM - (1+IE_ratio);}
      // Fetch pressure sensor values
      pressure_mask = (map(analogRead(pinguage_mask), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92;    
      pressure_diff = (map(analogRead(pinguage_diff), 0, 1023, 0, 1023) - pressureDiffSensorOffset)*10.1972/92;
      pressure_expiration = (map(analogRead(pinguage_expiration), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92;
      // Initiate the cycle
      if(firstRun)
      {
        pressure_mask = average_pressure_mask();
        cycleEndTime = simv_logic(pressure_mask, TidVol, IE_ratio);
        firstRun = false;
      }

      // ========= Identify trigger and initiate the cycle =============
      if(millis() - cycleEndTime >= (uint32_t)separation*1000 || average_pressure_mask() < -1)
      {
        pressure_mask = average_pressure_mask();
        cycleEndTime = simv_logic(pressure_mask, TidVol, IE_ratio);
      }
      print_to_screen(BPM, IE_ratio, pressure_mask, pressure_expiration, TidVol, pressure_diff);
  }
}

// =======================
// Average Pressure Function
// =======================

float average_pressure_mask()
{
    pressure_mask = 0;
    for(int i=0;i<5;i++)
    {
        pressure_mask = pressure_mask + (map(analogRead(pinguage_mask), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92;
        delay(3);
    }
    return (pressure_mask/5);
}


// =======================
// Inspiration Function
// =======================
void inspiration(float TidVol)
{ int pos;
  for(pos = 30; pos <= TidVol+30; pos += 1) // goes from 0 degrees to 180 degrees
  {                                  // in steps of 1 degree
    servoright.write(pos);
    delay(1000/TidVol);                       

    // ============ Update pressure values =========
    pressure_mask = (map(analogRead(pinguage_mask), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92;    
    pressure_diff = (map(analogRead(pinguage_diff), 0, 1023, 0, 1023) - pressureDiffSensorOffset)*10.1972/92; 
    pressure_expiration = (map(analogRead(pinguage_expiration), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92; 
    
    Serial.println(pressure_mask);
    //Serial.println(pressure_diff);
    //Serial.println(pressure_expiration);
  }

}

// =====================
// Expiration Function
// =====================

uint32_t expiration(float TidVol, float IE_ratio)
{
  for(pos = TidVol+30; pos>=30; pos-=1)     // goes from 180 degrees to 0 degrees
  {                                
    servoright.write(pos);
    delay(1000*IE_ratio/TidVol);                       
    // ============ Update pressure values =========
    pressure_mask = (map(analogRead(pinguage_mask), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92;    
    pressure_diff = (map(analogRead(pinguage_diff), 0, 1023, 0, 1023) - pressureDiffSensorOffset)*10.1972/92; 
    pressure_expiration = (map(analogRead(pinguage_expiration), 0, 1023, 0, 1023) - guageSensorOffset)*10.1972/92; 
    
    Serial.println(pressure_mask);
    //Serial.println(pressure_diff);
    //Serial.println(pressure_expiration);
  }  
  return millis();
}

// =====================
// Transmit to DB
// =====================




// =====================
// Print to Screen
// =====================
void print_to_screen(float BPM, float IE_ratio, float pressure_mask, float pressure_expiration, float TidVol, float pressure_diff){
  static char ps_diff[6];
  dtostrf(pressure_diff, 6, 2, ps_diff);
  static char ps_exp[6];
  dtostrf(pressure_expiration, 6, 2, ps_exp);
  static char ps_mask[6];
  dtostrf(pressure_expiration, 6, 2, ps_exp);
  t3.setText(ps_diff);
  t4.setText(ps_exp);
  t5.setText(ps_mask);
  static char ie_ratio[6];
  dtostrf(IE_ratio, 6, 2, ie_ratio);
  t_ie_ratio.setText(ie_ratio);
  static char bpm[6];
  dtostrf(BPM, 6, 2, bpm);
  t_bpm.setText(bpm);
  static char tidvol[6];
  dtostrf(TidVol, 6, 2, tidvol);
  t_tidvol.setText(tidvol);
}
