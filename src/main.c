#include "raylib.h"

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 640

#define GRID_WIDTH 31
#define GRID_HEIGTH 21
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
            grid[x][y] = CELL_WALL;
        }
    }
}

void DrawGrid(void)
{
    for (int y = 0; y < GRID_HEIGTH; y++)
    {
        for (int x = 0; x < GRID_WIDTH; x++) 
        {
            Color cellColor = BLACK;

            if (grid[y][x] == CELL_WALL)
            {
                cellColor = BLACK;
            }
            else if (grid[y][x] == CELL_PATH)
            {
                cellColor = BLACK;
            }
            else if (grid[y][x] == CELL_EXIT)
            {
                cellColor = GREEN;
            }

            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, cellColor);
        }
    }
}

int main(void)
{
    InitWindow(screenWidth, screenHeight, "ByteMaze");
    SetTargetFPS(60);

    FillGridWithWalls();

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
