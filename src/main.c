#include "raylib.h"
#include "math.h"
#include <stdlib.h>
#include <sys/stat.h>

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 640

#define GRID_HEIGTH 21
#define GRID_WIDTH 31
#define TILE_SIZE 24
#define HUD_HEIGHT 56.0f
#define MAZE_PADDING 24.0f
#define CONTEST_BYTE_LIMIT 1474560LL

#define CELL_WALL 0
#define CELL_PATH 1
#define CELL_EXIT 2
#define RED_ENEMY_COUNT 3
#define BLUE_ENEMY_COUNT 2
#define MAX_BULLETS 48
#define FLASHLIGHT_MAX_BATTERY 100.0f
#define PLAYER_BULLET_SPEED 220.0f
#define BLUE_BULLET_SPEED 140.0f
#define BOSS_BULLET_SPEED 150.0f
#define BULLET_RADIUS 4.0f
#define RED_HIT_LIMIT 5
#define BLUE_HIT_LIMIT 5
#define BLUE_KNOCKOUT_TIME 5.0f
#define BOSS_HIT_LIMIT 5
#define BOSS_KNOCKOUT_TIME 3.0f
#define PLAYER_MAX_HEALTH 100
#define PLAYER_MAX_AMMO 20
#define PLAYER_RELOAD_TIME 1.5f
#define BLUE_BULLET_DAMAGE 10
#define BOSS_FAR_DISTANCE_THRESHOLD (TILE_SIZE * 10.0f)
#define BOSS_FAR_SPEED_MULTIPLIER 1.8f

int grid[GRID_HEIGTH][GRID_WIDTH];
long long executableSizeBytes = -1;
float executableUsagePercent = 0.0f;
bool flashlightOn = false;
float flashlightBattery = FLASHLIGHT_MAX_BATTERY;
int playerHealth = PLAYER_MAX_HEALTH;
int playerAmmo = PLAYER_MAX_AMMO;
float playerReloadTimer = 0.0f;

Color HUD_PANEL_COLOR = { 10, 10, 10, 220 };
Color HUD_BORDER_COLOR = { 0, 255, 70, 255 };
Color MAZE_WALL_COLOR = { 56, 64, 72, 255 };
Color MAZE_PATH_COLOR = { 238, 240, 235, 255 };
Color MAZE_SHADOW_COLOR = { 16, 18, 20, 255 };
Color MAZE_EXIT_COLOR = { 0, 230, 70, 255 };

float GetMazeScale(void)
{
    float mazeWidth = (float)(GRID_WIDTH * TILE_SIZE);
    float mazeHeight = (float)(GRID_HEIGTH * TILE_SIZE);
    float availableWidth = (float)GetScreenWidth() - (MAZE_PADDING * 2.0f);
    float availableHeight = (float)GetScreenHeight() - HUD_HEIGHT - (MAZE_PADDING * 2.0f);
    float scaleX = availableWidth / mazeWidth;
    float scaleY = availableHeight / mazeHeight;
    return fminf(scaleX, scaleY);
}

Vector2 GetMazeOffset(float scale)
{
    float mazeWidth = (float)(GRID_WIDTH * TILE_SIZE) * scale;
    float mazeHeight = (float)(GRID_HEIGTH * TILE_SIZE) * scale;
    float freeWidth = (float)GetScreenWidth() - mazeWidth;
    float freeHeight = (float)GetScreenHeight() - HUD_HEIGHT - mazeHeight;

    Vector2 offset = { 0 };
    offset.x = freeWidth * 0.5f;
    offset.y = HUD_HEIGHT + (freeHeight * 0.5f);
    return offset;
}

Vector2 WorldToScreenPosition(Vector2 worldPosition, float scale, Vector2 offset)
{
    Vector2 screenPosition = { 0 };
    screenPosition.x = offset.x + (worldPosition.x * scale);
    screenPosition.y = offset.y + (worldPosition.y * scale);
    return screenPosition;
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

void UpdateBuildMetrics(const char *executablePath)
{
    executableSizeBytes = GetFileSizeBytes(executablePath);
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

void GenerateMaze(void)
{
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

    grid[1][GRID_WIDTH - 2] = CELL_EXIT;
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
} Bullet;

Enemy redEnemies[RED_ENEMY_COUNT];
Enemy blueEnemies[BLUE_ENEMY_COUNT];
Enemy bossEnemy;
Bullet bullets[MAX_BULLETS];
bool playerAlive = true;

Player player;

typedef enum GamePhase
{
    PHASE_INTRO,
    PHASE_INFO,
    PHASE_PLAYING
} GamePhase;

typedef struct RoundConfig
{
    bool redEnabled;
    bool blueEnabled;
    bool bossEnabled;
    bool flashlightEnabled;
} RoundConfig;

GamePhase gamePhase = PHASE_INTRO;
RoundConfig currentRoundConfig = { true, false, false, false };
bool inTutorialSequence = true;
int tutorialRound = 1;
int officialRound = 1;
int bestOfficialRound = 1;
bool roundNeedsSetup = true;

void SpawnBullet(Vector2 position, Vector2 direction, float speed, bool fromPlayer);

float GetDistanceBetweenPoints(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf((dx * dx) + (dy * dy));
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
    if (!currentRoundConfig.flashlightEnabled)
    {
        return true;
    }

    if (GetDistanceBetweenPoints(position, player.position) <= TILE_SIZE * 2.6f)
    {
        return true;
    }

    if (currentRoundConfig.flashlightEnabled && flashlightOn)
    {
        Vector2 flashlightCenter = GetFlashlightCenter();
        if (GetDistanceBetweenPoints(position, flashlightCenter) <= TILE_SIZE * 3.1f)
        {
            return true;
        }
    }

    return false;
}

void DrawTextStrong(const char *text, int x, int y, int fontSize, Color color, Color shadowColor)
{
    DrawText(text, x + 2, y + 2, fontSize, shadowColor);
    DrawText(text, x, y, fontSize, color);
}

/* Same as DrawTextStrong, but lets the caller control the space between
 * letters. Used where legibility matters most (tutorial explanations). */
void DrawTextStrongSpaced(const char *text, int x, int y, int fontSize, float spacing, Color color, Color shadowColor)
{
    Font font = GetFontDefault();
    Vector2 shadowPos = { (float)(x + 2), (float)(y + 2) };
    Vector2 textPos = { (float)x, (float)y };

    DrawTextEx(font, text, shadowPos, (float)fontSize, spacing, shadowColor);
    DrawTextEx(font, text, textPos, (float)fontSize, spacing, color);
}

/* Measures multiline text drawn with DrawTextStrongSpaced so panels can
 * size themselves around it instead of guessing a fixed height. */
Vector2 MeasureTextStrongSpaced(const char *text, int fontSize, float spacing)
{
    Font font = GetFontDefault();
    return MeasureTextEx(font, text, (float)fontSize, spacing);
}

void DrawMazeGrid(void)
{
    float scale = GetMazeScale();
    float drawTileSize = (float)TILE_SIZE * scale;
    Vector2 offset = GetMazeOffset(scale);
    Rectangle mazeFrame = {
        offset.x - (drawTileSize * 0.35f),
        offset.y - (drawTileSize * 0.35f),
        (GRID_WIDTH * drawTileSize) + (drawTileSize * 0.7f),
        (GRID_HEIGTH * drawTileSize) + (drawTileSize * 0.7f)
    };

    DrawRectangleRounded(mazeFrame, 0.02f, 8, MAZE_SHADOW_COLOR);
    DrawRectangleRoundedLinesEx(mazeFrame, 0.02f, 8, 2.0f, Fade(HUD_BORDER_COLOR, 0.45f));

    for (int y = 0; y < GRID_HEIGTH; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++)
        {
            Color cellColor = BLACK;
            Vector2 cellCenter = {
                ((float)x + 0.5f) * TILE_SIZE,
                ((float)y + 0.5f) * TILE_SIZE
            };

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

            if (!IsWorldPositionVisible(cellCenter))
            {
                cellColor = (Color){ 6, 6, 6, 255 };
            }

            Vector2 position = { offset.x + ((float)x * drawTileSize), offset.y + ((float)y * drawTileSize) };
            Vector2 size = { drawTileSize, drawTileSize };
            DrawRectangleV(position, size, cellColor);

            if (grid[y][x] == CELL_PATH && IsWorldPositionVisible(cellCenter))
            {
                DrawRectangleLinesEx((Rectangle){ position.x, position.y, size.x, size.y }, 1.0f, Fade(LIGHTGRAY, 0.08f));
            }
            else if (grid[y][x] == CELL_EXIT && IsWorldPositionVisible(cellCenter))
            {
                DrawRectangleLinesEx((Rectangle){ position.x, position.y, size.x, size.y }, 2.0f, RAYWHITE);
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

    return grid[y][x] == CELL_WALL;
}

bool IsPositionBlocked(Vector2 position, float radius)
{
    int left = (int)(position.x - radius) / TILE_SIZE;
    int right = (int)(position.x + radius) / TILE_SIZE;
    int top = (int)(position.y - radius) / TILE_SIZE;
    int bottom = (int)(position.y + radius) / TILE_SIZE;

    if (IsWallCell(left, top)) return true;
    if (IsWallCell(right, top)) return true;
    if (IsWallCell(left, bottom)) return true;
    if (IsWallCell(right, bottom)) return true;

    return false;
}

void InitPlayer(void)
{
    player.radius = 8.0f;
    player.speed = 65.0f;
    player.facingAngle = 0.0f;
    player.position.x = TILE_SIZE * 1.5f;
    player.position.y = TILE_SIZE * 1.5f;
}

void UpdatePlayer(void)
{
    Vector2 movement = { 0 };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) movement.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) movement.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) movement.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) movement.x += 1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f)
    {
        float length = sqrtf(movement.x * movement.x + movement.y * movement.y);
        movement.x /= length;
        movement.y /= length;
        player.facingAngle = atan2f(movement.y, movement.x);

        float frameSpeed = player.speed * GetFrameTime();
        Vector2 nextPosition = player.position;

        nextPosition.x += movement.x * frameSpeed;
        if (!IsPositionBlocked(nextPosition, player.radius))
        {
            player.position.x = nextPosition.x;
        }

        nextPosition = player.position;
        nextPosition.y += movement.y * frameSpeed;

        if (!IsPositionBlocked(nextPosition, player.radius))
        {
            player.position.y = nextPosition.y;
        }
    }
}

void UpdateFlashlight(void)
{
    if (!currentRoundConfig.flashlightEnabled)
    {
        flashlightOn = false;
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
            flashlightOn = true;
        }
    }

    if (flashlightOn)
    {
        flashlightBattery -= GetFrameTime();

        if (flashlightBattery <= 0.0f)
        {
            flashlightBattery = 0.0f;
            flashlightOn = false;
        }
    }
}

void UpdatePlayerShooting(void)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        if (playerAmmo <= 0 || playerReloadTimer > 0.0f)
        {
            return;
        }

        Vector2 shotDirection = { cosf(player.facingAngle), sinf(player.facingAngle) };
        Vector2 shotOrigin = player.position;
        shotOrigin.x += shotDirection.x * (player.radius + 8.0f);
        shotOrigin.y += shotDirection.y * (player.radius + 8.0f);
        SpawnBullet(shotOrigin, shotDirection, PLAYER_BULLET_SPEED, true);
        playerAmmo--;
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
            playerAmmo = PLAYER_MAX_AMMO;
        }
        return;
    }

    if (IsKeyPressed(KEY_R) && playerAmmo < PLAYER_MAX_AMMO)
    {
        playerReloadTimer = PLAYER_RELOAD_TIME;
    }
}

bool DidPlayerReachExit(void)
{
    int playerCellX = (int)(player.position.x / TILE_SIZE);
    int playerCellY = (int)(player.position.y / TILE_SIZE);

    return grid[playerCellY][playerCellX] == CELL_EXIT;
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

void SpawnBullet(Vector2 position, Vector2 direction, float speed, bool fromPlayer)
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
            break;
        }
    }
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
        redEnemies[i].speed = 34.0f + (float)(i * 4);
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
        blueEnemies[i].speed = 42.0f + (float)(i * 4);
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
    bossEnemy.speed = 42.0f;
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
    if (round >= 20) return (RoundConfig){ true, true, true, true };
    if (round == 15) return (RoundConfig){ true, true, true, false };
    if (round == 10) return (RoundConfig){ true, false, true, false };
    if (round == 5) return (RoundConfig){ false, false, true, false };
    if ((round >= 16 && round <= 19) ||
        (round >= 11 && round <= 14) ||
        (round >= 6 && round <= 9) ||
        (round >= 3 && round <= 4))
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
    playerAmmo = PLAYER_MAX_AMMO;
    playerReloadTimer = 0.0f;

    if (inTutorialSequence && tutorialRound == 3)
    {
        flashlightBattery = FLASHLIGHT_MAX_BATTERY;
    }

    if (!inTutorialSequence && currentRoundConfig.flashlightEnabled)
    {
        flashlightBattery = FLASHLIGHT_MAX_BATTERY;
    }
}

void SetupRound(void)
{
    playerAlive = true;
    playerHealth = PLAYER_MAX_HEALTH;
    GenerateMaze();
    InitPlayer();
    InitBullets();
    InitEnemies();
    InitBossEnemy();
    ApplyRoundConfig();
    roundNeedsSetup = false;
}

const char *GetIntroTitle(void)
{
    return "BYTE MAZE";
}

const char *GetIntroBody(void)
{
    return "Chegue na saida verde para vencer.\nDesvie dos inimigos e use ESPACO para atirar e R para recarregar.\nA cada novo tipo de round, o jogo explica a regra.";
}

const char *GetRoundInfoTitle(void)
{
    if (inTutorialSequence)
    {
        if (tutorialRound == 1) return "TUTORIAL 1";
        if (tutorialRound == 2) return "TUTORIAL 2";
        if (tutorialRound == 3) return "TUTORIAL 3";
        return "TUTORIAL 4";
    }

    return TextFormat("ROUND %d", officialRound);
}

const char *GetRoundInfoBody(void)
{
    if (inTutorialSequence)
    {
        if (tutorialRound == 1) return "Neste round voce enfrenta apenas os\ninimigos vermelhos.\nSe um deles tocar em voce, a partida\nacaba na hora.\nUse WASD ou as setas para se mover\ne alcance a saida verde.";
        if (tutorialRound == 2) return "Agora o inimigo rosa entrou na partida.\nEle aparece perto da saida e vai\npressionar voce logo no comeco.\nSao 5 tiros para nocautea-lo.\nSua arma carrega 5 balas.\nAtire com ESPACO e recarregue com R.";
        if (tutorialRound == 3) return "A lanterna foi liberada.\nPressione C para ligar ou desligar.\nEla amplia bastante sua visao,\nmas consome bateria enquanto estiver ativa.\nCada round comeca com 100%% de carga.\nVermelho e rosa continuam presentes.";
        return "Agora e a vez do chefao roxo.\nEle persegue voce com mais precisao,\natira enquanto se move\ne acelera muito se ficar longe.\nAssim como o rosa, ele pode ser\nnocauteado por alguns segundos.";
    }

    if (officialRound == 1) return "Jogo oficial iniciado.\nRounds 1 e 2: vermelho e rosa.\nArma com 20 balas e recarga no R.";
    if (officialRound == 3) return "Nova dinamica.\nRounds 3 e 4 com lanterna,\nvermelho e rosa.";
    if (officialRound == 5) return "Nova dinamica.\nRound 5: apenas o chefao roxo.";
    if (officialRound == 6) return "Nova dinamica.\nRounds 6 a 9 com lanterna,\nvermelho e rosa.";
    if (officialRound == 10) return "Nova dinamica.\nRound 10: roxo e vermelho.";
    if (officialRound == 11) return "Nova dinamica.\nRounds 11 a 14 com lanterna,\nvermelho e rosa.";
    if (officialRound == 15) return "Nova dinamica.\nRound 15: roxo, vermelho e rosa.";
    if (officialRound == 16) return "Nova dinamica.\nRounds 16 a 19 com lanterna,\nvermelho e rosa.";
    if (officialRound == 20) return "Dinamica final.\nRound 20 com lanterna,\nvermelho, rosa e roxo.";
    return "Atravesse o labirinto.\nChegue na saida verde\npara subir de round.";
}

bool ShouldShowRoundInfo(void)
{
    if (inTutorialSequence)
    {
        return true;
    }

    return officialRound == 1 || officialRound == 3 || officialRound == 5 ||
           officialRound == 6 || officialRound == 10 || officialRound == 11 ||
           officialRound == 15 || officialRound == 16 || officialRound == 20;
}

void StartCurrentStage(void)
{
    roundNeedsSetup = true;
    gamePhase = ShouldShowRoundInfo() ? PHASE_INFO : PHASE_PLAYING;
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
        StartCurrentStage();
        return;
    }

    officialRound++;
    if (officialRound > bestOfficialRound)
    {
        bestOfficialRound = officialRound;
    }
    StartCurrentStage();
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

    if (GetDistanceBetweenPoints(enemy->position, player.position) <= TILE_SIZE * 5.5f)
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

    if (validCount == 0)
    {
        if (!allowReverse)
        {
            enemy->direction = (Vector2){ -enemy->direction.x, -enemy->direction.y };
        }
        return;
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
            Vector2 shotOrigin = bossEnemy.position;
            shotOrigin.x += shotDirection.x * (bossEnemy.radius + 6.0f);
            shotOrigin.y += shotDirection.y * (bossEnemy.radius + 6.0f);
            SpawnBullet(shotOrigin, shotDirection, BOSS_BULLET_SPEED, false);
            bossEnemy.shootPauseTimer = 0.85f;
        }
    }

    float frameSpeed = bossEnemy.speed * GetFrameTime();
    if (GetDistanceBetweenPoints(bossEnemy.position, player.position) >= BOSS_FAR_DISTANCE_THRESHOLD)
    {
        frameSpeed *= BOSS_FAR_SPEED_MULTIPLIER;
    }

    Vector2 nextPosition = bossEnemy.position;
    nextPosition.x += bossEnemy.direction.x * frameSpeed;
    nextPosition.y += bossEnemy.direction.y * frameSpeed;

    if (IsPositionBlocked(nextPosition, bossEnemy.radius))
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

bool DidEnemyTouchPlayer(void)
{
    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (!IsEnemyDangerous(&redEnemies[i]))
        {
            continue;
        }

        float dx = redEnemies[i].position.x - player.position.x;
        float dy = redEnemies[i].position.y - player.position.y;
        float distance = sqrtf((dx * dx) + (dy * dy));
        float touchDistance = redEnemies[i].radius + player.radius;

        if (distance <= touchDistance)
        {
            return true;
        }
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (!IsEnemyDangerous(&blueEnemies[i]))
        {
            continue;
        }

        float dx = blueEnemies[i].position.x - player.position.x;
        float dy = blueEnemies[i].position.y - player.position.y;
        float distance = sqrtf((dx * dx) + (dy * dy));
        float touchDistance = blueEnemies[i].radius + player.radius;

        if (distance <= touchDistance)
        {
            return true;
        }
    }

    if (IsEnemyDangerous(&bossEnemy))
    {
        float dx = bossEnemy.position.x - player.position.x;
        float dy = bossEnemy.position.y - player.position.y;
        float distance = sqrtf((dx * dx) + (dy * dy));
        float touchDistance = bossEnemy.radius + player.radius;

        if (distance <= touchDistance)
        {
            return true;
        }
    }

    return false;
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
                    bullets[i].active = false;

                    if (redEnemies[enemyIndex].hitsTaken >= RED_HIT_LIMIT)
                    {
                        redEnemies[enemyIndex].hitsTaken = 0;
                        redEnemies[enemyIndex].knockoutTimer = BLUE_KNOCKOUT_TIME;
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
                    bullets[i].active = false;

                    if (blueEnemies[enemyIndex].hitsTaken >= BLUE_HIT_LIMIT)
                    {
                        blueEnemies[enemyIndex].hitsTaken = 0;
                        blueEnemies[enemyIndex].knockoutTimer = BLUE_KNOCKOUT_TIME;
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
                bullets[i].active = false;

                if (bossEnemy.hitsTaken >= BOSS_HIT_LIMIT)
                {
                    bossEnemy.hitsTaken = 0;
                    bossEnemy.knockoutTimer = BOSS_KNOCKOUT_TIME;
                    bossEnemy.shootPauseTimer = 0.0f;
                }
            }
        }
        else
        {
            float dx = player.position.x - bullets[i].position.x;
            float dy = player.position.y - bullets[i].position.y;
            float distance = sqrtf((dx * dx) + (dy * dy));

            if (distance <= player.radius + bullets[i].radius)
            {
                bullets[i].active = false;
                playerHealth -= BLUE_BULLET_DAMAGE;

                if (playerHealth <= 0)
                {
                    playerHealth = 0;
                    playerAlive = false;
                }
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

void DrawPlayerHealthBar(float x, float y)
{
    Rectangle barBackground = { x, y, 216.0f, 20.0f };
    Rectangle barFill = barBackground;
    Color healthColor = LIME;

    barFill.width = (barBackground.width * (float)playerHealth) / (float)PLAYER_MAX_HEALTH;

    if (playerHealth <= 30)
    {
        healthColor = RED;
    }
    else if (playerHealth <= 60)
    {
        healthColor = ORANGE;
    }

    DrawTextStrong(TextFormat("Vida: %d/%d", playerHealth, PLAYER_MAX_HEALTH), (int)x, (int)(y - 24.0f), 18, RAYWHITE, BLACK);
    DrawRectangleRounded(barBackground, 0.22f, 10, (Color){ 35, 35, 35, 255 });
    DrawRectangleRounded(barFill, 0.35f, 10, healthColor);
    DrawRectangleRoundedLinesEx(barBackground, 0.22f, 10, 2.0f, RAYWHITE);
}

void DrawHud(void)
{
    /* Clear vertical rhythm: one purpose per row, a divider between the
     * "brand" block and the stats block, and consistent left padding so
     * nothing overlaps or crowds the row above/below it (the old layout
     * used fixed y-values that collided once the title got bigger). */
    float panelX = 12.0f;
    float panelY = 10.0f;
    float panelWidth = 304.0f;
    float textX = panelX + 16.0f;
    float rowY = panelY + 16.0f;

    Rectangle panel = { panelX, panelY, panelWidth, currentRoundConfig.flashlightEnabled ? 346.0f : 290.0f };

    DrawRectangleRounded(panel, 0.06f, 10, HUD_PANEL_COLOR);
    DrawRectangleRoundedLinesEx(panel, 0.06f, 10, 2.0f, HUD_BORDER_COLOR);

    /* Title row */
    DrawTextStrongSpaced("BYTEMAZE", (int)textX, (int)rowY, 26, 2.0f, (Color){ 80, 255, 140, 255 }, BLACK);
    rowY += 34.0f;
    DrawTextStrong(TextFormat("%lld bytes   %.2f%%", executableSizeBytes, executableUsagePercent), (int)textX, (int)rowY, 16, LIGHTGRAY, BLACK);
    rowY += 26.0f;

    DrawLineEx((Vector2){ panelX + 10.0f, rowY }, (Vector2){ panelX + panelWidth - 10.0f, rowY }, 1.0f, Fade(HUD_BORDER_COLOR, 0.4f));
    rowY += 14.0f;

    /* Progress row */
    DrawTextStrong(TextFormat("Recorde pessoal: round %d", bestOfficialRound), (int)textX, (int)rowY, 18, GOLD, BLACK);
    rowY += 26.0f;
    DrawTextStrong(inTutorialSequence ? TextFormat("Tutorial %d/4", tutorialRound) : TextFormat("Round %d", officialRound), (int)textX, (int)rowY, 20, RAYWHITE, BLACK);
    rowY += 48.0f;

    /* Health row (label is drawn above its own bar inside the function) */
    DrawPlayerHealthBar(textX, rowY);
    rowY += 42.0f;

    DrawLineEx((Vector2){ panelX + 10.0f, rowY }, (Vector2){ panelX + panelWidth - 10.0f, rowY }, 1.0f, Fade(HUD_BORDER_COLOR, 0.4f));
    rowY += 14.0f;

    /* Controls row */
    DrawTextStrong("Atirar: ESPACO", (int)textX, (int)rowY, 20, ORANGE, BLACK);
    rowY += 28.0f;
    DrawTextStrong(TextFormat("Balas: %d/%d", playerAmmo, PLAYER_MAX_AMMO), (int)textX, (int)rowY, 18, RAYWHITE, BLACK);
    rowY += 22.0f;
    if (playerReloadTimer > 0.0f)
    {
        DrawTextStrong(TextFormat("Recarregando: %.1fs", playerReloadTimer), (int)textX, (int)rowY, 18, YELLOW, BLACK);
    }
    else
    {
        DrawTextStrong("Recarregar: R", (int)textX, (int)rowY, 18, SKYBLUE, BLACK);
    }
    rowY += 24.0f;

    if (currentRoundConfig.flashlightEnabled)
    {
        DrawTextStrong("Lanterna: C", (int)textX, (int)rowY, 18, GOLD, BLACK);
        rowY += 24.0f;
        DrawTextStrong(TextFormat("Lanterna: %s", flashlightOn ? "ligada" : "desligada"), (int)textX, (int)rowY, 18, flashlightOn ? GREEN : GRAY, BLACK);
        rowY += 24.0f;
        DrawTextStrong(TextFormat("Bateria: %.0f%%", flashlightBattery), (int)textX, (int)rowY, 18, GREEN, BLACK);
        rowY += 24.0f;
    }
}

void DrawGameOverOverlay(void)
{
    const char *title = "VOCE MORREU";
    const char *buttonText = "JOGAR DE NOVO";
    int titleFontSize = 42;
    int buttonFontSize = 24;
    int titleWidth = MeasureText(title, titleFontSize);
    int buttonTextWidth = MeasureText(buttonText, buttonFontSize);
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int panelWidth = 420;
    int panelHeight = 220;
    Rectangle panel = {
        (float)((screenWidth - panelWidth) / 2),
        (float)((screenHeight - panelHeight) / 2),
        (float)panelWidth,
        (float)panelHeight
    };
    Rectangle button = {
        panel.x + 70.0f,
        panel.y + 130.0f,
        panel.width - 140.0f,
        54.0f
    };
    Vector2 mousePosition = GetMousePosition();
    bool isButtonHovered = CheckCollisionPointRec(mousePosition, button);
    Color buttonColor = isButtonHovered ? LIME : GREEN;

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.55f));
    DrawRectangleRounded(panel, 0.12f, 12, (Color){ 24, 24, 24, 235 });
    DrawRectangleRoundedLinesEx(panel, 0.12f, 12, 3.0f, GREEN);
    DrawText(title, (int)(panel.x + (panel.width - titleWidth) * 0.5f), (int)panel.y + 44, titleFontSize, RAYWHITE);
    DrawRectangleRounded(button, 0.3f, 12, buttonColor);
    DrawText(buttonText, (int)(button.x + (button.width - buttonTextWidth) * 0.5f), (int)(button.y + 14.0f), buttonFontSize, BLACK);

    if (isButtonHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        ResetGame();
    }
}

void DrawPlayer(void)
{
    float scale = GetMazeScale();
    Vector2 offset = GetMazeOffset(scale);
    Vector2 center = WorldToScreenPosition(player.position, scale, offset);
    float drawRadius = fmaxf(player.radius * scale, 6.0f);

    Vector2 tip = {
        center.x + (cosf(player.facingAngle) * drawRadius * 1.15f),
        center.y + (sinf(player.facingAngle) * drawRadius * 1.15f)
    };
    
    Vector2 left = {
        center.x + (cosf(player.facingAngle + 2.45f) * drawRadius * 0.95f),
        center.y + (sinf(player.facingAngle + 2.45f) * drawRadius * 0.95f)
    };
    Vector2 right = {
        center.x + (cosf(player.facingAngle - 2.45f) * drawRadius * 0.95f),
        center.y + (sinf(player.facingAngle - 2.45f) * drawRadius * 0.95f)
    };

    DrawTriangle(tip, right, left, GOLD);
    DrawTriangleLines(tip, right, left, (Color){ 255, 250, 220, 255 });
}

void DrawEnemies(void)
{
    float scale = GetMazeScale();
    Vector2 offset = GetMazeOffset(scale);

    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (!redEnemies[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPosition(redEnemies[i].position, scale, offset);
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

        Vector2 center = WorldToScreenPosition(blueEnemies[i].position, scale, offset);
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
        Vector2 bossCenter = WorldToScreenPosition(bossEnemy.position, scale, offset);
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
        DrawRectangleV(bossPos, bossSize, bossBody);
        DrawRectangleLinesEx((Rectangle){ bossPos.x, bossPos.y, bossSize.x, bossSize.y }, 2.0f, RAYWHITE);
    }
}

void DrawBullets(void)
{
    float scale = GetMazeScale();
    Vector2 offset = GetMazeOffset(scale);

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPosition(bullets[i].position, scale, offset);
        float drawRadius = fmaxf(bullets[i].radius * scale, 3.0f);
        Color bulletColor = bullets[i].fromPlayer ? ORANGE : SKYBLUE;
        DrawCircleV(center, drawRadius, bulletColor);
    }
}

void DrawVisibilityEffects(void)
{
    if (!currentRoundConfig.flashlightEnabled)
    {
        return;
    }

    float scale = GetMazeScale();
    Vector2 offset = GetMazeOffset(scale);
    Vector2 playerCenter = WorldToScreenPosition(player.position, scale, offset);
    float playerAuraRadius = TILE_SIZE * scale * 2.9f;

    DrawCircleV(playerCenter, playerAuraRadius, Fade(GOLD, 0.07f));

    if (currentRoundConfig.flashlightEnabled && flashlightOn)
    {
        Vector2 flashlightCenter = WorldToScreenPosition(GetFlashlightCenter(), scale, offset);
        DrawCircleV(flashlightCenter, TILE_SIZE * scale * 4.8f, Fade((Color){ 255, 245, 180, 255 }, 0.13f));
    }
}

Rectangle GetRoundPanelRect(const char *title, const char *body)
{
    (void)title;
    const float bodyFontSize = 22.0f;
    const float bodySpacing = 2.4f;
    const float bodyTop = 96.0f;
    const float footerGap = 40.0f;
    const float bottomPadding = 30.0f;
    Vector2 bodySize = MeasureTextStrongSpaced(body, (int)bodyFontSize, bodySpacing);
    float panelWidth = fmaxf(600.0f, bodySize.x + 56.0f);
    float panelHeight = fmaxf(270.0f, bodyTop + bodySize.y + footerGap + bottomPadding);

    return (Rectangle){
        (float)(GetScreenWidth() / 2) - panelWidth * 0.5f,
        (float)(GetScreenHeight() / 2) - panelHeight * 0.5f,
        panelWidth,
        panelHeight
    };
}

Rectangle GetTutorialSkipButtonRect(Rectangle panel)
{
    return (Rectangle){
        panel.x + (panel.width - 200.0f) * 0.5f,
        panel.y + panel.height + 16.0f,
        200.0f,
        38.0f
    };
}

void DrawRoundPanel(const char *title, const char *body, const char *footer)
{
    /* Body text uses extra letter spacing for legibility, and the panel
     * height grows to fit however many lines the explanation needs, so
     * longer tutorial text never gets clipped or cramped. */
    const float bodyFontSize = 22.0f;
    const float bodySpacing = 2.4f;
    const float bodyTop = 96.0f;
    const float footerGap = 40.0f;
    const float bottomPadding = 30.0f;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int titleWidth = MeasureText(title, 30);
    Vector2 bodySize = MeasureTextStrongSpaced(body, (int)bodyFontSize, bodySpacing);
    Rectangle panel = GetRoundPanelRect(title, body);

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.78f));
    DrawRectangleRounded(panel, 0.08f, 12, (Color){ 18, 18, 18, 245 });
    DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 3.0f, GREEN);
    DrawTextStrong(title, (int)(panel.x + (panel.width - titleWidth) * 0.5f), (int)panel.y + 24, 30, GREEN, BLACK);
    DrawTextStrongSpaced(body, (int)panel.x + 28, (int)(panel.y + bodyTop), (int)bodyFontSize, bodySpacing, RAYWHITE, BLACK);
    DrawTextStrong(footer, (int)panel.x + 28, (int)(panel.y + bodyTop + bodySize.y + footerGap - 20.0f), 20, LIGHTGRAY, BLACK);

    if (inTutorialSequence)
    {
        Rectangle skipButton = GetTutorialSkipButtonRect(panel);
        Vector2 mousePosition = GetMousePosition();
        bool hovered = CheckCollisionPointRec(mousePosition, skipButton);
        Color fillColor = hovered ? (Color){ 35, 120, 60, 255 } : (Color){ 22, 72, 40, 255 };

        DrawRectangleRounded(skipButton, 0.28f, 10, fillColor);
        DrawRectangleRoundedLinesEx(skipButton, 0.28f, 10, 2.0f, HUD_BORDER_COLOR);
        DrawTextStrong("Pular tutorial", (int)skipButton.x + 28, (int)skipButton.y + 9, 20, RAYWHITE, BLACK);
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ByteMaze");
    MaximizeWindow();
    SetTargetFPS(60);
    SetRandomSeed((unsigned int)GetTime());
    UpdateBuildMetrics(argv[0]);

    while (!WindowShouldClose())
    {
        UpdateBuildMetrics(argv[0]);

        if (gamePhase == PHASE_INTRO)
        {
            if (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                tutorialRound = 1;
                inTutorialSequence = true;
                gamePhase = PHASE_INFO;
            }
        }
        else if (gamePhase == PHASE_INFO)
        {
            if (roundNeedsSetup)
            {
                SetupRound();
            }

            if (inTutorialSequence && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                const char *body = GetRoundInfoBody();
                Rectangle panel = GetRoundPanelRect(GetRoundInfoTitle(), body);
                Rectangle skipButton = GetTutorialSkipButtonRect(panel);

                if (CheckCollisionPointRec(GetMousePosition(), skipButton))
                {
                    inTutorialSequence = false;
                    tutorialRound = 1;
                    officialRound = 1;
                    bestOfficialRound = 1;
                    StartCurrentStage();
                    continue;
                }
            }

            if (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                gamePhase = PHASE_PLAYING;
            }
        }
        else if (gamePhase == PHASE_PLAYING)
        {
            if (roundNeedsSetup)
            {
                SetupRound();
            }

            if (playerAlive)
            {
                UpdatePlayer();
                UpdatePlayerShooting();
                UpdatePlayerReload();
                UpdateFlashlight();
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

                if (DidEnemyTouchPlayer())
                {
                    playerAlive = false;
                }

                if (DidPlayerReachExit())
                {
                    AdvanceToNextStage();
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (gamePhase == PHASE_PLAYING)
        {
            DrawMazeGrid();
            DrawVisibilityEffects();
            DrawPlayer();
            DrawEnemies();
            DrawBullets();
            DrawHud();
        }

        if (gamePhase == PHASE_PLAYING && !playerAlive)
        {
            DrawGameOverOverlay();
        }

        if (gamePhase == PHASE_INTRO)
        {
            DrawRoundPanel(GetIntroTitle(), GetIntroBody(), "Pressione qualquer tecla ou clique para iniciar o tutorial.");
        }
        else if (gamePhase == PHASE_INFO)
        {
            DrawRoundPanel(GetRoundInfoTitle(), GetRoundInfoBody(), "Pressione qualquer tecla ou clique para comecar.");
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
