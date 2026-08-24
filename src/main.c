#include "raylib.h"

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 640

#define GRID_HEIGTH 21
#define GRID_WIDTH 31
#define TILE_SIZE 24

#define CELL_WALL 0
#define CELL_PATH 1
#define CELL_EXIT 2

int grid[GRID_HEIGTH][GRID_WIDTH];

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
        int directions[4] = {0, 1, 2, 3};

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

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, cellColor);
        }
    }
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ByteMaze");
    SetTargetFPS(60);
    SetRandomSeed((unsigned int)GetTime());

    GenerateMaze();

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawMazeGrid();
        DrawText("ByteMaze - etapa inicial", 20, 20, 20, GREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
