#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <driver/spi_master.h>
#include <math.h>
#include <string.h>

// =======================================================
// OPZIONI PREPROCESSORE
// =======================================================

#define USE_DEPTH_BUFFER 1
#define DRAW_FACE_OUTLINES 0

// =======================================================
// CONFIGURAZIONE DISPLAY - LA TUA
// =======================================================

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;

public:
  LGFX(void) {
    { // SPI Bus DISPLAY
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc   = 2;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // TFT Panel
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 15;
      cfg.pin_rst          = -1;
      cfg.pin_busy         = -1;
      cfg.memory_width     = 240;
      cfg.memory_height    = 320;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }

    { // Backlight
      auto cfg = _light_instance.config();
      cfg.pin_bl = 21;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    { // Touch
      auto cfg = _touch_instance.config();
      cfg.x_min      = 652;
      cfg.x_max      = 3620;
      cfg.y_min      = 350;
      cfg.y_max      = 3750;
      cfg.pin_int    = -1;
      cfg.bus_shared = true;
      cfg.offset_rotation = 4;
      cfg.spi_host   = SPI3_HOST;
      cfg.freq       = 1000000;
      cfg.pin_sclk   = 25;
      cfg.pin_mosi   = 32;
      cfg.pin_miso   = 39;
      cfg.pin_cs     = 33;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

// =======================================================
// GLOBALI DISPLAY
// =======================================================

LGFX tft;
LGFX_Sprite canvas(&tft);

// =======================================================
// COSTANTI
// =======================================================

constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 320;

constexpr int TEX_SIZE = 8;

constexpr int WORLD_X = 14;
constexpr int WORLD_Z = 14;
constexpr int WORLD_MAX_Y = 9;

constexpr int MAX_WORLD_FACES = 1800;

constexpr float EYE_HEIGHT = 1.62f;
constexpr float MOVE_SPEED = 3.4f;
constexpr float LOOK_SPEED = 0.009f;

constexpr float RENDER_DISTANCE = 13.0f;
constexpr float RENDER_DISTANCE_SQ = RENDER_DISTANCE * RENDER_DISTANCE;

#if USE_DEPTH_BUFFER
static uint8_t zBuffer[SCREEN_W * SCREEN_H];
#endif

// Controlli touch
constexpr int CTRL_SIZE = 36;

constexpr int BTN_W_X = 52;
constexpr int BTN_W_Y = 225;

constexpr int BTN_A_X = 10;
constexpr int BTN_A_Y = 269;

constexpr int BTN_S_X = 52;
constexpr int BTN_S_Y = 269;

constexpr int BTN_D_X = 94;
constexpr int BTN_D_Y = 269;

// =======================================================
// STRUTTURE
// =======================================================

struct Vec2f
{
  float x;
  float y;
};

struct Vec3
{
  float x;
  float y;
  float z;
};

struct Vec4
{
  float x;
  float y;
  float z;
  float w;
};

struct UV
{
  float u;
  float v;
};

struct Mat4
{
  float m[4][4];
};

struct ScreenVertex
{
  float x;
  float y;
  float z;
};

enum BlockType
{
  BLOCK_AIR,
  BLOCK_GRASS,
  BLOCK_DIRT,
  BLOCK_STONE,
  BLOCK_WOOD,
  BLOCK_LEAVES
};

enum TextureKind
{
  TEX_GRASS_TOP,
  TEX_GRASS_SIDE,
  TEX_DIRT,
  TEX_STONE,
  TEX_WOOD_SIDE,
  TEX_WOOD_TOP,
  TEX_LEAVES
};

enum FaceDir
{
  FACE_FRONT,
  FACE_BACK,
  FACE_RIGHT,
  FACE_LEFT,
  FACE_TOP,
  FACE_BOTTOM
};

enum TouchControl
{
  CTRL_NONE,
  CTRL_FORWARD,
  CTRL_BACKWARD,
  CTRL_LEFT,
  CTRL_RIGHT
};

struct TreeDef
{
  int x;
  int z;
};

// Versione super compatta.
// Prima salvavamo 4 Vec4 + centro + normale: troppa RAM.
// Ora salviamo solo posizione blocco + direzione + texture.
struct WorldFace
{
  uint8_t x;
  uint8_t y;
  uint8_t z;
  uint8_t dir;
  uint8_t tex;
};

// =======================================================
// MONDO
// =======================================================

const int terrainHeights[WORLD_Z][WORLD_X] =
{
  {2,2,2,3,3,2,2,1,1,2,2,3,2,2},
  {2,2,3,3,4,3,2,2,1,2,3,3,3,2},
  {2,3,3,4,4,3,2,2,2,2,3,4,3,2},
  {2,3,4,4,4,3,3,2,2,3,3,3,2,2},
  {2,2,3,4,3,3,2,2,3,3,4,3,2,1},
  {1,2,3,3,3,2,2,2,3,4,4,3,2,1},
  {1,2,2,3,2,2,1,2,3,3,3,2,2,1},
  {1,1,2,2,2,1,1,2,2,3,2,2,1,1},
  {2,1,1,2,2,2,2,2,2,2,2,1,1,1},
  {2,2,1,1,2,3,3,2,1,1,2,2,1,1},
  {2,2,2,2,3,3,4,3,2,2,2,2,2,1},
  {2,3,3,2,3,4,4,3,2,2,1,2,2,2},
  {2,2,3,2,2,3,3,2,2,2,1,1,2,2},
  {2,2,2,2,2,3,3,2,2,2,1,1,2,2}
};

const TreeDef trees[] =
{
  {3, 3},
  {10, 4},
  {5, 10},
  {11, 10}
};

constexpr int TREE_COUNT = sizeof(trees) / sizeof(trees[0]);

WorldFace worldFaces[MAX_WORLD_FACES];
int worldFaceCount = 0;

// =======================================================
// CAMERA
// =======================================================

float cameraX = 7.0f;
float cameraY = 4.0f;
float cameraZ = 12.4f;

float cameraYaw = 0.0f;
float cameraPitch = -0.22f;

bool wasTouching = false;
uint16_t lastTouchX = 0;
uint16_t lastTouchY = 0;

unsigned long lastFrameTime = 0;
unsigned long lastUpdateTime = 0;

int lastVisibleFaces = 0;
int lastDrawnPixels = 0;

// =======================================================
// COLORI
// =======================================================

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t COL_SKY_TOP       = rgb565(90, 180, 255);
uint16_t COL_SKY_BOTTOM    = rgb565(170, 225, 255);
uint16_t COL_GROUND_FAR    = rgb565(95, 170, 80);
uint16_t COL_GROUND_NEAR   = rgb565(55, 115, 55);
uint16_t COL_UI_BG         = rgb565(45, 45, 45);
uint16_t COL_UI_BORDER     = rgb565(245, 245, 245);
uint16_t COL_UI_TEXT       = rgb565(255, 255, 255);
uint16_t COL_OUTLINE       = rgb565(26, 22, 20);

// =======================================================
// UTILITY
// =======================================================

float clampf(float v, float minV, float maxV)
{
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

int clampi(int v, int minV, int maxV)
{
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

uint16_t shade565(uint16_t color, float factor)
{
  factor = clampf(factor, 0.0f, 2.0f);

  uint8_t r5 = (color >> 11) & 0x1F;
  uint8_t g6 = (color >> 5)  & 0x3F;
  uint8_t b5 = color & 0x1F;

  uint8_t r = (r5 * 255) / 31;
  uint8_t g = (g6 * 255) / 63;
  uint8_t b = (b5 * 255) / 31;

  r = (uint8_t)clampf(r * factor, 0, 255);
  g = (uint8_t)clampf(g * factor, 0, 255);
  b = (uint8_t)clampf(b * factor, 0, 255);

  return rgb565(r, g, b);
}

uint16_t mix565(uint16_t a, uint16_t b, float t)
{
  t = clampf(t, 0.0f, 1.0f);

  uint8_t ar = ((a >> 11) & 0x1F) * 255 / 31;
  uint8_t ag = ((a >> 5)  & 0x3F) * 255 / 63;
  uint8_t ab = (a & 0x1F) * 255 / 31;

  uint8_t br = ((b >> 11) & 0x1F) * 255 / 31;
  uint8_t bg = ((b >> 5)  & 0x3F) * 255 / 63;
  uint8_t bb = (b & 0x1F) * 255 / 31;

  uint8_t r = ar + (br - ar) * t;
  uint8_t g = ag + (bg - ag) * t;
  uint8_t bl = ab + (bb - ab) * t;

  return rgb565(r, g, bl);
}

Vec3 sub3(const Vec3& a, const Vec3& b)
{
  return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 cross3(const Vec3& a, const Vec3& b)
{
  return {
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  };
}

float dot3(const Vec3& a, const Vec3& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length3(const Vec3& v)
{
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 normalize3(const Vec3& v)
{
  float len = length3(v);
  if (len < 0.00001f) return {0, 0, 0};
  return { v.x / len, v.y / len, v.z / len };
}

Vec3 toVec3(const Vec4& v)
{
  return { v.x, v.y, v.z };
}

float edgeFunction(const Vec2f& a, const Vec2f& b, const Vec2f& c)
{
  return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

// =======================================================
// MATRICI
// =======================================================

Mat4 identity()
{
  Mat4 r = {};
  for (int i = 0; i < 4; i++)
  {
    r.m[i][i] = 1.0f;
  }
  return r;
}

Mat4 multiplyMat4(const Mat4& a, const Mat4& b)
{
  Mat4 r = {};

  for (int row = 0; row < 4; row++)
  {
    for (int col = 0; col < 4; col++)
    {
      r.m[row][col] =
        a.m[row][0] * b.m[0][col] +
        a.m[row][1] * b.m[1][col] +
        a.m[row][2] * b.m[2][col] +
        a.m[row][3] * b.m[3][col];
    }
  }

  return r;
}

Vec4 multiplyMat4Vec4(const Mat4& m, const Vec4& v)
{
  Vec4 r;

  r.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
  r.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
  r.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
  r.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;

  return r;
}

Mat4 translation(float x, float y, float z)
{
  Mat4 r = identity();

  r.m[0][3] = x;
  r.m[1][3] = y;
  r.m[2][3] = z;

  return r;
}

Mat4 rotationX(float angle)
{
  Mat4 r = identity();

  float c = cosf(angle);
  float s = sinf(angle);

  r.m[1][1] = c;
  r.m[1][2] = -s;
  r.m[2][1] = s;
  r.m[2][2] = c;

  return r;
}

Mat4 rotationY(float angle)
{
  Mat4 r = identity();

  float c = cosf(angle);
  float s = sinf(angle);

  r.m[0][0] = c;
  r.m[0][2] = s;
  r.m[2][0] = -s;
  r.m[2][2] = c;

  return r;
}

Mat4 perspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
{
  Mat4 r = {};

  float fovRad = fovDegrees * 0.01745329252f;
  float f = 1.0f / tanf(fovRad * 0.5f);

  r.m[0][0] = f / aspect;
  r.m[1][1] = f;
  r.m[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
  r.m[2][3] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
  r.m[3][2] = -1.0f;

  return r;
}

Mat4 buildViewMatrix()
{
  Mat4 t = translation(-cameraX, -cameraY, -cameraZ);
  Mat4 ry = rotationY(cameraYaw);
  Mat4 rx = rotationX(-cameraPitch);

  return multiplyMat4(rx, multiplyMat4(ry, t));
}

// =======================================================
// TEXTURE
// =======================================================

uint16_t grassTopPalette[4] =
{
  rgb565(126, 180, 58),
  rgb565(105, 165, 48),
  rgb565(82, 145, 40),
  rgb565(150, 195, 70)
};

uint16_t grassSidePalette[6] =
{
  rgb565(115, 175, 55),
  rgb565(92, 155, 45),
  rgb565(70, 130, 38),
  rgb565(135, 96, 65),
  rgb565(115, 78, 52),
  rgb565(95, 63, 42)
};

uint16_t dirtPalette[4] =
{
  rgb565(140, 100, 70),
  rgb565(120, 82, 55),
  rgb565(100, 66, 44),
  rgb565(155, 112, 78)
};

uint16_t stonePalette[4] =
{
  rgb565(120, 120, 120),
  rgb565(100, 100, 100),
  rgb565(145, 145, 145),
  rgb565(80, 80, 80)
};

uint16_t woodPalette[4] =
{
  rgb565(120, 75, 35),
  rgb565(150, 95, 45),
  rgb565(95, 55, 25),
  rgb565(175, 120, 65)
};

uint16_t leavesPalette[4] =
{
  rgb565(55, 130, 45),
  rgb565(45, 110, 35),
  rgb565(75, 155, 55),
  rgb565(35, 90, 30)
};

const uint8_t GRASS_TOP_TEX[TEX_SIZE][TEX_SIZE] =
{
  {0,1,0,2,1,0,3,1},
  {1,0,2,1,0,1,0,2},
  {0,3,1,0,2,1,0,1},
  {2,1,0,1,3,0,2,0},
  {1,0,2,0,1,2,1,3},
  {0,1,0,3,2,0,1,0},
  {3,2,1,0,1,0,2,1},
  {1,0,1,2,0,3,1,0}
};

const uint8_t GRASS_SIDE_TEX[TEX_SIZE][TEX_SIZE] =
{
  {0,1,0,1,2,0,1,0},
  {1,0,2,0,1,1,0,2},
  {2,1,3,1,0,2,1,0},
  {3,4,3,5,3,4,5,3},
  {4,3,5,4,3,3,4,5},
  {3,5,4,3,5,4,3,4},
  {5,4,3,4,5,3,4,3},
  {4,3,4,5,3,4,5,4}
};

const uint8_t DIRT_TEX[TEX_SIZE][TEX_SIZE] =
{
  {0,1,0,2,1,0,3,1},
  {1,2,1,0,2,1,0,2},
  {0,1,3,1,0,2,1,0},
  {2,0,1,2,1,0,2,1},
  {1,3,0,1,2,1,0,2},
  {0,1,2,0,1,3,1,0},
  {2,0,1,3,0,1,2,1},
  {1,2,0,1,2,0,1,3}
};

uint16_t sampleTextureBase(TextureKind tex, int x, int y)
{
  x = clampi(x, 0, TEX_SIZE - 1);
  y = clampi(y, 0, TEX_SIZE - 1);

  if (tex == TEX_GRASS_TOP)
  {
    return grassTopPalette[GRASS_TOP_TEX[y][x]];
  }

  if (tex == TEX_GRASS_SIDE)
  {
    return grassSidePalette[GRASS_SIDE_TEX[y][x]];
  }

  if (tex == TEX_DIRT)
  {
    return dirtPalette[DIRT_TEX[y][x]];
  }

  if (tex == TEX_STONE)
  {
    int n = (x * 13 + y * 7 + x * y * 3) & 3;
    return stonePalette[n];
  }

  if (tex == TEX_WOOD_SIDE)
  {
    if (x == 2 || x == 5)
    {
      return woodPalette[2];
    }

    int n = (x * 5 + y * 11) & 3;
    return woodPalette[n];
  }

  if (tex == TEX_WOOD_TOP)
  {
    int dx = x - 3;
    int dy = y - 3;
    int d = dx * dx + dy * dy;

    if (d < 3) return woodPalette[3];
    if (d < 9) return woodPalette[1];
    return woodPalette[0];
  }

  int n = (x * 9 + y * 5 + x * y) & 3;
  return leavesPalette[n];
}

void buildShadedTexture(TextureKind tex, float brightness, uint16_t outTex[TEX_SIZE * TEX_SIZE])
{
  for (int y = 0; y < TEX_SIZE; y++)
  {
    for (int x = 0; x < TEX_SIZE; x++)
    {
      uint16_t base = sampleTextureBase(tex, x, y);
      outTex[y * TEX_SIZE + x] = shade565(base, brightness);
    }
  }
}

inline uint16_t sampleCachedTexture(uint16_t texCache[TEX_SIZE * TEX_SIZE], float u, float v)
{
  if (u < 0.0f) u = 0.0f;
  if (u > 1.0f) u = 1.0f;

  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;

  int tx = (int)(u * (TEX_SIZE - 1) + 0.5f);
  int ty = (int)(v * (TEX_SIZE - 1) + 0.5f);

  return texCache[ty * TEX_SIZE + tx];
}

// =======================================================
// WORLD LOGIC
// =======================================================

bool insideWorldXZ(int x, int z)
{
  return x >= 0 && x < WORLD_X && z >= 0 && z < WORLD_Z;
}

int getTerrainHeightCell(int x, int z)
{
  x = clampi(x, 0, WORLD_X - 1);
  z = clampi(z, 0, WORLD_Z - 1);

  return terrainHeights[z][x];
}

int getTerrainHeightWorld(float wx, float wz)
{
  int x = (int)floorf(wx);
  int z = (int)floorf(wz);

  x = clampi(x, 0, WORLD_X - 1);
  z = clampi(z, 0, WORLD_Z - 1);

  return terrainHeights[z][x];
}

BlockType blockAt(int x, int y, int z)
{
  if (!insideWorldXZ(x, z)) return BLOCK_AIR;
  if (y < 0 || y >= WORLD_MAX_Y) return BLOCK_AIR;

  int h = terrainHeights[z][x];

  if (y < h)
  {
    if (y == h - 1) return BLOCK_GRASS;
    if (y >= h - 3) return BLOCK_DIRT;
    return BLOCK_STONE;
  }

  for (int i = 0; i < TREE_COUNT; i++)
  {
    int tx = trees[i].x;
    int tz = trees[i].z;

    int base = terrainHeights[tz][tx];

    if (x == tx && z == tz && y >= base && y <= base + 3)
    {
      return BLOCK_WOOD;
    }

    if (y >= base + 2 && y <= base + 5)
    {
      int dx = abs(x - tx);
      int dz = abs(z - tz);

      if (dx <= 2 && dz <= 2)
      {
        if (y == base + 5 && dx == 2 && dz == 2)
        {
          continue;
        }

        return BLOCK_LEAVES;
      }
    }
  }

  return BLOCK_AIR;
}

bool blockExists(int x, int y, int z)
{
  return blockAt(x, y, z) != BLOCK_AIR;
}

TextureKind textureForFace(BlockType block, FaceDir dir)
{
  if (block == BLOCK_GRASS)
  {
    if (dir == FACE_TOP) return TEX_GRASS_TOP;
    if (dir == FACE_BOTTOM) return TEX_DIRT;
    return TEX_GRASS_SIDE;
  }

  if (block == BLOCK_DIRT) return TEX_DIRT;
  if (block == BLOCK_STONE) return TEX_STONE;

  if (block == BLOCK_WOOD)
  {
    if (dir == FACE_TOP || dir == FACE_BOTTOM) return TEX_WOOD_TOP;
    return TEX_WOOD_SIDE;
  }

  if (block == BLOCK_LEAVES) return TEX_LEAVES;

  return TEX_DIRT;
}

bool isWalkBlocked(float wx, float wz)
{
  int x = (int)floorf(wx);
  int z = (int)floorf(wz);

  if (!insideWorldXZ(x, z))
  {
    return true;
  }

  int h = getTerrainHeightCell(x, z);

  if (blockAt(x, h, z) != BLOCK_AIR)
  {
    return true;
  }

  if (blockAt(x, h + 1, z) != BLOCK_AIR)
  {
    return true;
  }

  return false;
}

// =======================================================
// FACCE COMPATTE
// =======================================================

Vec3 getFaceNormal(FaceDir dir)
{
  switch (dir)
  {
    case FACE_FRONT:  return { 0.0f,  0.0f,  1.0f };
    case FACE_BACK:   return { 0.0f,  0.0f, -1.0f };
    case FACE_RIGHT:  return { 1.0f,  0.0f,  0.0f };
    case FACE_LEFT:   return {-1.0f,  0.0f,  0.0f };
    case FACE_TOP:    return { 0.0f,  1.0f,  0.0f };
    case FACE_BOTTOM: return { 0.0f, -1.0f,  0.0f };
  }

  return {0.0f, 1.0f, 0.0f};
}

Vec3 getFaceCenter(uint8_t x, uint8_t y, uint8_t z, FaceDir dir)
{
  switch (dir)
  {
    case FACE_FRONT:  return {x + 0.5f, y + 0.5f, z + 1.0f};
    case FACE_BACK:   return {x + 0.5f, y + 0.5f, z + 0.0f};
    case FACE_RIGHT:  return {x + 1.0f, y + 0.5f, z + 0.5f};
    case FACE_LEFT:   return {x + 0.0f, y + 0.5f, z + 0.5f};
    case FACE_TOP:    return {x + 0.5f, y + 1.0f, z + 0.5f};
    case FACE_BOTTOM: return {x + 0.5f, y + 0.0f, z + 0.5f};
  }

  return {x + 0.5f, y + 0.5f, z + 0.5f};
}

void getFaceVertices(const WorldFace& f, Vec4& a, Vec4& b, Vec4& c, Vec4& d)
{
  float x0 = f.x;
  float x1 = f.x + 1.0f;

  float y0 = f.y;
  float y1 = f.y + 1.0f;

  float z0 = f.z;
  float z1 = f.z + 1.0f;

  FaceDir dir = (FaceDir)f.dir;

  switch (dir)
  {
    case FACE_FRONT:
      a = {x0, y0, z1, 1};
      b = {x1, y0, z1, 1};
      c = {x1, y1, z1, 1};
      d = {x0, y1, z1, 1};
      break;

    case FACE_BACK:
      a = {x1, y0, z0, 1};
      b = {x0, y0, z0, 1};
      c = {x0, y1, z0, 1};
      d = {x1, y1, z0, 1};
      break;

    case FACE_RIGHT:
      a = {x1, y0, z1, 1};
      b = {x1, y0, z0, 1};
      c = {x1, y1, z0, 1};
      d = {x1, y1, z1, 1};
      break;

    case FACE_LEFT:
      a = {x0, y0, z0, 1};
      b = {x0, y0, z1, 1};
      c = {x0, y1, z1, 1};
      d = {x0, y1, z0, 1};
      break;

    case FACE_TOP:
      a = {x0, y1, z1, 1};
      b = {x1, y1, z1, 1};
      c = {x1, y1, z0, 1};
      d = {x0, y1, z0, 1};
      break;

    case FACE_BOTTOM:
      a = {x0, y0, z0, 1};
      b = {x1, y0, z0, 1};
      c = {x1, y0, z1, 1};
      d = {x0, y0, z1, 1};
      break;
  }
}

// =======================================================
// BUILD STATIC WORLD MESH
// =======================================================

void addWorldFace(uint8_t x, uint8_t y, uint8_t z, FaceDir dir, TextureKind tex)
{
  if (worldFaceCount >= MAX_WORLD_FACES)
  {
    return;
  }

  WorldFace& f = worldFaces[worldFaceCount++];

  f.x = x;
  f.y = y;
  f.z = z;
  f.dir = (uint8_t)dir;
  f.tex = (uint8_t)tex;
}

void addBlockToMesh(int x, int y, int z)
{
  BlockType block = blockAt(x, y, z);

  if (block == BLOCK_AIR)
  {
    return;
  }

  if (!blockExists(x, y, z + 1))
  {
    addWorldFace(x, y, z, FACE_FRONT, textureForFace(block, FACE_FRONT));
  }

  if (!blockExists(x, y, z - 1))
  {
    addWorldFace(x, y, z, FACE_BACK, textureForFace(block, FACE_BACK));
  }

  if (!blockExists(x + 1, y, z))
  {
    addWorldFace(x, y, z, FACE_RIGHT, textureForFace(block, FACE_RIGHT));
  }

  if (!blockExists(x - 1, y, z))
  {
    addWorldFace(x, y, z, FACE_LEFT, textureForFace(block, FACE_LEFT));
  }

  if (!blockExists(x, y + 1, z))
  {
    addWorldFace(x, y, z, FACE_TOP, textureForFace(block, FACE_TOP));
  }

  if (!blockExists(x, y - 1, z))
  {
    addWorldFace(x, y, z, FACE_BOTTOM, textureForFace(block, FACE_BOTTOM));
  }
}

void buildWorldMesh()
{
  worldFaceCount = 0;

  for (int z = 0; z < WORLD_Z; z++)
  {
    for (int x = 0; x < WORLD_X; x++)
    {
      for (int y = 0; y < WORLD_MAX_Y; y++)
      {
        addBlockToMesh(x, y, z);
      }
    }
  }

  Serial.print("World faces: ");
  Serial.println(worldFaceCount);
}

// =======================================================
// Z-BUFFER
// =======================================================

void clearDepthBuffer()
{
#if USE_DEPTH_BUFFER
  memset(zBuffer, 255, sizeof(zBuffer));
#endif
}

inline uint8_t depthToByte(float z)
{
  int v = (int)(z * 8.0f);

  if (v < 0) v = 0;
  if (v > 254) v = 254;

  return (uint8_t)v;
}

inline void drawPixelDepth(int x, int y, uint16_t color, float z)
{
  if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H)
  {
    return;
  }

#if USE_DEPTH_BUFFER
  int idx = y * SCREEN_W + x;
  uint8_t zd = depthToByte(z);

  if (zd >= zBuffer[idx])
  {
    return;
  }

  zBuffer[idx] = zd;
#endif

  canvas.drawPixel(x, y, color);
  lastDrawnPixels++;
}

// =======================================================
// PROIEZIONE
// =======================================================

bool projectToScreen(const Vec4& clip, const Vec4& view, ScreenVertex& out)
{
  if (clip.w <= 0.025f)
  {
    return false;
  }

  float ndcX = clip.x / clip.w;
  float ndcY = clip.y / clip.w;

  out.x = (ndcX + 1.0f) * 0.5f * SCREEN_W;
  out.y = (1.0f - ndcY) * 0.5f * SCREEN_H;
  out.z = -view.z;

  if (out.z <= 0.02f)
  {
    return false;
  }

  return true;
}

// =======================================================
// RASTERIZER TEXTURE + Z-BUFFER
// =======================================================

void drawTexturedTriangleFast(
  const ScreenVertex& a,
  const ScreenVertex& b,
  const ScreenVertex& c,
  const UV& ta,
  const UV& tb,
  const UV& tc,
  uint16_t texCache[TEX_SIZE * TEX_SIZE])
{
  Vec2f pa = { a.x, a.y };
  Vec2f pb = { b.x, b.y };
  Vec2f pc = { c.x, c.y };

  float minXf = floorf(fminf(pa.x, fminf(pb.x, pc.x)));
  float minYf = floorf(fminf(pa.y, fminf(pb.y, pc.y)));
  float maxXf = ceilf (fmaxf(pa.x, fmaxf(pb.x, pc.x)));
  float maxYf = ceilf (fmaxf(pa.y, fmaxf(pb.y, pc.y)));

  int minX = (int)clampf(minXf, 0, SCREEN_W - 1);
  int minY = (int)clampf(minYf, 0, SCREEN_H - 1);
  int maxX = (int)clampf(maxXf, 0, SCREEN_W - 1);
  int maxY = (int)clampf(maxYf, 0, SCREEN_H - 1);

  float area = edgeFunction(pa, pb, pc);

  if (fabs(area) < 0.0001f)
  {
    return;
  }

  float invArea = 1.0f / area;

  float w0Dx = pc.y - pb.y;
  float w0Dy = -(pc.x - pb.x);

  float w1Dx = pa.y - pc.y;
  float w1Dy = -(pa.x - pc.x);

  float w2Dx = pb.y - pa.y;
  float w2Dy = -(pb.x - pa.x);

  Vec2f start = { minX + 0.5f, minY + 0.5f };

  float w0Row = edgeFunction(pb, pc, start);
  float w1Row = edgeFunction(pc, pa, start);
  float w2Row = edgeFunction(pa, pb, start);

  for (int y = minY; y <= maxY; y++)
  {
    float w0 = w0Row;
    float w1 = w1Row;
    float w2 = w2Row;

    for (int x = minX; x <= maxX; x++)
    {
      bool inside =
        (w0 >= 0 && w1 >= 0 && w2 >= 0) ||
        (w0 <= 0 && w1 <= 0 && w2 <= 0);

      if (inside)
      {
        float bw0 = w0 * invArea;
        float bw1 = w1 * invArea;
        float bw2 = w2 * invArea;

        float u = bw0 * ta.u + bw1 * tb.u + bw2 * tc.u;
        float v = bw0 * ta.v + bw1 * tb.v + bw2 * tc.v;

        float z = bw0 * a.z + bw1 * b.z + bw2 * c.z;

        uint16_t color = sampleCachedTexture(texCache, u, v);
        drawPixelDepth(x, y, color, z);
      }

      w0 += w0Dx;
      w1 += w1Dx;
      w2 += w2Dx;
    }

    w0Row += w0Dy;
    w1Row += w1Dy;
    w2Row += w2Dy;
  }
}

void drawTexturedQuadFast(
  const ScreenVertex& p0,
  const ScreenVertex& p1,
  const ScreenVertex& p2,
  const ScreenVertex& p3,
  TextureKind tex,
  float brightness)
{
  uint16_t texCache[TEX_SIZE * TEX_SIZE];
  buildShadedTexture(tex, brightness, texCache);

  UV uv0 = {0.0f, 1.0f};
  UV uv1 = {1.0f, 1.0f};
  UV uv2 = {1.0f, 0.0f};
  UV uv3 = {0.0f, 0.0f};

  drawTexturedTriangleFast(p0, p1, p2, uv0, uv1, uv2, texCache);
  drawTexturedTriangleFast(p0, p2, p3, uv0, uv2, uv3, texCache);

#if DRAW_FACE_OUTLINES
  canvas.drawLine((int)p0.x, (int)p0.y, (int)p1.x, (int)p1.y, COL_OUTLINE);
  canvas.drawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, COL_OUTLINE);
  canvas.drawLine((int)p2.x, (int)p2.y, (int)p3.x, (int)p3.y, COL_OUTLINE);
  canvas.drawLine((int)p3.x, (int)p3.y, (int)p0.x, (int)p0.y, COL_OUTLINE);
#endif
}

// =======================================================
// BACKGROUND
// =======================================================

void drawBackground()
{
  int horizon = 185 + (int)(cameraPitch * 90.0f);
  horizon = clampi(horizon, 70, 270);

  for (int y = 0; y < horizon; y++)
  {
    float t = (float)y / (float)horizon;
    uint16_t c = mix565(COL_SKY_TOP, COL_SKY_BOTTOM, t);
    canvas.drawFastHLine(0, y, SCREEN_W, c);
  }

  for (int y = horizon; y < SCREEN_H; y++)
  {
    float t = (float)(y - horizon) / (float)(SCREEN_H - horizon);
    uint16_t c = mix565(COL_GROUND_FAR, COL_GROUND_NEAR, t);
    canvas.drawFastHLine(0, y, SCREEN_W, c);
  }

  canvas.fillCircle(205, 42, 16, rgb565(255, 235, 120));
}

// =======================================================
// RENDER FACE
// =======================================================

bool drawWorldFaceIfVisible(const WorldFace& f, const Mat4& view, const Mat4& mvp)
{
  FaceDir dir = (FaceDir)f.dir;
  TextureKind tex = (TextureKind)f.tex;

  Vec3 center = getFaceCenter(f.x, f.y, f.z, dir);
  Vec3 normal = getFaceNormal(dir);

  float dx = center.x - cameraX;
  float dy = center.y - cameraY;
  float dz = center.z - cameraZ;

  float distSq = dx * dx + dy * dy + dz * dz;

  if (distSq > RENDER_DISTANCE_SQ)
  {
    return false;
  }

  Vec3 cameraToFace =
  {
    cameraX - center.x,
    cameraY - center.y,
    cameraZ - center.z
  };

  if (dot3(normal, cameraToFace) <= 0.0f)
  {
    return false;
  }

  Vec4 centerView = multiplyMat4Vec4(
    view,
    {center.x, center.y, center.z, 1.0f}
  );

  float z = -centerView.z;

  if (z < 0.05f || z > RENDER_DISTANCE)
  {
    return false;
  }

  float horizontalLimit = z * 1.15f + 1.8f;
  float verticalLimit   = z * 1.55f + 2.0f;

  if (fabs(centerView.x) > horizontalLimit)
  {
    return false;
  }

  if (fabs(centerView.y) > verticalLimit)
  {
    return false;
  }

  Vec4 a;
  Vec4 b;
  Vec4 c;
  Vec4 d;

  getFaceVertices(f, a, b, c, d);

  Vec4 v0 = multiplyMat4Vec4(view, a);
  Vec4 v1 = multiplyMat4Vec4(view, b);
  Vec4 v2 = multiplyMat4Vec4(view, c);
  Vec4 v3 = multiplyMat4Vec4(view, d);

  Vec4 c0 = multiplyMat4Vec4(mvp, a);
  Vec4 c1 = multiplyMat4Vec4(mvp, b);
  Vec4 c2 = multiplyMat4Vec4(mvp, c);
  Vec4 c3 = multiplyMat4Vec4(mvp, d);

  ScreenVertex p0;
  ScreenVertex p1;
  ScreenVertex p2;
  ScreenVertex p3;

  if (!projectToScreen(c0, v0, p0)) return false;
  if (!projectToScreen(c1, v1, p1)) return false;
  if (!projectToScreen(c2, v2, p2)) return false;
  if (!projectToScreen(c3, v3, p3)) return false;

  Vec3 lightDir = normalize3({0.35f, 0.85f, 0.55f});

  float light = dot3(normal, lightDir);
  float brightness = clampf(
    0.50f + fmaxf(0.0f, light) * 0.70f,
    0.42f,
    1.18f
  );

  drawTexturedQuadFast(p0, p1, p2, p3, tex, brightness);

  lastVisibleFaces++;

  return true;
}

// =======================================================
// CAMERA MOVEMENT
// =======================================================

void updateCameraHeight()
{
  int groundH = getTerrainHeightWorld(cameraX, cameraZ);
  cameraY = groundH + EYE_HEIGHT;
}

void tryMoveCamera(float dx, float dz)
{
  float nextX = cameraX + dx;
  float nextZ = cameraZ + dz;

  nextX = clampf(nextX, 0.35f, WORLD_X - 0.35f);
  nextZ = clampf(nextZ, 0.35f, WORLD_Z - 0.35f);

  if (!isWalkBlocked(nextX, cameraZ))
  {
    cameraX = nextX;
  }

  if (!isWalkBlocked(cameraX, nextZ))
  {
    cameraZ = nextZ;
  }

  updateCameraHeight();
}

void moveForward(float amount)
{
  float fx = sinf(cameraYaw);
  float fz = -cosf(cameraYaw);

  tryMoveCamera(fx * amount, fz * amount);
}

void moveRight(float amount)
{
  float rx = cosf(cameraYaw);
  float rz = sinf(cameraYaw);

  tryMoveCamera(rx * amount, rz * amount);
}

// =======================================================
// TOUCH
// =======================================================

bool pointInsideButton(uint16_t x, uint16_t y, int bx, int by, int bw, int bh)
{
  return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}

TouchControl getControlAt(uint16_t x, uint16_t y)
{
  if (pointInsideButton(x, y, BTN_W_X, BTN_W_Y, CTRL_SIZE, CTRL_SIZE)) return CTRL_FORWARD;
  if (pointInsideButton(x, y, BTN_S_X, BTN_S_Y, CTRL_SIZE, CTRL_SIZE)) return CTRL_BACKWARD;
  if (pointInsideButton(x, y, BTN_A_X, BTN_A_Y, CTRL_SIZE, CTRL_SIZE)) return CTRL_LEFT;
  if (pointInsideButton(x, y, BTN_D_X, BTN_D_Y, CTRL_SIZE, CTRL_SIZE)) return CTRL_RIGHT;

  return CTRL_NONE;
}

void handleTouch(float dt)
{
  uint16_t tx;
  uint16_t ty;

  bool touching = tft.getTouch(&tx, &ty);

  if (!touching)
  {
    wasTouching = false;
    return;
  }

  TouchControl control = getControlAt(tx, ty);

  float moveAmount = MOVE_SPEED * dt;

  if (control == CTRL_FORWARD)
  {
    moveForward(moveAmount);
    wasTouching = false;
    return;
  }

  if (control == CTRL_BACKWARD)
  {
    moveForward(-moveAmount);
    wasTouching = false;
    return;
  }

  if (control == CTRL_LEFT)
  {
    moveRight(-moveAmount);
    wasTouching = false;
    return;
  }

  if (control == CTRL_RIGHT)
  {
    moveRight(moveAmount);
    wasTouching = false;
    return;
  }

  if (wasTouching)
  {
    int dx = (int)tx - (int)lastTouchX;
    int dy = (int)ty - (int)lastTouchY;

    cameraYaw += dx * LOOK_SPEED;
    cameraPitch -= dy * LOOK_SPEED;

    cameraPitch = clampf(cameraPitch, -0.85f, 0.75f);
  }

  lastTouchX = tx;
  lastTouchY = ty;
  wasTouching = true;
}

// =======================================================
// UI
// =======================================================

void drawControlButton(int x, int y, const char* text)
{
  canvas.fillRoundRect(x, y, CTRL_SIZE, CTRL_SIZE, 7, COL_UI_BG);
  canvas.drawRoundRect(x, y, CTRL_SIZE, CTRL_SIZE, 7, COL_UI_BORDER);

  canvas.setTextDatum(middle_center);
  canvas.setTextSize(2);
  canvas.setTextColor(COL_UI_TEXT, COL_UI_BG);
  canvas.drawString(text, x + CTRL_SIZE / 2, y + CTRL_SIZE / 2);
}

void drawControls()
{
  drawControlButton(BTN_W_X, BTN_W_Y, "W");
  drawControlButton(BTN_A_X, BTN_A_Y, "A");
  drawControlButton(BTN_S_X, BTN_S_Y, "S");
  drawControlButton(BTN_D_X, BTN_D_Y, "D");
}

void drawInfoText()
{
  canvas.setTextDatum(top_left);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_BLACK, COL_SKY_BOTTOM);

  canvas.drawString("Optimized voxel renderer", 8, 8);
  canvas.drawString("Drag: look around", 8, 22);
  canvas.drawString("WASD: move", 8, 36);

  char buffer[96];

  snprintf(
    buffer,
    sizeof(buffer),
    "X %.1f Y %.1f Z %.1f",
    cameraX,
    cameraY,
    cameraZ
  );
  canvas.drawString(buffer, 8, 50);

  snprintf(
    buffer,
    sizeof(buffer),
    "Faces %d/%d Pix %d",
    lastVisibleFaces,
    worldFaceCount,
    lastDrawnPixels
  );
  canvas.drawString(buffer, 8, 64);
}

// =======================================================
// RENDER SCENA
// =======================================================

void drawScene()
{
  drawBackground();
  clearDepthBuffer();

  lastVisibleFaces = 0;
  lastDrawnPixels = 0;

  Mat4 view = buildViewMatrix();

  float aspect = (float)SCREEN_W / (float)SCREEN_H;

  Mat4 projection = perspective(
    72.0f,
    aspect,
    0.04f,
    40.0f
  );

  Mat4 mvp = multiplyMat4(projection, view);

  for (int i = 0; i < worldFaceCount; i++)
  {
    drawWorldFaceIfVisible(worldFaces[i], view, mvp);
  }

  drawInfoText();
  drawControls();

  canvas.pushSprite(0, 0);
}

// =======================================================
// TEST DISPLAY
// =======================================================

void startupScreenTest()
{
  tft.fillScreen(TFT_RED);
  delay(100);

  tft.fillScreen(TFT_GREEN);
  delay(100);

  tft.fillScreen(TFT_BLUE);
  delay(100);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 130);
  tft.print("Display OK");
  delay(250);
}

// =======================================================
// SETUP
// =======================================================

void setup()
{
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);
  tft.setBrightness(255);

  startupScreenTest();

  canvas.setColorDepth(8);

  void* spriteBuffer = canvas.createSprite(SCREEN_W, SCREEN_H);

  if (spriteBuffer == nullptr)
  {
    Serial.println("ERRORE: sprite non creato");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 120);
    tft.print("Sprite error");

    while (true)
    {
      delay(1000);
    }
  }

  buildWorldMesh();
  updateCameraHeight();

  lastFrameTime = millis();
  lastUpdateTime = millis();

  canvas.fillScreen(COL_SKY_BOTTOM);
  canvas.pushSprite(0, 0);
}

// =======================================================
// LOOP
// =======================================================

void loop()
{
  unsigned long now = millis();

  float dt = (now - lastUpdateTime) / 1000.0f;
  lastUpdateTime = now;

  if (dt > 0.08f)
  {
    dt = 0.08f;
  }

  handleTouch(dt);

  if (now - lastFrameTime >= 33)
  {
    lastFrameTime = now;
    drawScene();
  }
}