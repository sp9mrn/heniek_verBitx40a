/*
This entire program is taken from Jason Mildrum, NT7S and Przemek Sadowski, SQ9NJE.
http://nt7s.com/
http://sq9nje.pl/
http://ak2b.blogspot.com/
Based on Heniek By SP9MRN, Złomek by SQ9MDD and Raduino code
Pieces from PA3FAT (Ron). 
Compatibile with Raduino Hardware.

For non commertial Ham Radio - COPYLEFT.
Future versions must stay open and copyleft...
MAc SP9MRN
19 03 2017
01.2018
*/

#include <Rotary.h>
#include <si5351.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <LcdBarGraph.h>


#define ENCODER_A    3                      // Encoder pin A
#define ENCODER_B    2                      // Encoder pin B
#define StepPin      4  // now free pin
#define OUTpttPin    5  //ptt output to relays
#define INpttPin     6  //input from ptt switch
#define SmeterPin    A0 // s meter sense

// pin definitions for 1602 LCD
#define LCD_RS       8
#define LCD_E        9
#define LCD_D4      10
#define LCD_D5      11
#define LCD_D6      12
#define LCD_D7      13

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);       // LCD - pin assignement in
Si5351 si5351;
Rotary r = Rotary(ENCODER_A, ENCODER_B);
//SI5351_FREQ_MULT=100ULL

long IF_freq = 12000000;
int SSB_shift = 1500;
int CW_shift = 800;

volatile uint32_t vfo_freq = 710000000ULL / SI5351_FREQ_MULT; //start freq - change to suit
int STEP = 10;	//start step size - change to suit
boolean TUNE_FLAG = 0;
char buffor[] = "              ";            //lcd buffer.

// Variables for s-meter
unsigned long S_Timer = 0;
unsigned long s_metr_update_time = 0;
unsigned int s_metr_update_interval=50;
int S_value;

//variables for tuning step button
boolean buttonStep_lastState = 0;
boolean buttonStep_State = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

boolean mustShowStep = false;     // var to show the step instead the bargraph
boolean tx =           false;

//structures to save bands, modes and VFO's
typedef struct {
  long lower_limit;
  long upper_limit;
  long mid;
} band_t;

typedef struct {
  byte band;
  long freq[2];
  byte mode[2];
} vfo_t;

band_t bands[2] = {
  {3500000, 3800000, 3700000},
  {7000000, 7200000, 7100000}};
// data - freq(Hz), tuning step 10Hz


// mode 0-LSB,1-USB, 2-AM, 3-CW, 4-RTTY
byte modes[2]={0, 0};
 
vfo_t vfo[2];
byte active_vfo = 0;


/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result == DIR_CW)
    set_frequency(1);
  else if (result == DIR_CCW)
    set_frequency(-1);
}
/**************************************/
/* Change the frequency and setting tune_flag              */
/**************************************/
void set_frequency(short dir)
{
  if (dir == 1)
    vfo_freq += STEP;
  if (dir == -1)
    vfo_freq -= STEP;
  TUNE_FLAG = 1;
}



//****************SETUP***************************
void setup()
{
  Serial.begin(19200);
  lcd.begin(16, 2);                                                    // Initialize and clear the LCD
  lcd.clear();
  Wire.begin();
 pinMode(OUTpttPin, OUTPUT);  //testowe - wyjście --> 1=TX_on
 pinMode(INpttPin,INPUT_PULLUP);
 pinMode(StepPin,INPUT_PULLUP);
 
//********* VFO initial setup*********
  vfo[0].band = 1;  //początkowe ustawienia pasma po starcie systemu
  vfo[1].band = 1;
    
  for(byte i=0; i<2;i++)
  {
    vfo[0].freq[i] = bands[i].mid;
    vfo[1].freq[i] = bands[i].mid;
    vfo[0].mode[i] = modes[i];
    vfo[1].mode[i] = modes[i];
  }

  //si5351.set_correction(140); //**mine. There is a calibration sketch in File/Examples/si5351Arduino-Jason
  //where you can determine the correction by using the serial monitor.
  //initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0,0); // 27000000 for 27MHz, 0 for 25MHz Xtal
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  // Set CLK0 to output the starting "vfo" frequency as set above by vfo = ?

//vfo
  si5351.set_freq(vfo[1].freq[1]+IF_freq+SSB_shift,  SI5351_CLK2);
  si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_2MA); //you can set this to 2MA, 4MA, 6MA or 8MA
  
//bfo
  si5351.set_freq( IF_freq+SSB_shift,  SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_2MA); //you can set this to 2MA, 4MA, 6MA or 8MA

  
 //*****************deklaracja przerwan encodera (encoder interrupts)*************
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();  
  //****************koniec enkodera (end of declaration)**************************

  
  display_frequency();  // Update the display freq & STEP
  printStep();  
  
}
//***************end of setup**************


//***************main loop****************************
void loop(){
  smeter();

//**************Tuning step****************
  buttonStep_State = digitalRead(StepPin);
  if (buttonStep_State != buttonStep_lastState) {
    if (buttonStep_State == LOW) {set_step(); } 
       delay(50);
    }
  buttonStep_lastState = buttonStep_State; 
  
//*************update frequency************
  if (TUNE_FLAG)
  {
    display_frequency();


    
    si5351.set_freq(vfo[1].freq[1]+IF_freq+SSB_shift, SI5351_CLK2);
    //you can also subtract the bfo to suit your needs
    Serial.print("bfo : ");Serial.println(IF_freq+SSB_shift);
    Serial.print("vfo : ");Serial.println(vfo[1].freq[1]);
    Serial.print("dis : ");Serial.println(vfo[1].freq[1]+IF_freq+SSB_shift);
        
    TUNE_FLAG = 0;
  }
}
//**********************end of main loop************






void smeter(){
S_Timer = millis();
       if ((S_Timer - s_metr_update_time) >= s_metr_update_interval) { // czy już wyświetlić?
         analogReference(INTERNAL);
         S_value = analogRead(SmeterPin);
         analogReference(DEFAULT);       
         Serial.println(S_value);
         printSmeter();
         s_metr_update_time = S_Timer;
      } 
 }



