/*
roep show_number(<nodeID>) aan in de setup van de main code.
*/

#define LED1 15
#define LED2 4

void show_number(int id){
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  Serial.println("begin met juiste led aan zetten");
  if(id==1){
    digitalWrite(LED1, HIGH);
  }
  else if(id == 2){
    Serial.println("LED2 gaat aan");
    digitalWrite(LED2, HIGH);
  }
  else if(id == 3){
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  }
}
