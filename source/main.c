#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdlib.h>
#include <stdio.h>

// --- 1. HARDWARE REGISTERS FOR SOUND ---
#define REG_SND1SWEEP   *(volatile u16*)0x04000060
#define REG_SND1CNT     *(volatile u16*)0x04000062
#define REG_SND1FREQ    *(volatile u16*)0x04000064
#define REG_SND4CNT     *(volatile u16*)0x04000078
#define REG_SND4FREQ    *(volatile u16*)0x0400007C
#define REG_SNDDSCNT    *(volatile u16*)0x04000080
#define REG_SNDSTAT     *(volatile u16*)0x04000084

// --- 2. DEFINITIONS ---
#define RGB15(r,g,b)  ((r)|((g)<<5)|((b)<<10))
#define SCREEN_W      240
#define SCREEN_H      160
#define VRAM          ((volatile u16*)0x06000000)

// Colors
#define C_BG          RGB15(1, 2, 1)      // Dark Green
#define C_ROAD        RGB15(6, 6, 6)      // Dark Grey
#define C_WHITE       RGB15(31,31,31)
#define C_RED         RGB15(31, 5, 5)
#define C_HUD         RGB15(4, 4, 4)
#define C_GOLD        RGB15(31,25,0)
#define C_FUEL_HI     RGB15(0, 31, 0)
#define C_FUEL_LO     RGB15(31, 0, 0)
#define C_SKY         RGB15(0, 0, 5)

// --- 3. SPRITE DATA (Ported from your JS) ---
// Each sprite is 14x24 pixels. 0=Transparent.
// We map your JS numbers (1,2,3...) to specific GBA colors in getPixelColor()

const u8 SPRITE_CLASSIC[336] = {
    0,0,1,1,1,2,2,2,2,1,1,1,0,0, 0,1,6,1,1,2,2,2,2,1,1,6,1,0,
    0,1,6,1,1,1,7,7,1,1,1,6,1,0, 0,1,1,1,2,1,7,7,1,2,1,1,1,0,
    0,1,1,1,2,1,7,7,1,2,1,1,1,0, 0,1,1,1,1,1,7,7,1,1,1,1,1,0,
    4,1,1,1,1,3,7,7,3,1,1,1,1,4, 4,1,1,1,3,3,7,7,3,3,1,1,1,4,
    4,1,1,3,3,3,7,7,3,3,3,1,1,4, 4,1,1,3,3,3,7,7,3,3,3,1,1,4,
    0,1,1,3,3,3,7,7,3,3,3,1,1,0, 0,1,1,1,1,1,7,7,1,1,1,1,1,0,
    0,1,1,1,1,1,7,7,1,1,1,1,1,0, 0,1,1,1,1,1,7,7,1,1,1,1,1,0,
    0,1,1,1,1,1,7,7,1,1,1,1,1,0, 0,1,1,2,1,1,7,7,1,1,2,1,1,0,
    4,1,1,2,2,2,2,2,2,2,2,1,1,4, 4,1,1,2,2,2,2,2,2,2,2,1,1,4,
    4,1,1,2,2,2,2,2,2,2,2,1,1,4, 4,1,1,2,2,2,2,2,2,2,2,1,1,4,
    0,1,1,1,1,1,7,7,1,1,1,1,1,0, 0,1,1,5,5,1,7,7,1,5,5,1,1,0,
    0,1,1,5,5,1,1,1,1,5,5,1,1,0, 0,0,1,1,1,1,1,1,1,1,1,1,0,0
};

const u8 SPRITE_F1[336] = {
    0,0,0,0,1,1,1,1,1,1,0,0,0,0, 0,0,0,1,1,1,1,1,1,1,1,0,0,0,
    0,0,1,9,9,1,1,1,1,9,9,1,0,0, 4,4,0,0,0,1,1,1,1,0,0,0,4,4,
    4,4,0,9,9,1,7,7,1,9,9,0,4,4, 4,4,0,0,0,1,7,7,1,0,0,0,4,4,
    0,0,0,0,0,1,7,7,1,0,0,0,0,0, 0,0,0,0,0,1,3,3,1,0,0,0,0,0,
    0,0,0,0,1,1,3,3,1,1,0,0,0,0, 0,0,0,0,1,1,3,3,1,1,0,0,0,0,
    0,0,0,1,1,1,6,6,1,1,1,0,0,0, 0,0,0,1,1,1,1,1,1,1,1,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,1,0,0, 0,0,1,2,1,1,9,9,1,1,2,1,0,0,
    0,0,1,2,1,1,9,9,1,1,2,1,0,0, 0,0,1,2,1,1,9,9,1,1,2,1,0,0,
    4,4,0,0,0,1,1,1,1,0,0,0,4,4, 4,4,0,9,9,1,1,1,1,9,9,0,4,4,
    4,4,0,0,0,1,9,9,1,0,0,0,4,4, 0,0,0,0,0,1,9,9,1,0,0,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,1,0,0, 0,0,1,1,1,1,1,1,1,1,1,1,0,0,
    0,0,1,1,1,1,5,5,1,1,1,1,0,0, 0,0,0,0,0,2,2,2,2,0,0,0,0,0
};

const u8 SPRITE_TANK[336] = {
    0,0,0,0,0,0,9,9,0,0,0,0,0,0, 0,0,0,0,0,0,9,9,0,0,0,0,0,0,
    0,0,0,0,0,0,9,9,0,0,0,0,0,0, 0,2,2,1,1,1,1,1,1,1,1,2,2,0,
    0,2,2,1,1,1,1,1,1,1,1,2,2,0, 0,2,2,1,1,1,2,2,1,1,1,2,2,0,
    0,2,2,1,1,2,2,2,2,1,1,2,2,0, 0,2,2,1,1,2,2,2,2,1,1,2,2,0,
    0,2,2,1,1,2,2,2,2,1,1,2,2,0, 0,2,2,1,1,2,2,2,2,1,1,2,2,0,
    0,2,2,1,1,1,2,2,1,1,1,2,2,0, 0,2,2,1,1,1,1,1,1,1,1,1,2,2,0,
    0,2,2,1,1,1,1,1,1,1,1,2,2,0, 0,2,2,1,1,1,1,1,1,1,1,1,2,2,0,
    0,2,2,1,1,1,1,1,1,1,1,2,2,0, 0,2,2,1,1,1,1,1,1,1,1,1,2,2,0,
    0,2,2,1,1,1,1,1,1,1,1,2,2,0, 0,2,2,1,1,1,1,1,1,1,1,1,2,2,0,
    0,2,2,1,1,1,1,1,1,1,1,2,2,0, 0,2,2,1,1,1,1,1,1,1,1,1,2,2,0,
    0,2,2,1,1,1,5,5,1,1,1,2,2,0, 0,2,2,1,1,1,1,1,1,1,1,1,2,2,0,
    0,2,2,1,1,1,1,1,1,1,1,2,2,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

const u8 SPRITE_JERRYCAN[336] = { // Smaller, centered in 14x24 frame
    0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,9,9,9,9,0,0,0,0,0, 0,0,0,0,0,9,0,0,9,0,0,0,0,0,
    0,0,0,8,8,8,8,8,8,8,8,0,0,0, 0,0,0,8,8,8,8,8,8,8,8,0,0,0,
    0,0,0,8,8,8,6,6,8,8,8,0,0,0, 0,0,0,8,8,8,6,6,8,8,8,0,0,0,
    0,0,0,8,8,8,6,6,8,8,8,0,0,0, 0,0,0,8,8,8,6,6,8,8,8,0,0,0,
    0,0,0,8,8,8,6,6,8,8,8,0,0,0, 0,0,0,8,8,8,8,8,8,8,8,0,0,0,
    0,0,0,8,8,8,8,8,8,8,8,0,0,0, 0,0,0,8,8,8,8,8,8,8,8,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// --- 4. GAME STRUCTURES ---
typedef struct {
    float x, y;
    float speed, maxSpeed;
    float fuel, maxFuel;
    float grip;
    u16 color;
    int type; // 0=Classic, 1=F1, 2=Tank
    int active;
    int powerupType; // 0=None, 1=Nitro, 2=Shield
    int powerupTimer;
} Car;

typedef struct {
    float x, y;
    int active;
    int type; // 0=ClassicEnemy, 1=TankEnemy, 2=JerryCan
    u16 color;
} Enemy;

typedef struct {
    float x, y;
    int active;
    int type; // 0=Coin, 1=Nitro, 2=Shield
} Item;

// Global Game State
Car player;
Enemy enemies[4];
Item items[3];
int cash = 0;
int highScore = 0;
float roadY = 0;
float distance = 0;
int gameState = 0; // 0=Menu, 1=Garage, 2=Play, 3=GameOver

// --- 5. AUDIO ENGINE ---
void sndInit() {
    // Turn on sound circuit
    REG_SNDDSCNT = 0; 
    REG_SNDSTAT = 0x80; // Enable sound
    REG_SNDDSCNT = 0x0200; // DMG ratio to 100%
}

void sndPlayTone(int freq, int len) {
    // Channel 1: Square wave with sweep
    REG_SND1SWEEP = 0x007F; // No sweep
    REG_SND1CNT   = 0x8100; // 50% duty, envelope
    REG_SND1FREQ  = 0x8000 | freq; // Trigger, use freq
}

void sndPlayNoise() {
    // Channel 4: White noise (Crash)
    REG_SND4CNT = 0xC000; // Envelope
    REG_SND4FREQ = 0x8000 | 0x0020; // Trigger
}

// --- 6. RENDERER ---
void drawRect(int x, int y, int w, int h, u16 color) {
    for(int i=0; i<h; i++) {
        for(int j=0; j<w; j++) {
            int dx = x+j; int dy = y+i;
            if(dx >= 0 && dx < SCREEN_W && dy >= 0 && dy < SCREEN_H) {
                VRAM[dy*SCREEN_W + dx] = color;
            }
        }
    }
}

u16 getPixelColor(int id, u16 bodyColor) {
    switch(id) {
        case 1: return bodyColor;
        case 2: return RGB15(5,5,5);   // Dark Grey parts
        case 3: return RGB15(15,25,31);// Glass
        case 4: return RGB15(2,2,2);   // Tires
        case 5: return RGB15(31,0,0);  // Red Lights
        case 6: return RGB15(31,31,0); // Yellow Lights
        case 7: return RGB15(31,31,31);// White
        case 8: return RGB15(31,0,0);  // JerryCan Red
        case 9: return RGB15(15,15,15);// Metal
        default: return 0;
    }
}

void drawSprite(int x, int y, const u8* spriteData, u16 color) {
    if(y < -24 || y > SCREEN_H) return;
    for(int i=0; i<336; i++) {
        int r = i / 14;
        int c = i % 14;
        int pType = spriteData[i];
        if(pType != 0) {
            u16 col = getPixelColor(pType, color);
            int dx = x + c; 
            int dy = y + r;
            if(dx>=0 && dx<SCREEN_W && dy>=0 && dy<SCREEN_H) {
                VRAM[dy*SCREEN_W + dx] = col;
            }
        }
    }
}

// Simple text renderer (Draws dots for numbers)
void drawNum(int x, int y, int num, u16 color) {
    // Just drawing bars for simplicity in Mode 3
    // A real font requires a large array
    int h = 5;
    if(num > 10) h = 10;
    drawRect(x, y, num/2 + 2, h, color); 
}

// --- 7. GAME LOGIC ---
void initGame() {
    player.x = SCREEN_W / 2 - 7;
    player.y = SCREEN_H - 40;
    player.speed = 0;
    player.fuel = player.maxFuel;
    player.active = 1;
    player.powerupType = 0;
    
    distance = 0;
    for(int i=0; i<4; i++) enemies[i].active = 0;
    for(int i=0; i<3; i++) items[i].active = 0;
}

void updateGame() {
    scanKeys();
    u16 k = keysHeld();
    u16 kDown = keysDown();

    // 1. Controls
    if(k & KEY_LEFT) player.x -= 1.5 * player.grip;
    if(k & KEY_RIGHT) player.x += 1.5 * player.grip;
    if(player.x < 20) player.x = 20;
    if(player.x > SCREEN_W - 34) player.x = SCREEN_W - 34;

    // 2. Speed & Fuel
    if(player.speed < player.maxSpeed) player.speed += 0.05;
    
    // Nitro Logic
    if(player.powerupType == 1) { // Nitro Active
        player.speed = player.maxSpeed * 1.5;
        player.powerupTimer--;
        if(player.powerupTimer <= 0) player.powerupType = 0;
    }

    player.fuel -= 0.03;
    if(player.fuel <= 0) {
        sndPlayNoise();
        gameState = 3; // Game Over
    }

    // 3. World Movement
    float worldSpeed = player.speed;
    roadY += worldSpeed;
    if(roadY >= 20) roadY = 0;
    distance += worldSpeed * 0.01;

    // 4. Powerup Activation
    if((kDown & KEY_A) && player.powerupType == 2) { // Use Shield/Magnet?
        // Simple GBA version: Powerups auto-activate on pickup usually
        // But let's say A triggers horn
        sndPlayTone(1000, 100);
    }

    // 5. Enemies
    for(int i=0; i<4; i++) {
        if(enemies[i].active) {
            enemies[i].y += (player.speed * 0.4) + 1.0; 
            
            // Collision (AABB)
            if(player.x < enemies[i].x + 14 && player.x + 14 > enemies[i].x &&
               player.y < enemies[i].y + 24 && player.y + 24 > enemies[i].y) {
                   
                if(enemies[i].type == 2) { // JerryCan
                    player.fuel += 30;
                    if(player.fuel > player.maxFuel) player.fuel = player.maxFuel;
                    sndPlayTone(1500, 50);
                    enemies[i].active = 0;
                } else {
                    // Crash
                    if(player.powerupType == 1 || player.type == 2) { 
                        // Invincible (Nitro or Tank)
                        sndPlayNoise();
                        enemies[i].active = 0;
                    } else {
                        sndPlayNoise();
                        player.fuel -= 20;
                        enemies[i].active = 0;
                    }
                }
            }

            if(enemies[i].y > SCREEN_H) enemies[i].active = 0;
        } else if((rand() % 100) < 2) {
            enemies[i].active = 1;
            enemies[i].x = 25 + (rand() % (SCREEN_W - 60));
            enemies[i].y = -30;
            if((rand()%10) > 8) {
                enemies[i].type = 2; // JerryCan
            } else {
                enemies[i].type = rand() % 2; // Car
                enemies[i].color = RGB15(rand()%31, rand()%31, rand()%31);
            }
        }
    }

    // 6. Items
    for(int i=0; i<3; i++) {
        if(items[i].active) {
            items[i].y += player.speed;
            if(player.x < items[i].x + 8 && player.x + 14 > items[i].x &&
               player.y < items[i].y + 8 && player.y + 24 > items[i].y) {
                
                if(items[i].type == 0) { // Coin
                    cash += 10;
                    sndPlayTone(2000, 50);
                } else if(items[i].type == 1) { // Nitro
                    player.powerupType = 1;
                    player.powerupTimer = 300; // 5 seconds
                    sndPlayTone(500, 300);
                }
                items[i].active = 0;
            }
            if(items[i].y > SCREEN_H) items[i].active = 0;
        } else if((rand() % 300) < 2) {
            items[i].active = 1;
            items[i].x = 30 + (rand() % (SCREEN_W - 70));
            items[i].y = -20;
            items[i].type = rand() % 2; // Coin or Nitro
        }
    }
}

// --- 8. MAIN ---
int main() {
    irqInit();
    irqEnable(IRQ_VBLANK);
    sndInit();

    REG_DISPCNT = MODE_3 | BG2_ENABLE;

    // Initial Player Stats (Classic)
    player.maxSpeed = 4.0;
    player.maxFuel = 100;
    player.grip = 1.0;
    player.type = 0;
    player.color = C_RED;

    while(1) {
        VBlankIntrWait();
        scanKeys();

        if(gameState == 0) { 
            // MENU
            drawRect(0,0,SCREEN_W,SCREEN_H, RGB15(0,0,5)); // Blue BG
            drawRect(40, 40, 160, 20, C_WHITE); // Title Box
            
            // Draw spinning car
            Car p = {113, 80, 0,0,0,0,0, C_RED, 0, 1, 0, 0};
            p.type = (distance > 50) ? 1 : 0; 
            if(distance > 100) distance = 0;
            distance++;
            
            const u8* spr = (p.type==1) ? SPRITE_F1 : SPRITE_CLASSIC;
            drawSprite(113, 80, spr, C_RED);
            
            if(keysDown() & KEY_START) gameState = 1; // Go to Garage

        } else if(gameState == 1) {
            // GARAGE / CAR SELECT
            drawRect(0,0,SCREEN_W,SCREEN_H, RGB15(5,5,5)); // Grey BG
            
            // Draw Current Car
            const u8* spr = SPRITE_CLASSIC;
            if(player.type == 1) spr = SPRITE_F1;
            if(player.type == 2) spr = SPRITE_TANK;
            
            drawSprite(113, 70, spr, player.color);
            
            // Selection Logic
            if(keysDown() & KEY_RIGHT) {
                player.type++;
                if(player.type > 2) player.type = 0;
                sndPlayTone(600, 50);
            }
            if(keysDown() & KEY_LEFT) {
                player.type--;
                if(player.type < 0) player.type = 2;
                sndPlayTone(600, 50);
            }
            
            // Apply stats based on type
            if(player.type == 0) { player.maxSpeed=4.0; player.maxFuel=100; player.color=C_RED; } // Classic
            if(player.type == 1) { player.maxSpeed=6.0; player.maxFuel=60; player.color=RGB15(0,20,31); } // F1
            if(player.type == 2) { player.maxSpeed=2.5; player.maxFuel=200; player.color=RGB15(5,15,5); } // Tank

            if(keysDown() & KEY_A) {
                initGame();
                sndPlayTone(1000, 200);
                gameState = 2; // RACE!
            }

        } else if(gameState == 2) {
            // PLAY
            updateGame();

            // Background
            drawRect(0, 0, SCREEN_W, SCREEN_H, C_BG);
            drawRect(20, 0, SCREEN_W - 40, SCREEN_H, C_ROAD);

            // Stripes
            for(int i=-1; i<9; i++) {
                int y = i*20 + (int)roadY;
                u16 col = (i%2==0) ? C_WHITE : C_RED;
                drawRect(20, y, 5, 20, col);
                drawRect(SCREEN_W-25, y, 5, 20, col);
            }

            // Entities
            for(int i=0; i<3; i++) {
                if(items[i].active) {
                    u16 c = (items[i].type==0)?C_GOLD:RGB15(0,31,31); // Gold or Cyan(Nitro)
                    drawRect((int)items[i].x, (int)items[i].y, 8, 8, c);
                }
            }
            for(int i=0; i<4; i++) {
                if(enemies[i].active) {
                    const u8* eSpr = (enemies[i].type==2) ? SPRITE_JERRYCAN : SPRITE_CLASSIC;
                    if(enemies[i].type == 1) eSpr = SPRITE_TANK;
                    drawSprite((int)enemies[i].x, (int)enemies[i].y, eSpr, enemies[i].color);
                }
            }
            
            // Draw Player
            const u8* pSpr = SPRITE_CLASSIC;
            if(player.type == 1) pSpr = SPRITE_F1;
            if(player.type == 2) pSpr = SPRITE_TANK;
            
            u16 pColor = player.color;
            if(player.powerupType == 1 && (distance > 0)) pColor = RGB15(31,31,31); // Flash white if Nitro
            
            drawSprite((int)player.x, (int)player.y, pSpr, pColor);

            // HUD
            drawRect(0, 0, SCREEN_W, 10, C_HUD);
            // Fuel Bar
            int fW = (int)player.fuel;
            drawRect(5, 2, fW, 6, (player.fuel>30)?C_FUEL_HI:C_FUEL_LO);
            // Cash
            // We can't easily draw text, so we draw yellow dots for cash count approximation
            drawRect(200, 2, 5, 5, C_GOLD);
            drawNum(210, 2, cash, C_WHITE);

        } else if(gameState == 3) {
            // GAME OVER
            drawRect(0,0,SCREEN_W,SCREEN_H, C_RED);
            // Draw a big X
            for(int i=0; i<100; i++) {
                VRAM[(30+i)*SCREEN_W + (70+i)] = C_WHITE;
                VRAM[(30+i)*SCREEN_W + (170-i)] = C_WHITE;
            }
            if(keysDown() & KEY_START) {
                gameState = 1; // Back to Garage
            }
        }
    }
}
