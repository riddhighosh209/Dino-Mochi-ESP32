/*
 * ESP32-C3 SuperMini - Combined OLED Animation & Runner Game
 * Advanced State-Driven Animation Engine with Bitmap Dino Game
 * Controls:
 * - Quick Tap: Skip to next face (Animation) OR Jump / Retry (Game).
 * - Hold (1.5 seconds): Switch between Animation Mode and Game Mode.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "all_frames.h" // First 7 animations
#include "rain.h"
#include "weeping.h"
#include "sparkle.h"
#include "dizzy.h"

// Screen Properties
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

// Hardware Pins
#define I2C_SDA 20
#define I2C_SCL 21
#define TOUCH_PIN 1
#define BUZZER_PIN 2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- SYSTEM MODE CONFIG ---
enum SystemMode { MODE_ANIMATION, MODE_GAME };
SystemMode currentMode = MODE_ANIMATION;

unsigned long touchResetTime = 0;
bool touchHeld = false;
#define HOLD_DURATION 1500 
bool lastTouchState = LOW;

// --- ADVANCED ANIMATION ENGINE CONFIG ---
#define REST_DURATION 4000     // 4 seconds of resting on the default frame
#define DEFAULT_FRAME 1        // Default frame

struct FaceTrack {
  int startFrame;
  int endFrame;
};

// Mochi expression frame data blocks
FaceTrack faceAnimations[] = {
  {2, 84},  //Smirk
  {85, 154}, //IDK the name but its the second one
  {155, 266}, //Sleepy
  {267, 410}, //Speedometer
  {411, 494}, //Yawn
  {495, 549}, //Kiss
  {550, 771}  //Headlights
};

#define TOTAL_FACES (sizeof(faceAnimations) / sizeof(faceAnimations[0]))
#define TOTAL_ANIMATION_SEQUENCES (TOTAL_FACES + 4)

// Animation Playback States
enum AnimState { PLAYING_FACE, RESTING_ON_DEFAULT };
AnimState currentAnimState = PLAYING_FACE;

bool isPlaying = true;
int currentFaceIndex = 0;
int currentFrame = 2; 
unsigned long lastFrameTime = 0;
unsigned long restStartTime = 0;

// --- GAME CONFIG & BITMAPS ---
#define GRAVITY 1
#define JUMP_FORCE -7
#define GROUND_Y 56

int playerX = 20;
int playerY = GROUND_Y - 12;
int playerVelocityY = 0;
bool isJumping = false;
const int playerWidth = 8;
const int playerHeight = 12;

int obstacleX = 128;
int obstacleY = GROUND_Y - 10;
const int obstacleWidth = 6;
const int obstacleHeight = 10;
int gameSpeed = 3;

unsigned long score = 0;
bool gameOver = false;

// Custom 8x12 Google Dino Bitmap
const unsigned char dino_bitmap[] PROGMEM = {
  0b01111100, //   #####  
  0b01101110, //   ## ##  
  0b01111110, //   ###### 
  0b01111000, //   ####   
  0b01111110, //   ###### 
  0b01111010, //   #### # 
  0b00111000, //    ###   
  0b01111100, //   #####  
  0b10111010, //  # ### # 
  0b10110000, //  # ##    
  0b00011000, //     ##   
  0b00010100  //     # #  
};

// Custom 6x10 Cactus Bitmap (Padded to 8-bit width for GFX compliance)
const unsigned char cactus_bitmap[] PROGMEM = {
  0b00110000, //   ##    
  0b00110000, //   ##    
  0b01110100, //  ### #  
  0b11111100, // ######  
  0b11111100, // ######  
  0b11111000, // #####   
  0b00110000, //   ##    
  0b00110000, //   ##    
  0b00110000, //   ##    
  0b00110000  //   ##    
};

// --- ANIMATION SPEED ROUTER ---
int getFrameDelay(int faceIndex) {
  // Original animations (Indices 0 through 6)
  if (faceIndex < TOTAL_FACES) {
    return 42; 
  } 
  // Route 2: Shrink Animation (Index 7)
  else if (faceIndex >= TOTAL_FACES) {
    return 100; // INCREASE this to slow it down
  }
  
  // Fallback speed
  return 42; 
}

// --- AUDIO FUNCTIONS ---
void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}

void playStartupMelody() {
  tone(BUZZER_PIN, 440, 100); 
  delay(120);
  tone(BUZZER_PIN, 554, 100); 
  delay(120);
  tone(BUZZER_PIN, 659, 200); 
  delay(250);
  noTone(BUZZER_PIN);
}

void playTouchBeep() {
  tone(BUZZER_PIN, 880, 40); 
}

void playJumpSound() {
  tone(BUZZER_PIN, 600, 50); 
}

void playGameOverSound() {
  tone(BUZZER_PIN, 300, 150);
  delay(50);
  tone(BUZZER_PIN, 150, 300); 
}

void playModeSwitchSound() {
  tone(BUZZER_PIN, 523, 100); 
  delay(100);
  tone(BUZZER_PIN, 784, 150); 
  delay(150);
  noTone(BUZZER_PIN);
}

// --- GAME LOGIC FUNCTIONS ---
void resetGame(bool playChime = false) {
  playerY = GROUND_Y - playerHeight;
  playerVelocityY = 0;
  isJumping = false;
  obstacleX = 128;
  score = 0;
  gameSpeed = 3;
  gameOver = false;

  if (playChime) {
    playTone(440, 80);
    playTone(587, 120);
  }
}

// --- INITIALIZATION ---
void setup() {
  Serial.begin(115200);
  
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  playStartupMelody();
  
  Wire.begin(I2C_SDA, I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.display();
  
  resetGame(false);
  
  // Set initial frame based on the sequence
  if (TOTAL_ANIMATION_SEQUENCES > 0) {
    currentFrame = (currentFaceIndex < TOTAL_FACES) ? faceAnimations[currentFaceIndex].startFrame : 0;
  }
}

// --- MAIN LOOP ---
void loop() {
  unsigned long currentTime = millis();
  bool currentTouchState = digitalRead(TOUCH_PIN);
  
  // --- MODE SWITCHING & INPUT LOGIC ---
  if (currentTouchState == HIGH) {
    if (!touchHeld) {
      touchResetTime = currentTime;
      touchHeld = true;
    }
    
    if (currentTime - touchResetTime >= HOLD_DURATION) {
      currentMode = (currentMode == MODE_ANIMATION) ? MODE_GAME : MODE_ANIMATION;
      playModeSwitchSound();
      
      if (currentMode == MODE_GAME) {
        resetGame(false); 
      } else {
        currentFaceIndex = 0;
        currentAnimState = PLAYING_FACE;
        if (TOTAL_ANIMATION_SEQUENCES > 0) {
          currentFrame = (currentFaceIndex < TOTAL_FACES) ? faceAnimations[currentFaceIndex].startFrame : 0;
        }
      }
      
      display.clearDisplay();
      display.display();
      
      while(digitalRead(TOUCH_PIN) == HIGH) { delay(10); } 
      touchHeld = false;
      lastTouchState = LOW;
      return; 
    }
  } else {
    if (touchHeld) {
      touchHeld = false;
      
      if (currentMode == MODE_ANIMATION) {
        if (TOTAL_ANIMATION_SEQUENCES > 0) {
          // Increment sequence index and wrap around total sequences (Faces + 1)
          currentFaceIndex = (currentFaceIndex + 1) % TOTAL_ANIMATION_SEQUENCES;
          
          // Determine starting frame depending on sequence type
          currentFrame = (currentFaceIndex < TOTAL_FACES) ? faceAnimations[currentFaceIndex].startFrame : 0;
          
          currentAnimState = PLAYING_FACE;
          playTouchBeep(); 
        }
      } 
      else if (currentMode == MODE_GAME) {
        if (gameOver) {
          resetGame(true); 
        } else if (!isJumping) {
          playerVelocityY = JUMP_FORCE;
          isJumping = true;
          playJumpSound();
        }
      }
    }
  }
  lastTouchState = currentTouchState;

 // --- MODE EXECUTION ENGINE ---
  if (currentMode == MODE_ANIMATION) {
    if (isPlaying && TOTAL_ANIMATION_SEQUENCES > 0) {
      
      //Grab the specific speed for whatever animation is currently playing
      int currentDelay = getFrameDelay(currentFaceIndex);

      if (currentAnimState == PLAYING_FACE) {
        if (currentTime - lastFrameTime >= currentDelay) {
          lastFrameTime = currentTime;
          display.clearDisplay();

          // ROUTE 1: Original Array Animations
          if (currentFaceIndex < TOTAL_FACES) {
            display.drawBitmap(0, 0, frames[currentFrame], 128, 64, SSD1306_WHITE);
            currentFrame++;
            
            if (currentFrame > faceAnimations[currentFaceIndex].endFrame) {
              currentAnimState = RESTING_ON_DEFAULT;
              restStartTime = currentTime;
              currentFrame = DEFAULT_FRAME;
            }
          } 
          // ROUTE 2: Rain Animation 
          else if (currentFaceIndex == TOTAL_FACES ) {
            display.drawBitmap(0, 0, rain_frames[currentFrame], 128, 64, SSD1306_WHITE);
            currentFrame++;
            
            if (currentFrame >= RAIN_FRAME_COUNT) {
              currentAnimState = RESTING_ON_DEFAULT;
              restStartTime = currentTime;
              currentFrame = DEFAULT_FRAME; // Reverts back to standard rest face
            }
          }
          // ROUTE 3: Weeping Animation
          else if (currentFaceIndex == TOTAL_FACES + 1) {
            display.drawBitmap(0, 0, weeping_frames[currentFrame], 128, 64, SSD1306_WHITE);
            currentFrame++;
  
             if (currentFrame >= WEEPING_FRAME_COUNT) {
             currentAnimState = RESTING_ON_DEFAULT;
             restStartTime = currentTime;
            currentFrame = DEFAULT_FRAME; 
            }
          }
          // ROUTE 4: Sparkle Animation
          else if (currentFaceIndex == TOTAL_FACES + 2) {
            display.drawBitmap(0, 0, sparkle_frames[currentFrame], 128, 64, SSD1306_WHITE);
            currentFrame++;
  
             if (currentFrame >= SPARKLE_FRAME_COUNT) {
             currentAnimState = RESTING_ON_DEFAULT;
             restStartTime = currentTime;
            currentFrame = DEFAULT_FRAME; 
            }
          }
          // ROUTE 5: Dizzy Animation
          else if (currentFaceIndex == TOTAL_FACES + 3) {
            display.drawBitmap(0, 0, dizzy_frames[currentFrame], 128, 64, SSD1306_WHITE);
            currentFrame++;
  
             if (currentFrame >= DIZZY_FRAME_COUNT) {
             currentAnimState = RESTING_ON_DEFAULT;
             restStartTime = currentTime;
            currentFrame = DEFAULT_FRAME; 
            }
          }
          
          display.display();
        }
      } 
      else if (currentAnimState == RESTING_ON_DEFAULT) {
        if (currentTime - lastFrameTime >= currentDelay) {
          lastFrameTime = currentTime;
          display.clearDisplay();
          // Always use the standard frames array for the default face rest state
          display.drawBitmap(0, 0, frames[DEFAULT_FRAME], 128, 64, SSD1306_WHITE);
          display.display();
        }
        
        // Auto-advance to next animation sequence after rest
        if (currentTime - restStartTime >= REST_DURATION) {
          currentFaceIndex = (currentFaceIndex + 1) % TOTAL_ANIMATION_SEQUENCES;
          currentFrame = (currentFaceIndex < TOTAL_FACES) ? faceAnimations[currentFaceIndex].startFrame : 0;
          currentAnimState = PLAYING_FACE;
        }
      }
    }
  } 
  else {
    // --- GAME MODE ---
    if (gameOver) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 15);
      display.print("GAME OVER");
      
      display.setTextSize(1);
      display.setCursor(25, 40);
      display.print("Score: ");
      display.print(score);
      display.setCursor(15, 52);
      display.print("Tap Touch to Retry");
      display.display();
    } 
    else {
      playerVelocityY += GRAVITY;
      playerY += playerVelocityY;

      if (playerY >= GROUND_Y - playerHeight) {
        playerY = GROUND_Y - playerHeight;
        playerVelocityY = 0;
        isJumping = false;
      }

      obstacleX -= gameSpeed;
      if (obstacleX < -obstacleWidth) {
        obstacleX = 128; 
        score += 10;    
        
        if (score % 50 == 0 && gameSpeed < 7) {
          gameSpeed++;
        }
      }

      // Collision Detection (AABB)
      if (playerX < obstacleX + obstacleWidth &&
          playerX + playerWidth > obstacleX &&
          playerY < obstacleY + obstacleHeight &&
          playerY + playerHeight > obstacleY) {
        gameOver = true;
        playGameOverSound();
      }

      display.clearDisplay();
      
      // Draw Horizon Line
      display.drawFastHLine(0, GROUND_Y, 128, SSD1306_WHITE); 
      
      // Render custom Dino & Cactus Bitmaps
      display.drawBitmap(playerX, playerY, dino_bitmap, playerWidth, playerHeight, SSD1306_WHITE);
      display.drawBitmap(obstacleX, obstacleY, cactus_bitmap, obstacleWidth, obstacleHeight, SSD1306_WHITE); 
      
      // Scoreboard
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(85, 4);
      display.print("XP:");
      display.print(score);

      display.display();
    }
    delay(20); 
  }
}