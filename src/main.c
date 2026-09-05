#include "raylib.h"
#include "math.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 640

#define GRID_HEIGTH 21
#define GRID_WIDTH 31
#define TILE_SIZE 24
#define HUD_HEIGHT 92.0f
#define MAZE_PADDING 24.0f
#define HUD_MAZE_GAP 15.0f
#define CONTEST_BYTE_LIMIT 1474560LL

#define CELL_WALL 0
#define CELL_PATH 1
#define CELL_EXIT 2
#define CELL_COIN 3
#define CELL_AMMO_PICKUP 4
#define CELL_BATTERY_PICKUP 5
#define CELL_KEY 6
#define CELL_LOCKED_EXIT 7
#define CELL_HEALTH_PICKUP 9
#define CELL_TRAP 10
#define RED_ENEMY_COUNT 3
#define BLUE_ENEMY_COUNT 2
#define MAX_BULLETS 48
#define FLASHLIGHT_MAX_BATTERY 100.0f
#define FLASHLIGHT_DRAIN_INTERVAL 1.0f
#define FLASHLIGHT_DRAIN_AMOUNT 1.0f
#define FLASHLIGHT_TOGGLE_COST 1.0f
#define MAP_MAX_BATTERY 100.0f
#define MAP_DRAIN_AMOUNT 1.0f
#define MAP_TOGGLE_COST 1.0f
#define PLAYER_BULLET_SPEED 220.0f
#define BLUE_BULLET_SPEED 140.0f
#define BOSS_BULLET_SPEED 118.0f
#define BULLET_RADIUS 4.0f
#define RED_HIT_LIMIT 5
#define BLUE_HIT_LIMIT 5
#define ENEMY_KNOCKOUT_TIME 5.0f
#define BOSS_HIT_LIMIT 5
#define PLAYER_MAX_HEALTH 100
#define PLAYER_MAGAZINE_SIZE 15
#define PLAYER_START_AMMO 30
#define PLAYER_MAX_AMMO 100
#define PLAYER_AMMO_CAP PLAYER_MAX_AMMO
#define SHOP_AMMO_GAIN 30
#define PLAYER_RELOAD_TIME 1.5f
#define BLUE_BULLET_DAMAGE 15
#define ENEMY_TOUCH_DAMAGE 25
#define PLAYER_DAMAGE_COOLDOWN 1.0f
#define PLAYER_START_HEALTH 50
#define SHOP_START_COINS 0
#define SHOP_ROUND_REWARD 50
#define SAVE_FILE_NAME "bytemaze_save.dat"
#define SAVE_VERSION 1
#define EVOLUTION_BASE_XP 100
#define EVOLUTION_ROUND_XP 60
#define EVOLUTION_PICKUP_XP 5
#define EVOLUTION_HEALTH_GAIN 5
#define EVOLUTION_AMMO_GAIN 10
#define SHOP_STANDARD_PRICE 100
#define SHOP_LIGHTNING_PRICE 400
#define SHOP_HEALTH_GAIN 5
#define SHOP_BATTERY_GAIN 50.0f
#define PICKUP_COIN_VALUE 25
#define PICKUP_AMMO_VALUE 10
#define PICKUP_BATTERY_VALUE 15.0f
#define PICKUP_HEALTH_VALUE 15
#define LIGHTNING_REVEAL_INTERVAL 15.0f
#define LIGHTNING_REVEAL_TIME 3.0f
#define TRAP_ACTIVE_TIME 3.0f
#define TRAP_INACTIVE_TIME 3.0f
#define MAX_FLOATING_NOTICES 12
#define FLOATING_NOTICE_TIME 1.25f
#define MAX_PARTICLES 160
#define PARTICLE_TIME 0.62f
#define PLAYER_DASH_SPEED_MULTIPLIER 3.15f
#define PLAYER_DASH_TIME 0.16f
#define PLAYER_DASH_COOLDOWN 2.35f
#define PLAYER_DASH_DAMAGE_GRACE 0.22f
#define PLAYER_DASH_MAX_CHARGES 3
#define TRAP_DAMAGE 18
#define MODIFIER_START_ROUND 8
#define PLAYER_CROWD_RADIUS (TILE_SIZE * 2.75f)
#define PLAYER_MAX_NEAR_ENEMIES 2
#define PLAYER_COLLISION_STEP 2.0f
#define PLAYER_COLLISION_SKIN 1.0f
#define PLAYER_WALL_RADIUS_SCALE 0.72f
#define PLAYER_SHOT_VOLUME 0.95f
#define ENEMY_SHOT_VOLUME 0.9f
#define PLAYER_RELOAD_VOLUME 0.9f
#define VICTORY_VOLUME 0.62f
#define GAME_OVER_VOLUME 0.95f
#define AUDIO_SAMPLE_RATE 22050
#define UI_FONT_PATH "/assets/fonts/PressStart2P-Regular.ttf"
#define UI_FONT_BAKE_SIZE 48
#define UI_MAX_CODEPOINTS 512
#define BOSS_FAR_DISTANCE_THRESHOLD (TILE_SIZE * 10.0f)
#define BOSS_FAR_SPEED_MULTIPLIER 1.45f
#define MAX_SIMULATION_FRAME_TIME (1.0f / 45.0f)

typedef struct FloatingNotice
{
    Vector2 position;
    char text[32];
    Color color;
    float timer;
} FloatingNotice;

typedef struct Particle
{
    Vector2 position;
    Vector2 velocity;
    Color color;
    float radius;
    float timer;
    float maxTimer;
} Particle;

typedef struct RoundModifiers
{
    bool lowVisibility;
    bool mapJammed;
    bool trapSurge;
    bool lowDash;
    bool bossPressure;
} RoundModifiers;

int grid[GRID_HEIGTH][GRID_WIDTH];
long long executableSizeBytes = -1;
float executableUsagePercent = 0.0f;
bool flashlightOn = false;
float flashlightBattery = FLASHLIGHT_MAX_BATTERY;
float flashlightDrainTimer = 0.0f;
float mapBattery = MAP_MAX_BATTERY;
int playerMaxHealth = PLAYER_START_HEALTH;
int playerHealth = PLAYER_START_HEALTH;
int playerMaxAmmo = PLAYER_MAX_AMMO;
int playerAmmo = PLAYER_MAGAZINE_SIZE;
int playerTotalAmmo = PLAYER_START_AMMO;
float playerReloadTimer = 0.0f;
float playerDamageCooldown = 0.0f;
int playerCoins = SHOP_START_COINS;
int playerLevel = 1;
int playerXp = 0;
int lightningCharges = 0;
float lightningAutoTimer = 0.0f;
float lightningRevealTimer = 0.0f;
bool hasExitKey = false;
int lockedExitCellX = -1;
int lockedExitCellY = -1;
FloatingNotice floatingNotices[MAX_FLOATING_NOTICES] = { 0 };
Particle particles[MAX_PARTICLES] = { 0 };
int roundPickupsCollected = 0;
int roundShotsFired = 0;
bool roundTookDamage = false;
int pendingRoundBonus = 0;
int pendingEvolutionXp = 0;
bool victoryRoundsOpen = false;
int roundStartCoins = SHOP_START_COINS;
int roundStartLevel = 1;
int roundStartXp = 0;
int roundStartMaxAmmo = PLAYER_MAX_AMMO;
int roundStartTotalAmmo = PLAYER_START_AMMO;
int roundStartAmmo = PLAYER_MAGAZINE_SIZE;
float roundStartBattery = FLASHLIGHT_MAX_BATTERY;
float roundStartMapBattery = MAP_MAX_BATTERY;
int roundStartLightning = 0;
bool roundStartSnapshotActive = false;
float playerDashTimer = 0.0f;
float playerDashCooldown = 0.0f;
int playerDashCharges = PLAYER_DASH_MAX_CHARGES;
bool tacticalMapOpen = false;
RoundModifiers currentModifiers = { 0 };

extern bool inTutorialSequence;
extern int officialRound;
bool IsCellOccupiedByEnemy(int cellX, int cellY);
Vector2 GetCellCenter(int cellX, int cellY);

bool IsLockedExitRound(void)
{
    return !inTutorialSequence && officialRound >= 4;
}

RoundModifiers GetRoundModifiers(int round)
{
    RoundModifiers mods = { 0 };

    if (round < MODIFIER_START_ROUND)
    {
        return mods;
    }

    mods.lowVisibility = (round % 3) == 1;
    mods.mapJammed = (round % 4) == 0;
    mods.trapSurge = (round % 5) == 0;
    mods.lowDash = (round % 6) == 0;
    mods.bossPressure = (round % 7) == 0;
    return mods;
}

const char *FormatModifierSummary(RoundModifiers mods)
{
    static char summary[80];
    summary[0] = '\0';

    if (mods.lowVisibility) strncat(summary, " VISAO", sizeof(summary) - strlen(summary) - 1);
    if (mods.mapJammed) strncat(summary, " MAPA-", sizeof(summary) - strlen(summary) - 1);
    if (mods.trapSurge) strncat(summary, " TRAPS+", sizeof(summary) - strlen(summary) - 1);
    if (mods.lowDash) strncat(summary, " DASH-", sizeof(summary) - strlen(summary) - 1);
    if (mods.bossPressure) strncat(summary, " BOSS+", sizeof(summary) - strlen(summary) - 1);
    return (summary[0] == '\0') ? "PADRAO+" : summary;
}

const char *GetModifierSummary(void)
{
    if (inTutorialSequence || officialRound < MODIFIER_START_ROUND)
    {
        return "PADRAO";
    }

    return FormatModifierSummary(currentModifiers);
}

int GetRoundDashMaxCharges(void)
{
    return currentModifiers.lowDash ? 2 : PLAYER_DASH_MAX_CHARGES;
}

bool IsTacticalMapAvailable(void)
{
    return !currentModifiers.mapJammed;
}

float GetDarknessVisionScale(void)
{
    return currentModifiers.lowVisibility ? 0.72f : 1.0f;
}

Color HUD_PANEL_COLOR = { 5, 10, 32, 232 };
/* Softer, less saturated than the original neon cyan/magenta so the maze
 * (not the HUD chrome) reads as the visual focal point. */
Color HUD_BORDER_COLOR = { 70, 150, 200, 255 };
Color HUD_ACCENT_COLOR = { 150, 100, 195, 255 };
Color MAZE_WALL_COLOR = { 20, 22, 36, 255 };
Color MAZE_PATH_COLOR = { 235, 240, 250, 255 };
Color MAZE_SHADOW_COLOR = { 8, 9, 18, 255 };
Color MAZE_EXIT_COLOR = { 60, 230, 130, 255 };

typedef struct MazeLayout
{
    Vector2 offset;
    Vector2 scale;
    float drawScale;
} MazeLayout;

float GetUIScale(void);

/* The HUD panel's width is fixed (not shrunk to fit leftover space) so
 * its text never has to compress past its readable floor and overflow
 * the border. The maze reserves this much on the left when it lays
 * itself out, so there's always room and the panel never overlaps it. */
static bool IsCompactLayout(void)
{
    return GetScreenWidth() < 1250 || GetScreenHeight() < 720;
}

float GetHudPanelWidth(void)
{
    float scale = GetUIScale();
    float w = (float)GetScreenWidth();

    if (IsCompactLayout())
    {
        return fmaxf(230.0f * scale, fminf(w - 24.0f * scale, 380.0f * scale));
    }

    return fminf(330.0f * scale, w * 0.17f);
}

float GetMazeLeftReservedWidth(void)
{
    if (IsCompactLayout()) return 0.0f;
    return (10.0f * GetUIScale()) + GetHudPanelWidth() + HUD_MAZE_GAP;
}

float GetMazeTopBound(void)
{
    return (66.0f * GetUIScale()) + HUD_MAZE_GAP;
}

float GetMazeBottomBound(void)
{
    return (float)GetScreenHeight() - (22.0f * GetUIScale());
}

float GetHudRightPanelWidth(void)
{
    float scale = GetUIScale();
    float w = (float)GetScreenWidth();

    if (IsCompactLayout())
    {
        return fmaxf(230.0f * scale, fminf(w - 24.0f * scale, 380.0f * scale));
    }

    return fminf(330.0f * scale, w * 0.17f);
}

Rectangle GetMazeAvailableRect(void)
{
    float ui = GetUIScale();

    if (IsCompactLayout())
    {
        float horizontalPadding = 10.0f * ui;
        float topSpace = (78.0f * ui) + (74.0f * ui) + HUD_MAZE_GAP;
        float bottomSpace = (92.0f * ui) + HUD_MAZE_GAP;

        return (Rectangle){
            horizontalPadding,
            topSpace,
            fmaxf(1.0f, (float)GetScreenWidth() - horizontalPadding * 2.0f),
            fmaxf(1.0f, (float)GetScreenHeight() - topSpace - bottomSpace)
        };
    }

    float mazeLeftBound = GetMazeLeftReservedWidth();
    float mazeRightBound = (float)GetScreenWidth() - (10.0f * ui) - GetHudRightPanelWidth() - HUD_MAZE_GAP;

    return (Rectangle){
        mazeLeftBound,
        GetMazeTopBound(),
        fmaxf(1.0f, mazeRightBound - mazeLeftBound),
        fmaxf(1.0f, GetMazeBottomBound() - GetMazeTopBound())
    };
}

MazeLayout GetMazeLayout(void)
{
    Rectangle available = GetMazeAvailableRect();
    float mazeFrameWidth = (float)((GRID_WIDTH + 0.7f) * TILE_SIZE);
    float mazeFrameHeight = (float)((GRID_HEIGTH + 0.7f) * TILE_SIZE);
    MazeLayout layout = { 0 };

    layout.scale.x = fmaxf(0.25f, available.width / mazeFrameWidth);
    layout.scale.y = fmaxf(0.25f, available.height / mazeFrameHeight);
    layout.drawScale = fminf(layout.scale.x, layout.scale.y);
    layout.offset.x = available.x + ((float)TILE_SIZE * layout.scale.x * 0.35f);
    layout.offset.y = available.y + ((float)TILE_SIZE * layout.scale.y * 0.35f);

    return layout;
}

float GetMazeScale(void)
{
    return GetMazeLayout().drawScale;
}

Vector2 GetMazeOffset(float scale)
{
    (void)scale;
    return GetMazeLayout().offset;
}

Vector2 WorldToScreenPosition(Vector2 worldPosition, float scale, Vector2 offset)
{
    Vector2 screenPosition = { 0 };
    screenPosition.x = offset.x + (worldPosition.x * scale);
    screenPosition.y = offset.y + (worldPosition.y * scale);
    return screenPosition;
}

Vector2 WorldToScreenPositionLayout(Vector2 worldPosition, MazeLayout layout)
{
    return (Vector2){
        layout.offset.x + (worldPosition.x * layout.scale.x),
        layout.offset.y + (worldPosition.y * layout.scale.y)
    };
}

Rectangle GetMazeScreenRect(float scale, Vector2 offset)
{
    return (Rectangle){
        offset.x,
        offset.y,
        (float)(GRID_WIDTH * TILE_SIZE) * scale,
        (float)(GRID_HEIGTH * TILE_SIZE) * scale
    };
}

long long GetFileSizeBytes(const char *filePath)
{
    struct stat fileInfo = { 0 };

    if (stat(filePath, &fileInfo) != 0)
    {
        return -1;
    }

    return (long long)fileInfo.st_size;
}

float GetContestUsagePercent(long long usedBytes)
{
    if (usedBytes <= 0)
    {
        return 0.0f;
    }

    return ((float)usedBytes / (float)CONTEST_BYTE_LIMIT) * 100.0f;
}

long long GetRuntimeAssetSizeBytes(void)
{
    const char *bundledAssetPathGroups[][4] = {
        {
            "assets/fonts/PressStart2P-Regular.ttf",
            "src/assets/fonts/PressStart2P-Regular.ttf",
            NULL,
            NULL
        },
        {
            "assets/fonts/NotoSansKR-Subset-Bold.ttf",
            "src/assets/fonts/NotoSansKR-Subset-Bold.ttf",
            NULL,
            NULL
        }
    };
    long long totalBytes = 0;
    int groupCount = (int)(sizeof(bundledAssetPathGroups) / sizeof(bundledAssetPathGroups[0]));

    for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
    {
        for (int pathIndex = 0; bundledAssetPathGroups[groupIndex][pathIndex] != NULL; pathIndex++)
        {
            long long fileSize = GetFileSizeBytes(bundledAssetPathGroups[groupIndex][pathIndex]);
            if (fileSize > 0)
            {
                totalBytes += fileSize;
                break;
            }
        }
    }

    return totalBytes;
}

void UpdateBuildMetrics(const char *executablePath)
{
    executableSizeBytes = GetFileSizeBytes(executablePath);

    if (executableSizeBytes < 0)
    {
        executableSizeBytes = GetFileSizeBytes("./bytemaze");
    }

    if (executableSizeBytes < 0)
    {
        executableSizeBytes = GetFileSizeBytes("./bytemaze.exe");
    }

    if (executableSizeBytes > 0)
    {
        executableSizeBytes += GetRuntimeAssetSizeBytes();
    }

    executableUsagePercent = GetContestUsagePercent(executableSizeBytes);
}

void FillGridWithWalls(void)
{
    for (int y = 0; y < GRID_HEIGTH; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            grid[y][x] = CELL_WALL;
        }
    }
}

void ShuffleDirections(int directions[4])
{
    for (int i = 3; i > 0; i--)
    {
        int j = GetRandomValue(0, i);
        int temp = directions[i];
        directions[i] = directions[j];
        directions[j] = temp;
    }
}

void CarveExitBypass(int exitX, int exitY)
{
    for (int y = exitY - 1; y <= exitY + 1; y++)
    {
        for (int x = exitX - 1; x <= exitX + 1; x++)
        {
            if (x <= 0 || x >= GRID_WIDTH - 1 || y <= 0 || y >= GRID_HEIGTH - 1)
            {
                continue;
            }

            if (x == exitX && y == exitY)
            {
                continue;
            }

            grid[y][x] = CELL_PATH;
        }
    }
}

void GenerateMaze(void)
{
    bool lockedExitRound = IsLockedExitRound();

    FillGridWithWalls();

    int stackX[GRID_WIDTH * GRID_HEIGTH];
    int stackY[GRID_WIDTH * GRID_HEIGTH];
    int stackSize = 0;

    int startX = 1;
    int startY = 1;

    grid[startY][startX] = CELL_PATH;
    stackX[stackSize] = startX;
    stackY[stackSize] = startY;
    stackSize++;

    while (stackSize > 0)
    {
        int currentX = stackX[stackSize - 1];
        int currentY = stackY[stackSize - 1];
        int directions[4] = { 0, 1, 2, 3 };

        ShuffleDirections(directions);

        int carved = 0;

        for (int i = 0; i < 4; i++)
        {
            int dir = directions[i];
            int nextX = currentX;
            int nextY = currentY;
            int wallX = currentX;
            int wallY = currentY;

            if (dir == 0)
            {
                nextY = currentY - 2;
                wallY = currentY - 1;
            }
            else if (dir == 1)
            {
                nextX = currentX + 2;
                wallX = currentX + 1;
            }
            else if (dir == 2)
            {
                nextY = currentY + 2;
                wallY = currentY + 1;
            }
            else if (dir == 3)
            {
                nextX = currentX - 2;
                wallX = currentX - 1;
            }

            if (nextX > 0 && nextX < GRID_WIDTH - 1 && nextY > 0 && nextY < GRID_HEIGTH - 1)
            {
                if (grid[nextY][nextX] == CELL_WALL)
                {
                    grid[wallY][wallX] = CELL_PATH;
                    grid[nextY][nextX] = CELL_PATH;
                    stackX[stackSize] = nextX;
                    stackY[stackSize] = nextY;
                    stackSize++;
                    carved = 1;
                    break;
                }
            }
        }

        if (!carved)
        {
            stackSize--;
        }
    }

    if (!inTutorialSequence && officialRound >= MODIFIER_START_ROUND)
    {
        int exitX = GRID_WIDTH - 2;
        int exitY = 1;
        int bestDistance = -1;

        for (int attempt = 0; attempt < 80; attempt++)
        {
            int x = GetRandomValue(1, GRID_WIDTH - 2);
            int y = GetRandomValue(1, GRID_HEIGTH - 2);

            if (grid[y][x] != CELL_PATH || (x == 1 && y == 1))
            {
                continue;
            }

            int distance = abs(x - 1) + abs(y - 1);
            if (distance > bestDistance)
            {
                bestDistance = distance;
                exitX = x;
                exitY = y;
            }
        }

        CarveExitBypass(exitX, exitY);
        grid[exitY][exitX] = lockedExitRound ? CELL_LOCKED_EXIT : CELL_EXIT;
        lockedExitCellX = exitX;
        lockedExitCellY = exitY;
    }
    else
    {
        int exitX = GRID_WIDTH - 3;
        int exitY = 1;

        CarveExitBypass(exitX, exitY);
        grid[exitY][exitX] = lockedExitRound ? CELL_LOCKED_EXIT : CELL_EXIT;
        lockedExitCellX = exitX;
        lockedExitCellY = exitY;
    }
}

bool IsGridCellWalkableForPickup(int x, int y)
{
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGTH)
    {
        return false;
    }

    return grid[y][x] != CELL_WALL && grid[y][x] != CELL_LOCKED_EXIT;
}

int CountWalkablePickupNeighbors(int x, int y)
{
    const int neighborOffsets[4][2] = {
        { 1, 0 },
        { -1, 0 },
        { 0, 1 },
        { 0, -1 }
    };
    int neighbors = 0;

    for (int i = 0; i < 4; i++)
    {
        if (IsGridCellWalkableForPickup(x + neighborOffsets[i][0], y + neighborOffsets[i][1]))
        {
            neighbors++;
        }
    }

    return neighbors;
}

bool IsPickupNextToLockedExit(int x, int y)
{
    if (lockedExitCellX < 0 || lockedExitCellY < 0)
    {
        return false;
    }

    return abs(x - lockedExitCellX) + abs(y - lockedExitCellY) <= 1;
}

bool IsValidPickupCell(int x, int y, int cellType, bool relaxedKeyRules)
{
    if (grid[y][x] != CELL_PATH || IsCellOccupiedByEnemy(x, y) || (abs(x - 1) + abs(y - 1)) <= 8)
    {
        return false;
    }

    if (cellType == CELL_KEY)
    {
        int exitDistance = (lockedExitCellX >= 0 && lockedExitCellY >= 0) ? abs(x - lockedExitCellX) + abs(y - lockedExitCellY) : 99;
        int requiredNeighbors = relaxedKeyRules ? 1 : 2;

        if (IsPickupNextToLockedExit(x, y) || exitDistance <= 3)
        {
            return false;
        }

        if (CountWalkablePickupNeighbors(x, y) < requiredNeighbors)
        {
            return false;
        }
    }

    return true;
}

bool TryPlacePickupCell(int cellType, bool relaxedKeyRules)
{
    for (int attempt = 0; attempt < 90; attempt++)
    {
        int x = GetRandomValue(1, GRID_WIDTH - 2);
        int y = GetRandomValue(1, GRID_HEIGTH - 2);

        if (IsValidPickupCell(x, y, cellType, relaxedKeyRules))
        {
            grid[y][x] = cellType;
            return true;
        }
    }

    for (int y = 1; y < GRID_HEIGTH - 1; y++)
    {
        for (int x = 1; x < GRID_WIDTH - 1; x++)
        {
            if (IsValidPickupCell(x, y, cellType, relaxedKeyRules))
            {
                grid[y][x] = cellType;
                return true;
            }
        }
    }

    return false;
}

void PlacePickupCell(int cellType, int amount)
{
    for (int placed = 0; placed < amount; placed++)
    {
        if (!TryPlacePickupCell(cellType, false) && cellType == CELL_KEY)
        {
            TryPlacePickupCell(cellType, true);
        }
    }
}

void PlaceRoundPickups(bool flashlightEnabled)
{
    if (inTutorialSequence)
    {
        return;
    }

    PlacePickupCell(CELL_COIN, 2 + (officialRound >= 4 ? 1 : 0));
    PlacePickupCell(CELL_AMMO_PICKUP, 1 + (officialRound >= 8 ? 1 : 0));
    PlacePickupCell(CELL_HEALTH_PICKUP, officialRound >= 3 ? 1 : 0);

    if (IsLockedExitRound())
    {
        PlacePickupCell(CELL_KEY, 1);
    }

    if (officialRound >= 9)
    {
        PlacePickupCell(CELL_TRAP, 2 + (officialRound >= 14 ? 1 : 0) + (currentModifiers.trapSurge ? 3 : 0));
    }

    if (flashlightEnabled)
    {
        PlacePickupCell(CELL_BATTERY_PICKUP, 1);
    }
}

typedef struct Player
{
    Vector2 position;
    float radius;
    float speed;
    float facingAngle;
} Player;

typedef enum EnemyType
{
    ENEMY_RED,
    ENEMY_BLUE,
    ENEMY_BOSS
} EnemyType;

typedef struct Enemy
{
    Vector2 position;
    Vector2 direction;
    float radius;
    float speed;
    bool active;
    EnemyType type;
    int hitsTaken;
    float knockoutTimer;
    float shootPauseTimer;
} Enemy;

typedef struct Bullet
{
    Vector2 position;
    Vector2 direction;
    float speed;
    float radius;
    bool active;
    bool fromPlayer;
    bool fromBoss;
} Bullet;

Enemy redEnemies[RED_ENEMY_COUNT];
Enemy blueEnemies[BLUE_ENEMY_COUNT];
Enemy bossEnemy;
Bullet bullets[MAX_BULLETS];
bool playerAlive = true;
Sound playerShotSound;
Sound enemyShotSound;
Sound playerReloadSound;
Sound playerHitSound;
Sound flashlightToggleSound;
Sound lightningSound;
Sound victorySound;
Sound gameOverSound;
Sound dashSound;
Sound trapSound;
Sound bossAlertSound;
bool gameAudioLoaded = false;
Font uiFont;
bool uiFontLoaded = false;
Font uiLatinFont;
bool uiLatinFontLoaded = false;
Color currentDrawColor = WHITE;

Player player;
typedef enum GamePhase
{
    PHASE_INTRO,
    PHASE_SHOP,
    PHASE_INFO,
    PHASE_PLAYING
} GamePhase;

typedef enum Language
{
    LANGUAGE_PT_BR,
    LANGUAGE_ES,
    LANGUAGE_EN,
    LANGUAGE_KO,
    LANGUAGE_COUNT
} Language;

typedef enum TextId
{
    TEXT_LANGUAGE_NAME,
    TEXT_LANGUAGE_BUTTON,
    TEXT_BEST_ROUND,
    TEXT_TUTORIAL_PROGRESS,
    TEXT_ROUND_PROGRESS,
    TEXT_HEALTH,
    TEXT_SHOOT,
    TEXT_AMMO,
    TEXT_RELOADING,
    TEXT_RELOAD,
    TEXT_FLASHLIGHT_CONTROL,
    TEXT_FLASHLIGHT_STATE,
    TEXT_FLASHLIGHT_ON,
    TEXT_FLASHLIGHT_OFF,
    TEXT_BATTERY,
    TEXT_GAME_OVER,
    TEXT_PLAY_AGAIN,
    TEXT_SKIP_TUTORIAL,
    TEXT_CONTINUE_TUTORIAL,
    TEXT_INTRO_FOOTER,
    TEXT_ROUND_FOOTER,
    TEXT_HUD_STATUS_TITLE,
    TEXT_HUD_MAIN_DATA,
    TEXT_HUD_LOG_TITLE,
    TEXT_HUD_EXECUTABLE,
    TEXT_HUD_BYTES,
    TEXT_HUD_LIMIT,
    TEXT_HUD_VITAL_TITLE,
    TEXT_HUD_VIDA,
    TEXT_HUD_WEAPON_TITLE,
    TEXT_HUD_BALAS,
    TEXT_HUD_CONTROLS_TITLE,
    TEXT_HUD_ATIRAR,
    TEXT_HUD_LASER_SHOT,
    TEXT_HUD_RECARREGAR,
    TEXT_HUD_FILL_AMMO,
    TEXT_HUD_LANTERNA,
    TEXT_RELOAD_IN_PROGRESS,
    TEXT_STAGE_COMPLETE,
    TEXT_NEXT_ROUND,
    TEXT_SHOP_TITLE,
    TEXT_SHOP_COINS,
    TEXT_SHOP_BUY_AMMO,
    TEXT_SHOP_BUY_BATTERY,
    TEXT_SHOP_BUY_HEALTH,
    TEXT_SHOP_BUY_LIGHTNING,
    TEXT_SHOP_START,
    TEXT_SHOP_MAX,
    TEXT_SHOP_LOCKED,
    TEXT_HUD_LIGHTNING,
    TEXT_COUNT
} TextId;

typedef struct RoundConfig
{
    bool redEnabled;
    bool blueEnabled;
    bool bossEnabled;
    bool flashlightEnabled;
} RoundConfig;

GamePhase gamePhase = PHASE_INTRO;
RoundConfig currentRoundConfig = { true, false, false, false };
Language currentLanguage = LANGUAGE_PT_BR;
bool inTutorialSequence = true;
int tutorialRound = 1;
int officialRound = 1;
int bestOfficialRound = 1;
bool roundNeedsSetup = true;
bool waitingForVictorySound = false;
bool playerStartAuraVisible = true;

int GetCurrentPlayerMaxHealth(void)
{
    return inTutorialSequence ? PLAYER_MAX_HEALTH : playerMaxHealth;
}

void SpawnBullet(Vector2 position, Vector2 direction, float speed, bool fromPlayer);
void SpawnBossBullet(Vector2 position, Vector2 direction);
int GetDifficultyRampLevel(void);
float GetRedEnemyTrackDistance(void);
float GetBlueEnemyPathfindChance(void);
float GetEnemySpeedBonus(void);
float GetBossSpeedBonus(void);
float GetBossShootCooldown(void);
float GetBossFarSpeedMultiplier(void);
bool IsCellOccupiedByEnemy(int cellX, int cellY);
void InitGameAudio(void);
Sound CreateSynthSound(int soundType);
void UpdateGameAudio(void);
void ShutdownGameAudio(void);
void InitUIFont(void);
void ShutdownUIFont(void);
void DrawTacticalMapOverlay(void);
void SpawnFloatingNotice(Vector2 position, const char *text, Color color);
bool DamagePlayer(int damage);
RoundModifiers GetRoundModifiers(int round);
const char *GetModifierSummary(void);
int GetRoundDashMaxCharges(void);
bool IsTacticalMapAvailable(void);
float GetDarknessVisionScale(void);
void SpawnParticleBurst(Vector2 position, Color color, int count, float speed, float radius);
void UpdateParticles(void);
void DrawParticles(void);
void DrawButtonArrow(Rectangle button, Color color);
Vector2 MeasureTextStrongSpaced(const char *text, int fontSize, float spacing);
void SaveProgress(void);
void SaveProgressForRound(int savedRound, int savedBestRound);
void LoadProgress(void);

float GetUIScale(void)
{
    float widthScale = (float)GetScreenWidth() / (float)SCREEN_WIDTH;
    float heightScale = (float)GetScreenHeight() / (float)SCREEN_HEIGHT;
    return fminf(fmaxf(fminf(widthScale, heightScale), 0.78f), 1.22f);
}

int ScaleFontSize(float fontSize)
{
    /* Global readability boost: every label in the game (HUD, buttons, tutorial
     * panels, language selector) reads this value. The game now uses the
     * loaded system font instead of raylib's pixel fallback, so this keeps
     * text comfortably large without making fitted HUD labels collide. */
    #define UI_READABILITY_BOOST 1.48f
    return (int)fmaxf(13.0f, roundf(fontSize * UI_READABILITY_BOOST * GetUIScale()));
}

float GetStableFrameTime(void)
{
    return fminf(GetFrameTime(), MAX_SIMULATION_FRAME_TIME);
}

int GetEvolutionXpToNextLevel(void)
{
    return EVOLUTION_BASE_XP * playerLevel;
}

void ClampProgressState(void)
{
    if (officialRound < 1) officialRound = 1;
    if (bestOfficialRound < officialRound) bestOfficialRound = officialRound;
    if (playerCoins < 0) playerCoins = 0;
    if (playerLevel < 1) playerLevel = 1;
    if (playerXp < 0) playerXp = 0;
    if (playerMaxHealth < PLAYER_START_HEALTH) playerMaxHealth = PLAYER_START_HEALTH;
    if (playerMaxHealth > PLAYER_MAX_HEALTH) playerMaxHealth = PLAYER_MAX_HEALTH;
    if (playerHealth < 1) playerHealth = playerMaxHealth;
    if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;
    if (playerMaxAmmo < PLAYER_MAX_AMMO) playerMaxAmmo = PLAYER_MAX_AMMO;
    if (playerMaxAmmo > PLAYER_AMMO_CAP) playerMaxAmmo = PLAYER_AMMO_CAP;
    if (playerTotalAmmo < 0) playerTotalAmmo = 0;
    if (playerTotalAmmo > PLAYER_AMMO_CAP) playerTotalAmmo = PLAYER_AMMO_CAP;
    if (playerTotalAmmo > playerMaxAmmo) playerTotalAmmo = playerMaxAmmo;
    if (playerAmmo < 0) playerAmmo = 0;
    if (playerAmmo > PLAYER_MAGAZINE_SIZE) playerAmmo = PLAYER_MAGAZINE_SIZE;
    if (playerAmmo > playerTotalAmmo) playerAmmo = playerTotalAmmo;
    if (flashlightBattery < 0.0f) flashlightBattery = 0.0f;
    if (flashlightBattery > FLASHLIGHT_MAX_BATTERY) flashlightBattery = FLASHLIGHT_MAX_BATTERY;
    if (mapBattery < 0.0f) mapBattery = 0.0f;
    if (mapBattery > MAP_MAX_BATTERY) mapBattery = MAP_MAX_BATTERY;
    if (lightningCharges < 0) lightningCharges = 0;
}

void AddEvolutionXp(int amount)
{
    if (amount <= 0 || inTutorialSequence)
    {
        return;
    }

    playerXp += amount;

    while (playerXp >= GetEvolutionXpToNextLevel())
    {
        playerXp -= GetEvolutionXpToNextLevel();
        playerLevel++;

        if (playerMaxHealth < PLAYER_MAX_HEALTH && (playerLevel % 2) == 0)
        {
            playerMaxHealth += EVOLUTION_HEALTH_GAIN;
            if (playerMaxHealth > PLAYER_MAX_HEALTH)
            {
                playerMaxHealth = PLAYER_MAX_HEALTH;
            }
            playerHealth = playerMaxHealth;
        }

        playerTotalAmmo += EVOLUTION_AMMO_GAIN;
        ClampProgressState();
        SpawnFloatingNotice(player.position, TextFormat("NIVEL %d", playerLevel), GOLD);
        SpawnParticleBurst(player.position, GOLD, 28, 130.0f, 3.0f);
    }
}

void SaveProgressForRound(int savedRound, int savedBestRound)
{
    FILE *file = fopen(SAVE_FILE_NAME, "w");
    if (file == NULL)
    {
        return;
    }

    fprintf(file, "%d %d %d %d %d %d %d %d %d %.2f %d\n",
            SAVE_VERSION,
            savedRound,
            savedBestRound,
            playerCoins,
            playerLevel,
            playerXp,
            playerMaxHealth,
            playerMaxAmmo,
            playerTotalAmmo,
            flashlightBattery,
            lightningCharges);
    fclose(file);
}

void SaveProgress(void)
{
    SaveProgressForRound(officialRound, bestOfficialRound);
}

void CaptureRoundStartSnapshot(void)
{
    if (inTutorialSequence)
    {
        roundStartSnapshotActive = false;
        return;
    }

    roundStartCoins = playerCoins;
    roundStartLevel = playerLevel;
    roundStartXp = playerXp;
    roundStartMaxAmmo = playerMaxAmmo;
    roundStartTotalAmmo = playerTotalAmmo;
    roundStartAmmo = playerAmmo;
    roundStartBattery = flashlightBattery;
    roundStartMapBattery = mapBattery;
    roundStartLightning = lightningCharges;
    roundStartSnapshotActive = true;
}

void RestoreRoundStartSnapshot(void)
{
    if (!roundStartSnapshotActive)
    {
        return;
    }

    playerCoins = roundStartCoins;
    playerLevel = roundStartLevel;
    playerXp = roundStartXp;
    playerMaxAmmo = roundStartMaxAmmo;
    playerTotalAmmo = roundStartTotalAmmo;
    playerAmmo = roundStartAmmo;
    flashlightBattery = roundStartBattery;
    mapBattery = roundStartMapBattery;
    lightningCharges = roundStartLightning;
    pendingRoundBonus = 0;
    pendingEvolutionXp = 0;
    roundPickupsCollected = 0;
    ClampProgressState();
    roundStartSnapshotActive = false;
}

void LoadProgress(void)
{
    int version = 0;
    int savedRound = 1;
    int savedBestRound = 1;
    int savedCoins = SHOP_START_COINS;
    int savedLevel = 1;
    int savedXp = 0;
    int savedMaxHealth = PLAYER_START_HEALTH;
    int savedMaxAmmo = PLAYER_MAX_AMMO;
    int savedTotalAmmo = PLAYER_START_AMMO;
    float savedBattery = FLASHLIGHT_MAX_BATTERY;
    int savedLightning = 0;
    FILE *file = fopen(SAVE_FILE_NAME, "r");

    if (file == NULL)
    {
        return;
    }

    int readCount = fscanf(file, "%d %d %d %d %d %d %d %d %d %f %d",
                           &version,
                           &savedRound,
                           &savedBestRound,
                           &savedCoins,
                           &savedLevel,
                           &savedXp,
                           &savedMaxHealth,
                           &savedMaxAmmo,
                           &savedTotalAmmo,
                           &savedBattery,
                           &savedLightning);
    fclose(file);

    if (readCount != 11 || version != SAVE_VERSION)
    {
        return;
    }

    inTutorialSequence = false;
    tutorialRound = 1;
    officialRound = savedRound;
    bestOfficialRound = savedBestRound;
    playerCoins = savedCoins;
    playerLevel = savedLevel;
    playerXp = savedXp;
    playerMaxHealth = savedMaxHealth;
    playerHealth = playerMaxHealth;
    playerMaxAmmo = savedMaxAmmo;
    playerTotalAmmo = savedTotalAmmo;
    playerAmmo = (playerTotalAmmo < PLAYER_MAGAZINE_SIZE) ? playerTotalAmmo : PLAYER_MAGAZINE_SIZE;
    flashlightBattery = savedBattery;
    lightningCharges = savedLightning;
    ClampProgressState();
    roundNeedsSetup = true;
    gamePhase = PHASE_INTRO;
}

const char *uiText[LANGUAGE_COUNT][TEXT_COUNT] = {
    [LANGUAGE_PT_BR] = {
        [TEXT_LANGUAGE_NAME] = "PT-BR",
        [TEXT_LANGUAGE_BUTTON] = "PT-BR",
        [TEXT_BEST_ROUND] = "Recorde pessoal: round %d",
        [TEXT_TUTORIAL_PROGRESS] = "Tutorial %d/4",
        [TEXT_ROUND_PROGRESS] = "Round %d",
        [TEXT_HEALTH] = "Vida: %d/%d",
        [TEXT_SHOOT] = "Atirar: ESPACO",
        [TEXT_AMMO] = "Balas: %d/%d",
        [TEXT_RELOADING] = "Recarregando: %.1fs",
        [TEXT_RELOAD] = "Recarregar: R",
        [TEXT_FLASHLIGHT_CONTROL] = "Lanterna: C",
        [TEXT_FLASHLIGHT_STATE] = "Lanterna: %s",
        [TEXT_FLASHLIGHT_ON] = "ligada",
        [TEXT_FLASHLIGHT_OFF] = "desligada",
        [TEXT_BATTERY] = "Bateria: %.0f%%",
        [TEXT_GAME_OVER] = "VOCE MORREU",
        [TEXT_PLAY_AGAIN] = "JOGAR DE NOVO",
        [TEXT_SKIP_TUTORIAL] = "Pular tutorial",
        [TEXT_CONTINUE_TUTORIAL] = "Continuar",
        [TEXT_INTRO_FOOTER] = "Escolha um idioma acima e continue abaixo.",
        [TEXT_ROUND_FOOTER] = "Clique em Continuar para jogar este passo.",
        [TEXT_HUD_STATUS_TITLE] = "STATUS DO SISTEMA",
        [TEXT_HUD_MAIN_DATA] = "DADOS PRINCIPAIS",
        [TEXT_HUD_LOG_TITLE] = "LOG DE DADOS",
        [TEXT_HUD_EXECUTABLE] = "PACOTE",
        [TEXT_HUD_BYTES] = "bytes",
        [TEXT_HUD_LIMIT] = "LIMITE",
        [TEXT_HUD_VITAL_TITLE] = "VITAL DO NUCLEO",
        [TEXT_HUD_VIDA] = "VIDA",
        [TEXT_HUD_WEAPON_TITLE] = "ARMAMENTO",
        [TEXT_HUD_BALAS] = "BALAS",
        [TEXT_HUD_CONTROLS_TITLE] = "CONTROLES DO TERMINAL",
        [TEXT_HUD_ATIRAR] = "ATIRAR",
        [TEXT_HUD_LASER_SHOT] = "Disparo laser",
        [TEXT_HUD_RECARREGAR] = "RECARREGAR",
        [TEXT_HUD_FILL_AMMO] = "Preencher balas",
        [TEXT_HUD_LANTERNA] = "LANTERNA",
        [TEXT_RELOAD_IN_PROGRESS] = "Recarregando",
        [TEXT_STAGE_COMPLETE] = "%s %d completo!",
        [TEXT_NEXT_ROUND] = "Proximo round",
        [TEXT_SHOP_TITLE] = "LOJA",
        [TEXT_SHOP_COINS] = "%d MOEDAS",
        [TEXT_SHOP_BUY_AMMO] = "MUNICAO",
        [TEXT_SHOP_BUY_BATTERY] = "BATERIA",
        [TEXT_SHOP_BUY_HEALTH] = "VIDA",
        [TEXT_SHOP_BUY_LIGHTNING] = "RAIO",
        [TEXT_SHOP_START] = "VOLTAR AO JOGO",
        [TEXT_SHOP_MAX] = "MAX",
        [TEXT_SHOP_LOCKED] = "Round 10",
        [TEXT_HUD_LIGHTNING] = "RAIO"
    },
    [LANGUAGE_ES] = {
        [TEXT_LANGUAGE_NAME] = "ES",
        [TEXT_LANGUAGE_BUTTON] = "ES",
        [TEXT_BEST_ROUND] = "Mejor marca: ronda %d",
        [TEXT_TUTORIAL_PROGRESS] = "Tutorial %d/4",
        [TEXT_ROUND_PROGRESS] = "Ronda %d",
        [TEXT_HEALTH] = "Vida: %d/%d",
        [TEXT_SHOOT] = "Disparar: ESPACIO",
        [TEXT_AMMO] = "Balas: %d/%d",
        [TEXT_RELOADING] = "Recargando: %.1fs",
        [TEXT_RELOAD] = "Recargar: R",
        [TEXT_FLASHLIGHT_CONTROL] = "Linterna: C",
        [TEXT_FLASHLIGHT_STATE] = "Linterna: %s",
        [TEXT_FLASHLIGHT_ON] = "encendida",
        [TEXT_FLASHLIGHT_OFF] = "apagada",
        [TEXT_BATTERY] = "Bateria: %.0f%%",
        [TEXT_GAME_OVER] = "HAS MUERTO",
        [TEXT_PLAY_AGAIN] = "JUGAR DE NUEVO",
        [TEXT_SKIP_TUTORIAL] = "Saltar tutorial",
        [TEXT_CONTINUE_TUTORIAL] = "Continuar",
        [TEXT_INTRO_FOOTER] = "Elige un idioma arriba y continua abajo.",
        [TEXT_ROUND_FOOTER] = "Haz clic en Continuar para jugar este paso.",
        [TEXT_HUD_STATUS_TITLE] = "ESTADO DEL SISTEMA",
        [TEXT_HUD_MAIN_DATA] = "DATOS PRINCIPALES",
        [TEXT_HUD_LOG_TITLE] = "REGISTRO DE DATOS",
        [TEXT_HUD_EXECUTABLE] = "PAQUETE",
        [TEXT_HUD_BYTES] = "bytes",
        [TEXT_HUD_LIMIT] = "LIMITE",
        [TEXT_HUD_VITAL_TITLE] = "VITAL DEL NUCLEO",
        [TEXT_HUD_VIDA] = "VIDA",
        [TEXT_HUD_WEAPON_TITLE] = "ARMAMENTO",
        [TEXT_HUD_BALAS] = "BALAS",
        [TEXT_HUD_CONTROLS_TITLE] = "CONTROLES DE LA TERMINAL",
        [TEXT_HUD_ATIRAR] = "DISPARAR",
        [TEXT_HUD_LASER_SHOT] = "Disparo laser",
        [TEXT_HUD_RECARREGAR] = "RECARGAR",
        [TEXT_HUD_FILL_AMMO] = "Rellenar balas",
        [TEXT_HUD_LANTERNA] = "LINTERNA",
        [TEXT_RELOAD_IN_PROGRESS] = "Recargando",
        [TEXT_STAGE_COMPLETE] = "%s %d completado!",
        [TEXT_NEXT_ROUND] = "Siguiente ronda",
        [TEXT_SHOP_TITLE] = "TIENDA",
        [TEXT_SHOP_COINS] = "%d MONEDAS",
        [TEXT_SHOP_BUY_AMMO] = "MUNICION",
        [TEXT_SHOP_BUY_BATTERY] = "BATERIA",
        [TEXT_SHOP_BUY_HEALTH] = "VIDA",
        [TEXT_SHOP_BUY_LIGHTNING] = "RAYO",
        [TEXT_SHOP_START] = "VOLVER AL JUEGO",
        [TEXT_SHOP_MAX] = "MAX",
        [TEXT_SHOP_LOCKED] = "Ronda 10",
        [TEXT_HUD_LIGHTNING] = "RAYO"
    },
    [LANGUAGE_EN] = {
        [TEXT_LANGUAGE_NAME] = "EN",
        [TEXT_LANGUAGE_BUTTON] = "EN",
        [TEXT_BEST_ROUND] = "Personal best: round %d",
        [TEXT_TUTORIAL_PROGRESS] = "Tutorial %d/4",
        [TEXT_ROUND_PROGRESS] = "Round %d",
        [TEXT_HEALTH] = "Health: %d/%d",
        [TEXT_SHOOT] = "Shoot: SPACE",
        [TEXT_AMMO] = "Ammo: %d/%d",
        [TEXT_RELOADING] = "Reloading: %.1fs",
        [TEXT_RELOAD] = "Reload: R",
        [TEXT_FLASHLIGHT_CONTROL] = "Flashlight: C",
        [TEXT_FLASHLIGHT_STATE] = "Flashlight: %s",
        [TEXT_FLASHLIGHT_ON] = "on",
        [TEXT_FLASHLIGHT_OFF] = "off",
        [TEXT_BATTERY] = "Battery: %.0f%%",
        [TEXT_GAME_OVER] = "YOU DIED",
        [TEXT_PLAY_AGAIN] = "PLAY AGAIN",
        [TEXT_SKIP_TUTORIAL] = "Skip tutorial",
        [TEXT_CONTINUE_TUTORIAL] = "Continue",
        [TEXT_INTRO_FOOTER] = "Choose a language above and continue below.",
        [TEXT_ROUND_FOOTER] = "Click Continue to play this step.",
        [TEXT_HUD_STATUS_TITLE] = "SYSTEM STATUS",
        [TEXT_HUD_MAIN_DATA] = "MAIN DATA",
        [TEXT_HUD_LOG_TITLE] = "DATA LOG",
        [TEXT_HUD_EXECUTABLE] = "PACKAGE",
        [TEXT_HUD_BYTES] = "bytes",
        [TEXT_HUD_LIMIT] = "LIMIT",
        [TEXT_HUD_VITAL_TITLE] = "CORE VITALS",
        [TEXT_HUD_VIDA] = "HEALTH",
        [TEXT_HUD_WEAPON_TITLE] = "WEAPONRY",
        [TEXT_HUD_BALAS] = "AMMO",
        [TEXT_HUD_CONTROLS_TITLE] = "TERMINAL CONTROLS",
        [TEXT_HUD_ATIRAR] = "SHOOT",
        [TEXT_HUD_LASER_SHOT] = "Laser shot",
        [TEXT_HUD_RECARREGAR] = "RELOAD",
        [TEXT_HUD_FILL_AMMO] = "Refill ammo",
        [TEXT_HUD_LANTERNA] = "FLASHLIGHT",
        [TEXT_RELOAD_IN_PROGRESS] = "Reloading",
        [TEXT_STAGE_COMPLETE] = "%s %d complete!",
        [TEXT_NEXT_ROUND] = "Next round",
        [TEXT_SHOP_TITLE] = "SHOP",
        [TEXT_SHOP_COINS] = "%d COINS",
        [TEXT_SHOP_BUY_AMMO] = "AMMO",
        [TEXT_SHOP_BUY_BATTERY] = "BATTERY",
        [TEXT_SHOP_BUY_HEALTH] = "HEALTH",
        [TEXT_SHOP_BUY_LIGHTNING] = "LIGHTNING",
        [TEXT_SHOP_START] = "BACK TO GAME",
        [TEXT_SHOP_MAX] = "MAX",
        [TEXT_SHOP_LOCKED] = "Round 10",
        [TEXT_HUD_LIGHTNING] = "LIGHTNING"
    },
    [LANGUAGE_KO] = {
        [TEXT_LANGUAGE_NAME] = "KO",
        [TEXT_LANGUAGE_BUTTON] = "KO",
        [TEXT_BEST_ROUND] = "최고 기록: 라운드 %d",
        [TEXT_TUTORIAL_PROGRESS] = "튜토리얼 %d/4",
        [TEXT_ROUND_PROGRESS] = "라운드 %d",
        [TEXT_HEALTH] = "체력: %d/%d",
        [TEXT_SHOOT] = "발사: 스페이스",
        [TEXT_AMMO] = "탄약: %d/%d",
        [TEXT_RELOADING] = "재장전 중: %.1f초",
        [TEXT_RELOAD] = "재장전: R",
        [TEXT_FLASHLIGHT_CONTROL] = "손전등: C",
        [TEXT_FLASHLIGHT_STATE] = "손전등: %s",
        [TEXT_FLASHLIGHT_ON] = "켜짐",
        [TEXT_FLASHLIGHT_OFF] = "꺼짐",
        [TEXT_BATTERY] = "배터리: %.0f%%",
        [TEXT_GAME_OVER] = "사망했습니다",
        [TEXT_PLAY_AGAIN] = "다시 시작",
        [TEXT_SKIP_TUTORIAL] = "튜토리얼 건너뛰기",
        [TEXT_CONTINUE_TUTORIAL] = "계속",
        [TEXT_INTRO_FOOTER] = "위에서 언어를 고르고 아래에서 계속하세요.",
        [TEXT_ROUND_FOOTER] = "계속을 클릭해 이 단계를 플레이하세요.",
        [TEXT_HUD_STATUS_TITLE] = "시스템 상태",
        [TEXT_HUD_MAIN_DATA] = "주요 데이터",
        [TEXT_HUD_LOG_TITLE] = "데이터 로그",
        [TEXT_HUD_EXECUTABLE] = "패키지",
        [TEXT_HUD_BYTES] = "바이트",
        [TEXT_HUD_LIMIT] = "제한",
        [TEXT_HUD_VITAL_TITLE] = "코어 생명 신호",
        [TEXT_HUD_VIDA] = "체력",
        [TEXT_HUD_WEAPON_TITLE] = "무장",
        [TEXT_HUD_BALAS] = "탄약",
        [TEXT_HUD_CONTROLS_TITLE] = "터미널 조작",
        [TEXT_HUD_ATIRAR] = "발사",
        [TEXT_HUD_LASER_SHOT] = "레이저 발사",
        [TEXT_HUD_RECARREGAR] = "재장전",
        [TEXT_HUD_FILL_AMMO] = "탄약 채우기",
        [TEXT_HUD_LANTERNA] = "손전등",
        [TEXT_RELOAD_IN_PROGRESS] = "재장전 중",
        [TEXT_STAGE_COMPLETE] = "%s %d 성공!",
        [TEXT_NEXT_ROUND] = "다음 라운드",
        [TEXT_SHOP_TITLE] = "SHOP",
        [TEXT_SHOP_COINS] = "코인: %d",
        [TEXT_SHOP_BUY_AMMO] = "탄약",
        [TEXT_SHOP_BUY_BATTERY] = "배터리 +50%",
        [TEXT_SHOP_BUY_HEALTH] = "최대 체력 +5",
        [TEXT_SHOP_BUY_LIGHTNING] = "LIGHTNING +1",
        [TEXT_SHOP_START] = "라운드 시작",
        [TEXT_SHOP_MAX] = "최대",
        [TEXT_SHOP_LOCKED] = "라운드 10",
        [TEXT_HUD_LIGHTNING] = "LIGHTNING"
    }
};

const char *T(TextId id)
{
    return uiText[currentLanguage][id];
}

Font GetUIFont(void)
{
    return uiFontLoaded ? uiFont : GetFontDefault();
}

Font GetUILatinFont(void)
{
    return uiLatinFontLoaded ? uiLatinFont : GetFontDefault();
}

/* True: this codepoint belongs to the Korean Hangul ranges and must use
 * the CJK font, since the pixel font has no glyphs for it. */
bool IsHangulCodepoint(int codepoint)
{
    if (codepoint >= 0xAC00 && codepoint <= 0xD7A3) return true;   /* syllables */
    if (codepoint >= 0x1100 && codepoint <= 0x11FF) return true;   /* jamo */
    if (codepoint >= 0x3130 && codepoint <= 0x318F) return true;   /* compat jamo */
    return false;
}

/* Try common system fonts first and fall back to raylib's built-in font.
 * Shipping a CJK font would exceed the contest limit by itself. */
static const char *uiFontRelativeCandidates[] = {
    "assets/fonts/NotoSansKR-Subset-Bold.ttf",
    "src/assets/fonts/NotoSansKR-Subset-Bold.ttf",
    UI_FONT_PATH,
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/noto/NotoSansKR-Regular.otf",
    "/usr/share/fonts/truetype/noto/NotoSansKR-Regular.ttf",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    "C:\\Windows\\Fonts\\malgunbd.ttf",
    "C:\\Windows\\Fonts\\malgun.ttf",
    "C:/Windows/Fonts/malgunbd.ttf",
    "C:/Windows/Fonts/malgun.ttf",
    "/mnt/c/Windows/Fonts/malgunbd.ttf",
    "/mnt/c/Windows/Fonts/malgun.ttf",
    "/c/Windows/Fonts/malgunbd.ttf",
    "/c/Windows/Fonts/malgun.ttf",
    "/System/Library/Fonts/Supplemental/AppleSDGothicNeo.ttc",
    "/System/Library/Fonts/AppleSDGothicNeo.ttc",
    NULL
};

static const char *uiLatinFontRelativeCandidates[] = {
    "C:\\Windows\\Fonts\\segoeui.ttf",
    "C:\\Windows\\Fonts\\segoeuib.ttf",
    "C:\\Windows\\Fonts\\arial.ttf",
    "C:\\Windows\\Fonts\\arialbd.ttf",
    "C:/Windows/Fonts/segoeui.ttf",
    "C:/Windows/Fonts/segoeuib.ttf",
    "C:/Windows/Fonts/arial.ttf",
    "C:/Windows/Fonts/arialbd.ttf",
    "/mnt/c/Windows/Fonts/segoeui.ttf",
    "/mnt/c/Windows/Fonts/segoeuib.ttf",
    "/mnt/c/Windows/Fonts/arial.ttf",
    "/mnt/c/Windows/Fonts/arialbd.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
    NULL
};

/* Returns true if `path` is already absolute (starts with '/', a drive
 * letter like "C:", or a UNC/backslash root) and therefore should not be
 * re-anchored to the executable's folder. */
static bool IsAbsoluteFontPath(const char *path)
{
    if (path[0] == '/' || path[0] == '\\')
    {
        return true;
    }

    if (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':')
    {
        return true;
    }

    return false;
}

/* Turns a list of candidate font paths into absolute paths so the font
 * loads correctly no matter what directory the game was launched from.
 * Relative candidates are tried both as-is (covers the working directory
 * the tool used to package/run the game) and anchored to
 * GetApplicationDirectory() (covers double-clicking the .exe/binary
 * directly). Writes up to maxOut paths (each up to maxLen bytes) into
 * outPaths/outBuffer and returns how many were written. */
static int BuildFontCandidatePaths(const char *const *relativeCandidates, char outBuffer[][512], const char **outPaths, int maxOut)
{
    const char *appDir = GetApplicationDirectory();
    int count = 0;

    for (int i = 0; relativeCandidates[i] != NULL && count < maxOut; i++)
    {
        const char *candidate = relativeCandidates[i];

        /* As given (relative to whatever the current working directory is,
         * or already absolute). */
        outPaths[count] = candidate;
        count++;

        if (count >= maxOut)
        {
            break;
        }

        /* Re-anchored to the executable's own folder. */
        if (!IsAbsoluteFontPath(candidate) && appDir != NULL)
        {
            snprintf(outBuffer[count], 512, "%s%s", appDir, candidate);
            outPaths[count] = outBuffer[count];
            count++;
        }
    }

    return count;
}

static bool HasFontExtension(const char *path, const char *extension)
{
    size_t pathLen = strlen(path);
    size_t extensionLen = strlen(extension);

    if (pathLen < extensionLen)
    {
        return false;
    }

    return strcmp(path + pathLen - extensionLen, extension) == 0;
}

static Font LoadUIFontCandidate(const char *path, int fontSize, const int *codepoints, int codepointCount)
{
    if (HasFontExtension(path, ".ttc"))
    {
        int dataSize = 0;
        unsigned char *fileData = LoadFileData(path, &dataSize);
        Font font = { 0 };

        if (fileData != NULL)
        {
            font = LoadFontFromMemory(".ttf", fileData, dataSize, fontSize, codepoints, codepointCount);
            UnloadFileData(fileData);
        }

        return font;
    }

    return LoadFontEx(path, fontSize, codepoints, codepointCount);
}

static void AddUniqueCodepoint(int *codepoints, int *count, int codepoint)
{
    if (*count >= UI_MAX_CODEPOINTS)
    {
        return;
    }

    for (int i = 0; i < *count; i++)
    {
        if (codepoints[i] == codepoint)
        {
            return;
        }
    }

    codepoints[*count] = codepoint;
    (*count)++;
}

int *BuildUICodepoints(int *codepointCount)
{
    static int codepoints[UI_MAX_CODEPOINTS];
    int count = 0;

    for (int c = 32; c <= 126; c++)
    {
        AddUniqueCodepoint(codepoints, &count, c);
    }

    int extraCount = 0;
    int *extraCodepoints = LoadCodepoints("áàâãéêíóôõúçÁÀÂÃÉÊÍÓÔÕÚÇüÜñÑ", &extraCount);
    for (int i = 0; i < extraCount; i++)
    {
        AddUniqueCodepoint(codepoints, &count, extraCodepoints[i]);
    }
    UnloadCodepoints(extraCodepoints);

    for (int textId = 0; textId < (int)TEXT_COUNT; textId++)
    {
        const char *text = uiText[LANGUAGE_KO][textId];
        int textLength = (int)strlen(text);
        int byteIndex = 0;

        while (byteIndex < textLength)
        {
            int codepointByteCount = 0;
            int codepoint = GetCodepointNext(&text[byteIndex], &codepointByteCount);
            if (codepointByteCount <= 0)
            {
                codepointByteCount = 1;
            }

            AddUniqueCodepoint(codepoints, &count, codepoint);
            byteIndex += codepointByteCount;
        }
    }

    const char *extraKoreanText =
        "목표는초록출구까지가는것입니다노란삼각형을조종하고벽에막히지않게길을찾으세요"
        "빨간원은닿으면체력이깎입니다적은가까이오면음악이커집니다분홍마름모는총을쏩니다"
        "직선통로에서멈추면위험합니다총알은적을잠깐녹다운시킵니다"
        "손전등은어두운곳을밝히지만배터리를소모합니다튜토리얼공식게임보스보라색사각형"
        "플레이어를추적하고멀어지면빨라지며사격합니다두명넘게둘러싸지않도록움직입니다"
        "통로에서멈추지말고이동하세요같은행이나열에서벽없이마주치면멈추고발사합니다"
        "재장전사용할수있습니다켜고끕니다모퉁이와출구를확인할때짧게사용하세요"
        "난이도상승올라갈수록더빠르고자주길을계산합니다타이밍관리가중요합니다"
        "감갑경과규께껴꿉끄넓능대되된됩둠든들또뜻럼려맞매박방버번변별부비성센순쏘쏠쓰안압야여옵외의절접정죽줍즉착찰처충칙켭키튼퍼피함항향혀화활후다음";
    int extraKoreanLength = (int)strlen(extraKoreanText);
    for (int byteIndex = 0; byteIndex < extraKoreanLength; )
    {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&extraKoreanText[byteIndex], &codepointByteCount);
        if (codepointByteCount <= 0)
        {
            codepointByteCount = 1;
        }

        AddUniqueCodepoint(codepoints, &count, codepoint);
        byteIndex += codepointByteCount;
    }

    *codepointCount = count;
    return codepoints;
}

void InitUIFont(void)
{
    int codepointCount = 0;
    int *codepoints = BuildUICodepoints(&codepointCount);

    char cjkPathBuffer[80][512];
    const char *cjkPaths[80];
    int cjkPathCount = BuildFontCandidatePaths(uiFontRelativeCandidates, cjkPathBuffer, cjkPaths, 80);

    for (int i = 0; i < cjkPathCount; i++)
    {
        if (!FileExists(cjkPaths[i]))
        {
            continue;
        }

        uiFont = LoadUIFontCandidate(cjkPaths[i], UI_FONT_BAKE_SIZE, codepoints, codepointCount);
        if (uiFont.texture.id > 0)
        {
            break;
        }
    }

    uiFontLoaded = uiFont.texture.id > 0;

    if (uiFontLoaded)
    {
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    int latinCodepoints[140];
    int latinCount = 0;
    for (int c = 32; c <= 126; c++)
    {
        latinCodepoints[latinCount++] = c;
    }

    int latinExtraCount = 0;
    int *latinExtraCodepoints = LoadCodepoints("áàâãéêíóôõúçÁÀÂÃÉÊÍÓÔÕÚÇüÜñÑ", &latinExtraCount);
    for (int i = 0; i < latinExtraCount && latinCount < 140; i++)
    {
        latinCodepoints[latinCount++] = latinExtraCodepoints[i];
    }
    UnloadCodepoints(latinExtraCodepoints);

    char latinPathBuffer[16][512];
    const char *latinPaths[16];
    int latinPathCount = BuildFontCandidatePaths(uiLatinFontRelativeCandidates, latinPathBuffer, latinPaths, 16);

    for (int i = 0; i < latinPathCount; i++)
    {
        if (!FileExists(latinPaths[i]))
        {
            continue;
        }

        uiLatinFont = LoadUIFontCandidate(latinPaths[i], UI_FONT_BAKE_SIZE, latinCodepoints, latinCount);
        if (uiLatinFont.texture.id > 0)
        {
            break;
        }
    }

    uiLatinFontLoaded = uiLatinFont.texture.id > 0;

    if (uiLatinFontLoaded)
    {
        SetTextureFilter(uiLatinFont.texture, TEXTURE_FILTER_BILINEAR);
    }
}

void ShutdownUIFont(void)
{
    if (uiFontLoaded)
    {
        UnloadFont(uiFont);
        uiFontLoaded = false;
    }

    if (uiLatinFontLoaded)
    {
        UnloadFont(uiLatinFont);
        uiLatinFontLoaded = false;
    }
}

float GetDistanceBetweenPoints(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf((dx * dx) + (dy * dy));
}

float GetPointSegmentDistance(Vector2 point, Vector2 segmentStart, Vector2 segmentEnd)
{
    float segmentX = segmentEnd.x - segmentStart.x;
    float segmentY = segmentEnd.y - segmentStart.y;
    float segmentLengthSquared = (segmentX * segmentX) + (segmentY * segmentY);

    if (segmentLengthSquared <= 0.0001f)
    {
        return GetDistanceBetweenPoints(point, segmentStart);
    }

    float t = ((point.x - segmentStart.x) * segmentX + (point.y - segmentStart.y) * segmentY) / segmentLengthSquared;
    t = fmaxf(0.0f, fminf(1.0f, t));

    Vector2 closestPoint = {
        segmentStart.x + segmentX * t,
        segmentStart.y + segmentY * t
    };

    return GetDistanceBetweenPoints(point, closestPoint);
}

float CrossProduct(Vector2 a, Vector2 b, Vector2 c)
{
    return ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
}

bool IsPointInTriangle(Vector2 point, Vector2 a, Vector2 b, Vector2 c)
{
    float area1 = CrossProduct(point, a, b);
    float area2 = CrossProduct(point, b, c);
    float area3 = CrossProduct(point, c, a);
    bool hasNegative = (area1 < 0.0f) || (area2 < 0.0f) || (area3 < 0.0f);
    bool hasPositive = (area1 > 0.0f) || (area2 > 0.0f) || (area3 > 0.0f);

    return !(hasNegative && hasPositive);
}

bool IsPointOnSegment(Vector2 point, Vector2 segmentStart, Vector2 segmentEnd)
{
    return fabsf(CrossProduct(segmentStart, segmentEnd, point)) <= 0.0001f &&
           point.x >= fminf(segmentStart.x, segmentEnd.x) - 0.0001f &&
           point.x <= fmaxf(segmentStart.x, segmentEnd.x) + 0.0001f &&
           point.y >= fminf(segmentStart.y, segmentEnd.y) - 0.0001f &&
           point.y <= fmaxf(segmentStart.y, segmentEnd.y) + 0.0001f;
}

bool DoSegmentsIntersect(Vector2 a, Vector2 b, Vector2 c, Vector2 d)
{
    float abC = CrossProduct(a, b, c);
    float abD = CrossProduct(a, b, d);
    float cdA = CrossProduct(c, d, a);
    float cdB = CrossProduct(c, d, b);

    if (fabsf(abC) <= 0.0001f && IsPointOnSegment(c, a, b)) return true;
    if (fabsf(abD) <= 0.0001f && IsPointOnSegment(d, a, b)) return true;
    if (fabsf(cdA) <= 0.0001f && IsPointOnSegment(a, c, d)) return true;
    if (fabsf(cdB) <= 0.0001f && IsPointOnSegment(b, c, d)) return true;

    if (fabsf(abC) <= 0.0001f || fabsf(abD) <= 0.0001f || fabsf(cdA) <= 0.0001f || fabsf(cdB) <= 0.0001f)
    {
        return false;
    }

    return ((abC < 0.0f && abD > 0.0f) || (abC > 0.0f && abD < 0.0f)) &&
           ((cdA < 0.0f && cdB > 0.0f) || (cdA > 0.0f && cdB < 0.0f));
}

typedef struct TriangleHitbox
{
    Vector2 tip;
    Vector2 right;
    Vector2 left;
} TriangleHitbox;

TriangleHitbox GetPlayerTriangleHitboxScaled(Vector2 position, float scale)
{
    float hitboxRadius = player.radius * 1.1f * scale;

    return (TriangleHitbox){
        {
            position.x + cosf(player.facingAngle) * hitboxRadius * 1.2f,
            position.y + sinf(player.facingAngle) * hitboxRadius * 1.2f
        },
        {
            position.x + cosf(player.facingAngle - 2.45f) * hitboxRadius * 0.98f,
            position.y + sinf(player.facingAngle - 2.45f) * hitboxRadius * 0.98f
        },
        {
            position.x + cosf(player.facingAngle + 2.45f) * hitboxRadius * 0.98f,
            position.y + sinf(player.facingAngle + 2.45f) * hitboxRadius * 0.98f
        }
    };
}

TriangleHitbox GetPlayerTriangleHitbox(Vector2 position)
{
    return GetPlayerTriangleHitboxScaled(position, 1.0f);
}

int GetDifficultyRampLevel(void)
{
    if (inTutorialSequence || officialRound < MODIFIER_START_ROUND)
    {
        return 0;
    }

    return officialRound - MODIFIER_START_ROUND + 1;
}

float GetPlayerSpeedForLevel(void)
{
    float levelBonus = fminf((float)(playerLevel - 1) * 0.75f, 10.0f);
    return 58.0f + levelBonus;
}

float GetRedEnemyTrackDistance(void)
{
    return TILE_SIZE * (5.5f + fminf((float)GetDifficultyRampLevel() * 0.45f, 4.5f));
}

float GetBlueEnemyPathfindChance(void)
{
    if (GetDifficultyRampLevel() <= 0)
    {
        return 0.0f;
    }

    return fminf(0.20f + ((float)GetDifficultyRampLevel() - 1.0f) * 0.08f, 0.85f);
}

float GetEnemySpeedBonus(void)
{
    return (float)GetDifficultyRampLevel() * 1.4f;
}

float GetBossSpeedBonus(void)
{
    return (float)GetDifficultyRampLevel() * 1.15f;
}

float GetBossShootCooldown(void)
{
    float pressureBonus = currentModifiers.bossPressure ? 0.10f : 0.0f;
    return fmaxf(0.48f, 0.92f - ((float)GetDifficultyRampLevel() * 0.018f) - pressureBonus);
}

float GetBossFarSpeedMultiplier(void)
{
    return fminf(BOSS_FAR_SPEED_MULTIPLIER + ((float)GetDifficultyRampLevel() * 0.03f), 2.0f);
}

bool IsTrapActiveAtCell(int x, int y)
{
    float cycle = TRAP_ACTIVE_TIME + TRAP_INACTIVE_TIME;
    float offset = (float)((x * 17 + y * 11) % 100) * 0.01f;
    return fmodf((float)GetTime() + offset, cycle) < TRAP_ACTIVE_TIME;
}

Vector2 GetFlashlightCenter(void)
{
    Vector2 flashlightCenter = player.position;
    flashlightCenter.x += cosf(player.facingAngle) * (TILE_SIZE * 2.4f);
    flashlightCenter.y += sinf(player.facingAngle) * (TILE_SIZE * 2.4f);
    return flashlightCenter;
}

bool IsWorldPositionVisible(Vector2 position)
{
    if (lightningRevealTimer > 0.0f)
    {
        return true;
    }

    if (!currentRoundConfig.flashlightEnabled)
    {
        return true;
    }

    if (GetDistanceBetweenPoints(position, player.position) <= TILE_SIZE * 2.6f * GetDarknessVisionScale())
    {
        return true;
    }

    if (currentRoundConfig.flashlightEnabled && flashlightOn)
    {
        Vector2 flashlightCenter = GetFlashlightCenter();
        if (GetDistanceBetweenPoints(position, flashlightCenter) <= TILE_SIZE * 3.1f * GetDarknessVisionScale())
        {
            return true;
        }
    }

    return false;
}

float GetReadableGlyphSpacing(bool isHangul, float spacing)
{
    float scale = GetUIScale();
    float glyphSpacing = isHangul ? spacing * 0.35f : spacing;
    float minimumSpacing = isHangul ? 0.20f * scale : 0.45f * scale;

    return fmaxf(glyphSpacing, minimumSpacing);
}

/* Draws one line (no '\n'), using a readable Latin font for PT/ES/EN and
 * a Korean-capable font only for Hangul glyphs. */
void DrawMixedLine(Font latinFont, Font cjkFont, const char *line, Vector2 position, float fontSize, float spacing)
{
    int length = (int)strlen(line);
    int byteIndex = 0;
    float x = position.x;

    while (byteIndex < length)
    {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&line[byteIndex], &codepointByteCount);
        if (codepointByteCount <= 0)
        {
            codepointByteCount = 1;
        }

        bool isHangul = IsHangulCodepoint(codepoint);
        Font font = isHangul ? cjkFont : latinFont;
        float glyphSpacing = GetReadableGlyphSpacing(isHangul, spacing);

        char glyphBuf[8];
        int copyLen = (codepointByteCount < 7) ? codepointByteCount : 7;
        memcpy(glyphBuf, &line[byteIndex], copyLen);
        glyphBuf[copyLen] = '\0';

        DrawTextEx(font, glyphBuf, (Vector2){ x, position.y }, fontSize, 0.0f, currentDrawColor);
        Vector2 glyphSize = MeasureTextEx(font, glyphBuf, fontSize, 0.0f);
        x += glyphSize.x + glyphSpacing;

        byteIndex += codepointByteCount;
    }
}

Vector2 MeasureMixedLine(Font latinFont, Font cjkFont, const char *line, float fontSize, float spacing)
{
    int length = (int)strlen(line);
    int byteIndex = 0;
    float width = 0.0f;
    float lastGlyphSpacing = 0.0f;

    while (byteIndex < length)
    {
        int codepointByteCount = 0;
        int codepoint = GetCodepointNext(&line[byteIndex], &codepointByteCount);
        if (codepointByteCount <= 0)
        {
            codepointByteCount = 1;
        }

        bool isHangul = IsHangulCodepoint(codepoint);
        Font font = isHangul ? cjkFont : latinFont;
        float glyphSpacing = GetReadableGlyphSpacing(isHangul, spacing);

        char glyphBuf[8];
        int copyLen = (codepointByteCount < 7) ? codepointByteCount : 7;
        memcpy(glyphBuf, &line[byteIndex], copyLen);
        glyphBuf[copyLen] = '\0';

        Vector2 glyphSize = MeasureTextEx(font, glyphBuf, fontSize, 0.0f);
        width += glyphSize.x + glyphSpacing;
        lastGlyphSpacing = glyphSpacing;

        byteIndex += codepointByteCount;
    }

    if (width > 0.0f)
    {
        width -= lastGlyphSpacing;
    }

    return (Vector2){ width, fontSize };
}

/* Handles '\n' with a single medium-weight pass and a small shadow, instead
 * of faking heavy bold with many offset copies. */
void DrawTextBoldEx(Font latinFont, Font cjkFont, const char *text, Vector2 position, float fontSize, float spacing, Color color)
{
    float lineHeight = fontSize * 1.4f;
    char lineBuf[512];
    int textLen = (int)strlen(text);
    int lineStart = 0;
    int lineIndex = 0;

    for (int i = 0; i <= textLen; i++)
    {
        if (text[i] != '\n' && text[i] != '\0')
        {
            continue;
        }

        int lineLen = i - lineStart;
        if (lineLen > 511)
        {
            lineLen = 511;
        }
        memcpy(lineBuf, &text[lineStart], lineLen);
        lineBuf[lineLen] = '\0';

        float y = position.y + lineIndex * lineHeight;

        currentDrawColor = color;
        DrawMixedLine(latinFont, cjkFont, lineBuf, (Vector2){ position.x, y }, fontSize, spacing);

        lineStart = i + 1;
        lineIndex++;
    }
}

void DrawTextStrong(const char *text, int x, int y, int fontSize, Color color, Color shadowColor)
{
    Font latinFont = GetUILatinFont();
    Font cjkFont = GetUIFont();
    float shadowOffset = fmaxf(1.0f, GetUIScale());
    Vector2 shadowPos = { (float)x + shadowOffset, (float)y + shadowOffset };
    Vector2 textPos = { (float)x, (float)y };
    Color softShadow = Fade(shadowColor, 0.62f);

    DrawTextBoldEx(latinFont, cjkFont, text, shadowPos, (float)fontSize, 1.0f, softShadow);
    DrawTextBoldEx(latinFont, cjkFont, text, textPos, (float)fontSize, 1.0f, color);
}

/* Same as DrawTextStrong, but lets the caller control the space between
 * letters. Used where legibility matters most (tutorial explanations). */
void DrawTextStrongSpaced(const char *text, int x, int y, int fontSize, float spacing, Color color, Color shadowColor)
{
    Font latinFont = GetUILatinFont();
    Font cjkFont = GetUIFont();
    float shadowOffset = fmaxf(1.0f, GetUIScale());
    Vector2 shadowPos = { (float)x + shadowOffset, (float)y + shadowOffset };
    Vector2 textPos = { (float)x, (float)y };
    Color softShadow = Fade(shadowColor, 0.62f);

    DrawTextBoldEx(latinFont, cjkFont, text, shadowPos, (float)fontSize, spacing, softShadow);
    DrawTextBoldEx(latinFont, cjkFont, text, textPos, (float)fontSize, spacing, color);
}

int FitFontSizeToWidth(const char *text, int fontSize, int minFontSize, float spacing, float maxWidth)
{
    int fittedSize = fontSize;

    while (fittedSize > minFontSize && MeasureTextStrongSpaced(text, fittedSize, spacing).x > maxWidth)
    {
        fittedSize--;
    }

    return fittedSize;
}

void DrawTextStrongFit(const char *text, int x, int y, int fontSize, int minFontSize, float spacing, float maxWidth, Color color, Color shadowColor)
{
    int fittedSize = FitFontSizeToWidth(text, fontSize, minFontSize, spacing, maxWidth);

    if (spacing == 1.0f)
    {
        DrawTextStrong(text, x, y, fittedSize, color, shadowColor);
    }
    else
    {
        DrawTextStrongSpaced(text, x, y, fittedSize, spacing, color, shadowColor);
    }
}

void DrawLanguageFlag(Language language, Rectangle bounds)
{
    DrawRectangleRounded(bounds, 0.18f, 6, RAYWHITE);

    if (language == LANGUAGE_PT_BR)
    {
        /* Bandeira do Brasil no estilo do emoji: fundo verde, losango
         * amarelo ocupando quase toda a moldura, globo azul com a faixa
         * branca e algumas estrelas, para ficar claramente reconhecível
         * mesmo no tamanho pequeno do botão de idioma. */
        DrawRectangleRec(bounds, (Color){ 0, 155, 68, 255 });

        float marginX = bounds.width * 0.04f;
        float marginY = bounds.height * 0.06f;
        Vector2 top = { bounds.x + bounds.width * 0.5f, bounds.y + marginY };
        Vector2 right = { bounds.x + bounds.width - marginX, bounds.y + bounds.height * 0.5f };
        Vector2 bottom = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height - marginY };
        Vector2 left = { bounds.x + marginX, bounds.y + bounds.height * 0.5f };
        Vector2 center = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };

        DrawTriangle(top, right, bottom, (Color){ 255, 205, 0, 255 });
        DrawTriangle(top, bottom, left, (Color){ 255, 205, 0, 255 });

        float globeRadius = bounds.height * 0.30f;
        DrawCircleV(center, globeRadius, (Color){ 30, 75, 165, 255 });
        /* Faixa branca central com leve inclinação, como na bandeira real. */
        DrawLineEx(
            (Vector2){ center.x - globeRadius * 0.95f, center.y - globeRadius * 0.12f },
            (Vector2){ center.x + globeRadius * 0.95f, center.y + globeRadius * 0.30f },
            fmaxf(1.5f, bounds.height * 0.07f),
            RAYWHITE
        );
        /* Pequenas estrelas para reforçar o padrão do globo. */
        DrawCircleV((Vector2){ center.x - globeRadius * 0.35f, center.y - globeRadius * 0.45f }, fmaxf(0.6f, bounds.height * 0.02f), RAYWHITE);
        DrawCircleV((Vector2){ center.x + globeRadius * 0.15f, center.y - globeRadius * 0.55f }, fmaxf(0.6f, bounds.height * 0.02f), RAYWHITE);
        DrawCircleV((Vector2){ center.x + globeRadius * 0.5f, center.y + globeRadius * 0.05f }, fmaxf(0.6f, bounds.height * 0.02f), RAYWHITE);
        DrawCircleLinesV(center, globeRadius, Fade(BLACK, 0.35f));
    }
    else if (language == LANGUAGE_ES)
    {
        Color spainRed = (Color){ 173, 21, 35, 255 };
        Color spainYellow = (Color){ 255, 196, 0, 255 };
        DrawRectangleRec((Rectangle){ bounds.x, bounds.y, bounds.width, bounds.height * 0.25f }, spainRed);
        DrawRectangleRec((Rectangle){ bounds.x, bounds.y + bounds.height * 0.25f, bounds.width, bounds.height * 0.5f }, spainYellow);
        DrawRectangleRec((Rectangle){ bounds.x, bounds.y + bounds.height * 0.75f, bounds.width, bounds.height * 0.25f }, spainRed);
        DrawRectangleRec((Rectangle){ bounds.x + bounds.width * 0.14f, bounds.y + bounds.height * 0.32f, bounds.width * 0.18f, bounds.height * 0.36f }, Fade(spainRed, 0.85f));
    }
    else if (language == LANGUAGE_EN)
    {
        for (int stripe = 0; stripe < 7; stripe++)
        {
            Color stripeColor = (stripe % 2 == 0) ? (Color){ 190, 20, 40, 255 } : RAYWHITE;
            DrawRectangleRec((Rectangle){ bounds.x, bounds.y + (float)stripe * bounds.height / 7.0f, bounds.width, bounds.height / 7.0f }, stripeColor);
        }
        DrawRectangleRec((Rectangle){ bounds.x, bounds.y, bounds.width * 0.45f, bounds.height * 0.55f }, (Color){ 30, 55, 130, 255 });
    }
    else
    {
        DrawRectangleRec(bounds, RAYWHITE);
        DrawCircle((int)(bounds.x + bounds.width * 0.5f), (int)(bounds.y + bounds.height * 0.5f), bounds.height * 0.25f, (Color){ 210, 35, 55, 255 });
        DrawCircle((int)(bounds.x + bounds.width * 0.5f), (int)(bounds.y + bounds.height * 0.58f), bounds.height * 0.22f, (Color){ 20, 65, 160, 255 });
    }

    DrawRectangleRoundedLinesEx(bounds, 0.18f, 6, 1.0f, Fade(BLACK, 0.45f));
}
Rectangle GetLanguageButtonRect(int languageIndex)
{
    float scale = GetUIScale();
    float gap = 6.0f * scale;
    float w = (float)GetScreenWidth();

    if (IsCompactLayout())
    {
        /* A single compact row sits below the logo. It scales down with the
         * window, so all four language buttons remain inside the viewport. */
        float buttonWidth = (w - (28.0f * scale) - (3.0f * gap)) / 4.0f;
        buttonWidth = fmaxf(62.0f * scale, buttonWidth);
        return (Rectangle){
            14.0f * scale + languageIndex * (buttonWidth + gap),
            45.0f * scale,
            buttonWidth,
            28.0f * scale
        };
    }

    float maxWidth = 112.0f * scale;
    float minWidth = 78.0f * scale;
    float available = w - (300.0f * scale);
    float buttonWidth = fminf(maxWidth, fmaxf(minWidth, (available - (3.0f * gap)) / 4.0f));
    float totalWidth = (4.0f * buttonWidth) + (3.0f * gap);
    float startX = w - totalWidth - (16.0f * scale);

    return (Rectangle){
        startX + ((float)languageIndex * (buttonWidth + gap)),
        16.0f * scale,
        buttonWidth,
        34.0f * scale
    };
}

bool HandleLanguageButtons(void)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return false;
    }

    Vector2 mousePosition = GetMousePosition();
    for (int i = 0; i < (int)LANGUAGE_COUNT; i++)
    {
        if (CheckCollisionPointRec(mousePosition, GetLanguageButtonRect(i)))
        {
            currentLanguage = (Language)i;
            return true;
        }
    }

    return false;
}
void DrawLanguageButtons(void)
{
    Vector2 mousePosition = GetMousePosition();
    float scale = GetUIScale();

    for (int i = 0; i < (int)LANGUAGE_COUNT; i++)
    {
        Rectangle button = GetLanguageButtonRect(i);
        bool selected = currentLanguage == (Language)i;
        bool hovered = CheckCollisionPointRec(mousePosition, button);
        Color fillColor = selected ? (Color){ 18, 34, 88, 245 } : (Color){ 5, 14, 40, 235 };
        Color borderColor = selected ? (Color){ 130, 105, 255, 255 } : Fade(HUD_BORDER_COLOR, 0.65f);
        const char *label = uiText[i][TEXT_LANGUAGE_BUTTON];

        if (hovered && !selected)
        {
            fillColor = (Color){ 12, 28, 68, 245 };
            borderColor = HUD_BORDER_COLOR;
        }

        DrawRectangleRounded(button, 0.16f, 8, fillColor);
        DrawRectangleRoundedLinesEx(
            button, 0.16f, 8,
            selected ? 2.0f * scale : 1.0f * scale,
            borderColor
        );

        /*
         * Reserve a dedicated area for the flag.  PT-BR is the longest label,
         * so its font is fitted independently instead of colliding with the
         * flag or touching the border.
         */
        float flagWidth = 30.0f * scale;
        float flagHeight = 21.0f * scale;
        float flagRight = 7.0f * scale;
        float labelLeft = 10.0f * scale;
        float labelRight = flagRight + flagWidth + 7.0f * scale;
        float labelMaxWidth = button.width - labelLeft - labelRight;

        int fontSize = FitFontSizeToWidth(
            label,
            ScaleFontSize(13.0f),
            ScaleFontSize(8.0f),
            0.35f * scale,
            labelMaxWidth
        );

        float labelWidth = MeasureTextStrongSpaced(
            label, fontSize, 0.35f * scale
        ).x;

        float labelX = button.x + labelLeft;
        if (labelWidth < labelMaxWidth)
        {
            labelX += (labelMaxWidth - labelWidth) * 0.5f;
        }

        float labelY = button.y + (button.height - (float)fontSize * 1.15f) * 0.5f;

        DrawTextStrongSpaced(
            label,
            (int)labelX,
            (int)labelY,
            fontSize,
            0.35f * scale,
            RAYWHITE,
            BLACK
        );

        Rectangle flag = {
            button.x + button.width - flagRight - flagWidth,
            button.y + (button.height - flagHeight) * 0.5f,
            flagWidth,
            flagHeight
        };

        DrawLanguageFlag((Language)i, flag);
    }
}

/* Measures multiline text drawn with DrawTextStrongSpaced so panels can
 * size themselves around it instead of guessing a fixed height. */
Vector2 MeasureTextStrongSpaced(const char *text, int fontSize, float spacing)
{
    Font latinFont = GetUILatinFont();
    Font cjkFont = GetUIFont();
    float lineHeight = (float)fontSize * 1.4f;
    float maxWidth = 0.0f;
    int lineCount = 1;
    char lineBuf[512];
    int textLen = (int)strlen(text);
    int lineStart = 0;

    for (int i = 0; i <= textLen; i++)
    {
        if (text[i] != '\n' && text[i] != '\0')
        {
            continue;
        }

        int lineLen = i - lineStart;
        if (lineLen > 511)
        {
            lineLen = 511;
        }
        memcpy(lineBuf, &text[lineStart], lineLen);
        lineBuf[lineLen] = '\0';

        Vector2 size = MeasureMixedLine(latinFont, cjkFont, lineBuf, (float)fontSize, spacing);
        if (size.x > maxWidth)
        {
            maxWidth = size.x;
        }

        lineStart = i + 1;
        if (text[i] == '\n')
        {
            lineCount++;
        }
    }

    return (Vector2){ maxWidth, lineCount * lineHeight };
}

void DrawMazeGrid(void)
{
    MazeLayout layout = GetMazeLayout();
    float drawTileWidth = (float)TILE_SIZE * layout.scale.x;
    float drawTileHeight = (float)TILE_SIZE * layout.scale.y;
    Rectangle mazeFrame = {
        layout.offset.x - (drawTileWidth * 0.35f),
        layout.offset.y - (drawTileHeight * 0.35f),
        (GRID_WIDTH * drawTileWidth) + (drawTileWidth * 0.7f),
        (GRID_HEIGTH * drawTileHeight) + (drawTileHeight * 0.7f)
    };

    DrawRectangleRounded(mazeFrame, 0.05f, 10, MAZE_SHADOW_COLOR);
    /* Glowing rounded border, blue on the top-left fading to purple on the
     * bottom-right, echoing the reference dashboard's frame glow. Layered
     * outlines (soft, wide, faded outward + a crisp inner line) fake the
     * soft-glow look without needing shaders or image assets. */
    DrawRectangleRoundedLinesEx((Rectangle){ mazeFrame.x - 3.0f, mazeFrame.y - 3.0f, mazeFrame.width + 6.0f, mazeFrame.height + 6.0f }, 0.05f, 10, 6.0f, Fade(HUD_ACCENT_COLOR, 0.18f));
    DrawRectangleRoundedLinesEx((Rectangle){ mazeFrame.x - 1.5f, mazeFrame.y - 1.5f, mazeFrame.width + 3.0f, mazeFrame.height + 3.0f }, 0.05f, 10, 3.0f, Fade(HUD_BORDER_COLOR, 0.35f));
    DrawRectangleRoundedLinesEx(mazeFrame, 0.05f, 10, 2.0f, HUD_BORDER_COLOR);

    for (int y = 0; y < GRID_HEIGTH; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            Color cellColor = BLACK;
            Vector2 cellCenter = {
                ((float)x + 0.5f) * TILE_SIZE,
                ((float)y + 0.5f) * TILE_SIZE
            };
            bool cellVisible = IsWorldPositionVisible(cellCenter);

            if (grid[y][x] == CELL_WALL)
            {
                cellColor = MAZE_WALL_COLOR;
            }
            else if (grid[y][x] == CELL_PATH)
            {
                cellColor = MAZE_PATH_COLOR;
            }
            else if (grid[y][x] == CELL_EXIT)
            {
                cellColor = MAZE_EXIT_COLOR;
            }
            else if (grid[y][x] == CELL_LOCKED_EXIT)
            {
                cellColor = (Color){ 82, 56, 18, 255 };
            }
            else if (grid[y][x] == CELL_COIN || grid[y][x] == CELL_AMMO_PICKUP || grid[y][x] == CELL_BATTERY_PICKUP || grid[y][x] == CELL_KEY || grid[y][x] == CELL_HEALTH_PICKUP || grid[y][x] == CELL_TRAP)
            {
                cellColor = MAZE_PATH_COLOR;
            }

            if (!cellVisible && grid[y][x] != CELL_TRAP)
            {
                cellColor = (Color){ 6, 6, 6, 255 };
            }

            Vector2 position = { layout.offset.x + ((float)x * drawTileWidth), layout.offset.y + ((float)y * drawTileHeight) };
            Vector2 size = { drawTileWidth, drawTileHeight };
            bool drawHiddenKeyPulse = grid[y][x] == CELL_KEY && currentRoundConfig.flashlightEnabled && !cellVisible;
            bool drawHiddenExtraPulse = currentRoundConfig.flashlightEnabled && !cellVisible &&
                                        (grid[y][x] == CELL_AMMO_PICKUP ||
                                         grid[y][x] == CELL_BATTERY_PICKUP ||
                                         grid[y][x] == CELL_HEALTH_PICKUP);
            float keyCycle = fmodf((float)GetTime(), 5.0f);
            float keyPulse = drawHiddenKeyPulse && keyCycle < 0.9f ? sinf((keyCycle / 0.9f) * 3.1415926f) : 0.0f;
            DrawRectangleV(position, size, cellColor);

            if (grid[y][x] == CELL_PATH && cellVisible)
            {
                DrawRectangleLinesEx((Rectangle){ position.x, position.y, size.x, size.y }, fmaxf(1.0f, layout.drawScale), Fade(LIGHTGRAY, 0.08f));
            }
            else if (grid[y][x] == CELL_EXIT && cellVisible)
            {
                DrawRectangleLinesEx((Rectangle){ position.x, position.y, size.x, size.y }, fmaxf(2.0f, 2.0f * layout.drawScale), RAYWHITE);
            }
            else if (grid[y][x] == CELL_LOCKED_EXIT && cellVisible)
            {
                DrawRectangleLinesEx((Rectangle){ position.x, position.y, size.x, size.y }, fmaxf(2.0f, 2.0f * layout.drawScale), GOLD);
                DrawLineEx((Vector2){ position.x + size.x * 0.25f, position.y + size.y * 0.25f },
                           (Vector2){ position.x + size.x * 0.75f, position.y + size.y * 0.75f }, fmaxf(2.0f, 2.0f * layout.drawScale), GOLD);
                DrawLineEx((Vector2){ position.x + size.x * 0.75f, position.y + size.y * 0.25f },
                           (Vector2){ position.x + size.x * 0.25f, position.y + size.y * 0.75f }, fmaxf(2.0f, 2.0f * layout.drawScale), GOLD);
            }

            if (cellVisible || keyPulse > 0.02f || drawHiddenExtraPulse || grid[y][x] == CELL_TRAP)
            {
                Vector2 pickupCenter = { position.x + size.x * 0.5f, position.y + size.y * 0.5f };
                float pickupRadius = fmaxf(3.0f, 4.5f * layout.drawScale);
                float softPickupPulse = currentRoundConfig.flashlightEnabled ? (0.55f + 0.45f * sinf((float)GetTime() * 3.4f)) : 0.0f;

                if (grid[y][x] == CELL_COIN)
                {
                    DrawCircleV(pickupCenter, pickupRadius * 1.7f, Fade(GOLD, 0.25f));
                    DrawCircleLines((int)pickupCenter.x, (int)pickupCenter.y, pickupRadius, GOLD);
                    DrawCircleV(pickupCenter, pickupRadius * 0.35f, GOLD);
                }
                else if (grid[y][x] == CELL_AMMO_PICKUP)
                {
                    Color ammoColor = cellVisible ? HUD_BORDER_COLOR : Fade(HUD_BORDER_COLOR, 0.38f + 0.30f * softPickupPulse);
                    Color ammoCore = cellVisible ? RAYWHITE : Fade(RAYWHITE, 0.35f + 0.25f * softPickupPulse);
                    DrawCircleV(pickupCenter, pickupRadius * (2.2f + softPickupPulse * 0.9f), Fade(HUD_BORDER_COLOR, 0.08f + 0.14f * softPickupPulse));
                    DrawRectangleRounded((Rectangle){ pickupCenter.x - pickupRadius * 1.4f, pickupCenter.y - pickupRadius, pickupRadius * 2.8f, pickupRadius * 2.0f }, 0.18f, 5, Fade(ammoColor, cellVisible ? 0.35f : 0.18f));
                    DrawRectangleRoundedLinesEx((Rectangle){ pickupCenter.x - pickupRadius * 1.4f, pickupCenter.y - pickupRadius, pickupRadius * 2.8f, pickupRadius * 2.0f }, 0.18f, 5, 1.5f, ammoColor);
                    DrawLineEx((Vector2){ pickupCenter.x - pickupRadius * 0.8f, pickupCenter.y }, (Vector2){ pickupCenter.x + pickupRadius * 0.8f, pickupCenter.y }, 2.0f, ammoCore);
                }
                else if (grid[y][x] == CELL_BATTERY_PICKUP)
                {
                    Color batteryColor = cellVisible ? GOLD : Fade(GOLD, 0.36f + 0.32f * softPickupPulse);
                    DrawCircleV(pickupCenter, pickupRadius * (2.1f + softPickupPulse * 0.8f), Fade(GOLD, 0.08f + 0.13f * softPickupPulse));
                    DrawRectangleRoundedLinesEx((Rectangle){ pickupCenter.x - pickupRadius * 1.2f, pickupCenter.y - pickupRadius * 0.7f, pickupRadius * 2.0f, pickupRadius * 1.4f }, 0.2f, 5, 1.5f, batteryColor);
                    DrawRectangleRec((Rectangle){ pickupCenter.x + pickupRadius * 0.95f, pickupCenter.y - pickupRadius * 0.25f, pickupRadius * 0.3f, pickupRadius * 0.5f }, batteryColor);
                }
                else if (grid[y][x] == CELL_KEY)
                {
                    if (drawHiddenKeyPulse)
                    {
                        DrawCircleV(pickupCenter, pickupRadius * (3.8f + keyPulse * 4.4f), Fade(GOLD, 0.16f + keyPulse * 0.62f));
                    }
                    else
                    {
                        DrawCircleV(pickupCenter, pickupRadius * (2.3f + softPickupPulse * 1.2f), Fade(GOLD, 0.12f + 0.20f * softPickupPulse));
                    }
                    DrawCircleV((Vector2){ pickupCenter.x - pickupRadius * 0.55f, pickupCenter.y }, pickupRadius * 0.75f, Fade(GOLD, 0.25f));
                    DrawCircleLines((int)(pickupCenter.x - pickupRadius * 0.55f), (int)pickupCenter.y, pickupRadius * 0.7f, GOLD);
                    DrawLineEx((Vector2){ pickupCenter.x, pickupCenter.y }, (Vector2){ pickupCenter.x + pickupRadius * 1.25f, pickupCenter.y }, 2.0f, GOLD);
                    DrawLineEx((Vector2){ pickupCenter.x + pickupRadius * 0.72f, pickupCenter.y }, (Vector2){ pickupCenter.x + pickupRadius * 0.72f, pickupCenter.y + pickupRadius * 0.55f }, 2.0f, GOLD);
                    DrawLineEx((Vector2){ pickupCenter.x + pickupRadius * 1.08f, pickupCenter.y }, (Vector2){ pickupCenter.x + pickupRadius * 1.08f, pickupCenter.y + pickupRadius * 0.45f }, 2.0f, GOLD);
                }
                else if (grid[y][x] == CELL_HEALTH_PICKUP)
                {
                    Color healthColor = cellVisible ? (Color){ 190, 70, 255, 255 } : Fade((Color){ 190, 70, 255, 255 }, 0.36f + 0.32f * softPickupPulse);
                    DrawCircleV(pickupCenter, pickupRadius * (1.8f + softPickupPulse * 0.9f), Fade(healthColor, 0.14f + 0.15f * softPickupPulse));
                    DrawRectangleRounded((Rectangle){ pickupCenter.x - pickupRadius * 0.35f, pickupCenter.y - pickupRadius * 1.25f, pickupRadius * 0.7f, pickupRadius * 2.5f }, 0.2f, 5, healthColor);
                    DrawRectangleRounded((Rectangle){ pickupCenter.x - pickupRadius * 1.25f, pickupCenter.y - pickupRadius * 0.35f, pickupRadius * 2.5f, pickupRadius * 0.7f }, 0.2f, 5, healthColor);
                    DrawRectangleLinesEx((Rectangle){ pickupCenter.x - pickupRadius * 1.25f, pickupCenter.y - pickupRadius * 1.25f, pickupRadius * 2.5f, pickupRadius * 2.5f }, 1.0f, Fade(RAYWHITE, 0.65f));
                }
                else if (grid[y][x] == CELL_TRAP)
                {
                    bool trapActive = IsTrapActiveAtCell(x, y);
                    float pulse = trapActive ? (0.55f + 0.45f * sinf((float)GetTime() * 10.0f)) : 0.25f;
                    Color trapColor = trapActive ? (Color){ 255, 70, 90, 255 } : (Color){ 90, 95, 115, 255 };
                    if (!cellVisible)
                    {
                        trapColor = trapActive ? Fade(trapColor, 0.92f) : Fade(trapColor, 0.55f);
                    }
                    DrawCircleV(pickupCenter, pickupRadius * 1.35f, Fade(trapColor, trapActive ? 0.24f + 0.28f * pulse : 0.18f));
                    DrawCircleV(pickupCenter, pickupRadius * 0.92f, trapColor);
                    DrawRectangleRounded((Rectangle){ pickupCenter.x - pickupRadius * 0.52f, pickupCenter.y - pickupRadius * 1.34f, pickupRadius * 1.04f, pickupRadius * 0.42f }, 0.25f, 5, Fade(RAYWHITE, trapActive ? 0.82f : 0.35f));
                    DrawLineEx((Vector2){ pickupCenter.x + pickupRadius * 0.18f, pickupCenter.y - pickupRadius * 1.36f },
                               (Vector2){ pickupCenter.x + pickupRadius * 0.82f, pickupCenter.y - pickupRadius * 1.78f }, fmaxf(1.0f, 1.6f * layout.drawScale), trapColor);
                    if (trapActive)
                    {
                        DrawCircleV((Vector2){ pickupCenter.x + pickupRadius * 0.98f, pickupCenter.y - pickupRadius * 1.88f }, pickupRadius * (0.28f + 0.18f * pulse), GOLD);
                    }
                }
            }
        }
    }
}

bool IsWallCell(int x, int y)
{
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGTH)
    {
        return true;
    }

    return grid[y][x] == CELL_WALL || grid[y][x] == CELL_LOCKED_EXIT;
}

/* True circle-vs-tile overlap test: finds the closest point of the wall
 * cell's rectangle to the circle's center and checks if that point is
 * actually inside the circle. This lets the player hug a wall and only
 * get blocked once it truly touches it, instead of the old bounding-box
 * check which blocked movement early (including on the empty corner of a
 * diagonal cell that the circle never actually reached). */
bool CircleOverlapsWallCell(Vector2 center, float radius, int cellX, int cellY)
{
    if (!IsWallCell(cellX, cellY))
    {
        return false;
    }

    float cellLeft = (float)(cellX * TILE_SIZE);
    float cellTop = (float)(cellY * TILE_SIZE);
    float cellRight = cellLeft + TILE_SIZE;
    float cellBottom = cellTop + TILE_SIZE;

    float closestX = fmaxf(cellLeft, fminf(center.x, cellRight));
    float closestY = fmaxf(cellTop, fminf(center.y, cellBottom));

    float dx = center.x - closestX;
    float dy = center.y - closestY;

    return (dx * dx + dy * dy) < (radius * radius);
}

bool IsPointInRectangleInclusive(Vector2 point, Rectangle rectangle)
{
    return point.x >= rectangle.x && point.x <= rectangle.x + rectangle.width &&
           point.y >= rectangle.y && point.y <= rectangle.y + rectangle.height;
}

bool TriangleOverlapsRectangle(TriangleHitbox triangle, Rectangle rectangle)
{
    Vector2 topLeft = { rectangle.x, rectangle.y };
    Vector2 topRight = { rectangle.x + rectangle.width, rectangle.y };
    Vector2 bottomRight = { rectangle.x + rectangle.width, rectangle.y + rectangle.height };
    Vector2 bottomLeft = { rectangle.x, rectangle.y + rectangle.height };
    Vector2 trianglePoints[3] = { triangle.tip, triangle.right, triangle.left };
    Vector2 rectanglePoints[4] = { topLeft, topRight, bottomRight, bottomLeft };

    for (int i = 0; i < 3; i++)
    {
        if (IsPointInRectangleInclusive(trianglePoints[i], rectangle))
        {
            return true;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        if (IsPointInTriangle(rectanglePoints[i], triangle.tip, triangle.right, triangle.left))
        {
            return true;
        }
    }

    for (int triangleEdge = 0; triangleEdge < 3; triangleEdge++)
    {
        Vector2 triangleStart = trianglePoints[triangleEdge];
        Vector2 triangleEnd = trianglePoints[(triangleEdge + 1) % 3];

        for (int rectangleEdge = 0; rectangleEdge < 4; rectangleEdge++)
        {
            Vector2 rectangleStart = rectanglePoints[rectangleEdge];
            Vector2 rectangleEnd = rectanglePoints[(rectangleEdge + 1) % 4];

            if (DoSegmentsIntersect(triangleStart, triangleEnd, rectangleStart, rectangleEnd))
            {
                return true;
            }
        }
    }

    return false;
}

bool TriangleOverlapsWallCell(TriangleHitbox triangle, int cellX, int cellY)
{
    if (!IsWallCell(cellX, cellY))
    {
        return false;
    }

    Rectangle cellRectangle = {
        (float)(cellX * TILE_SIZE) + PLAYER_COLLISION_SKIN,
        (float)(cellY * TILE_SIZE) + PLAYER_COLLISION_SKIN,
        TILE_SIZE - PLAYER_COLLISION_SKIN * 2.0f,
        TILE_SIZE - PLAYER_COLLISION_SKIN * 2.0f
    };

    return TriangleOverlapsRectangle(triangle, cellRectangle);
}

bool IsPositionBlocked(Vector2 position, float radius)
{
    int left = (int)floorf((position.x - radius) / TILE_SIZE);
    int right = (int)floorf((position.x + radius) / TILE_SIZE);
    int top = (int)floorf((position.y - radius) / TILE_SIZE);
    int bottom = (int)floorf((position.y + radius) / TILE_SIZE);

    for (int y = top; y <= bottom; y++)
    {
        for (int x = left; x <= right; x++)
        {
            if (CircleOverlapsWallCell(position, radius, x, y))
            {
                return true;
            }
        }
    }

    return false;
}

bool IsPlayerPositionBlocked(Vector2 position)
{
    float radius = player.radius * PLAYER_WALL_RADIUS_SCALE;
    int left = (int)floorf((position.x - radius) / TILE_SIZE);
    int right = (int)floorf((position.x + radius) / TILE_SIZE);
    int top = (int)floorf((position.y - radius) / TILE_SIZE);
    int bottom = (int)floorf((position.y + radius) / TILE_SIZE);

    for (int y = top; y <= bottom; y++)
    {
        for (int x = left; x <= right; x++)
        {
            if (CircleOverlapsWallCell(position, radius, x, y))
            {
                return true;
            }
        }
    }

    return false;
}

bool CircleOverlapsTriangle(Vector2 center, float radius, TriangleHitbox triangle)
{
    if (IsPointInTriangle(center, triangle.tip, triangle.right, triangle.left))
    {
        return true;
    }

    return GetPointSegmentDistance(center, triangle.tip, triangle.right) <= radius ||
           GetPointSegmentDistance(center, triangle.right, triangle.left) <= radius ||
           GetPointSegmentDistance(center, triangle.left, triangle.tip) <= radius;
}

bool TryMovePlayerTo(Vector2 position)
{
    if (IsPlayerPositionBlocked(position))
    {
        return false;
    }

    player.position = position;
    return true;
}

void MovePlayerCollisionStep(Vector2 stepDelta)
{
    Vector2 startPosition = player.position;
    Vector2 fullPosition = {
        startPosition.x + stepDelta.x,
        startPosition.y + stepDelta.y
    };

    if (TryMovePlayerTo(fullPosition))
    {
        return;
    }

    bool tryXFirst = fabsf(stepDelta.x) >= fabsf(stepDelta.y);

    for (int attempt = 0; attempt < 2; attempt++)
    {
        bool moveX = (attempt == 0) ? tryXFirst : !tryXFirst;
        Vector2 axisPosition = player.position;

        if (moveX)
        {
            axisPosition.x += stepDelta.x;
        }
        else
        {
            axisPosition.y += stepDelta.y;
        }

        TryMovePlayerTo(axisPosition);
    }
}

void MovePlayerWithCollision(Vector2 direction, float distance)
{
    Vector2 startPosition = player.position;
    Vector2 totalDelta = {
        direction.x * distance,
        direction.y * distance
    };
    float maxDelta = fmaxf(fabsf(totalDelta.x), fabsf(totalDelta.y));
    int steps = (int)ceilf(maxDelta / PLAYER_COLLISION_STEP);

    if (steps < 1)
    {
        steps = 1;
    }

    Vector2 stepDelta = {
        totalDelta.x / (float)steps,
        totalDelta.y / (float)steps
    };

    for (int i = 0; i < steps; i++)
    {
        MovePlayerCollisionStep(stepDelta);
    }

    Vector2 actualDelta = {
        player.position.x - startPosition.x,
        player.position.y - startPosition.y
    };
    float actualDistance = sqrtf((actualDelta.x * actualDelta.x) + (actualDelta.y * actualDelta.y));

    if (actualDistance > distance && actualDistance > 0.0001f)
    {
        float clampScale = distance / actualDistance;
        Vector2 clampedPosition = {
            startPosition.x + actualDelta.x * clampScale,
            startPosition.y + actualDelta.y * clampScale
        };

        TryMovePlayerTo(clampedPosition);
    }
}

void InitPlayer(void)
{
    player.radius = 8.0f;
    player.speed = GetPlayerSpeedForLevel();
    player.facingAngle = 0.0f;
    player.position.x = TILE_SIZE * 1.5f;
    player.position.y = TILE_SIZE * 1.5f;
    playerStartAuraVisible = true;
}

void UpdatePlayer(void)
{
    Vector2 movement = { 0 };

    if (IsKeyPressed(KEY_TAB))
    {
        if (IsTacticalMapAvailable())
        {
            if (tacticalMapOpen)
            {
                tacticalMapOpen = false;
            }
            else if (mapBattery > 0.0f)
            {
                mapBattery -= MAP_TOGGLE_COST;
                if (mapBattery < 0.0f)
                {
                    mapBattery = 0.0f;
                }
                tacticalMapOpen = true;
            }
            else
            {
                SpawnFloatingNotice(player.position, "MAPA SEM BATERIA", (Color){ 255, 205, 60, 255 });
            }
        }
        else
        {
            tacticalMapOpen = false;
            SpawnFloatingNotice(player.position, "MAPA BLOQUEADO", (Color){ 255, 70, 90, 255 });
        }
    }

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) movement.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) movement.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) movement.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) movement.x += 1.0f;

    if (playerDashCooldown > 0.0f)
    {
        playerDashCooldown -= GetStableFrameTime();
        if (playerDashCooldown < 0.0f) playerDashCooldown = 0.0f;
    }

    if (playerDashTimer > 0.0f)
    {
        playerDashTimer -= GetStableFrameTime();
        if (playerDashTimer < 0.0f) playerDashTimer = 0.0f;
    }

    if (movement.x != 0.0f || movement.y != 0.0f)
    {
        playerStartAuraVisible = false;

        float length = sqrtf(movement.x * movement.x + movement.y * movement.y);
        movement.x /= length;
        movement.y /= length;
        player.facingAngle = atan2f(movement.y, movement.x);

        if ((IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) && playerDashCooldown <= 0.0f && playerDashCharges > 0)
        {
            playerDashTimer = PLAYER_DASH_TIME;
            playerDashCooldown = PLAYER_DASH_COOLDOWN;
            playerDashCharges--;
            playerDamageCooldown = fmaxf(playerDamageCooldown, PLAYER_DASH_DAMAGE_GRACE);
            SpawnFloatingNotice(player.position, "DASH", (Color){ 120, 230, 255, 255 });
            SpawnParticleBurst(player.position, (Color){ 120, 230, 255, 255 }, 18, 90.0f, 3.0f);
            if (gameAudioLoaded)
            {
                PlaySound(dashSound);
            }
        }

        float speedMultiplier = (playerDashTimer > 0.0f) ? PLAYER_DASH_SPEED_MULTIPLIER : 1.0f;
        MovePlayerWithCollision(movement, player.speed * speedMultiplier * GetStableFrameTime());
    }
}

void UpdateTacticalMapBattery(void)
{
    if (!tacticalMapOpen)
    {
        return;
    }

    if (!IsTacticalMapAvailable() || mapBattery <= 0.0f)
    {
        tacticalMapOpen = false;
        return;
    }

    mapBattery -= MAP_DRAIN_AMOUNT * GetStableFrameTime();
    if (mapBattery <= 0.0f)
    {
        mapBattery = 0.0f;
        tacticalMapOpen = false;
        SpawnFloatingNotice(player.position, "MAPA SEM BATERIA", (Color){ 255, 205, 60, 255 });
    }
}

void SpawnFloatingNotice(Vector2 position, const char *text, Color color)
{
    int slot = 0;

    for (int i = 0; i < MAX_FLOATING_NOTICES; i++)
    {
        if (floatingNotices[i].timer <= 0.0f)
        {
            slot = i;
            break;
        }
    }

    floatingNotices[slot].position = position;
    snprintf(floatingNotices[slot].text, sizeof(floatingNotices[slot].text), "%s", text);
    floatingNotices[slot].color = color;
    floatingNotices[slot].timer = FLOATING_NOTICE_TIME;
}

void UpdateFloatingNotices(void)
{
    for (int i = 0; i < MAX_FLOATING_NOTICES; i++)
    {
        if (floatingNotices[i].timer > 0.0f)
        {
            floatingNotices[i].timer -= GetStableFrameTime();
            floatingNotices[i].position.y -= 18.0f * GetStableFrameTime();

            if (floatingNotices[i].timer < 0.0f)
            {
                floatingNotices[i].timer = 0.0f;
            }
        }
    }
}

void SpawnParticleBurst(Vector2 position, Color color, int count, float speed, float radius)
{
    for (int i = 0; i < count; i++)
    {
        int slot = -1;
        for (int j = 0; j < MAX_PARTICLES; j++)
        {
            if (particles[j].timer <= 0.0f)
            {
                slot = j;
                break;
            }
        }

        if (slot < 0)
        {
            return;
        }

        float angle = ((float)GetRandomValue(0, 6283) / 1000.0f);
        float velocity = speed * (0.35f + (float)GetRandomValue(0, 100) / 100.0f);
        particles[slot].position = position;
        particles[slot].velocity = (Vector2){ cosf(angle) * velocity, sinf(angle) * velocity };
        particles[slot].color = color;
        particles[slot].radius = radius * (0.55f + (float)GetRandomValue(0, 80) / 100.0f);
        particles[slot].maxTimer = PARTICLE_TIME * (0.65f + (float)GetRandomValue(0, 80) / 100.0f);
        particles[slot].timer = particles[slot].maxTimer;
    }
}

void UpdateParticles(void)
{
    float dt = GetStableFrameTime();

    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].timer <= 0.0f)
        {
            continue;
        }

        particles[i].timer -= dt;
        particles[i].position.x += particles[i].velocity.x * dt;
        particles[i].position.y += particles[i].velocity.y * dt;
        particles[i].velocity.x *= 0.94f;
        particles[i].velocity.y *= 0.94f;

        if (particles[i].timer < 0.0f)
        {
            particles[i].timer = 0.0f;
        }
    }
}

void OpenLockedExit(void)
{
    hasExitKey = true;
    SpawnFloatingNotice(player.position, "SAIDA ABERTA", MAZE_EXIT_COLOR);

    if (lockedExitCellX >= 0 && lockedExitCellY >= 0 && grid[lockedExitCellY][lockedExitCellX] == CELL_LOCKED_EXIT)
    {
        grid[lockedExitCellY][lockedExitCellX] = CELL_EXIT;
        return;
    }

    for (int y = 0; y < GRID_HEIGTH; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            if (grid[y][x] == CELL_LOCKED_EXIT)
            {
                grid[y][x] = CELL_EXIT;
                lockedExitCellX = x;
                lockedExitCellY = y;
                return;
            }
        }
    }
}

void CollectCurrentCellPickup(void)
{
    int cellX = (int)(player.position.x / TILE_SIZE);
    int cellY = (int)(player.position.y / TILE_SIZE);
    int cell = grid[cellY][cellX];

    if (cell == CELL_COIN)
    {
        roundPickupsCollected++;
        playerCoins += PICKUP_COIN_VALUE;
        SpawnFloatingNotice(GetCellCenter(cellX, cellY), TextFormat("+%d MOEDAS", PICKUP_COIN_VALUE), GOLD);
        grid[cellY][cellX] = CELL_PATH;
    }
    else if (cell == CELL_AMMO_PICKUP)
    {
        roundPickupsCollected++;
        playerTotalAmmo += PICKUP_AMMO_VALUE;
        ClampProgressState();
        SpawnFloatingNotice(GetCellCenter(cellX, cellY), TextFormat("+%d MUNICAO", PICKUP_AMMO_VALUE), HUD_BORDER_COLOR);
        grid[cellY][cellX] = CELL_PATH;
    }
    else if (cell == CELL_BATTERY_PICKUP)
    {
        roundPickupsCollected++;
        flashlightBattery += PICKUP_BATTERY_VALUE;
        mapBattery += PICKUP_BATTERY_VALUE;
        if (flashlightBattery > FLASHLIGHT_MAX_BATTERY)
        {
            flashlightBattery = FLASHLIGHT_MAX_BATTERY;
        }
        if (mapBattery > MAP_MAX_BATTERY)
        {
            mapBattery = MAP_MAX_BATTERY;
        }
        SpawnFloatingNotice(GetCellCenter(cellX, cellY), "+15 BATERIAS", GOLD);
        grid[cellY][cellX] = CELL_PATH;
    }
    else if (cell == CELL_KEY)
    {
        roundPickupsCollected++;
        OpenLockedExit();
        grid[cellY][cellX] = CELL_PATH;
    }
    else if (cell == CELL_HEALTH_PICKUP)
    {
        roundPickupsCollected++;
        int maxHealth = GetCurrentPlayerMaxHealth();
        playerHealth += PICKUP_HEALTH_VALUE;
        if (playerHealth > maxHealth)
        {
            playerHealth = maxHealth;
        }
        SpawnFloatingNotice(GetCellCenter(cellX, cellY), "+15 VIDA", (Color){ 190, 70, 255, 255 });
        grid[cellY][cellX] = CELL_PATH;
    }
    else if (cell == CELL_TRAP)
    {
        if (!IsTrapActiveAtCell(cellX, cellY))
        {
            return;
        }

        if (playerDashTimer <= 0.0f && DamagePlayer(TRAP_DAMAGE))
        {
            SpawnFloatingNotice(GetCellCenter(cellX, cellY), "-18 ARMADILHA", (Color){ 255, 70, 90, 255 });
            SpawnParticleBurst(GetCellCenter(cellX, cellY), (Color){ 255, 70, 90, 255 }, 24, 120.0f, 3.0f);
            if (gameAudioLoaded && playerAlive)
            {
                PlaySound(trapSound);
            }
        }
        else
        {
            SpawnFloatingNotice(GetCellCenter(cellX, cellY), "EVADIU", (Color){ 120, 230, 255, 255 });
            SpawnParticleBurst(GetCellCenter(cellX, cellY), (Color){ 120, 230, 255, 255 }, 18, 90.0f, 2.5f);
        }
        grid[cellY][cellX] = CELL_PATH;
    }
}

void DrainFlashlightBatteryPerSecond(void)
{
    if (!flashlightOn)
    {
        return;
    }

    flashlightBattery -= (FLASHLIGHT_DRAIN_AMOUNT / FLASHLIGHT_DRAIN_INTERVAL) * GetStableFrameTime();

    if (flashlightBattery <= 0.0f)
    {
        flashlightBattery = 0.0f;
        flashlightOn = false;
    }
}

void UpdateFlashlight(void)
{
    if (!currentRoundConfig.flashlightEnabled)
    {
        flashlightOn = false;
        flashlightDrainTimer = 0.0f;
        return;
    }

    if (IsKeyPressed(KEY_C))
    {
        if (flashlightOn)
        {
            flashlightOn = false;
        }
        else if (flashlightBattery > 0.0f)
        {
            flashlightBattery -= FLASHLIGHT_TOGGLE_COST;
            if (flashlightBattery < 0.0f)
            {
                flashlightBattery = 0.0f;
            }
            flashlightOn = true;
            if (gameAudioLoaded)
            {
                PlaySound(flashlightToggleSound);
            }
        }
    }

    DrainFlashlightBatteryPerSecond();
}

bool IsLightningAvailableThisRound(void)
{
    return !inTutorialSequence && officialRound >= 10;
}

void TriggerLightningReveal(void)
{
    lightningRevealTimer = LIGHTNING_REVEAL_TIME;
    SpawnParticleBurst(player.position, (Color){ 120, 230, 255, 255 }, 48, 170.0f, 4.0f);

    if (gameAudioLoaded)
    {
        PlaySound(lightningSound);
    }
}

void UpdateLightningReveal(void)
{
    if (lightningRevealTimer > 0.0f)
    {
        lightningRevealTimer -= GetStableFrameTime();
        if (lightningRevealTimer < 0.0f)
        {
            lightningRevealTimer = 0.0f;
        }
    }

    if (!IsLightningAvailableThisRound())
    {
        lightningAutoTimer = 0.0f;
        return;
    }

    lightningAutoTimer += GetStableFrameTime();
    if (lightningAutoTimer >= LIGHTNING_REVEAL_INTERVAL)
    {
        lightningAutoTimer = 0.0f;
        TriggerLightningReveal();
    }

    if (IsKeyPressed(KEY_F) && lightningCharges > 0)
    {
        lightningCharges--;
        lightningAutoTimer = 0.0f;
        TriggerLightningReveal();
    }
}

void UpdatePlayerShooting(void)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        if (playerAmmo <= 0 || playerTotalAmmo <= 0 || playerReloadTimer > 0.0f)
        {
            return;
        }

        Vector2 shotDirection = { cosf(player.facingAngle), sinf(player.facingAngle) };
        Vector2 shotOrigin = player.position;
        shotOrigin.x += shotDirection.x * (player.radius + 8.0f);
        shotOrigin.y += shotDirection.y * (player.radius + 8.0f);
        SpawnBullet(shotOrigin, shotDirection, PLAYER_BULLET_SPEED, true);
        playerAmmo--;
        playerTotalAmmo--;
        roundShotsFired++;
    }
}

void UpdatePlayerReload(void)
{
    if (playerReloadTimer > 0.0f)
    {
        playerReloadTimer -= GetFrameTime();
        if (playerReloadTimer <= 0.0f)
        {
            playerReloadTimer = 0.0f;
            playerAmmo = (playerTotalAmmo < PLAYER_MAGAZINE_SIZE) ? playerTotalAmmo : PLAYER_MAGAZINE_SIZE;
        }
        return;
    }

    int loadedLimit = (playerTotalAmmo < PLAYER_MAGAZINE_SIZE) ? playerTotalAmmo : PLAYER_MAGAZINE_SIZE;
    if (IsKeyPressed(KEY_R) && playerAmmo < loadedLimit)
    {
        playerReloadTimer = PLAYER_RELOAD_TIME;
        if (gameAudioLoaded)
        {
            PlaySound(playerReloadSound);
        }
    }
}

bool DidPlayerReachExit(void)
{
    if (!playerAlive)
    {
        return false;
    }

    int playerCellX = (int)(player.position.x / TILE_SIZE);
    int playerCellY = (int)(player.position.y / TILE_SIZE);

    return grid[playerCellY][playerCellX] == CELL_EXIT;
}

void FinishRoundRewards(void)
{
    pendingRoundBonus = 0;
    pendingEvolutionXp = 0;

    if (!inTutorialSequence)
    {
        pendingRoundBonus = SHOP_ROUND_REWARD;
        pendingEvolutionXp = EVOLUTION_ROUND_XP + (officialRound * 10) + (roundPickupsCollected * EVOLUTION_PICKUP_XP);
        playerCoins += pendingRoundBonus;
        AddEvolutionXp(pendingEvolutionXp);
        roundStartSnapshotActive = false;
        if (officialRound + 1 > bestOfficialRound)
        {
            bestOfficialRound = officialRound + 1;
        }
        SaveProgressForRound(officialRound + 1, bestOfficialRound);
    }
}

bool IsCellOccupiedByEnemy(int cellX, int cellY)
{
    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (!redEnemies[i].active)
        {
            continue;
        }

        int otherCellX = (int)(redEnemies[i].position.x / TILE_SIZE);
        int otherCellY = (int)(redEnemies[i].position.y / TILE_SIZE);

        if (otherCellX == cellX && otherCellY == cellY)
        {
            return true;
        }
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (!blueEnemies[i].active)
        {
            continue;
        }

        int otherCellX = (int)(blueEnemies[i].position.x / TILE_SIZE);
        int otherCellY = (int)(blueEnemies[i].position.y / TILE_SIZE);

        if (otherCellX == cellX && otherCellY == cellY)
        {
            return true;
        }
    }

    if (bossEnemy.active)
    {
        int bossCellX = (int)(bossEnemy.position.x / TILE_SIZE);
        int bossCellY = (int)(bossEnemy.position.y / TILE_SIZE);

        if (bossCellX == cellX && bossCellY == cellY)
        {
            return true;
        }
    }

    return false;
}

bool IsSpawnCellAvailable(int cellX, int cellY)
{
    if (grid[cellY][cellX] != CELL_PATH)
    {
        return false;
    }

    if (cellX == 1 && cellY == 1)
    {
        return false;
    }

    return !IsCellOccupiedByEnemy(cellX, cellY);
}

Vector2 GetCellCenter(int cellX, int cellY)
{
    return (Vector2){ ((float)cellX + 0.5f) * TILE_SIZE, ((float)cellY + 0.5f) * TILE_SIZE };
}

Vector2 FindEnemySpawnPositionForDistance(int spawnSlot, int totalSlots, float minDistanceFromPlayer, bool preferFarExitSide)
{
    int pathCellsX[GRID_WIDTH * GRID_HEIGTH];
    int pathCellsY[GRID_WIDTH * GRID_HEIGTH];
    int pathCount = 0;

    for (int y = 1; y < GRID_HEIGTH - 1; y++)
    {
        for (int x = 1; x < GRID_WIDTH - 1; x++)
        {
            if (IsSpawnCellAvailable(x, y))
            {
                Vector2 center = GetCellCenter(x, y);
                float playerDistance = GetDistanceBetweenPoints(center, player.position);

                if (playerDistance >= minDistanceFromPlayer)
                {
                    pathCellsX[pathCount] = x;
                    pathCellsY[pathCount] = y;
                    pathCount++;
                }
            }
        }
    }

    if (pathCount > 0)
    {
        int index = (pathCount * (spawnSlot + 1)) / (totalSlots + 1);
        if (preferFarExitSide)
        {
            index = pathCount - 1 - index;
        }
        if (index >= pathCount) index = pathCount - 1;
        if (index < 0) index = 0;
        Vector2 spawnPosition = { 0 };
        spawnPosition.x = ((float)pathCellsX[index] + 0.5f) * TILE_SIZE;
        spawnPosition.y = ((float)pathCellsY[index] + 0.5f) * TILE_SIZE;
        return spawnPosition;
    }

    return (Vector2){ TILE_SIZE * 1.5f, TILE_SIZE * 1.5f };
}

Vector2 FindEnemySpawnPosition(int spawnSlot, int totalSlots)
{
    return FindEnemySpawnPositionForDistance(spawnSlot, totalSlots, TILE_SIZE * 3.0f, false);
}

Vector2 FindMandatoryBlueSpawnNearExit(void)
{
    int exitX = GRID_WIDTH - 2;
    int exitY = 1;
    Vector2 bestPosition = FindEnemySpawnPositionForDistance(0, 1, TILE_SIZE * 5.0f, true);
    float bestDistance = 1000000.0f;

    for (int y = 1; y < GRID_HEIGTH - 1; y++)
    {
        for (int x = 1; x < GRID_WIDTH - 1; x++)
        {
            if (!IsSpawnCellAvailable(x, y))
            {
                continue;
            }

            Vector2 center = GetCellCenter(x, y);
            if (GetDistanceBetweenPoints(center, player.position) < TILE_SIZE * 7.0f)
            {
                continue;
            }

            float exitDistance = (float)(abs(exitX - x) + abs(exitY - y));
            if (exitDistance < bestDistance)
            {
                bestDistance = exitDistance;
                bestPosition = center;
            }
        }
    }

    return bestPosition;
}

Vector2 FindTutorialBlueSpawnNearPlayer(void)
{
    int exitX = GRID_WIDTH - 2;
    int exitY = 1;
    Vector2 bestPosition = FindMandatoryBlueSpawnNearExit();
    float bestDistance = 1000000.0f;
    int playerCellX = (int)(player.position.x / TILE_SIZE);
    int playerCellY = (int)(player.position.y / TILE_SIZE);

    for (int y = 1; y < GRID_HEIGTH - 1; y++)
    {
        for (int x = 1; x < GRID_WIDTH - 1; x++)
        {
            if (!IsSpawnCellAvailable(x, y))
            {
                continue;
            }

            Vector2 center = GetCellCenter(x, y);
            float playerDistance = GetDistanceBetweenPoints(center, player.position);
            if (playerDistance < TILE_SIZE * 7.0f || playerDistance > TILE_SIZE * 13.0f)
            {
                continue;
            }

            float exitDistance = (float)(abs(exitX - x) + abs(exitY - y));
            float score = exitDistance * TILE_SIZE + playerDistance * 0.2f;

            if (x == playerCellX || y == playerCellY)
            {
                score -= TILE_SIZE * 2.0f;
            }

            if (score < bestDistance)
            {
                bestDistance = score;
                bestPosition = center;
            }
        }
    }

    return bestPosition;
}

void InitBullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].active = false;
    }
}

void SpawnBulletInternal(Vector2 position, Vector2 direction, float speed, bool fromPlayer, bool fromBoss)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].active = true;
            bullets[i].position = position;
            bullets[i].direction = direction;
            bullets[i].speed = speed;
            bullets[i].radius = BULLET_RADIUS;
            bullets[i].fromPlayer = fromPlayer;
            bullets[i].fromBoss = fromBoss;
            if (gameAudioLoaded)
            {
                PlaySound(fromPlayer ? playerShotSound : enemyShotSound);
            }
            break;
        }
    }
}

void SpawnBullet(Vector2 position, Vector2 direction, float speed, bool fromPlayer)
{
    SpawnBulletInternal(position, direction, speed, fromPlayer, false);
}

void SpawnBossBullet(Vector2 position, Vector2 direction)
{
    SpawnBulletInternal(position, direction, BOSS_BULLET_SPEED, false, true);
}

void InitEnemies(void)
{
    int spawnSlot = 0;
    int totalSlots = RED_ENEMY_COUNT + BLUE_ENEMY_COUNT;
    float redMinSpawnDistance = (inTutorialSequence && tutorialRound >= 3) ? TILE_SIZE * 10.0f : TILE_SIZE * 7.0f;
    float blueMinSpawnDistance = (inTutorialSequence && tutorialRound >= 3) ? TILE_SIZE * 8.0f : TILE_SIZE * 5.0f;

    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        redEnemies[i].radius = 8.0f;
        redEnemies[i].speed = 34.0f + (float)(i * 4) + GetEnemySpeedBonus();
        redEnemies[i].active = true;
        redEnemies[i].type = ENEMY_RED;
        redEnemies[i].hitsTaken = 0;
        redEnemies[i].knockoutTimer = 0.0f;
        redEnemies[i].shootPauseTimer = 0.0f;
        redEnemies[i].position = FindEnemySpawnPositionForDistance(spawnSlot, totalSlots, redMinSpawnDistance, true);
        redEnemies[i].direction = (Vector2){ -1.0f, 0.0f };
        spawnSlot++;
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        blueEnemies[i].radius = 9.0f;
        blueEnemies[i].speed = 42.0f + (float)(i * 4) + GetEnemySpeedBonus();
        blueEnemies[i].active = true;
        blueEnemies[i].type = ENEMY_BLUE;
        blueEnemies[i].hitsTaken = 0;
        blueEnemies[i].knockoutTimer = 0.0f;
        blueEnemies[i].shootPauseTimer = 0.0f;
        blueEnemies[i].position = FindEnemySpawnPositionForDistance(spawnSlot, totalSlots, blueMinSpawnDistance, true);
        blueEnemies[i].direction = (Vector2){ 1.0f, 0.0f };
        spawnSlot++;
    }

    if (inTutorialSequence && tutorialRound == 2)
    {
        blueEnemies[0].position = FindTutorialBlueSpawnNearPlayer();
        blueEnemies[0].speed = 48.0f;
        blueEnemies[0].direction = (player.position.x >= blueEnemies[0].position.x) ? (Vector2){ 1.0f, 0.0f } : (Vector2){ -1.0f, 0.0f };
    }
    else if (inTutorialSequence && tutorialRound == 3)
    {
        blueEnemies[0].position = FindMandatoryBlueSpawnNearExit();
    }
}

void InitBossEnemy(void)
{
    bossEnemy.radius = 10.0f;
    bossEnemy.speed = fmaxf(1.0f, player.speed - 13.0f) + GetBossSpeedBonus();
    bossEnemy.active = true;
    bossEnemy.type = ENEMY_BOSS;
    bossEnemy.hitsTaken = 0;
    bossEnemy.knockoutTimer = 0.0f;
    bossEnemy.shootPauseTimer = 0.0f;
    bossEnemy.position = FindEnemySpawnPosition(0, 1);
    bossEnemy.direction = (Vector2){ -1.0f, 0.0f };
}

RoundConfig GetTutorialRoundConfig(int round)
{
    if (round == 1) return (RoundConfig){ true, false, false, false };
    if (round == 2) return (RoundConfig){ true, true, false, false };
    if (round == 3) return (RoundConfig){ true, true, false, true };
    return (RoundConfig){ false, false, true, false };
}

RoundConfig GetOfficialRoundConfig(int round)
{
    if (round >= 10) return (RoundConfig){ true, true, true, true };
    if (round >= 8) return (RoundConfig){ true, true, true, false };
    if (round >= 6) return (RoundConfig){ true, false, true, false };
    if (round == 5) return (RoundConfig){ false, false, true, false };
    if (round >= 3 && round <= 4)
    {
        return (RoundConfig){ true, true, false, true };
    }

    return (RoundConfig){ true, true, false, false };
}

void ApplyRoundConfig(void)
{
    currentRoundConfig = inTutorialSequence ? GetTutorialRoundConfig(tutorialRound) : GetOfficialRoundConfig(officialRound);

    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        redEnemies[i].active = currentRoundConfig.redEnabled;
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        blueEnemies[i].active = currentRoundConfig.blueEnabled;
    }

    if (inTutorialSequence && tutorialRound >= 2 && tutorialRound <= 3)
    {
        blueEnemies[0].active = true;
        for (int i = 1; i < BLUE_ENEMY_COUNT; i++)
        {
            blueEnemies[i].active = false;
        }
    }

    bossEnemy.active = currentRoundConfig.bossEnabled;
    flashlightOn = false;
    flashlightDrainTimer = 0.0f;
    playerReloadTimer = 0.0f;

    if (inTutorialSequence)
    {
        playerTotalAmmo = PLAYER_START_AMMO;
        playerAmmo = PLAYER_MAGAZINE_SIZE;
        flashlightBattery = FLASHLIGHT_MAX_BATTERY;
        mapBattery = MAP_MAX_BATTERY;
    }

    if (!inTutorialSequence && (officialRound == 1 || officialRound % 5 == 0))
    {
        flashlightBattery = FLASHLIGHT_MAX_BATTERY;
        mapBattery = MAP_MAX_BATTERY;
    }
}

void SetupRound(void)
{
    currentModifiers = inTutorialSequence ? (RoundModifiers){ 0 } : GetRoundModifiers(officialRound);
    playerAlive = true;
    playerHealth = inTutorialSequence ? PLAYER_MAX_HEALTH : playerMaxHealth;
    playerAmmo = (playerTotalAmmo < PLAYER_MAGAZINE_SIZE) ? playerTotalAmmo : PLAYER_MAGAZINE_SIZE;
    hasExitKey = false;
    lockedExitCellX = -1;
    lockedExitCellY = -1;
    roundPickupsCollected = 0;
    roundShotsFired = 0;
    roundTookDamage = false;
    pendingRoundBonus = 0;
    pendingEvolutionXp = 0;
    victoryRoundsOpen = false;
    for (int i = 0; i < MAX_FLOATING_NOTICES; i++)
    {
        floatingNotices[i].timer = 0.0f;
    }
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        particles[i].timer = 0.0f;
    }
    playerDamageCooldown = 0.0f;
    playerDashTimer = 0.0f;
    playerDashCooldown = 0.0f;
    playerDashCharges = GetRoundDashMaxCharges();
    tacticalMapOpen = false;
    waitingForVictorySound = false;
    lightningAutoTimer = 0.0f;
    lightningRevealTimer = 0.0f;
    GenerateMaze();
    InitPlayer();
    InitBullets();
    InitEnemies();
    InitBossEnemy();
    ApplyRoundConfig();
    CaptureRoundStartSnapshot();
    PlaceRoundPickups(currentRoundConfig.flashlightEnabled);
    roundNeedsSetup = false;
}

const char *GetIntroTitle(void)
{
    return "BYTE MAZE";
}

const char *GetIntroBody(void)
{
    if (currentLanguage == LANGUAGE_EN)
    {
        return "Goal: guide the yellow triangle to the green exit.\n"
               "Move with WASD or arrow keys.\n"
               "Shoot with SPACE, reload with R, dash with SHIFT, and open the map with TAB.\n"
               "Enemies remove health, shots can knock them out for 5 seconds.\n"
               "Each tutorial step introduces one part of the game.";
    }
    if (currentLanguage == LANGUAGE_ES)
    {
        return "Objetivo: lleva el triangulo amarillo hasta la salida verde.\n"
               "Muevete con WASD o las flechas.\n"
               "Dispara con ESPACIO, recarga con R, dash con SHIFT y mapa con TAB.\n"
               "Los enemigos quitan vida; los disparos pueden dejarlos K.O. por 5 segundos.\n"
               "Cada tutorial presenta una parte del juego.";
    }
    if (currentLanguage == LANGUAGE_KO)
    {
        return "목표: 노란 삼각형을 초록 출구까지 이동하세요.\n"
               "WASD 또는 방향키로 움직입니다.\n"
               "스페이스로 발사, R로 재장전, SHIFT와 TAB도 사용합니다.\n"
               "적에게 닿으면 체력이 감소하고, 총알은 적을 5초 동안 기절시킵니다.\n"
               "튜토리얼은 게임의 규칙을 단계별로 알려줍니다.";
    }
    return "Objetivo: leve o triangulo amarelo ate a saida verde.\n"
           "Mova com WASD ou setas.\n"
           "Atire com ESPACO, recarregue com R, use SHIFT para dash e TAB para mapa.\n"
           "Inimigos tiram vida; tiros podem nocautea-los por 5 segundos.\n"
           "Cada tutorial apresenta uma parte do jogo.";
}

const char *GetRoundInfoTitle(void)
{
    if (inTutorialSequence)
    {
        if (currentLanguage == LANGUAGE_KO) return TextFormat("튜토리얼 %d", tutorialRound);
        return TextFormat("TUTORIAL %d", tutorialRound);
    }

    if (currentLanguage == LANGUAGE_KO) return TextFormat("라운드 %d", officialRound);
    if (currentLanguage == LANGUAGE_ES) return TextFormat("RONDA %d", officialRound);
    return TextFormat("ROUND %d", officialRound);
}

const char *GetRoundInfoBody(void)
{
    if (inTutorialSequence)
    {
        if (currentLanguage == LANGUAGE_EN)
        {
            if (tutorialRound == 1) return "Movement, dash and map.\nWASD/arrows move. SHIFT dashes in your facing direction, with limited charges per round.\nTAB opens the tactical map while its battery lasts; close it to save charge.\nReach the green exit tile to advance.";
            if (tutorialRound == 2) return "Shooting and reload.\nSPACE fires toward the triangle tip. Each shot spends one bullet from the 15-shot magazine.\nR reloads when you have ammo left, up to 100 total. Five hits knock any enemy out for 5 seconds.\nPink enemies shoot only when aligned in a clear row or column.";
            if (tutorialRound == 3) return "Light and resources.\nC toggles the flashlight; it reveals farther but drains its own battery.\nCoins buy upgrades, ammo boxes give 10 bullets, health boxes heal, and battery boxes refill tools.\nResources are kept only when the round is won.";
            return "Boss, key and modifiers.\nThe purple square boss chases, shoots when aligned, and speeds up when far away.\nFrom locked rounds, find the golden key before stepping on the green exit.\nLater rounds add traps, low visibility, map jams and dash limits.";
        }
        if (currentLanguage == LANGUAGE_KO)
        {
            if (tutorialRound == 1) return "이동과 추가 조작.\nWASD 또는 방향키로 움직이고 TAB을 사용합니다.\nSHIFT는 라운드마다 3번만 사용할 수 있습니다.\n빨간 적은 가까우면 추격합니다. 초록 출구로 가세요.";
            if (tutorialRound == 2) return "분홍 적은 총을 쏘는 적입니다.\n같은 행이나 열에서 벽 없이 마주치면 멈추고 발사합니다.\n스페이스로 쏘세요. 적은 5번 맞으면 5초 동안 기절합니다.\n탄창은 15발이고 최대 탄약은 100입니다.";
            if (tutorialRound == 3) return "손전등을 사용할 수 있습니다.\nC로 켜고 끕니다. 더 넓게 보이지만 배터리를 소모합니다.\n튜토리얼 라운드는 항상 배터리 100퍼센트로 시작합니다.\n모퉁이와 출구를 확인할 때 짧게 사용하세요.";
            return "보라색 사각형은 보스입니다.\n플레이어를 찾아오고, 일직선이면 쏘며, 멀어지면 빨라집니다.\n보스도 5번 맞으면 5초 동안 기절합니다.\n게임은 두 명 넘는 적이 동시에 플레이어를 막지 않게 조정합니다.";
        }
        if (currentLanguage == LANGUAGE_ES)
        {
            if (tutorialRound == 1) return "Movimiento, dash y mapa.\nWASD/flechas mueven. SHIFT hace dash en la direccion actual, con cargas limitadas por ronda.\nTAB abre el mapa tactico mientras tenga bateria; cierralo para ahorrar carga.\nPisa la salida verde para avanzar.";
            if (tutorialRound == 2) return "Disparo y recarga.\nESPACIO dispara hacia la punta del triangulo. Cada disparo gasta una bala del cargador de 15.\nR recarga si tienes municion, hasta 100 balas totales. Cinco impactos dejan K.O. a cualquier enemigo por 5 segundos.\nLos rosas disparan solo si te ven en fila o columna sin pared.";
            if (tutorialRound == 3) return "Luz y recursos.\nC enciende o apaga la linterna; revela mas, pero consume su propia bateria.\nMonedas compran mejoras, cajas de municion dan 10 balas, vida cura y baterias recargan herramientas.\nLos recursos se guardan solo si ganas la ronda.";
            return "Jefe, llave y modificadores.\nEl jefe morado persigue, dispara alineado y acelera cuando esta lejos.\nEn rondas cerradas, encuentra la llave dorada antes de pisar la salida verde.\nLuego aparecen trampas, baja vision, mapa bloqueado y menos dash.";
        }

        if (tutorialRound == 1) return "Movimento, dash e mapa.\nWASD/setas movem. SHIFT da dash na direcao atual, com cargas limitadas por round.\nTAB abre o mapa tatico enquanto a bateria dele durar; feche para economizar carga.\nPise no bloco verde para avancar.";
        if (tutorialRound == 2) return "Tiro e recarga.\nESPACO atira para onde o triangulo aponta. Cada tiro gasta 1 bala do pente de 15.\nR recarrega quando existe municao, ate 100 municoes totais. Cinco acertos nocauteiam qualquer inimigo por 5 segundos.\nRosas so atiram quando enxergam voce em linha ou coluna sem parede.";
        if (tutorialRound == 3) return "Luz e recursos.\nC liga/desliga a lanterna; ela revela mais longe, mas gasta a propria bateria.\nMoedas compram melhorias, caixas de municao dao 10 balas, vida cura e baterias recarregam ferramentas.\nRecursos so ficam se voce vencer o round.";
        return "Chefao, chave e modificadores.\nO quadrado roxo persegue, atira quando fica alinhado e acelera quando esta longe.\nEm rounds trancados, pegue a chave dourada antes de pisar no bloco verde.\nDepois entram armadilhas, pouca visao, mapa bloqueado e menos dash.";
    }

    if (currentLanguage == LANGUAGE_EN)
    {
        if (officialRound == 1) return "Official run starts now.\nRounds 1 and 2 use red patrol enemies and pink shooters.\nYou start with 50 health, 15 loaded shots and 30 total ammo, capped at 100.\nReach the green exit to advance.";
        if (officialRound == 3) return "Flashlight rounds begin.\nRounds 3 and 4 include red and pink enemies plus limited visibility.\nYour battery does not refill every round, so spend it carefully.\nIt refills on round 5 and then every 5 rounds.";
        if (officialRound == 4) return "Security doors are now active.\nFind the golden key before the exit opens.\nCoins, ammo, health and battery cells can appear in side paths.";
        if (officialRound == 5) return "Round 5 refills tool batteries to 100 percent.\nThis round focuses on the purple boss only.\nKeep moving, break alignment when it shoots, and hit it five times to knock it out.";
        if (officialRound == 6) return "Rounds 6 and 7 combine the boss with red enemies.\nThe boss hunts directly while red enemies pressure nearby routes.\nUse shots to create space and avoid being boxed in.";
        if (officialRound == 7) return "Side paths can now hold stronger rewards.\nCoins, ammo, health and battery are only kept when the round is won.\nIf you die, resources return to the pre-round state.";
        if (officialRound == 8) return "Rounds 8 and 9 add every enemy type except flashlight darkness.\nRed enemies chase, pink enemies shoot, and the boss accelerates from far away.\nKeep moving and break straight lines before shots.";
        if (officialRound == 9) return "Bomb traps are active.\nThey blink: 3 seconds armed, 3 seconds safe.\nCross while they are off or dash through with SHIFT.\nDash has only 3 charges per round, so do not waste it.";
        if (officialRound == 10) return "Lightning is now active.\nEvery 15 seconds the whole maze flashes for 3 seconds.\nThe shop can sell extra manual charges for F.\nUse lightning when the key, exit or boss route is unclear.";
        if (officialRound == 15) return "The maze keeps changing from here.\nExit placement, enemy speed and pathfinding continue scaling every round.\nKnockouts, reload timing, and battery control matter more now.";
        if (officialRound == 16) return "Round modifiers keep rotating.\nSome rounds reduce visibility, jam TAB map, add traps, reduce dash charges or pressure boss timing.\nThe status panel shows the active modifier list.\nTreat each modifier as a small challenge mode.";
        return "Round briefing.\nWASD/arrows move, SPACE shoots, R reloads, and SHIFT dashes with limited charges.\nTAB opens the map while its battery lasts; C uses flashlight when unlocked; F spends lightning charges.\nFind the key if the exit is locked, then step on the green tile to advance.";
    }
    if (currentLanguage == LANGUAGE_ES)
    {
        if (officialRound == 1) return "La partida oficial empieza ahora.\nLas rondas 1 y 2 usan enemigos rojos de patrulla y rosas que disparan.\nEmpiezas con 50 de vida, 15 balas cargadas y 30 totales, con limite de 100.\nLlega a la salida verde para avanzar.";
        if (officialRound == 3) return "Empiezan las rondas con linterna.\nLas rondas 3 y 4 tienen rojos, rosas y vision limitada.\nLa bateria no se recarga cada ronda; usala con cuidado.\nSe recarga en la ronda 5 y luego cada 5 rondas.";
        if (officialRound == 4) return "Las puertas de seguridad estan activas.\nEncuentra la llave dorada antes de abrir la salida.\nHay monedas, municion, vida y bateria en rutas laterales.";
        if (officialRound == 5) return "La ronda 5 recarga las baterias de herramientas al 100 por ciento.\nEsta ronda se centra solo en el jefe morado.\nSigue moviendote, rompe la alineacion cuando dispare y acierta cinco veces para dejarlo K.O.";
        if (officialRound == 6) return "Las rondas 6 y 7 combinan al jefe con enemigos rojos.\nEl jefe te persigue directo mientras los rojos presionan las rutas cercanas.\nUsa disparos para abrir espacio y evita quedar acorralado.";
        if (officialRound == 7) return "Las rutas laterales pueden tener mejores recompensas.\nMonedas, municion, vida y bateria solo se guardan si ganas.\nSi mueres, todo vuelve al estado previo a la ronda.";
        if (officialRound == 8) return "Las rondas 8 y 9 suman todos los tipos de enemigo menos la oscuridad de la linterna.\nLos rojos persiguen, los rosas disparan y el jefe acelera cuando esta lejos.\nSigue moviendote y rompe lineas rectas antes de los disparos.";
        if (officialRound == 9) return "Las bombas trampa estan activas.\nParpadean: 3 segundos armadas, 3 segundos seguras.\nCruza cuando esten apagadas o usa SHIFT.\nEl dash tiene solo 3 cargas por ronda.";
        if (officialRound == 10) return "El rayo ya esta disponible.\nCada 15 segundos todo el laberinto aparece por 3 segundos.\nLa tienda vende cargas manuales para F.\nUsalo cuando la llave, la salida o la ruta del jefe no esten claras.";
        if (officialRound == 15) return "El laberinto sigue cambiando desde aqui.\nLa salida, velocidad enemiga y calculo de caminos continuan escalando cada ronda.\nEl K.O., la recarga y la bateria importan mas ahora.";
        if (officialRound == 16) return "Los modificadores siguen rotando.\nAlgunas rondas reducen vision, bloquean TAB, suman trampas, bajan dash o presionan al jefe.\nEl panel de estado muestra la lista activa.\nCada modificador funciona como un pequeno modo desafio.";
        return "Resumen de ronda.\nWASD/flechas mueven, ESPACIO dispara, R recarga y SHIFT hace dash con cargas limitadas.\nTAB abre el mapa mientras tenga bateria; C usa linterna cuando este disponible; F gasta cargas de rayo.\nBusca la llave si la salida esta cerrada y pisa el bloque verde para avanzar.";
    }
    if (currentLanguage == LANGUAGE_KO)
    {
        if (officialRound == 1) return "공식 게임이 시작됩니다.\n라운드 1과 2에는 빨간 순찰 적과 분홍 사격 적이 나옵니다.\n체력 50, 장전 15발, 총 탄약 30발로 시작하며 최대는 100입니다.\n초록 출구에 도착하면 다음 라운드로 갑니다.";
        if (officialRound == 3) return "손전등 라운드가 시작됩니다.\n라운드 3과 4에는 빨간 적, 분홍 적, 제한된 시야가 있습니다.\n배터리는 매 라운드 충전되지 않으니 아껴 쓰세요.\n라운드 5부터 5라운드마다 충전됩니다.";
        if (officialRound == 5) return "라운드 5에서는 손전등 배터리가 100퍼센트로 충전됩니다.\n이번 라운드는 보라색 보스 하나에 집중합니다.\n계속 움직이고, 보스가 쏠 때 일직선을 피하고, 5번 맞혀 기절시키세요.";
        if (officialRound == 6) return "라운드 6과 7은 보스와 빨간 적이 함께 나옵니다.\n보스는 직접 추적하고 빨간 적은 주변 길을 압박합니다.\n총으로 공간을 만들고 막히지 않게 움직이세요.";
        if (officialRound == 8) return "라운드 8과 9에는 손전등 어둠을 제외한 모든 적이 나옵니다.\n빨간 적은 추격하고, 분홍 적은 쏘고, 보스는 멀면 빨라집니다.\n음악이 커지면 가까운 위험이 있다는 뜻입니다.";
        if (officialRound == 9) return "위험 타일이 활성화됩니다.\n빨간 표식은 한 번 피해를 주고 사라집니다.\nSHIFT로 지나가면 피해를 피할 수 있습니다.\nSHIFT는 라운드마다 3번만 사용할 수 있습니다.";
        if (officialRound == 10) return "라운드 10부터는 보스, 손전등, LIGHTNING이 모두 활성화됩니다.\n15초마다 3초 동안 미로가 보입니다.\nSHOP: F 키 LIGHTNING +1.\nKEY, 출구, 보스 경로가 안 보일 때 사용하세요.";
        if (officialRound == 15) return "이제 난이도 상승이 시작됩니다.\n라운드가 올라갈수록 적이 더 빠르고 더 자주 길을 계산합니다.\n기절, 재장전 타이밍, 배터리 관리가 중요합니다.";
        if (officialRound == 16) return "추가 규칙이 시작됩니다.\n일부 라운드는 시야, TAB, 함정, SHIFT, 보스를 바꿉니다.\n상태 패널에서 이 규칙을 확인하세요.\n각 규칙은 작은 도전 모드입니다.";
        return "미로를 지나 초록 출구에 도착하세요.\n스페이스로 발사, R로 재장전, 가능할 때 C로 손전등을 켭니다.\n계속 움직이고 기절 시간을 이용해 길을 여세요.";
    }

    if (officialRound == 1) return "A partida oficial comeca agora.\nWASD/setas movem; ESPACO atira; R recarrega; SHIFT da dash; TAB abre o mapa com bateria propria.\nVoce inicia com 50 de vida, 15 no pente e 30 municoes totais, com limite de 100.\nPise no bloco verde para avancar.";
    if (officialRound == 3) return "Comecam os rounds com lanterna.\nC liga/desliga a lanterna; ela revela mais longe, mas consome a propria bateria.\nTAB tambem tem bateria separada, entao use o mapa em consultas curtas.\nAs baterias voltam no round 5 e depois a cada 5 rounds.";
    if (officialRound == 4) return "Portas de seguranca foram ativadas.\nAche a chave dourada antes de abrir a saida.\nMoedas, municao, vida e bateria podem aparecer em rotas laterais.";
    if (officialRound == 5) return "No round 5 as baterias das ferramentas voltam para 100 por cento.\nEste round foca apenas no chefao roxo.\nContinue se movendo, quebre o alinhamento quando ele atirar e acerte cinco tiros para nocautea-lo.";
    if (officialRound == 6) return "Rounds 6 e 7 juntam chefao e inimigos vermelhos.\nO chefao persegue direto, enquanto os vermelhos pressionam rotas proximas.\nUse tiros para abrir espaco e evitar ficar preso.";
    if (officialRound == 8) return "Rounds 8 e 9 trazem todos os tipos de inimigo, mas sem escuridao da lanterna.\nVermelhos perseguem, rosas atiram e o chefao acelera quando esta longe.\nContinue se movendo e quebre linhas retas antes dos tiros.";
    if (officialRound == 9) return "Bombas-armadilha foram ativadas.\nElas piscam: 3 segundos armadas, 3 segundos seguras.\nPasse quando estiverem apagadas ou use SHIFT para atravessar.\nO dash tem apenas 3 cargas por round.";
    if (officialRound == 10) return "Raio foi liberado.\nA cada 15 segundos o mapa inteiro aparece por 3 segundos.\nA loja vende cargas extras para usar com F.\nUse quando chave, saida ou rota do chefao estiverem incertas.";
    if (officialRound == 15) return "O labirinto segue mudando daqui em diante.\nPosicao da saida, velocidade inimiga e caminho inteligente continuam escalando a cada round.\nNocaute, recarga e controle da bateria importam mais agora.";
    if (officialRound == 16) return "Modificadores de round seguem rotacionando.\nAlguns rounds reduzem visao, bloqueiam TAB, adicionam armadilhas, reduzem dash ou pressionam o chefao.\nO painel de status mostra a lista ativa.\nCada modificador funciona como um pequeno modo desafio.";
    if (officialRound == 7) return "Rotas laterais podem ter recompensas melhores.\nMoedas, municao, vida e bateria so ficam se voce vencer o round.\nSe morrer, tudo volta para antes do round.";
    return "Round em andamento.\nWASD/setas movem, ESPACO atira, R recarrega e SHIFT da dash com cargas limitadas.\nTAB abre o mapa enquanto a bateria dele durar; C usa lanterna quando liberada; F aciona raio quando houver carga.\nPegue chave se a saida estiver trancada e pise no bloco verde para avancar.";
}

bool ShouldShowRoundInfo(void)
{
    if (inTutorialSequence)
    {
        return true;
    }

    return true;
}

void StartCurrentStage(void)
{
    waitingForVictorySound = false;
    roundNeedsSetup = true;
    gamePhase = (inTutorialSequence || officialRound == 1) ? PHASE_INFO : PHASE_SHOP;
}

void ResetOfficialRunStats(void)
{
    playerCoins = SHOP_START_COINS;
    playerLevel = 1;
    playerXp = 0;
    playerMaxHealth = PLAYER_START_HEALTH;
    playerHealth = playerMaxHealth;
    playerMaxAmmo = PLAYER_MAX_AMMO;
    playerTotalAmmo = PLAYER_START_AMMO;
    playerAmmo = PLAYER_MAGAZINE_SIZE;
    flashlightBattery = FLASHLIGHT_MAX_BATTERY;
    mapBattery = MAP_MAX_BATTERY;
    lightningCharges = 0;
}

void AdvanceToNextStage(void)
{
    if (inTutorialSequence)
    {
        tutorialRound++;

        if (tutorialRound <= 4)
        {
            StartCurrentStage();
            return;
        }

        inTutorialSequence = false;
        officialRound = 1;
        bestOfficialRound = 1;
        ResetOfficialRunStats();
        SaveProgress();
        StartCurrentStage();
        return;
    }

    officialRound++;
    if (officialRound > bestOfficialRound)
    {
        bestOfficialRound = officialRound;
    }
    SaveProgress();
    StartCurrentStage();
}

const char *GetStageCompleteLabel(void)
{
    if (inTutorialSequence)
    {
        return "TUTORIAL";
    }

    switch (currentLanguage)
    {
        case LANGUAGE_ES: return "RONDA";
        case LANGUAGE_KO: return "라운드";
        default: return "ROUND";
    }
}

int CollectEnemyDirections(Enemy *enemy, Vector2 options[4], bool allowReverse)
{
    Vector2 oppositeDirection = { -enemy->direction.x, -enemy->direction.y };
    float testDistance = TILE_SIZE * 0.5f;
    int validCount = 0;
    Vector2 directions[4] = {
        { 1.0f, 0.0f },
        { -1.0f, 0.0f },
        { 0.0f, 1.0f },
        { 0.0f, -1.0f }
    };

    for (int i = 0; i < 4; i++)
    {
        Vector2 testPosition = enemy->position;
        testPosition.x += directions[i].x * testDistance;
        testPosition.y += directions[i].y * testDistance;

        if (!IsPositionBlocked(testPosition, enemy->radius))
        {
            if (!allowReverse && directions[i].x == oppositeDirection.x && directions[i].y == oppositeDirection.y)
            {
                continue;
            }

            options[validCount] = directions[i];
            validCount++;
        }
    }

    return validCount;
}

bool IsEnemyNearCellCenter(Enemy *enemy)
{
    int cellX = (int)(enemy->position.x / TILE_SIZE);
    int cellY = (int)(enemy->position.y / TILE_SIZE);
    float centerX = ((float)cellX + 0.5f) * TILE_SIZE;
    float centerY = ((float)cellY + 0.5f) * TILE_SIZE;
    float tolerance = 2.0f;

    return fabsf(enemy->position.x - centerX) <= tolerance && fabsf(enemy->position.y - centerY) <= tolerance;
}

bool FindBossPathDirection(Vector2 *direction)
{
    int startX = (int)(bossEnemy.position.x / TILE_SIZE);
    int startY = (int)(bossEnemy.position.y / TILE_SIZE);
    int goalX = (int)(player.position.x / TILE_SIZE);
    int goalY = (int)(player.position.y / TILE_SIZE);
    bool visited[GRID_HEIGTH][GRID_WIDTH] = { false };
    int parentX[GRID_HEIGTH][GRID_WIDTH] = { 0 };
    int parentY[GRID_HEIGTH][GRID_WIDTH] = { 0 };
    int queueX[GRID_WIDTH * GRID_HEIGTH];
    int queueY[GRID_WIDTH * GRID_HEIGTH];
    int head = 0;
    int tail = 0;
    int directionsX[4] = { 1, -1, 0, 0 };
    int directionsY[4] = { 0, 0, 1, -1 };

    visited[startY][startX] = true;
    parentX[startY][startX] = startX;
    parentY[startY][startX] = startY;
    queueX[tail] = startX;
    queueY[tail] = startY;
    tail++;

    while (head < tail)
    {
        int currentX = queueX[head];
        int currentY = queueY[head];
        head++;

        if (currentX == goalX && currentY == goalY)
        {
            break;
        }

        for (int i = 0; i < 4; i++)
        {
            int nextX = currentX + directionsX[i];
            int nextY = currentY + directionsY[i];

            if (nextX < 0 || nextX >= GRID_WIDTH || nextY < 0 || nextY >= GRID_HEIGTH)
            {
                continue;
            }

            if (visited[nextY][nextX] || grid[nextY][nextX] == CELL_WALL)
            {
                continue;
            }

            visited[nextY][nextX] = true;
            parentX[nextY][nextX] = currentX;
            parentY[nextY][nextX] = currentY;
            queueX[tail] = nextX;
            queueY[tail] = nextY;
            tail++;
        }
    }

    if (!visited[goalY][goalX])
    {
        return false;
    }

    int stepX = goalX;
    int stepY = goalY;

    while (!(parentX[stepY][stepX] == startX && parentY[stepY][stepX] == startY))
    {
        int previousX = parentX[stepY][stepX];
        int previousY = parentY[stepY][stepX];

        if (previousX == stepX && previousY == stepY)
        {
            break;
        }

        stepX = previousX;
        stepY = previousY;
    }

    direction->x = (float)(stepX - startX);
    direction->y = (float)(stepY - startY);
    return true;
}

float GetDirectionPlayerDistance(Enemy *enemy, Vector2 direction)
{
    Vector2 testPosition = enemy->position;
    testPosition.x += direction.x * TILE_SIZE;
    testPosition.y += direction.y * TILE_SIZE;
    return GetDistanceBetweenPoints(testPosition, player.position);
}

bool IsEnemyCountingForCrowd(Enemy *enemy, Enemy *excludedEnemy)
{
    return enemy != excludedEnemy && enemy->active && enemy->knockoutTimer <= 0.0f;
}

int CountEnemiesNearPlayer(Enemy *excludedEnemy)
{
    int nearCount = 0;

    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (IsEnemyCountingForCrowd(&redEnemies[i], excludedEnemy) &&
            GetDistanceBetweenPoints(redEnemies[i].position, player.position) <= PLAYER_CROWD_RADIUS)
        {
            nearCount++;
        }
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (IsEnemyCountingForCrowd(&blueEnemies[i], excludedEnemy) &&
            GetDistanceBetweenPoints(blueEnemies[i].position, player.position) <= PLAYER_CROWD_RADIUS)
        {
            nearCount++;
        }
    }

    if (IsEnemyCountingForCrowd(&bossEnemy, excludedEnemy) &&
        GetDistanceBetweenPoints(bossEnemy.position, player.position) <= PLAYER_CROWD_RADIUS)
    {
        nearCount++;
    }

    return nearCount;
}

bool WouldEnemyCrowdPlayer(Enemy *enemy, Vector2 position)
{
    if (GetDistanceBetweenPoints(position, player.position) > PLAYER_CROWD_RADIUS)
    {
        return false;
    }

    return CountEnemiesNearPlayer(enemy) >= PLAYER_MAX_NEAR_ENEMIES;
}

bool FindCrowdSafeDirection(Enemy *enemy, Vector2 validDirections[4], int validCount, Vector2 *direction)
{
    if (CountEnemiesNearPlayer(enemy) < PLAYER_MAX_NEAR_ENEMIES)
    {
        return false;
    }

    float bestDistance = -1.0f;
    bool foundSafeDirection = false;

    for (int i = 0; i < validCount; i++)
    {
        Vector2 testPosition = enemy->position;
        testPosition.x += validDirections[i].x * TILE_SIZE;
        testPosition.y += validDirections[i].y * TILE_SIZE;

        if (WouldEnemyCrowdPlayer(enemy, testPosition))
        {
            continue;
        }

        float distance = GetDistanceBetweenPoints(testPosition, player.position);
        if (!foundSafeDirection || distance > bestDistance)
        {
            bestDistance = distance;
            *direction = validDirections[i];
            foundSafeDirection = true;
        }
    }

    return foundSafeDirection;
}

bool FindBestPathDirectionForEnemy(Enemy *enemy, Vector2 *direction)
{
    int startX = (int)(enemy->position.x / TILE_SIZE);
    int startY = (int)(enemy->position.y / TILE_SIZE);
    int goalX = (int)(player.position.x / TILE_SIZE);
    int goalY = (int)(player.position.y / TILE_SIZE);
    bool visited[GRID_HEIGTH][GRID_WIDTH] = { false };
    int parentX[GRID_HEIGTH][GRID_WIDTH] = { 0 };
    int parentY[GRID_HEIGTH][GRID_WIDTH] = { 0 };
    int queueX[GRID_WIDTH * GRID_HEIGTH];
    int queueY[GRID_WIDTH * GRID_HEIGTH];
    int head = 0;
    int tail = 0;
    int directionsX[4] = { 1, -1, 0, 0 };
    int directionsY[4] = { 0, 0, 1, -1 };

    visited[startY][startX] = true;
    parentX[startY][startX] = startX;
    parentY[startY][startX] = startY;
    queueX[tail] = startX;
    queueY[tail] = startY;
    tail++;

    while (head < tail)
    {
        int currentX = queueX[head];
        int currentY = queueY[head];
        head++;

        if (currentX == goalX && currentY == goalY)
        {
            break;
        }

        for (int i = 0; i < 4; i++)
        {
            int nextX = currentX + directionsX[i];
            int nextY = currentY + directionsY[i];

            if (nextX < 0 || nextX >= GRID_WIDTH || nextY < 0 || nextY >= GRID_HEIGTH)
            {
                continue;
            }

            if (visited[nextY][nextX] || grid[nextY][nextX] == CELL_WALL)
            {
                continue;
            }

            visited[nextY][nextX] = true;
            parentX[nextY][nextX] = currentX;
            parentY[nextY][nextX] = currentY;
            queueX[tail] = nextX;
            queueY[tail] = nextY;
            tail++;
        }
    }

    if (!visited[goalY][goalX])
    {
        return false;
    }

    int stepX = goalX;
    int stepY = goalY;

    while (!(parentX[stepY][stepX] == startX && parentY[stepY][stepX] == startY))
    {
        int previousX = parentX[stepY][stepX];
        int previousY = parentY[stepY][stepX];

        if (previousX == stepX && previousY == stepY)
        {
            break;
        }

        stepX = previousX;
        stepY = previousY;
    }

    direction->x = (float)(stepX - startX);
    direction->y = (float)(stepY - startY);
    return true;
}

void ChooseRedEnemyDirection(Enemy *enemy, bool allowReverse)
{
    Vector2 validDirections[4];
    int validCount = CollectEnemyDirections(enemy, validDirections, allowReverse);

    if (validCount == 0)
    {
        if (!allowReverse)
        {
            enemy->direction = (Vector2){ -enemy->direction.x, -enemy->direction.y };
        }
        return;
    }

    if (FindCrowdSafeDirection(enemy, validDirections, validCount, &enemy->direction))
    {
        return;
    }

    if (GetDistanceBetweenPoints(enemy->position, player.position) <= GetRedEnemyTrackDistance())
    {
        Vector2 pathDirection = { 0 };

        if (FindBestPathDirectionForEnemy(enemy, &pathDirection))
        {
            for (int i = 0; i < validCount; i++)
            {
                if (validDirections[i].x == pathDirection.x && validDirections[i].y == pathDirection.y)
                {
                    enemy->direction = pathDirection;
                    return;
                }
            }
        }
    }

    enemy->direction = validDirections[GetRandomValue(0, validCount - 1)];
}

void ChooseBlueEnemyDirection(Enemy *enemy, bool allowReverse)
{
    Vector2 validDirections[4];
    int validCount = CollectEnemyDirections(enemy, validDirections, allowReverse);
    Vector2 pathDirection = { 0 };

    if (validCount == 0)
    {
        if (!allowReverse)
        {
            enemy->direction = (Vector2){ -enemy->direction.x, -enemy->direction.y };
        }
        return;
    }

    if (FindCrowdSafeDirection(enemy, validDirections, validCount, &enemy->direction))
    {
        return;
    }

    if (GetRandomValue(0, 1000) <= (int)(GetBlueEnemyPathfindChance() * 1000.0f) &&
        FindBestPathDirectionForEnemy(enemy, &pathDirection))
    {
        for (int i = 0; i < validCount; i++)
        {
            if (validDirections[i].x == pathDirection.x && validDirections[i].y == pathDirection.y)
            {
                enemy->direction = pathDirection;
                return;
            }
        }
    }

    float bestDistance = 1000000.0f;
    int bestIndices[4] = { 0 };
    int bestCount = 0;

    for (int i = 0; i < validCount; i++)
    {
        float distance = GetDirectionPlayerDistance(enemy, validDirections[i]);

        if (distance < bestDistance - 0.1f)
        {
            bestDistance = distance;
            bestIndices[0] = i;
            bestCount = 1;
        }
        else if (fabsf(distance - bestDistance) <= 0.1f)
        {
            bestIndices[bestCount] = i;
            bestCount++;
        }
    }

    enemy->direction = validDirections[bestIndices[GetRandomValue(0, bestCount - 1)]];
}

bool IsEnemyAlignedWithPlayer(Enemy *enemy, Vector2 *shotDirection)
{
    int enemyCellX = (int)(enemy->position.x / TILE_SIZE);
    int enemyCellY = (int)(enemy->position.y / TILE_SIZE);
    int playerCellX = (int)(player.position.x / TILE_SIZE);
    int playerCellY = (int)(player.position.y / TILE_SIZE);

    if (enemyCellX == playerCellX)
    {
        int step = (playerCellY > enemyCellY) ? 1 : -1;
        for (int y = enemyCellY + step; y != playerCellY; y += step)
        {
            if (grid[y][enemyCellX] == CELL_WALL)
            {
                return false;
            }
        }

        *shotDirection = (Vector2){ 0.0f, (playerCellY > enemyCellY) ? 1.0f : -1.0f };
        return true;
    }

    if (enemyCellY == playerCellY)
    {
        int step = (playerCellX > enemyCellX) ? 1 : -1;
        for (int x = enemyCellX + step; x != playerCellX; x += step)
        {
            if (grid[enemyCellY][x] == CELL_WALL)
            {
                return false;
            }
        }

        *shotDirection = (Vector2){ (playerCellX > enemyCellX) ? 1.0f : -1.0f, 0.0f };
        return true;
    }

    return false;
}

void UpdateRedEnemy(Enemy *enemy)
{
    if (!enemy->active || !playerAlive)
    {
        return;
    }

    if (enemy->knockoutTimer > 0.0f)
    {
        enemy->knockoutTimer -= GetFrameTime();
        if (enemy->knockoutTimer <= 0.0f)
        {
            enemy->knockoutTimer = 0.0f;
            enemy->hitsTaken = 0;
        }
        return;
    }

    float frameSpeed = enemy->speed * GetFrameTime();
    Vector2 nextPosition = enemy->position;

    nextPosition.x += enemy->direction.x * frameSpeed;
    nextPosition.y += enemy->direction.y * frameSpeed;

    if (IsPositionBlocked(nextPosition, enemy->radius))
    {
        ChooseRedEnemyDirection(enemy, true);
        return;
    }

    if (WouldEnemyCrowdPlayer(enemy, nextPosition))
    {
        ChooseRedEnemyDirection(enemy, true);
        return;
    }

    enemy->position = nextPosition;

    if (IsEnemyNearCellCenter(enemy))
    {
        Vector2 validDirections[4];
        int validCount = CollectEnemyDirections(enemy, validDirections, false);

        if (validCount > 1)
        {
            ChooseRedEnemyDirection(enemy, false);
        }
    }
}

void UpdateBlueEnemy(Enemy *enemy)
{
    if (!enemy->active || !playerAlive)
    {
        return;
    }

    if (enemy->knockoutTimer > 0.0f)
    {
        enemy->knockoutTimer -= GetFrameTime();
        if (enemy->knockoutTimer <= 0.0f)
        {
            enemy->knockoutTimer = 0.0f;
            enemy->hitsTaken = 0;
        }
        return;
    }

    if (enemy->shootPauseTimer > 0.0f)
    {
        enemy->shootPauseTimer -= GetFrameTime();

        if (enemy->shootPauseTimer <= 0.0f)
        {
            Vector2 shotDirection = { 0 };
            if (IsEnemyAlignedWithPlayer(enemy, &shotDirection))
            {
                Vector2 shotOrigin = enemy->position;
                shotOrigin.x += shotDirection.x * (enemy->radius + 6.0f);
                shotOrigin.y += shotDirection.y * (enemy->radius + 6.0f);
                SpawnBullet(shotOrigin, shotDirection, BLUE_BULLET_SPEED, false);
            }
        }
        return;
    }

    Vector2 shotDirection = { 0 };
    if (IsEnemyNearCellCenter(enemy) && IsEnemyAlignedWithPlayer(enemy, &shotDirection))
    {
        enemy->direction = shotDirection;
        enemy->shootPauseTimer = 0.45f;
        return;
    }

    float frameSpeed = enemy->speed * GetFrameTime();
    Vector2 nextPosition = enemy->position;
    nextPosition.x += enemy->direction.x * frameSpeed;
    nextPosition.y += enemy->direction.y * frameSpeed;

    if (IsPositionBlocked(nextPosition, enemy->radius))
    {
        ChooseBlueEnemyDirection(enemy, true);
        return;
    }

    if (WouldEnemyCrowdPlayer(enemy, nextPosition))
    {
        ChooseBlueEnemyDirection(enemy, true);
        return;
    }

    enemy->position = nextPosition;

    if (IsEnemyNearCellCenter(enemy))
    {
        ChooseBlueEnemyDirection(enemy, false);
    }
}

void UpdateBossEnemy(void)
{
    if (!bossEnemy.active || !playerAlive)
    {
        return;
    }

    if (bossEnemy.knockoutTimer > 0.0f)
    {
        bossEnemy.knockoutTimer -= GetFrameTime();

        if (bossEnemy.knockoutTimer <= 0.0f)
        {
            bossEnemy.knockoutTimer = 0.0f;
            bossEnemy.hitsTaken = 0;
        }
        return;
    }

    if (IsEnemyNearCellCenter(&bossEnemy))
    {
        Vector2 pathDirection = { 0 };

        if (FindBossPathDirection(&pathDirection))
        {
            bossEnemy.direction = pathDirection;
        }
        else
        {
            ChooseBlueEnemyDirection(&bossEnemy, false);
        }
    }

    bossEnemy.shootPauseTimer -= GetFrameTime();

    if (bossEnemy.shootPauseTimer <= 0.0f)
    {
        Vector2 shotDirection = { 0 };

        if (IsEnemyAlignedWithPlayer(&bossEnemy, &shotDirection))
        {
            int bossCellX = (int)(bossEnemy.position.x / TILE_SIZE);
            int bossCellY = (int)(bossEnemy.position.y / TILE_SIZE);
            Vector2 shotOrigin = GetCellCenter(bossCellX, bossCellY);
            shotOrigin.x += shotDirection.x * (bossEnemy.radius + BULLET_RADIUS + 3.0f);
            shotOrigin.y += shotDirection.y * (bossEnemy.radius + BULLET_RADIUS + 3.0f);
            if (gameAudioLoaded)
            {
                PlaySound(bossAlertSound);
            }
            SpawnBossBullet(shotOrigin, shotDirection);
            bossEnemy.shootPauseTimer = GetBossShootCooldown();
        }
    }

    float frameSpeed = bossEnemy.speed * GetFrameTime();
    if (GetDistanceBetweenPoints(bossEnemy.position, player.position) >= BOSS_FAR_DISTANCE_THRESHOLD)
    {
        frameSpeed *= GetBossFarSpeedMultiplier();
    }

    Vector2 nextPosition = bossEnemy.position;
    nextPosition.x += bossEnemy.direction.x * frameSpeed;
    nextPosition.y += bossEnemy.direction.y * frameSpeed;

    if (IsPositionBlocked(nextPosition, bossEnemy.radius))
    {
        ChooseBlueEnemyDirection(&bossEnemy, true);
        return;
    }

    if (WouldEnemyCrowdPlayer(&bossEnemy, nextPosition))
    {
        ChooseBlueEnemyDirection(&bossEnemy, true);
        return;
    }

    bossEnemy.position = nextPosition;
}

bool IsEnemyDangerous(Enemy *enemy)
{
    if (!enemy->active)
    {
        return false;
    }

    if (enemy->knockoutTimer > 0.0f)
    {
        return false;
    }

    return true;
}

Sound CreateSynthSound(int soundType)
{
    float duration = 0.14f;
    if (soundType == 2) duration = 0.28f;
    if (soundType == 4) duration = 0.72f;
    if (soundType == 5) duration = 0.8f;
    if (soundType == 6) duration = 0.18f;
    if (soundType == 7) duration = 0.12f;
    if (soundType == 8) duration = 0.55f;
    if (soundType == 9) duration = 0.20f;
    if (soundType == 10) duration = 0.32f;
    if (soundType == 11) duration = 0.16f;

    int sampleCount = (int)((float)AUDIO_SAMPLE_RATE * duration);
    int16_t *samples = (int16_t *)malloc((size_t)sampleCount * sizeof(int16_t));

    if (samples == NULL)
    {
        static int16_t silence = 0;
        Wave emptyWave = { 1, AUDIO_SAMPLE_RATE, 16, 1, &silence };
        return LoadSoundFromWave(emptyWave);
    }

    for (int i = 0; i < sampleCount; i++)
    {
        float t = (float)i / (float)AUDIO_SAMPLE_RATE;
        float p = (float)i / (float)sampleCount;
        float envelope = fminf(1.0f, p * 18.0f) * (1.0f - p);
        float value = 0.0f;

        if (soundType == 0)
        {
            float freq = 980.0f - (520.0f * p);
            value = sinf(t * freq * 6.2831853f) * envelope;
        }
        else if (soundType == 1)
        {
            float freq = 420.0f + (180.0f * sinf(p * 18.0f));
            value = sinf(t * freq * 6.2831853f) * envelope;
        }
        else if (soundType == 2)
        {
            float step = (p < 0.33f) ? 520.0f : ((p < 0.66f) ? 700.0f : 880.0f);
            value = sinf(t * step * 6.2831853f) * envelope;
        }
        else if (soundType == 4)
        {
            float note = (p < 0.34f) ? 523.25f : ((p < 0.67f) ? 659.25f : 783.99f);
            float softEnvelope = fminf(1.0f, p * 10.0f) * (1.0f - p) * (1.0f - p);
            float shimmer = sinf(t * note * 6.2831853f) + (0.18f * sinf(t * note * 2.0f * 6.2831853f));
            value = shimmer * softEnvelope;
        }
        else if (soundType == 5)
        {
            float fall = 190.0f - 125.0f * p;
            float alarm = sinf(t * fall * 6.2831853f) + 0.28f * sinf(t * 47.0f * 6.2831853f);
            float heavyEnvelope = fminf(1.0f, p * 8.0f) * (1.0f - p) * (1.0f - p);
            value = alarm * heavyEnvelope;
        }
        else if (soundType == 6)
        {
            float freq = 150.0f - (70.0f * p);
            float thump = sinf(t * freq * 6.2831853f);
            value = thump * envelope;
        }
        else if (soundType == 7)
        {
            float freq = 820.0f + (320.0f * p);
            float clickEnvelope = fminf(1.0f, p * 28.0f) * (1.0f - p) * (1.0f - p);
            value = sinf(t * freq * 6.2831853f) * clickEnvelope;
        }
        else if (soundType == 8)
        {
            float crack = sinf(t * (1100.0f - 650.0f * p) * 6.2831853f);
            float rumble = 0.45f * sinf(t * (95.0f - 35.0f * p) * 6.2831853f);
            float snapEnvelope = fminf(1.0f, p * 38.0f) * (1.0f - p);
            value = (crack + rumble) * snapEnvelope;
        }
        else if (soundType == 9)
        {
            float freq = 360.0f + 820.0f * p;
            float sweep = sinf(t * freq * 6.2831853f);
            value = sweep * envelope;
        }
        else if (soundType == 10)
        {
            float buzz = sinf(t * 75.0f * 6.2831853f) + 0.55f * sinf(t * 145.0f * 6.2831853f);
            value = buzz * envelope;
        }
        else if (soundType == 11)
        {
            float beep = sinf(t * 1240.0f * 6.2831853f);
            float gate = (fmodf(p * 6.0f, 1.0f) < 0.5f) ? 1.0f : 0.25f;
            value = beep * envelope * gate;
        }
        else
        {
            float freq = 220.0f - (120.0f * p);
            value = sinf(t * freq * 6.2831853f) * envelope;
        }

        samples[i] = (int16_t)(fmaxf(-1.0f, fminf(1.0f, value * 0.82f)) * 32767.0f);
    }

    Wave wave = {
        (unsigned int)sampleCount,
        AUDIO_SAMPLE_RATE,
        16,
        1,
        samples
    };
    Sound sound = LoadSoundFromWave(wave);
    free(samples);
    return sound;
}

void InitGameAudio(void)
{
#if defined(__linux__)
    if (!DirectoryExists("/dev/snd"))
    {
        gameAudioLoaded = false;
        return;
    }
#endif

    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        gameAudioLoaded = false;
        return;
    }

    SetMasterVolume(1.0f);

    playerShotSound = CreateSynthSound(0);
    enemyShotSound = CreateSynthSound(1);
    playerReloadSound = CreateSynthSound(2);
    playerHitSound = CreateSynthSound(6);
    flashlightToggleSound = CreateSynthSound(7);
    lightningSound = CreateSynthSound(8);
    dashSound = CreateSynthSound(9);
    trapSound = CreateSynthSound(10);
    bossAlertSound = CreateSynthSound(11);
    victorySound = CreateSynthSound(4);
    gameOverSound = CreateSynthSound(5);

    SetSoundVolume(playerShotSound, PLAYER_SHOT_VOLUME);
    SetSoundVolume(enemyShotSound, ENEMY_SHOT_VOLUME);
    SetSoundVolume(playerReloadSound, PLAYER_RELOAD_VOLUME);
    SetSoundVolume(playerHitSound, 0.9f);
    SetSoundVolume(flashlightToggleSound, 0.72f);
    SetSoundVolume(lightningSound, 0.9f);
    SetSoundVolume(dashSound, 0.78f);
    SetSoundVolume(trapSound, 0.78f);
    SetSoundVolume(bossAlertSound, 0.6f);
    SetSoundVolume(victorySound, VICTORY_VOLUME);
    SetSoundVolume(gameOverSound, GAME_OVER_VOLUME);

    gameAudioLoaded = true;
}

void UpdateGameAudio(void)
{
    if (!gameAudioLoaded)
    {
        return;
    }
}

void ShutdownGameAudio(void)
{
    if (!gameAudioLoaded)
    {
        return;
    }

    UnloadSound(playerShotSound);
    UnloadSound(enemyShotSound);
    UnloadSound(playerReloadSound);
    UnloadSound(playerHitSound);
    UnloadSound(flashlightToggleSound);
    UnloadSound(lightningSound);
    UnloadSound(dashSound);
    UnloadSound(trapSound);
    UnloadSound(bossAlertSound);
    UnloadSound(victorySound);
    UnloadSound(gameOverSound);
    CloseAudioDevice();
    gameAudioLoaded = false;
}

bool IsEnemyTouchingPlayer(Enemy *enemy)
{
    if (!IsEnemyDangerous(enemy))
    {
        return false;
    }

    return CircleOverlapsTriangle(enemy->position, enemy->radius, GetPlayerTriangleHitbox(player.position));
}

bool DamagePlayer(int damage)
{
    if (!playerAlive || playerDamageCooldown > 0.0f)
    {
        return false;
    }

    playerHealth -= damage;
    roundTookDamage = true;
    playerDamageCooldown = PLAYER_DAMAGE_COOLDOWN;

    if (playerHealth <= 0)
    {
        playerHealth = 0;
        playerAlive = false;
        RestoreRoundStartSnapshot();
        playerHealth = 0;
        if (gameAudioLoaded)
        {
            PlaySound(gameOverSound);
        }
    }

    return true;
}

void PushEnemyAwayFromPlayer(Enemy *enemy)
{
    Vector2 away = {
        enemy->position.x - player.position.x,
        enemy->position.y - player.position.y
    };
    float length = sqrtf((away.x * away.x) + (away.y * away.y));

    if (length <= 0.0001f)
    {
        away.x = -enemy->direction.x;
        away.y = -enemy->direction.y;
        length = sqrtf((away.x * away.x) + (away.y * away.y));
    }

    if (length <= 0.0001f)
    {
        away = (Vector2){ 1.0f, 0.0f };
        length = 1.0f;
    }

    away.x /= length;
    away.y /= length;

    for (float pushDistance = TILE_SIZE * 1.25f; pushDistance >= TILE_SIZE * 0.25f; pushDistance -= 3.0f)
    {
        Vector2 pushedPosition = {
            enemy->position.x + away.x * pushDistance,
            enemy->position.y + away.y * pushDistance
        };

        if (!IsPositionBlocked(pushedPosition, enemy->radius))
        {
            enemy->position = pushedPosition;
            enemy->direction = away;
            return;
        }
    }

    enemy->direction = away;
}

void ApplyEnemyTouchDamage(void)
{
    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (IsEnemyTouchingPlayer(&redEnemies[i]))
        {
            if (DamagePlayer(ENEMY_TOUCH_DAMAGE))
            {
                PushEnemyAwayFromPlayer(&redEnemies[i]);
                if (gameAudioLoaded && playerAlive)
                {
                    PlaySound(playerHitSound);
                }
            }
            return;
        }
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (IsEnemyTouchingPlayer(&blueEnemies[i]))
        {
            if (DamagePlayer(BLUE_BULLET_DAMAGE))
            {
                PushEnemyAwayFromPlayer(&blueEnemies[i]);
                if (gameAudioLoaded && playerAlive)
                {
                    PlaySound(playerHitSound);
                }
            }
            return;
        }
    }

    if (IsEnemyTouchingPlayer(&bossEnemy))
    {
        if (DamagePlayer(ENEMY_TOUCH_DAMAGE))
        {
            PushEnemyAwayFromPlayer(&bossEnemy);
            bossEnemy.shootPauseTimer = fmaxf(bossEnemy.shootPauseTimer, 0.35f);
            SpawnFloatingNotice(player.position, "CHEFAO EMPURRADO", (Color){ 255, 70, 170, 255 });
            SpawnParticleBurst(bossEnemy.position, (Color){ 255, 70, 170, 255 }, 24, 115.0f, 3.0f);
            if (gameAudioLoaded && playerAlive)
            {
                PlaySound(playerHitSound);
            }
        }
    }
}

void UpdateBullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            continue;
        }

        bullets[i].position.x += bullets[i].direction.x * bullets[i].speed * GetFrameTime();
        bullets[i].position.y += bullets[i].direction.y * bullets[i].speed * GetFrameTime();

        if (IsPositionBlocked(bullets[i].position, bullets[i].radius))
        {
            SpawnParticleBurst(bullets[i].position, bullets[i].fromPlayer ? GOLD : SKYBLUE, 8, 58.0f, 2.2f);
            bullets[i].active = false;
            continue;
        }

        if (bullets[i].fromPlayer)
        {
            for (int enemyIndex = 0; enemyIndex < RED_ENEMY_COUNT; enemyIndex++)
            {
                float dx = redEnemies[enemyIndex].position.x - bullets[i].position.x;
                float dy = redEnemies[enemyIndex].position.y - bullets[i].position.y;
                float distance = sqrtf((dx * dx) + (dy * dy));

                if (redEnemies[enemyIndex].active && redEnemies[enemyIndex].knockoutTimer <= 0.0f &&
                    distance <= redEnemies[enemyIndex].radius + bullets[i].radius)
                {
                    redEnemies[enemyIndex].hitsTaken++;
                    SpawnParticleBurst(bullets[i].position, RED, 10, 70.0f, 2.5f);
                    bullets[i].active = false;

                    if (redEnemies[enemyIndex].hitsTaken >= RED_HIT_LIMIT)
                    {
                        redEnemies[enemyIndex].hitsTaken = 0;
                        redEnemies[enemyIndex].knockoutTimer = ENEMY_KNOCKOUT_TIME;
                        redEnemies[enemyIndex].shootPauseTimer = 0.0f;
                    }
                    break;
                }
            }

            if (!bullets[i].active)
            {
                continue;
            }

            for (int enemyIndex = 0; enemyIndex < BLUE_ENEMY_COUNT; enemyIndex++)
            {
                float dx = blueEnemies[enemyIndex].position.x - bullets[i].position.x;
                float dy = blueEnemies[enemyIndex].position.y - bullets[i].position.y;
                float distance = sqrtf((dx * dx) + (dy * dy));

                if (blueEnemies[enemyIndex].active && blueEnemies[enemyIndex].knockoutTimer <= 0.0f &&
                    distance <= blueEnemies[enemyIndex].radius + bullets[i].radius)
                {
                    blueEnemies[enemyIndex].hitsTaken++;
                    SpawnParticleBurst(bullets[i].position, (Color){ 255, 45, 165, 255 }, 10, 70.0f, 2.5f);
                    bullets[i].active = false;

                    if (blueEnemies[enemyIndex].hitsTaken >= BLUE_HIT_LIMIT)
                    {
                        blueEnemies[enemyIndex].hitsTaken = 0;
                        blueEnemies[enemyIndex].knockoutTimer = ENEMY_KNOCKOUT_TIME;
                        blueEnemies[enemyIndex].shootPauseTimer = 0.0f;
                    }
                    break;
                }
            }

            if (!bullets[i].active)
            {
                continue;
            }

            float dx = bossEnemy.position.x - bullets[i].position.x;
            float dy = bossEnemy.position.y - bullets[i].position.y;
            float distance = sqrtf((dx * dx) + (dy * dy));

            if (bossEnemy.active && bossEnemy.knockoutTimer <= 0.0f && distance <= bossEnemy.radius + bullets[i].radius)
            {
                bossEnemy.hitsTaken++;
                SpawnParticleBurst(bullets[i].position, VIOLET, 14, 90.0f, 3.0f);
                bullets[i].active = false;

                if (bossEnemy.hitsTaken >= BOSS_HIT_LIMIT)
                {
                    bossEnemy.hitsTaken = 0;
                    bossEnemy.knockoutTimer = ENEMY_KNOCKOUT_TIME;
                    bossEnemy.shootPauseTimer = 0.0f;
                }
            }
        }
        else
        {
            if (CircleOverlapsTriangle(bullets[i].position, bullets[i].radius, GetPlayerTriangleHitbox(player.position)))
            {
                SpawnParticleBurst(player.position, (Color){ 255, 70, 90, 255 }, 18, 90.0f, 3.0f);
                bullets[i].active = false;
                DamagePlayer(BLUE_BULLET_DAMAGE);
            }
        }
    }
}

void ResetGame(void)
{
    if (inTutorialSequence)
    {
        gamePhase = PHASE_INFO;
    }
    else
    {
        gamePhase = ShouldShowRoundInfo() ? PHASE_INFO : PHASE_PLAYING;
    }

    roundNeedsSetup = true;
}

/* Small hex/circuit glyph drawn with plain primitives (no image assets,
 * so it costs nothing in binary size) to echo the reference dashboard's
 * corner icon. */
void DrawHudIcon(Vector2 center, float radius, Color color)
{
    DrawPolyLines(center, 6, radius, 0.0f, color);
    DrawPolyLines(center, 6, radius * 0.62f, 0.0f, Fade(color, 0.7f));
    DrawCircleV(center, radius * 0.16f, color);
}

/* Per-panel glyphs, each drawn with plain primitives (no image assets),
 * echoing the distinct icon-per-panel look of the reference dashboard:
 * a little monitor, a document, a heartbeat pulse, a crosshair and a
 * keyboard key, instead of reusing the same hex glyph everywhere. */
void DrawMonitorIcon(Vector2 center, float radius, Color color)
{
    Rectangle screen = { center.x - radius, center.y - radius * 0.75f, radius * 2.0f, radius * 1.35f };
    DrawRectangleRoundedLinesEx(screen, 0.2f, 6, fmaxf(1.0f, radius * 0.16f), color);
    DrawLineEx((Vector2){ center.x, center.y + radius * 0.6f }, (Vector2){ center.x, center.y + radius * 0.95f }, fmaxf(1.0f, radius * 0.16f), color);
    DrawLineEx((Vector2){ center.x - radius * 0.5f, center.y + radius * 0.95f }, (Vector2){ center.x + radius * 0.5f, center.y + radius * 0.95f }, fmaxf(1.0f, radius * 0.16f), color);
}

void DrawDocumentIcon(Vector2 center, float radius, Color color)
{
    Rectangle page = { center.x - radius * 0.65f, center.y - radius, radius * 1.3f, radius * 2.0f };
    DrawRectangleRoundedLinesEx(page, 0.2f, 6, fmaxf(1.0f, radius * 0.14f), color);
    for (int i = 0; i < 3; i++)
    {
        float lineY = center.y - radius * 0.35f + (float)i * radius * 0.5f;
        DrawLineEx((Vector2){ center.x - radius * 0.35f, lineY }, (Vector2){ center.x + radius * 0.35f, lineY }, fmaxf(1.0f, radius * 0.12f), Fade(color, 0.8f));
    }
}

void DrawHeartbeatIcon(Vector2 center, float radius, Color color)
{
    Vector2 points[6] = {
        { center.x - radius, center.y },
        { center.x - radius * 0.4f, center.y },
        { center.x - radius * 0.15f, center.y - radius * 0.8f },
        { center.x + radius * 0.1f, center.y + radius * 0.8f },
        { center.x + radius * 0.35f, center.y },
        { center.x + radius, center.y }
    };
    for (int i = 0; i < 5; i++)
    {
        DrawLineEx(points[i], points[i + 1], fmaxf(1.0f, radius * 0.18f), color);
    }
}

void DrawCrosshairIcon(Vector2 center, float radius, Color color)
{
    DrawCircleLinesV(center, radius, color);
    DrawCircleLinesV(center, radius * 0.5f, Fade(color, 0.8f));
    DrawLineEx((Vector2){ center.x - radius * 1.3f, center.y }, (Vector2){ center.x - radius * 0.65f, center.y }, fmaxf(1.0f, radius * 0.16f), color);
    DrawLineEx((Vector2){ center.x + radius * 0.65f, center.y }, (Vector2){ center.x + radius * 1.3f, center.y }, fmaxf(1.0f, radius * 0.16f), color);
    DrawLineEx((Vector2){ center.x, center.y - radius * 1.3f }, (Vector2){ center.x, center.y - radius * 0.65f }, fmaxf(1.0f, radius * 0.16f), color);
    DrawLineEx((Vector2){ center.x, center.y + radius * 0.65f }, (Vector2){ center.x, center.y + radius * 1.3f }, fmaxf(1.0f, radius * 0.16f), color);
}

void DrawKeyboardIcon(Vector2 center, float radius, Color color)
{
    Rectangle body = { center.x - radius, center.y - radius * 0.65f, radius * 2.0f, radius * 1.3f };
    DrawRectangleRoundedLinesEx(body, 0.25f, 6, fmaxf(1.0f, radius * 0.16f), color);
    for (int row = 0; row < 2; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            float kx = body.x + radius * 0.35f + (float)col * radius * 0.7f;
            float ky = body.y + radius * 0.35f + (float)row * radius * 0.65f;
            DrawCircleV((Vector2){ kx, ky }, fmaxf(0.8f, radius * 0.12f), Fade(color, 0.8f));
        }
    }
}

/* Segmented bar: a row of small rounded blocks instead of one solid fill,
 * matching the reference HUD's "chip" style bars for ammo/health. Pure
 * vector shapes, so it adds no asset weight. */
void DrawSegmentedBar(float x, float y, float width, float height, int totalSegments, int filledSegments, Color filledColor, Color emptyColor, float scale)
{
    if (totalSegments < 1)
    {
        totalSegments = 1;
    }

    float gap = 3.0f * scale;
    float segmentWidth = (width - (gap * (float)(totalSegments - 1))) / (float)totalSegments;

    for (int i = 0; i < totalSegments; i++)
    {
        Rectangle segment = { x + (float)i * (segmentWidth + gap), y, segmentWidth, height };
        Color segmentColor = (i < filledSegments) ? filledColor : emptyColor;
        DrawRectangleRounded(segment, 0.35f, 4, segmentColor);
    }
}

void DrawPlayerHealthBar(float x, float y, float width, float scale)
{
    Color healthColor = (Color){ 80, 230, 255, 255 };
    int healthFontSize = ScaleFontSize(18.0f);
    int currentMaxHealth = GetCurrentPlayerMaxHealth();
    float healthRatio = (currentMaxHealth > 0) ? ((float)playerHealth / (float)currentMaxHealth) : 0.0f;
    int totalSegments = 10;
    int filledSegments = (int)ceilf(healthRatio * (float)totalSegments);

    if (healthRatio <= 0.3f)
    {
        healthColor = (Color){ 255, 70, 90, 255 };
    }
    else if (healthRatio <= 0.6f)
    {
        healthColor = (Color){ 255, 170, 60, 255 };
    }

    DrawTextStrongFit(TextFormat(T(TEXT_HEALTH), playerHealth, currentMaxHealth), (int)x, (int)(y - 25.0f * scale), healthFontSize, 12, 1.0f * scale, width, RAYWHITE, BLACK);
    DrawSegmentedBar(x, y, width, 20.0f * scale, totalSegments, filledSegments, healthColor, (Color){ 30, 34, 52, 255 }, scale);
}
static void DrawTechPanel(Rectangle panel, Color accent)
{
    float scale = GetUIScale();
    DrawRectangleRounded((Rectangle){ panel.x + 4.0f * scale, panel.y + 5.0f * scale, panel.width, panel.height }, 0.1f, 10, Fade(BLACK, 0.55f));
    DrawRectangleRounded(panel, 0.1f, 10, HUD_PANEL_COLOR);
    /* Soft outward glow (wide, faded) plus a crisp inner border, matching
     * the reference dashboard's glowing panel outlines. */
    DrawRectangleRoundedLinesEx((Rectangle){ panel.x - 2.0f, panel.y - 2.0f, panel.width + 4.0f, panel.height + 4.0f }, 0.1f, 10, 4.0f * scale, Fade(accent, 0.22f));
    DrawRectangleRoundedLinesEx(panel, 0.1f, 10, 1.5f * scale, Fade(accent, 0.95f));
    DrawLineEx((Vector2){ panel.x + 12.0f * scale, panel.y + 42.0f * scale },
               (Vector2){ panel.x + panel.width - 12.0f * scale, panel.y + 42.0f * scale },
               1.0f * scale, Fade(accent, 0.28f));
}

static void DrawKeyCap(const char *key, float x, float y, float width, float scale)
{
    Rectangle keyRect = { x, y, width, 29.0f * scale };
    DrawRectangleRounded(keyRect, 0.14f, 5, (Color){ 13, 18, 48, 255 });
    DrawRectangleRoundedLinesEx(keyRect, 0.14f, 5, 1.5f * scale, HUD_ACCENT_COLOR);
    int size = ScaleFontSize(12.0f);
    float textWidth = MeasureTextStrongSpaced(key, size, 1.0f * scale).x;
    DrawTextStrongSpaced(key, (int)(x + (width - textWidth) * 0.5f), (int)(y + 7.0f * scale), size, 0.8f * scale, RAYWHITE, BLACK);
}

static float GetMapExploredPercent(void)
{
    int visibleCells = 0;
    int playableCells = 0;

    for (int y = 0; y < GRID_HEIGTH; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            if (grid[y][x] != CELL_WALL)
            {
                playableCells++;
                Vector2 center = { ((float)x + 0.5f) * TILE_SIZE, ((float)y + 0.5f) * TILE_SIZE };
                if (IsWorldPositionVisible(center))
                {
                    visibleCells++;
                }
            }
        }
    }

    if (playableCells == 0) return 0.0f;
    return ((float)visibleCells / (float)playableCells) * 100.0f;
}

static void DrawDashboardBackground(void)
{
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawRectangleGradientV(0, 0, w, h, (Color){ 3, 7, 28, 255 }, (Color){ 1, 3, 16, 255 });

    float scale = GetUIScale();
    Color circuit = Fade(HUD_BORDER_COLOR, 0.18f);

    for (int i = 0; i < 7; i++)
    {
        float y = (70.0f + i * 92.0f) * scale;
        if (y >= h) break;
        DrawLineEx((Vector2){ 12.0f * scale, y }, (Vector2){ 120.0f * scale, y }, 1.0f, circuit);
        DrawLineEx((Vector2){ (float)w - 120.0f * scale, y }, (Vector2){ (float)w - 12.0f * scale, y }, 1.0f, circuit);
    }

    DrawLineEx((Vector2){ 18.0f * scale, 66.0f * scale }, (Vector2){ (float)w - 18.0f * scale, 66.0f * scale }, 1.0f, Fade(HUD_BORDER_COLOR, 0.35f));
    DrawCircleV((Vector2){ 18.0f * scale, 66.0f * scale }, 2.0f * scale, HUD_BORDER_COLOR);
    DrawCircleV((Vector2){ (float)w - 18.0f * scale, 66.0f * scale }, 2.0f * scale, HUD_ACCENT_COLOR);
}

static void DrawDashboardHeader(void)
{
    float scale = GetUIScale();
    float x = 24.0f * scale;
    float y = 14.0f * scale;

    if (IsCompactLayout())
    {
        float logoWidth = 210.0f * scale;
        float logoX = ((float)GetScreenWidth() - logoWidth) * 0.5f;
        DrawTextStrongFit("BYTEMAZE", (int)logoX, (int)(y + 1.0f * scale),
                          ScaleFontSize(18.0f), 14, 0.6f * scale, logoWidth,
                          (Color){ 225, 240, 255, 255 }, BLACK);
        return;
    }

    DrawTextStrongFit("BYTEMAZE", (int)x, (int)(y + 1.0f * scale),
                      ScaleFontSize(22.0f), 16, 0.7f * scale, 260.0f * scale,
                      (Color){ 225, 240, 255, 255 }, BLACK);
}

/* Small HUD round label + number switches between "TUTORIAL" (while
 * inTutorialSequence) and the localized word for "ROUND", so the big
 * number underneath is never ambiguous about which counter it shows -
 * previously the label always said "ROUND" even during the tutorial,
 * which read as if the official round counter was stuck. */
const char *GetHudRoundLabel(void)
{
    if (inTutorialSequence)
    {
        return "TUTORIAL";
    }

    switch (currentLanguage)
    {
        case LANGUAGE_ES: return "RONDA";
        case LANGUAGE_KO: return "라운드";
        default: return "ROUND";
    }
}

void DrawHud(void)
{
    float scale = GetUIScale();

    /* On narrow/short windows the HUD becomes a compact two-row layout:
     * status + ammo above the maze, controls + health below it. This keeps
     * every panel inside the window and gives the maze the most space. */
    if (IsCompactLayout())
    {
        float margin = 10.0f * scale;
        float gap = 8.0f * scale;
        float availableWidth = (float)GetScreenWidth() - margin * 2.0f;
        float halfWidth = (availableWidth - gap) * 0.5f;
        float panelY = 78.0f * scale;
        float panelH = 74.0f * scale;
        float pad = 10.0f * scale;
        int small = ScaleFontSize(8.0f);
        int medium = ScaleFontSize(10.0f);
        int large = ScaleFontSize(15.0f);

        Rectangle status = { margin, panelY, halfWidth, panelH };
        Rectangle weapon = { margin + halfWidth + gap, panelY, halfWidth, panelH };

        DrawTechPanel(status, HUD_BORDER_COLOR);
        DrawMonitorIcon((Vector2){ status.x + 16.0f * scale, status.y + 16.0f * scale }, 8.0f * scale, HUD_BORDER_COLOR);
        DrawTextStrongFit(T(TEXT_HUD_STATUS_TITLE), (int)(status.x + 30.0f * scale), (int)(status.y + 8.0f * scale),
                          medium, 7, 0.3f * scale, status.width - 40.0f * scale, HUD_BORDER_COLOR, BLACK);
        DrawTextStrongSpaced(GetHudRoundLabel(), (int)(status.x + pad), (int)(status.y + 36.0f * scale),
                             small, 0.3f * scale, RAYWHITE, BLACK);
        DrawTextStrongFit(TextFormat("%d", inTutorialSequence ? tutorialRound : officialRound),
                          (int)(status.x + pad), (int)(status.y + 47.0f * scale), large, 8, 0.3f * scale,
                          status.width - 2.0f * pad, (Color){ 105, 180, 255, 255 }, BLACK);

        DrawTechPanel(weapon, HUD_BORDER_COLOR);
        DrawCrosshairIcon((Vector2){ weapon.x + 16.0f * scale, weapon.y + 16.0f * scale }, 8.0f * scale, HUD_BORDER_COLOR);
        DrawTextStrongFit(T(TEXT_HUD_WEAPON_TITLE), (int)(weapon.x + 30.0f * scale), (int)(weapon.y + 8.0f * scale),
                          medium, 7, 0.3f * scale, weapon.width - 40.0f * scale, HUD_BORDER_COLOR, BLACK);
        DrawTextStrongSpaced(T(TEXT_HUD_BALAS), (int)(weapon.x + pad), (int)(weapon.y + 36.0f * scale),
                             small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
        DrawTextStrongFit(TextFormat("%d / %d", playerAmmo, playerTotalAmmo),
                          (int)(weapon.x + weapon.width - 65.0f * scale), (int)(weapon.y + 31.0f * scale),
                          medium, 7, 0.3f * scale, 55.0f * scale, RAYWHITE, BLACK);
        int ammoSegments = 15;
        int filledAmmoSegments = (PLAYER_MAGAZINE_SIZE > 0) ? (int)ceilf(((float)playerAmmo / (float)PLAYER_MAGAZINE_SIZE) * (float)ammoSegments) : 0;
        DrawSegmentedBar(weapon.x + pad, weapon.y + 51.0f * scale, weapon.width - 2.0f * pad, 8.0f * scale,
                         ammoSegments, filledAmmoSegments, (Color){ 45, 195, 255, 255 }, (Color){ 20, 35, 66, 255 }, scale);

        float bottomY = (float)GetScreenHeight() - 92.0f * scale;
        Rectangle vital = { margin, bottomY, halfWidth, 82.0f * scale };
        Rectangle controls = { margin + halfWidth + gap, bottomY, halfWidth, 82.0f * scale };

        DrawTechPanel(vital, HUD_ACCENT_COLOR);
        DrawHeartbeatIcon((Vector2){ vital.x + 16.0f * scale, vital.y + 16.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
        DrawTextStrongFit(T(TEXT_HUD_VITAL_TITLE), (int)(vital.x + 30.0f * scale), (int)(vital.y + 8.0f * scale),
                          medium, 7, 0.25f * scale, vital.width - 40.0f * scale, HUD_ACCENT_COLOR, BLACK);
        DrawTextStrongSpaced(T(TEXT_HUD_VIDA), (int)(vital.x + pad), (int)(vital.y + 34.0f * scale),
                             small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
        int currentMaxHealth = GetCurrentPlayerMaxHealth();
        DrawTextStrongFit(TextFormat("%d/%d", playerHealth, currentMaxHealth),
                          (int)(vital.x + 48.0f * scale), (int)(vital.y + 30.0f * scale), medium, 7, 0.25f * scale,
                          vital.width - 58.0f * scale, RAYWHITE, BLACK);
        DrawSegmentedBar(vital.x + pad, vital.y + 50.0f * scale, vital.width - 2.0f * pad, 8.0f * scale,
                         10, (int)ceilf(((float)playerHealth / currentMaxHealth) * 10.0f),
                         (Color){ 190, 70, 255, 255 }, (Color){ 35, 25, 60, 255 }, scale);

        DrawTechPanel(controls, HUD_ACCENT_COLOR);
        DrawKeyboardIcon((Vector2){ controls.x + 16.0f * scale, controls.y + 16.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
        DrawTextStrongFit(T(TEXT_HUD_CONTROLS_TITLE), (int)(controls.x + 30.0f * scale), (int)(controls.y + 8.0f * scale),
                          medium, 7, 0.25f * scale, controls.width - 40.0f * scale, HUD_ACCENT_COLOR, BLACK);
        DrawKeyCap("ESPACO", controls.x + pad, controls.y + 31.0f * scale, 54.0f * scale, scale);
        DrawTextStrongFit(T(TEXT_HUD_ATIRAR), (int)(controls.x + 70.0f * scale), (int)(controls.y + 34.0f * scale),
                          small, 7, 0.2f * scale, controls.width - 78.0f * scale, RAYWHITE, BLACK);
        DrawKeyCap("R", controls.x + pad, controls.y + 50.0f * scale, 28.0f * scale, scale);
        DrawTextStrongFit(T(TEXT_HUD_RECARREGAR), (int)(controls.x + 48.0f * scale), (int)(controls.y + 53.0f * scale),
                          small, 7, 0.15f * scale, controls.width - 56.0f * scale, RAYWHITE, BLACK);
        DrawKeyCap("TAB", controls.x + controls.width - 88.0f * scale, controls.y + 31.0f * scale, 38.0f * scale, scale);
        if (!IsTacticalMapAvailable())
        {
            DrawLineEx((Vector2){ controls.x + controls.width - 86.0f * scale, controls.y + 33.0f * scale },
                       (Vector2){ controls.x + controls.width - 52.0f * scale, controls.y + 58.0f * scale },
                       2.0f * scale, (Color){ 255, 70, 90, 255 });
        }
        DrawTextStrongFit(TextFormat("D%d", playerDashCharges), (int)(controls.x + controls.width - 42.0f * scale), (int)(controls.y + 36.0f * scale),
                          small, 7, 0.0f, 30.0f * scale, GOLD, BLACK);
        float mapBatteryFraction = fmaxf(0.0f, fminf(1.0f, mapBattery / MAP_MAX_BATTERY));
        DrawSegmentedBar(controls.x + controls.width - 88.0f * scale, controls.y + 62.0f * scale, 78.0f * scale, 6.0f * scale,
                         10, (int)ceilf(mapBatteryFraction * 10.0f),
                         (Color){ 120, 230, 255, 255 }, (Color){ 20, 42, 58, 255 }, scale);
        DrawTextStrongFit(TextFormat("M%.0f%%", mapBatteryFraction * 100.0f), (int)(controls.x + controls.width - 88.0f * scale), (int)(controls.y + 68.0f * scale),
                          ScaleFontSize(6.0f), 6, 0.0f, 78.0f * scale, RAYWHITE, BLACK);

        return;
    }

    float leftX = 10.0f * scale;
    float panelWidth = GetHudPanelWidth();
    float gap = 10.0f * scale;
    float y = 78.0f * scale;
    float panelHeight = 130.0f * scale;
    float pad = 15.0f * scale;
    float contentWidth = panelWidth - 2.0f * pad;
    int small = ScaleFontSize(10.0f);
    int medium = ScaleFontSize(12.0f);
    int large = ScaleFontSize(27.0f);

    Rectangle status = { leftX, y, panelWidth, panelHeight };
    DrawTechPanel(status, HUD_BORDER_COLOR);
    DrawMonitorIcon((Vector2){ status.x + 18.0f * scale, status.y + 17.0f * scale }, 8.0f * scale, HUD_BORDER_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_STATUS_TITLE), (int)(status.x + 32.0f * scale), (int)(status.y + 11.0f * scale),
                   small, ScaleFontSize(7.0f), 0.25f * scale, panelWidth - 44.0f * scale,
                   HUD_BORDER_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_MAIN_DATA), (int)(status.x + pad), (int)(status.y + 48.0f * scale),
                   small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawTextStrongSpaced(GetHudRoundLabel(), (int)(status.x + pad), (int)(status.y + 68.0f * scale),
                   small, 0.4f * scale, RAYWHITE, BLACK);
    DrawTextStrongFit(TextFormat("%d", inTutorialSequence ? tutorialRound : officialRound),
                      (int)(status.x + pad), (int)(status.y + 82.0f * scale),
                      large, 10, 0.5f * scale, contentWidth,
                      (Color){ 105, 180, 255, 255 }, BLACK);
    if (!inTutorialSequence && officialRound >= MODIFIER_START_ROUND)
    {
        DrawTextStrongFit(GetModifierSummary(), (int)(status.x + pad), (int)(status.y + 112.0f * scale),
                          small, 7, 0.0f, contentWidth, GOLD, BLACK);
    }

    y += panelHeight + gap;
    Rectangle log = { leftX, y, panelWidth, 140.0f * scale };
    DrawTechPanel(log, HUD_ACCENT_COLOR);
    DrawDocumentIcon((Vector2){ log.x + 18.0f * scale, log.y + 17.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_LOG_TITLE), (int)(log.x + 32.0f * scale), (int)(log.y + 11.0f * scale),
                   small, ScaleFontSize(7.0f), 0.25f * scale, panelWidth - 44.0f * scale,
                   HUD_ACCENT_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_EXECUTABLE), (int)(log.x + pad), (int)(log.y + 48.0f * scale),
                   small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawTextStrongFit(TextFormat("%lld %s", executableSizeBytes, T(TEXT_HUD_BYTES)), (int)(log.x + pad), (int)(log.y + 64.0f * scale),
                      medium, 9, 0.25f * scale, contentWidth, RAYWHITE, BLACK);
    DrawTextStrongFit(TextFormat("%.2f%% %s", executableUsagePercent, T(TEXT_HUD_LIMIT)), (int)(log.x + pad), (int)(log.y + 86.0f * scale),
                      medium, 9, 0.4f * scale, contentWidth, (Color){ 80, 200, 235, 255 }, BLACK);
    DrawSegmentedBar(log.x + pad, log.y + 114.0f * scale, contentWidth, 10.0f * scale,
                     10, (int)ceilf(fminf(executableUsagePercent, 100.0f) / 10.0f),
                     (Color){ 50, 205, 255, 255 }, (Color){ 18, 28, 60, 255 }, scale);

    y += log.height + gap;
    float dataPanelHeight = 194.0f * scale;
    float controlsPanelHeight = 460.0f * scale;

    Rectangle vital = { leftX, y, panelWidth, dataPanelHeight };
    DrawTechPanel(vital, HUD_ACCENT_COLOR);
    DrawHeartbeatIcon((Vector2){ vital.x + 18.0f * scale, vital.y + 17.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_VITAL_TITLE), (int)(vital.x + 32.0f * scale), (int)(vital.y + 11.0f * scale),
                   medium, ScaleFontSize(8.0f), 0.25f * scale, vital.width - 44.0f * scale,
                   HUD_ACCENT_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_VIDA), (int)(vital.x + pad), (int)(vital.y + 54.0f * scale),
                   medium, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    int currentMaxHealth = GetCurrentPlayerMaxHealth();
    DrawTextStrongFit(TextFormat("%d/%d", playerHealth, currentMaxHealth),
                      (int)(vital.x + pad), (int)(vital.y + 78.0f * scale),
                      large, ScaleFontSize(11.0f), 0.35f * scale, vital.width - 2.0f * pad, RAYWHITE, BLACK);
    DrawSegmentedBar(vital.x + pad, vital.y + vital.height - 34.0f * scale, vital.width - 2.0f * pad, 14.0f * scale,
                     10, (int)ceilf(((float)playerHealth / currentMaxHealth) * 10.0f),
                     (Color){ 190, 70, 255, 255 }, (Color){ 35, 25, 60, 255 }, scale);

    float rightWidth = GetHudRightPanelWidth();
    float rightX = (float)GetScreenWidth() - rightWidth - (10.0f * scale);
    float rightContentWidth = rightWidth - 2.0f * pad;

    Rectangle weapon = { rightX, 92.0f * scale, rightWidth, dataPanelHeight };
    DrawTechPanel(weapon, HUD_BORDER_COLOR);
    DrawCrosshairIcon((Vector2){ weapon.x + 18.0f * scale, weapon.y + 17.0f * scale }, 8.0f * scale, HUD_BORDER_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_WEAPON_TITLE), (int)(weapon.x + 32.0f * scale), (int)(weapon.y + 11.0f * scale),
                   medium, ScaleFontSize(8.0f), 0.25f * scale, weapon.width - 44.0f * scale,
                   HUD_BORDER_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_BALAS), (int)(weapon.x + pad), (int)(weapon.y + 54.0f * scale),
                   medium, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawTextStrongFit(TextFormat("%d / %d", playerAmmo, playerTotalAmmo),
                      (int)(weapon.x + pad), (int)(weapon.y + 78.0f * scale),
                      large, ScaleFontSize(11.0f), 0.35f * scale, rightContentWidth, RAYWHITE, BLACK);
    int ammoSegments = 15;
    int filledAmmoSegments = (PLAYER_MAGAZINE_SIZE > 0) ? (int)ceilf(((float)playerAmmo / (float)PLAYER_MAGAZINE_SIZE) * (float)ammoSegments) : 0;
    DrawSegmentedBar(weapon.x + pad, weapon.y + weapon.height - 34.0f * scale, rightContentWidth, 14.0f * scale,
                     ammoSegments, filledAmmoSegments, (Color){ 45, 195, 255, 255 }, (Color){ 20, 35, 66, 255 }, scale);
    if (playerReloadTimer > 0.0f)
    {
        /* Reload animation: a horizontal sweep bar fills up under the ammo
         * count while playerReloadTimer counts down, and the label pulses
         * so a reload in progress is unmistakable at a glance. */
        float reloadFraction = 1.0f - (playerReloadTimer / PLAYER_RELOAD_TIME);
        DrawRectangleRounded((Rectangle){ weapon.x + pad, weapon.y + weapon.height - 34.0f * scale, rightContentWidth, 14.0f * scale }, 0.5f, 6, (Color){ 20, 35, 66, 255 });
        DrawRectangleRounded((Rectangle){ weapon.x + pad, weapon.y + weapon.height - 34.0f * scale, rightContentWidth * reloadFraction, 14.0f * scale }, 0.5f, 6, (Color){ 255, 205, 60, 255 });
        float pulse = 0.55f + 0.45f * sinf((float)GetTime() * 8.0f);
        DrawTextStrongSpaced(TextFormat("%s %.1fs", T(TEXT_RELOAD_IN_PROGRESS), playerReloadTimer),
                       (int)(weapon.x + pad), (int)(weapon.y + weapon.height - 58.0f * scale),
                       small, 0.2f * scale, Fade((Color){ 255, 205, 60, 255 }, pulse), BLACK);
    }

    Rectangle controls = { rightX, weapon.y + weapon.height + gap, rightWidth, controlsPanelHeight };
    DrawTechPanel(controls, HUD_ACCENT_COLOR);
    DrawKeyboardIcon((Vector2){ controls.x + 18.0f * scale, controls.y + 17.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_CONTROLS_TITLE), (int)(controls.x + 32.0f * scale), (int)(controls.y + 11.0f * scale),
                   medium, ScaleFontSize(8.0f), 0.15f * scale, rightWidth - 44.0f * scale,
                   HUD_ACCENT_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_ATIRAR), (int)(controls.x + pad), (int)(controls.y + 54.0f * scale),
                   small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawKeyCap("ESPACO", controls.x + pad, controls.y + 76.0f * scale, fminf(104.0f * scale, rightContentWidth), scale);
    DrawTextStrongSpaced(T(TEXT_HUD_RECARREGAR), (int)(controls.x + pad), (int)(controls.y + 112.0f * scale),
                   small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawKeyCap("R", controls.x + pad, controls.y + 132.0f * scale, 44.0f * scale, scale);
    DrawTextStrongSpaced("DASH / MAPA", (int)(controls.x + pad), (int)(controls.y + 164.0f * scale),
                   small, 0.15f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawKeyCap("SHIFT", controls.x + pad, controls.y + 184.0f * scale, 66.0f * scale, scale);
    DrawKeyCap("TAB", controls.x + pad + 76.0f * scale, controls.y + 184.0f * scale, 52.0f * scale, scale);
    DrawTextStrongFit(TextFormat("%d/%d DASH", playerDashCharges, GetRoundDashMaxCharges()),
                      (int)(controls.x + pad + 138.0f * scale), (int)(controls.y + 190.0f * scale),
                      small, 7, 0.0f, rightContentWidth - 138.0f * scale, GOLD, BLACK);
    float infoY = 222.0f * scale;
    if (!IsTacticalMapAvailable())
    {
        DrawTextStrongFit("MAPA BLOQUEADO", (int)(controls.x + pad), (int)(controls.y + infoY),
                          small, 7, 0.0f, rightContentWidth, (Color){ 255, 70, 90, 255 }, BLACK);
        infoY += 18.0f * scale;
    }
    if (playerDashCooldown > 0.0f || playerDashCharges <= 0)
    {
        DrawTextStrongFit((playerDashCharges <= 0) ? "DASH ESGOTADO" : TextFormat("DASH %.1fs", playerDashCooldown), (int)(controls.x + pad), (int)(controls.y + infoY),
                          small, 7, 0.0f, rightContentWidth, GOLD, BLACK);
        infoY += 18.0f * scale;
    }

    if (IsLockedExitRound())
    {
        DrawTextStrongSpaced(hasExitKey ? "CHAVE: OK" : "ACHE A CHAVE",
                             (int)(controls.x + pad), (int)(controls.y + infoY),
                             small, 0.1f * scale, hasExitKey ? MAZE_EXIT_COLOR : GOLD, BLACK);
        infoY += 28.0f * scale;
    }

    float mapY = infoY + 10.0f * scale;
    float mapBatteryFraction = fmaxf(0.0f, fminf(1.0f, mapBattery / MAP_MAX_BATTERY));
    DrawTextStrongSpaced("MAPA", (int)(controls.x + pad), (int)(controls.y + mapY),
                         small, 0.3f * scale, HUD_ACCENT_COLOR, BLACK);
    DrawTextStrongFit(TextFormat("%.0f%%", (mapBattery / MAP_MAX_BATTERY) * 100.0f),
                      (int)(controls.x + rightWidth - pad - 54.0f * scale), (int)(controls.y + mapY),
                      small, 6, 0.2f * scale, 54.0f * scale, RAYWHITE, BLACK);
    DrawSegmentedBar(controls.x + pad, controls.y + mapY + 26.0f * scale, rightContentWidth, 8.0f * scale,
                     10, (int)ceilf(mapBatteryFraction * 10.0f),
                     (Color){ 120, 230, 255, 255 }, (Color){ 20, 42, 58, 255 }, scale);

    if (currentRoundConfig.flashlightEnabled)
    {
        float percentWidth = 54.0f * scale;
        float flashlightY = mapY + 50.0f * scale;
        DrawTextStrongSpaced(T(TEXT_HUD_LANTERNA), (int)(controls.x + pad), (int)(controls.y + flashlightY),
                   small, 0.3f * scale, HUD_ACCENT_COLOR, BLACK);
        DrawKeyCap("C", controls.x + pad, controls.y + flashlightY + 20.0f * scale, 44.0f * scale, scale);
        float batteryFraction = fmaxf(0.0f, fminf(1.0f, flashlightBattery / FLASHLIGHT_MAX_BATTERY));
        DrawTextStrongFit(TextFormat("%.0f%%", (flashlightBattery / FLASHLIGHT_MAX_BATTERY) * 100.0f), (int)(controls.x + rightWidth - pad - percentWidth), (int)(controls.y + flashlightY + 24.0f * scale),
                       small, 6, 0.2f * scale, percentWidth, RAYWHITE, BLACK);
        DrawSegmentedBar(controls.x + pad, controls.y + flashlightY + 56.0f * scale, rightContentWidth, 8.0f * scale,
                         10, (int)ceilf(batteryFraction * 10.0f),
                         (Color){ 255, 205, 60, 255 }, (Color){ 60, 50, 20, 255 }, scale);
    }

    if (IsLightningAvailableThisRound())
    {
        float lightningY = currentRoundConfig.flashlightEnabled ? (mapY + 122.0f * scale) : (mapY + 44.0f * scale);
        DrawTextStrongSpaced(T(TEXT_HUD_LIGHTNING), (int)(controls.x + pad), (int)(controls.y + lightningY),
                   small, 0.3f * scale, (Color){ 120, 230, 255, 255 }, BLACK);
        DrawKeyCap("F", controls.x + pad, controls.y + lightningY + 20.0f * scale, 44.0f * scale, scale);
        DrawTextStrongFit(TextFormat("%d", lightningCharges), (int)(controls.x + 72.0f * scale), (int)(controls.y + lightningY + 24.0f * scale),
                       small, 6, 0.2f * scale, rightContentWidth - 72.0f * scale, RAYWHITE, BLACK);
    }

}

void DrawGameOverOverlay(void)
{
    const char *title = T(TEXT_GAME_OVER);
    const char *buttonText = T(TEXT_PLAY_AGAIN);
    float scale = GetUIScale();
    int titleFontSize = ScaleFontSize(42.0f);
    int buttonFontSize = ScaleFontSize(24.0f);
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float panelWidth = fminf((float)screenWidth - (40.0f * scale), 460.0f * scale);
    float panelHeight = 240.0f * scale;
    Rectangle panel = {
        ((float)screenWidth - panelWidth) * 0.5f,
        ((float)screenHeight - panelHeight) * 0.5f,
        panelWidth,
        panelHeight
    };
    Rectangle button = {
        panel.x + (70.0f * scale),
        panel.y + (146.0f * scale),
        panel.width - (140.0f * scale),
        58.0f * scale
    };
    titleFontSize = FitFontSizeToWidth(title, titleFontSize, ScaleFontSize(24.0f), 1.0f * scale, panel.width - (44.0f * scale));
    buttonFontSize = FitFontSizeToWidth(buttonText, buttonFontSize, ScaleFontSize(15.0f), 1.0f * scale, button.width - (24.0f * scale));
    int titleWidth = (int)MeasureTextStrongSpaced(title, titleFontSize, 1.0f * scale).x;
    int buttonTextWidth = (int)MeasureTextStrongSpaced(buttonText, buttonFontSize, 1.0f * scale).x;
    Vector2 mousePosition = GetMousePosition();
    bool isButtonHovered = CheckCollisionPointRec(mousePosition, button);
    Color dangerColor = (Color){ 230, 70, 90, 255 };
    Color buttonColor = isButtonHovered ? (Color){ 255, 110, 125, 255 } : dangerColor;

    /* Darker, blurred-glass backdrop (instead of the flat grey wash) keeps
     * the maze faintly visible behind the panel while pulling focus to the
     * "system compromised" message. */
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.72f));

    /* Soft red glow radiating from the panel, echoing a "critical alert"
     * rather than a plain grey error box. */
    DrawRectangleRounded((Rectangle){ panel.x - 10.0f * scale, panel.y - 10.0f * scale, panel.width + 20.0f * scale, panel.height + 20.0f * scale }, 0.14f, 12, Fade(dangerColor, 0.10f));
    DrawRectangleRounded((Rectangle){ panel.x - 4.0f * scale, panel.y - 4.0f * scale, panel.width + 8.0f * scale, panel.height + 8.0f * scale }, 0.13f, 12, Fade(dangerColor, 0.16f));
    DrawRectangleRounded(panel, 0.12f, 12, (Color){ 10, 8, 20, 250 });
    DrawRectangleRoundedLinesEx(panel, 0.12f, 12, 2.0f * scale, Fade(dangerColor, 0.9f));

    /* A thin horizontal "system fault" divider under the title, and a
     * small warning glyph above it, to sell the alert without needing an
     * image asset. */
    Vector2 warnCenter = { panel.x + panel.width * 0.5f, panel.y + 30.0f * scale };
    DrawTriangleLines(
        (Vector2){ warnCenter.x, warnCenter.y - 12.0f * scale },
        (Vector2){ warnCenter.x - 12.0f * scale, warnCenter.y + 9.0f * scale },
        (Vector2){ warnCenter.x + 12.0f * scale, warnCenter.y + 9.0f * scale },
        dangerColor
    );
    DrawCircleV((Vector2){ warnCenter.x, warnCenter.y + 4.0f * scale }, fmaxf(1.0f, 1.4f * scale), dangerColor);
    DrawRectangleRec((Rectangle){ warnCenter.x - 1.0f * scale, warnCenter.y - 6.0f * scale, 2.0f * scale, 7.0f * scale }, dangerColor);

    DrawTextStrong(title, (int)(panel.x + (panel.width - titleWidth) * 0.5f), (int)(panel.y + 58.0f * scale), titleFontSize, RAYWHITE, dangerColor);
    DrawLineEx((Vector2){ panel.x + 40.0f * scale, panel.y + 108.0f * scale }, (Vector2){ panel.x + panel.width - 40.0f * scale, panel.y + 108.0f * scale }, 1.0f * scale, Fade(dangerColor, 0.4f));

    DrawRectangleRounded(button, 0.3f, 12, Fade(BLACK, 0.3f));
    DrawRectangleRounded((Rectangle){ button.x - 2.0f * scale, button.y - 2.0f * scale, button.width + 4.0f * scale, button.height + 4.0f * scale }, 0.32f, 12, Fade(buttonColor, isButtonHovered ? 0.4f : 0.22f));
    DrawRectangleRounded(button, 0.3f, 12, buttonColor);
    DrawTextStrong(buttonText, (int)(button.x + (button.width - buttonTextWidth) * 0.5f), (int)(button.y + 16.0f * scale), buttonFontSize, RAYWHITE, Fade(BLACK, 0.35f));

    if (isButtonHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        ResetGame();
    }
}

Rectangle GetVictoryNextButtonRect(Rectangle panel)
{
    float scale = GetUIScale();
    float gap = 16.0f * scale;
    float buttonWidth = (panel.width - 88.0f * scale - gap) * 0.5f;

    return (Rectangle){
        panel.x + 44.0f * scale + buttonWidth + gap,
        panel.y + panel.height - 78.0f * scale,
        buttonWidth,
        58.0f * scale
    };
}

Rectangle GetVictoryRoundsButtonRect(Rectangle panel)
{
    float scale = GetUIScale();
    float gap = 16.0f * scale;
    float buttonWidth = (panel.width - 88.0f * scale - gap) * 0.5f;

    return (Rectangle){
        panel.x + 44.0f * scale,
        panel.y + panel.height - 78.0f * scale,
        buttonWidth,
        58.0f * scale
    };
}

Rectangle GetRoundHistoryPanelRect(void)
{
    float scale = GetUIScale();
    float panelWidth = fminf((float)GetScreenWidth() - 48.0f * scale, 860.0f * scale);
    float panelHeight = fminf((float)GetScreenHeight() - 80.0f * scale, 560.0f * scale);

    return (Rectangle){
        ((float)GetScreenWidth() - panelWidth) * 0.5f,
        ((float)GetScreenHeight() - panelHeight) * 0.5f,
        panelWidth,
        panelHeight
    };
}

int GetCompletedRoundCountForHistory(void)
{
    int completedRounds = (bestOfficialRound > 1) ? bestOfficialRound - 1 : 0;

    if (inTutorialSequence)
    {
        return 0;
    }

    if (gamePhase == PHASE_PLAYING && waitingForVictorySound)
    {
        return (officialRound > completedRounds) ? officialRound : completedRounds;
    }

    return completedRounds;
}

int GetNextRoundForHistory(void)
{
    if (gamePhase == PHASE_PLAYING && waitingForVictorySound)
    {
        return officialRound + 1;
    }

    return officialRound;
}

void DrawRoundHistoryPanel(void)
{
    float scale = GetUIScale();
    Rectangle panel = GetRoundHistoryPanelRect();
    int completedRounds = GetCompletedRoundCountForHistory();
    int nextRound = GetNextRoundForHistory();
    int titleSize = ScaleFontSize(25.0f);
    int smallSize = ScaleFontSize(10.0f);
    int numberSize = ScaleFontSize(14.0f);
    float pad = 28.0f * scale;
    float headerBottom = panel.y + 156.0f * scale;
    float footerTop = panel.y + panel.height - 82.0f * scale;
    float gridGap = 8.0f * scale;
    float gridHeight = fmaxf(44.0f * scale, footerTop - headerBottom - 12.0f * scale);
    int maxRowsAtMinSize = (int)fmaxf(1.0f, floorf((gridHeight + gridGap) / (18.0f * scale + gridGap)));
    int columns = IsCompactLayout() ? 5 : 8;
    if (completedRounds > columns * maxRowsAtMinSize)
    {
        columns = (int)ceilf((float)completedRounds / (float)maxRowsAtMinSize);
    }
    if (columns < 1) columns = 1;
    float cellWidth = (panel.width - pad * 2.0f - gridGap * (float)(columns - 1)) / (float)columns;
    if (cellWidth < 22.0f * scale)
    {
        gridGap = 4.0f * scale;
        cellWidth = (panel.width - pad * 2.0f - gridGap * (float)(columns - 1)) / (float)columns;
    }
    int rows = (completedRounds > 0) ? (int)ceilf((float)completedRounds / (float)columns) : 1;
    float cellHeight = fminf(42.0f * scale, (gridHeight - gridGap * (float)(rows - 1)) / (float)rows);
    cellHeight = fmaxf(18.0f * scale, cellHeight);

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.72f));
    DrawRectangleRounded(panel, 0.08f, 12, (Color){ 6, 12, 36, 248 });
    DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 3.0f * scale, HUD_BORDER_COLOR);
    DrawTextStrongFit("ROUNDS FEITOS", (int)(panel.x + pad), (int)(panel.y + 30.0f * scale),
                      titleSize, ScaleFontSize(14.0f), 0.5f * scale, panel.width - pad * 2.0f, RAYWHITE, BLACK);
    DrawTextStrongFit(TextFormat("CONCLUIDOS: %d    PROXIMO: %d", completedRounds, nextRound),
                      (int)(panel.x + pad), (int)(panel.y + 78.0f * scale),
                      ScaleFontSize(13.0f), ScaleFontSize(9.0f), 0.0f, panel.width - pad * 2.0f,
                      (Color){ 120, 230, 255, 255 }, BLACK);
    DrawTextStrongFit(TextFormat("RECORDE %d   NIVEL %d   MOEDAS %d   PROX MOD: %s",
                                 bestOfficialRound, playerLevel, playerCoins,
                                 (nextRound >= MODIFIER_START_ROUND) ? FormatModifierSummary(GetRoundModifiers(nextRound)) : "PADRAO"),
                      (int)(panel.x + pad), (int)(panel.y + 108.0f * scale),
                      ScaleFontSize(10.0f), ScaleFontSize(8.0f), 0.0f, panel.width - pad * 2.0f,
                      GOLD, BLACK);
    DrawRectangleRec((Rectangle){ panel.x + pad, panel.y + 138.0f * scale, panel.width - pad * 2.0f, 4.0f * scale }, MAGENTA);

    for (int i = 0; i < completedRounds; i++)
    {
        int roundNumber = i + 1;
        int row = i / columns;
        int column = i % columns;
        Rectangle card = {
            panel.x + pad + (float)column * (cellWidth + gridGap),
            headerBottom + (float)row * (cellHeight + gridGap),
            cellWidth,
            cellHeight
        };
        bool selected = roundNumber == officialRound;
        Color cardFill = selected ? (Color){ 22, 62, 112, 255 } : (Color){ 8, 14, 34, 245 };
        Color cardBorder = selected ? GOLD : Fade(HUD_BORDER_COLOR, 0.68f);
        DrawRectangleRounded(card, 0.08f, 8, cardFill);
        DrawRectangleRoundedLinesEx(card, 0.08f, 8, selected ? 2.0f * scale : 1.0f * scale, cardBorder);
        DrawTextStrongFit(TextFormat("%02d", roundNumber), (int)(card.x + 8.0f * scale), (int)(card.y + 10.0f * scale),
                          numberSize, 8, 0.0f, card.width - 16.0f * scale, RAYWHITE, BLACK);
    }

    if (completedRounds <= 0)
    {
        DrawTextStrongFit("Nenhum round oficial concluido ainda.", (int)(panel.x + pad), (int)(headerBottom + 12.0f * scale),
                          smallSize, 8, 0.0f, panel.width - pad * 2.0f, LIGHTGRAY, BLACK);
    }

    Rectangle nextButton = GetVictoryNextButtonRect(panel);
    Rectangle backButton = GetVictoryRoundsButtonRect(panel);
    Vector2 mouse = GetMousePosition();
    bool nextHover = CheckCollisionPointRec(mouse, nextButton);
    bool backHover = CheckCollisionPointRec(mouse, backButton);

    DrawRectangleRounded(backButton, 0.18f, 10, backHover ? (Color){ 28, 38, 78, 255 } : (Color){ 12, 18, 44, 255 });
    DrawRectangleRoundedLinesEx(backButton, 0.18f, 10, 1.5f * scale, HUD_BORDER_COLOR);
    DrawTextStrongFit("VOLTAR", (int)(backButton.x + 14.0f * scale), (int)(backButton.y + 14.0f * scale),
                      smallSize, 8, 0.0f, backButton.width - 28.0f * scale, RAYWHITE, BLACK);

    DrawRectangleRounded(nextButton, 0.18f, 10, nextHover ? (Color){ 120, 255, 190, 255 } : (Color){ 65, 230, 150, 255 });
    DrawRectangleRoundedLinesEx(nextButton, 0.18f, 10, 1.5f * scale, RAYWHITE);
    DrawTextStrongFit(T(TEXT_NEXT_ROUND), (int)(nextButton.x + 14.0f * scale), (int)(nextButton.y + 14.0f * scale),
                      smallSize, 8, 0.0f, nextButton.width - 28.0f * scale, BLACK, Fade(WHITE, 0.2f));
}

int GetClickedHistoryRound(Vector2 mouse)
{
    float scale = GetUIScale();
    Rectangle panel = GetRoundHistoryPanelRect();
    int completedRounds = GetCompletedRoundCountForHistory();
    float pad = 28.0f * scale;
    float headerBottom = panel.y + 156.0f * scale;
    float footerTop = panel.y + panel.height - 82.0f * scale;
    float gridGap = 8.0f * scale;
    float gridHeight = fmaxf(44.0f * scale, footerTop - headerBottom - 12.0f * scale);
    int maxRowsAtMinSize = (int)fmaxf(1.0f, floorf((gridHeight + gridGap) / (18.0f * scale + gridGap)));
    int columns = IsCompactLayout() ? 5 : 8;

    if (completedRounds <= 0)
    {
        return 0;
    }

    if (completedRounds > columns * maxRowsAtMinSize)
    {
        columns = (int)ceilf((float)completedRounds / (float)maxRowsAtMinSize);
    }
    if (columns < 1) columns = 1;

    float cellWidth = (panel.width - pad * 2.0f - gridGap * (float)(columns - 1)) / (float)columns;
    if (cellWidth < 22.0f * scale)
    {
        gridGap = 4.0f * scale;
        cellWidth = (panel.width - pad * 2.0f - gridGap * (float)(columns - 1)) / (float)columns;
    }

    int rows = (int)ceilf((float)completedRounds / (float)columns);
    float cellHeight = fminf(42.0f * scale, (gridHeight - gridGap * (float)(rows - 1)) / (float)rows);
    cellHeight = fmaxf(18.0f * scale, cellHeight);

    for (int i = 0; i < completedRounds; i++)
    {
        int row = i / columns;
        int column = i % columns;
        Rectangle card = {
            panel.x + pad + (float)column * (cellWidth + gridGap),
            headerBottom + (float)row * (cellHeight + gridGap),
            cellWidth,
            cellHeight
        };

        if (CheckCollisionPointRec(mouse, card))
        {
            return i + 1;
        }
    }

    return 0;
}

void DrawVictoryOverlay(void)
{
    if (victoryRoundsOpen)
    {
        DrawRoundHistoryPanel();
        return;
    }

    float scale = GetUIScale();
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int completedNumber = inTutorialSequence ? tutorialRound : officialRound;
    const char *title = TextFormat(T(TEXT_STAGE_COMPLETE), GetStageCompleteLabel(), completedNumber);
    const char *buttonText = T(TEXT_NEXT_ROUND);
    const char *roundsText = "ROUNDS FEITOS";
    int titleFontSize = ScaleFontSize(33.0f);
    int buttonFontSize = ScaleFontSize(22.0f);
    float panelWidth = fminf((float)screenWidth - 40.0f * scale, 540.0f * scale);
    float panelHeight = 300.0f * scale;
    Rectangle panel = {
        ((float)screenWidth - panelWidth) * 0.5f,
        ((float)screenHeight - panelHeight) * 0.5f,
        panelWidth,
        panelHeight
    };
    Rectangle button = GetVictoryNextButtonRect(panel);
    Rectangle roundsButton = GetVictoryRoundsButtonRect(panel);
    Vector2 mousePosition = GetMousePosition();
    bool isButtonHovered = CheckCollisionPointRec(mousePosition, button);
    bool isRoundsHovered = CheckCollisionPointRec(mousePosition, roundsButton);
    Color successColor = (Color){ 65, 230, 150, 255 };
    Color buttonColor = isButtonHovered ? (Color){ 120, 255, 190, 255 } : successColor;

    titleFontSize = FitFontSizeToWidth(title, titleFontSize, ScaleFontSize(18.0f), 0.8f * scale, panel.width - 48.0f * scale);
    buttonFontSize = FitFontSizeToWidth(buttonText, buttonFontSize, ScaleFontSize(13.0f), 0.7f * scale, button.width - 62.0f * scale);
    int roundsFontSize = FitFontSizeToWidth(roundsText, ScaleFontSize(15.0f), ScaleFontSize(9.0f), 0.2f * scale, roundsButton.width - 24.0f * scale);

    int titleWidth = (int)MeasureTextStrongSpaced(title, titleFontSize, 0.8f * scale).x;

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.62f));
    DrawRectangleRounded((Rectangle){ panel.x - 12.0f * scale, panel.y - 12.0f * scale, panel.width + 24.0f * scale, panel.height + 24.0f * scale }, 0.14f, 12, Fade(successColor, 0.12f));
    DrawRectangleRounded((Rectangle){ panel.x - 4.0f * scale, panel.y - 4.0f * scale, panel.width + 8.0f * scale, panel.height + 8.0f * scale }, 0.13f, 12, Fade(HUD_BORDER_COLOR, 0.20f));
    DrawRectangleRounded(panel, 0.12f, 12, (Color){ 6, 18, 34, 250 });
    DrawRectangleRoundedLinesEx(panel, 0.12f, 12, 2.0f * scale, successColor);

    Vector2 badgeCenter = { panel.x + panel.width * 0.5f, panel.y + 42.0f * scale };
    DrawCircleV(badgeCenter, 22.0f * scale, Fade(successColor, 0.20f));
    DrawCircleLinesV(badgeCenter, 19.0f * scale, successColor);
    DrawLineEx((Vector2){ badgeCenter.x - 8.0f * scale, badgeCenter.y },
               (Vector2){ badgeCenter.x - 2.0f * scale, badgeCenter.y + 7.0f * scale },
               3.0f * scale, successColor);
    DrawLineEx((Vector2){ badgeCenter.x - 2.0f * scale, badgeCenter.y + 7.0f * scale },
               (Vector2){ badgeCenter.x + 10.0f * scale, badgeCenter.y - 8.0f * scale },
               3.0f * scale, successColor);

    DrawTextStrongSpaced(title,
                         (int)(panel.x + (panel.width - (float)titleWidth) * 0.5f),
                         (int)(panel.y + 82.0f * scale),
                         titleFontSize,
                         0.8f * scale,
                         RAYWHITE,
                         successColor);
    DrawLineEx((Vector2){ panel.x + 42.0f * scale, panel.y + 130.0f * scale },
               (Vector2){ panel.x + panel.width - 42.0f * scale, panel.y + 130.0f * scale },
               1.0f * scale, Fade(successColor, 0.42f));

    if (!inTutorialSequence)
    {
        const char *bonusText = TextFormat("ROUND +%d  |  ITENS %d  |  XP +%d", pendingRoundBonus, roundPickupsCollected, pendingEvolutionXp);
        DrawTextStrongFit(bonusText, (int)(panel.x + 44.0f * scale), (int)(panel.y + 148.0f * scale),
                          ScaleFontSize(10.0f), ScaleFontSize(8.0f), 0.0f, panel.width - 88.0f * scale, GOLD, BLACK);
    }

    DrawRectangleRounded(roundsButton, 0.24f, 10, isRoundsHovered ? (Color){ 28, 38, 78, 255 } : (Color){ 12, 18, 44, 255 });
    DrawRectangleRoundedLinesEx(roundsButton, 0.24f, 10, 1.5f * scale, HUD_BORDER_COLOR);
    DrawTextStrongSpaced(roundsText,
                         (int)(roundsButton.x + 12.0f * scale),
                         (int)(roundsButton.y + (roundsButton.height - (float)roundsFontSize) * 0.45f),
                         roundsFontSize,
                         0.2f * scale,
                         RAYWHITE,
                         BLACK);

    DrawRectangleRounded((Rectangle){ button.x - 2.0f * scale, button.y - 2.0f * scale, button.width + 4.0f * scale, button.height + 4.0f * scale }, 0.32f, 12, Fade(buttonColor, isButtonHovered ? 0.42f : 0.24f));
    DrawRectangleRounded(button, 0.3f, 12, buttonColor);
    DrawRectangleRoundedLinesEx(button, 0.3f, 12, 1.5f * scale, RAYWHITE);
    DrawTextStrongSpaced(buttonText,
                         (int)(button.x + 20.0f * scale),
                         (int)(button.y + (button.height - (float)buttonFontSize) * 0.45f),
                         buttonFontSize,
                         0.7f * scale,
                         BLACK,
                         Fade(WHITE, 0.25f));
    DrawButtonArrow(button, BLACK);
}

bool HandleVictoryButton(void)
{
    if (gamePhase != PHASE_PLAYING || !waitingForVictorySound || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return false;
    }

    float scale = GetUIScale();
    float panelWidth = fminf((float)GetScreenWidth() - 40.0f * scale, 540.0f * scale);
    float panelHeight = 300.0f * scale;
    Rectangle panel = {
        ((float)GetScreenWidth() - panelWidth) * 0.5f,
        ((float)GetScreenHeight() - panelHeight) * 0.5f,
        panelWidth,
        panelHeight
    };

    if (victoryRoundsOpen)
    {
        panel = GetRoundHistoryPanelRect();
        int selectedRound = GetClickedHistoryRound(GetMousePosition());
        if (selectedRound > 0)
        {
            officialRound = selectedRound;
            victoryRoundsOpen = false;
            waitingForVictorySound = false;
            StartCurrentStage();
            return true;
        }

        if (CheckCollisionPointRec(GetMousePosition(), GetVictoryRoundsButtonRect(panel)))
        {
            victoryRoundsOpen = false;
            return true;
        }
    }

    if (CheckCollisionPointRec(GetMousePosition(), GetVictoryNextButtonRect(panel)))
    {
        if (gameAudioLoaded && IsSoundPlaying(victorySound))
        {
            StopSound(victorySound);
        }

        AdvanceToNextStage();
        return true;
    }

    if (!victoryRoundsOpen && CheckCollisionPointRec(GetMousePosition(), GetVictoryRoundsButtonRect(panel)))
    {
        victoryRoundsOpen = true;
        return true;
    }

    return false;
}

void DrawPlayer(void)
{
    MazeLayout layout = GetMazeLayout();
    float scale = layout.drawScale;
    Vector2 center = WorldToScreenPositionLayout(player.position, layout);
    float drawRadius = fmaxf(player.radius * scale * 1.1f, 8.0f);
    Color playerCore = (Color){ 255, 185, 55, 255 };
    Color playerEdge = (Color){ 255, 235, 200, 255 };
    Color playerOuter = (Color){ 40, 18, 0, 255 };
    Color playerGlow = (Color){ 80, 220, 255, 255 };

    Vector2 tip = {
        center.x + (cosf(player.facingAngle) * drawRadius * 1.2f),
        center.y + (sinf(player.facingAngle) * drawRadius * 1.2f)
    };
    
    Vector2 left = {
        center.x + (cosf(player.facingAngle + 2.45f) * drawRadius * 0.98f),
        center.y + (sinf(player.facingAngle + 2.45f) * drawRadius * 0.98f)
    };
    Vector2 right = {
        center.x + (cosf(player.facingAngle - 2.45f) * drawRadius * 0.98f),
        center.y + (sinf(player.facingAngle - 2.45f) * drawRadius * 0.98f)
    };
    Vector2 shadowTip = { tip.x + 2.0f * scale, tip.y + 2.0f * scale };
    Vector2 shadowLeft = { left.x + 2.0f * scale, left.y + 2.0f * scale };
    Vector2 shadowRight = { right.x + 2.0f * scale, right.y + 2.0f * scale };

    if (playerStartAuraVisible)
    {
        float blink = (sinf((float)GetTime() * 9.0f) + 1.0f) * 0.5f;
        DrawCircleV(center, drawRadius * (3.0f + blink * 0.6f), Fade(playerGlow, 0.18f + blink * 0.14f));
        DrawCircleLines((int)center.x, (int)center.y, drawRadius * (2.15f + blink * 0.35f), Fade(playerGlow, 0.75f + blink * 0.25f));
    }

    if (playerDashTimer > 0.0f || playerDamageCooldown > 0.0f)
    {
        float pulse = 0.45f + 0.35f * sinf((float)GetTime() * 14.0f);
        DrawCircleV(center, drawRadius * 2.25f, Fade(playerGlow, pulse));
    }

    DrawTriangle(shadowTip, shadowRight, shadowLeft, Fade(BLACK, 0.55f));
    DrawTriangleLines(tip, right, left, playerOuter);
    DrawTriangleLines(tip, right, left, playerOuter);
    DrawTriangle(tip, right, left, playerCore);
    DrawTriangleLines(tip, right, left, playerEdge);
    DrawCircleV(center, fmaxf(drawRadius * 0.18f, 2.0f), playerEdge);
}

void DrawFloatingNotices(void)
{
    MazeLayout layout = GetMazeLayout();
    int fontSize = ScaleFontSize(8.0f);

    for (int i = 0; i < MAX_FLOATING_NOTICES; i++)
    {
        if (floatingNotices[i].timer <= 0.0f)
        {
            continue;
        }

        Vector2 screenPosition = WorldToScreenPositionLayout(floatingNotices[i].position, layout);
        float alpha = fminf(1.0f, floatingNotices[i].timer / FLOATING_NOTICE_TIME);
        DrawTextStrongSpaced(floatingNotices[i].text, (int)(screenPosition.x - 38.0f * layout.drawScale),
                             (int)(screenPosition.y - 24.0f * layout.drawScale), fontSize, 0.0f,
                             Fade(floatingNotices[i].color, alpha), BLACK);
    }
}

void DrawEnemies(void)
{
    MazeLayout layout = GetMazeLayout();
    float scale = layout.drawScale;

    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (!redEnemies[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPositionLayout(redEnemies[i].position, layout);
        float drawRadius = fmaxf(redEnemies[i].radius * scale, 6.0f);
        Color redBody = RED;

        if (redEnemies[i].knockoutTimer > 0.0f)
        {
            redBody = ((int)(redEnemies[i].knockoutTimer * 10.0f) % 2 == 0) ? (Color){ 255, 200, 200, 255 } : Fade(RED, 0.2f);
        }

        DrawCircleV(center, drawRadius, redBody);
        DrawCircleLines((int)center.x, (int)center.y, drawRadius, RAYWHITE);
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (!blueEnemies[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPositionLayout(blueEnemies[i].position, layout);
        float drawRadius = fmaxf(blueEnemies[i].radius * scale, 7.0f);
        Color pinkColor = (Color){ 255, 45, 165, 255 };
        Color bodyColor = pinkColor;

        if (blueEnemies[i].knockoutTimer > 0.0f)
        {
            bodyColor = ((int)(blueEnemies[i].knockoutTimer * 8.0f) % 2 == 0) ? (Color){ 255, 190, 225, 255 } : pinkColor;
        }

        /* Soft pink glow behind the shape so it stays readable in the dark,
         * matching the treatment red enemies already get. */
        DrawCircleV(center, drawRadius * 1.8f, Fade(pinkColor, 0.18f));

        Vector2 top = { center.x, center.y - drawRadius };
        Vector2 right = { center.x + drawRadius, center.y };
        Vector2 bottom = { center.x, center.y + drawRadius };
        Vector2 left = { center.x - drawRadius, center.y };

        /* Fill first, then a solid pink core, then the white outline on
         * top last so the diamond always reads as pink, never hollow. */
        DrawTriangle(top, right, bottom, bodyColor);
        DrawTriangle(top, bottom, left, bodyColor);
        DrawCircleV(center, drawRadius * 0.4f, bodyColor);
        DrawLineEx(top, right, 2.0f, RAYWHITE);
        DrawLineEx(right, bottom, 2.0f, RAYWHITE);
        DrawLineEx(bottom, left, 2.0f, RAYWHITE);
        DrawLineEx(left, top, 2.0f, RAYWHITE);
    }

    if (bossEnemy.active)
    {
        Vector2 bossCenter = WorldToScreenPositionLayout(bossEnemy.position, layout);
        float bossRadius = fmaxf(bossEnemy.radius * scale, 8.0f);
        Vector2 bossPos = { bossCenter.x - bossRadius, bossCenter.y - bossRadius };
        Vector2 bossSize = { bossRadius * 2.0f, bossRadius * 2.0f };
        Color bossAura = Fade(VIOLET, 0.12f);
        Color bossBody = VIOLET;

        if (bossEnemy.knockoutTimer > 0.0f)
        {
            bossAura = ((int)(bossEnemy.knockoutTimer * 10.0f) % 2 == 0) ? Fade(SKYBLUE, 0.18f) : Fade(VIOLET, 0.05f);
            bossBody = ((int)(bossEnemy.knockoutTimer * 10.0f) % 2 == 0) ? SKYBLUE : VIOLET;
        }

        DrawCircleV(bossCenter, bossRadius * 2.5f, bossAura);
        Vector2 warningDirection = { 0 };
        if (bossEnemy.knockoutTimer <= 0.0f && IsEnemyAlignedWithPlayer(&bossEnemy, &warningDirection))
        {
            Vector2 warningEnd = {
                bossCenter.x + warningDirection.x * TILE_SIZE * layout.scale.x * 3.0f,
                bossCenter.y + warningDirection.y * TILE_SIZE * layout.scale.y * 3.0f
            };
            float pulse = 0.35f + 0.35f * sinf((float)GetTime() * 9.0f);
            DrawLineEx(bossCenter, warningEnd, fmaxf(2.0f, 3.0f * scale), Fade(SKYBLUE, pulse));
        }
        DrawRectangleV(bossPos, bossSize, bossBody);
        DrawRectangleLinesEx((Rectangle){ bossPos.x, bossPos.y, bossSize.x, bossSize.y }, 2.0f, RAYWHITE);
    }
}

void DrawBullets(void)
{
    MazeLayout layout = GetMazeLayout();
    float scale = layout.drawScale;

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPositionLayout(bullets[i].position, layout);
        float drawRadius = fmaxf(bullets[i].radius * scale, 3.0f);
        Color bulletColor = bullets[i].fromPlayer ? (Color){ 255, 190, 65, 255 } : SKYBLUE;

        if (bullets[i].fromBoss)
        {
            drawRadius = fmaxf(bullets[i].radius * scale * 1.55f, 5.0f);
            bulletColor = SKYBLUE;
            DrawCircleV(center, drawRadius * 1.8f, Fade(SKYBLUE, 0.22f));
        }

        DrawCircleV(center, drawRadius, bulletColor);
    }
}

void DrawParticles(void)
{
    MazeLayout layout = GetMazeLayout();

    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].timer <= 0.0f)
        {
            continue;
        }

        float alpha = particles[i].timer / particles[i].maxTimer;
        Vector2 center = WorldToScreenPositionLayout(particles[i].position, layout);
        DrawCircleV(center, fmaxf(1.4f, particles[i].radius * layout.drawScale), Fade(particles[i].color, alpha));
    }
}

void DrawVisibilityEffects(void)
{
    if (lightningRevealTimer > 0.0f)
    {
        float flashAlpha = 0.05f + 0.13f * fminf(1.0f, lightningRevealTimer / LIGHTNING_REVEAL_TIME);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RAYWHITE, flashAlpha));
    }

    if (!currentRoundConfig.flashlightEnabled)
    {
        return;
    }

    MazeLayout layout = GetMazeLayout();
    float scale = layout.drawScale;
    Vector2 playerCenter = WorldToScreenPositionLayout(player.position, layout);
    float playerAuraRadius = TILE_SIZE * scale * 2.9f * GetDarknessVisionScale();

    DrawCircleV(playerCenter, playerAuraRadius, Fade(GOLD, 0.07f));

    if (currentRoundConfig.flashlightEnabled && flashlightOn)
    {
        Vector2 flashlightCenter = WorldToScreenPositionLayout(GetFlashlightCenter(), layout);
        DrawCircleV(flashlightCenter, TILE_SIZE * scale * 4.8f * GetDarknessVisionScale(), Fade((Color){ 255, 245, 180, 255 }, 0.13f));
    }
}

void DrawTacticalMapOverlay(void)
{
    if (!tacticalMapOpen)
    {
        return;
    }

    if (!IsTacticalMapAvailable())
    {
        return;
    }

    float scale = GetUIScale();
    float rightWidth = GetHudRightPanelWidth();
    float rightX = (float)GetScreenWidth() - rightWidth - (10.0f * scale);
    Rectangle panel = {
        rightX,
        IsCompactLayout() ? ((float)GetScreenHeight() - 150.0f * scale) : (296.0f * scale),
        rightWidth,
        IsCompactLayout() ? (140.0f * scale) : fmaxf(180.0f * scale, fminf(470.0f * scale, (float)GetScreenHeight() - 306.0f * scale))
    };
    float cell = fminf((panel.width - 18.0f * scale) / (float)GRID_WIDTH,
                       (panel.height - 42.0f * scale) / (float)GRID_HEIGTH);
    cell = fmaxf(2.6f, cell);

    if (panel.y + panel.height > (float)GetScreenHeight() - 10.0f * scale)
    {
        panel.y = (float)GetScreenHeight() - panel.height - 10.0f * scale;
    }

    DrawRectangleRounded(panel, 0.06f, 8, (Color){ 3, 8, 24, 232 });
    DrawRectangleRoundedLinesEx(panel, 0.06f, 8, 1.5f * scale, Fade(HUD_BORDER_COLOR, 0.85f));
    DrawTextStrongFit("MAPA TAB", (int)(panel.x + 8.0f * scale), (int)(panel.y + 8.0f * scale),
                      ScaleFontSize(9.0f), 7, 0.0f, panel.width * 0.45f, HUD_BORDER_COLOR, BLACK);
    float mapBatteryFraction = fmaxf(0.0f, fminf(1.0f, mapBattery / MAP_MAX_BATTERY));
    DrawTextStrongFit(TextFormat("BAT %.0f%%", mapBatteryFraction * 100.0f),
                      (int)(panel.x + panel.width - 76.0f * scale), (int)(panel.y + 8.0f * scale),
                      ScaleFontSize(8.0f), 7, 0.0f, 68.0f * scale, RAYWHITE, BLACK);
    DrawSegmentedBar(panel.x + panel.width * 0.48f, panel.y + 25.0f * scale, panel.width * 0.44f, 5.0f * scale,
                     10, (int)ceilf(mapBatteryFraction * 10.0f),
                     (Color){ 120, 230, 255, 255 }, (Color){ 20, 42, 58, 255 }, scale);

    float mapWidth = GRID_WIDTH * cell;
    float mapHeight = GRID_HEIGTH * cell;
    Vector2 origin = { panel.x + (panel.width - mapWidth) * 0.5f, panel.y + 31.0f * scale + (panel.height - 42.0f * scale - mapHeight) * 0.5f };
    for (int y = 0; y < GRID_HEIGTH; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            Color color = (grid[y][x] == CELL_WALL) ? (Color){ 20, 24, 42, 255 } : Fade(MAZE_PATH_COLOR, 0.74f);
            if (grid[y][x] == CELL_EXIT) color = MAZE_EXIT_COLOR;
            if (grid[y][x] == CELL_LOCKED_EXIT) color = GOLD;
            if (grid[y][x] == CELL_KEY) color = GOLD;
            if (grid[y][x] == CELL_TRAP) color = (Color){ 255, 70, 90, 255 };
            DrawRectangleV((Vector2){ origin.x + (float)x * cell, origin.y + (float)y * cell },
                           (Vector2){ cell - 0.5f, cell - 0.5f }, color);
        }
    }

    int px = (int)(player.position.x / TILE_SIZE);
    int py = (int)(player.position.y / TILE_SIZE);
    DrawCircleV((Vector2){ origin.x + ((float)px + 0.5f) * cell, origin.y + ((float)py + 0.5f) * cell }, cell * 1.1f, GOLD);
}

void WrapTextToWidth(const char *source, int fontSize, float spacing, float maxWidth, char *destination, int destinationSize)
{
    int sourceLength = (int)strlen(source);
    int sourceIndex = 0;
    int destinationIndex = 0;

    destination[0] = '\0';

    while (sourceIndex < sourceLength && destinationIndex < destinationSize - 1)
    {
        char line[512] = { 0 };
        char candidate[512] = { 0 };
        int lineLength = 0;

        while (sourceIndex < sourceLength && source[sourceIndex] != '\n')
        {
            char word[160] = { 0 };
            int wordLength = 0;

            while (sourceIndex < sourceLength && source[sourceIndex] == ' ')
            {
                sourceIndex++;
            }

            int wordStart = sourceIndex;
            while (sourceIndex < sourceLength && source[sourceIndex] != ' ' && source[sourceIndex] != '\n' && wordLength < 159)
            {
                word[wordLength++] = source[sourceIndex++];
            }
            word[wordLength] = '\0';

            if (wordLength == 0)
            {
                break;
            }

            if (lineLength == 0)
            {
                snprintf(candidate, sizeof(candidate), "%s", word);
            }
            else
            {
                snprintf(candidate, sizeof(candidate), "%s %s", line, word);
            }

            if (MeasureTextStrongSpaced(candidate, fontSize, spacing).x > maxWidth)
            {
                sourceIndex = wordStart;
                break;
            }

            snprintf(line, sizeof(line), "%s", candidate);
            lineLength = (int)strlen(line);
        }

        if (lineLength == 0 && sourceIndex < sourceLength && source[sourceIndex] != '\n')
        {
            int start = sourceIndex;
            int lastFit = sourceIndex;

            while (sourceIndex < sourceLength && source[sourceIndex] != '\n' && source[sourceIndex] != ' ')
            {
                int codepointByteCount = 0;
                GetCodepointNext(&source[sourceIndex], &codepointByteCount);
                if (codepointByteCount <= 0)
                {
                    codepointByteCount = 1;
                }

                int nextIndex = sourceIndex + codepointByteCount;
                int candidateLength = nextIndex - start;
                if (candidateLength > 511)
                {
                    break;
                }

                char candidate[512] = { 0 };
                memcpy(candidate, &source[start], candidateLength);
                candidate[candidateLength] = '\0';

                if (lastFit > start && MeasureTextStrongSpaced(candidate, fontSize, spacing).x > maxWidth)
                {
                    break;
                }

                sourceIndex = nextIndex;
                lastFit = sourceIndex;
            }

            if (lastFit > start)
            {
                sourceIndex = lastFit;
            }

            lineLength = sourceIndex - start;
            if (lineLength > 511) lineLength = 511;
            memcpy(line, &source[start], lineLength);
            line[lineLength] = '\0';
        }

        int written = snprintf(&destination[destinationIndex], (size_t)(destinationSize - destinationIndex), "%s%s",
                               (destinationIndex > 0) ? "\n" : "", line);
        if (written < 0)
        {
            break;
        }
        destinationIndex += written;

        if (sourceIndex < sourceLength && source[sourceIndex] == '\n')
        {
            sourceIndex++;
        }
    }
}

Rectangle GetRoundPanelRect(const char *title, const char *body, const char *footer)
{
    (void)title;
    float scale = GetUIScale();
    int bodyFontSize = ScaleFontSize((currentLanguage == LANGUAGE_KO) ? 20.0f : 23.0f);
    int footerFontSize = ScaleFontSize(21.0f);
    float bodySpacing = ((currentLanguage == LANGUAGE_KO) ? 0.55f : 1.05f) * scale;
    float footerSpacing = 0.75f * scale;
    float maxPanelWidth = fminf((float)GetScreenWidth() - (32.0f * scale), 1080.0f * scale);
    float minPanelWidth = fminf(760.0f * scale, maxPanelWidth);
    float maxBodyWidth = maxPanelWidth - (56.0f * scale);
    char wrappedBody[2048];
    char wrappedFooter[512];
    WrapTextToWidth(body, bodyFontSize, bodySpacing, maxBodyWidth, wrappedBody, sizeof(wrappedBody));
    WrapTextToWidth(footer, footerFontSize, footerSpacing, maxBodyWidth, wrappedFooter, sizeof(wrappedFooter));
    Vector2 bodySize = MeasureTextStrongSpaced(wrappedBody, bodyFontSize, bodySpacing);
    Vector2 footerSize = MeasureTextStrongSpaced(wrappedFooter, footerFontSize, footerSpacing);
    float bodyTop = 104.0f * scale;
    float footerGap = 34.0f * scale;
    float bottomPadding = 30.0f * scale;
    float contentWidth = fmaxf(bodySize.x, footerSize.x);
    float panelWidth = fminf(maxPanelWidth, fmaxf(minPanelWidth, contentWidth + (56.0f * scale)));
    float panelHeight = fmaxf(320.0f * scale, bodyTop + bodySize.y + footerGap + footerSize.y + bottomPadding);

    return (Rectangle){
        (float)(GetScreenWidth() / 2) - panelWidth * 0.5f,
        fmaxf(70.0f * scale, (float)(GetScreenHeight() / 2) - panelHeight * 0.5f),
        panelWidth,
        panelHeight
    };
}

Rectangle GetTutorialSkipButtonRect(Rectangle panel)
{
    float scale = GetUIScale();
    int skipFontSize = ScaleFontSize(17.0f);
    int continueFontSize = ScaleFontSize(18.0f);
    float skipWidth = fmaxf(290.0f * scale, MeasureTextStrongSpaced(T(TEXT_SKIP_TUTORIAL), skipFontSize, 1.0f * scale).x + (48.0f * scale));
    float continueWidth = fmaxf(260.0f * scale, MeasureTextStrongSpaced(T(TEXT_CONTINUE_TUTORIAL), continueFontSize, 1.0f * scale).x + (82.0f * scale));
    float gap = 18.0f * scale;
    float totalWidth = skipWidth + gap + continueWidth;

    return (Rectangle){
        panel.x + (panel.width - totalWidth) * 0.5f,
        panel.y + panel.height + (16.0f * scale),
        skipWidth,
        50.0f * scale
    };
}

Rectangle GetContinueButtonRect(Rectangle panel)
{
    float scale = GetUIScale();
    int skipFontSize = ScaleFontSize(17.0f);
    int continueFontSize = ScaleFontSize(18.0f);
    float continueWidth = fmaxf(260.0f * scale, MeasureTextStrongSpaced(T(TEXT_CONTINUE_TUTORIAL), continueFontSize, 1.0f * scale).x + (82.0f * scale));

    if (!inTutorialSequence)
    {
        return (Rectangle){
            panel.x + (panel.width - continueWidth) * 0.5f,
            panel.y + panel.height + (16.0f * scale),
            continueWidth,
            50.0f * scale
        };
    }

    float skipWidth = fmaxf(290.0f * scale, MeasureTextStrongSpaced(T(TEXT_SKIP_TUTORIAL), skipFontSize, 1.0f * scale).x + (48.0f * scale));
    float gap = 18.0f * scale;
    float totalWidth = skipWidth + gap + continueWidth;

    return (Rectangle){
        panel.x + (panel.width - totalWidth) * 0.5f + skipWidth + gap,
        panel.y + panel.height + (16.0f * scale),
        continueWidth,
        50.0f * scale
    };
}

Rectangle GetOfficialRoundHistoryButtonRect(Rectangle panel)
{
    float scale = GetUIScale();
    float buttonWidth = fmaxf(260.0f * scale, MeasureTextStrongSpaced("ROUNDS FEITOS", ScaleFontSize(17.0f), 1.0f * scale).x + (54.0f * scale));
    float continueWidth = fmaxf(260.0f * scale, MeasureTextStrongSpaced(T(TEXT_CONTINUE_TUTORIAL), ScaleFontSize(18.0f), 1.0f * scale).x + (82.0f * scale));
    float gap = 18.0f * scale;
    float totalWidth = buttonWidth + gap + continueWidth;

    return (Rectangle){
        panel.x + (panel.width - totalWidth) * 0.5f,
        panel.y + panel.height + (16.0f * scale),
        buttonWidth,
        50.0f * scale
    };
}

void DrawButtonArrow(Rectangle button, Color color)
{
    float scale = GetUIScale();
    Vector2 start = { button.x + button.width - (38.0f * scale), button.y + button.height * 0.5f };
    Vector2 end = { button.x + button.width - (18.0f * scale), button.y + button.height * 0.5f };
    Vector2 top = { end.x - (7.0f * scale), end.y - (7.0f * scale) };
    Vector2 bottom = { end.x - (7.0f * scale), end.y + (7.0f * scale) };

    DrawLineEx(start, end, 3.0f * scale, color);
    DrawTriangle(end, top, bottom, color);
}

void DrawRoundHistoryButton(Rectangle button)
{
    float scale = GetUIScale();
    Vector2 mousePosition = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePosition, button);
    Color fill = hovered ? (Color){ 55, 72, 155, 255 } : (Color){ 17, 30, 78, 255 };
    int fontSize = FitFontSizeToWidth("ROUNDS FEITOS", ScaleFontSize(17.0f), ScaleFontSize(10.0f), 0.7f * scale, button.width - 40.0f * scale);

    DrawRectangleRounded(button, 0.28f, 10, fill);
    DrawRectangleRoundedLinesEx(button, 0.28f, 10, 2.0f, HUD_BORDER_COLOR);
    DrawTextStrongSpaced("ROUNDS FEITOS", (int)(button.x + 20.0f * scale), (int)(button.y + (button.height - fontSize) * 0.45f), fontSize, 0.7f * scale, RAYWHITE, BLACK);
}

void DrawContinueButton(Rectangle continueButton)
{
    float scale = GetUIScale();
    Vector2 mousePosition = GetMousePosition();
    bool continueHovered = CheckCollisionPointRec(mousePosition, continueButton);
    Color continueFill = continueHovered ? (Color){ 100, 235, 255, 255 } : (Color){ 24, 74, 170, 255 };
    int continueFontSize = FitFontSizeToWidth(T(TEXT_CONTINUE_TUTORIAL), ScaleFontSize(18.0f), ScaleFontSize(10.0f), 0.7f * scale, continueButton.width - 62.0f * scale);

    DrawRectangleRounded(continueButton, 0.28f, 10, continueFill);
    DrawRectangleRoundedLinesEx(continueButton, 0.28f, 10, 2.0f, HUD_BORDER_COLOR);
    DrawTextStrongSpaced(T(TEXT_CONTINUE_TUTORIAL), (int)(continueButton.x + 20.0f * scale), (int)(continueButton.y + (continueButton.height - continueFontSize) * 0.45f), continueFontSize, 0.7f * scale, BLACK, Fade(WHITE, 0.25f));
    DrawButtonArrow(continueButton, BLACK);
}

void DrawPanelButtons(Rectangle panel)
{
    Rectangle continueButton = GetContinueButtonRect(panel);

    if (!inTutorialSequence)
    {
        if (officialRound > 1)
        {
            Rectangle historyButton = GetOfficialRoundHistoryButtonRect(panel);
            continueButton.x = historyButton.x + historyButton.width + (18.0f * GetUIScale());
            DrawRoundHistoryButton(historyButton);
        }

        DrawContinueButton(continueButton);
        return;
    }

    Rectangle skipButton = GetTutorialSkipButtonRect(panel);
    float scale = GetUIScale();
    Vector2 mousePosition = GetMousePosition();
    bool skipHovered = CheckCollisionPointRec(mousePosition, skipButton);
    Color skipFill = skipHovered ? (Color){ 55, 72, 155, 255 } : (Color){ 17, 30, 78, 255 };
    int skipFontSize = FitFontSizeToWidth(T(TEXT_SKIP_TUTORIAL), ScaleFontSize(17.0f), ScaleFontSize(10.0f), 0.7f * scale, skipButton.width - 40.0f * scale);

    DrawRectangleRounded(skipButton, 0.28f, 10, skipFill);
    DrawRectangleRoundedLinesEx(skipButton, 0.28f, 10, 2.0f, HUD_BORDER_COLOR);
    DrawTextStrongSpaced(T(TEXT_SKIP_TUTORIAL), (int)(skipButton.x + 20.0f * scale), (int)(skipButton.y + (skipButton.height - skipFontSize) * 0.45f), skipFontSize, 0.7f * scale, RAYWHITE, BLACK);
    DrawContinueButton(continueButton);
}

void DrawRoundPanel(const char *title, const char *body, const char *footer)
{
    if (victoryRoundsOpen && !inTutorialSequence)
    {
        DrawRoundHistoryPanel();
        return;
    }

    /* Body text uses extra letter spacing for legibility, and the panel
     * height grows to fit however many lines the explanation needs, so
     * longer tutorial text never gets clipped or cramped. */
    float scale = GetUIScale();
    int bodyFontSize = ScaleFontSize((currentLanguage == LANGUAGE_KO) ? 20.0f : 23.0f);
    float bodySpacing = ((currentLanguage == LANGUAGE_KO) ? 0.55f : 1.05f) * scale;
    float footerSpacing = 0.75f * scale;
    float bodyTop = 104.0f * scale;
    float footerGap = 34.0f * scale;
    float maxPanelWidth = fminf((float)GetScreenWidth() - (32.0f * scale), 1080.0f * scale);
    float maxBodyWidth = maxPanelWidth - (56.0f * scale);
    char wrappedBody[2048];
    char wrappedFooter[512];
    WrapTextToWidth(body, bodyFontSize, bodySpacing, maxBodyWidth, wrappedBody, sizeof(wrappedBody));
    WrapTextToWidth(footer, ScaleFontSize(21.0f), footerSpacing, maxBodyWidth, wrappedFooter, sizeof(wrappedFooter));

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int titleFontSize = ScaleFontSize(34.0f);
    int footerFontSize = ScaleFontSize(21.0f);
    Vector2 bodySize = MeasureTextStrongSpaced(wrappedBody, bodyFontSize, bodySpacing);
    Rectangle panel = GetRoundPanelRect(title, body, footer);
    titleFontSize = FitFontSizeToWidth(title, titleFontSize, ScaleFontSize(18.0f), 0.8f * scale, panel.width - 56.0f * scale);
    int titleWidth = (int)MeasureTextStrongSpaced(title, titleFontSize, 0.8f * scale).x;

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.78f));
    DrawRectangleRounded(panel, 0.08f, 12, (Color){ 6, 12, 36, 245 });
    DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 3.0f * scale, HUD_BORDER_COLOR);
    DrawTextStrongSpaced(title, (int)(panel.x + (panel.width - titleWidth) * 0.5f), (int)(panel.y + 26.0f * scale), titleFontSize, 0.8f * scale, HUD_BORDER_COLOR, BLACK);
    DrawTextStrongSpaced(wrappedBody, (int)(panel.x + 28.0f * scale), (int)(panel.y + bodyTop), bodyFontSize, bodySpacing, RAYWHITE, BLACK);
    DrawTextStrongSpaced(wrappedFooter, (int)(panel.x + 28.0f * scale), (int)(panel.y + bodyTop + bodySize.y + footerGap), footerFontSize, footerSpacing, LIGHTGRAY, BLACK);

    DrawPanelButtons(panel);
}

bool HandlePanelButtons(void)
{
    if ((gamePhase != PHASE_INTRO && gamePhase != PHASE_INFO) || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return false;
    }

    const char *body = (gamePhase == PHASE_INTRO) ? GetIntroBody() : GetRoundInfoBody();
    const char *footer = (gamePhase == PHASE_INTRO) ? T(TEXT_INTRO_FOOTER) : T(TEXT_ROUND_FOOTER);
    Rectangle panel = GetRoundPanelRect((gamePhase == PHASE_INTRO) ? GetIntroTitle() : GetRoundInfoTitle(), body, footer);
    Rectangle skipButton = GetTutorialSkipButtonRect(panel);
    Rectangle continueButton = GetContinueButtonRect(panel);
    Rectangle historyButton = GetOfficialRoundHistoryButtonRect(panel);
    Vector2 mousePosition = GetMousePosition();

    if (victoryRoundsOpen && !inTutorialSequence)
    {
        Rectangle historyPanel = GetRoundHistoryPanelRect();
        int selectedRound = GetClickedHistoryRound(mousePosition);

        if (selectedRound > 0)
        {
            officialRound = selectedRound;
            victoryRoundsOpen = false;
            StartCurrentStage();
            return true;
        }

        if (CheckCollisionPointRec(mousePosition, GetVictoryRoundsButtonRect(historyPanel)))
        {
            victoryRoundsOpen = false;
            return true;
        }

        if (CheckCollisionPointRec(mousePosition, GetVictoryNextButtonRect(historyPanel)))
        {
            victoryRoundsOpen = false;
            gamePhase = PHASE_PLAYING;
            return true;
        }

        return false;
    }

    if (inTutorialSequence && CheckCollisionPointRec(mousePosition, skipButton))
    {
        inTutorialSequence = false;
        tutorialRound = 1;
        officialRound = 1;
        bestOfficialRound = 1;
        ResetOfficialRunStats();
        SaveProgress();
        StartCurrentStage();
        return true;
    }

    if (!inTutorialSequence && officialRound > 1)
    {
        continueButton.x = historyButton.x + historyButton.width + (18.0f * GetUIScale());

        if (CheckCollisionPointRec(mousePosition, historyButton))
        {
            victoryRoundsOpen = true;
            return true;
        }
    }

    if (CheckCollisionPointRec(mousePosition, continueButton))
    {
        if (gamePhase == PHASE_INTRO)
        {
            tutorialRound = 1;
            inTutorialSequence = true;
            gamePhase = PHASE_INFO;
        }
        else
        {
            gamePhase = PHASE_PLAYING;
        }

        return true;
    }

    return false;
}

int ScaleShopFontSize(float fontSize)
{
    return ScaleFontSize(fontSize);
}

Rectangle GetShopPanelRect(void)
{
    float scale = GetUIScale();
    float panelWidth = fminf((float)GetScreenWidth() - (56.0f * scale), 1040.0f * scale);
    float panelHeight = fminf((float)GetScreenHeight() - (112.0f * scale), 560.0f * scale);
    panelHeight = fmaxf(panelHeight, 470.0f * scale);

    return (Rectangle){
        ((float)GetScreenWidth() - panelWidth) * 0.5f,
        fmaxf(56.0f * scale, ((float)GetScreenHeight() - panelHeight) * 0.5f),
        panelWidth,
        panelHeight
    };
}

int GetShopColumnCount(void)
{
    Rectangle panel = GetShopPanelRect();

    return panel.width >= 980.0f * GetUIScale() ? 4 : 2;
}

Rectangle GetShopCardRect(int index)
{
    Rectangle panel = GetShopPanelRect();
    float scale = GetUIScale();
    float pad = 30.0f * scale;
    float headerHeight = 150.0f * scale;
    float footerHeight = 112.0f * scale;
    float gap = 16.0f * scale;
    int columns = GetShopColumnCount();
    int row = index / columns;
    int column = index % columns;
    int rows = (columns == 4) ? 1 : 2;
    float contentWidth = panel.width - (pad * 2.0f);
    float contentHeight = panel.height - headerHeight - footerHeight;
    float cardWidth = (contentWidth - gap * (float)(columns - 1)) / (float)columns;
    float cardHeight = (contentHeight - gap * (float)(rows - 1)) / (float)rows;

    return (Rectangle){
        panel.x + pad + (float)column * (cardWidth + gap),
        panel.y + headerHeight + (float)row * (cardHeight + gap),
        cardWidth,
        cardHeight
    };
}

Rectangle GetShopBuyButtonRect(int index)
{
    Rectangle card = GetShopCardRect(index);
    float scale = GetUIScale();
    return (Rectangle){
        card.x + 18.0f * scale,
        card.y + card.height - 60.0f * scale,
        card.width - 36.0f * scale,
        42.0f * scale
    };
}

Rectangle GetShopStartButtonRect(void)
{
    Rectangle panel = GetShopPanelRect();
    float scale = GetUIScale();
    float width = fminf(300.0f * scale, panel.width - 56.0f * scale);

    return (Rectangle){
        panel.x + (panel.width - width) * 0.5f,
        panel.y + panel.height - 78.0f * scale,
        width,
        46.0f * scale
    };
}

Rectangle GetShopButtonRect(int index)
{
    if (index == 4)
    {
        return GetShopStartButtonRect();
    }

    return GetShopBuyButtonRect(index);
}

bool IsShopBatteryAvailable(void)
{
    return !inTutorialSequence &&
           (flashlightBattery < FLASHLIGHT_MAX_BATTERY || mapBattery < MAP_MAX_BATTERY);
}

const char *GetShopBuyLabel(void)
{
    if (currentLanguage == LANGUAGE_EN) return "BUY";
    if (currentLanguage == LANGUAGE_KO) return "구매";
    return "COMPRAR";
}

const char *GetShopItemDescription(int index)
{
    if (currentLanguage == LANGUAGE_EN)
    {
        if (index == 0) return "Buy +30 ammo";
        if (index == 1) return "Max health +5";
        if (index == 2) return "Reveal map for 3s";
        return "Tool batteries +50%";
    }

    if (currentLanguage == LANGUAGE_ES)
    {
        if (index == 0) return "Compra +30 municion";
        if (index == 1) return "Vida max +5";
        if (index == 2) return "Revela mapa 3s";
        return "Baterias +50%";
    }

    if (currentLanguage == LANGUAGE_KO)
    {
        if (index == 0) return "최대 탄약 +30";
        if (index == 1) return "최대 체력 +5";
        if (index == 2) return "3초 동안 지도를 밝게 합니다";
        return "도구 배터리 +50%";
    }

    if (index == 0) return "Compra +30 municao";
    if (index == 1) return "Vida max +5";
    if (index == 2) return "Revela mapa 3s";
    return "Baterias +50%";
}

void DrawShopItemIcon(int index, Vector2 center, float radius, Color accent)
{
    Rectangle iconBack = {
        center.x - radius * 1.8f,
        center.y - radius * 1.05f,
        radius * 3.6f,
        radius * 2.1f
    };

    DrawRectangleRounded(iconBack, 0.18f, 8, (Color){ 3, 10, 26, 255 });
    DrawRectangleRoundedLinesEx(iconBack, 0.18f, 8, 1.0f, Fade(accent, 0.35f));

    if (index == 0)
    {
        DrawRectangleLinesEx((Rectangle){ center.x - radius * 0.55f, center.y - radius * 0.45f, radius * 1.1f, radius * 0.9f }, 2.0f, accent);
        DrawLineEx((Vector2){ center.x - radius * 0.55f, center.y - radius * 0.45f }, (Vector2){ center.x, center.y - radius * 0.78f }, 2.0f, accent);
        DrawLineEx((Vector2){ center.x + radius * 0.55f, center.y - radius * 0.45f }, (Vector2){ center.x, center.y - radius * 0.78f }, 2.0f, accent);
        DrawLineEx((Vector2){ center.x, center.y + radius * 0.78f }, (Vector2){ center.x - radius * 0.55f, center.y + radius * 0.45f }, 2.0f, accent);
        DrawLineEx((Vector2){ center.x, center.y + radius * 0.78f }, (Vector2){ center.x + radius * 0.55f, center.y + radius * 0.45f }, 2.0f, accent);
    }
    else if (index == 1)
    {
        DrawCircleV((Vector2){ center.x - radius * 0.32f, center.y - radius * 0.25f }, radius * 0.42f, Fade(accent, 0.85f));
        DrawCircleV((Vector2){ center.x + radius * 0.32f, center.y - radius * 0.25f }, radius * 0.42f, Fade(accent, 0.85f));
        DrawTriangle((Vector2){ center.x - radius * 0.75f, center.y - radius * 0.05f },
                     (Vector2){ center.x + radius * 0.75f, center.y - radius * 0.05f },
                     (Vector2){ center.x, center.y + radius * 0.82f }, Fade(accent, 0.85f));
        DrawLineEx((Vector2){ center.x - radius * 0.55f, center.y }, (Vector2){ center.x - radius * 0.1f, center.y }, 2.0f, BLACK);
        DrawLineEx((Vector2){ center.x - radius * 0.1f, center.y }, (Vector2){ center.x + radius * 0.05f, center.y - radius * 0.32f }, 2.0f, BLACK);
        DrawLineEx((Vector2){ center.x + radius * 0.05f, center.y - radius * 0.32f }, (Vector2){ center.x + radius * 0.2f, center.y + radius * 0.18f }, 2.0f, BLACK);
        DrawLineEx((Vector2){ center.x + radius * 0.2f, center.y + radius * 0.18f }, (Vector2){ center.x + radius * 0.58f, center.y + radius * 0.18f }, 2.0f, BLACK);
    }
    else if (index == 2)
    {
        Vector2 points[6] = {
            { center.x + radius * 0.15f, center.y - radius * 0.92f },
            { center.x - radius * 0.62f, center.y + radius * 0.1f },
            { center.x - radius * 0.05f, center.y + radius * 0.1f },
            { center.x - radius * 0.24f, center.y + radius * 0.92f },
            { center.x + radius * 0.68f, center.y - radius * 0.22f },
            { center.x + radius * 0.08f, center.y - radius * 0.22f }
        };
        for (int i = 0; i < 5; i++) DrawLineEx(points[i], points[i + 1], 3.0f, accent);
    }
    else
    {
        DrawRectangleRoundedLinesEx((Rectangle){ center.x - radius * 0.65f, center.y - radius * 0.48f, radius * 1.15f, radius * 0.96f }, 0.2f, 6, 2.0f, accent);
        DrawRectangleRec((Rectangle){ center.x + radius * 0.58f, center.y - radius * 0.18f, radius * 0.18f, radius * 0.36f }, accent);
        Vector2 points[6] = {
            { center.x + radius * 0.02f, center.y - radius * 0.38f },
            { center.x - radius * 0.3f, center.y + radius * 0.05f },
            { center.x - radius * 0.02f, center.y + radius * 0.05f },
            { center.x - radius * 0.16f, center.y + radius * 0.43f },
            { center.x + radius * 0.33f, center.y - radius * 0.08f },
            { center.x + radius * 0.08f, center.y - radius * 0.08f }
        };
        for (int i = 0; i < 5; i++) DrawLineEx(points[i], points[i + 1], 2.0f, accent);
    }
}

void DrawShopPricePill(Rectangle card, const char *detail, bool enabled, Color accent)
{
    float scale = GetUIScale();
    Rectangle pill = { card.x + 18.0f * scale, card.y + card.height - 104.0f * scale, 146.0f * scale, 36.0f * scale };
    int priceSize = ScaleShopFontSize(11.0f);

    DrawRectangleRounded(pill, 0.22f, 8, enabled ? (Color){ 55, 4, 75, 255 } : (Color){ 22, 22, 38, 255 });
    DrawRectangleRoundedLinesEx(pill, 0.22f, 8, 1.0f * scale, enabled ? MAGENTA : Fade(LIGHTGRAY, 0.3f));
    DrawCircleV((Vector2){ pill.x + 14.0f * scale, pill.y + pill.height * 0.5f }, 4.0f * scale, GOLD);
    DrawCircleV((Vector2){ pill.x + 14.0f * scale, pill.y + pill.height * 0.5f }, 2.0f * scale, (Color){ 55, 4, 75, 255 });
    DrawTextStrongFit(detail, (int)(pill.x + 28.0f * scale), (int)(pill.y + 10.0f * scale),
                      priceSize, ScaleShopFontSize(9.0f), 0.0f, pill.width - 42.0f * scale,
                      enabled ? RAYWHITE : Fade(LIGHTGRAY, 0.6f), BLACK);
}

void DrawShopButton(int index, const char *label, const char *detail, bool enabled, Color accent)
{
    float scale = GetUIScale();
    Rectangle card = GetShopCardRect(index);
    Rectangle buyButton = GetShopBuyButtonRect(index);
    Vector2 mousePosition = GetMousePosition();
    bool hovered = enabled && CheckCollisionPointRec(mousePosition, buyButton);
    Color fill = enabled ? (hovered ? (Color){ 20, 235, 250, 255 } : (Color){ 18, 215, 235, 255 }) : (Color){ 24, 28, 42, 255 };
    Color border = enabled ? accent : Fade(LIGHTGRAY, 0.25f);
    int labelSize = ScaleShopFontSize(18.0f);
    int descriptionSize = ScaleShopFontSize(12.0f);
    int buySize = ScaleShopFontSize(12.0f);
    float iconY = card.y + 54.0f * scale;

    if (card.height < 190.0f * scale)
    {
        iconY = card.y + 34.0f * scale;
    }

    DrawRectangleRounded(card, 0.03f, 8, (Color){ 8, 12, 32, 248 });
    DrawRectangleRoundedLinesEx((Rectangle){ card.x - 2.0f, card.y - 2.0f, card.width + 4.0f, card.height + 4.0f }, 0.03f, 8, 4.0f * scale, Fade(accent, 0.12f));
    DrawRectangleRoundedLinesEx(card, 0.03f, 8, 1.5f * scale, border);

    DrawShopItemIcon(index, (Vector2){ card.x + card.width * 0.5f, iconY }, 19.0f * scale, accent);
    DrawTextStrongFit(label, (int)(card.x + 18.0f * scale), (int)(iconY + 48.0f * scale),
                      labelSize, ScaleShopFontSize(13.0f), 0.0f,
                      card.width - 36.0f * scale,
                      enabled ? RAYWHITE : Fade(LIGHTGRAY, 0.6f), BLACK);
    DrawTextStrongFit(GetShopItemDescription(index), (int)(card.x + 18.0f * scale), (int)(iconY + 82.0f * scale),
                      descriptionSize, ScaleShopFontSize(10.0f), 0.0f,
                      card.width - 36.0f * scale,
                      enabled ? (Color){ 190, 205, 230, 255 } : Fade(LIGHTGRAY, 0.65f), BLACK);

    DrawShopPricePill(card, detail, enabled, accent);
    DrawRectangleRounded(buyButton, 0.13f, 8, fill);
    DrawRectangleRoundedLinesEx(buyButton, 0.13f, 8, 1.0f * scale, Fade(RAYWHITE, hovered ? 0.8f : 0.25f));
    DrawTextStrongFit(GetShopBuyLabel(), (int)(buyButton.x + 12.0f * scale), (int)(buyButton.y + 9.0f * scale),
                      buySize, ScaleShopFontSize(10.0f), 0.0f,
                      buyButton.width - 24.0f * scale, enabled ? BLACK : Fade(LIGHTGRAY, 0.65f), Fade(WHITE, 0.2f));
}

void DrawShopPanel(void)
{
    float scale = GetUIScale();
    Rectangle panel = GetShopPanelRect();
    Rectangle coinPill = { panel.x + panel.width - 180.0f * scale, panel.y + 34.0f * scale, 148.0f * scale, 34.0f * scale };
    int smallSize = ScaleShopFontSize(9.0f);
    int titleSize = ScaleShopFontSize(32.0f);
    int coinsSize = ScaleShopFontSize(11.0f);
    bool batteryAvailable = IsShopBatteryAvailable();
    bool healthCanGrow = playerMaxHealth < PLAYER_MAX_HEALTH;
    bool ammoCanGrow = playerTotalAmmo < PLAYER_AMMO_CAP;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.78f));
    DrawRectangleRounded(panel, 0.08f, 12, (Color){ 6, 12, 36, 245 });
    DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 3.0f * scale, HUD_BORDER_COLOR);

    DrawCircleV((Vector2){ panel.x + 30.0f * scale, panel.y + 40.0f * scale }, 3.5f * scale, (Color){ 0, 240, 255, 255 });
    DrawTextStrongSpaced("SYSTEM: ONLINE", (int)(panel.x + 42.0f * scale), (int)(panel.y + 34.0f * scale),
                         smallSize, 0.0f, (Color){ 0, 245, 255, 255 }, BLACK);
    DrawTextStrongSpaced("SECTOR_SHOP_V2.4", (int)(panel.x + panel.width - 172.0f * scale), (int)(panel.y + 82.0f * scale),
                         smallSize, 0.0f, (Color){ 160, 175, 205, 255 }, BLACK);
    DrawTextStrongSpaced("BYTEMAZE // TERMINAL", (int)(panel.x + 30.0f * scale), (int)(panel.y + 82.0f * scale),
                         smallSize, 0.0f, (Color){ 0, 245, 255, 255 }, BLACK);
    DrawTextStrongSpaced(T(TEXT_SHOP_TITLE), (int)(panel.x + 30.0f * scale), (int)(panel.y + 104.0f * scale),
                         titleSize, 1.0f * scale, RAYWHITE, MAGENTA);
    DrawTextStrongFit(TextFormat("EVOLUCAO NIVEL %d  XP %d/%d", playerLevel, playerXp, GetEvolutionXpToNextLevel()),
                      (int)(panel.x + 250.0f * scale), (int)(panel.y + 116.0f * scale),
                      ScaleShopFontSize(10.0f), ScaleShopFontSize(8.0f), 0.0f,
                      panel.width - 460.0f * scale, (Color){ 120, 230, 255, 255 }, BLACK);
    DrawTextStrongFit(TextFormat("PROX ROUND %d  RECORDE %d  MOD %s", officialRound, bestOfficialRound,
                                 (officialRound >= MODIFIER_START_ROUND) ? FormatModifierSummary(GetRoundModifiers(officialRound)) : "PADRAO"),
                      (int)(panel.x + 250.0f * scale), (int)(panel.y + 132.0f * scale),
                      ScaleShopFontSize(9.0f), ScaleShopFontSize(7.0f), 0.0f,
                      panel.width - 460.0f * scale, GOLD, BLACK);

    DrawRectangleRounded(coinPill, 0.2f, 8, (Color){ 35, 30, 0, 255 });
    DrawRectangleRoundedLinesEx(coinPill, 0.2f, 8, 1.5f * scale, GOLD);
    DrawCircleV((Vector2){ coinPill.x + 17.0f * scale, coinPill.y + coinPill.height * 0.5f }, 5.5f * scale, GOLD);
    DrawCircleV((Vector2){ coinPill.x + 17.0f * scale, coinPill.y + coinPill.height * 0.5f }, 3.0f * scale, (Color){ 35, 30, 0, 255 });
    DrawTextStrongFit(TextFormat(T(TEXT_SHOP_COINS), playerCoins), (int)(coinPill.x + 30.0f * scale), (int)(coinPill.y + 10.0f * scale),
                      coinsSize, ScaleShopFontSize(9.0f), 0.0f, coinPill.width - 46.0f * scale, RAYWHITE, BLACK);

    DrawRectangleRec((Rectangle){ panel.x + 30.0f * scale, panel.y + 144.0f * scale, panel.width - 60.0f * scale, 4.0f * scale }, MAGENTA);

    DrawShopButton(0, T(TEXT_SHOP_BUY_AMMO), ammoCanGrow ? TextFormat("%d PTS", SHOP_STANDARD_PRICE) : T(TEXT_SHOP_MAX),
                   ammoCanGrow && playerCoins >= SHOP_STANDARD_PRICE, HUD_BORDER_COLOR);
    DrawShopButton(1, T(TEXT_SHOP_BUY_HEALTH), healthCanGrow ? TextFormat("%d PTS", SHOP_STANDARD_PRICE) : T(TEXT_SHOP_MAX),
                   healthCanGrow && playerCoins >= SHOP_STANDARD_PRICE, (Color){ 190, 70, 255, 255 });
    DrawShopButton(2, T(TEXT_SHOP_BUY_LIGHTNING), IsLightningAvailableThisRound() ? TextFormat("%d PTS", SHOP_LIGHTNING_PRICE) : T(TEXT_SHOP_LOCKED),
                   IsLightningAvailableThisRound() && playerCoins >= SHOP_LIGHTNING_PRICE, (Color){ 120, 230, 255, 255 });
    DrawShopButton(3, T(TEXT_SHOP_BUY_BATTERY), batteryAvailable ? TextFormat("%d PTS", SHOP_STANDARD_PRICE) : (inTutorialSequence ? T(TEXT_SHOP_LOCKED) : T(TEXT_SHOP_MAX)),
                   batteryAvailable && playerCoins >= SHOP_STANDARD_PRICE, (Color){ 255, 205, 60, 255 });

    Rectangle startButton = GetShopStartButtonRect();
    Vector2 mousePosition = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePosition, startButton);
    DrawRectangleRounded(startButton, 0.18f, 10, hovered ? (Color){ 70, 18, 115, 255 } : (Color){ 16, 10, 44, 255 });
    DrawRectangleRoundedLinesEx(startButton, 0.18f, 10, 1.5f * scale, MAGENTA);
    DrawTextStrongFit(T(TEXT_SHOP_START), (int)(startButton.x + 16.0f * scale), (int)(startButton.y + 14.0f * scale),
                      ScaleShopFontSize(13.0f), ScaleShopFontSize(10.0f), 0.0f,
                      startButton.width - 32.0f * scale, MAGENTA, BLACK);
}

bool HandleShopButtons(void)
{
    if (gamePhase != PHASE_SHOP || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        return false;
    }

    Vector2 mousePosition = GetMousePosition();

    if (CheckCollisionPointRec(mousePosition, GetShopButtonRect(4)))
    {
        gamePhase = ShouldShowRoundInfo() ? PHASE_INFO : PHASE_PLAYING;
        return true;
    }

    if (CheckCollisionPointRec(mousePosition, GetShopButtonRect(0)) && playerTotalAmmo < PLAYER_AMMO_CAP && playerCoins >= SHOP_STANDARD_PRICE)
    {
        playerCoins -= SHOP_STANDARD_PRICE;
        playerTotalAmmo += SHOP_AMMO_GAIN;
        ClampProgressState();
        SaveProgress();
        return true;
    }

    if (CheckCollisionPointRec(mousePosition, GetShopButtonRect(1)) && playerMaxHealth < PLAYER_MAX_HEALTH && playerCoins >= SHOP_STANDARD_PRICE)
    {
        playerCoins -= SHOP_STANDARD_PRICE;
        playerMaxHealth += SHOP_HEALTH_GAIN;
        if (playerMaxHealth > PLAYER_MAX_HEALTH)
        {
            playerMaxHealth = PLAYER_MAX_HEALTH;
        }
        playerHealth = playerMaxHealth;
        SaveProgress();
        return true;
    }

    if (CheckCollisionPointRec(mousePosition, GetShopButtonRect(2)) && IsLightningAvailableThisRound() && playerCoins >= SHOP_LIGHTNING_PRICE)
    {
        playerCoins -= SHOP_LIGHTNING_PRICE;
        lightningCharges++;
        SaveProgress();
        return true;
    }

    if (CheckCollisionPointRec(mousePosition, GetShopButtonRect(3)) && IsShopBatteryAvailable() && playerCoins >= SHOP_STANDARD_PRICE)
    {
        playerCoins -= SHOP_STANDARD_PRICE;
        flashlightBattery += SHOP_BATTERY_GAIN;
        mapBattery += SHOP_BATTERY_GAIN;
        if (flashlightBattery > FLASHLIGHT_MAX_BATTERY)
        {
            flashlightBattery = FLASHLIGHT_MAX_BATTERY;
        }
        if (mapBattery > MAP_MAX_BATTERY)
        {
            mapBattery = MAP_MAX_BATTERY;
        }
        SaveProgress();
        return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    (void)argc;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ByteMaze");
    InitUIFont();
    InitGameAudio();
    MaximizeWindow();
    SetTargetFPS(60);
    SetRandomSeed((unsigned int)GetTime());
    UpdateBuildMetrics(argv[0]);
    LoadProgress();

    while (!WindowShouldClose())
    {
        UpdateBuildMetrics(argv[0]);
        UpdateGameAudio();
        HandleLanguageButtons();
        HandlePanelButtons();
        HandleShopButtons();
        HandleVictoryButton();

        if (gamePhase == PHASE_INTRO)
        {
            /* Intro advances only through the tutorial buttons. */
        }
        else if (gamePhase == PHASE_INFO)
        {
            if (roundNeedsSetup)
            {
                SetupRound();
            }

        }
        else if (gamePhase == PHASE_SHOP)
        {
            /* Purchases happen before the round is generated. */
        }
        else if (gamePhase == PHASE_PLAYING)
        {
            if (roundNeedsSetup)
            {
                SetupRound();
            }

            if (playerAlive)
            {
                if (waitingForVictorySound)
                {
                    /* Victory waits on the completion overlay button now. */
                }
                else
                {
                    if (playerDamageCooldown > 0.0f)
                    {
                        playerDamageCooldown -= GetFrameTime();
                        if (playerDamageCooldown < 0.0f)
                        {
                            playerDamageCooldown = 0.0f;
                        }
                    }

                    UpdatePlayer();
                    UpdateTacticalMapBattery();
                    CollectCurrentCellPickup();
                    UpdateFloatingNotices();
                    UpdateParticles();
                    UpdatePlayerShooting();
                    UpdatePlayerReload();
                    UpdateFlashlight();
                    UpdateLightningReveal();
                    for (int i = 0; i < RED_ENEMY_COUNT; i++)
                    {
                        UpdateRedEnemy(&redEnemies[i]);
                    }
                    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
                    {
                        UpdateBlueEnemy(&blueEnemies[i]);
                    }
                    UpdateBossEnemy();
                    UpdateBullets();
                    ApplyEnemyTouchDamage();

                    if (playerAlive && DidPlayerReachExit())
                    {
                        FinishRoundRewards();
                        SpawnParticleBurst(player.position, MAZE_EXIT_COLOR, 46, 150.0f, 3.5f);
                        if (gameAudioLoaded)
                        {
                            PlaySound(victorySound);
                        }

                        waitingForVictorySound = true;
                    }
                }
            }
        }

        BeginDrawing();
        DrawDashboardBackground();
        DrawDashboardHeader();

        if (gamePhase == PHASE_PLAYING)
        {
            DrawMazeGrid();
            DrawVisibilityEffects();
            DrawFloatingNotices();
            DrawPlayer();
            DrawEnemies();
            DrawBullets();
            DrawParticles();
            DrawHud();
            DrawTacticalMapOverlay();
        }

        if (gamePhase == PHASE_PLAYING && !playerAlive)
        {
            DrawGameOverOverlay();
        }

        if (gamePhase == PHASE_PLAYING && playerAlive && waitingForVictorySound)
        {
            DrawVictoryOverlay();
        }

        if (gamePhase == PHASE_INTRO)
        {
            DrawRoundPanel(GetIntroTitle(), GetIntroBody(), T(TEXT_INTRO_FOOTER));
        }
        else if (gamePhase == PHASE_SHOP)
        {
            DrawShopPanel();
        }
        else if (gamePhase == PHASE_INFO)
        {
            DrawRoundPanel(GetRoundInfoTitle(), GetRoundInfoBody(), T(TEXT_ROUND_FOOTER));
        }

        if (gamePhase != PHASE_SHOP)
        {
            DrawLanguageButtons();
        }

        EndDrawing();
    }

    ShutdownGameAudio();
    ShutdownUIFont();
    CloseWindow();
    return 0;
}
