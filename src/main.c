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
#define MUSIC_BASE_VOLUME 0.12f
#define MUSIC_NEAR_ENEMY_VOLUME 0.45f
#define MUSIC_NEAR_ENEMY_DISTANCE (TILE_SIZE * 8.0f)
#define PLAYER_SHOT_VOLUME 0.7f
#define ENEMY_SHOT_VOLUME 0.65f
#define PLAYER_RELOAD_VOLUME 0.75f
#define VICTORY_VOLUME 0.8f
#define GAME_OVER_VOLUME 0.8f
#define UI_FONT_PATH "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#define UI_FONT_SIZE 28
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
float playerDamageCooldown = 0.0f;

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
Music backgroundMusic;
Sound playerShotSound;
Sound enemyShotSound;
Sound playerReloadSound;
Sound victorySound;
Sound gameOverSound;
bool gameAudioLoaded = false;
Font uiFont;
bool uiFontLoaded = false;

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
    LANGUAGE_EN,
    LANGUAGE_KO,
    LANGUAGE_COUNT
} Language;

typedef enum TextId
{
    TEXT_LANGUAGE_NAME,
    TEXT_LANGUAGE_HINT,
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
    TEXT_INTRO_FOOTER,
    TEXT_ROUND_FOOTER,
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

void SpawnBullet(Vector2 position, Vector2 direction, float speed, bool fromPlayer);
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

const char *uiText[LANGUAGE_COUNT][TEXT_COUNT] = {
    [LANGUAGE_PT_BR] = {
        [TEXT_LANGUAGE_NAME] = "PT-BR",
        [TEXT_LANGUAGE_HINT] = "Idioma: PT-BR  |  L muda idioma",
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
        [TEXT_INTRO_FOOTER] = "Qualquer tecla ou clique inicia. L muda idioma.",
        [TEXT_ROUND_FOOTER] = "Qualquer tecla ou clique comeca. L muda idioma."
    },
    [LANGUAGE_EN] = {
        [TEXT_LANGUAGE_NAME] = "EN",
        [TEXT_LANGUAGE_HINT] = "Language: EN  |  L changes language",
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
        [TEXT_INTRO_FOOTER] = "Any key or click starts. L changes language.",
        [TEXT_ROUND_FOOTER] = "Any key or click starts. L changes language."
    },
    [LANGUAGE_KO] = {
        [TEXT_LANGUAGE_NAME] = "KO",
        [TEXT_LANGUAGE_HINT] = "언어: 한국어  |  L: 언어 변경",
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
        [TEXT_INTRO_FOOTER] = "아무 키나 클릭으로 시작. L: 언어 변경.",
        [TEXT_ROUND_FOOTER] = "아무 키나 클릭으로 시작. L: 언어 변경."
    }
};

const char koreanGlyphText[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,;:!?%-/|()"
    "언어한국어변경최고기록라운드튜토리얼체력발사스페이스탄약재장전중초손전등켜짐꺼짐배터리"
    "사망했습니다다시시작건너뛰기아무키나누르거나클릭해서을하세요"
    "목표는초록출구까지가는것입니다노란삼각형조종하고벽에막히지않게길을찾으세요"
    "빨간원닿으면체력이깎입니다적은가까이오면음악이커집니다분홍마름모총을쏩니다"
    "직선통로에서멈추면위험합니다총알발로번맞히면간녹다운됩니다"
    "손전등어두운곳밝히지만배터리를소모합니다튜토리얼언제나퍼센트공식전첫번째"
    "이후오마다충전됩니다보스보라색사각형플레이어추적하고멀어지면빨라지며사격합니다"
    "두명넘게둘러싸지않도록움직입니다"
    "목표노란삼각형을초록출구까지이동하세요또는방향키로움직입니다로쏘세요"
    "적에게닿으면체력이감소하고총알은적을동안기절시킵니다게임의규칙을단계별로알려줍니다"
    "빨간적은미로를순찰하고가까우면추격합니다즉시죽지않고감소합니다플레이어체력은이고피해후잠시무적입니다"
    "통로에서멈추지말고초록출구로이동하세요분홍적은총을쏘는적입니다같은행이나열에서벽없이마주치면멈추고발사합니다"
    "적은번맞으면동안기절합니다탄약은발이고로재장전합니다사용할수있습니다켜고끕니다더넓게보이지만배터리를소모합니다"
    "튜토리얼라운드는항상배터리퍼센트로시작합니다모퉁이와출구를확인할때짧게사용하세요"
    "보라색사각형은보스입니다찾아오고일직선이면쏘며멀어지면빨라집니다보스도번맞으면동안기절합니다"
    "게임은두명넘는적이동시에플레이어를막지않게조정합니다공식게임이시작됩니다"
    "라운드과에는빨간순찰적과분홍사격적이나옵니다체력탄약발로시작하고배터리는이후라운드에대비합니다"
    "도착하면다음라운드로갑니다손전등라운드가시작됩니다제한된시야가있습니다매라운드충전되지않으니아껴쓰세요"
    "라운드부터마다충전됩니다이번라운드는보라색보스하나에집중합니다계속움직이고보스가쏠때일직선을피하고맞혀기절시키세요"
    "함께나옵니다보스는직접추적하고빨간적은주변길을압박합니다총으로공간을만들고막히지않게움직이세요"
    "손전등어둠을제외한모든적이나옵니다추격하고쏘고멀면빨라집니다음악이커지면가까운위험이있다는뜻입니다"
    "부터는빨간적분홍적보스손전등이모두활성화됩니다처럼마다만충전됩니다끄기전에이동경로를확인하세요"
    "이제난이도상승이시작됩니다올라갈수록적이더빠르고더자주길을계산합니다타이밍관리가중요합니다"
    "미로를지나도착하세요가능할때변경계속움직이고기절시간을이용해길을여세요"
    "바꿉클릭으로";

const char *T(TextId id)
{
    return uiText[currentLanguage][id];
}

Font GetUIFont(void)
{
    return uiFontLoaded ? uiFont : GetFontDefault();
}

/* UI_FONT_PATH only exists on some Linux distros. Try it first, then fall
 * back to the common Korean-capable fonts shipped with Windows and macOS,
 * so glyphs render correctly instead of falling back to the default font
 * (which has no CJK glyphs and draws "?" for every Korean character). */
static const char *uiFontCandidatePaths[] = {
    UI_FONT_PATH,
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    "C:\\Windows\\Fonts\\malgun.ttf",
    "C:\\Windows\\Fonts\\malgunbd.ttf",
    "/System/Library/Fonts/Supplemental/AppleSDGothicNeo.ttc",
    "/System/Library/Fonts/AppleSDGothicNeo.ttc",
    NULL
};

void InitUIFont(void)
{
    int codepointCount = 0;
    int *codepoints = LoadCodepoints(koreanGlyphText, &codepointCount);

    for (int i = 0; uiFontCandidatePaths[i] != NULL; i++)
    {
        if (!FileExists(uiFontCandidatePaths[i]))
        {
            continue;
        }

        uiFont = LoadFontEx(uiFontCandidatePaths[i], UI_FONT_SIZE, codepoints, codepointCount);
        if (uiFont.texture.id > 0)
        {
            break;
        }
    }

    UnloadCodepoints(codepoints);
    uiFontLoaded = uiFont.texture.id > 0;

    if (uiFontLoaded)
    {
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
    }
}

void ShutdownUIFont(void)
{
    if (uiFontLoaded)
    {
        UnloadFont(uiFont);
        uiFontLoaded = false;
    }
}

float GetDistanceBetweenPoints(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf((dx * dx) + (dy * dy));
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

/* Draws text as bold by stacking several 1px-offset copies before the
 * final pass, thickening every stroke instead of relying on the font's
 * own (often thin) regular weight. */
void DrawTextBoldEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color color)
{
    static const Vector2 boldOffsets[] = {
        { -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, -1.0f }, { 0.0f, 1.0f },
        { -1.0f, -1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }
    };

    for (int i = 0; i < 8; i++)
    {
        Vector2 offsetPos = { position.x + boldOffsets[i].x, position.y + boldOffsets[i].y };
        DrawTextEx(font, text, offsetPos, fontSize, spacing, color);
    }

    DrawTextEx(font, text, position, fontSize, spacing, color);
}

void DrawTextStrong(const char *text, int x, int y, int fontSize, Color color, Color shadowColor)
{
    Font font = GetUIFont();
    Vector2 shadowPos = { (float)(x + 2), (float)(y + 2) };
    Vector2 textPos = { (float)x, (float)y };

    DrawTextBoldEx(font, text, shadowPos, (float)fontSize, 1.0f, shadowColor);
    DrawTextBoldEx(font, text, textPos, (float)fontSize, 1.0f, color);
}

/* Same as DrawTextStrong, but lets the caller control the space between
 * letters. Used where legibility matters most (tutorial explanations). */
void DrawTextStrongSpaced(const char *text, int x, int y, int fontSize, float spacing, Color color, Color shadowColor)
{
    Font font = GetUIFont();
    Vector2 shadowPos = { (float)(x + 2), (float)(y + 2) };
    Vector2 textPos = { (float)x, (float)y };

    DrawTextBoldEx(font, text, shadowPos, (float)fontSize, spacing, shadowColor);
    DrawTextBoldEx(font, text, textPos, (float)fontSize, spacing, color);
}

/* Measures multiline text drawn with DrawTextStrongSpaced so panels can
 * size themselves around it instead of guessing a fixed height. */
Vector2 MeasureTextStrongSpaced(const char *text, int fontSize, float spacing)
{
    Font font = GetUIFont();
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
            if (gameAudioLoaded)
            {
                PlaySound(fromPlayer ? playerShotSound : enemyShotSound);
            }
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
               "Shoot with SPACE, reload with R, change language with L.\n"
               "Enemies remove health, shots can knock them out for 5 seconds.\n"
               "Each tutorial step introduces one part of the game.";
    }
    if (currentLanguage == LANGUAGE_KO)
    {
        return "목표: 노란 삼각형을 초록 출구까지 이동하세요.\n"
               "WASD 또는 방향키로 움직입니다.\n"
               "스페이스로 발사, R로 재장전, L로 언어를 바꿉니다.\n"
               "적에게 닿으면 체력이 감소하고, 총알은 적을 5초 동안 기절시킵니다.\n"
               "튜토리얼은 게임의 규칙을 단계별로 알려줍니다.";
    }
    return "Objetivo: leve o triangulo amarelo ate a saida verde.\n"
           "Mova com WASD ou setas.\n"
           "Atire com ESPACO, recarregue com R e mude idioma com L.\n"
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
        return "Cross the maze and reach the green exit.\nUse SPACE to shoot, R to reload, C for flashlight when available, and L for language.\nStay mobile and use knockouts to open a path.";
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
        return "미로를 지나 초록 출구에 도착하세요.\n스페이스로 발사, R로 재장전, 가능할 때 C로 손전등, L로 언어 변경.\n계속 움직이고 기절 시간을 이용해 길을 여세요.";
    }

    if (officialRound == 1) return "A partida oficial comeca agora.\nRounds 1 e 2 usam inimigos vermelhos de patrulha e rosas atiradores.\nVoce inicia com 100 de vida, 15 balas e bateria cheia guardada para os proximos rounds.\nChegue na saida verde para avancar.";
    if (officialRound == 3) return "Comecam os rounds com lanterna.\nRounds 3 e 4 tem vermelhos, rosas e visao limitada.\nA bateria nao recarrega todo round; use com cuidado.\nEla volta no round 5 e depois a cada 5 rounds.";
    if (officialRound == 5) return "No round 5 a bateria da lanterna volta para 100 por cento.\nEste round foca apenas no chefao roxo.\nContinue se movendo, quebre o alinhamento quando ele atirar e acerte cinco tiros para nocautea-lo.";
    if (officialRound == 6) return "Rounds 6 e 7 juntam chefao e inimigos vermelhos.\nO chefao persegue direto, enquanto os vermelhos pressionam rotas proximas.\nUse tiros para abrir espaco e evitar ficar preso.";
    if (officialRound == 8) return "Rounds 8 e 9 trazem todos os tipos de inimigo, mas sem escuridao da lanterna.\nVermelhos perseguem, rosas atiram e o chefao acelera quando esta longe.\nObserve a musica: ela aumenta quando o perigo esta perto.";
    if (officialRound == 10) return "Do round 10 em diante, tudo fica ativo: vermelho, rosa, chefao e lanterna.\nA bateria so recarrega nos rounds 10, 15, 20 e assim por diante.\nPlaneje o caminho antes de desligar a lanterna.";
    if (officialRound == 15) return "A dificuldade crescente esta ativa.\nOs inimigos ficam mais rapidos e usam caminho inteligente com mais frequencia a cada round.\nNocaute, recarga e controle da bateria passam a ser essenciais.";
    return "Atravesse o labirinto e alcance a saida verde.\nUse ESPACO para atirar, R para recarregar, C para lanterna quando disponivel e L para idioma.\nMantenha movimento e use nocautes para abrir caminho.";
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
            Vector2 shotOrigin = bossEnemy.position;
            shotOrigin.x += shotDirection.x * (bossEnemy.radius + 6.0f);
            shotOrigin.y += shotDirection.y * (bossEnemy.radius + 6.0f);
            SpawnBullet(shotOrigin, shotDirection, BOSS_BULLET_SPEED, false);
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
    backgroundMusic = LoadMusicStream("src/guitar-loops.wav");
    playerShotSound = LoadSound("src/laser-shot-player.wav");
    enemyShotSound = LoadSound("src/aser-shot-enemy.wav");
    playerReloadSound = LoadSound("src/recargapistola.wav");
    victorySound = LoadSound("src/victory.wav");
    gameOverSound = LoadSound("src/game_over.wav");

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

    return GetDistanceBetweenPoints(enemy->position, player.position) <= enemy->radius + player.radius;
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
            float dx = player.position.x - bullets[i].position.x;
            float dy = player.position.y - bullets[i].position.y;
            float distance = sqrtf((dx * dx) + (dy * dy));

            if (distance <= player.radius + bullets[i].radius)
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

void DrawPlayerHealthBar(float x, float y)
{
    Rectangle barBackground = { x, y, 216.0f, 20.0f };
    Rectangle barFill = barBackground;
    Color healthColor = LIME;

    barFill.width = (barBackground.width * (float)playerHealth) / (float)PLAYER_MAX_HEALTH;

    float healthRatio = (PLAYER_MAX_HEALTH > 0) ? ((float)playerHealth / (float)PLAYER_MAX_HEALTH) : 0.0f;

    if (healthRatio <= 0.3f)
    {
        healthColor = RED;
    }
    else if (healthRatio <= 0.6f)
    {
        healthColor = ORANGE;
    }

    DrawTextStrong(TextFormat(T(TEXT_HEALTH), playerHealth, PLAYER_MAX_HEALTH), (int)x, (int)(y - 24.0f), 18, RAYWHITE, BLACK);
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

    Rectangle panel = { panelX, panelY, panelWidth, currentRoundConfig.flashlightEnabled ? 448.0f : 374.0f };

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
    DrawTextStrong(TextFormat(T(TEXT_BEST_ROUND), bestOfficialRound), (int)textX, (int)rowY, 18, GOLD, BLACK);
    rowY += 26.0f;
    DrawTextStrong(inTutorialSequence ? TextFormat(T(TEXT_TUTORIAL_PROGRESS), tutorialRound) : TextFormat(T(TEXT_ROUND_PROGRESS), officialRound), (int)textX, (int)rowY, 20, RAYWHITE, BLACK);
    rowY += 48.0f;

    /* Health row (label is drawn above its own bar inside the function) */
    DrawPlayerHealthBar(textX, rowY);
    rowY += 42.0f;

    DrawLineEx((Vector2){ panelX + 10.0f, rowY }, (Vector2){ panelX + panelWidth - 10.0f, rowY }, 1.0f, Fade(HUD_BORDER_COLOR, 0.4f));
    rowY += 14.0f;

    /* Controls row */
    DrawTextStrong(T(TEXT_SHOOT), (int)textX, (int)rowY, 20, ORANGE, BLACK);
    rowY += 28.0f;
    DrawTextStrong(TextFormat(T(TEXT_AMMO), playerAmmo, PLAYER_MAX_AMMO), (int)textX, (int)rowY, 18, RAYWHITE, BLACK);
    rowY += 22.0f;
    if (playerReloadTimer > 0.0f)
    {
        DrawTextStrong(TextFormat(T(TEXT_RELOADING), playerReloadTimer), (int)textX, (int)rowY, 18, YELLOW, BLACK);
    }
    else
    {
        DrawTextStrong(T(TEXT_RELOAD), (int)textX, (int)rowY, 18, SKYBLUE, BLACK);
    }
    rowY += 24.0f;

    if (currentRoundConfig.flashlightEnabled)
    {
        DrawTextStrong(T(TEXT_FLASHLIGHT_CONTROL), (int)textX, (int)rowY, 18, GOLD, BLACK);
        rowY += 24.0f;
        DrawTextStrong(TextFormat(T(TEXT_FLASHLIGHT_STATE), flashlightOn ? T(TEXT_FLASHLIGHT_ON) : T(TEXT_FLASHLIGHT_OFF)), (int)textX, (int)rowY, 18, flashlightOn ? GREEN : GRAY, BLACK);
        rowY += 24.0f;
        DrawTextStrong(TextFormat(T(TEXT_BATTERY), flashlightBattery), (int)textX, (int)rowY, 18, GREEN, BLACK);
        rowY += 24.0f;
    }

    DrawTextStrong(T(TEXT_LANGUAGE_HINT), (int)textX, (int)rowY, 15, LIGHTGRAY, BLACK);
}

void DrawGameOverOverlay(void)
{
    const char *title = T(TEXT_GAME_OVER);
    const char *buttonText = T(TEXT_PLAY_AGAIN);
    int titleFontSize = 42;
    int buttonFontSize = 24;
    int titleWidth = (int)MeasureTextStrongSpaced(title, titleFontSize, 1.0f).x;
    int buttonTextWidth = (int)MeasureTextStrongSpaced(buttonText, buttonFontSize, 1.0f).x;
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
    DrawTextStrong(title, (int)(panel.x + (panel.width - titleWidth) * 0.5f), (int)panel.y + 44, titleFontSize, RAYWHITE, BLACK);
    DrawRectangleRounded(button, 0.3f, 12, buttonColor);
    DrawTextStrong(buttonText, (int)(button.x + (button.width - buttonTextWidth) * 0.5f), (int)(button.y + 14.0f), buttonFontSize, BLACK, Fade(WHITE, 0.25f));

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
    int titleWidth = (int)MeasureTextStrongSpaced(title, 30, 1.0f).x;
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
        DrawTextStrong(T(TEXT_SKIP_TUTORIAL), (int)skipButton.x + 20, (int)skipButton.y + 9, 20, RAYWHITE, BLACK);
    }
}

void CycleLanguage(void)
{
    currentLanguage = (Language)(((int)currentLanguage + 1) % (int)LANGUAGE_COUNT);
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
        bool languagePressed = IsKeyPressed(KEY_L);

        if (languagePressed)
        {
            CycleLanguage();
        }

        if (gamePhase == PHASE_INTRO)
        {
            if (!languagePressed && (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
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

            if (!languagePressed && (GetKeyPressed() != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
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
            DrawRoundPanel(GetIntroTitle(), GetIntroBody(), T(TEXT_INTRO_FOOTER));
        }
        else if (gamePhase == PHASE_INFO)
        {
            DrawRoundPanel(GetRoundInfoTitle(), GetRoundInfoBody(), T(TEXT_ROUND_FOOTER));
        }

        EndDrawing();
    }

    ShutdownGameAudio();
    ShutdownUIFont();
    CloseWindow();
    return 0;
}