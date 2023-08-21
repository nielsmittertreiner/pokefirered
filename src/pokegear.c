#include "global.h"
#include "bg.h"
#include "blend_palette.h"
#include "decompress.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "malloc.h"
#include "task.h"
#include "main.h"
#include "menu.h"
#include "new_menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "pokegear.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "start_menu.h"
#include "strings.h"
#include "text.h"
#include "window.h"
#include "constants/flags.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define tPosition data[0]
#define tState data[1]
#define tTimer data[2]

#define STATE_INIT               -1

#define OPTION_WOBBLE_TIMER      120
#define OPTION_SCROLL_X          212
#define OPTION_SCROLL_Y          48
#define OPTION_SCROLL_SPEED      4

#define MENU_BOX_NORMAL_PAL      15
#define MENU_BOX_HIGHLIGHT_PAL   14

#define ACTIVE_THEME_TILE_OFFSET 6
#define ACTIVE_THEME_TILES       9
#define ACTIVE_THEME_PAL_OFFSET  16
#define ACTIVE_THEME_PAL_SIZE    3

enum
{
    WIN_CLOCK,
    WIN_DESC,
};

enum
{
    TAG_POKEBALL,
    TAG_DIGITS,
    TAG_EXIT,
    TAG_EXIT_BG,
    TAG_CLOCK_BAR,
    TAG_REGION_MAP,
    TAG_PHONE,
    TAG_RADIO,
    TAG_MON_STATUS,
    TAG_SETTINGS,
    TAG_THEME_ICON,
};

enum
{
    OPTION_SETTINGS,
    OPTION_MON_STATUS,
    OPTION_RADIO,
    OPTION_PHONE,
    OPTION_REGION_MAP,
    OPTION_MAIN_MENU,
};

enum
{
    POKEGEAR_STYLE_SYPLH,
    POKEGEAR_STYLE_RETRO,
    POKEGEAR_STYLE_GOLD,
    POKEGEAR_STYLE_SILVER,
    POKEGEAR_STYLE_CRYSTAL,
    POKEGEAR_STYLE_ROCKET,
    POKEGEAR_STYLE_EGG,
    POKEGEAR_STYLE_UNOWN,
    POKEGEAR_STYLE_GBA,
    POKEGEAR_STYLE_LOCATION,
    POKEGEAR_STYLE_COUNT,
};

struct PokeGearStyle
{
    const void *backgroundTileset;
    const void *backgroundTilemap;
    const void *foregroundTileset;
    const void *foregroundTilemap;
    const void *palette;
    void (*gfxInitFunc)(void);
    void (*gfxMainFunc)(void);
    void (*gfxExitFunc)(void);
};

struct PokeGearResources
{
    void *bg1TilemapBuffer;
    void *bg2TilemapBuffer;
    struct UCoords8 bgOffset;
    u8 activeStyle;
    u8 selectedStyle;
    u8 scrollState;
    u16 scrollTimer;
    u8 selectedOption;
    u8 numOptions;
    u8 actions[5];
    u8 cursor:3;
    u8 style:5;
    u8 spriteId;
    u8 buttonSpriteIds[10];
    u8 digitSpriteIds[6];
    u8 menuSpriteIds[5];
    u8 themeSpriteIds[10];
    u8 exitSpriteId;
    u8 exitBgSpriteId;
    u8 clockSpriteId;
    u8 fakeSeconds;
};

static void Task_PokeGear_1(u8 taskId);
static void Task_PokeGear_2(u8 taskId);
static void Task_Settings(u8 taskId);
static void Task_MonStatus(u8 taskId);
static void Task_Radio(u8 taskId);
static void Task_Phone(u8 taskId);
static void Task_RegionMap(u8 taskId);
static void Task_LoadPokeGearOption_1(u8 taskId);
static void Task_LoadPokeGearOption_2(u8 taskId);
static void Task_LoadPokeGearOption_3(u8 taskId);
static void Task_LoadPokeGearOption_4(u8 taskId);
static void Task_LoadPokeGearOption_5(u8 taskId);
static void Task_ReturnToMainMenu_1(u8 taskId);
static void Task_ReturnToMainMenu_2(u8 taskId);
static void Task_ReturnToMainMenu_3(u8 taskId);
static void Task_ReturnToMainMenu_4(u8 taskId);
static void Task_ReturnToMainMenu_5(u8 taskId);
static void Task_ExitPokeGear_1(u8 taskId);
static void Task_ExitPokeGear_2(u8 taskId);
static void Task_ExitPokeGear_3(u8 taskId);
static void LoadPokeGearOptionBg(u8 option);
static void LoadPokeGearOptionData(u8 option);
static void UnloadPokeGearOptionBg(u8 option);
static void UnloadPokeGearOptionData(u8 option);
static void LoadPokeGearStyle(u8 style);
static void PokeGearStyleFunc_CreateStyleSylphPokeBallSprite(void);
static void PokeGearStyleFunc_CreateStyleRetroPokeBallSprite(void);
static void PokeGearStyleFunc_CreateStyleUnownPokeBallSprite(void);
static void PokeGearStyleFunc_CreateStyleGBAButtonSprites(void);
static void PokeGearStyleFunc_DestroyPokeBallSprite(void);
static void PokeGearStyleFunc_DestroyGBAButtonSprites(void);
static void PokeGearStyleFunc_ScrollDownRight(void);
static void PokeGearStyleFunc_ScrollRight(void);
static void PokeGearStyleFunc_ScrollUp(void);
static void PokeGearStyleFunc_ScrollZigZag(void);
static void SpriteCB_PokeBall(struct Sprite *sprite);
static void SpriteCB_ClockDigits(struct Sprite *sprite);
static void SpriteCB_Exit(struct Sprite *sprite);
static void SpriteCB_ExitBg(struct Sprite *sprite);
static void SpriteCB_ClockBar(struct Sprite *sprite);
static void SpriteCB_MainMenuIcon(struct Sprite *sprite);
static void SpriteCB_ThemeIcon(struct Sprite *sprite);
static void SpriteCB_GBAButtons(struct Sprite *sprite);
static bool32 SlideMainMenuIn(void);
static bool32 SlideMainMenuOut(void);
static void SetBackgroundOffsetScanlineBuffers(u8 offset);

// ewram
EWRAM_DATA static struct PokeGearResources *sPokeGearResources = NULL;

// .rodata
static const u8 sPokeGearClockX[] = 
{
    16, 30, 38, 47, 61, 73,
};

static const struct UCoords8 sPokeGearGBAButtonsPositions[] =
{
    // a, b, start, select, right, left, up, down, r , l
    {231, 128},
    {215, 144},
    {103, 132},
    {143, 132},
    {23, 143 - 8},
    {7, 143 - 8},
    {15, 134 - 8},
    {15, 152 - 8},
    {209, 112},
    {31, 112},
};

static const u8 sPokeGearTextColors[] = 
{
    TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY,
};

static const u8 *sPokeGearMainMenuDescriptions[] =
{
    gText_PokeGear_MainMenu_SettingsDesc,
    gText_PokeGear_MainMenu_MonStatusDesc,
    gText_PokeGear_MainMenu_RadioDesc,
    gText_PokeGear_MainMenu_PhoneDesc,
    gText_PokeGear_MainMenu_RegionMapDesc,
};

static const void (*const sPokeGearFuncs[])(u8) =
{
    Task_Settings,
    Task_MonStatus,
    Task_Radio,
    Task_Phone,
    Task_RegionMap,
};

static const struct BgTemplate sPokeGearBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 25,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 26,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 1,
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 2
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
    },
};

static const struct WindowTemplate sPokeGearWindowTemplates[] =
{
    [WIN_CLOCK] = 
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 9,
        .width = 11,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x10,
    },
    [WIN_DESC] = 
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 17,
        .width = 23,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x26,
    },
};

const struct ScanlineEffectParams sPokeGearScanlineEffect =
{
    (void *) REG_ADDR_BG1VOFS, SCANLINE_EFFECT_DMACNT_16BIT, 1
};

static const struct OamData sOamData_PokeGearPokeBall =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearDigits =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearExit =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearExitBg =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_BLEND,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearClockBar =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_BLEND,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_MainMenuIcon =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_GBAButtonStartSelect =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x8),
    .x = 0,
    .size = SPRITE_SIZE(32x8),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_GBAButtonLR =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 0,
};

static const union AffineAnimCmd sAffineAnim_PokeGearPokeBall[] =
{
    AFFINEANIMCMD_FRAME(0, 0, 2, 1),
    AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd *const sAffineAnims_PokeGearPokeBall[] =
{
    sAffineAnim_PokeGearPokeBall,
};

static const union AnimCmd sSpriteAnim_Digit0[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit1[] =
{
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit2[] =
{
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit3[] =
{
    ANIMCMD_FRAME(12, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit4[] =
{
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit5[] =
{
    ANIMCMD_FRAME(20, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit6[] =
{
    ANIMCMD_FRAME(24, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit7[] =
{
    ANIMCMD_FRAME(28, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit8[] =
{
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit9[] =
{
    ANIMCMD_FRAME(36, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_DigitOff[] =
{
    ANIMCMD_FRAME(40, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ColonOff[] =
{
    ANIMCMD_FRAME(44, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ColonOn[] =
{
    ANIMCMD_FRAME(48, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_AM[] =
{
    ANIMCMD_FRAME(52, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_PM[] =
{
    ANIMCMD_FRAME(56, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_AMPMOff[] =
{
    ANIMCMD_FRAME(60, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Point[] =
{
    ANIMCMD_FRAME(64, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_PokeGearDigits[] =
{
    sSpriteAnim_Digit0,
    sSpriteAnim_Digit1,
    sSpriteAnim_Digit2,
    sSpriteAnim_Digit3,
    sSpriteAnim_Digit4,
    sSpriteAnim_Digit5,
    sSpriteAnim_Digit6,
    sSpriteAnim_Digit7,
    sSpriteAnim_Digit8,
    sSpriteAnim_Digit9,
    sSpriteAnim_DigitOff,
    sSpriteAnim_ColonOff,
    sSpriteAnim_ColonOn,
    sSpriteAnim_AM,
    sSpriteAnim_PM,
    sSpriteAnim_AMPMOff,
    sSpriteAnim_Point,
};

static const union AnimCmd sSpriteAnim_Exit_Unselected[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Exit_Selected[] =
{
    ANIMCMD_FRAME(4, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Return_Unselected[] =
{
    ANIMCMD_FRAME(8, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Return_Selected[] =
{
    ANIMCMD_FRAME(12, 8),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_PokeGearExit[] =
{
    sSpriteAnim_Exit_Unselected,
    sSpriteAnim_Exit_Selected,
    sSpriteAnim_Return_Unselected,
    sSpriteAnim_Return_Selected,
};

static const union AnimCmd sSpriteAnim_Flip[] =
{
    ANIMCMD_FRAME(0, 8, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_PokeGearExitBg[] =
{
    sSpriteAnim_Flip,
};

static const union AffineAnimCmd sAffineAnim_MainMenuIcon_Static[] =
{
    AFFINEANIMCMD_FRAME(0, 0, 0, 1),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sAffineAnim_MainMenuIcon_Wobble[] =
{
    AFFINEANIMCMD_FRAME(0, 0, 1, 4),
    AFFINEANIMCMD_FRAME(0, 0, -1, 4),
    AFFINEANIMCMD_FRAME(0, 0, -1, 4),
    AFFINEANIMCMD_FRAME(0, 0, 1, 4),
    AFFINEANIMCMD_FRAME(0, 0, 0, 120),
    AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd *const sAffineAnims_MainMenuIcons[] =
{
    sAffineAnim_MainMenuIcon_Static,
    sAffineAnim_MainMenuIcon_Wobble,
};


static const union AnimCmd sSpriteAnim_MainMenuIcon_Static[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_MainMenuRegionMap_Selected[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_MainMenuRegionMap_Unselected[] =
{
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_MainMenuRegionMap[] =
{
    sSpriteAnim_MainMenuIcon_Static,
    sSpriteAnim_MainMenuRegionMap_Selected,
    sSpriteAnim_MainMenuRegionMap_Unselected,
};

static const union AnimCmd sSpriteAnim_MainMenuPhone_Selected[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(48, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_MainMenuPhone_Unselected[] =
{
    ANIMCMD_FRAME(48, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_MainMenuPhone[] =
{
    sSpriteAnim_MainMenuIcon_Static,
    sSpriteAnim_MainMenuPhone_Selected,
    sSpriteAnim_MainMenuPhone_Unselected,
};

static const union AnimCmd sSpriteAnim_MainMenuRadio_Selected[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(48, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_MainMenuRadio_Unselected[] =
{
    ANIMCMD_FRAME(48, 8),
    ANIMCMD_FRAME(32, 8),
    ANIMCMD_FRAME(16, 8),
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_MainMenuRadio[] =
{
    sSpriteAnim_MainMenuIcon_Static,
    sSpriteAnim_MainMenuRadio_Selected,
    sSpriteAnim_MainMenuRadio_Unselected,
};

static const union AnimCmd sSpriteAnim_MainMenuMonStatus_Selected[] =
{
    ANIMCMD_FRAME(0, 6),
    ANIMCMD_FRAME(16, 6),
    ANIMCMD_FRAME(32, 6),
    ANIMCMD_FRAME(16, 6),
    ANIMCMD_FRAME(0, 6),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_MainMenuMonStatus[] =
{
    sSpriteAnim_MainMenuIcon_Static,
    sSpriteAnim_MainMenuMonStatus_Selected,
    sSpriteAnim_MainMenuIcon_Static,
};

static const union AnimCmd sSpriteAnim_MainMenuSettings_Selected[] =
{
    ANIMCMD_FRAME(0, 6),
    ANIMCMD_FRAME(16, 6),
    ANIMCMD_FRAME(32, 6),
    ANIMCMD_FRAME(16, 6),
    ANIMCMD_FRAME(0, 6),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_MainMenuSettings[] =
{
    sSpriteAnim_MainMenuIcon_Static,
    sSpriteAnim_MainMenuSettings_Selected,
    sSpriteAnim_MainMenuIcon_Static,
};

static const union AnimCmd sSpriteAnim_ThemeIcons_0[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_1[] =
{
    ANIMCMD_FRAME(16, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_2[] =
{
    ANIMCMD_FRAME(32, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_3[] =
{
    ANIMCMD_FRAME(48, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_4[] =
{
    ANIMCMD_FRAME(64, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_5[] =
{
    ANIMCMD_FRAME(80, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_6[] =
{
    ANIMCMD_FRAME(96, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_7[] =
{
    ANIMCMD_FRAME(112, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_8[] =
{
    ANIMCMD_FRAME(128, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ThemeIcons_9[] =
{
    ANIMCMD_FRAME(144, 1),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_ThemeIcons[] =
{
    sSpriteAnim_ThemeIcons_0,
    sSpriteAnim_ThemeIcons_1,
    sSpriteAnim_ThemeIcons_2,
    sSpriteAnim_ThemeIcons_3,
    sSpriteAnim_ThemeIcons_4,
    sSpriteAnim_ThemeIcons_5,
    sSpriteAnim_ThemeIcons_6,
    sSpriteAnim_ThemeIcons_7,
    sSpriteAnim_ThemeIcons_8,
    sSpriteAnim_ThemeIcons_9,
};

static const union AnimCmd sSpriteAnim_GBAButtons_A_Normal[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_A_Pressed[] =
{
    ANIMCMD_FRAME(4, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_B_Normal[] =
{
    ANIMCMD_FRAME(8, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_B_Pressed[] =
{
    ANIMCMD_FRAME(12, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_Select_Normal[] =
{
    ANIMCMD_FRAME(16, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_Select_Pressed[] =
{
    ANIMCMD_FRAME(20, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_Start_Normal[] =
{
    ANIMCMD_FRAME(24, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_Start_Pressed[] =
{
    ANIMCMD_FRAME(28, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADRight_Normal[] =
{
    ANIMCMD_FRAME(32, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADRight_Pressed[] =
{
    ANIMCMD_FRAME(36, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADLeft_Normal[] =
{
    ANIMCMD_FRAME(40, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADLeft_Pressed[] =
{
    ANIMCMD_FRAME(44, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADUp_Normal[] =
{
    ANIMCMD_FRAME(48, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADUp_Pressed[] =
{
    ANIMCMD_FRAME(52, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADDown_Normal[] =
{
    ANIMCMD_FRAME(56, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_DPADDown_Pressed[] =
{
    ANIMCMD_FRAME(60, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_R_Normal[] =
{
    ANIMCMD_FRAME(64, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_R_Pressed[] =
{
    ANIMCMD_FRAME(72, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_L_Normal[] =
{
    ANIMCMD_FRAME(64, 1, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_GBAButtons_L_Pressed[] =
{
    ANIMCMD_FRAME(72, 1, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_GBAButtons[] =
{
    sSpriteAnim_GBAButtons_A_Normal,
    sSpriteAnim_GBAButtons_A_Pressed,
    sSpriteAnim_GBAButtons_B_Normal,
    sSpriteAnim_GBAButtons_B_Pressed,
    sSpriteAnim_GBAButtons_Select_Normal,
    sSpriteAnim_GBAButtons_Select_Pressed,
    sSpriteAnim_GBAButtons_Start_Normal,
    sSpriteAnim_GBAButtons_Start_Pressed,
    sSpriteAnim_GBAButtons_DPADRight_Normal,
    sSpriteAnim_GBAButtons_DPADRight_Pressed,
    sSpriteAnim_GBAButtons_DPADLeft_Normal,
    sSpriteAnim_GBAButtons_DPADLeft_Pressed,
    sSpriteAnim_GBAButtons_DPADUp_Normal,
    sSpriteAnim_GBAButtons_DPADUp_Pressed,
    sSpriteAnim_GBAButtons_DPADDown_Normal,
    sSpriteAnim_GBAButtons_DPADDown_Pressed,
    sSpriteAnim_GBAButtons_R_Normal,
    sSpriteAnim_GBAButtons_R_Pressed,
    sSpriteAnim_GBAButtons_L_Normal,
    sSpriteAnim_GBAButtons_L_Pressed,
};

static const struct SpriteTemplate sPokeGearPokeBallSpriteTemplate =
{
    .tileTag = TAG_POKEBALL,
    .paletteTag = TAG_POKEBALL,
    .oam = &sOamData_PokeGearPokeBall,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_PokeGearPokeBall,
    .callback = SpriteCB_PokeBall,
};

static const struct SpriteTemplate sPokeGearDigitsSpriteTemplate =
{
    .tileTag = TAG_DIGITS,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearDigits,
    .anims = sAnims_PokeGearDigits,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ClockDigits,
};

static const struct SpriteTemplate sPokeGearExitSpriteTemplate =
{
    .tileTag = TAG_EXIT,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearExit,
    .anims = sAnims_PokeGearExit,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Exit,
};

static const struct SpriteTemplate sPokeGearExitBgSpriteTemplate =
{
    .tileTag = TAG_EXIT_BG,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearExitBg,
    .anims = sAnims_PokeGearExitBg,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ExitBg,
};

static const struct SpriteTemplate sPokeGearClockBarSpriteTemplate =
{
    .tileTag = TAG_CLOCK_BAR,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearClockBar,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ClockBar,
};

static const struct SpriteTemplate sPokeGearMainMenuSpriteTemplates[] =
{
    {
        .tileTag = TAG_REGION_MAP,
        .paletteTag = TAG_REGION_MAP,
        .oam = &sOamData_MainMenuIcon,
        .anims = sAnims_MainMenuRegionMap,
        .images = NULL,
        .affineAnims = sAffineAnims_MainMenuIcons,
        .callback = SpriteCB_MainMenuIcon,
    },
    {
        .tileTag = TAG_PHONE,
        .paletteTag = TAG_PHONE,
        .oam = &sOamData_MainMenuIcon,
        .anims = sAnims_MainMenuPhone,
        .images = NULL,
        .affineAnims = sAffineAnims_MainMenuIcons,
        .callback = SpriteCB_MainMenuIcon,
    },
    {
        .tileTag = TAG_RADIO,
        .paletteTag = TAG_RADIO,
        .oam = &sOamData_MainMenuIcon,
        .anims = sAnims_MainMenuRadio,
        .images = NULL,
        .affineAnims = sAffineAnims_MainMenuIcons,
        .callback = SpriteCB_MainMenuIcon,
    },
    {
        .tileTag = TAG_MON_STATUS,
        .paletteTag = TAG_MON_STATUS,
        .oam = &sOamData_MainMenuIcon,
        .anims = sAnims_MainMenuMonStatus,
        .images = NULL,
        .affineAnims = sAffineAnims_MainMenuIcons,
        .callback = SpriteCB_MainMenuIcon,
    },
    {
        .tileTag = TAG_SETTINGS,
        .paletteTag = TAG_SETTINGS,
        .oam = &sOamData_MainMenuIcon,
        .anims = sAnims_MainMenuSettings,
        .images = NULL,
        .affineAnims = sAffineAnims_MainMenuIcons,
        .callback = SpriteCB_MainMenuIcon,
    },
};

static const struct SpriteTemplate sPokeGearGBAButtonsSpriteTemplates[] =
{
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_PokeGearDigits,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_PokeGearDigits,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_GBAButtonStartSelect,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_GBAButtonStartSelect,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_PokeGearDigits,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_PokeGearDigits,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_PokeGearDigits,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_PokeGearDigits,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_GBAButtonLR,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
    {
        .tileTag = TAG_POKEBALL,
        .paletteTag = TAG_POKEBALL,
        .oam = &sOamData_GBAButtonLR,
        .anims = sAnims_GBAButtons,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_GBAButtons,
    },
};

static const struct SpriteTemplate sPokeGearThemeIconsSpriteTemplate =
{
    .tileTag = TAG_THEME_ICON,
    .paletteTag = TAG_THEME_ICON,
    .oam = &sOamData_MainMenuIcon,
    .anims = sAnims_ThemeIcons,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ThemeIcon,
};

static const struct Subsprite sPokeGearClockBarSubsprites[] =
{
    {
        .x = -64,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 0,
        .priority = 1,
    },
    {
        .x = -32,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 0,
        .priority = 1,
    },
    {
        .x = 0,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 0,
        .priority = 1,
    },
    {
        .x = 32,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 32,
        .priority = 1,
    },
};

static const struct SubspriteTable sPokeGearClockBarSubspriteTable[] =
{
    {ARRAY_COUNT(sPokeGearClockBarSubsprites), sPokeGearClockBarSubsprites},
};

static const struct CompressedSpriteSheet sPokeGearPokeBallSpriteSheet =
{
    .data = gPokeGearPokeBall_Gfx,
    .size = 512,
    .tag = TAG_POKEBALL,
};

static const struct CompressedSpriteSheet sPokeGearGBAButtonsSpriteSheet =
{
    .data = gPokeGearStyleGBA_Buttons_Gfx,
    .size = 2560,
    .tag = TAG_POKEBALL,
};

static const struct CompressedSpriteSheet sPokeGearDigitSpriteSheet =
{
    .data = gPokeGearMainMenu_Digits_Gfx,
    .size = 2176,
    .tag = TAG_DIGITS,
};

static const struct CompressedSpriteSheet sPokeGearExitSpriteSheet =
{
    .data = gPokeGearMainMenu_Exit_Gfx,
    .size = 512,
    .tag = TAG_EXIT,
};

static const struct CompressedSpriteSheet sPokeGearExitBgSpriteSheet =
{
    .data = gPokeGearMainMenu_BarEnd_Gfx,
    .size = 512,
    .tag = TAG_EXIT_BG,
};

static const struct CompressedSpriteSheet sPokeGearClockBarSpriteSheet =
{
    .data = gPokeGearMainMenu_ClockBar_Gfx,
    .size = 1024,
    .tag = TAG_CLOCK_BAR,
};

static const struct CompressedSpriteSheet sPokeGearMainMenuSpriteSheets[] =
{
    {
        .data = gPokeGearMainMenu_RegionMap_Gfx,
        .size = 1536,
        .tag = TAG_REGION_MAP,
    },
    {
        .data = gPokeGearMainMenu_Phone_Gfx,
        .size = 2048,
        .tag = TAG_PHONE,
    },
    {
        .data = gPokeGearMainMenu_Radio_Gfx,
        .size = 2048,
        .tag = TAG_RADIO,
    },
    {
        .data = gPokeGearMainMenu_MonStatus_Gfx,
        .size = 1536,
        .tag = TAG_MON_STATUS,
    },
    {
        .data = gPokeGearMainMenu_Settings_Gfx,
        .size = 1536,
        .tag = TAG_SETTINGS,
    },
};

static const struct CompressedSpriteSheet sPokeGearThemeIconsSpriteSheet =
{
    .data = gPokeGearSettings_ThemeIcons_Gfx,
    .size = 5120,
    .tag = TAG_THEME_ICON,
};

static const struct SpritePalette sPokeGearStyleSylphPokeBallSpritePalette =
{   
    .data = gPokeGearStyleSylph_PokeBall_Pal,
    .tag =  TAG_POKEBALL,
};

static const struct SpritePalette sPokeGearStyleRetroPokeBallSpritePalette =
{   
    .data = gPokeGearStyleRetro_PokeBall_Pal,
    .tag =  TAG_POKEBALL,
};

static const struct SpritePalette sPokeGearStyleUnownPokeBallSpritePalette =
{   
    .data = gPokeGearStyleUnown_PokeBall_Pal,
    .tag =  TAG_POKEBALL,
};

static const struct SpritePalette sPokeGearStyleGBAButtonsSpritePalette =
{   
    .data = gPokeGearStyleGBA_Buttons_Pal,
    .tag =  TAG_POKEBALL,
};

static const struct SpritePalette sPokeGearDigitSpritePalette =
{
    .data = gPokeGearMainMenu_Digits_Pal,
    .tag = TAG_DIGITS,
};

static const struct SpritePalette sPokeGearMainMenuSpritePalettes[] =
{
    {
        .data = gPokeGearMainMenu_RegionMap_Pal,
        .tag = TAG_REGION_MAP,
    },
    {
        .data = gPokeGearMainMenu_Phone_Pal,
        .tag = TAG_PHONE,
    },
    {
        .data = gPokeGearMainMenu_Radio_Pal,
        .tag = TAG_RADIO,
    },
    {
        .data = gPokeGearMainMenu_MonStatus_Pal,
        .tag = TAG_MON_STATUS,
    },
    {
        .data = gPokeGearMainMenu_Settings_Pal,
        .tag = TAG_SETTINGS,
    },
};

static const struct SpritePalette sPokeGearThemeIconsSpritePalette =
{   
    .data = gPokeGearSettings_ThemeIcons_Pal,
    .tag =  TAG_THEME_ICON,
};

static const struct PokeGearStyle sPokeGearStyleData[] =
{
    [POKEGEAR_STYLE_SYPLH] =
    {
        .backgroundTileset = gPokeGearStyleSylph_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleSylph_Background_Map,
        .foregroundTileset = gPokeGearStyleSylph_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleSylph_Foreground_Map,
        .palette = gPokeGearStyleSylph_Pal,
        .gfxInitFunc = PokeGearStyleFunc_CreateStyleSylphPokeBallSprite,
        .gfxMainFunc = PokeGearStyleFunc_ScrollDownRight,
        .gfxExitFunc = PokeGearStyleFunc_DestroyPokeBallSprite,
    },
    [POKEGEAR_STYLE_RETRO] =
    {
        .backgroundTileset = gPokeGearStyleRetro_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleRetro_Background_Map,
        .foregroundTileset = gPokeGearStyleRetro_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleRetro_Foreground_Map,
        .palette = gPokeGearStyleRetro_Pal,
        .gfxInitFunc = PokeGearStyleFunc_CreateStyleRetroPokeBallSprite,
        .gfxMainFunc = PokeGearStyleFunc_ScrollUp,
        .gfxExitFunc = PokeGearStyleFunc_DestroyPokeBallSprite,
    },
    [POKEGEAR_STYLE_GOLD] =
    {
        .backgroundTileset = gPokeGearStyleGold_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleGold_Background_Map,
        .foregroundTileset = gPokeGearStyleGold_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleGold_Foreground_Map,
        .palette = gPokeGearStyleGold_Pal,
        .gfxInitFunc = NULL,
        .gfxMainFunc = PokeGearStyleFunc_ScrollDownRight,
        .gfxExitFunc = NULL,
    },
    [POKEGEAR_STYLE_SILVER] =
    {
        .backgroundTileset = gPokeGearStyleSilver_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleSilver_Background_Map,
        .foregroundTileset = gPokeGearStyleSilver_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleSilver_Foreground_Map,
        .palette = gPokeGearStyleSilver_Pal,
        .gfxInitFunc = NULL,
        .gfxMainFunc = PokeGearStyleFunc_ScrollDownRight,
        .gfxExitFunc = NULL,
    },
    [POKEGEAR_STYLE_CRYSTAL] =
    {
        .backgroundTileset = gPokeGearStyleCrystal_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleCrystal_Background_Map,
        .foregroundTileset = gPokeGearStyleCrystal_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleCrystal_Foreground_Map,
        .palette = gPokeGearStyleCrystal_Pal,
        .gfxInitFunc = NULL,
        .gfxMainFunc = PokeGearStyleFunc_ScrollDownRight,
        .gfxExitFunc = NULL,
    },
    [POKEGEAR_STYLE_ROCKET] =
    {
        .backgroundTileset = gPokeGearStyleRocket_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleRocket_Background_Map,
        .foregroundTileset = gPokeGearStyleRocket_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleRocket_Foreground_Map,
        .palette = gPokeGearStyleRocket_Pal,
        .gfxInitFunc = NULL,
        .gfxMainFunc = PokeGearStyleFunc_ScrollRight,
        .gfxExitFunc = NULL,
    },
    [POKEGEAR_STYLE_EGG] =
    {
        .backgroundTileset = gPokeGearStyleRocket_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleRocket_Background_Map,
        .foregroundTileset = gPokeGearStyleRocket_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleRocket_Foreground_Map,
        .palette = gPokeGearStyleRocket_Pal,
        .gfxInitFunc = NULL,
        .gfxMainFunc = PokeGearStyleFunc_ScrollRight,
        .gfxExitFunc = NULL,
    },
    [POKEGEAR_STYLE_UNOWN] =
    {
        .backgroundTileset = gPokeGearStyleUnown_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleUnown_Background_Map,
        .foregroundTileset = gPokeGearStyleUnown_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleUnown_Foreground_Map,
        .palette = gPokeGearStyleUnown_Pal,
        .gfxInitFunc = PokeGearStyleFunc_CreateStyleUnownPokeBallSprite,
        .gfxMainFunc = PokeGearStyleFunc_ScrollZigZag,
        .gfxExitFunc = PokeGearStyleFunc_DestroyPokeBallSprite,
    },
    [POKEGEAR_STYLE_GBA] =
    {
        .backgroundTileset = gPokeGearStyleGBA_Background_Gfx,
        .backgroundTilemap = gPokeGearStyleGBA_Background_Map,
        .foregroundTileset = gPokeGearStyleGBA_Foreground_Gfx,
        .foregroundTilemap = gPokeGearStyleGBA_Foreground_Map,
        .palette = gPokeGearStyleGBA_Pal,
        .gfxInitFunc = PokeGearStyleFunc_CreateStyleGBAButtonSprites,
        .gfxMainFunc = PokeGearStyleFunc_ScrollUp,
        .gfxExitFunc = PokeGearStyleFunc_DestroyGBAButtonSprites,
    },
};

static void VBlankCB_PokeGear(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ScanlineEffect_InitHBlankDmaTransfer();
    if (sPokeGearStyleData[sPokeGearResources->activeStyle].gfxMainFunc)
        sPokeGearStyleData[sPokeGearResources->activeStyle].gfxMainFunc();
}

static void CB2_PokeGear(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

void CB2_InitPokeGear(void)
{
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    FillPalette(RGB_BLACK, 0, PLTT_SIZE);
    ResetBgsAndClearDma3BusyFlags(TRUE);
    ClearScheduledBgCopiesToVram();
    DeactivateAllTextPrinters();
    FreeAllSpritePalettes();
    ScanlineEffect_Stop();
    ScanlineEffect_Clear();
    ResetPaletteFade();
    ResetSpriteData();
    ResetTasks();

    FlagSet(FLAG_SYS_POKEGEAR_RADIO);
    FlagSet(FLAG_SYS_POKEGEAR_MON_STATUS);

    sPokeGearResources = AllocZeroed(sizeof(struct PokeGearResources));
    sPokeGearResources->bg1TilemapBuffer = AllocZeroed(BG_SCREEN_SIZE);
    sPokeGearResources->bg2TilemapBuffer = AllocZeroed(BG_SCREEN_SIZE);

    InitBgsFromTemplates(0, sPokeGearBgTemplates, ARRAY_COUNT(sPokeGearBgTemplates));
    SetBgTilemapBuffer(1, sPokeGearResources->bg1TilemapBuffer);
    SetBgTilemapBuffer(2, sPokeGearResources->bg2TilemapBuffer);
    InitWindows(sPokeGearWindowTemplates);
    LoadPalette(gStandardMenuPalette, 0xF0, 0x20);
    ScanlineEffect_SetParams(sPokeGearScanlineEffect);

    sPokeGearResources->numOptions = 0;
    AppendToList(sPokeGearResources->actions, &sPokeGearResources->numOptions, OPTION_REGION_MAP);
    AppendToList(sPokeGearResources->actions, &sPokeGearResources->numOptions, OPTION_PHONE);
    if (FlagGet(FLAG_SYS_POKEGEAR_RADIO))
        AppendToList(sPokeGearResources->actions, &sPokeGearResources->numOptions, OPTION_RADIO);
    if (FlagGet(FLAG_SYS_POKEGEAR_MON_STATUS))
        AppendToList(sPokeGearResources->actions, &sPokeGearResources->numOptions, OPTION_MON_STATUS);
    AppendToList(sPokeGearResources->actions, &sPokeGearResources->numOptions, OPTION_SETTINGS);
    sPokeGearResources->selectedOption = sPokeGearResources->numOptions - 1;

    sPokeGearResources->activeStyle = gSaveBlock2Ptr->pokeGearStyle;
    LoadPokeGearStyle(sPokeGearResources->activeStyle);
    LoadPokeGearOptionBg(OPTION_MAIN_MENU);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_TGT1_OBJ | BLDCNT_TGT2_BG3 | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 8));

    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB_PokeGear);
    SetMainCallback2(CB2_PokeGear);
    CreateTask(Task_PokeGear_1, 1);
    sPokeGearResources->bgOffset.x = OPTION_SCROLL_X;
    sPokeGearResources->bgOffset.y = OPTION_SCROLL_Y;

    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
    PlaySE(SE_POKENAV_ON);
}

static void Task_PokeGear_1(u8 taskId)
{
    if (SlideMainMenuIn())
    {
        LoadPokeGearOptionData(OPTION_MAIN_MENU);
        gTasks[taskId].func = Task_PokeGear_2;
    }
}

static void Task_PokeGear_2(u8 taskId)
{
    struct Sprite *digit;
    u32 i;

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].func = Task_LoadPokeGearOption_1;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ExitPokeGear_1;
        PlaySE(SE_POKENAV_OFF);
    }
    else if (JOY_NEW(START_BUTTON))
    {
        PlaySE(SE_BALL_TRAY_EXIT);
        gSaveBlock2Ptr->twentyFourHourClock ^= 1;
        for (i = 0; i < ARRAY_COUNT(sPokeGearResources->digitSpriteIds); i++)
        {
            digit = &gSprites[sPokeGearResources->digitSpriteIds[i]];
            digit->tState = STATE_INIT;
        }
    }
    else if (JOY_NEW(DPAD_LEFT) && sPokeGearResources->selectedOption < sPokeGearResources->numOptions - 1)
    {
        PlaySE(SE_SELECT);
        CopyToBgTilemapBufferRect_ChangePalette(2, gPokeGearMainMenu_Option_Map, 25 - (5 * sPokeGearResources->selectedOption), 9, 4, 5, MENU_BOX_NORMAL_PAL);
        sPokeGearResources->selectedOption++;
        CopyToBgTilemapBufferRect_ChangePalette(2, gPokeGearMainMenu_Option_Map, 25 - (5 * sPokeGearResources->selectedOption), 9, 4, 5, MENU_BOX_HIGHLIGHT_PAL);
        FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(0));
        AddTextPrinterParameterized3(WIN_DESC, FONT_SMALL, 12, 0, sPokeGearTextColors, 0, sPokeGearMainMenuDescriptions[sPokeGearResources->selectedOption]);
        CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
        CopyBgTilemapBufferToVram(2);
    }
    else if (JOY_NEW(DPAD_RIGHT) && sPokeGearResources->selectedOption > 0)
    {
        PlaySE(SE_SELECT);
        CopyToBgTilemapBufferRect_ChangePalette(2, gPokeGearMainMenu_Option_Map, 25 - (5 * sPokeGearResources->selectedOption), 9, 4, 5, MENU_BOX_NORMAL_PAL);
        sPokeGearResources->selectedOption--;
        CopyToBgTilemapBufferRect_ChangePalette(2, gPokeGearMainMenu_Option_Map, 25 - (5 * sPokeGearResources->selectedOption), 9, 4, 5, MENU_BOX_HIGHLIGHT_PAL);
        FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(0));
        AddTextPrinterParameterized3(WIN_DESC, FONT_SMALL, 12, 0, sPokeGearTextColors, 0, sPokeGearMainMenuDescriptions[sPokeGearResources->selectedOption]);
        CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
        CopyBgTilemapBufferToVram(2);
    }

    if (++sPokeGearResources->fakeSeconds >= 60)
    {
        sPokeGearResources->fakeSeconds = 0;
    }
}

static void Task_Settings(u8 taskId)
{
    u8 x;
    u8 y;
    u8 palette;

    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
    else if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SUCCESS);
        if (sPokeGearResources->selectedStyle != sPokeGearResources->activeStyle)
        {
            LoadPokeGearStyle(sPokeGearResources->selectedStyle);

            x = (3 + (5 * (sPokeGearResources->activeStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->activeStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
            sPokeGearResources->activeStyle = sPokeGearResources->selectedStyle;
    
            x = (3 + (5 * (sPokeGearResources->activeStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->activeStyle >= 5 ? 1 : 0)));
            palette = sPokeGearResources->activeStyle < 5 ? 1 : 2;
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 0, x, y, 4, 4, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 1, x, y, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 2, x + 3, y, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 3, x, y + 3, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 4, x + 3, y + 3, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 5, x, y + 1, 1, 2, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 6, x + 1, y, 2, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 7, x + 1, y + 3, 2, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->activeStyle) + 8, x + 3, y + 1, 1, 2, palette);
            CopyBgTilemapBufferToVram(2);
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->activeStyle) + 1, ACTIVE_THEME_PAL_SIZE, 8, gPlttBufferFaded[239]);
        }
    }
    else if (JOY_NEW(DPAD_LEFT) && ((sPokeGearResources->selectedStyle > 0 && sPokeGearResources->selectedStyle < (POKEGEAR_STYLE_COUNT / 2)) || 
                                    (sPokeGearResources->selectedStyle > (POKEGEAR_STYLE_COUNT / 2) && sPokeGearResources->selectedStyle < POKEGEAR_STYLE_COUNT)))
    {
        PlaySE(SE_SELECT);
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 0, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
        }
        sPokeGearResources->selectedStyle--;
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 8, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
        }
        CopyBgTilemapBufferToVram(2);
    }
    else if (JOY_NEW(DPAD_RIGHT) && ((sPokeGearResources->selectedStyle >= 0 && sPokeGearResources->selectedStyle < (POKEGEAR_STYLE_COUNT / 2) - 1) || 
                                     (sPokeGearResources->selectedStyle >= (POKEGEAR_STYLE_COUNT / 2) && sPokeGearResources->selectedStyle < POKEGEAR_STYLE_COUNT - 1)))
    {
        PlaySE(SE_SELECT);
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 0, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
        }
        sPokeGearResources->selectedStyle++;
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 8, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
        }
        CopyBgTilemapBufferToVram(2);
    }
    else if (JOY_NEW(DPAD_UP) && (sPokeGearResources->selectedStyle > (POKEGEAR_STYLE_COUNT / 2) - 1 && sPokeGearResources->selectedStyle < POKEGEAR_STYLE_COUNT))
    {
        PlaySE(SE_SELECT);
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 0, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
        }
        sPokeGearResources->selectedStyle -= (POKEGEAR_STYLE_COUNT / 2);
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 8, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
        }
        CopyBgTilemapBufferToVram(2);
    }
    else if (JOY_NEW(DPAD_DOWN) && (sPokeGearResources->selectedStyle < (POKEGEAR_STYLE_COUNT / 2)) && sPokeGearResources->selectedStyle >= 0)
    {
        PlaySE(SE_SELECT);
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 0, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_NORMAL_PAL);
        }
        sPokeGearResources->selectedStyle += (POKEGEAR_STYLE_COUNT / 2);
        if (sPokeGearResources->selectedStyle == sPokeGearResources->activeStyle)
        {
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 8, gPlttBufferFaded[239]);
        }
        else
        {
            x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            FillBgTilemapBufferRect(2, 1, x, y, 4, 4, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 2, x, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 3, x + 3, y, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 4, x, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
            FillBgTilemapBufferRect(2, 5, x + 3, y + 3, 1, 1, MENU_BOX_HIGHLIGHT_PAL);
        }
        CopyBgTilemapBufferToVram(2);
    }
}

static void Task_MonStatus(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
}

static void Task_Radio(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
}

static void Task_Phone(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
}

static void Task_RegionMap(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
}

static void Task_LoadPokeGearOption_1(u8 taskId)
{
    UnloadPokeGearOptionData(OPTION_MAIN_MENU);
    gTasks[taskId].func = Task_LoadPokeGearOption_2;
}

static void Task_LoadPokeGearOption_2(u8 taskId)
{
    if (SlideMainMenuOut())
    {
        UnloadPokeGearOptionBg(OPTION_MAIN_MENU);
        gTasks[taskId].func = Task_LoadPokeGearOption_3;
    }
}

static void Task_LoadPokeGearOption_3(u8 taskId)
{
    LoadPokeGearOptionBg(sPokeGearResources->selectedOption);
    sPokeGearResources->bgOffset.x = OPTION_SCROLL_X;
    sPokeGearResources->bgOffset.y = OPTION_SCROLL_Y;
    gTasks[taskId].func = Task_LoadPokeGearOption_4;
}

static void Task_LoadPokeGearOption_4(u8 taskId)
{
    if (SlideMainMenuIn())
    {
        gTasks[taskId].func = Task_LoadPokeGearOption_5;
    }
}

static void Task_LoadPokeGearOption_5(u8 taskId)
{
    LoadPokeGearOptionData(sPokeGearResources->selectedOption);
    gTasks[taskId].func = sPokeGearFuncs[sPokeGearResources->selectedOption];
}

static void Task_ReturnToMainMenu_1(u8 taskId)
{
    UnloadPokeGearOptionData(sPokeGearResources->selectedOption);
    sPokeGearResources->bgOffset.x = 0;
    sPokeGearResources->bgOffset.y = 0;
    gTasks[taskId].func = Task_ReturnToMainMenu_2;
}

static void Task_ReturnToMainMenu_2(u8 taskId)
{
    if (SlideMainMenuOut())
    {
        UnloadPokeGearOptionBg(sPokeGearResources->selectedOption);
        gTasks[taskId].func = Task_ReturnToMainMenu_3;
    }
}

static void Task_ReturnToMainMenu_3(u8 taskId)
{
    LoadPokeGearOptionBg(OPTION_MAIN_MENU);
    sPokeGearResources->bgOffset.x = OPTION_SCROLL_X;
    sPokeGearResources->bgOffset.y = OPTION_SCROLL_Y;
    gTasks[taskId].func = Task_ReturnToMainMenu_4;
}

static void Task_ReturnToMainMenu_4(u8 taskId)
{
    if (SlideMainMenuIn())
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_5;
    }
}

static void Task_ReturnToMainMenu_5(u8 taskId)
{
    LoadPokeGearOptionData(OPTION_MAIN_MENU);
    gTasks[taskId].func = Task_PokeGear_2;
}

static void Task_ExitPokeGear_1(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    UnloadPokeGearOptionData(OPTION_MAIN_MENU);
    gTasks[taskId].func = Task_ExitPokeGear_2;
}

static void Task_ExitPokeGear_2(u8 taskId)
{
    if (SlideMainMenuOut())
        gTasks[taskId].func = Task_ExitPokeGear_3;
}

static void Task_ExitPokeGear_3(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        UnsetBgTilemapBuffer(1);
        UnsetBgTilemapBuffer(2);
        FREE_AND_SET_NULL(sPokeGearResources->bg1TilemapBuffer);
        FREE_AND_SET_NULL(sPokeGearResources->bg2TilemapBuffer);
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}

// PokeGear Option Functions
static void LoadPokeGearOptionBg(u8 option)
{
    DmaClear16(3, (u16 *)BG_CHAR_ADDR(2), BG_CHAR_SIZE);
    DmaClear16(3, BG_SCREEN_ADDR(26), BG_SCREEN_SIZE * 2);
    DmaClear16(3, BG_SCREEN_ADDR(28), BG_SCREEN_SIZE * 2);
    DmaClear16(3, sPokeGearResources->bg1TilemapBuffer, BG_SCREEN_SIZE);
    DmaClear16(3, sPokeGearResources->bg2TilemapBuffer, BG_SCREEN_SIZE);

    if (sPokeGearStyleData[sPokeGearResources->activeStyle].gfxExitFunc)
        sPokeGearStyleData[sPokeGearResources->activeStyle].gfxExitFunc();

    switch (option)
    {
        case OPTION_REGION_MAP:
            FillBgTilemapBufferRect(1, 0x01, 0, 0, 30, 2, 0);
            FillBgTilemapBufferRect(1, 0x5A, 0, 2, 30, 1, 0);
            WriteSequenceToBgTilemapBuffer(1, 0x18, 3, 0, 6, 2, 0, 1);
            WriteSequenceToBgTilemapBuffer(1, 0x24, 10, 0, 3, 2, 0, 1);
            break;
        case OPTION_PHONE:
            FillBgTilemapBufferRect(1, 0x01, 0, 0, 30, 2, 0);
            FillBgTilemapBufferRect(1, 0x5A, 0, 2, 30, 1, 0);
            WriteSequenceToBgTilemapBuffer(1, 0x2A, 3, 0, 5, 2, 0, 1);
            break;
        case OPTION_RADIO:
            FillBgTilemapBufferRect(1, 0x01, 0, 0, 30, 2, 0);
            FillBgTilemapBufferRect(1, 0x5A, 0, 2, 30, 1, 0);
            WriteSequenceToBgTilemapBuffer(1, 0x34, 3, 0, 5, 2, 0, 1);
            break;
        case OPTION_MON_STATUS:
            FillBgTilemapBufferRect(1, 0x01, 0, 0, 30, 2, 0);
            FillBgTilemapBufferRect(1, 0x5A, 0, 2, 30, 1, 0);
            WriteSequenceToBgTilemapBuffer(1, 0x02, 3, 0, 7, 2, 0, 1);
            WriteSequenceToBgTilemapBuffer(1, 0x3E, 11, 0, 6, 2, 0, 1);
            break;
        case OPTION_SETTINGS:
            LZ77UnCompVram(gPokeGearSettings_ThemeTiles_Gfx, BG_CHAR_ADDR(2));
            LZ77UnCompWram(gPokeGearSettings_ThemeTiles_Map, sPokeGearResources->bg2TilemapBuffer);
            FillBgTilemapBufferRect(1, 0x01, 0, 0, 30, 2, 0);
            FillBgTilemapBufferRect(1, 0x5A, 0, 2, 30, 1, 0);
            WriteSequenceToBgTilemapBuffer(1, 0x4A, 3, 0, 8, 2, 0, 1);
            LoadSpritePalette(&sPokeGearDigitSpritePalette);
            LoadSpritePalette(&sPokeGearThemeIconsSpritePalette);
            LoadCompressedSpriteSheet(&sPokeGearExitBgSpriteSheet);
            LoadCompressedSpriteSheet(&sPokeGearExitSpriteSheet);
            LoadCompressedSpriteSheet(&sPokeGearThemeIconsSpriteSheet);
            LoadPalette(gPokeGearSettings_ThemeTiles_Pal, 16, 64);
            sPokeGearResources->selectedStyle = sPokeGearResources->activeStyle;

            u8 x = (3 + (5 * (sPokeGearResources->selectedStyle % 5)));
            u8 y = (8 + (5 * (sPokeGearResources->selectedStyle >= 5 ? 1 : 0)));
            u8 palette = sPokeGearResources->selectedStyle < 5 ? 1 : 2;
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 0, x, y, 4, 4, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 1, x, y, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 2, x + 3, y, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 3, x, y + 3, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 4, x + 3, y + 3, 1, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 5, x, y + 1, 1, 2, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 6, x + 1, y, 2, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 7, x + 1, y + 3, 2, 1, palette);
            FillBgTilemapBufferRect(2, ACTIVE_THEME_TILE_OFFSET + (ACTIVE_THEME_TILES * sPokeGearResources->selectedStyle) + 8, x + 3, y + 1, 1, 2, palette);
            BlendPalette(ACTIVE_THEME_PAL_OFFSET + (ACTIVE_THEME_PAL_SIZE * sPokeGearResources->selectedStyle) + 1, ACTIVE_THEME_PAL_SIZE, 8, gPlttBufferFaded[239]);

            sPokeGearResources->exitBgSpriteId = CreateSprite(&sPokeGearExitBgSpriteTemplate, 224, 48, 1);
            sPokeGearResources->exitSpriteId = CreateSprite(&sPokeGearExitSpriteTemplate, 228, 44, 0);
            gSprites[sPokeGearResources->exitSpriteId].data[0] = FALSE;

            for (u32 i = 0; i < POKEGEAR_STYLE_COUNT; i++)
            {
                s16 x = ((3 + (5 * (i % 5))) * 8) + 16;
                s16 y = ((8 + (5 * (i >= 5 ? 1 : 0))) * 8) + 16;

                sPokeGearResources->themeSpriteIds[i] = CreateSprite(&sPokeGearThemeIconsSpriteTemplate, x, y, 0);
                StartSpriteAnim(&gSprites[sPokeGearResources->themeSpriteIds[i]], i);
            }
            break;
        case OPTION_MAIN_MENU:
            LZ77UnCompVram(gPokeGearMainMenu_Gfx, BG_CHAR_ADDR(2));
            LZ77UnCompWram(sPokeGearStyleData[sPokeGearResources->activeStyle].foregroundTilemap, sPokeGearResources->bg1TilemapBuffer);
            FillBgTilemapBufferRect(1, 0x01, 0, 0, 30, 2, 0);
            FillBgTilemapBufferRect(1, 0x5A, 0, 2, 30, 1, 0);
            WriteSequenceToBgTilemapBuffer(1, 0x02, 3, 0, 7, 2, 0, 1);
            WriteSequenceToBgTilemapBuffer(1, 0x10, 11, 0, 4, 2, 0, 1);
            LoadSpritePalette(&sPokeGearDigitSpritePalette);
            LoadCompressedSpriteSheet(&sPokeGearDigitSpriteSheet);
            LoadCompressedSpriteSheet(&sPokeGearClockBarSpriteSheet);
            LoadCompressedSpriteSheet(&sPokeGearExitBgSpriteSheet);
            LoadCompressedSpriteSheet(&sPokeGearExitSpriteSheet);

            for (u32 i = 0; i < sPokeGearResources->numOptions; i++)
            {
                CopyToBgTilemapBufferRect(2, gPokeGearMainMenu_OptionBar_Map, 25 - (5 * i) + 4, 11, 1, 2);
                CopyToBgTilemapBufferRect(2, gPokeGearMainMenu_Option_Map, 25 - (5 * i), 9, 4, 5);
                LoadCompressedSpriteSheet(&sPokeGearMainMenuSpriteSheets[sPokeGearResources->actions[i]]);
                LoadSpritePalette(&sPokeGearMainMenuSpritePalettes[sPokeGearResources->actions[i]]);
                sPokeGearResources->menuSpriteIds[i] = CreateSprite(&sPokeGearMainMenuSpriteTemplates[sPokeGearResources->actions[i]], ((25 - (5 * i)) * 8) + 16, 92, 0);
                gSprites[sPokeGearResources->menuSpriteIds[i]].tPosition = i;
                gSprites[sPokeGearResources->menuSpriteIds[i]].tState = sPokeGearResources->actions[i];
            }

            CopyToBgTilemapBufferRect(2, gPokeGearMainMenu_OptionEnd_Map, 29 - (5 * (sPokeGearResources->numOptions)) - 1, 11, 2, 2);
            CopyToBgTilemapBufferRect_ChangePalette(2, gPokeGearMainMenu_Option_Map, 25 - (5 * sPokeGearResources->selectedOption), 9, 4, 5, MENU_BOX_HIGHLIGHT_PAL);

            sPokeGearResources->exitBgSpriteId = CreateSprite(&sPokeGearExitBgSpriteTemplate, 224, 48, 1);
            sPokeGearResources->exitSpriteId = CreateSprite(&sPokeGearExitSpriteTemplate, 228, 44, 0);
            gSprites[sPokeGearResources->exitSpriteId].data[0] = TRUE;
            sPokeGearResources->clockSpriteId = CreateSprite(&sPokeGearClockBarSpriteTemplate, 64, 48, 2);
            SetSubspriteTables(&gSprites[sPokeGearResources->clockSpriteId], sPokeGearClockBarSubspriteTable);

            for (u32 i = 0; i < 6; i++)
            {
                sPokeGearResources->digitSpriteIds[i] = CreateSprite(&sPokeGearDigitsSpriteTemplate, sPokeGearClockX[i], 44, 0);
                gSprites[sPokeGearResources->digitSpriteIds[i]].tPosition = i;
                gSprites[sPokeGearResources->digitSpriteIds[i]].tState = STATE_INIT;
            }

            if (sPokeGearStyleData[sPokeGearResources->activeStyle].gfxInitFunc)
                sPokeGearStyleData[sPokeGearResources->activeStyle].gfxInitFunc();
            break;
    }

    CopyBgTilemapBufferToVram(1);
    CopyBgTilemapBufferToVram(2);
}

static void LoadPokeGearOptionData(u8 option)
{
    switch (option)
    {
        case OPTION_REGION_MAP:
            break;
        case OPTION_PHONE:
            break;
        case OPTION_RADIO:
            break;
        case OPTION_MON_STATUS:
            break;
        case OPTION_SETTINGS:
            break;
        case OPTION_MAIN_MENU:
            PutWindowTilemap(WIN_DESC);
            FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(0));
            AddTextPrinterParameterized3(WIN_DESC, FONT_SMALL, 8, 0, sPokeGearTextColors, 0, sPokeGearMainMenuDescriptions[sPokeGearResources->selectedOption]);
            CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
            break;
    }
}

static void UnloadPokeGearOptionBg(u8 option)
{
    switch (option)
    {
        case OPTION_REGION_MAP:
            break;
        case OPTION_PHONE:
            break;
        case OPTION_RADIO:
            break;
        case OPTION_MON_STATUS:
            break;
        case OPTION_SETTINGS:
            DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->exitBgSpriteId]);
            DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->exitSpriteId]);

            for (u32 i = 0; i < 10; i++)
            {
                DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->themeSpriteIds[i]]);
            }
            break;
        case OPTION_MAIN_MENU:
            for (u32 i = 0; i < sPokeGearResources->numOptions; i++)
            {
                DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->menuSpriteIds[i]]);
            }

            DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->exitBgSpriteId]);
            DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->exitSpriteId]);
            DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->clockSpriteId]);

            for (u32 i = 0; i < 6; i++)
            {
                DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->digitSpriteIds[i]]);
            }
            break;
    }
}

static void UnloadPokeGearOptionData(u8 option)
{
    switch (option)
    {
        case OPTION_REGION_MAP:
            break;
        case OPTION_PHONE:
            break;
        case OPTION_RADIO:
            break;
        case OPTION_MON_STATUS:
            break;
        case OPTION_SETTINGS:
            gSaveBlock2Ptr->pokeGearStyle = sPokeGearResources->activeStyle;
            break;
        case OPTION_MAIN_MENU:
            ClearWindowTilemap(WIN_DESC);
            CopyWindowToVram(WIN_DESC, COPYWIN_FULL);
            break;
    }
}

// PokeGear Style Functions
static void LoadPokeGearStyle(u8 style)
{
    LZ77UnCompVram(sPokeGearStyleData[style].backgroundTileset, BG_CHAR_ADDR(3));
    LZ77UnCompVram(sPokeGearStyleData[style].foregroundTileset, BG_CHAR_ADDR(1));
    LZ77UnCompVram(sPokeGearStyleData[style].backgroundTilemap, BG_SCREEN_ADDR(30));
    LoadPalette(sPokeGearStyleData[style].palette, 0, 32);

    if (style == POKEGEAR_STYLE_RETRO)
        FillPalette(gPlttBufferFaded[10], 239, sizeof(u16));
    else
        FillPalette(RGB_WHITE, 239, sizeof(u16));
    
    sPokeGearResources->scrollState = 0;
    ChangeBgX(3, 0, 0);
    ChangeBgY(3, 0, 0);
}

static void PokeGearStyleFunc_CreateStyleSylphPokeBallSprite(void)
{
    LoadSpritePalette(&sPokeGearStyleSylphPokeBallSpritePalette);
    LoadCompressedSpriteSheet(&sPokeGearPokeBallSpriteSheet);
    sPokeGearResources->spriteId = CreateSprite(&sPokeGearPokeBallSpriteTemplate, 224, 144, 0);
}

static void PokeGearStyleFunc_CreateStyleRetroPokeBallSprite(void)
{
    LoadSpritePalette(&sPokeGearStyleRetroPokeBallSpritePalette);
    LoadCompressedSpriteSheet(&sPokeGearPokeBallSpriteSheet);
    sPokeGearResources->spriteId = CreateSprite(&sPokeGearPokeBallSpriteTemplate, 224, 144, 0);
}

static void PokeGearStyleFunc_CreateStyleUnownPokeBallSprite(void)
{
    LoadSpritePalette(&sPokeGearStyleUnownPokeBallSpritePalette);
    LoadCompressedSpriteSheet(&sPokeGearPokeBallSpriteSheet);
    sPokeGearResources->spriteId = CreateSprite(&sPokeGearPokeBallSpriteTemplate, 224, 144, 0);
}

static void PokeGearStyleFunc_CreateStyleGBAButtonSprites(void)
{
    LoadSpritePalette(&sPokeGearStyleGBAButtonsSpritePalette);
    LoadCompressedSpriteSheet(&sPokeGearGBAButtonsSpriteSheet);

    for (u32 i = 0; i < 10; i++)
    {
        sPokeGearResources->buttonSpriteIds[i] = CreateSprite(&sPokeGearGBAButtonsSpriteTemplates[i], sPokeGearGBAButtonsPositions[i].x, sPokeGearGBAButtonsPositions[i].y, i);
        StartSpriteAnim(&gSprites[sPokeGearResources->buttonSpriteIds[i]], i * 2);
        gSprites[sPokeGearResources->buttonSpriteIds[i]].tPosition = i;
    }
}

static void PokeGearStyleFunc_DestroyPokeBallSprite(void)
{
    DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->spriteId]);
}

static void PokeGearStyleFunc_DestroyGBAButtonSprites(void)
{
    for (u32 i = 0; i < 10; i++)
    {
        DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->buttonSpriteIds[i]]);
    }
}

static void PokeGearStyleFunc_ScrollDownRight(void)
{
    ChangeBgX(3, Q_8_8(0.25), BG_COORD_SUB);
    ChangeBgY(3, Q_8_8(0.25), BG_COORD_SUB);
}

static void PokeGearStyleFunc_ScrollRight(void)
{
    ChangeBgX(3, Q_8_8(0.25), BG_COORD_SUB);
}

static void PokeGearStyleFunc_ScrollUp(void)
{
    ChangeBgY(3, Q_8_8(0.25), BG_COORD_ADD);
}

static void PokeGearStyleFunc_ScrollZigZag(void)
{
    switch (sPokeGearResources->scrollState)
    {
        case 0:
            ChangeBgX(3, Q_8_8(0.25), BG_COORD_ADD);
            sPokeGearResources->scrollTimer++;
            
            if (sPokeGearResources->scrollTimer == 300)
            {
                sPokeGearResources->scrollState++;
                sPokeGearResources->scrollTimer = 0;
            }
            break;
        case 1:
            ChangeBgX(3, Q_8_8(0.25), BG_COORD_SUB);
            ChangeBgY(3, Q_8_8(0.125), BG_COORD_ADD);
            sPokeGearResources->scrollTimer++;

            if (sPokeGearResources->scrollTimer == 300)
            {
                sPokeGearResources->scrollState--;
                sPokeGearResources->scrollTimer = 0;
            }
            break;
    }
}

// Sprite Callbacks
static void SpriteCB_PokeBall(struct Sprite *sprite)
{
    sprite->y2 = sPokeGearResources->bgOffset.y;
}

static void SpriteCB_ClockDigits(struct Sprite *sprite)
{
    bool8 clock = gSaveBlock2Ptr->twentyFourHourClock;
    u8 value = sprite->tState;

    if (sprite->tState == STATE_INIT || sPokeGearResources->fakeSeconds == 0)
    {
        switch (sprite->tPosition)
        {
            case 0:
                value = gSaveBlock2Ptr->inGameTime.hours;
                if (!clock)
                {
                    if (value > 12)
                        value -= 12;
                    else if (value == 0)
                        value = 12;
                }
                value = value / 10;
                if (value != 0)
                    value += 1;
                break;
            case 1:
                value = gSaveBlock2Ptr->inGameTime.hours;
                if (!clock)
                {
                    if (value > 12)
                        value -= 12;
                    else if (value == 0)
                        value = 12;
                }
                value = value % 10 + 1;
                break;
            case 2:
                // handled outside of switch
                break;
            case 3:
                value = gSaveBlock2Ptr->inGameTime.minutes / 10 + 1;
                break;
            case 4:
                value = gSaveBlock2Ptr->inGameTime.minutes % 10 + 1;
                break;
            case 5:
                if (clock)
                    value = 13;
                else if (gSaveBlock2Ptr->inGameTime.hours < 12)
                    value = 14;
                else
                    value = 15;
                break;
            default:
                value = 0;
                break;
        }
    }

    if (sprite->tPosition == 2)
    {
        if (sPokeGearResources->fakeSeconds < 30)
            value = 12;
        else
            value = 11;
    }

    if (sprite->tState != value)
    {
        sprite->tState = value;
        StartSpriteAnim(sprite, value);
    }

    sprite->x2 = 512 - sPokeGearResources->bgOffset.x;
}

static void SpriteCB_Exit(struct Sprite *sprite)
{
    if (JOY_NEW(B_BUTTON))
    {
        switch (sprite->data[0])
        {
            case TRUE:
                StartSpriteAnimIfDifferent(sprite, 1);
                break;
            case FALSE:
                StartSpriteAnimIfDifferent(sprite, 3);
                break;
        }
    }

    if (sprite->animEnded)
    {
        switch (sprite->data[0])
        {
            case TRUE:
                StartSpriteAnimIfDifferent(sprite, 0);
                break;
            case FALSE:
                StartSpriteAnimIfDifferent(sprite, 2);
                break;
        }
    }

    sprite->x2 = sPokeGearResources->bgOffset.y;
}

static void SpriteCB_ExitBg(struct Sprite *sprite)
{
    sprite->x2 = sPokeGearResources->bgOffset.y;
}

static void SpriteCB_ClockBar(struct Sprite *sprite)
{
    sprite->x2 = 512 - sPokeGearResources->bgOffset.x;
}

static void SpriteCB_MainMenuIcon(struct Sprite *sprite)
{
    if (sPokeGearResources->selectedOption == sprite->tPosition)
    {
        StartSpriteAnimIfDifferent(sprite, 1);
        if (sprite->tTimer < OPTION_WOBBLE_TIMER)
            sprite->tTimer++;
    }
    else if (sPokeGearResources->selectedOption != sprite->tPosition && sprite->animNum == 1)
    {
        StartSpriteAnimIfDifferent(sprite, 2);
        sprite->tTimer = 0;
    }

    if (sPokeGearResources->selectedOption == sprite->tPosition && sprite->animEnded && sprite->tTimer == OPTION_WOBBLE_TIMER)
        StartSpriteAffineAnimIfDifferent(sprite, 1);
    else
        StartSpriteAffineAnimIfDifferent(sprite, 0);

    sprite->x2 = sPokeGearResources->bgOffset.x;
}

static void SpriteCB_ThemeIcon(struct Sprite *sprite)
{
    sprite->x2 = sPokeGearResources->bgOffset.x;
}

static void SpriteCB_GBAButtons(struct Sprite *sprite)
{
    if (JOY_HELD_RAW(1 << sprite->tPosition))
        sprite->tState = TRUE;
    else
        sprite->tState = FALSE;

    StartSpriteAnimIfDifferent(sprite, (sprite->tPosition * 2) + sprite->tState);
    sprite->y2 = sPokeGearResources->bgOffset.y;
}

// Animation
static bool32 SlideMainMenuIn(void)
{
    u32 i;

    for (i = 0; i < 4; i++)
    {
        if (sPokeGearResources->bgOffset.x > 1)
        {
            sPokeGearResources->bgOffset.x -= OPTION_SCROLL_SPEED;
            SetGpuReg(REG_OFFSET_BG2HOFS, 512 - sPokeGearResources->bgOffset.x);
        }
        else
        {
            SetGpuReg(REG_OFFSET_BG2HOFS, 0);
            sPokeGearResources->bgOffset.x = 0;
            return TRUE;
        }
    }

    if (sPokeGearResources->bgOffset.y > 1)
    {
        sPokeGearResources->bgOffset.y -= OPTION_SCROLL_SPEED;
        SetBackgroundOffsetScanlineBuffers(sPokeGearResources->bgOffset.y);
    }
    else
    {
        sPokeGearResources->bgOffset.y = 0;
        SetBackgroundOffsetScanlineBuffers(sPokeGearResources->bgOffset.y);
    }

    return FALSE;
}

static bool32 SlideMainMenuOut(void)
{
    u32 i;

    for (i = 0; i < 4; i++)
    {
        if (sPokeGearResources->bgOffset.x < OPTION_SCROLL_X)
        {
            sPokeGearResources->bgOffset.x += OPTION_SCROLL_SPEED;
            SetGpuReg(REG_OFFSET_BG2HOFS, 512 - sPokeGearResources->bgOffset.x);
        }
        else
        {
            SetGpuReg(REG_OFFSET_BG2HOFS, OPTION_SCROLL_X);
            sPokeGearResources->bgOffset.x = 0;
            return TRUE;
        }
    }

    if (sPokeGearResources->bgOffset.y < OPTION_SCROLL_Y)
    {
        sPokeGearResources->bgOffset.y += OPTION_SCROLL_SPEED;
        SetBackgroundOffsetScanlineBuffers(sPokeGearResources->bgOffset.y);
    }
    else
    {
        sPokeGearResources->bgOffset.y = OPTION_SCROLL_Y;
        SetBackgroundOffsetScanlineBuffers(sPokeGearResources->bgOffset.y);
        return TRUE;
    }

    return FALSE;
}

static void SetBackgroundOffsetScanlineBuffers(u8 offset)
{
    u32 i;

    for (i = 0; i < 24; i++)
    {
        gScanlineEffectRegBuffers[0][i] = offset;
        gScanlineEffectRegBuffers[1][i] = offset;
    }

    for (i = DISPLAY_HEIGHT - 56; i < DISPLAY_HEIGHT; i++)
    {
        gScanlineEffectRegBuffers[0][i] = -offset;
        gScanlineEffectRegBuffers[1][i] = -offset;
    }
}
