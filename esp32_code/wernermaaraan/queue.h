/*
gebruik: queue_setup() om de queue aan te maken. doe dit voor dat je de andere functies gebruikt.
gebruik: queue_insert(<String>) om iets in de queue te stoppen
gebruik: queue_get() om de eerste item uit de queue te halen. dit item wordt dan ook verwijderd
*/

#define MAX_QUEUE_SIZE 15
QueueHandle_t queue; //declear the queue handle
SemaphoreHandle_t queue_Semaphore;
bool queue_check(String data);//prototype function

void queue_insert(String data){
  if(xSemaphoreTake(queue_Semaphore, (TickType_t) 10) == pdTRUE){

    if(queue_check(data)){
      Serial.println("Zit al in de queue");
      xSemaphoreGive(queue_Semaphore);
      return;
    }

    if (uxQueueSpacesAvailable(queue) == 0) {
        char *oldest_data;
        // Verwijder de oudste tijdstempel en zegt welke het is
        xQueueReceive(queue, &oldest_data, 0);
        Serial.print("Queue full!. The deleted data is");
        Serial.println(oldest_data);
        free(oldest_data);
        
    } 
  //convert the string to char
    const char* insert_data = data.c_str();

    // Allocate memory for new_data and copy temp into it
    char *new_data = (char*) malloc((strlen(insert_data ) + 1) * sizeof(char));
    strcpy(new_data, insert_data );
    Serial.println("stop data in queue");
    Serial.println(uxQueueSpacesAvailable(queue));
    xQueueSendToBack(queue, &new_data, 0);
    Serial.println("in queue gestopt");
    xSemaphoreGive(queue_Semaphore);
  }
}

char* queue_get(){
  char *data = "queue is niet vrij"; //als de queue vrij is komt in deze variabele de data in de queue

  if(xSemaphoreTake(queue_Semaphore, (TickType_t) 10) == pdTRUE){
    if (uxQueueSpacesAvailable(queue) == MAX_QUEUE_SIZE){

      xSemaphoreGive(queue_Semaphore);
      return "queue is empty";
    }

    xQueueReceive(queue, &data, 0);

    xSemaphoreGive(queue_Semaphore);
  }
  return data; 
}


bool queue_check(String data){
  if(uxQueueSpacesAvailable(queue) == MAX_QUEUE_SIZE){
  Serial.println("queue is empty");
  return false;
  }
  bool results = false;
  const char *check_data = data.c_str(); //turns the String to char
  int lenght = MAX_QUEUE_SIZE - int(uxQueueSpacesAvailable(queue));
  int count = 0;

  Serial.print("The lenght of the queue:");
  Serial.println(lenght);

  while(count != lenght){
    char *queue_data;
    xQueueReceive(queue, &queue_data, 0);
    Serial.println("got an item to check from queue");
    Serial.print("check_data:");
    Serial.println(check_data);
    Serial.print("queue_data:");
    Serial.println(queue_data);
    if(String(queue_data) == String(check_data)){
      Serial.print(check_data);
      Serial.println(" Zit al in queue");
      results = true;
    }
      xQueueSendToBack(queue, &queue_data, 0);
      count ++;
  }
  return results;
}

void queue_setup(){
  queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(char *));
  assert(queue);
  queue_Semaphore = xSemaphoreCreateBinary();
  // handle = binary semaphore.........
  if(queue_Semaphore != NULL){
  // free binarys semaphore........
    xSemaphoreGive(queue_Semaphore);
  }
  
}