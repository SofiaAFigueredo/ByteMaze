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
/* Baked at a low pixel size on purpose: the game draws it upscaled at
 * whatever size each label needs, and with point-sampling (no bilinear
 * filtering) that upscale turns the glyph edges chunky and blocky for a
 * retro pixel-art look instead of smooth modern type. */
#define UI_FONT_BAKE_SIZE 14
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

Color HUD_PANEL_COLOR = { 8, 10, 22, 225 };
Color HUD_BORDER_COLOR = { 60, 210, 255, 255 };
Color HUD_ACCENT_COLOR = { 190, 90, 255, 255 };
Color MAZE_WALL_COLOR = { 20, 22, 36, 255 };
Color MAZE_PATH_COLOR = { 235, 240, 250, 255 };
Color MAZE_SHADOW_COLOR = { 8, 9, 18, 255 };
Color MAZE_EXIT_COLOR = { 60, 230, 130, 255 };

float GetUIScale(void);

/* The HUD panel's width is fixed (not shrunk to fit leftover space) so
 * its text never has to compress past its readable floor and overflow
 * the border. The maze reserves this much on the left when it lays
 * itself out, so there's always room and the panel never overlaps it. */
float GetHudPanelWidth(void)
{
    float scale = GetUIScale();
    return fminf((float)GetScreenWidth() - (20.0f * scale), 408.0f * scale);
}

float GetMazeLeftReservedWidth(void)
{
    float scale = GetUIScale();
    return (10.0f * scale) + GetHudPanelWidth() + (20.0f * scale);
}

float GetMazeScale(void)
{
    float mazeWidth = (float)(GRID_WIDTH * TILE_SIZE);
    float mazeHeight = (float)(GRID_HEIGTH * TILE_SIZE);
    float leftReserved = GetMazeLeftReservedWidth();
    float availableWidth = (float)GetScreenWidth() - leftReserved - MAZE_PADDING;
    float availableHeight = (float)GetScreenHeight() - HUD_HEIGHT - (MAZE_PADDING * 2.0f);
    float scaleX = availableWidth / mazeWidth;
    float scaleY = availableHeight / mazeHeight;
    return fminf(scaleX, scaleY);
}

Vector2 GetMazeOffset(float scale)
{
    float mazeWidth = (float)(GRID_WIDTH * TILE_SIZE) * scale;
    float mazeHeight = (float)(GRID_HEIGTH * TILE_SIZE) * scale;
    float leftReserved = GetMazeLeftReservedWidth();
    float freeWidth = (float)GetScreenWidth() - leftReserved - mazeWidth;
    float freeHeight = (float)GetScreenHeight() - HUD_HEIGHT - mazeHeight;

    Vector2 offset = { 0 };
    offset.x = leftReserved + (freeWidth * 0.5f);
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
    return (int)fmaxf(10.0f, roundf(fontSize * GetUIScale()));
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
        [TEXT_ROUND_FOOTER] = "Clique em Continuar para jogar este passo."
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
        [TEXT_ROUND_FOOTER] = "Haz clic en Continuar para jugar este paso."
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
        [TEXT_ROUND_FOOTER] = "Click Continue to play this step."
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
        [TEXT_ROUND_FOOTER] = "계속을 클릭해 이 단계를 플레이하세요."
    }
};

static const char koreanGlyphText[] =
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

static const char uiGlyphText[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,;:!?%-/|()"
    "áàâãéêíóôõúçÁÀÂÃÉÊÍÓÔÕÚÇüÜñÑ"
    "언어한국어변경최고기록라운드튜토리얼체력발사스페이스탄약재장전중초손전등켜짐꺼짐배터리"
    "사망했습니다다시시작건너뛰기아무키나클릭으로목표노란삼각형을초록출구까지이동하세요"
    "또는방향키움직입니다적에게닿으면감소하고총알동안기절시킵니다게임의규칙단계별알려줍니다"
    "빨간미로순찰가까우면추격즉시죽지않고플레이어피해후잠시무적통로에서멈추지말고"
    "분홍총쏘는같은행이나열에서벽없이마주치면멈추고쏩니다번맞으면기절합니다"
    "사용할수있습니다더넓게보이지만소모합니다항상퍼센트로모퉁이와확인할때짧게"
    "보라색사각형보스찾아오고일직선이면쏘며멀어지면빨라집니다두명넘는동시에막지않게조정"
    "공식게임시작됩니다과에는나옵니다도착하면다음갑니다제한된시야매충전되지않으니아껴쓰세요"
    "부터마다충전이번하나에집중계속피하고맞혀함께주변길을압박공간을만들고"
    "어둠제외한모든음악커지면가까운위험뜻활성화처럼끄기전에경로난이도상승올라갈수록"
    "더빠르고자주계산타이밍관리가중요지나가능할때시간이용해길을여세요";

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
 * (which has no CJK glyphs and draws "?" for every Korean character). */
static const char *uiFontCandidatePaths[] = {
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

/* Pixel/retro display font (Press Start 2P), used for every non-Korean
 * character. Ship PressStart2P-Regular.ttf next to the executable (or in
 * an "assets" folder beside it) for this to be found. */
static const char *uiLatinFontCandidatePaths[] = {
    "src/assets/PressStart2P-Regular.ttf",
    "src/assets/fonts/PressStart2P-Regular.ttf",
    "PressStart2P-Regular.ttf",
    "assets/PressStart2P-Regular.ttf",
    "resources/PressStart2P-Regular.ttf",
    "fonts/PressStart2P-Regular.ttf",
    NULL
};

int *BuildUICodepoints(int *codepointCount)
{
    return LoadCodepoints(uiGlyphText, codepointCount);
}

void InitUIFont(void)
{
    int codepointCount = 0;
    int *codepoints = BuildUICodepoints(&codepointCount);

    for (int i = 0; uiFontCandidatePaths[i] != NULL; i++)
    {
        if (!FileExists(uiFontCandidatePaths[i]))
        {
            continue;
        }

        uiFont = LoadFontEx(uiFontCandidatePaths[i], UI_FONT_BAKE_SIZE, codepoints, codepointCount);
        if (uiFont.texture.id > 0)
        {
            break;
        }
    }

    UnloadCodepoints(codepoints);
    uiFontLoaded = uiFont.texture.id > 0;

    if (uiFontLoaded)
    {
        /* Point filtering (no smoothing) keeps the upscaled glyphs crisp
         * and blocky instead of blurry, which is what sells the pixel
         * look at the larger sizes the HUD actually draws with. */
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_POINT);
    }

    /* Build the Latin codepoint set: full printable ASCII plus the
     * accented characters Portuguese needs (the pixel font's default
     * charset only covers ASCII). */
    int latinCodepoints[160];
    int latinCount = 0;
    for (int c = 32; c <= 126; c++)
    {
        latinCodepoints[latinCount++] = c;
    }
    int extraCount = 0;
    int *extraCodepoints = LoadCodepoints("áàâãéêíóôõúçÁÀÂÃÉÊÍÓÔÕÚÇüÜñÑ", &extraCount);
    for (int i = 0; i < extraCount && latinCount < 160; i++)
    {
        latinCodepoints[latinCount++] = extraCodepoints[i];
    }
    UnloadCodepoints(extraCodepoints);

    for (int i = 0; uiLatinFontCandidatePaths[i] != NULL; i++)
    {
        if (!FileExists(uiLatinFontCandidatePaths[i]))
        {
            continue;
        }

        uiLatinFont = LoadFontEx(uiLatinFontCandidatePaths[i], UI_FONT_BAKE_SIZE * 2, latinCodepoints, latinCount);
        if (uiLatinFont.texture.id > 0)
        {
            break;
        }
    }

    uiLatinFontLoaded = uiLatinFont.texture.id > 0;

    if (uiLatinFontLoaded)
    {
        SetTextureFilter(uiLatinFont.texture, TEXTURE_FILTER_POINT);
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

/* Draws one line (no '\n') of text, picking the pixel Latin font or the
 * CJK font per-codepoint, since a single font file can't provide both a
 * pixel-art look and Korean glyph coverage. */
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
        float drawFontSize = isHangul ? fontSize * 1.02f : fontSize;
        float drawY = isHangul ? position.y - fontSize * 0.02f : position.y;
        float glyphSpacing = isHangul ? spacing * 0.22f : spacing;

        char glyphBuf[8];
        int copyLen = (codepointByteCount < 7) ? codepointByteCount : 7;
        memcpy(glyphBuf, &line[byteIndex], copyLen);
        glyphBuf[copyLen] = '\0';

        DrawTextEx(font, glyphBuf, (Vector2){ x, drawY }, drawFontSize, 0.0f, currentDrawColor);
        Vector2 glyphSize = MeasureTextEx(font, glyphBuf, drawFontSize, 0.0f);
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
        float measureFontSize = isHangul ? fontSize * 1.02f : fontSize;
        float glyphSpacing = isHangul ? spacing * 0.22f : spacing;

        char glyphBuf[8];
        int copyLen = (codepointByteCount < 7) ? codepointByteCount : 7;
        memcpy(glyphBuf, &line[byteIndex], copyLen);
        glyphBuf[copyLen] = '\0';

        Vector2 glyphSize = MeasureTextEx(font, glyphBuf, measureFontSize, 0.0f);
        width += glyphSize.x + glyphSpacing;

        byteIndex += codepointByteCount;
    }

    if (width > 0.0f)
    {
        width -= spacing;
    }

    return (Vector2){ width, fontSize };
}

/* Handles '\n' and mixes the pixel Latin font with the CJK font
 * per-character. No longer fakes bold with offset copies — that muddied
 * the pixel font's crisp strokes into an unreadable blob at small sizes. */
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
        DrawRectangleRec(bounds, (Color){ 0, 150, 70, 255 });
        Vector2 top = { bounds.x + bounds.width * 0.5f, bounds.y + 3.0f };
        Vector2 right = { bounds.x + bounds.width - 5.0f, bounds.y + bounds.height * 0.5f };
        Vector2 bottom = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height - 3.0f };
        Vector2 left = { bounds.x + 5.0f, bounds.y + bounds.height * 0.5f };
        Vector2 center = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
        DrawTriangle(top, right, bottom, GOLD);
        DrawTriangle(top, bottom, left, GOLD);
        DrawCircleV(center, bounds.height * 0.26f, (Color){ 20, 65, 160, 255 });
        DrawLineEx((Vector2){ center.x - bounds.width * 0.12f, center.y - 1.0f }, (Vector2){ center.x + bounds.width * 0.12f, center.y + 1.0f }, 2.0f, RAYWHITE);
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
    float gap = 8.0f * scale;
    float availableWidth = (float)GetScreenWidth() - (20.0f * scale);
    float buttonWidth = 140.0f * scale;
    float buttonHeight = 34.0f * scale;
    float totalWidth = ((float)LANGUAGE_COUNT * buttonWidth) + ((float)(LANGUAGE_COUNT - 1) * gap);

    if (totalWidth > availableWidth)
    {
        buttonWidth = (availableWidth - ((float)(LANGUAGE_COUNT - 1) * gap)) / (float)LANGUAGE_COUNT;
        totalWidth = availableWidth;
    }

    float startX = ((float)GetScreenWidth() - totalWidth) * 0.5f;

    return (Rectangle){
        startX + ((float)languageIndex * (buttonWidth + gap)),
        10.0f * scale,
        buttonWidth,
        buttonHeight
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

    for (int i = 0; i < (int)LANGUAGE_COUNT; i++)
    {
        Rectangle button = GetLanguageButtonRect(i);
        bool selected = currentLanguage == (Language)i;
        bool hovered = CheckCollisionPointRec(mousePosition, button);
        Color fillColor = selected ? (Color){ 0, 130, 54, 245 } : (Color){ 18, 48, 30, 235 };
        Color borderColor = selected ? LIME : HUD_BORDER_COLOR;
        const char *label = uiText[i][TEXT_LANGUAGE_BUTTON];
        float scale = GetUIScale();
        int fontSize = ScaleFontSize(14.0f);
        Rectangle flag = {
            button.x + button.width - (46.0f * scale),
            button.y + (6.0f * scale),
            36.0f * scale,
            22.0f * scale
        };

        if (hovered && !selected)
        {
            fillColor = (Color){ 25, 80, 44, 245 };
        }

        DrawRectangleRounded(button, 0.18f, 10, fillColor);
        DrawRectangleRoundedLinesEx(button, 0.18f, 10, selected ? 3.0f : 2.0f, borderColor);
        DrawTextStrong(label, (int)(button.x + 14.0f * scale), (int)(button.y + 9.0f * scale), fontSize, RAYWHITE, BLACK);
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

void DrawHud(void)
{
    /* Clear vertical rhythm: one purpose per row, a divider between the
     * "brand" block and the stats block, and consistent left padding so
     * nothing overlaps or crowds the row above/below it (the old layout
     * used fixed y-values that collided once the title got bigger). */
    float scale = GetUIScale();
    float panelX = 10.0f * scale;
    float panelY = 58.0f * scale;
    float panelWidth = GetHudPanelWidth();
    float padding = 16.0f * scale;
    float textX = panelX + padding;
    float contentWidth = panelWidth - (padding * 2.0f);
    float rowY = panelY + padding;
    float rowGapSmall = 9.0f * scale;
    float rowGap = 24.0f * scale;
    float dividerInset = 10.0f * scale;
    float panelHeight = currentRoundConfig.flashlightEnabled ? 484.0f * scale : 412.0f * scale;
    int titleFontSize = ScaleFontSize(25.0f);
    int metricFontSize = ScaleFontSize(15.0f);
    int mediumFontSize = ScaleFontSize(18.0f);
    int progressFontSize = ScaleFontSize(20.0f);

    Rectangle panel = { panelX, panelY, panelWidth, panelHeight };

    DrawRectangleRounded(panel, 0.06f, 10, HUD_PANEL_COLOR);
    DrawRectangleRoundedLinesEx(panel, 0.06f, 10, 2.0f * scale, HUD_BORDER_COLOR);

    /* Title row */
    float iconRadius = 11.0f * scale;
    Vector2 iconCenter = { textX + iconRadius, rowY + iconRadius * 0.85f };
    DrawHudIcon(iconCenter, iconRadius, HUD_BORDER_COLOR);
    DrawTextStrongFit("BYTEMAZE", (int)(textX + iconRadius * 2.6f), (int)rowY, titleFontSize, 16, 1.5f * scale, contentWidth - iconRadius * 2.6f, (Color){ 120, 235, 255, 255 }, BLACK);
    rowY += 34.0f * scale;
    DrawTextStrongFit(TextFormat("%lld bytes   %.2f%%", executableSizeBytes, executableUsagePercent), (int)textX, (int)rowY, metricFontSize, 10, 1.0f * scale, contentWidth, LIGHTGRAY, BLACK);
    rowY += 26.0f * scale;

    DrawLineEx((Vector2){ panelX + dividerInset, rowY }, (Vector2){ panelX + panelWidth - dividerInset, rowY }, 1.0f * scale, Fade(HUD_BORDER_COLOR, 0.4f));
    rowY += 14.0f * scale;

    /* Progress row */
    DrawTextStrongFit(TextFormat(T(TEXT_BEST_ROUND), bestOfficialRound), (int)textX, (int)rowY, mediumFontSize, 11, 1.0f * scale, contentWidth, (Color){ 190, 90, 255, 255 }, BLACK);
    rowY += 25.0f * scale;
    DrawTextStrongFit(inTutorialSequence ? TextFormat(T(TEXT_TUTORIAL_PROGRESS), tutorialRound) : TextFormat(T(TEXT_ROUND_PROGRESS), officialRound), (int)textX, (int)rowY, progressFontSize, 12, 1.0f * scale, contentWidth, RAYWHITE, BLACK);
    rowY += 49.0f * scale;

    /* Health row (label is drawn above its own bar inside the function) */
    DrawPlayerHealthBar(textX, rowY, fminf(250.0f * scale, contentWidth), scale);
    rowY += 43.0f * scale;

    DrawLineEx((Vector2){ panelX + dividerInset, rowY }, (Vector2){ panelX + panelWidth - dividerInset, rowY }, 1.0f * scale, Fade(HUD_BORDER_COLOR, 0.4f));
    rowY += 14.0f * scale;

    /* Controls row */
    DrawTextStrongFit(T(TEXT_SHOOT), (int)textX, (int)rowY, progressFontSize, 12, 1.0f * scale, contentWidth, (Color){ 190, 90, 255, 255 }, BLACK);
    rowY += 28.0f * scale;
    DrawTextStrongFit(TextFormat(T(TEXT_AMMO), playerAmmo, PLAYER_MAX_AMMO), (int)textX, (int)rowY, mediumFontSize, 11, 1.0f * scale, contentWidth, RAYWHITE, BLACK);
    rowY += 21.0f * scale;
    DrawSegmentedBar(textX, rowY, fminf(250.0f * scale, contentWidth), 12.0f * scale, PLAYER_MAX_AMMO, playerAmmo, (Color){ 80, 200, 255, 255 }, (Color){ 30, 34, 52, 255 }, scale);
    rowY += 24.0f * scale;
    if (playerReloadTimer > 0.0f)
    {
        DrawTextStrongFit(TextFormat(T(TEXT_RELOADING), playerReloadTimer), (int)textX, (int)rowY, mediumFontSize, 11, 1.0f * scale, contentWidth, YELLOW, BLACK);
    }
    else
    {
        DrawTextStrongFit(T(TEXT_RELOAD), (int)textX, (int)rowY, mediumFontSize, 11, 1.0f * scale, contentWidth, SKYBLUE, BLACK);
    }
    rowY += rowGap;

    if (currentRoundConfig.flashlightEnabled)
    {
        DrawTextStrongFit(T(TEXT_FLASHLIGHT_CONTROL), (int)textX, (int)rowY, mediumFontSize, 11, 1.0f * scale, contentWidth, (Color){ 190, 90, 255, 255 }, BLACK);
        rowY += rowGapSmall + (15.0f * scale);
        DrawTextStrongFit(TextFormat(T(TEXT_FLASHLIGHT_STATE), flashlightOn ? T(TEXT_FLASHLIGHT_ON) : T(TEXT_FLASHLIGHT_OFF)), (int)textX, (int)rowY, mediumFontSize, 11, 1.0f * scale, contentWidth, flashlightOn ? (Color){ 80, 230, 255, 255 } : GRAY, BLACK);
        rowY += rowGapSmall + (15.0f * scale);
        DrawTextStrongFit(TextFormat(T(TEXT_BATTERY), flashlightBattery), (int)textX, (int)rowY, mediumFontSize, 11, 1.0f * scale, contentWidth, (Color){ 80, 230, 255, 255 }, BLACK);
        rowY += rowGap;
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
    float panelHeight = 230.0f * scale;
    Rectangle panel = {
        ((float)screenWidth - panelWidth) * 0.5f,
        ((float)screenHeight - panelHeight) * 0.5f,
        panelWidth,
        panelHeight
    };
    Rectangle button = {
        panel.x + (70.0f * scale),
        panel.y + (136.0f * scale),
        panel.width - (140.0f * scale),
        58.0f * scale
    };
    titleFontSize = FitFontSizeToWidth(title, titleFontSize, ScaleFontSize(24.0f), 1.0f * scale, panel.width - (44.0f * scale));
    buttonFontSize = FitFontSizeToWidth(buttonText, buttonFontSize, ScaleFontSize(15.0f), 1.0f * scale, button.width - (24.0f * scale));
    int titleWidth = (int)MeasureTextStrongSpaced(title, titleFontSize, 1.0f * scale).x;
    int buttonTextWidth = (int)MeasureTextStrongSpaced(buttonText, buttonFontSize, 1.0f * scale).x;
    Vector2 mousePosition = GetMousePosition();
    bool isButtonHovered = CheckCollisionPointRec(mousePosition, button);
    Color buttonColor = isButtonHovered ? LIME : GREEN;

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.55f));
    DrawRectangleRounded(panel, 0.12f, 12, (Color){ 24, 24, 24, 235 });
    DrawRectangleRoundedLinesEx(panel, 0.12f, 12, 3.0f * scale, GREEN);
    DrawTextStrong(title, (int)(panel.x + (panel.width - titleWidth) * 0.5f), (int)(panel.y + 44.0f * scale), titleFontSize, RAYWHITE, BLACK);
    DrawRectangleRounded(button, 0.3f, 12, buttonColor);
    DrawTextStrong(buttonText, (int)(button.x + (button.width - buttonTextWidth) * 0.5f), (int)(button.y + 16.0f * scale), buttonFontSize, BLACK, Fade(WHITE, 0.25f));

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
    Color playerCore = ORANGE;
    Color playerEdge = (Color){ 255, 235, 200, 255 };
    Color playerOuter = (Color){ 40, 18, 0, 255 };
    Color playerGlow = (Color){ 255, 135, 0, 255 };

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
        Color bulletColor = bullets[i].fromPlayer ? ORANGE : SKYBLUE;

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

            if (lineLength > 0 && MeasureTextStrongSpaced(candidate, fontSize, spacing).x > maxWidth)
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
            while (sourceIndex < sourceLength && source[sourceIndex] != '\n' && source[sourceIndex] != ' ')
            {
                sourceIndex++;
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
    int bodyFontSize = ScaleFontSize(24.0f);
    int footerFontSize = ScaleFontSize(21.0f);
    float bodySpacing = 1.8f * scale;
    float footerSpacing = 1.0f * scale;
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
    Color continueFill = continueHovered ? LIME : (Color){ 0, 170, 70, 255 };
    int continueFontSize = ScaleFontSize(18.0f);

    DrawRectangleRounded(continueButton, 0.28f, 10, continueFill);
    DrawRectangleRoundedLinesEx(continueButton, 0.28f, 10, 2.0f, HUD_BORDER_COLOR);
    DrawTextStrong(T(TEXT_CONTINUE_TUTORIAL), (int)(continueButton.x + 20.0f * scale), (int)(continueButton.y + 14.0f * scale), continueFontSize, BLACK, Fade(WHITE, 0.25f));
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
    Color skipFill = skipHovered ? (Color){ 35, 120, 60, 255 } : (Color){ 22, 72, 40, 255 };
    int skipFontSize = ScaleFontSize(17.0f);

    DrawRectangleRounded(skipButton, 0.28f, 10, skipFill);
    DrawRectangleRoundedLinesEx(skipButton, 0.28f, 10, 2.0f, HUD_BORDER_COLOR);
    DrawTextStrong(T(TEXT_SKIP_TUTORIAL), (int)(skipButton.x + 20.0f * scale), (int)(skipButton.y + 15.0f * scale), skipFontSize, RAYWHITE, BLACK);
    DrawContinueButton(continueButton);
}

void DrawRoundPanel(const char *title, const char *body, const char *footer)
{
    /* Body text uses extra letter spacing for legibility, and the panel
     * height grows to fit however many lines the explanation needs, so
     * longer tutorial text never gets clipped or cramped. */
    float scale = GetUIScale();
    int bodyFontSize = ScaleFontSize(24.0f);
    float bodySpacing = 1.8f * scale;
    float footerSpacing = 1.0f * scale;
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
    int titleWidth = (int)MeasureTextStrongSpaced(title, titleFontSize, 1.0f * scale).x;
    Vector2 bodySize = MeasureTextStrongSpaced(wrappedBody, bodyFontSize, bodySpacing);
    Rectangle panel = GetRoundPanelRect(title, body, footer);

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.78f));
    DrawRectangleRounded(panel, 0.08f, 12, (Color){ 18, 18, 18, 245 });
    DrawRectangleRoundedLinesEx(panel, 0.08f, 12, 3.0f * scale, GREEN);
    DrawTextStrong(title, (int)(panel.x + (panel.width - titleWidth) * 0.5f), (int)(panel.y + 26.0f * scale), titleFontSize, GREEN, BLACK);
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

        DrawLanguageButtons();

        EndDrawing();
    }

    ShutdownGameAudio();
    ShutdownUIFont();
    CloseWindow();
    return 0;
}