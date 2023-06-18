#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "LedControl.h"
#include "binary.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define X_PIN A6
#define Y_PIN A7

#define DIN_PIN 16
#define CLK_PIN 15
#define CS_PIN 9
#define MAX_DEVICES 1

enum Direction {UP, DOWN, LEFT, RIGHT};
enum GameState {START, RUNNING, GAME_OVER};

typedef struct{
  int x;
  int y;
} Point;

// Head of the snake is always at body[size - 1]
typedef struct{
    Point** body;
    Point head;
    Direction direction;
    int size;
} Snake;

LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

Snake mySnake;

Point apple;

GameState gameState;

bool appleFlag;

// Picks a valid point to spawn on the game board
// Currently, it just picks a random point and checks to make sure it isn't occupied
// There might be a faster way to do this
void spawnApple(){
  bool validPoint = true;
  while (true){
    validPoint = true;
    apple.x = random(0, 8);
    apple.y = random(0, 8);

    for (int i = 0; i < mySnake.size; ++i){
      if (apple.x == mySnake.body[i]->x && apple.y == mySnake.body[i]->y){
        validPoint = false;
        break;
      }
    }

    if (validPoint){
      break;
    }
  }
}

// Grow snake body after eating an apple
void growBody(){
  // Increase size and resize body array
  mySnake.size += 1;
  mySnake.body = (Point **)realloc(mySnake.body, mySnake.size * sizeof(Point*));

  // Set last element in body array to head position
  mySnake.body[mySnake.size - 1] = (Point*)calloc(1, sizeof(Point));
  mySnake.body[mySnake.size - 1]->x = mySnake.head.x;
  mySnake.body[mySnake.size - 1]->y = mySnake.head.y;
}

// Move snake body along game board
void moveBody(){
  // Remove last point from body
  free(mySnake.body[0]);
  mySnake.body[0] = NULL;
  
  // Move every point in the body one back in the body array
  for (int i = 1; i < mySnake.size; ++i){
    mySnake.body[i - 1] = mySnake.body[i];
  }

  // Add point at head position to last index of array
  mySnake.body[mySnake.size - 1] = (Point *)calloc(1, sizeof(Point));
  mySnake.body[mySnake.size - 1]->x = mySnake.head.x;
  mySnake.body[mySnake.size - 1]->y = mySnake.head.y;
}

// Clears and updates display
void updateDisplay(){
  // Clear display
  lc.clearDisplay(0);
  
  // Set all leds matching index in body to on
  for (int i = 0; i < mySnake.size; ++i){
    lc.setLed(0, mySnake.body[i]->y, mySnake.body[i]->x, true);
  }

  // Turn apple coordinate on    
  lc.setLed(0, apple.y, apple.x, true);
}

void setup() {
  // Set gameState to START
  gameState = GameState::START;

  // Start serial output
  Serial.begin(9600);

  // Set random seed
  randomSeed(analogRead(0));

  // Reset max7219 display
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  myDisplay.begin();
  myDisplay.displayClear();

  myDisplay.displayText("SNAKE", PA_CENTER, 200, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  // Set head to (0, 0)
  mySnake.head.x = 0;
  mySnake.head.y = 0;

  // Set initial body at (0, 0)
  mySnake.body = (Point **)calloc(1, sizeof(Point *));
  mySnake.body[0] = (Point*)calloc(1, sizeof(Point));
  mySnake.body[0]->x = mySnake.head.x;
  mySnake.body[0]->y = mySnake.head.y;

  // Set initial direction to right
  mySnake.direction = Direction::RIGHT;

  // Set size of snake to 1
  mySnake.size = 1;

  // Show body on display
  lc.setLed(0, mySnake.body[0]->y, mySnake.body[0]->x, true);
  spawnApple();
  lc.setLed(0, apple.y, apple.x, true);
}

void printBody(){
  Serial.print("[");
  for (int i = 0; i < mySnake.size; i++){
    Serial.print("(");
    Serial.print(mySnake.body[i]->x);
    Serial.print(", ");
    Serial.print(mySnake.body[i]->y);
    Serial.print("), ");
  }
  Serial.println("]");
}

void loop() {
  if (gameState == GameState::START){  
    if (myDisplay.displayAnimate()) {
      myDisplay.displayReset();
    }
  } else {

  
  // Get joystick analog input
  int x = analogRead(X_PIN);
  int y = analogRead(Y_PIN); 

  // Change direction based off input
  if (x <= 100){
    mySnake.direction = Direction::UP;
  } else if (y >= 1000){
    mySnake.direction = Direction::LEFT;
  } else if (x >= 1000){
    mySnake.direction = Direction::DOWN;
  } else if (y <= 100){
    mySnake.direction = Direction::RIGHT;
  }

  // Move head of snake based on direction
  if (mySnake.direction == Direction::UP){
    mySnake.head.y = mySnake.head.y == 0 ? 7 : (mySnake.head.y - 1);
  } else if (mySnake.direction == Direction::LEFT){
    mySnake.head.x = mySnake.head.x == 0 ? 7 : (mySnake.head.x - 1);
  } else if (mySnake.direction == Direction::DOWN){
    mySnake.head.y = (mySnake.head.y + 1) % 8;
  } else if (mySnake.direction == Direction::RIGHT){
    mySnake.head.x = (mySnake.head.x + 1) % 8;
  }

  // If snake head coordinates matches apple coordinates, grow body
  // Otherwise move body
  if (apple.x == mySnake.head.x && apple.y == mySnake.head.y){
    growBody();
    spawnApple();
  } else {
    moveBody();
  }
  
  updateDisplay();

  delay(300);
  }
}
