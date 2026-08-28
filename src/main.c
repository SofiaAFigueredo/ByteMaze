#include "raylib.h"
#include "math.h"
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
#define BLUE_HIT_LIMIT 5
#define BLUE_KNOCKOUT_TIME 5.0f
#define BOSS_HIT_LIMIT 5
#define BOSS_KNOCKOUT_TIME 3.0f
#define PLAYER_MAX_HEALTH 100
#define BLUE_BULLET_DAMAGE 10

int grid[GRID_HEIGTH][GRID_WIDTH];
long long executableSizeBytes = -1;
float executableUsagePercent = 0.0f;
bool flashlightOn = false;
float flashlightBattery = FLASHLIGHT_MAX_BATTERY;
int playerHealth = PLAYER_MAX_HEALTH;

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
    (void)position;
    return true;

    /*
    if (GetDistanceBetweenPoints(position, player.position) <= TILE_SIZE * 2.6f)
    {
        return true;
    }

    if (flashlightOn)
    {
        Vector2 flashlightCenter = GetFlashlightCenter();
        if (GetDistanceBetweenPoints(position, flashlightCenter) <= TILE_SIZE * 3.1f)
        {
            return true;
        }
    }

    return false;
    */
}

void DrawMazeGrid(void)
{
    float scale = GetMazeScale();
    float drawTileSize = (float)TILE_SIZE * scale;
    Vector2 offset = GetMazeOffset(scale);

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
                cellColor = DARKGRAY;
            }
            else if (grid[y][x] == CELL_PATH)
            {
                cellColor = RAYWHITE;
            }
            else if (grid[y][x] == CELL_EXIT)
            {
                cellColor = GREEN;
            }

            if (!IsWorldPositionVisible(cellCenter))
            {
                cellColor = (Color){ 6, 6, 6, 255 };
            }

            Vector2 position = { offset.x + ((float)x * drawTileSize), offset.y + ((float)y * drawTileSize) };
            Vector2 size = { drawTileSize, drawTileSize };
            DrawRectangleV(position, size, cellColor);
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
        Vector2 shotDirection = { cosf(player.facingAngle), sinf(player.facingAngle) };
        Vector2 shotOrigin = player.position;
        shotOrigin.x += shotDirection.x * (player.radius + 8.0f);
        shotOrigin.y += shotDirection.y * (player.radius + 8.0f);
        SpawnBullet(shotOrigin, shotDirection, PLAYER_BULLET_SPEED, true);
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

Vector2 FindEnemySpawnPosition(int spawnSlot, int totalSlots)
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
                pathCellsX[pathCount] = x;
                pathCellsY[pathCount] = y;
                pathCount++;
            }
        }
    }

    if (pathCount > 0)
    {
        int index = (pathCount * (spawnSlot + 1)) / (totalSlots + 1);
        if (index >= pathCount) index = pathCount - 1;
        Vector2 spawnPosition = { 0 };
        spawnPosition.x = ((float)pathCellsX[index] + 0.5f) * TILE_SIZE;
        spawnPosition.y = ((float)pathCellsY[index] + 0.5f) * TILE_SIZE;
        return spawnPosition;
    }

    return (Vector2){ TILE_SIZE * 1.5f, TILE_SIZE * 1.5f };
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

    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        redEnemies[i].radius = 8.0f;
        redEnemies[i].speed = 45.0f + (float)(i * 5);
        redEnemies[i].active = true;
        redEnemies[i].type = ENEMY_RED;
        redEnemies[i].hitsTaken = 0;
        redEnemies[i].knockoutTimer = 0.0f;
        redEnemies[i].shootPauseTimer = 0.0f;
        redEnemies[i].position = FindEnemySpawnPosition(spawnSlot, totalSlots);
        redEnemies[i].direction = (Vector2){ -1.0f, 0.0f };
        spawnSlot++;
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        blueEnemies[i].radius = 9.0f;
        blueEnemies[i].speed = 56.0f + (float)(i * 6);
        blueEnemies[i].active = true;
        blueEnemies[i].type = ENEMY_BLUE;
        blueEnemies[i].hitsTaken = 0;
        blueEnemies[i].knockoutTimer = 0.0f;
        blueEnemies[i].shootPauseTimer = 0.0f;
        blueEnemies[i].position = FindEnemySpawnPosition(spawnSlot, totalSlots);
        blueEnemies[i].direction = (Vector2){ 1.0f, 0.0f };
        spawnSlot++;
    }
}

void InitBossEnemy(void)
{
    bossEnemy.radius = 10.0f;
    bossEnemy.speed = 54.0f;
    bossEnemy.active = true;
    bossEnemy.type = ENEMY_BOSS;
    bossEnemy.hitsTaken = 0;
    bossEnemy.knockoutTimer = 0.0f;
    bossEnemy.shootPauseTimer = 0.0f;
    bossEnemy.position = FindEnemySpawnPosition(0, 1);
    bossEnemy.direction = (Vector2){ -1.0f, 0.0f };
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

    if ((enemy->type == ENEMY_BLUE || enemy->type == ENEMY_BOSS) && enemy->knockoutTimer > 0.0f)
    {
        return false;
    }

    return true;
}

bool DidEnemyTouchPlayer(void)
{
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

    /*
    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
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

    return false;
    */

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
    playerAlive = true;
    playerHealth = PLAYER_MAX_HEALTH;
    flashlightOn = false;
    flashlightBattery = FLASHLIGHT_MAX_BATTERY;
    GenerateMaze();
    InitPlayer();
    InitBullets();
    /* InitEnemies(); */
    InitBossEnemy();
}

void DrawPlayerHealthBar(void)
{
    Rectangle barBackground = { 20.0f, 126.0f, 220.0f, 18.0f };
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

    DrawText(TextFormat("Vida: %d/%d", playerHealth, PLAYER_MAX_HEALTH), 20, 106, 16, RAYWHITE);
    DrawRectangleRounded(barBackground, 0.35f, 10, (Color){ 35, 35, 35, 255 });
    DrawRectangleRounded(barFill, 0.35f, 10, healthColor);
    DrawRectangleRoundedLinesEx(barBackground, 0.35f, 10, 2.0f, RAYWHITE);
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
}

void DrawEnemies(void)
{
    float scale = GetMazeScale();
    Vector2 offset = GetMazeOffset(scale);

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

    /*
    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (!redEnemies[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPosition(redEnemies[i].position, scale, offset);
        float drawRadius = fmaxf(redEnemies[i].radius * scale, 6.0f);
        DrawCircleV(center, drawRadius * 2.4f, Fade(RED, 0.10f));
        DrawCircleV(center, drawRadius, RED);
        DrawCircleLines((int)center.x, (int)center.y, drawRadius * 2.4f, RAYWHITE);
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (!blueEnemies[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPosition(blueEnemies[i].position, scale, offset);
        float drawRadius = fmaxf(blueEnemies[i].radius * scale, 7.0f);
        Color auraColor = Fade(BLUE, 0.10f);
        Color bodyColor = BLUE;

        if (blueEnemies[i].knockoutTimer > 0.0f)
        {
            auraColor = ((int)(blueEnemies[i].knockoutTimer * 8.0f) % 2 == 0) ? Fade(SKYBLUE, 0.18f) : Fade(BLUE, 0.06f);
            bodyColor = ((int)(blueEnemies[i].knockoutTimer * 8.0f) % 2 == 0) ? SKYBLUE : BLUE;
        }

        Vector2 top = { center.x, center.y - drawRadius };
        Vector2 right = { center.x + drawRadius, center.y };
        Vector2 bottom = { center.x, center.y + drawRadius };
        Vector2 left = { center.x - drawRadius, center.y };

        DrawCircleV(center, drawRadius * 2.4f, auraColor);
        DrawCircleLines((int)center.x, (int)center.y, drawRadius * 2.4f, RAYWHITE);
        DrawTriangle(top, right, bottom, bodyColor);
        DrawTriangle(top, bottom, left, bodyColor);
        DrawLineEx(top, right, 2.0f, RAYWHITE);
        DrawLineEx(right, bottom, 2.0f, RAYWHITE);
        DrawLineEx(bottom, left, 2.0f, RAYWHITE);
        DrawLineEx(left, top, 2.0f, RAYWHITE);
    }
    */
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
    /*
    float scale = GetMazeScale();
    Vector2 offset = GetMazeOffset(scale);
    Vector2 playerCenter = WorldToScreenPosition(player.position, scale, offset);
    float playerAuraRadius = TILE_SIZE * scale * 2.6f;

    DrawCircleV(playerCenter, playerAuraRadius, Fade(GOLD, 0.08f));

    if (flashlightOn)
    {
        Vector2 flashlightCenter = WorldToScreenPosition(GetFlashlightCenter(), scale, offset);
        DrawCircleV(flashlightCenter, TILE_SIZE * scale * 3.1f, Fade(GREEN, 0.07f));
    }
    */
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

    GenerateMaze();
    InitPlayer();
    InitBullets();
    /* InitEnemies(); */
    InitBossEnemy();

    while (!WindowShouldClose())
    {
        UpdateBuildMetrics(argv[0]);

        if (playerAlive)
        {
            UpdatePlayer();
            UpdatePlayerShooting();
            /* UpdateFlashlight(); */
            /*
            for (int i = 0; i < RED_ENEMY_COUNT; i++)
            {
                UpdateRedEnemy(&redEnemies[i]);
            }
            for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
            {
                UpdateBlueEnemy(&blueEnemies[i]);
            }
            */
            UpdateBossEnemy();
            UpdateBullets();

            if (DidEnemyTouchPlayer())
            {
                playerAlive = false;
            }
        }

        if (playerAlive && DidPlayerReachExit())
        {
            ResetGame();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawMazeGrid();
        /* DrawVisibilityEffects(); */
        DrawPlayer();
        DrawEnemies();
        DrawBullets();
        DrawText("ByteMaze - etapa inicial", 20, 18, 20, GREEN);
        DrawText(TextFormat("Build: %lld bytes (%.2f%% de 1,474,560)", executableSizeBytes, executableUsagePercent), 20, 40, 16, LIGHTGRAY);
        /* DrawText(TextFormat("Lanterna: %s", flashlightOn ? "ligada" : "desligada"), 20, 62, 16, flashlightOn ? GREEN : GRAY); */
        /* DrawText(TextFormat("Bateria: %.0f%%", flashlightBattery), 20, 84, 16, GREEN); */
        DrawText("Atirar: ESPACO", 20, 106, 16, ORANGE);
        DrawText("Teste: chefao roxo", 20, 62, 16, VIOLET);
        DrawPlayerHealthBar();

        if (!playerAlive)
        {
            DrawGameOverOverlay();
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
