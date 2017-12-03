#include <QueueList.h>
#include <StackArray.h>
#include <Servo.h>

#define LOG_OUT 1 // use the log output function
#define FFT_N 128 // set to 64 point fft

#include <FFT.h> // include the library
int treasures = B000;


#define ROWS 4
#define COLS 5

#define LeftWall 3
#define RightWall 4
#define OutLineLeft 2
#define OutLineRight 7


int sensorPin = A5;

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

typedef struct{
  int x; int y;
} Square;

typedef struct{
  int n; int w; int s; int e;
} Square_wall;

typedef struct{
  int dist;
  Square prev;
} Square_dist;

//light sensors (analog)
//(>950 : black line ; <900 : white space)
int inLeft;
int inRight;

//digital line sensors
//0 = black; 1 = white
int outLeft;
int outRight;

//  //analog wall sensor ->
//wall: > 250; no wall: < 250
int frontwall;

//digital wall sensors ->
//0 = wall; 1 = no wall
int wallL;
int wallR;

int goback;

char orient;



Servo leftservo;
Servo rightservo;

Square_wall maze[ROWS][COLS];


Square start;

StackArray <Square> frontier;
StackArray <Square> path;

RF24 radio(9,10);
const uint64_t pipes[2] = { 0x0000000006LL, 0x0000000007LL };
// Square current;
// int visitedSize;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  radio_init();
  leftservo.attach(5);
  rightservo.attach(6);
  stp();
  pinMode(OutLineLeft, INPUT);
  pinMode(LeftWall, INPUT);
  pinMode(RightWall, INPUT);
  pinMode(OutLineRight, INPUT);
  pinMode(8, INPUT);
  start.x = 3;
  start.y = 0;
  frontier.push(start);
  orient = 1;
  initialize_walls();
}

bool check_mic(){
//   int reset1 = ADCSRA;
//  int reset2 = ADMUX;
//  int reset3 = DIDR0;
//  int reset4 = TIMSK0;
//  //TIMSK0 = 0; // turn off timer0 for lower jitter
//  ADCSRA = 0xe7; // set the adc to free running mode
//  ADMUX = 0x45; // use adc3
//  //DIDR0 = 0x06; // turn off the digital input for adc3
  
  //while(1) { // reduces jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 256 ; i += 2) { // save 64 samples
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf7; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fft_input[i] = k; // put real data into even bins
      fft_input[i+1] = 0; // set odd bins to 0
    }
    fft_window(); // window the data for better frequency response
    fft_reorder(); // reorder the data before doing the fft
    fft_run(); // process the data in the fft
    fft_mag_log(); // take the output of the fft
    sei();
//    Serial.println("start here");
    //Serial.println(ADMUX);
//    for (byte i = 0 ; i < FFT_N/2 ; i++) { 
     // Serial.println(fft_log_out[10]); // send out the data      
//    }
    
    if (fft_log_out[5] > 90) {         //660Hz
      return true;
      //digitalWrite(8, HIGH);
    }      
    else {
      return false;
      //digitalWrite(8, LOW);
    }

  

     //stp();
     //Serial.println(fft_log_out[28]); // send out the data
  
//      //Serial.println((int)treasures); // send out the data
//     //TIMSK0 = reset4; // turn off timer0 for lower jitter
//  ADCSRA = reset1; // set the adc to free running mode
//  ADMUX = reset2; // use adc3
//  //DIDR0 = reset3;
   
}

void radio_init() {
  printf_begin();
  radio.begin();
  radio.setRetries(15,15);
  radio.setAutoAck(true);
  // set the channel
  radio.setChannel(0x50);
  // set the power
  // RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM, and RF24_PA_HIGH=0dBm.
  radio.setPALevel(RF24_PA_MIN);
  //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.setDataRate(RF24_250KBPS);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(16);
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  radio.startListening();

  //radio.printDetails();
}

// the loop routine runs over and over again forever:
void loop() {
  while(1){
    find_treasures();
  }
  bool start_signal = false;
  int i = 0;

  int reset1 = ADCSRA;
  int reset2 = ADMUX;
  int reset3 = DIDR0;
  int reset4 = TIMSK0;
  //TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe7; // set the adc to free running mode
  ADMUX = 0x45; // use adc3
  DIDR0 = 0x06; // turn off the digital input for adc3
  


  Serial.println("looking for 660Hz");
  while ((!start_signal) || !(i == 10)){
//while(1){
    
    Serial.println("still looking");
    delay(10);
    start_signal = check_mic();
    //Serial.println(start_signal);
    //Serial.println(i);
    if (start_signal) {
      i++;
    }
    else{
       if (i == 0) {
        i = 0;
       }
       else {
        i--;
       }
    }
    if(digitalRead(8)){
      start_signal = true;
      i = 10;
    }
    
//            check_mem(); 
//        Serial.write("heapptr: "); Serial.print((int)heapptr);
//        Serial.write(", stackptr: "); Serial.println((int)stackptr);

    
  }
  //TIMSK0 = reset4; // turn off timer0 for lower jitter
  ADCSRA = reset1; // set the adc to free running mode
  ADMUX = reset2; // use adc3
  DIDR0 = reset3;
  Serial.println("Heard 660Hz");
  dfs();

}

/********************************HELPER FUNCTIONS******************************/
void dfs(){
  int visitedSize = 0;
  //StackArray <Square> path;
  StackArray <Square> backto;
  Square visited[20];
  Square current;
  Square next;
  char done;
  char walls;
  //current = start;
  char c_coor [] = {0,0};
  char n_coor [] = {0,0};
  //followline();
  int newsquare;

  while(!frontier.isEmpty()){
    readSensor();
    if (outLeft == 0 && outRight == 0){ //while both outer sensors see black
      treasures = B000;
      stp();
      find_treasures();
     
    //if we decide to start at (0,0) add code to check if we are at an intersection
    if(goback == 0){
      current = frontier.pop();
      Serial.print("CURRENT:   ");
      Serial.print(current.x);
      Serial.println(current.y);
      c_coor[0] = current.x;
      c_coor[1] = current.y;

      if (!isMember(current, visited, visitedSize)){
        visited[visitedSize] = current;
        visitedSize ++;
        newsquare = 1;
      }
      else{
        newsquare = 0;
      }
      int length = frontier.count();
      if(wallL == 1){ //no wall on left
        if(orient == 3){
          next.x = current.x + 1;
          next.y = current.y;
        }
        else if(orient == 2){
          next.x = current.x;
          next.y = current.y + 1;
        }
        else if(orient == 1){
          next.x = current.x - 1;
          next.y = current.y;
        }
        else if(orient == 0){
          next.x = current.x;
          next.y = current.y - 1;
        }
        if(!isMember(next, visited, visitedSize)){
          frontier.push(next);
        }
      }
      else{
        if(orient == 3){
          maze[current.x][current.y].s = 1;
        }
        else if(orient == 2){
          maze[current.x][current.y].e = 1;
        }
        else if(orient == 1){
          maze[current.x][current.y].n = 1;
        }
        else if(orient == 0){
          maze[current.x][current.y].w = 1;
        }
      }
      if(wallR == 1){ //no wall on right
        if(orient == 3){
          next.x = current.x - 1;
          next.y = current.y;
        }
        else if(orient == 2){
          next.x = current.x;
          next.y = current.y - 1;
        }
        else if(orient == 1){
          next.x = current.x + 1;
          next.y = current.y;
        }
        else if(orient == 0){
          next.x = current.x;
          next.y = current.y + 1;
        }
        if(!isMember(next, visited, visitedSize)){
          frontier.push(next);
        }
      }
      else{
        if(orient == 3){
          maze[current.x][current.y].n = 1;
        }
        else if(orient == 2){
          maze[current.x][current.y].w = 1;
        }
        else if(orient == 1){
          maze[current.x][current.y].s = 1;
        }
        else if(orient == 0){
          maze[current.x][current.y].e = 1;
        }
      }
      if(frontwall < 170){ //no wall in front
        if(orient == 3){
          next.x = current.x;
          next.y = current.y - 1;
        }
        else if(orient == 2){
          next.x = current.x + 1;
          next.y = current.y;
        }
        else if(orient == 1){
          next.x = current.x;
          next.y = current.y + 1;
        }
        else if(orient == 0){
          next.x = current.x - 1;
          next.y = current.y;
        }
        if(!isMember(next, visited, visitedSize)){
          frontier.push(next);
        }
      }
      else{
        if(orient == 3){
          maze[current.x][current.y].w = 1;
        }
        else if(orient == 2){
          maze[current.x][current.y].s = 1;
        }
        else if(orient == 1){
          maze[current.x][current.y].e = 1;
        }
        else if(orient == 0){
          maze[current.x][current.y].n = 1;
        }
      }

      done = 0;
      if (frontier.isEmpty()) done = 1;
      walls = B0000;
      walls = checkWalls (current.x, current.y);
//      Serial.println("stuck");
//      while (! radio_send(done, treasures, walls, orient, current.x, current.y)){
        radio_send(done, treasures, walls, orient, current.x, current.y);
//      }  
  
      
      if(frontier.count()>length){
        n_coor[0] = frontier.peek().x;
        n_coor[1] = frontier.peek().y;
        if((frontier.count() - length) == 2){
            backto.push(current);
        }
        else if((frontier.count() - length) == 3){
            backto.push(current);
            backto.push(current);
        }
        goback = 0;
        orient = reorient(c_coor, n_coor, orient);
        treasures = B000;
        stp();
        find_treasures();
        
      }
      else{
        goback = 1;
      }

       done = 0;
       walls = B0000;
       walls = checkWalls (current.x, current.y);
//       while (! radio_send(done, treasures, walls, orient, current.x, current.y)){
       radio_send(done, treasures, walls, orient, current.x, current.y);
//       }  
 
      


    }
    else{
      Square back;
      Serial.print("newSquare:  ");
      Serial.println(newsquare);
      next = frontier.peek();
      if (!isMember(next, visited, visitedSize)){
        if(newsquare == 1){
          back = backto.pop();
          frontier.pop();
          shortest_path(current,back,maze); 
        }
        Square tmp = path.pop();
        while (!squareCompare(tmp,back)){
          n_coor[0] = tmp.x;
          n_coor[1] = tmp.y;
          done = 0;
          treasures = B000;
          stp();
          find_treasures();
          walls = B0000;
          walls = checkWalls (c_coor[0], c_coor[1]);
////          while (! radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1])){
//            radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1]);
////          }  
    
          radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1]);
          orient = reorient(c_coor, n_coor, orient);
          treasures = B000;
          stp();
          find_treasures();
          done = 0;
          walls = B0000;
          walls = checkWalls (c_coor[0], c_coor[1]);
//          while (! radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1])){
            radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1]);
//          }  
    
          followline();
          c_coor[0] = n_coor[0];
          c_coor[1] = n_coor[1];
          tmp = path.pop();
        }
        n_coor[0] = tmp.x;
        n_coor[1] = tmp.y;
        done = 0;
        treasures = B000;
        stp();
        find_treasures();
        walls = B0000;
        walls = checkWalls (c_coor[0], c_coor[1]);
//        while (! radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1])){
            radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1]);
//          }  
    
        orient = reorient(c_coor, n_coor, orient);
        treasures = B000;
        stp();
        find_treasures();
        done = 0;
        walls = B0000;
        walls = checkWalls (c_coor[0], c_coor[1]);
//        while (! radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1])){
            radio_send(done, treasures, walls, orient, c_coor[0], c_coor[1]);
//          }  
    
        frontier.push(tmp);
        goback = 0;
      }
      else{
//        Serial.print("NEXT on front:   ");
//          Serial.print(frontier.peek().x);
//          Serial.println(frontier.peek().y);
        frontier.pop();
        backto.pop();
//        Serial.print("Frontier size:  ");
//        Serial.println(frontier.count());
//        Serial.print("NEXT on front:   ");
////          Serial.print(frontier.peek().x);
////          Serial.println(frontier.peek().y);
////        Serial.print("      ");
//        Serial.print("backto size:  ");
//        Serial.println(backto.count());
      }
      
      
    }
    followline();
    }
    else{
    followline();
    //stp();
  }
  }
    Serial.print("size");
  Serial.println(frontier.count());
  Serial.println("DONE");
  //digitalWrite(8, HIGH);
  done = 1;
//  treasures = 0;
//  walls = 0;
//  orient = 0;
//  current.x = 0;
//  current.y = 0;
// while (! radio_send(done, treasures, walls, orient, current.x, current.y)){
      radio_send(done, treasures, walls, orient, current.x, current.y);
//    }  
  
  while(frontier.isEmpty()){
    stp();
  }

}

void shortest_path (Square a, Square b, Square_wall maze[ROWS][COLS]){
  QueueList <Square> queue;
  Square blank;
  blank.x = 1287;
  blank.y = 1287;
  Square visited [20];
  int visitedSize = 0;
  Square_dist best_path [ROWS][COLS];
  //initialize best_path array
  for (int i = 0; i < ROWS; i++){
    for (int j = 0; j < COLS; j++){
      best_path[i][j].dist = 21;
      best_path[i][j].prev = blank;
    }
  }
  queue.push(a);
  bool stop = true;
  Square current;
  Square next;
  best_path[a.x][a.y].dist = 0;
  Serial.println("stuck in first while");
  while ((!queue.isEmpty())||stop){
    current = queue.pop();
    if (!isMember(current, visited, visitedSize)){
      visited[visitedSize] = current;
      visitedSize ++;
    }
    if (squareCompare(b, current)){
      stop = false;
    }
    if(maze[current.x][current.y].n == 0){
      int temp = best_path[current.x][current.y].dist + 1;
      next.x = current.x - 1;
      next.y = current.y;
      if (best_path[next.x][next.y].dist > temp){
        best_path[next.x][next.y].dist = temp;
        best_path[next.x][next.y].prev = current;
      }
      if(!isMember(next, visited, visitedSize)){
        queue.push(next);
      }
    }
    if(maze[current.x][current.y].w == 0){
      int temp = best_path[current.x][current.y].dist + 1;
      next.x = current.x;
      next.y = current.y - 1;
      if (best_path[next.x][next.y].dist > temp){
        best_path[next.x][next.y].dist = temp;
        best_path[next.x][next.y].prev = current;
      }
      if(!isMember(next, visited, visitedSize)){
        queue.push(next);
      }
    }
    if(maze[current.x][current.y].s == 0){
      int temp = best_path[current.x][current.y].dist + 1;
      next.x = current.x + 1;
      next.y = current.y;
      if (best_path[next.x][next.y].dist > temp){
        best_path[next.x][next.y].dist = temp;
        best_path[next.x][next.y].prev = current;
      }
      if(!isMember(next, visited, visitedSize)){
        queue.push(next);
      }
    }
    if(maze[current.x][current.y].e == 0){
      int temp = best_path[current.x][current.y].dist + 1;
      next.x = current.x;
      next.y = current.y + 1;
      if (best_path[next.x][next.y].dist > temp){
        best_path[next.x][next.y].dist = temp;
        best_path[next.x][next.y].prev = current;
      }
      if(!isMember(next, visited, visitedSize)){
        queue.push(next);
      }

    }

  }
  Serial.println("stuck in second while");
  //make path back.
  current = b;
  while(!squareCompare(a, current)){
    path.push(current);
    current = best_path[current.x][current.y].prev;
    Serial.print("current:   ");
    Serial.print(path.peek().x);
    Serial.println(path.peek().y);
  }

}

void initialize_walls(){
  for (int i = 0; i < ROWS; i++){
    for (int j = 0; j < COLS; j++){
      maze[i][j].n = 0;
      maze[i][j].s = 0;
      maze[i][j].e = 0;
      maze[i][j].w = 0;
      if (i == 0){
        maze[i][j].n = 1;
      }

      if (i == 3){
        maze[i][j].s = 1;
      }

      if (j == 0){
        maze[i][j].w = 1;
      }

      if (j == 4){
        maze[i][j].e = 1;
      }



    }
  }
}

char checkWalls (int x, int y) {
  char temp = B0000;
  if(maze[x][y].n == 1)
      {
        temp = temp + 8;
      }
      if(maze[x][y].e == 1)
      {
        temp = temp + 4;
      }
      if(maze[x][y].s == 1)
      {
        temp = temp + 2;
      }
      if(maze[x][y].w == 1)
      {
        temp = temp + 1;
      }
   return temp;
}

bool isMember (Square a, Square visit[], int sizeV){
  for(int i = 0; i < sizeV; i ++){
    if (squareCompare(a,visit[i])){
      return true;
    }
  }
  return false;
}

bool squareCompare (Square a, Square b){
  if(a.x == b.x){
    if(a.y == b.y){
      return true;
    }
  }
  return false;
}


char reorient(char current[], char next[], char curr_o) {
  char next_o = 0;
  char diff[] = {0, 0};
  diff[0] = next[0]-current[0];
  diff[1] = next[1]-current[1];

  if (diff[0] == -1){ //north
    next_o = 0;
  }
  else if (diff[0] == 1){ // south
    next_o = 2;
  }
  else if (diff[1] == -1){ // west
    next_o = 3;
  }
  else if (diff[1] == 1){ // east
    next_o = 1;
  }
  if (next_o - curr_o == 0){
    //straight
    forward();
    delay(250);

  }
  else if (next_o - curr_o == 1 || next_o - curr_o == -3){
    //turn right
    right();
    delay(800);


  }
  else if (abs(next_o - curr_o) == 2){
    char done = 0;
    char walls;
    walls = checkWalls(current[0],current[1]);
    //flip
//    right();
    flip();
    delay(600);
    stp();
    treasures = B000;
    find_treasures();
////    while (! radio_send(done, treasures, walls, next_o - 1, current[0], current[1])){
//      radio_send(done, treasures, walls, next_o, current[0], current[1]);
////    }
    //radio_send(done, treasures, walls, orient, current[0], current[1]);
    flip();
    delay(700);  
    stp();
    treasures = B000;
    find_treasures();
//    while (! radio_send(done, treasures, walls, next_o, current[0], current[1])){
      radio_send(done, treasures, walls, next_o, current[0], current[1]);
//    }  

//flip
//    flip();
//    delay(1250);


  }
  else if (next_o - curr_o == -1 || next_o - curr_o == 3){
    //turn left
    left();
    delay(800);
  }

  curr_o = next_o;
  //  Serial.println((int) curr_o);

  stp();
  
  return curr_o;
}

void find_treasures() {
  int reset1 = ADCSRA;
  int reset2 = ADMUX;
  int reset3 = DIDR0;
  int reset4 = TIMSK0;
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x43; // use adc3
  DIDR0 = 0x04; // turn off the digital input for adc3

  for(int i = 0; i < 4; i++){
  
//  //while(1) { // reduces jitter
//    if (ADMUX == 0x43){
//      ADMUX = 0x44;       //a4
//      DIDR0 = 0x05;
//    }
//    else if (ADMUX == 0x44){
//      ADMUX = 0x43;       //a3
//      DIDR0 = 0x04;
//    }
//  
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 256 ; i += 2) { // save 64 samples
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fft_input[i] = k; // put real data into even bins
      fft_input[i+1] = 0; // set odd bins to 0
    }
    fft_window(); // window the data for better frequency response
    fft_reorder(); // reorder the data before doing the fft
    fft_run(); // process the data in the fft
    fft_mag_log(); // take the output of the fft
    sei();
    Serial.println("start here");
    Serial.println(ADMUX);
    for (byte i = 0 ; i < FFT_N/2 ; i++) { 
      Serial.println(fft_log_out[42]); // send out the data    
        
    }
    
    if (fft_log_out[24] > 140) {         //7kHz
      treasures |= 1;
////       digitalWrite(8, HIGH);
//      delay(2500);
//      digitalWrite(8, LOW);
    }
    else if (fft_log_out[21] > 130) {    //12kHz      
      treasures |= 1 << 1;
//       digitalWrite(8, HIGH);
//      delay(2500);
//      digitalWrite(8, LOW);
     
    
    }
    else if (fft_log_out[28] > 120) {    //17kHz
      treasures |= 1 << 2;
//      digitalWrite(8, HIGH);
//      delay(2500);
//      digitalWrite(8, LOW);
      //stp();
    
    }        
    else {
//      digitalWrite(7, LOW);
    }

  }

     //stp();
     //Serial.println(fft_log_out[28]); // send out the data
  
      //Serial.println((int)treasures); // send out the data
     TIMSK0 = reset4; // turn off timer0 for lower jitter
     ADCSRA = reset1; // set the adc to free running mode
     ADMUX = reset2; // use adc3
     DIDR0 = reset3;
  }

bool radio_send(char done, char treasures, char walls, char curr_o, char x_coord, char y_coord){
    // First, stop listening so we can talk.
    radio.stopListening();
    // Take the time, and send it.  This will block until complete
    unsigned long time = millis();
//    Serial.print("walls: ");
//    Serial.print(String(bitRead(walls, 3)) + String(bitRead(walls, 2)) + String(bitRead(walls, 1)) + String(bitRead(walls, 0)));
//    Serial.print("      treasures: ");
//    Serial.println((int) treasures);
    int i=0;
    unsigned int new_data;
    new_data = 1 << 15 | done << 14 | treasures << 11 | walls << 7 | curr_o << 5 | y_coord << 2 | x_coord;
    
    // * | D | T T T | W W W W | O O | Y Y Y | X X
    
//    printf("Now sending new map data\n");
    bool ok = radio.write( &new_data, sizeof(unsigned int) );

//    Serial.println("in while");
//    while((!ok)&&(i == 5)){
//      Serial.println(i);
//      ok = radio.write( &new_data, sizeof(unsigned int) );
//      delay(20);
//      i++;
//    }
//     Serial.println("finish while");
    if (ok)
      printf("ok... \n");
    else
      printf("failed.\n\r");

    delay(20);

    // Now, continue listening
    radio.startListening();
    

//    // Wait here until we get a response, or timeout (250ms)
//    unsigned long started_waiting_at = millis();
//    bool timeout = false;
//    while ( ! radio.available() && ! timeout )
//      if (millis() - started_waiting_at > 200 )
//        timeout = true;
//
//    // Describe the results
//    if ( timeout )
//    {
//      return false;
//      printf("Failed, response timed out.\n\r");
//    }
//    else
//    {
//      
//
//    unsigned long got_data_t;
//      radio.read( &got_data_t, sizeof(unsigned long) );
//      if (got_data_t == 1) return true;
//      // Spew it
//      //Serial.println("Got response " + got_string);
//      }

    // Try again 1s later
    //delay(250);
  }


void readSensor(){
  inLeft = analogRead(A0);
  inRight = analogRead(A1);
  frontwall = analogRead(A2);
  outLeft = digitalRead(OutLineLeft);
  outRight = digitalRead(OutLineRight);
  wallL = digitalRead(LeftWall);
  wallR = digitalRead(RightWall);
}

//turning functions
void forward() {
  leftservo.write(180);
  rightservo.write(0);
  readSensor();
}

void right() {
  leftservo.write(180);
  rightservo.write(95);
  readSensor();
}

void left() {
  leftservo.write(85);
  rightservo.write(0);
  readSensor();
}

void flip() {
  leftservo.write(180);
  rightservo.write(180);
  readSensor();
  //delay(time);
}


void stp() {
  leftservo.write(90);
  rightservo.write(90);
  readSensor();
}

void followline(){
  while (outLeft == 1 || outRight == 1){ //while both outer sensors see white
    readSensor();
    if (abs(inLeft-inRight)<70){ //if inner are similar
      forward();
    }
    else if (inLeft>inRight){ //if tilted left, correct
      leftservo.write(90);
      rightservo.write(0);
    }
    else if (inRight>inLeft){ //if tilted right
      leftservo.write(180);
      rightservo.write(90);
    }
    readSensor();
    }
  stp();
}
