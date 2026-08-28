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
#define ENEMY_COUNT 3

int grid[GRID_HEIGTH][GRID_WIDTH];
long long executableSizeBytes = -1;
float executableUsagePercent = 0.0f;

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

            Vector2 position = { offset.x + ((float)x * drawTileSize), offset.y + ((float)y * drawTileSize) };
            Vector2 size = { drawTileSize, drawTileSize };
            DrawRectangleV(position, size, cellColor);
        }
    }
}

typedef struct Player
{
    Vector2 position;
    float radius;
    float speed;
    float facingAngle;
} Player;

typedef struct Enemy
{
    Vector2 position;
    Vector2 direction;
    float radius;
    float speed;
    bool active;
} Enemy;

Enemy enemies[ENEMY_COUNT];
bool playerAlive = true;

Player player;

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

bool DidPlayerReachExit(void)
{
    int playerCellX = (int)(player.position.x / TILE_SIZE);
    int playerCellY = (int)(player.position.y / TILE_SIZE);

    return grid[playerCellY][playerCellX] == CELL_EXIT;
}

bool IsSpawnCellAvailable(int cellX, int cellY, int enemyIndex)
{
    if (grid[cellY][cellX] != CELL_PATH)
    {
        return false;
    }

    for (int i = 0; i < enemyIndex; i++)
    {
        int otherCellX = (int)(enemies[i].position.x / TILE_SIZE);
        int otherCellY = (int)(enemies[i].position.y / TILE_SIZE);

        if (otherCellX == cellX && otherCellY == cellY)
        {
            return false;
        }
    }

    return !(cellX == 1 && cellY == 1);
}

Vector2 FindEnemySpawnPosition(int enemyIndex)
{
    int pathCellsX[GRID_WIDTH * GRID_HEIGTH];
    int pathCellsY[GRID_WIDTH * GRID_HEIGTH];
    int pathCount = 0;

    for (int y = 1; y < GRID_HEIGTH - 1; y++)
    {
        for (int x = 1; x < GRID_WIDTH - 1; x++)
        {
            if (IsSpawnCellAvailable(x, y, enemyIndex))
            {
                pathCellsX[pathCount] = x;
                pathCellsY[pathCount] = y;
                pathCount++;
            }
        }
    }

    if (pathCount > 0)
    {
        int index = (pathCount * (enemyIndex + 1)) / (ENEMY_COUNT + 1);
        Vector2 spawnPosition = { 0 };
        spawnPosition.x = ((float)pathCellsX[index] + 0.5f) * TILE_SIZE;
        spawnPosition.y = ((float)pathCellsY[index] + 0.5f) * TILE_SIZE;
        return spawnPosition;
    }

    return (Vector2){ TILE_SIZE * 1.5f, TILE_SIZE * 1.5f };
}

void InitEnemies(void)
{
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        enemies[i].radius = 8.0f;
        enemies[i].speed = 45.0f + (float)(i * 5);
        enemies[i].active = true;
        enemies[i].position = FindEnemySpawnPosition(i);
        enemies[i].direction = (Vector2){ -1.0f, 0.0f };
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

void ChooseEnemyDirection(Enemy *enemy, bool allowReverse)
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

void UpdateEnemy(Enemy *enemy)
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
        ChooseEnemyDirection(enemy, true);
        return;
    }

    enemy->position = nextPosition;

    if (IsEnemyNearCellCenter(enemy))
    {
        Vector2 validDirections[4];
        int validCount = CollectEnemyDirections(enemy, validDirections, false);

        if (validCount > 1)
        {
            ChooseEnemyDirection(enemy, false);
        }
    }
}

bool DidEnemyTouchPlayer(void)
{
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        float dx = enemies[i].position.x - player.position.x;
        float dy = enemies[i].position.y - player.position.y;
        float distance = sqrtf((dx * dx) + (dy * dy));
        float touchDistance = enemies[i].radius + player.radius;

        if (distance <= touchDistance)
        {
            return true;
        }
    }

    return false;
}

void ResetGame(void)
{
    playerAlive = true;
    GenerateMaze();
    InitPlayer();
    InitEnemies();
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

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        if (!enemies[i].active)
        {
            continue;
        }

        Vector2 center = WorldToScreenPosition(enemies[i].position, scale, offset);
        float drawRadius = fmaxf(enemies[i].radius * scale, 6.0f);
        DrawCircleV(center, drawRadius, RED);
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

    GenerateMaze();
    InitPlayer();
    InitEnemies();

    while (!WindowShouldClose())
    {
        UpdateBuildMetrics(argv[0]);

        if (playerAlive)
        {
            UpdatePlayer();
            for (int i = 0; i < ENEMY_COUNT; i++)
            {
                UpdateEnemy(&enemies[i]);
            }

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
        DrawPlayer();
        DrawEnemies();
        DrawText("ByteMaze - etapa inicial", 20, 18, 20, GREEN);
        DrawText(TextFormat("Build: %lld bytes (%.2f%% de 1,474,560)", executableSizeBytes, executableUsagePercent), 20, 40, 16, LIGHTGRAY);

        if (!playerAlive)
        {
            DrawGameOverOverlay();
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
