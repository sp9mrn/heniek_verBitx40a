



LcdBarGraph lbg0(&lcd, 6, 1, 1); // -- First line at column 0, rząd dolny




/**************************************/
/* Displays the frequency change step */

/**************************************
void printStep()
{
  lcd.setCursor(1, 0);
  switch (STEP)
  {
    case 10:
      lcd.print(" 10");
      break;
    case 100:
      lcd.print("100");
      break;
    case 1000:
      lcd.print(" 1k");
      break;
    case 10000:
      lcd.print("10k");
      break;
  }
}

****************************************/

 void printSmeter() {
   lbg0.drawValue( S_value, 512);
}

/*****freq_LCD 1602******by sq9mdd***************************/
void display_frequency(){
  long mhz = vfo[1].freq[1] / 1000000;                        // MHz
  long khz = vfo[1].freq[1] % 1000000;                        // kHz
  khz = khz / 1000;                                     // kHz wihout Hz
  long hz = vfo[1].freq[1] % 1000;                            // Hz
  hz = hz / 10;                                         // tens Hz
  sprintf(buffor,"%1lu.%03lu.%02lu",mhz,khz,hz);        // create LCD buffer
  lcd.setCursor(7,0);                                    
  lcd.print(buffor);                                      
}
