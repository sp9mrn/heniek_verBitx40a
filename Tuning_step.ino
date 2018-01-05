/********************Tuning step*******************
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
*******************************/
