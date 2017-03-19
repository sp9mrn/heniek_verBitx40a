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

*/

#include <Rotary.h>
#include <si5351.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <LcdBarGraph.h>

#define F_MIN        100000000UL               // Lower frequency limit
#define F_MAX        5000000000UL

#define ENCODER_A    3                      // Encoder pin A
#define ENCODER_B    2                      // Encoder pin B
#define StepPin      4
#define LCD_RS		   8
#define LCD_E		     9
#define LCD_D4		  10
#define LCD_D5		  11
#define LCD_D6		  12
#define LCD_D7		  13
#define OUTpttPin    7
#define INpttPin     6
#define SmeterPin  A0


#define CALLSIGN "SP9MRN"
#define CALLSIGN_POS 4
#define BITX40   "BITX40 V4b"
#define BITX40_POS 3

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);       // LCD - pin assignement in
LcdBarGraph lbg0(&lcd, 6, 1, 1); // -- First line at column 0, rząd dolny

Si5351 si5351;
Rotary r = Rotary(ENCODER_A, ENCODER_B);
//volatile uint32_t LSB = 219850000ULL;
//volatile uint32_t USB = 220150000ULL;
volatile uint32_t LSB = 1199850000ULL;
volatile uint32_t USB = 1200150000ULL;
volatile uint32_t bfo = LSB; //start in lsb
//volatile uint32_t if_offset = 00003200UL; 
//These USB/LSB frequencies are added to or subtracted from the vfo frequency in the "Loop()"
//In this example my start frequency will be 7.100000 Mhz
volatile uint32_t vfo = 710000000ULL / SI5351_FREQ_MULT; //start freq - change to suit
volatile uint32_t STEP = 100;	//start step size - change to suit
boolean changed_f = 0;
String tbfo = "";

unsigned long S_Timer = 0;
unsigned long s_metr_update_time = 0;
unsigned int s_metr_update_interval=50;
int S_value;

boolean buttonStep_lastState = 0;
boolean buttonStep_State = 0;

boolean mustShowStep = false;     // var to show the step instead the bargraph
boolean tx =           false;


//------------------------------- Set Optional Features here --------------------------------------
//Remove comment (//) from the option you want to use. Pick only one
#define IF_Offset //Output is the display plus or minus the bfo frequency
//#define Direct_conversion //What you see on display is what you get
//#define FreqX4  //output is four times the display frequency
//--------------------------------------------------------------------------------------------------


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
/* Change the frequency               */
/* dir = 1    Increment               */
/* dir = -1   Decrement               */
/**************************************/
void set_frequency(short dir)
{
  if (dir == 1)
    vfo += STEP;
  if (dir == -1)
    vfo -= STEP;

  //    if(vfo > F_MAX)
  //      vfo = F_MAX;
  //    if(vfo < F_MIN)
  //      vfo = F_MIN;

  changed_f = 1;
}


/**************************************/
/* Displays the frequency             */
/**************************************/
void display_frequency()
{
  uint16_t f, g;

  lcd.setCursor(7, 0);
  f = vfo / 1000000; 	//variable is now vfo instead of 'frequency'
  if (f < 10)
    lcd.print(' ');
  lcd.print(f);
  //lcd.print('.');
  f = (vfo % 1000000) / 1000;
  if (f < 100)
    lcd.print('0');
  if (f < 10)
    lcd.print('0');
  lcd.print(f);
  lcd.print('.');
  f = vfo % 1000;
  if (f < 100)
    lcd.print('0');
  if (f < 10)
    lcd.print('0');
  lcd.print(f);
  //lcd.print("Hz");
  lcd.setCursor(0, 0);
  lcd.print(tbfo);
  //Serial.println(vfo + bfo);
  //Serial.println(tbfo);

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
 
// Show the CALL SIGN and BITX40 version on the display. Settings in define section

  lcd.setCursor(CALLSIGN_POS, 0);
  lcd.print(CALLSIGN);
  lcd.setCursor(BITX40_POS, 1); 
  lcd.print(BITX40); 
  delay(1000);
  lcd.clear();

  si5351.set_correction(140); //**mine. There is a calibration sketch in File/Examples/si5351Arduino-Jason
  //where you can determine the correction by using the serial monitor.

  //initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0); // 27000000 for 27MHz, 0 for 25MHz Xtal

  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  // Set CLK0 to output the starting "vfo" frequency as set above by vfo = ?

#ifdef IF_Offset
  uint32_t vco =bfo-(vfo * SI5351_FREQ_MULT);
  si5351.set_freq(vco, SI5351_CLK2);
  // seems not used : volatile uint32_t vfoT = (vfo * SI5351_FREQ_MULT) - bfo;
  tbfo = "LSB";
  // Set CLK2 to output bfo frequency
  si5351.set_freq( bfo, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0,SI5351_DRIVE_2MA); //you can set this to 2MA, 4MA, 6MA or 8MA
  //si5351.drive_strength(SI5351_CLK1,SI5351_DRIVE_2MA); //be careful though - measure into 50ohms
  //si5351.drive_strength(SI5351_CLK2,SI5351_DRIVE_2MA); //
#endif

#ifdef Direct_conversion
  si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_PLL_FIXED, SI5351_CLK2);
#endif

#ifdef FreqX4
  si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4, SI5351_PLL_FIXED, SI5351_CLK2);
#endif

  pinMode(StepPin, INPUT_PULLUP);
  
 //*****************deklaracja przerwan encodera (encoder interrupts)*************
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();  
  //****************koniec enkodera (end of declaration)**************************

  
  display_frequency();  // Update the display
  printStep();
  
}
//***************end of setup**************

//***************main loop*****************
void loop(){
  smeter();
 

//**************Tuning step*********************************
  buttonStep_State = digitalRead(StepPin);
  if (buttonStep_State != buttonStep_lastState) {
       if (buttonStep_State == LOW) {set_step(); } 
    }
  buttonStep_lastState = buttonStep_State; 
  


  // Update the display if the frequency has been changed
  if (changed_f)
  {
    display_frequency();

#ifdef IF_Offset
    uint32_t vco =bfo-(vfo * SI5351_FREQ_MULT);
    si5351.set_freq(vco, SI5351_CLK2);
    //you can also subtract the bfo to suit your needs
    //si5351.set_freq((vfo * SI5351_FREQ_MULT) + bfo  , SI5351_PLL_FIXED, SI5351_CLK0);
    Serial.print("bfo : ");Serial.println(bfo);
    Serial.print("vfo : ");Serial.println(vfo);
    Serial.print("mult : ");Serial.println(int(SI5351_FREQ_MULT));
    Serial.print("vco : ");Serial.println(vco);
    
    if (vfo >= 10000000ULL & tbfo != "USB")
    {
      bfo = USB;
      tbfo = "USB";
      si5351.set_freq( bfo, SI5351_CLK0);
      Serial.println("We've switched from LSB to USB");
    }
    else if (vfo < 10000000ULL & tbfo != "LSB")
    {
      bfo = LSB;
      tbfo = "LSB";
      si5351.set_freq( bfo, SI5351_CLK0);
      Serial.println("We've switched from USB to LSB");
    }
#endif

#ifdef Direct_conversion
    si5351.set_freq((vfo * SI5351_FREQ_MULT), SI5351_CLK0);
    tbfo = "";
#endif

#ifdef FreqX4
    si5351.set_freq((vfo * SI5351_FREQ_MULT) * 4, SI5351_CLK0);
    tbfo = "";
#endif

    changed_f = 0;
  }
}
//**********************end of main loop************

//********************Tuning step*******************
void set_step(){
  switch (STEP){ 
    case 10000:
      STEP=1000;
      break;
    case 10:
      STEP=10000;
      break;
    case 100:
      STEP=10;
      break;
    case 1000:
      STEP=100;
      break;
  }
 printStep(); 
}

/**************************************/
/* Displays the frequency change step */
/**************************************/
void printStep()
{
  lcd.setCursor(11, 1);
  switch (STEP)
  {
    case 10:
      lcd.print("   10");
      break;
    case 100:
      lcd.print("  100");
      break;
    case 1000:
      lcd.print("   1k");
      break;
    case 10000:
      lcd.print("  10k");
      break;
  }
}


void smeter(){
S_Timer = millis();
       if ((S_Timer - s_metr_update_time) >= s_metr_update_interval) { // czy już wyświetlić?
         analogReference(INTERNAL);
         S_value = analogRead(SmeterPin);
         analogReference(DEFAULT);       
         //Serial.println(S_value);
         printSmeter();
         s_metr_update_time = S_Timer;
      } 
 }

 void printSmeter() {
   lbg0.drawValue( S_value, 1024);
}

