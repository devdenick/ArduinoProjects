#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "LGFX_4D_43CT.h"
#include "GameTypes.h"

LGFX_4D_43CT lcd;
LGFX_Sprite canvas(&lcd);

// ============================================================
// DISPLAY
// ============================================================

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 800;

constexpr int VIEW_W = 480;
constexpr int VIEW_H = 640;

constexpr int CONTROL_Y = 640;
constexpr int CONTROL_H = 160;

// Renderizziamo 240 raggi larghi 2 pixel.
// Look retro + prestazioni migliori.
constexpr int NUM_RAYS = 240;
constexpr int RAY_WIDTH = 2;

// ============================================================
// COLORI
// ============================================================

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) |
         ((g & 0xFC) << 3) |
         (b >> 3);
}

constexpr uint16_t COL_BLACK     = rgb565(0, 0, 0);
constexpr uint16_t COL_WHITE     = rgb565(255, 255, 255);
constexpr uint16_t COL_SKY       = rgb565(28, 36, 55);
constexpr uint16_t COL_FLOOR     = rgb565(38, 32, 28);

constexpr uint16_t COL_RED       = rgb565(220, 40, 40);
constexpr uint16_t COL_DARK_RED  = rgb565(110, 20, 20);

constexpr uint16_t COL_GRAY      = rgb565(90, 90, 90);
constexpr uint16_t COL_DARK_GRAY = rgb565(30, 30, 30);

constexpr uint16_t COL_GREEN     = rgb565(50, 220, 80);
constexpr uint16_t COL_YELLOW    = rgb565(255, 210, 40);

constexpr uint16_t COL_SKIN      = rgb565(220, 165, 120);

// ============================================================
// MAPPA
// ============================================================

constexpr int MAP_W = 16;
constexpr int MAP_H = 16;

const char* WORLD[MAP_H] =
{
  "################",
  "#..............#",
  "#..##..........#",
  "#..##....####..#",
  "#........#.....#",
  "#........#.....#",
  "#...###..#.....#",
  "#..............#",
  "#.......###....#",
  "#.......#......#",
  "#..###..#......#",
  "#..#...........#",
  "#..#....####...#",
  "#..............#",
  "#..............#",
  "################"
};

// ============================================================
// PLAYER
// ============================================================

float playerX = 2.5f;
float playerY = 2.5f;
float playerAngle = 0.0f;

int health = 100;

constexpr float PLAYER_RADIUS = 0.20f;

constexpr float MOVE_SPEED = 2.2f;
constexpr float TURN_SPEED = 2.4f;

constexpr float PI_F = 3.14159265358979323846f;

constexpr float FOV = PI_F / 3.0f; // 60°

// ============================================================
// NEMICI
// ============================================================

struct Enemy
{
  float x;
  float y;
  bool alive;
};

constexpr int ENEMY_COUNT = 6;

const float ENEMY_START[ENEMY_COUNT][2] =
{
  { 7.5f,  2.5f  },
  { 12.5f, 3.5f  },
  { 5.5f,  8.5f  },
  { 13.0f, 9.5f  },
  { 5.5f,  13.0f },
  { 11.5f, 13.5f }
};

Enemy enemies[ENEMY_COUNT];

int kills = 0;

// ============================================================
// RAYCASTING
// ============================================================

float zBuffer[NUM_RAYS];


float normalizeAngle(float angle)
{
  while (angle > PI_F)
    angle -= 2.0f * PI_F;

  while (angle < -PI_F)
    angle += 2.0f * PI_F;

  return angle;
}

bool isWall(int x, int y)
{
  if (x < 0 || x >= MAP_W ||
      y < 0 || y >= MAP_H)
    return true;

  return WORLD[y][x] == '#';
}

RayHit castRay(float angle)
{
  float rayDirX = cosf(angle);
  float rayDirY = sinf(angle);

  int mapX = (int)playerX;
  int mapY = (int)playerY;

  float deltaDistX =
    (fabsf(rayDirX) < 0.00001f)
    ? 1000000.0f
    : fabsf(1.0f / rayDirX);

  float deltaDistY =
    (fabsf(rayDirY) < 0.00001f)
    ? 1000000.0f
    : fabsf(1.0f / rayDirY);

  int stepX;
  int stepY;

  float sideDistX;
  float sideDistY;

  if (rayDirX < 0)
  {
    stepX = -1;
    sideDistX =
      (playerX - mapX) * deltaDistX;
  }
  else
  {
    stepX = 1;
    sideDistX =
      (mapX + 1.0f - playerX) *
      deltaDistX;
  }

  if (rayDirY < 0)
  {
    stepY = -1;
    sideDistY =
      (playerY - mapY) *
      deltaDistY;
  }
  else
  {
    stepY = 1;
    sideDistY =
      (mapY + 1.0f - playerY) *
      deltaDistY;
  }

  int side = 0;

  for (int i = 0; i < 64; i++)
  {
    if (sideDistX < sideDistY)
    {
      sideDistX += deltaDistX;
      mapX += stepX;
      side = 0;
    }
    else
    {
      sideDistY += deltaDistY;
      mapY += stepY;
      side = 1;
    }

    if (isWall(mapX, mapY))
      break;
  }

  float distance;

  if (side == 0)
    distance = sideDistX - deltaDistX;
  else
    distance = sideDistY - deltaDistY;

  if (distance < 0.001f)
    distance = 0.001f;

  return
  {
    distance,
    side,
    mapX,
    mapY
  };
}

// ============================================================
// COLORAZIONE MURI
// ============================================================

uint16_t shadeColor(uint16_t color, float factor)
{
  if (factor < 0.15f)
    factor = 0.15f;

  if (factor > 1.0f)
    factor = 1.0f;

  int r = (color >> 11) & 31;
  int g = (color >> 5) & 63;
  int b = color & 31;

  r = (int)(r * factor);
  g = (int)(g * factor);
  b = (int)(b * factor);

  return (r << 11) |
         (g << 5) |
         b;
}

// ============================================================
// COLLISIONE PLAYER
// ============================================================

bool canStand(float x, float y)
{
  float r = PLAYER_RADIUS;

  if (isWall((int)(x - r), (int)(y - r)))
    return false;

  if (isWall((int)(x + r), (int)(y - r)))
    return false;

  if (isWall((int)(x - r), (int)(y + r)))
    return false;

  if (isWall((int)(x + r), (int)(y + r)))
    return false;

  return true;
}

void movePlayer(float movement)
{
  float newX =
    playerX +
    cosf(playerAngle) * movement;

  float newY =
    playerY +
    sinf(playerAngle) * movement;

  // Collisione separata X/Y:
  // permette di "scivolare" sui muri.

  if (canStand(newX, playerY))
    playerX = newX;

  if (canStand(playerX, newY))
    playerY = newY;
}

// ============================================================
// LINE OF SIGHT
// ============================================================

bool enemyVisible(float ex, float ey)
{
  float dx = ex - playerX;
  float dy = ey - playerY;

  float enemyDistance =
    sqrtf(dx * dx + dy * dy);

  float angle =
    atan2f(dy, dx);

  RayHit hit = castRay(angle);

  return hit.distance >
         enemyDistance - 0.20f;
}

// ============================================================
// SPARO
// ============================================================

uint32_t lastShot = 0;
uint32_t muzzleFlashUntil = 0;

void shoot()
{
  uint32_t now = millis();

  if (now - lastShot < 250)
    return;

  lastShot = now;
  muzzleFlashUntil = now + 80;

  int target = -1;
  float nearest = 99999.0f;

  for (int i = 0;
       i < ENEMY_COUNT;
       i++)
  {
    if (!enemies[i].alive)
      continue;

    float dx =
      enemies[i].x - playerX;

    float dy =
      enemies[i].y - playerY;

    float distance =
      sqrtf(dx * dx + dy * dy);

    float enemyAngle =
      atan2f(dy, dx);

    float diff =
      normalizeAngle(
        enemyAngle -
        playerAngle);

    // Larghezza angolare del nemico
    float hitAngle =
      atan2f(0.35f, distance);

    if (fabsf(diff) < hitAngle)
    {
      if (enemyVisible(
            enemies[i].x,
            enemies[i].y))
      {
        if (distance < nearest)
        {
          nearest = distance;
          target = i;
        }
      }
    }
  }

  if (target >= 0)
  {
    enemies[target].alive = false;
    kills++;
  }
}

// ============================================================
// AI NEMICI
// ============================================================

uint32_t lastEnemyDamage = 0;

void updateEnemies(float dt)
{
  for (int i = 0;
       i < ENEMY_COUNT;
       i++)
  {
    if (!enemies[i].alive)
      continue;

    float dx =
      playerX - enemies[i].x;

    float dy =
      playerY - enemies[i].y;

    float distance =
      sqrtf(dx * dx + dy * dy);

    // Attacco
    if (distance < 0.65f)
    {
      if (millis() -
          lastEnemyDamage >
          650)
      {
        health -= 10;

        if (health < 0)
          health = 0;

        lastEnemyDamage =
          millis();
      }

      continue;
    }

    // Il nemico si muove solo quando
    // "vede" il giocatore.
    if (!enemyVisible(
          enemies[i].x,
          enemies[i].y))
      continue;

    if (distance < 0.001f)
      continue;

    float speed = 0.55f;

    float vx =
      dx / distance *
      speed * dt;

    float vy =
      dy / distance *
      speed * dt;

    float nx =
      enemies[i].x + vx;

    float ny =
      enemies[i].y + vy;

    if (!isWall(
          (int)nx,
          (int)enemies[i].y))
    {
      enemies[i].x = nx;
    }

    if (!isWall(
          (int)enemies[i].x,
          (int)ny))
    {
      enemies[i].y = ny;
    }
  }
}

// ============================================================
// DISEGNO MURI
// ============================================================

void drawWorld()
{
  const int horizon =
    VIEW_H / 2;

  // Cielo
  canvas.fillRect(
    0,
    0,
    VIEW_W,
    horizon,
    COL_SKY);

  // Pavimento
  canvas.fillRect(
    0,
    horizon,
    VIEW_W,
    VIEW_H - horizon,
    COL_FLOOR);

  for (int ray = 0;
       ray < NUM_RAYS;
       ray++)
  {
    float camera =
      ((float)ray /
       (NUM_RAYS - 1)) -
      0.5f;

    float rayAngle =
      playerAngle +
      camera * FOV;

    RayHit hit =
      castRay(rayAngle);

    zBuffer[ray] =
      hit.distance;

    // Correzione fisheye
    float correctedDistance =
      hit.distance *
      cosf(rayAngle -
           playerAngle);

    if (correctedDistance <
        0.01f)
    {
      correctedDistance =
        0.01f;
    }

    int wallHeight =
      (int)(
        VIEW_H /
        correctedDistance);

    int startY =
      horizon -
      wallHeight / 2;

    int endY =
      horizon +
      wallHeight / 2;

    if (startY < 0)
      startY = 0;

    if (endY >= VIEW_H)
      endY = VIEW_H - 1;

    uint16_t baseColor;

    int variant =
      (hit.mapX +
       hit.mapY) % 3;

    if (variant == 0)
      baseColor =
        rgb565(150, 45, 40);

    else if (variant == 1)
      baseColor =
        rgb565(105, 105, 115);

    else
      baseColor =
        rgb565(105, 70, 40);

    float shade =
      1.0f /
      (1.0f +
       correctedDistance *
       0.16f);

    if (hit.side == 1)
      shade *= 0.72f;

    uint16_t wallColor =
      shadeColor(
        baseColor,
        shade);

    canvas.fillRect(
      ray * RAY_WIDTH,
      startY,
      RAY_WIDTH,
      endY - startY + 1,
      wallColor);
  }
}

// ============================================================
// NEMICI 3D
// ============================================================

void drawEnemies()
{
  // Ordine dal più lontano
  // al più vicino.

  int order[ENEMY_COUNT];

  for (int i = 0;
       i < ENEMY_COUNT;
       i++)
    order[i] = i;

  for (int i = 0;
       i < ENEMY_COUNT - 1;
       i++)
  {
    for (int j = i + 1;
         j < ENEMY_COUNT;
         j++)
    {
      float dx1 =
        enemies[order[i]].x -
        playerX;

      float dy1 =
        enemies[order[i]].y -
        playerY;

      float d1 =
        dx1 * dx1 +
        dy1 * dy1;

      float dx2 =
        enemies[order[j]].x -
        playerX;

      float dy2 =
        enemies[order[j]].y -
        playerY;

      float d2 =
        dx2 * dx2 +
        dy2 * dy2;

      if (d1 < d2)
      {
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
      }
    }
  }

  for (int o = 0;
       o < ENEMY_COUNT;
       o++)
  {
    int i = order[o];

    if (!enemies[i].alive)
      continue;

    float dx =
      enemies[i].x -
      playerX;

    float dy =
      enemies[i].y -
      playerY;

    float distance =
      sqrtf(dx * dx +
            dy * dy);

    float enemyAngle =
      atan2f(dy, dx);

    float diff =
      normalizeAngle(
        enemyAngle -
        playerAngle);

    if (fabsf(diff) >
        FOV * 0.65f)
      continue;

    // Posizione prospettica
    float projectedX =
      VIEW_W / 2.0f +
      tanf(diff) /
      tanf(FOV / 2.0f) *
      (VIEW_W / 2.0f);

    int spriteHeight =
      (int)(330.0f /
            distance);

    if (spriteHeight > 420)
      spriteHeight = 420;

    if (spriteHeight < 4)
      continue;

    int spriteWidth =
      (int)(spriteHeight *
            0.52f);

    int left =
      (int)projectedX -
      spriteWidth / 2;

    int right =
      left + spriteWidth;

    int top =
      VIEW_H / 2 -
      spriteHeight / 2;

    int bottom =
      top + spriteHeight;

    int headHeight =
      spriteHeight / 3;

    int headWidth =
      spriteWidth / 2;

    int headLeft =
      (int)projectedX -
      headWidth / 2;

    int headRight =
      headLeft +
      headWidth;

    // Corpo + testa disegnati
    // verticalmente per poter
    // essere nascosti dai muri.

    for (int x = left;
         x <= right;
         x += 2)
    {
      if (x < 0 ||
          x >= VIEW_W)
        continue;

      int rayIndex =
        x / RAY_WIDTH;

      if (rayIndex < 0)
        rayIndex = 0;

      if (rayIndex >= NUM_RAYS)
        rayIndex =
          NUM_RAYS - 1;

      if (distance >
          zBuffer[rayIndex])
        continue;

      // Corpo
      canvas.fillRect(
        x,
        top +
        headHeight - 5,
        2,
        bottom -
        (top + headHeight),
        COL_DARK_RED);

      // Testa
      if (x >= headLeft &&
          x <= headRight)
      {
        canvas.fillRect(
          x,
          top,
          2,
          headHeight,
          COL_SKIN);
      }
    }

    // Faccia
    int centerRay =
      ((int)projectedX) /
      RAY_WIDTH;

    if (centerRay >= 0 &&
        centerRay < NUM_RAYS)
    {
      if (distance <
          zBuffer[centerRay])
      {
        int eyeSize =
          spriteHeight / 35;

        if (eyeSize < 2)
          eyeSize = 2;

        int eyeY =
          top +
          headHeight / 3;

        canvas.fillRect(
          (int)projectedX -
          headWidth / 4,
          eyeY,
          eyeSize,
          eyeSize,
          COL_BLACK);

        canvas.fillRect(
          (int)projectedX +
          headWidth / 4 -
          eyeSize,
          eyeY,
          eyeSize,
          eyeSize,
          COL_BLACK);
      }
    }
  }
}

// ============================================================
// PISTOLA
// ============================================================

void drawWeapon()
{
  int center =
    VIEW_W / 2;

  int bottom =
    VIEW_H;

  // Mano
  canvas.fillRect(
    center - 55,
    bottom - 55,
    110,
    55,
    rgb565(150, 95, 65));

  // Corpo pistola
  canvas.fillRect(
    center - 34,
    bottom - 125,
    68,
    85,
    rgb565(75, 80, 85));

  // Canna
  canvas.fillRect(
    center - 20,
    bottom - 155,
    40,
    40,
    rgb565(110, 115, 120));

  // Parte scura
  canvas.fillRect(
    center - 15,
    bottom - 160,
    30,
    15,
    COL_BLACK);

  // Muzzle flash
  if (millis() <
      muzzleFlashUntil)
  {
    canvas.fillTriangle(
      center,
      bottom - 190,

      center - 25,
      bottom - 155,

      center + 25,
      bottom - 155,

      COL_YELLOW);
  }
}

// ============================================================
// MIRINO
// ============================================================

void drawCrosshair()
{
  int x = VIEW_W / 2;
  int y = VIEW_H / 2;

  canvas.drawFastHLine(
    x - 10,
    y,
    7,
    COL_WHITE);

  canvas.drawFastHLine(
    x + 4,
    y,
    7,
    COL_WHITE);

  canvas.drawFastVLine(
    x,
    y - 10,
    7,
    COL_WHITE);

  canvas.drawFastVLine(
    x,
    y + 4,
    7,
    COL_WHITE);

  canvas.fillCircle(
    x,
    y,
    2,
    COL_RED);
}

// ============================================================
// MINIMAP
// ============================================================

void drawMinimap()
{
  constexpr int SCALE = 4;

  int mapPixelW =
    MAP_W * SCALE;

  int mapPixelH =
    MAP_H * SCALE;

  int originX =
    VIEW_W -
    mapPixelW -
    8;

  int originY = 8;

  canvas.fillRect(
    originX - 3,
    originY - 3,
    mapPixelW + 6,
    mapPixelH + 6,
    COL_BLACK);

  for (int y = 0;
       y < MAP_H;
       y++)
  {
    for (int x = 0;
         x < MAP_W;
         x++)
    {
      uint16_t color =
        WORLD[y][x] == '#'
        ? rgb565(110, 110, 110)
        : rgb565(25, 25, 25);

      canvas.fillRect(
        originX +
        x * SCALE,

        originY +
        y * SCALE,

        SCALE - 1,
        SCALE - 1,

        color);
    }
  }

  // Nemici
  for (int i = 0;
       i < ENEMY_COUNT;
       i++)
  {
    if (!enemies[i].alive)
      continue;

    canvas.fillCircle(
      originX +
      (int)(enemies[i].x *
            SCALE),

      originY +
      (int)(enemies[i].y *
            SCALE),

      2,
      COL_RED);
  }

  // Player
  int px =
    originX +
    (int)(playerX *
          SCALE);

  int py =
    originY +
    (int)(playerY *
          SCALE);

  canvas.fillCircle(
    px,
    py,
    2,
    COL_GREEN);

  canvas.drawLine(
    px,
    py,

    px +
    cosf(playerAngle) * 7,

    py +
    sinf(playerAngle) * 7,

    COL_GREEN);
}

// ============================================================
// HUD
// ============================================================

void drawHUD()
{
  canvas.setTextColor(
    COL_WHITE,
    COL_BLACK);

  canvas.setTextSize(2);

  canvas.setTextDatum(
    textdatum_t::top_left);

  char buffer[32];

  snprintf(
    buffer,
    sizeof(buffer),
    "HP %03d",
    health);

  canvas.drawString(
    buffer,
    10,
    10);

  snprintf(
    buffer,
    sizeof(buffer),
    "KILLS %d/%d",
    kills,
    ENEMY_COUNT);

  canvas.drawString(
    buffer,
    10,
    35);
}

// ============================================================
// RENDER COMPLETO
// ============================================================

void renderGame()
{
  drawWorld();
  drawEnemies();
  drawCrosshair();
  drawWeapon();
  drawMinimap();
  drawHUD();

  canvas.pushSprite(0, 0);
}

// ============================================================
// TOUCH CONTROLS
// ============================================================

bool fireTouch = false;

void processControls(
  float dt,
  bool touched,
  int tx,
  int ty)
{
  fireTouch = false;

  if (!touched)
    return;

  if (ty < CONTROL_Y)
    return;

  // ========================================================
  // FIRE
  // ========================================================

  float fireDX =
    tx - 390;

  float fireDY =
    ty - 720;

  float fireDistance =
    sqrtf(
      fireDX * fireDX +
      fireDY * fireDY);

  if (fireDistance < 62)
  {
    fireTouch = true;
    shoot();
    return;
  }

  // ========================================================
  // JOYSTICK
  // ========================================================

  if (tx < 275)
  {
    float dx =
      (tx - 130) /
      85.0f;

    float dy =
      (ty - 720) /
      60.0f;

    if (dx < -1)
      dx = -1;

    if (dx > 1)
      dx = 1;

    if (dy < -1)
      dy = -1;

    if (dy > 1)
      dy = 1;

    // Deadzone
    if (fabsf(dx) < 0.15f)
      dx = 0;

    if (fabsf(dy) < 0.15f)
      dy = 0;

    // Destra / sinistra
    playerAngle +=
      dx *
      TURN_SPEED *
      dt;

    // Su = avanti
    movePlayer(
      -dy *
      MOVE_SPEED *
      dt);

    playerAngle =
      normalizeAngle(
        playerAngle);
  }
}

// ============================================================
// DISEGNO CONTROLLI
// ============================================================

void drawControls(
  bool touched,
  int tx,
  int ty)
{
  lcd.fillRect(
    0,
    CONTROL_Y,
    SCREEN_W,
    CONTROL_H,
    rgb565(18, 18, 20));

  lcd.drawFastHLine(
    0,
    CONTROL_Y,
    SCREEN_W,
    rgb565(80, 80, 80));

  // ========================================================
  // JOYSTICK
  // ========================================================

  constexpr int joyX = 130;
  constexpr int joyY = 720;
  constexpr int joyR = 62;

  lcd.drawCircle(
    joyX,
    joyY,
    joyR,
    rgb565(120, 120, 120));

  lcd.drawCircle(
    joyX,
    joyY,
    joyR - 1,
    rgb565(80, 80, 80));

  int knobX = joyX;
  int knobY = joyY;

  if (touched &&
      ty >= CONTROL_Y &&
      tx < 275)
  {
    float dx =
      tx - joyX;

    float dy =
      ty - joyY;

    float d =
      sqrtf(dx * dx +
            dy * dy);

    if (d > joyR - 20)
    {
      dx =
        dx / d *
        (joyR - 20);

      dy =
        dy / d *
        (joyR - 20);
    }

    knobX =
      joyX + (int)dx;

    knobY =
      joyY + (int)dy;
  }

  lcd.fillCircle(
    knobX,
    knobY,
    22,
    rgb565(100, 100, 110));

  lcd.drawCircle(
    knobX,
    knobY,
    22,
    COL_WHITE);

  // ========================================================
  // FIRE
  // ========================================================

  uint16_t fireColor =
    fireTouch
    ? rgb565(255, 80, 45)
    : rgb565(170, 30, 25);

  lcd.fillCircle(
    390,
    720,
    58,
    fireColor);

  lcd.drawCircle(
    390,
    720,
    58,
    COL_WHITE);

  lcd.drawCircle(
    390,
    720,
    57,
    COL_WHITE);

  lcd.setTextDatum(
    textdatum_t::middle_center);

  lcd.setTextColor(
    COL_WHITE);

  lcd.setTextSize(2);

  lcd.drawString(
    "FIRE",
    390,
    720);
}

// ============================================================
// GAME STATE
// ============================================================

bool gameOver = false;
bool victory = false;
bool previousTouch = false;

void resetGame()
{
  playerX = 2.5f;
  playerY = 2.5f;
  playerAngle = 0.0f;

  health = 100;
  kills = 0;

  for (int i = 0;
       i < ENEMY_COUNT;
       i++)
  {
    enemies[i].x =
      ENEMY_START[i][0];

    enemies[i].y =
      ENEMY_START[i][1];

    enemies[i].alive = true;
  }

  gameOver = false;
  victory = false;

  lastEnemyDamage = 0;
  lastShot = 0;
}

// ============================================================
// END SCREEN
// ============================================================

void drawEndScreen()
{
  renderGame();

  canvas.fillRect(
    40,
    220,
    400,
    190,
    rgb565(5, 5, 5));

  canvas.drawRect(
    40,
    220,
    400,
    190,
    victory
      ? COL_GREEN
      : COL_RED);

  canvas.drawRect(
    41,
    221,
    398,
    188,
    victory
      ? COL_GREEN
      : COL_RED);

  canvas.setTextDatum(
    textdatum_t::middle_center);

  canvas.setTextSize(3);

  canvas.setTextColor(
    victory
      ? COL_GREEN
      : COL_RED);

  if (victory)
  {
    canvas.drawString(
      "AREA CLEARED",
      VIEW_W / 2,
      270);
  }
  else
  {
    canvas.drawString(
      "YOU DIED",
      VIEW_W / 2,
      270);
  }

  canvas.setTextSize(2);

  canvas.setTextColor(
    COL_WHITE);

  canvas.drawString(
    "TOCCA PER RIPARTIRE",
    VIEW_W / 2,
    350);

  canvas.pushSprite(0, 0);
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  lcd.init();

  // La tua configurazione touch
  // è stata fatta per questa rotazione.
  lcd.setRotation(3);

  lcd.setBrightness(255);

  lcd.fillScreen(COL_BLACK);

  Serial.printf(
    "Display: %d x %d\n",
    lcd.width(),
    lcd.height());

  // ========================================================
  // FRAMEBUFFER PSRAM
  // ========================================================

  canvas.setPsram(true);
  canvas.setColorDepth(16);

  if (!canvas.createSprite(
        VIEW_W,
        VIEW_H))
  {
    Serial.println(
      "ERRORE: impossibile creare framebuffer.");

    while (true)
      delay(1000);
  }

  resetGame();

  lcd.setTextDatum(
    textdatum_t::middle_center);

  lcd.setTextColor(
    COL_WHITE);

  lcd.setTextSize(2);

  lcd.drawString(
    "ESP32 DOOM-LIKE",
    SCREEN_W / 2,
    SCREEN_H / 2);

  delay(700);
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  static uint32_t previousTime =
    millis();

  uint32_t now =
    millis();

  float dt =
    (now - previousTime) /
    1000.0f;

  previousTime = now;

  // Evita salti enormi se il sistema
  // rimane occupato.
  if (dt > 0.05f)
    dt = 0.05f;

  // ========================================================
  // TOUCH
  // ========================================================

  uint16_t tx = 0;
  uint16_t ty = 0;

  bool touched =
    lcd.getTouch(&tx, &ty);

  // ========================================================
  // GAME OVER / VITTORIA
  // ========================================================

  if (gameOver || victory)
  {
    drawEndScreen();

    drawControls(
      touched,
      tx,
      ty);

    // Serve un nuovo tap
    if (touched &&
        !previousTouch)
    {
      resetGame();
    }

    previousTouch =
      touched;

    delay(1);
    return;
  }

  // ========================================================
  // INPUT
  // ========================================================

  processControls(
    dt,
    touched,
    tx,
    ty);

  // ========================================================
  // AI
  // ========================================================

  updateEnemies(dt);

  // ========================================================
  // STATO
  // ========================================================

  if (health <= 0)
    gameOver = true;

  if (kills >= ENEMY_COUNT)
    victory = true;

  // ========================================================
  // RENDER
  // ========================================================

  renderGame();

  drawControls(
    touched,
    tx,
    ty);

  previousTouch =
    touched;

  delay(1);
}