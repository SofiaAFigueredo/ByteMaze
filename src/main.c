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

    while (!WindowShouldClose())
    {
        UpdatePlayer();

        BeginDrawing();
        ClearBackground(BLACK);

        DrawMazeGrid();
        DrawPlayer();
        DrawText("ByteMaze - etapa inicial", 20, 18, 20, GREEN);
        DrawText(TextFormat("Build: %lld bytes (%.2f%% de 1,474,560)", executableSizeBytes, executableUsagePercent), 20, 40, 16, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
