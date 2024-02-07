/*This file contains the code to show the node number in binary.
  Call show_number(<nodeNumber>) in the setup function of the main code.*/

#define LED1 15
#define LED2 4

void show_number(int number){
  /*This function takes a number as input.
    If the number is between 0 and 3 it will turn on the correct LEDs to show the number in binary.*/
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  if(number == 1){
    digitalWrite(LED1, HIGH);
  }
  else if(number == 2){
    digitalWrite(LED2, HIGH);
  }
  else if(number == 3){
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
  }
}
