#include "raylib.h"
#include "math.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 640

#define GRID_HEIGTH 33
#define GRID_WIDTH 51
#define TILE_SIZE 24
#define HUD_HEIGHT 92.0f
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
#define ENEMY_KNOCKOUT_TIME 5.0f
#define BOSS_HIT_LIMIT 5
#define PLAYER_MAX_HEALTH 100
#define PLAYER_MAX_AMMO 15
#define PLAYER_RELOAD_TIME 1.5f
#define BLUE_BULLET_DAMAGE 10
#define ENEMY_TOUCH_DAMAGE 25
#define PLAYER_DAMAGE_COOLDOWN 1.0f
#define PLAYER_CROWD_RADIUS (TILE_SIZE * 2.75f)
#define PLAYER_MAX_NEAR_ENEMIES 2
#define PLAYER_COLLISION_STEP 2.0f
#define PLAYER_COLLISION_SKIN 1.0f
#define PLAYER_WALL_RADIUS_SCALE 0.72f
#define MUSIC_BASE_VOLUME 0.12f
#define MUSIC_NEAR_ENEMY_VOLUME 0.45f
#define MUSIC_NEAR_ENEMY_DISTANCE (TILE_SIZE * 8.0f)
#define PLAYER_SHOT_VOLUME 0.7f
#define ENEMY_SHOT_VOLUME 0.65f
#define PLAYER_RELOAD_VOLUME 0.75f
#define VICTORY_VOLUME 0.8f
#define GAME_OVER_VOLUME 0.8f
#define UI_FONT_PATH "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#define UI_FONT_BAKE_SIZE 48
#define UI_MAX_CODEPOINTS 512
#define BOSS_FAR_DISTANCE_THRESHOLD (TILE_SIZE * 10.0f)
#define BOSS_FAR_SPEED_MULTIPLIER 1.8f
#define MAX_SIMULATION_FRAME_TIME (1.0f / 45.0f)

int grid[GRID_HEIGTH][GRID_WIDTH];
long long executableSizeBytes = -1;
float executableUsagePercent = 0.0f;
bool flashlightOn = false;
float flashlightBattery = FLASHLIGHT_MAX_BATTERY;
int playerHealth = PLAYER_MAX_HEALTH;
int playerAmmo = PLAYER_MAX_AMMO;
float playerReloadTimer = 0.0f;
float playerDamageCooldown = 0.0f;

Color HUD_PANEL_COLOR = { 5, 10, 32, 232 };
/* Softer, less saturated than the original neon cyan/magenta so the maze
 * (not the HUD chrome) reads as the visual focal point. */
Color HUD_BORDER_COLOR = { 70, 150, 200, 255 };
Color HUD_ACCENT_COLOR = { 150, 100, 195, 255 };
Color MAZE_WALL_COLOR = { 20, 22, 36, 255 };
Color MAZE_PATH_COLOR = { 235, 240, 250, 255 };
Color MAZE_SHADOW_COLOR = { 8, 9, 18, 255 };
Color MAZE_EXIT_COLOR = { 60, 230, 130, 255 };

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
    return GetHudPanelWidth() + (10.0f * GetUIScale());
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

float GetMazeScale(void)
{
    float mazeWidth = (float)(GRID_WIDTH * TILE_SIZE);
    float mazeHeight = (float)(GRID_HEIGTH * TILE_SIZE);
    float ui = GetUIScale();

    if (IsCompactLayout())
    {
        /* Compact mode: the maze gets the entire central area. HUD panels
         * are drawn above/below it instead of stealing horizontal space. */
        float horizontalPadding = 10.0f * ui;
        float topSpace = 162.0f * ui;
        float bottomSpace = 108.0f * ui;
        float availableWidth = (float)GetScreenWidth() - horizontalPadding * 2.0f;
        float availableHeight = (float)GetScreenHeight() - topSpace - bottomSpace;
        float scaleX = availableWidth / mazeWidth;
        float scaleY = availableHeight / mazeHeight;
        return fmaxf(0.42f, fminf(fminf(scaleX, scaleY), 1.85f));
    }

    float sideGap = 20.0f * ui;
    float availableWidth = (float)GetScreenWidth()
        - GetMazeLeftReservedWidth()
        - GetHudRightPanelWidth()
        - sideGap;
    float availableHeight = (float)GetScreenHeight()
        - (70.0f * ui)
        - (22.0f * ui);
    float scaleX = availableWidth / mazeWidth;
    float scaleY = availableHeight / mazeHeight;

    /* The floor here must never exceed what the available space can
     * actually fit - a fixed 0.65 floor used to win over a smaller
     * computed scale, which is exactly why the maze kept rendering at
     * its old size and drawing over the HUD panels once they got wider.
     * Clamping the floor itself to availableWidth/Height fixes that: the
     * maze always shrinks to fit first, and only grows up to 1.65x when
     * there's genuinely room to spare. */
    float fitScale = fminf(scaleX, scaleY);
    float minScale = fminf(0.65f, fmaxf(0.35f, fitScale));
    return fmaxf(minScale, fminf(fitScale, 1.85f));
}

Vector2 GetMazeOffset(float scale)
{
    float mazeWidth = (float)(GRID_WIDTH * TILE_SIZE) * scale;
    float mazeHeight = (float)(GRID_HEIGTH * TILE_SIZE) * scale;
    float ui = GetUIScale();

    if (IsCompactLayout())
    {
        float topSpace = 162.0f * ui;
        float bottomSpace = 108.0f * ui;
        return (Vector2){
            ((float)GetScreenWidth() - mazeWidth) * 0.5f,
            topSpace + (((float)GetScreenHeight() - topSpace - bottomSpace) - mazeHeight) * 0.5f
        };
    }

    float leftReserved = GetMazeLeftReservedWidth();
    float rightReserved = GetHudRightPanelWidth();
    float sideGap = 20.0f * ui;
    float availableWidth = (float)GetScreenWidth() - leftReserved - rightReserved - sideGap;
    /* Clamped to >= 0: with the scale now always fit to availableWidth
     * (see GetMazeScale), freeWidth should never go negative, but this
     * guards against ever pushing the maze left edge into the HUD panel
     * if screen math rounds awkwardly at extreme window sizes. */
    float freeWidth = fmaxf(0.0f, availableWidth - mazeWidth);
    float freeHeight = fmaxf(0.0f, (float)GetScreenHeight() - (70.0f * ui) - mazeHeight);

    return (Vector2){
        leftReserved + (sideGap * 0.5f) + (freeWidth * 0.5f),
        (70.0f * ui) + (freeHeight * 0.5f)
    };
}

Vector2 WorldToScreenPosition(Vector2 worldPosition, float scale, Vector2 offset)
{
    Vector2 screenPosition = { 0 };
    screenPosition.x = offset.x + (worldPosition.x * scale);
    screenPosition.y = offset.y + (worldPosition.y * scale);
    return screenPosition;
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
    bool fromBoss;
} Bullet;

Enemy redEnemies[RED_ENEMY_COUNT];
Enemy blueEnemies[BLUE_ENEMY_COUNT];
Enemy bossEnemy;
Bullet bullets[MAX_BULLETS];
bool playerAlive = true;
Music backgroundMusic;
Sound playerShotSound;
Sound enemyShotSound;
Sound playerReloadSound;
Sound victorySound;
Sound gameOverSound;
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
bool playerStartAuraVisible = true;

void SpawnBullet(Vector2 position, Vector2 direction, float speed, bool fromPlayer);
void SpawnBossBullet(Vector2 position, Vector2 direction);
int GetDifficultyRampLevel(void);
float GetRedEnemyTrackDistance(void);
float GetBlueEnemyPathfindChance(void);
float GetEnemySpeedBonus(void);
float GetBossSpeedBonus(void);
float GetBossShootCooldown(void);
float GetBossFarSpeedMultiplier(void);
void InitGameAudio(void);
void UpdateGameAudio(void);
void ShutdownGameAudio(void);
void InitUIFont(void);
void ShutdownUIFont(void);
Vector2 MeasureTextStrongSpaced(const char *text, int fontSize, float spacing);

float GetUIScale(void)
{
    float widthScale = (float)GetScreenWidth() / (float)SCREEN_WIDTH;
    float heightScale = (float)GetScreenHeight() / (float)SCREEN_HEIGHT;
    return fminf(fmaxf(fminf(widthScale, heightScale), 0.78f), 1.22f);
}

int ScaleFontSize(float fontSize)
{
    /* Global readability boost: every label in the game (HUD, buttons, tutorial
     * panels, language selector) reads this value, so bumping it here
     * raises legibility everywhere at once instead of hunting down each
     * call site - and it stays responsive since it's still multiplied by
     * GetUIScale(), and every fitted label (DrawTextStrongFit) shrinks
     * itself back down if the extra size wouldn't fit its box. */
    #define UI_READABILITY_BOOST 1.62f
    return (int)fmaxf(13.0f, roundf(fontSize * UI_READABILITY_BOOST * GetUIScale()));
}

float GetStableFrameTime(void)
{
    return fminf(GetFrameTime(), MAX_SIMULATION_FRAME_TIME);
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
        [TEXT_HUD_EXECUTABLE] = "EXECUTAVEL",
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
        [TEXT_RELOAD_IN_PROGRESS] = "Recarregando"
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
        [TEXT_HUD_EXECUTABLE] = "EJECUTABLE",
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
        [TEXT_RELOAD_IN_PROGRESS] = "Recargando"
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
        [TEXT_HUD_EXECUTABLE] = "EXECUTABLE",
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
        [TEXT_RELOAD_IN_PROGRESS] = "Reloading"
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
        [TEXT_HUD_EXECUTABLE] = "실행 파일",
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
        [TEXT_RELOAD_IN_PROGRESS] = "재장전 중"
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
    return uiLatinFontLoaded ? uiLatinFont : GetUIFont();
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

/* UI_FONT_PATH only exists on some Linux distros. Try it first, then fall
 * back to the common Korean-capable fonts shipped with Windows and macOS,
 * so glyphs render correctly instead of falling back to the default font
 * (which has no CJK glyphs and draws "?" for every Korean character).
 *
 * The relative candidates ("src/assets/...", "assets/...") only resolve
 * when the game happens to be launched with that folder as the current
 * working directory. Double-clicking the executable, or launching it from
 * a shortcut/launcher, almost always uses a different working directory,
 * which is why Korean silently fell back to "?" glyphs even when the font
 * file was sitting right next to the .exe. BuildFontCandidatePaths()
 * below rewrites every relative candidate into an absolute path anchored
 * to the executable's own folder (GetApplicationDirectory()), so the font
 * is found regardless of how the game is launched. */
static const char *uiFontRelativeCandidates[] = {
    "NanumGothic-Regular.ttf",
    "fonts/NanumGothic-Regular.ttf",
    "src/assets/fonts/NanumGothic-Regular.ttf",
    "assets/fonts/NanumGothic-Regular.ttf",
    UI_FONT_PATH,
    "src/assets/fonts/NotoSansCJK-Regular.ttc",
    "src/assets/fonts/NotoSansKR-Regular.otf",
    "src/assets/fonts/NotoSansKR-Regular.ttf",
    "assets/fonts/NotoSansCJK-Regular.ttc",
    "assets/fonts/NotoSansKR-Regular.otf",
    "assets/fonts/NotoSansKR-Regular.ttf",
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
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
    "src/assets/fonts/DejaVuSans.ttf",
    "assets/fonts/DejaVuSans.ttf",
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
        "감갑경과규께껴꿉끄넓능대되된됩둠든들또뜻럼려맞매박방버번변별부비성센순쏘쏠쓰안압야여옵외의절접정죽줍즉착찰처충칙켭키튼퍼피함항향혀화활후";
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
        /* Bilinear filtering (smooth, not point-sampled) keeps Hangul
         * strokes clean and readable when the HUD upscales this font -
         * Korean has no "chunky pixel-art" goal, unlike the Latin font. */
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
    if (inTutorialSequence || officialRound < 15)
    {
        return 0;
    }

    return officialRound - 14;
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
    return (float)GetDifficultyRampLevel() * 1.8f;
}

float GetBossShootCooldown(void)
{
    return fmaxf(0.45f, 0.85f - ((float)GetDifficultyRampLevel() * 0.025f));
}

float GetBossFarSpeedMultiplier(void)
{
    return fminf(BOSS_FAR_SPEED_MULTIPLIER + ((float)GetDifficultyRampLevel() * 0.04f), 2.4f);
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
        float glyphSpacing = isHangul ? spacing * 0.35f : spacing;

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
        float glyphSpacing = isHangul ? spacing * 0.35f : spacing;

        char glyphBuf[8];
        int copyLen = (codepointByteCount < 7) ? codepointByteCount : 7;
        memcpy(glyphBuf, &line[byteIndex], copyLen);
        glyphBuf[copyLen] = '\0';

        Vector2 glyphSize = MeasureTextEx(font, glyphBuf, fontSize, 0.0f);
        width += glyphSize.x + glyphSpacing;

        byteIndex += codepointByteCount;
    }

    if (width > 0.0f)
    {
        width -= spacing;
    }

    return (Vector2){ width, fontSize };
}

/* Handles '\n' without faking bold with many offset copies. */
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
    Vector2 shadowPos = { (float)(x + 2), (float)(y + 2) };
    Vector2 textPos = { (float)x, (float)y };

    DrawTextBoldEx(latinFont, cjkFont, text, shadowPos, (float)fontSize, 1.0f, shadowColor);
    DrawTextBoldEx(latinFont, cjkFont, text, textPos, (float)fontSize, 1.0f, color);
}

/* Same as DrawTextStrong, but lets the caller control the space between
 * letters. Used where legibility matters most (tutorial explanations). */
void DrawTextStrongSpaced(const char *text, int x, int y, int fontSize, float spacing, Color color, Color shadowColor)
{
    Font latinFont = GetUILatinFont();
    Font cjkFont = GetUIFont();
    Vector2 shadowPos = { (float)(x + 2), (float)(y + 2) };
    Vector2 textPos = { (float)x, (float)y };

    DrawTextBoldEx(latinFont, cjkFont, text, shadowPos, (float)fontSize, spacing, shadowColor);
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
    float scale = GetMazeScale();
    float drawTileSize = (float)TILE_SIZE * scale;
    Vector2 offset = GetMazeOffset(scale);
    Rectangle mazeFrame = {
        offset.x - (drawTileSize * 0.35f),
        offset.y - (drawTileSize * 0.35f),
        (GRID_WIDTH * drawTileSize) + (drawTileSize * 0.7f),
        (GRID_HEIGTH * drawTileSize) + (drawTileSize * 0.7f)
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
}

void InitPlayer(void)
{
    player.radius = 8.0f;
    player.speed = 58.0f;
    player.facingAngle = 0.0f;
    player.position.x = TILE_SIZE * 1.5f;
    player.position.y = TILE_SIZE * 1.5f;
    playerStartAuraVisible = true;
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
        playerStartAuraVisible = false;

        float length = sqrtf(movement.x * movement.x + movement.y * movement.y);
        movement.x /= length;
        movement.y /= length;
        player.facingAngle = atan2f(movement.y, movement.x);

        MovePlayerWithCollision(movement, player.speed * GetStableFrameTime());
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
        flashlightBattery -= 2.0f * GetStableFrameTime();

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
        if (gameAudioLoaded)
        {
            PlaySound(playerReloadSound);
        }
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
    bossEnemy.speed = 42.0f + GetBossSpeedBonus();
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
    playerAmmo = PLAYER_MAX_AMMO;
    playerReloadTimer = 0.0f;

    if (inTutorialSequence)
    {
        flashlightBattery = FLASHLIGHT_MAX_BATTERY;
    }

    if (!inTutorialSequence && (officialRound == 1 || officialRound % 5 == 0))
    {
        flashlightBattery = FLASHLIGHT_MAX_BATTERY;
    }
}

void SetupRound(void)
{
    playerAlive = true;
    playerHealth = PLAYER_MAX_HEALTH;
    playerDamageCooldown = 0.0f;
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
    if (currentLanguage == LANGUAGE_EN)
    {
        return "Goal: guide the yellow triangle to the green exit.\n"
               "Move with WASD or arrow keys.\n"
               "Shoot with SPACE, reload with R, and use the top buttons for language.\n"
               "Enemies remove health, shots can knock them out for 5 seconds.\n"
               "Each tutorial step introduces one part of the game.";
    }
    if (currentLanguage == LANGUAGE_ES)
    {
        return "Objetivo: lleva el triangulo amarillo hasta la salida verde.\n"
               "Muevete con WASD o las flechas.\n"
               "Dispara con ESPACIO, recarga con R y usa los botones de arriba para el idioma.\n"
               "Los enemigos quitan vida; los disparos pueden dejarlos K.O. por 5 segundos.\n"
               "Cada tutorial presenta una parte del juego.";
    }
    if (currentLanguage == LANGUAGE_KO)
    {
        return "목표: 노란 삼각형을 초록 출구까지 이동하세요.\n"
               "WASD 또는 방향키로 움직입니다.\n"
               "스페이스로 발사, R로 재장전, 위 버튼으로 언어를 바꿉니다.\n"
               "적에게 닿으면 체력이 감소하고, 총알은 적을 5초 동안 기절시킵니다.\n"
               "튜토리얼은 게임의 규칙을 단계별로 알려줍니다.";
    }
    return "Objetivo: leve o triangulo amarelo ate a saida verde.\n"
           "Mova com WASD ou setas.\n"
           "Atire com ESPACO, recarregue com R e use os botoes do topo para idioma.\n"
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
            if (tutorialRound == 1) return "Red enemies patrol the maze and chase when close.\nTouching one deals 25 damage, not instant death.\nYou have 100 health and brief invulnerability after damage.\nDo not stand still in corridors; keep moving toward the green exit.";
            if (tutorialRound == 2) return "Pink enemies are shooters.\nIf they line up with you in a clear row or column, they stop and fire.\nUse SPACE to shoot. Five hits knock any enemy out for 5 seconds.\nYour weapon has 15 shots; press R to reload.";
            if (tutorialRound == 3) return "The flashlight is available.\nPress C to toggle it. It reveals more of the maze but spends battery.\nTutorial rounds always start with 100 percent battery.\nUse it in short bursts to check corners and find the exit.";
            return "The purple boss is the square enemy.\nIt pathfinds toward you, shoots when aligned, and speeds up if far away.\nIt can also be knocked out for 5 seconds after five hits.\nThe game limits crowding so more than two enemies should not trap you at once.";
        }
        if (currentLanguage == LANGUAGE_KO)
        {
            if (tutorialRound == 1) return "빨간 적은 미로를 순찰하고 가까우면 추격합니다.\n닿으면 즉시 죽지 않고 체력이 25 감소합니다.\n플레이어 체력은 100이고 피해 후 잠시 무적입니다.\n통로에서 멈추지 말고 초록 출구로 이동하세요.";
            if (tutorialRound == 2) return "분홍 적은 총을 쏘는 적입니다.\n같은 행이나 열에서 벽 없이 마주치면 멈추고 발사합니다.\n스페이스로 쏘세요. 적은 5번 맞으면 5초 동안 기절합니다.\n탄약은 15발이고 R로 재장전합니다.";
            if (tutorialRound == 3) return "손전등을 사용할 수 있습니다.\nC로 켜고 끕니다. 더 넓게 보이지만 배터리를 소모합니다.\n튜토리얼 라운드는 항상 배터리 100퍼센트로 시작합니다.\n모퉁이와 출구를 확인할 때 짧게 사용하세요.";
            return "보라색 사각형은 보스입니다.\n플레이어를 찾아오고, 일직선이면 쏘며, 멀어지면 빨라집니다.\n보스도 5번 맞으면 5초 동안 기절합니다.\n게임은 두 명 넘는 적이 동시에 플레이어를 막지 않게 조정합니다.";
        }
        if (currentLanguage == LANGUAGE_ES)
        {
            if (tutorialRound == 1) return "Los enemigos rojos patrullan el laberinto y persiguen de cerca.\nTocarlos causa 25 de dano, no muerte instantanea.\nTienes 100 de vida y quedas invulnerable un instante despues de recibir dano.\nNo te quedes quieto en los pasillos; avanza hacia la salida verde.";
            if (tutorialRound == 2) return "Los enemigos rosas disparan.\nSi se alinean contigo en linea recta sin pared de por medio, se detienen y disparan.\nUsa ESPACIO para disparar. Cinco impactos dejan K.O. a cualquier enemigo por 5 segundos.\nTu arma tiene 15 balas; pulsa R para recargar.";
            if (tutorialRound == 3) return "La linterna ya esta disponible.\nPulsa C para encenderla o apagarla. Revela mas del laberinto, pero gasta bateria.\nEn el tutorial, cada ronda empieza con 100 por ciento de bateria.\nUsala en rafagas cortas para revisar esquinas y encontrar la salida.";
            return "El jefe morado es el enemigo cuadrado.\nCalcula el camino hacia ti, dispara cuando se alinea y acelera si esta lejos.\nTambien puede quedar K.O. por 5 segundos tras cinco impactos.\nEl juego limita el acoso para que mas de dos enemigos no te atrapen a la vez.";
        }

        if (tutorialRound == 1) return "Inimigos vermelhos patrulham o labirinto e perseguem de perto.\nEncostar neles causa 25 de dano, nao morte instantanea.\nVoce tem 100 de vida e fica invulneravel por um instante apos dano.\nNao pare nos corredores; avance ate a saida verde.";
        if (tutorialRound == 2) return "Inimigos rosas atiram.\nSe ficarem alinhados com voce em linha reta sem parede, eles param e disparam.\nUse ESPACO para atirar. Cinco acertos nocauteiam qualquer inimigo por 5 segundos.\nSua arma tem 15 balas; pressione R para recarregar.";
        if (tutorialRound == 3) return "A lanterna foi liberada.\nPressione C para ligar ou desligar. Ela revela mais do labirinto, mas gasta bateria.\nNo tutorial, todo round comeca com 100 por cento de bateria.\nUse em rajadas curtas para checar esquinas e encontrar a saida.";
        return "O chefao roxo e o inimigo quadrado.\nEle calcula caminho ate voce, atira quando fica alinhado e acelera se estiver longe.\nEle tambem cai por 5 segundos depois de cinco acertos.\nO jogo limita cercos para mais de dois inimigos nao prenderem voce de uma vez.";
    }

    if (currentLanguage == LANGUAGE_EN)
    {
        if (officialRound == 1) return "Official run starts now.\nRounds 1 and 2 use red patrol enemies and pink shooters.\nYou start with 100 health, 15 shots, and full battery saved for later rounds.\nReach the green exit to advance.";
        if (officialRound == 3) return "Flashlight rounds begin.\nRounds 3 and 4 include red and pink enemies plus limited visibility.\nYour battery does not refill every round, so spend it carefully.\nIt refills on round 5 and then every 5 rounds.";
        if (officialRound == 5) return "Round 5 refills the flashlight battery to 100 percent.\nThis round focuses on the purple boss only.\nKeep moving, break alignment when it shoots, and hit it five times to knock it out.";
        if (officialRound == 6) return "Rounds 6 and 7 combine the boss with red enemies.\nThe boss hunts directly while red enemies pressure nearby routes.\nUse shots to create space and avoid being boxed in.";
        if (officialRound == 8) return "Rounds 8 and 9 add every enemy type except flashlight darkness.\nRed enemies chase, pink enemies shoot, and the boss accelerates from far away.\nWatch the music volume: it rises when danger is close.";
        if (officialRound == 10) return "From round 10 on, everything is active: red, pink, boss, and flashlight.\nThe battery refills only on rounds 10, 15, 20, and so on.\nPlan routes before switching the flashlight off.";
        if (officialRound == 15) return "Difficulty ramp is now active.\nEnemies become faster and use pathfinding more often each round.\nKnockouts, reload timing, and battery control matter more from here.";
        return "Cross the maze and reach the green exit.\nUse SPACE to shoot, R to reload, and C for flashlight when available.\nStay mobile and use knockouts to open a path.";
    }
    if (currentLanguage == LANGUAGE_ES)
    {
        if (officialRound == 1) return "La partida oficial empieza ahora.\nLas rondas 1 y 2 usan enemigos rojos de patrulla y rosas que disparan.\nEmpiezas con 100 de vida, 15 balas y bateria llena guardada para mas adelante.\nLlega a la salida verde para avanzar.";
        if (officialRound == 3) return "Empiezan las rondas con linterna.\nLas rondas 3 y 4 tienen rojos, rosas y vision limitada.\nLa bateria no se recarga cada ronda; usala con cuidado.\nSe recarga en la ronda 5 y luego cada 5 rondas.";
        if (officialRound == 5) return "La ronda 5 recarga la bateria de la linterna al 100 por ciento.\nEsta ronda se centra solo en el jefe morado.\nSigue moviendote, rompe la alineacion cuando dispare y acierta cinco veces para dejarlo K.O.";
        if (officialRound == 6) return "Las rondas 6 y 7 combinan al jefe con enemigos rojos.\nEl jefe te persigue directo mientras los rojos presionan las rutas cercanas.\nUsa disparos para abrir espacio y evita quedar acorralado.";
        if (officialRound == 8) return "Las rondas 8 y 9 suman todos los tipos de enemigo menos la oscuridad de la linterna.\nLos rojos persiguen, los rosas disparan y el jefe acelera cuando esta lejos.\nPresta atencion a la musica: sube cuando el peligro esta cerca.";
        if (officialRound == 10) return "Desde la ronda 10, todo esta activo: rojo, rosa, jefe y linterna.\nLa bateria solo se recarga en las rondas 10, 15, 20 y asi sucesivamente.\nPlanea la ruta antes de apagar la linterna.";
        if (officialRound == 15) return "La dificultad creciente ya esta activa.\nLos enemigos se vuelven mas rapidos y calculan camino con mas frecuencia cada ronda.\nEl K.O., el tiempo de recarga y la gestion de bateria importan mas desde aqui.";
        return "Cruza el laberinto y llega a la salida verde.\nUsa ESPACIO para disparar, R para recargar y C para la linterna cuando este disponible.\nMantente en movimiento y usa los K.O. para abrir camino.";
    }
    if (currentLanguage == LANGUAGE_KO)
    {
        if (officialRound == 1) return "공식 게임이 시작됩니다.\n라운드 1과 2에는 빨간 순찰 적과 분홍 사격 적이 나옵니다.\n체력 100, 탄약 15발로 시작하고 배터리는 이후 라운드에 대비합니다.\n초록 출구에 도착하면 다음 라운드로 갑니다.";
        if (officialRound == 3) return "손전등 라운드가 시작됩니다.\n라운드 3과 4에는 빨간 적, 분홍 적, 제한된 시야가 있습니다.\n배터리는 매 라운드 충전되지 않으니 아껴 쓰세요.\n라운드 5부터 5라운드마다 충전됩니다.";
        if (officialRound == 5) return "라운드 5에서는 손전등 배터리가 100퍼센트로 충전됩니다.\n이번 라운드는 보라색 보스 하나에 집중합니다.\n계속 움직이고, 보스가 쏠 때 일직선을 피하고, 5번 맞혀 기절시키세요.";
        if (officialRound == 6) return "라운드 6과 7은 보스와 빨간 적이 함께 나옵니다.\n보스는 직접 추적하고 빨간 적은 주변 길을 압박합니다.\n총으로 공간을 만들고 막히지 않게 움직이세요.";
        if (officialRound == 8) return "라운드 8과 9에는 손전등 어둠을 제외한 모든 적이 나옵니다.\n빨간 적은 추격하고, 분홍 적은 쏘고, 보스는 멀면 빨라집니다.\n음악이 커지면 가까운 위험이 있다는 뜻입니다.";
        if (officialRound == 10) return "라운드 10부터는 빨간 적, 분홍 적, 보스, 손전등이 모두 활성화됩니다.\n배터리는 10, 15, 20 라운드처럼 5라운드마다만 충전됩니다.\n손전등을 끄기 전에 이동 경로를 확인하세요.";
        if (officialRound == 15) return "이제 난이도 상승이 시작됩니다.\n라운드가 올라갈수록 적이 더 빠르고 더 자주 길을 계산합니다.\n기절, 재장전 타이밍, 배터리 관리가 중요합니다.";
        return "미로를 지나 초록 출구에 도착하세요.\n스페이스로 발사, R로 재장전, 가능할 때 C로 손전등을 켭니다.\n계속 움직이고 기절 시간을 이용해 길을 여세요.";
    }

    if (officialRound == 1) return "A partida oficial comeca agora.\nRounds 1 e 2 usam inimigos vermelhos de patrulha e rosas atiradores.\nVoce inicia com 100 de vida, 15 balas e bateria cheia guardada para os proximos rounds.\nChegue na saida verde para avancar.";
    if (officialRound == 3) return "Comecam os rounds com lanterna.\nRounds 3 e 4 tem vermelhos, rosas e visao limitada.\nA bateria nao recarrega todo round; use com cuidado.\nEla volta no round 5 e depois a cada 5 rounds.";
    if (officialRound == 5) return "No round 5 a bateria da lanterna volta para 100 por cento.\nEste round foca apenas no chefao roxo.\nContinue se movendo, quebre o alinhamento quando ele atirar e acerte cinco tiros para nocautea-lo.";
    if (officialRound == 6) return "Rounds 6 e 7 juntam chefao e inimigos vermelhos.\nO chefao persegue direto, enquanto os vermelhos pressionam rotas proximas.\nUse tiros para abrir espaco e evitar ficar preso.";
    if (officialRound == 8) return "Rounds 8 e 9 trazem todos os tipos de inimigo, mas sem escuridao da lanterna.\nVermelhos perseguem, rosas atiram e o chefao acelera quando esta longe.\nObserve a musica: ela aumenta quando o perigo esta perto.";
    if (officialRound == 10) return "Do round 10 em diante, tudo fica ativo: vermelho, rosa, chefao e lanterna.\nA bateria so recarrega nos rounds 10, 15, 20 e assim por diante.\nPlaneje o caminho antes de desligar a lanterna.";
    if (officialRound == 15) return "A dificuldade crescente esta ativa.\nOs inimigos ficam mais rapidos e usam caminho inteligente com mais frequencia a cada round.\nNocaute, recarga e controle da bateria passam a ser essenciais.";
    return "Atravesse o labirinto e alcance a saida verde.\nUse ESPACO para atirar, R para recarregar e C para lanterna quando disponivel.\nMantenha movimento e use nocautes para abrir caminho.";
}

bool ShouldShowRoundInfo(void)
{
    if (inTutorialSequence)
    {
        return true;
    }

    return officialRound == 1 || officialRound == 3 || officialRound == 5 ||
           officialRound == 6 || officialRound == 8 || officialRound == 10 ||
           officialRound == 15;
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

float GetNearestDangerousEnemyDistance(void)
{
    float nearestDistance = MUSIC_NEAR_ENEMY_DISTANCE;

    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (IsEnemyDangerous(&redEnemies[i]))
        {
            nearestDistance = fminf(nearestDistance, GetDistanceBetweenPoints(redEnemies[i].position, player.position));
        }
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (IsEnemyDangerous(&blueEnemies[i]))
        {
            nearestDistance = fminf(nearestDistance, GetDistanceBetweenPoints(blueEnemies[i].position, player.position));
        }
    }

    if (IsEnemyDangerous(&bossEnemy))
    {
        nearestDistance = fminf(nearestDistance, GetDistanceBetweenPoints(bossEnemy.position, player.position));
    }

    return nearestDistance;
}

void InitGameAudio(void)
{
    InitAudioDevice();
    backgroundMusic = LoadMusicStream("src/assets/audio/guitar-loops.wav");
    playerShotSound = LoadSound("src/assets/audio/laser-shot-player.wav");
    enemyShotSound = LoadSound("src/assets/audio/aser-shot-enemy.wav");
    playerReloadSound = LoadSound("src/assets/audio/recargapistola.wav");
    victorySound = LoadSound("src/assets/audio/victory.wav");
    gameOverSound = LoadSound("src/assets/audio/game_over.wav");

    SetMusicVolume(backgroundMusic, MUSIC_BASE_VOLUME);
    SetSoundVolume(playerShotSound, PLAYER_SHOT_VOLUME);
    SetSoundVolume(enemyShotSound, ENEMY_SHOT_VOLUME);
    SetSoundVolume(playerReloadSound, PLAYER_RELOAD_VOLUME);
    SetSoundVolume(victorySound, VICTORY_VOLUME);
    SetSoundVolume(gameOverSound, GAME_OVER_VOLUME);
    PlayMusicStream(backgroundMusic);
    gameAudioLoaded = true;
}

void UpdateGameAudio(void)
{
    if (!gameAudioLoaded)
    {
        return;
    }

    UpdateMusicStream(backgroundMusic);

    if (gamePhase == PHASE_PLAYING && playerAlive)
    {
        float nearestDistance = GetNearestDangerousEnemyDistance();
        float danger = 1.0f - fminf(nearestDistance / MUSIC_NEAR_ENEMY_DISTANCE, 1.0f);
        SetMusicVolume(backgroundMusic, MUSIC_BASE_VOLUME + (danger * (MUSIC_NEAR_ENEMY_VOLUME - MUSIC_BASE_VOLUME)));
    }
    else
    {
        SetMusicVolume(backgroundMusic, MUSIC_BASE_VOLUME);
    }
}

void ShutdownGameAudio(void)
{
    if (!gameAudioLoaded)
    {
        return;
    }

    UnloadMusicStream(backgroundMusic);
    UnloadSound(playerShotSound);
    UnloadSound(enemyShotSound);
    UnloadSound(playerReloadSound);
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

void DamagePlayer(int damage)
{
    if (!playerAlive || playerDamageCooldown > 0.0f)
    {
        return;
    }

    playerHealth -= damage;
    playerDamageCooldown = PLAYER_DAMAGE_COOLDOWN;

    if (playerHealth <= 0)
    {
        playerHealth = 0;
        playerAlive = false;
        if (gameAudioLoaded)
        {
            PlaySound(gameOverSound);
        }
    }
}

void ApplyEnemyTouchDamage(void)
{
    for (int i = 0; i < RED_ENEMY_COUNT; i++)
    {
        if (IsEnemyTouchingPlayer(&redEnemies[i]))
        {
            DamagePlayer(ENEMY_TOUCH_DAMAGE);
            return;
        }
    }

    for (int i = 0; i < BLUE_ENEMY_COUNT; i++)
    {
        if (IsEnemyTouchingPlayer(&blueEnemies[i]))
        {
            DamagePlayer(ENEMY_TOUCH_DAMAGE);
            return;
        }
    }

    if (IsEnemyTouchingPlayer(&bossEnemy))
    {
        DamagePlayer(ENEMY_TOUCH_DAMAGE);
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
    float healthRatio = (PLAYER_MAX_HEALTH > 0) ? ((float)playerHealth / (float)PLAYER_MAX_HEALTH) : 0.0f;
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

    DrawTextStrongFit(TextFormat(T(TEXT_HEALTH), playerHealth, PLAYER_MAX_HEALTH), (int)x, (int)(y - 25.0f * scale), healthFontSize, 12, 1.0f * scale, width, RAYWHITE, BLACK);
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
        DrawTextStrongFit(TextFormat("%d / %d", playerAmmo, PLAYER_MAX_AMMO),
                          (int)(weapon.x + weapon.width - 65.0f * scale), (int)(weapon.y + 31.0f * scale),
                          medium, 7, 0.3f * scale, 55.0f * scale, RAYWHITE, BLACK);
        DrawSegmentedBar(weapon.x + pad, weapon.y + 51.0f * scale, weapon.width - 2.0f * pad, 8.0f * scale,
                         PLAYER_MAX_AMMO, playerAmmo, (Color){ 45, 195, 255, 255 }, (Color){ 20, 35, 66, 255 }, scale);

        float bottomY = (float)GetScreenHeight() - 92.0f * scale;
        Rectangle vital = { margin, bottomY, halfWidth, 82.0f * scale };
        Rectangle controls = { margin + halfWidth + gap, bottomY, halfWidth, 82.0f * scale };

        DrawTechPanel(vital, HUD_ACCENT_COLOR);
        DrawHeartbeatIcon((Vector2){ vital.x + 16.0f * scale, vital.y + 16.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
        DrawTextStrongFit(T(TEXT_HUD_VITAL_TITLE), (int)(vital.x + 30.0f * scale), (int)(vital.y + 8.0f * scale),
                          medium, 7, 0.25f * scale, vital.width - 40.0f * scale, HUD_ACCENT_COLOR, BLACK);
        DrawTextStrongSpaced(T(TEXT_HUD_VIDA), (int)(vital.x + pad), (int)(vital.y + 34.0f * scale),
                             small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
        DrawTextStrongFit(TextFormat("%d/%d", playerHealth, PLAYER_MAX_HEALTH),
                          (int)(vital.x + 48.0f * scale), (int)(vital.y + 30.0f * scale), medium, 7, 0.25f * scale,
                          vital.width - 58.0f * scale, RAYWHITE, BLACK);
        DrawSegmentedBar(vital.x + pad, vital.y + 50.0f * scale, vital.width - 2.0f * pad, 8.0f * scale,
                         10, (int)ceilf(((float)playerHealth / PLAYER_MAX_HEALTH) * 10.0f),
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
    float equalPanelHeight = 194.0f * scale;

    Rectangle vital = { leftX, y, panelWidth, equalPanelHeight };
    DrawTechPanel(vital, HUD_ACCENT_COLOR);
    DrawHeartbeatIcon((Vector2){ vital.x + 18.0f * scale, vital.y + 17.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_VITAL_TITLE), (int)(vital.x + 32.0f * scale), (int)(vital.y + 11.0f * scale),
                   medium, ScaleFontSize(8.0f), 0.25f * scale, vital.width - 44.0f * scale,
                   HUD_ACCENT_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_VIDA), (int)(vital.x + pad), (int)(vital.y + 54.0f * scale),
                   medium, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawTextStrongFit(TextFormat("%d/%d", playerHealth, PLAYER_MAX_HEALTH),
                      (int)(vital.x + pad), (int)(vital.y + 78.0f * scale),
                      large, ScaleFontSize(11.0f), 0.35f * scale, vital.width - 2.0f * pad, RAYWHITE, BLACK);
    DrawSegmentedBar(vital.x + pad, vital.y + vital.height - 34.0f * scale, vital.width - 2.0f * pad, 14.0f * scale,
                     10, (int)ceilf(((float)playerHealth / PLAYER_MAX_HEALTH) * 10.0f),
                     (Color){ 190, 70, 255, 255 }, (Color){ 35, 25, 60, 255 }, scale);

    float rightWidth = GetHudRightPanelWidth();
    float rightX = (float)GetScreenWidth() - rightWidth - (10.0f * scale);
    float rightContentWidth = rightWidth - 2.0f * pad;

    Rectangle weapon = { rightX, 92.0f * scale, rightWidth, equalPanelHeight };
    DrawTechPanel(weapon, HUD_BORDER_COLOR);
    DrawCrosshairIcon((Vector2){ weapon.x + 18.0f * scale, weapon.y + 17.0f * scale }, 8.0f * scale, HUD_BORDER_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_WEAPON_TITLE), (int)(weapon.x + 32.0f * scale), (int)(weapon.y + 11.0f * scale),
                   medium, ScaleFontSize(8.0f), 0.25f * scale, weapon.width - 44.0f * scale,
                   HUD_BORDER_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_BALAS), (int)(weapon.x + pad), (int)(weapon.y + 54.0f * scale),
                   medium, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawTextStrongFit(TextFormat("%d / %d", playerAmmo, PLAYER_MAX_AMMO),
                      (int)(weapon.x + pad), (int)(weapon.y + 78.0f * scale),
                      large, ScaleFontSize(11.0f), 0.35f * scale, rightContentWidth, RAYWHITE, BLACK);
    DrawSegmentedBar(weapon.x + pad, weapon.y + weapon.height - 34.0f * scale, rightContentWidth, 14.0f * scale,
                     PLAYER_MAX_AMMO, playerAmmo, (Color){ 45, 195, 255, 255 }, (Color){ 20, 35, 66, 255 }, scale);
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

    Rectangle controls = { rightX, weapon.y + weapon.height + gap, rightWidth, equalPanelHeight };
    DrawTechPanel(controls, HUD_ACCENT_COLOR);
    DrawKeyboardIcon((Vector2){ controls.x + 18.0f * scale, controls.y + 17.0f * scale }, 8.0f * scale, HUD_ACCENT_COLOR);
    DrawTextStrongFit(T(TEXT_HUD_CONTROLS_TITLE), (int)(controls.x + 32.0f * scale), (int)(controls.y + 11.0f * scale),
                   medium, ScaleFontSize(8.0f), 0.15f * scale, rightWidth - 44.0f * scale,
                   HUD_ACCENT_COLOR, BLACK);
    DrawTextStrongSpaced(T(TEXT_HUD_ATIRAR), (int)(controls.x + pad), (int)(controls.y + 54.0f * scale),
                   small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawKeyCap("ESPACO", controls.x + pad, controls.y + 76.0f * scale, fminf(104.0f * scale, rightContentWidth), scale);
    DrawTextStrongSpaced(T(TEXT_HUD_RECARREGAR), (int)(controls.x + pad), (int)(controls.y + 118.0f * scale),
                   small, 0.3f * scale, (Color){ 150, 190, 240, 255 }, BLACK);
    DrawKeyCap("R", controls.x + pad, controls.y + 140.0f * scale, 44.0f * scale, scale);

    if (currentRoundConfig.flashlightEnabled)
    {
        DrawTextStrongFit(T(TEXT_HUD_LANTERNA), (int)(controls.x + 74.0f * scale), (int)(controls.y + 143.0f * scale),
                       small, ScaleFontSize(6.0f), 0.2f * scale, rightContentWidth - 76.0f * scale, HUD_ACCENT_COLOR, BLACK);
        float batteryFraction = fmaxf(0.0f, fminf(1.0f, flashlightBattery / FLASHLIGHT_MAX_BATTERY));
        DrawTextStrongFit(TextFormat("%.0f%%", flashlightBattery), (int)(controls.x + rightWidth - 62.0f * scale), (int)(controls.y + 143.0f * scale),
                       small, 6, 0.2f * scale, 42.0f * scale, RAYWHITE, BLACK);
        DrawSegmentedBar(controls.x + pad, controls.y + controls.height - 20.0f * scale, rightContentWidth, 8.0f * scale,
                         10, (int)ceilf(batteryFraction * 10.0f),
                         (Color){ 255, 205, 60, 255 }, (Color){ 60, 50, 20, 255 }, scale);
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

void DrawPlayer(void)
{
    float scale = GetMazeScale();
    Vector2 offset = GetMazeOffset(scale);
    Vector2 center = WorldToScreenPosition(player.position, scale, offset);
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

    DrawTriangle(shadowTip, shadowRight, shadowLeft, Fade(BLACK, 0.55f));
    DrawTriangleLines(tip, right, left, playerOuter);
    DrawTriangleLines(tip, right, left, playerOuter);
    DrawTriangle(tip, right, left, playerCore);
    DrawTriangleLines(tip, right, left, playerEdge);
    DrawCircleV(center, fmaxf(drawRadius * 0.18f, 2.0f), playerEdge);
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
    Vector2 mousePosition = GetMousePosition();

    if (inTutorialSequence && CheckCollisionPointRec(mousePosition, skipButton))
    {
        inTutorialSequence = false;
        tutorialRound = 1;
        officialRound = 1;
        bestOfficialRound = 1;
        StartCurrentStage();
        return true;
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

    while (!WindowShouldClose())
    {
        UpdateBuildMetrics(argv[0]);
        UpdateGameAudio();
        HandleLanguageButtons();
        HandlePanelButtons();

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
        else if (gamePhase == PHASE_PLAYING)
        {
            if (roundNeedsSetup)
            {
                SetupRound();
            }

            if (playerAlive)
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
                ApplyEnemyTouchDamage();

                if (DidPlayerReachExit())
                {
                    if (gameAudioLoaded)
                    {
                        PlaySound(victorySound);
                    }
                    AdvanceToNextStage();
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
            DrawRoundPanel(GetIntroTitle(), GetIntroBody(), T(TEXT_INTRO_FOOTER));
        }
        else if (gamePhase == PHASE_INFO)
        {
            DrawRoundPanel(GetRoundInfoTitle(), GetRoundInfoBody(), T(TEXT_ROUND_FOOTER));
        }

        DrawLanguageButtons();

        EndDrawing();
    }

    ShutdownGameAudio();
    ShutdownUIFont();
    CloseWindow();
    return 0;
}