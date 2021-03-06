﻿#ifndef _DEFINES_H
#define _DEFINES_H

#include "Point.h"
#include "Rect.h"
#include "gameData/DescIdx.h"
#include <SDL.h>
#include <array>
#include <string>
#include <vector>

struct TerrainDesc;

// define the mode to compile (if all is uncommented, the game will compile in normal mode
// in admin mode, there are some key combos to open debugger, resource viewer and so on
//#define _ADMINMODE

// callback parameters
enum
{
    // NOTE: we don't have a global WINDOW_QUIT_MESSAGE, cause if there were more than one window created by a callback function
    //      we wouldn't know which window was closed by the user. so we need a specified quit-message for each window.

    // first call of a callback function
    INITIALIZING_CALL = -1,
    // a callback that is registered at the gameloop will be called from the gameloop with this value
    CALL_FROM_GAMELOOP = -2,
    // if user goes to main menu, all menubar callbacks will be called with MAP_QUIT
    MAP_QUIT = -3,
    // parameter for closing the debugger window
    DEBUGGER_QUIT = -4,
    // this will happen every time the user clicks anywhere on the window
    WINDOW_CLICKED_CALL = -5,
    // this window quit message is ONLY usable to call a callback function explicit with this value
    WINDOW_QUIT_MESSAGE = -6
};

// BOBTYPES
enum
{
    BOBTYPE01 = 1,
    BOBTYPE02 = 2,
    BOBTYPE03 = 3,
    BOBTYPE04 = 4,
    BOBTYPE05 = 5,
    BOBTYPE07 = 7,
    BOBTYPE14 = 14
};

// FILE ENDINGS
enum
{
    LST = 0,
    BOB,
    IDX,
    BBM,
    LBM,
    WLD,
    SWD,
    GOU
};

// Structure for Bobtypes 2 (RLE-Bitmaps), 4 (specific Bitmaps), 14 (uncompressed Bitmaps)
struct bobBMP
{
    Uint16 nx;
    Uint16 ny;
    Uint16 w;
    Uint16 h;
    SDL_Surface* surface = nullptr;
};

// Structure for Bobtype 5 (Palette)
struct bobPAL
{
    std::array<SDL_Color, 256> colors;
};

// Structure for Bobtype 7 (Shadow-Bitmaps)
struct bobSHADOW
{
    Uint16 nx;
    Uint16 ny;
    Uint16 w;
    Uint16 h;
    SDL_Surface* surface = nullptr;
};

// Datatypes for the Map
// vector structure
struct vector
{
    float x, y, z;
};
struct IntVector
{
    Sint32 x, y, z;
};
// structure for the 250 9Byte-Items from die 2250Bytes long map header
struct MapHeaderItem
{
    Uint8 type; // land or water (snow, swamp and lava are not counted)
    Uint16 x;
    Uint16 y;
    Uint32 area; // number of vertices this area has
};
// point structure
struct MapNode
{
    Uint16 VertexX; /* number of the vertex on x-axis */
    Uint16 VertexY; /* number of the vertex on y-axis */
    Sint32 x;
    Sint32 y; /* calculated with section 1 */
    int z;    /* calculated with section 1 */
    Uint8 h;  /* section 1 */
    Sint32 i; /* calculated light values for new shading by SGE (a 16 bit integer shifted left 16 times --> fixed point math for speed) */
    vector flatVector;
    vector normVector;
    Uint8 rsuTexture; /* section 2 */
    Uint8 usdTexture; /* section 3 */
    Uint8 road;       /* section 4 */
    Uint8 objectType; /* section 5 */
    Uint8 objectInfo; /* section 6 */
    Uint8 animal;     /* section 7 */
    Uint8 unknown1;   /* section 8 */
    Uint8 build;      /* section 9 */
    Uint8 unknown2;   /* section 10 */
    Uint8 unknown3;   /* section 11 */
    Uint8 resource;   /* section 12 */
    Uint8 shading;    /* section 13 */
    Uint8 unknown5;   /* section 14 */

    operator IntVector() const
    {
        IntVector result;
        result.x = x;
        result.y = y;
        result.z = z;
        return result;
    }
};
// structure for display, cause SDL_Rect's datatypes are too small
using DisplayRectangle = RectBase<Sint32>;
using Point16 = Point<Sint16>;
using Point32 = Point<Sint32>;
// map types
enum MapType
{
    MAP_GREENLAND = 0x00,
    MAP_WASTELAND = 0x01,
    MAP_WINTERLAND = 0x02
};
// map strutcture
struct bobMAP
{
    Uint16 height;
    Uint16 height_old;
    Uint16 height_pixel;
    Uint16 width;
    Uint16 width_old;
    Uint16 width_pixel;
    MapType type;
    Uint8 player;
    // these are the original values
    std::array<Uint16, 7> HQx;
    std::array<Uint16, 7> HQy;
    // 250 items from the big map header
    std::array<MapHeaderItem, 250> header;
    std::vector<MapNode> vertex;
    MapNode& getVertex(unsigned x, unsigned y) { return vertex[y * width + x]; }
    MapNode& getVertex(Point32 pos) { return vertex[pos.y * width + pos.x]; }
    const MapNode& getVertex(unsigned x, unsigned y) const { return vertex[y * width + x]; }
    const MapNode& getVertex(Point32 pos) const { return vertex[pos.y * width + pos.x]; }
    std::vector<DescIdx<TerrainDesc>> s2IdToTerrain;
    // Initializes or updates the vertex indices and coordinates
    void initVertexCoords();
    /// Updates x,y,z positions (e.g. after height change)
    void updateVertexCoords();

    const std::string& getName() const { return name; }
    const std::string& getAuthor() const { return author; }
    void setName(const std::string& newName);
    void setAuthor(const std::string& newAuthor);

private:
    std::string name;
    std::string author;
};
// structure to save vertex coordinates
struct cursorPoint : public Point32
{
    int blit_x;
    int blit_y;
    bool active;
    bool fill_rsu;
    bool fill_usd;
};

// IMPORTANT: for enumerating the contents of loaded files put the constants in the right order here.
//           if the order of file loading changes, so change the constants in the same way!
//           (these are the array-indices for an array of BobtypeBMP-Structures)

// enumeration for BobtypePAL (palettes)
enum
{
    RESOURCE_PALETTE = 0,
    IO_PALETTE
};

// font alignment (after all used by CFont and other objects using CFont)
enum FontAlign
{
    ALIGN_LEFT = 0,
    ALIGN_MIDDLE,
    ALIGN_RIGHT
};
// i put some color values here, cause we need NUM_FONT_COLORS in the next enumeration
// font color (after all used by CFont and other objects using CFont)
enum
{
    FONT_BLUE = 0,
    FONT_RED,
    FONT_ORANGE,
    FONT_GREEN,
    FONT_MINTGREEN,
    FONT_YELLOW,
    FONT_RED_BRIGHT,

    NUM_FONT_COLORS
};
// player colors, necessary for the read_bob03- and read_bob04-function
enum
{
    PLAYER_BLUE = 0x80,
    PLAYER_RED = 0x88,
    PLAYER_ORANGE = 0x04,
    PLAYER_GREEN = 0x85,
    PLAYER_MINTGREEN = 0x94,
    PLAYER_YELLOW = 0x01,
    PLAYER_RED_BRIGHT = 0x10
};

// enumeration for BobtypeBMP (pics)
enum
{
    // BEGIN: /GFX/PICS/SETUP997.LBM
    SPLASHSCREEN_LOADING_S2SCREEN = 0,
    // END: /GFX/PICS/SETUP997.LBM

    // BEGIN: /GFX/PICS/SETUP000.LBM
    SPLASHSCREEN_MAINMENU_BROWN,
    // END: /GFX/PICS/SETUP000.LBM

    // BEGIN: /GFX/PICS/SETUP010.LBM
    SPLASHSCREEN_MAINMENU,
    // END: /GFX/PICS/SETUP010.LBM

    // BEGIN: /GFX/PICS/SETUP011.LBM
    SPLASHSCREEN_SUBMENU1,
    // END: /GFX/PICS/SETUP011.LBM

    // BEGIN: /GFX/PICS/SETUP012.LBM
    SPLASHSCREEN_SUBMENU2,
    // END: /GFX/PICS/SETUP012.LBM

    // BEGIN: /GFX/PICS/SETUP013.LBM
    SPLASHSCREEN_SUBMENU3,
    // END: /GFX/PICS/SETUP013.LBM

    // BEGIN: /GFX/PICS/SETUP014.LBM
    SPLASHSCREEN_SUBMENU4,
    // END: /GFX/PICS/SETUP014.LBM

    // BEGIN: /GFX/PICS/SETUP015.LBM
    SPLASHSCREEN_SUBMENU5,
    // END: /GFX/PICS/SETUP015.LBM

    // BEGIN: /GFX/PICS/SETUP666.LBM
    SPLASHSCREEN_UNIVERSE1,
    // END: /GFX/PICS/SETUP666.LBM

    // BEGIN: /GFX/PICS/SETUP667.LBM
    SPLASHSCREEN_SUN1,
    // END: /GFX/PICS/SETUP667.LBM

    // BEGIN: /GFX/PICS/SETUP801.LBM
    SPLASHSCREEN_SETUP801,
    // END: /GFX/PICS/SETUP801.LBM

    // BEGIN: /GFX/PICS/SETUP802.LBM
    SPLASHSCREEN_LOADING_STANDARD,
    // END: /GFX/PICS/SETUP802.LBM

    // BEGIN: /GFX/PICS/SETUP803.LBM
    SPLASHSCREEN_LOADING_GREENLAND1,
    // END: /GFX/PICS/SETUP803.LBM

    // BEGIN: /GFX/PICS/SETUP804.LBM
    SPLASHSCREEN_LOADING_WASTELAND,
    // END: /GFX/PICS/SETUP804.LBM

    // BEGIN: /GFX/PICS/SETUP805.LBM
    SPLASHSCREEN_LOADING_GREENLAND2,
    // END: /GFX/PICS/SETUP805.LBM

    // BEGIN: /GFX/PICS/SETUP806.LBM
    SPLASHSCREEN_LOADING_GREENLAND3,
    // END: /GFX/PICS/SETUP806.LBM

    // BEGIN: /GFX/PICS/SETUP810.LBM
    SPLASHSCREEN_LOADING_WINTER1,
    // END: /GFX/PICS/SETUP810.LBM

    // BEGIN: /GFX/PICS/SETUP811.LBM
    SPLASHSCREEN_LOADING_WINTER2,
    // END: /GFX/PICS/SETUP811.LBM

    // BEGIN: /GFX/PICS/SETUP895.LBM
    SPLASHSCREEN_LOADING_SETUP895,
    // END: /GFX/PICS/SETUP895.LBM

    // BEGIN: /GFX/PICS/SETUP896.LBM
    SPLASHSCREEN_LOADING_ROMANCAMPAIGN1,
    // END: /GFX/PICS/SETUP896.LBM

    // BEGIN: /GFX/PICS/SETUP897.LBM
    SPLASHSCREEN_LOADING_ROMANCAMPAIGN2,
    // END: /GFX/PICS/SETUP897.LBM

    // BEGIN: /GFX/PICS/SETUP898.LBM
    SPLASHSCREEN_LOADING_ROMANCAMPAIGN3,
    // END: /GFX/PICS/SETUP898.LBM

    // BEGIN: /GFX/PICS/SETUP899.LBM
    SPLASHSCREEN_LOADING_ROMANCAMPAIGN_GREY,
    // END: /GFX/PICS/SETUP899.LBM

    // BEGIN: /GFX/PICS/SETUP990.LBM
    SPLASHSCREEN_SETUP990,
    // END: /GFX/PICS/SETUP990.LBM

    // BEGIN: /GFX/PICS/WORLD.LBM
    SPLASHSCREEN_WORLDCAMPAIGN,
    // END: /GFX/PICS/WORLD.LBM

    // BEGIN: /GFX/PICS/WORLDMSK.LBM
    SPLASHSCREEN_WORLDCAMPAIGN_SECTIONS,
    // END: /GFX/PICS/WORLDMSK.LBM

    // BEGIN: /DATA/RESOURCE.IDX (AND /DATA/RESOURCE.DAT) OR /DATA/EDITRES.IDX (AND /DATA/EDITRES.DAT)
    // BEGIN: FONT

    /// IMPORTANT:   BECAUSE OF MULTIPLE COLORS FOR EACH CHARACTER THIS FONT-ENUMERATION IS NO LONGER CONSISTENT.
    ///             ONLY THE START-VALUES (FONT9_SPACE, FONT11_SPACE, FONT14_SPACE) HAVE THE RIGHT INDEX!

    // fontsize 11
    FONT11_SPACE,                                       // spacebar
    FONT11_EXCLAMATION_POINT,                           // !
    FONT11_DOUBLE_QUOTES,                               // "
    FONT11_SHARP,                                       // #
    FONT11_DOLLAR,                                      // $
    FONT11_PERCENT,                                     // %
    FONT11_AMPERSAND,                                   // &
    FONT11_SINGLE_QUOTES,                               // '
    FONT11_ROUND_BRACKET_OPEN,                          // (
    FONT11_ROUND_BRACKET_CLOSE,                         // )
    FONT11_STAR,                                        // *
    FONT11_PLUS,                                        // +
    FONT11_COMMA,                                       // ,
    FONT11_MINUS,                                       // -
    FONT11_DOT,                                         // .
    FONT11_SLASH,                                       // /
    FONT11_0,                                           // 0
    FONT11_1,                                           // 1
    FONT11_2,                                           // 2
    FONT11_3,                                           // 3
    FONT11_4,                                           // 4
    FONT11_5,                                           // 5
    FONT11_6,                                           // 6
    FONT11_7,                                           // 7
    FONT11_8,                                           // 8
    FONT11_9,                                           // 9
    FONT11_COLON,                                       // :
    FONT11_SEMICOLON,                                   // ;
    FONT11_ARROW_BRACKET_OPEN,                          // <
    FONT11_EQUAL,                                       // =
    FONT11_ARROW_BRACKET_CLOSE,                         // >
    FONT11_INTERROGATION_POINT,                         // ?
    FONT11_AT,                                          // @
    FONT11_A,                                           // A
    FONT11_B,                                           // B
    FONT11_C,                                           // C
    FONT11_D,                                           // D
    FONT11_E,                                           // E
    FONT11_F,                                           // F
    FONT11_G,                                           // G
    FONT11_H,                                           // H
    FONT11_I,                                           // I
    FONT11_J,                                           // J
    FONT11_K,                                           // K
    FONT11_L,                                           // L
    FONT11_M,                                           // M
    FONT11_N,                                           // N
    FONT11_O,                                           // O
    FONT11_P,                                           // P
    FONT11_Q,                                           // Q
    FONT11_R,                                           // R
    FONT11_S,                                           // S
    FONT11_T,                                           // T
    FONT11_U,                                           // U
    FONT11_V,                                           // V
    FONT11_W,                                           // W
    FONT11_X,                                           // X
    FONT11_Y,                                           // Y
    FONT11_Z,                                           // Z
    FONT11_BACKSLASH,                                   //
    FONT11_UNDERSCORE,                                  // _
    FONT11_a,                                           // a
    FONT11_b,                                           // b
    FONT11_c,                                           // c
    FONT11_d,                                           // d
    FONT11_e,                                           // e
    FONT11_f,                                           // f
    FONT11_g,                                           // g
    FONT11_h,                                           // h
    FONT11_i,                                           // i
    FONT11_j,                                           // j
    FONT11_k,                                           // k
    FONT11_l,                                           // l
    FONT11_m,                                           // m
    FONT11_n,                                           // n
    FONT11_o,                                           // o
    FONT11_p,                                           // p
    FONT11_q,                                           // q
    FONT11_r,                                           // r
    FONT11_s,                                           // s
    FONT11_t,                                           // t
    FONT11_u,                                           // u
    FONT11_v,                                           // v
    FONT11_w,                                           // w
    FONT11_x,                                           // x
    FONT11_y,                                           // y
    FONT11_z,                                           // z
    FONT11_ANSI_199,                                    // Ç
    FONT11_ANSI_252,                                    // ü
    FONT11_ANSI_233,                                    // é
    FONT11_ANSI_226,                                    // â
    FONT11_ANSI_228,                                    // ä
    FONT11_ANSI_224,                                    // à
    FONT11_ANSI_231,                                    // ç
    FONT11_ANSI_234,                                    // ê
    FONT11_ANSI_235,                                    // ë
    FONT11_ANSI_232,                                    // è
    FONT11_ANSI_239,                                    // ï
    FONT11_ANSI_238,                                    // î
    FONT11_ANSI_236,                                    // ì
    FONT11_ANSI_196,                                    // Ä
    FONT11_ANSI_244,                                    // ô
    FONT11_ANSI_246,                                    // ö
    FONT11_ANSI_242,                                    // ò
    FONT11_ANSI_251,                                    // û
    FONT11_ANSI_249,                                    // ù
    FONT11_ANSI_214,                                    // Ö
    FONT11_ANSI_220,                                    // Ü
    FONT11_ANSI_225,                                    // á
    FONT11_ANSI_237,                                    // í
    FONT11_ANSI_243,                                    // ó
    FONT11_ANSI_250,                                    // ú
    FONT11_ANSI_241,                                    // ñ
    FONT11_ANSI_223,                                    // ß
    FONT11_ANSI_169,                                    // ©
                                                        // fontsize 9
    FONT9_SPACE = FONT11_SPACE + NUM_FONT_COLORS * 115, // spacebar
    FONT9_EXCLAMATION_POINT,                            // !
    FONT9_DOUBLE_QUOTES,                                // "
    FONT9_SHARP,                                        // #
    FONT9_DOLLAR,                                       // $
    FONT9_PERCENT,                                      // %
    FONT9_AMPERSAND,                                    // &
    FONT9_SINGLE_QUOTES,                                // '
    FONT9_ROUND_BRACKET_OPEN,                           // (
    FONT9_ROUND_BRACKET_CLOSE,                          // )
    FONT9_STAR,                                         // *
    FONT9_PLUS,                                         // +
    FONT9_COMMA,                                        // ,
    FONT9_MINUS,                                        // -
    FONT9_DOT,                                          // .
    FONT9_SLASH,                                        // /
    FONT9_0,                                            // 0
    FONT9_1,                                            // 1
    FONT9_2,                                            // 2
    FONT9_3,                                            // 3
    FONT9_4,                                            // 4
    FONT9_5,                                            // 5
    FONT9_6,                                            // 6
    FONT9_7,                                            // 7
    FONT9_8,                                            // 8
    FONT9_9,                                            // 9
    FONT9_COLON,                                        // :
    FONT9_SEMICOLON,                                    // ;
    FONT9_ARROW_BRACKET_OPEN,                           // <
    FONT9_EQUAL,                                        // =
    FONT9_ARROW_BRACKET_CLOSE,                          // >
    FONT9_INTERROGATION_POINT,                          // ?
    FONT9_AT,                                           // @
    FONT9_A,                                            // A
    FONT9_B,                                            // B
    FONT9_C,                                            // C
    FONT9_D,                                            // D
    FONT9_E,                                            // E
    FONT9_F,                                            // F
    FONT9_G,                                            // G
    FONT9_H,                                            // H
    FONT9_I,                                            // I
    FONT9_J,                                            // J
    FONT9_K,                                            // K
    FONT9_L,                                            // L
    FONT9_M,                                            // M
    FONT9_N,                                            // N
    FONT9_O,                                            // O
    FONT9_P,                                            // P
    FONT9_Q,                                            // Q
    FONT9_R,                                            // R
    FONT9_S,                                            // S
    FONT9_T,                                            // T
    FONT9_U,                                            // U
    FONT9_V,                                            // V
    FONT9_W,                                            // W
    FONT9_X,                                            // X
    FONT9_Y,                                            // Y
    FONT9_Z,                                            // Z
    FONT9_BACKSLASH,                                    //
    FONT9_UNDERSCORE,                                   // _
    FONT9_a,                                            // a
    FONT9_b,                                            // b
    FONT9_c,                                            // c
    FONT9_d,                                            // d
    FONT9_e,                                            // e
    FONT9_f,                                            // f
    FONT9_g,                                            // g
    FONT9_h,                                            // h
    FONT9_i,                                            // i
    FONT9_j,                                            // j
    FONT9_k,                                            // k
    FONT9_l,                                            // l
    FONT9_m,                                            // m
    FONT9_n,                                            // n
    FONT9_o,                                            // o
    FONT9_p,                                            // p
    FONT9_q,                                            // q
    FONT9_r,                                            // r
    FONT9_s,                                            // s
    FONT9_t,                                            // t
    FONT9_u,                                            // u
    FONT9_v,                                            // v
    FONT9_w,                                            // w
    FONT9_x,                                            // x
    FONT9_y,                                            // y
    FONT9_z,                                            // z
    FONT9_ANSI_199,                                     // Ç
    FONT9_ANSI_252,                                     // ü
    FONT9_ANSI_233,                                     // é
    FONT9_ANSI_226,                                     // â
    FONT9_ANSI_228,                                     // ä
    FONT9_ANSI_224,                                     // à
    FONT9_ANSI_231,                                     // ç
    FONT9_ANSI_234,                                     // ê
    FONT9_ANSI_235,                                     // ë
    FONT9_ANSI_232,                                     // è
    FONT9_ANSI_239,                                     // ï
    FONT9_ANSI_238,                                     // î
    FONT9_ANSI_236,                                     // ì
    FONT9_ANSI_196,                                     // Ä
    FONT9_ANSI_244,                                     // ô
    FONT9_ANSI_246,                                     // ö
    FONT9_ANSI_242,                                     // ò
    FONT9_ANSI_251,                                     // û
    FONT9_ANSI_249,                                     // ù
    FONT9_ANSI_214,                                     // Ö
    FONT9_ANSI_220,                                     // Ü
    FONT9_ANSI_225,                                     // á
    FONT9_ANSI_237,                                     // í
    FONT9_ANSI_243,                                     // ó
    FONT9_ANSI_250,                                     // ú
    FONT9_ANSI_241,                                     // ñ
    FONT9_ANSI_223,                                     // ß
    FONT9_ANSI_169,                                     // ©
                                                        // fontsize 14
    FONT14_SPACE = FONT9_SPACE + NUM_FONT_COLORS * 115, // spacebar
    FONT14_EXCLAMATION_POINT,                           // !
    FONT14_DOUBLE_QUOTES,                               // "
    FONT14_SHARP,                                       // #
    FONT14_DOLLAR,                                      // $
    FONT14_PERCENT,                                     // %
    FONT14_AMPERSAND,                                   // &
    FONT14_SINGLE_QUOTES,                               // '
    FONT14_ROUND_BRACKET_OPEN,                          // (
    FONT14_ROUND_BRACKET_CLOSE,                         // )
    FONT14_STAR,                                        // *
    FONT14_PLUS,                                        // +
    FONT14_COMMA,                                       // ,
    FONT14_MINUS,                                       // -
    FONT14_DOT,                                         // .
    FONT14_SLASH,                                       // /
    FONT14_0,                                           // 0
    FONT14_1,                                           // 1
    FONT14_2,                                           // 2
    FONT14_3,                                           // 3
    FONT14_4,                                           // 4
    FONT14_5,                                           // 5
    FONT14_6,                                           // 6
    FONT14_7,                                           // 7
    FONT14_8,                                           // 8
    FONT14_9,                                           // 9
    FONT14_COLON,                                       // :
    FONT14_SEMICOLON,                                   // ;
    FONT14_ARROW_BRACKET_OPEN,                          // <
    FONT14_EQUAL,                                       // =
    FONT14_ARROW_BRACKET_CLOSE,                         // >
    FONT14_INTERROGATION_POINT,                         // ?
    FONT14_AT,                                          // @
    FONT14_A,                                           // A
    FONT14_B,                                           // B
    FONT14_C,                                           // C
    FONT14_D,                                           // D
    FONT14_E,                                           // E
    FONT14_F,                                           // F
    FONT14_G,                                           // G
    FONT14_H,                                           // H
    FONT14_I,                                           // I
    FONT14_J,                                           // J
    FONT14_K,                                           // K
    FONT14_L,                                           // L
    FONT14_M,                                           // M
    FONT14_N,                                           // N
    FONT14_O,                                           // O
    FONT14_P,                                           // P
    FONT14_Q,                                           // Q
    FONT14_R,                                           // R
    FONT14_S,                                           // S
    FONT14_T,                                           // T
    FONT14_U,                                           // U
    FONT14_V,                                           // V
    FONT14_W,                                           // W
    FONT14_X,                                           // X
    FONT14_Y,                                           // Y
    FONT14_Z,                                           // Z
    FONT14_BACKSLASH,                                   //
    FONT14_UNDERSCORE,                                  // _
    FONT14_a,                                           // a
    FONT14_b,                                           // b
    FONT14_c,                                           // c
    FONT14_d,                                           // d
    FONT14_e,                                           // e
    FONT14_f,                                           // f
    FONT14_g,                                           // g
    FONT14_h,                                           // h
    FONT14_i,                                           // i
    FONT14_j,                                           // j
    FONT14_k,                                           // k
    FONT14_l,                                           // l
    FONT14_m,                                           // m
    FONT14_n,                                           // n
    FONT14_o,                                           // o
    FONT14_p,                                           // p
    FONT14_q,                                           // q
    FONT14_r,                                           // r
    FONT14_s,                                           // s
    FONT14_t,                                           // t
    FONT14_u,                                           // u
    FONT14_v,                                           // v
    FONT14_w,                                           // w
    FONT14_x,                                           // x
    FONT14_y,                                           // y
    FONT14_z,                                           // z
    FONT14_ANSI_199,                                    // Ç
    FONT14_ANSI_252,                                    // ü
    FONT14_ANSI_233,                                    // é
    FONT14_ANSI_226,                                    // â
    FONT14_ANSI_228,                                    // ä
    FONT14_ANSI_224,                                    // à
    FONT14_ANSI_231,                                    // ç
    FONT14_ANSI_234,                                    // ê
    FONT14_ANSI_235,                                    // ë
    FONT14_ANSI_232,                                    // è
    FONT14_ANSI_239,                                    // ï
    FONT14_ANSI_238,                                    // î
    FONT14_ANSI_236,                                    // ì
    FONT14_ANSI_196,                                    // Ä
    FONT14_ANSI_244,                                    // ô
    FONT14_ANSI_246,                                    // ö
    FONT14_ANSI_242,                                    // ò
    FONT14_ANSI_251,                                    // û
    FONT14_ANSI_249,                                    // ù
    FONT14_ANSI_214,                                    // Ö
    FONT14_ANSI_220,                                    // Ü
    FONT14_ANSI_225,                                    // á
    FONT14_ANSI_237,                                    // í
    FONT14_ANSI_243,                                    // ó
    FONT14_ANSI_250,                                    // ú
    FONT14_ANSI_241,                                    // ñ
    FONT14_ANSI_223,                                    // ß
    FONT14_ANSI_169,                                    // ©
                                                        // END: FONT

    // now the main resources will follow (frames, cursor, ...)
    // resolution behind means not resolution of the pic but window resolution the pic belongs to
    MAINFRAME_640_480 = 2441,
    SPLITFRAME_LEFT_640_480,
    SPLITFRAME_RIGHT_640_480,
    MAINFRAME_800_600,
    SPLITFRAME_LEFT_800_600,
    SPLITFRAME_RIGHT_800_600,
    MAINFRAME_1024_768,
    SPLITFRAME_LEFT_1024_768,
    SPLITFRAME_RIGHT_1024_768,
    MAINFRAME_LEFT_1280_1024,
    MAINFRAME_RIGHT_1280_1024,
    SPLITFRAME_LEFT_1280_1024,
    SPLITFRAME_RIGHT_1280_1024,
    STATUE_UP_LEFT,
    STATUE_UP_RIGHT,
    STATUE_DOWN_LEFT,
    STATUE_DOWN_RIGHT,
    SPLITFRAME_ADDITIONAL_LEFT_640_480,
    SPLITFRAME_ADDITIONAL_RIGHT_640_480,
    SPLITFRAME_ADDITIONAL_LEFT_800_600,
    SPLITFRAME_ADDITIONAL_RIGHT_800_600,
    SPLITFRAME_ADDITIONAL_LEFT_1024_768,
    SPLITFRAME_ADDITIONAL_RIGHT_1024_768,
    SPLITFRAME_ADDITIONAL_LEFT_1280_1024,
    SPLITFRAME_ADDITIONAL_RIGHT_1280_1024,
    MENUBAR,
    CURSOR,
    CURSOR_CLICKED,
    CROSS,
    MOON,
    CIRCLE_HIGH_GREY,
    CIRCLE_FLAT_GREY,
    WINDOW_LEFT_UPPER_CORNER,
    WINDOW_RIGHT_UPPER_CORNER,
    WINDOW_LEFT_FRAME,
    WINDOW_RIGHT_FRAME,
    WINDOW_LOWER_FRAME,
    WINDOW_BACKGROUND,
    WINDOW_UPPER_FRAME,
    WINDOW_UPPER_FRAME_MARKED,
    WINDOW_UPPER_FRAME_CLICKED,
    WINDOW_CORNER_RECTANGLE,
    WINDOW_BUTTON_RESIZE,
    WINDOW_BUTTON_CLOSE,
    WINDOW_BUTTON_MINIMIZE,
    WINDOW_CORNER_RECTANGLE_2, // unknown function
    WINDOW_BUTTON_RESIZE_CLICKED,
    WINDOW_BUTTON_CLOSE_CLICKED,
    WINDOW_BUTTON_MINIMIZE_CLICKED,
    WINDOW_CORNER_RECTANGLE_3, // unknown function
    WINDOW_BUTTON_RESIZE_MARKED,
    WINDOW_BUTTON_CLOSE_MARKED,
    WINDOW_BUTTON_MINIMIZE_MARKED,
    // END: /DATA/RESOURCE.IDX (AND /DATA/RESOURCE.DAT) OR /DATA/EDITRES.IDX (AND /DATA/EDITRES.DAT)

    // BEGIN: /DATA/IO/EDITIO.IDX (AND /DATA/IO/EDITIO.DAT)
    BUTTON_GREY_BRIGHT,
    BUTTON_GREY_DARK,
    BUTTON_RED1_BRIGHT,
    BUTTON_RED1_DARK,
    BUTTON_GREEN1_BRIGHT,
    BUTTON_GREEN1_DARK,
    BUTTON_GREEN2_BRIGHT,
    BUTTON_GREEN2_DARK,
    BUTTON_RED2_BRIGHT,
    BUTTON_RED2_DARK,
    BUTTON_STONE_BRIGHT,
    BUTTON_STONE_DARK,
    BUTTON_GREY_BACKGROUND,
    BUTTON_RED1_BACKGROUND,
    BUTTON_GREEN1_BACKGROUND,
    BUTTON_GREEN2_BACKGROUND,
    BUTTON_RED2_BACKGROUND,
    BUTTON_STONE_BACKGROUND,
    MENUBAR_BUILDHELP,
    PICTURE_SHOW_POLITICAL_EDGES,
    CIRCLE_BANG,
    MENUBAR_BUGKILL,
    PICTURE_SMALL_ARROW_UP,
    PICTURE_SMALL_ARROW_DOWN,
    PICTURE_SMALL_CIRCLE,
    PICTURE_TROWEL,
    MENUBAR_COMPUTER,
    MENUBAR_LOUPE,
    PICTURE_SMALL_TICK,
    PICTURE_SMALL_CROSS,
    MENUBAR_MINIMAP,
    PICTURE_SMALL_ARROW_LEFT,
    PICTURE_SMALL_ARROW_RIGHT,
    PICTURE_LETTER_I,
    PICTURE_GREENLAND_TEXTURE_SNOW,
    PICTURE_GREENLAND_TEXTURE_STEPPE,
    PICTURE_GREENLAND_TEXTURE_SWAMP,
    PICTURE_GREENLAND_TEXTURE_FLOWER,
    PICTURE_GREENLAND_TEXTURE_MINING1,
    PICTURE_GREENLAND_TEXTURE_MINING2,
    PICTURE_GREENLAND_TEXTURE_MINING3,
    PICTURE_GREENLAND_TEXTURE_MINING4,
    PICTURE_GREENLAND_TEXTURE_STEPPE_MEADOW1,
    PICTURE_GREENLAND_TEXTURE_MEADOW1,
    PICTURE_GREENLAND_TEXTURE_MEADOW2,
    PICTURE_GREENLAND_TEXTURE_MEADOW3,
    PICTURE_GREENLAND_TEXTURE_STEPPE_MEADOW2,
    PICTURE_GREENLAND_TEXTURE_MINING_MEADOW,
    PICTURE_GREENLAND_TEXTURE_WATER,
    PICTURE_GREENLAND_TEXTURE_LAVA,
    PICTURE_GREENLAND_TEXTURE_MEADOW_MIXED,
    PICTURE_WASTELAND_TEXTURE_SNOW,
    PICTURE_WASTELAND_TEXTURE_STEPPE,
    PICTURE_WASTELAND_TEXTURE_SWAMP,
    PICTURE_WASTELAND_TEXTURE_FLOWER,
    PICTURE_WASTELAND_TEXTURE_MINING1,
    PICTURE_WASTELAND_TEXTURE_MINING2,
    PICTURE_WASTELAND_TEXTURE_MINING3,
    PICTURE_WASTELAND_TEXTURE_MINING4,
    PICTURE_WASTELAND_TEXTURE_STEPPE_MEADOW1,
    PICTURE_WASTELAND_TEXTURE_MEADOW1,
    PICTURE_WASTELAND_TEXTURE_MEADOW2,
    PICTURE_WASTELAND_TEXTURE_MEADOW3,
    PICTURE_WASTELAND_TEXTURE_STEPPE_MEADOW2,
    PICTURE_WASTELAND_TEXTURE_MINING_MEADOW,
    PICTURE_WASTELAND_TEXTURE_WATER,
    PICTURE_WASTELAND_TEXTURE_LAVA,
    PICTURE_WINTERLAND_TEXTURE_SNOW,
    PICTURE_WINTERLAND_TEXTURE_STEPPE,
    PICTURE_WINTERLAND_TEXTURE_SWAMP,
    PICTURE_WINTERLAND_TEXTURE_FLOWER,
    PICTURE_WINTERLAND_TEXTURE_MINING1,
    PICTURE_WINTERLAND_TEXTURE_MINING2,
    PICTURE_WINTERLAND_TEXTURE_MINING3,
    PICTURE_WINTERLAND_TEXTURE_MINING4,
    PICTURE_WINTERLAND_TEXTURE_STEPPE_MEADOW1,
    PICTURE_WINTERLAND_TEXTURE_MEADOW1,
    PICTURE_WINTERLAND_TEXTURE_MEADOW2,
    PICTURE_WINTERLAND_TEXTURE_MEADOW3,
    PICTURE_WINTERLAND_TEXTURE_STEPPE_MEADOW2,
    PICTURE_WINTERLAND_TEXTURE_MINING_MEADOW,
    PICTURE_WINTERLAND_TEXTURE_WATER,
    PICTURE_WINTERLAND_TEXTURE_LAVA,
    PICTURE_WINTERLAND_TEXTURE_MEADOW_MIXED,
    PICTURE_TREE_CYPRESS,
    PICTURE_TREE_PINE,
    PICTURE_TREE_PALM2,
    PICTURE_TREE_PINEAPPLE,
    PICTURE_TREE_FIR,
    PICTURE_TREE_OAK,
    PICTURE_TREE_BIRCH,
    PICTURE_TREE_CHERRY,
    PICTURE_TREE_PALM1,
    PICTURE_TREE_FLAPHAT,
    PICTURE_TREE_SPIDER,
    PICTURE_TREE_WOOD_MIXED,
    PICTURE_TREE_PALM_MIXED,
    PICTURE_SQUARE_CIRCLE1,
    PICTURE_SQUARE_CIRCLE2,
    PICTURE_SQUARE_CIRCLE3,
    PICTURE_SQUARE_CIRCLE4,
    PICTURE_RESOURCE_GOLD,
    PICTURE_RESOURCE_ORE,
    PICTURE_RESOURCE_COAL,
    PICTURE_RESOURCE_GRANITE,
    // some bobtype 14 pictures missing here
    MENUBAR_TREE = 538 + 2069,
    MENUBAR_RESOURCE,
    MENUBAR_TEXTURE,
    MENUBAR_HEIGHT,
    MENUBAR_PLAYER,
    MENUBAR_LANDSCAPE,
    MENUBAR_ANIMAL,
    MENUBAR_NEWWORLD,
    PICTURE_EYE_CROSS,
    MENUBAR_COMPUTER_DOUBLE_ENTRY,
    // some bobtype 14 pictures missing here
    PICTURE_LANDSCAPE_GRANITE = MENUBAR_COMPUTER_DOUBLE_ENTRY + 4,
    PICTURE_LANDSCAPE_TREE_DEAD,
    PICTURE_LANDSCAPE_STONE,
    PICTURE_LANDSCAPE_CACTUS,
    PICTURE_LANDSCAPE_PEBBLE,
    PICTURE_LANDSCAPE_BUSH,
    PICTURE_LANDSCAPE_SHRUB,
    PICTURE_LANDSCAPE_BONE,
    PICTURE_LANDSCAPE_MUSHROOM,
    PICTURE_LANDSCAPE_STALAGMITE,
    PICTURE_LANDSCAPE_GRANITE_WINTER,
    PICTURE_LANDSCAPE_TREE_DEAD_WINTER,
    PICTURE_LANDSCAPE_STONE_WINTER,
    PICTURE_LANDSCAPE_PEBBLE_WINTER,
    PICTURE_LANDSCAPE_BONE_WINTER,
    PICTURE_LANDSCAPE_MUSHROOM_WINTER,
    PICTURE_ANIMAL_BEAR,
    PICTURE_ANIMAL_RABBIT,
    PICTURE_ANIMAL_FOX,
    PICTURE_ANIMAL_STAG,
    PICTURE_ANIMAL_ROE,
    PICTURE_ANIMAL_DUCK,
    PICTURE_ANIMAL_SHEEP,
    // END: /DATA/IO/EDITIO.IDX (AND /DATA/IO/EDITIO.DAT)

    // BEGIN: /DATA/EDITBOB.LST
    CURSOR_SYMBOL_SCISSORS,
    CURSOR_SYMBOL_TREE,
    CURSOR_SYMBOL_ARROW_UP,
    CURSOR_SYMBOL_ARROW_DOWN,
    CURSOR_SYMBOL_TEXTURE,
    CURSOR_SYMBOL_LANDSCAPE,
    CURSOR_SYMBOL_FLAG,
    CURSOR_SYMBOL_PICKAXE_MINUS,
    CURSOR_SYMBOL_PICKAXE_PLUS,
    CURSOR_SYMBOL_ANIMAL,
    FLAG_BLUE_DARK,
    FLAG_YELLOW,
    FLAG_RED,
    FLAG_BLUE_BRIGHT,
    FLAG_GREEN_DARK,
    FLAG_GREEN_BRIGHT,
    FLAG_ORANGE,
    PICTURE_SMALL_BEAR,
    PICTURE_SMALL_RABBIT,
    PICTURE_SMALL_FOX,
    PICTURE_SMALL_STAG,
    PICTURE_SMALL_DEER,
    PICTURE_SMALL_DUCK,
    PICTURE_SMALL_SHEEP,
    // END: /DATA/EDITBOB.LST

    // BEGIN: /GFX/TEXTURES/TEX5.LBM
    TILESET_GREENLAND_8BPP,
    TILESET_GREENLAND_32BPP,
    // END: /GFX/TEXTURES/TEX5.LBM

    // BEGIN: /GFX/TEXTURES/TEX6.LBM
    TILESET_WASTELAND_8BPP,
    TILESET_WASTELAND_32BPP,
    // END: /GFX/TEXTURES/TEX6.LBM

    // BEGIN: /GFX/TEXTURES/TEX7.LBM
    TILESET_WINTERLAND_8BPP,
    TILESET_WINTERLAND_32BPP,
    // END: /GFX/TEXTURES/TEX7.LBM

    // BEGIN: /DATA/MIS*BOBS.LST   * = 0,1,2,3,4,5
    MIS0BOBS_SHIP,
    MIS0BOBS_TENT,
    MIS1BOBS_STONE1,
    MIS1BOBS_STONE2,
    MIS1BOBS_STONE3,
    MIS1BOBS_STONE4,
    MIS1BOBS_STONE5,
    MIS1BOBS_STONE6,
    MIS1BOBS_STONE7,
    MIS1BOBS_TREE1,
    MIS1BOBS_TREE2,
    MIS1BOBS_SKELETON,
    MIS2BOBS_TENT,
    MIS2BOBS_GUARDHOUSE,
    MIS2BOBS_GUARDTOWER,
    MIS2BOBS_FORTRESS,
    MIS2BOBS_PUPPY,
    MIS3BOBS_VIKING,
    MIS4BOBS_SCROLLS,
    MIS5BOBS_SKELETON1,
    MIS5BOBS_SKELETON2,
    MIS5BOBS_CAVE,
    MIS5BOBS_VIKING,
    // END: /DATA/MIS*BOBS.LST

    // BEGIN: /DATA/MAP00.LST (ONLY IF A MAP IS ACTIVE)
    MAPPIC_ARROWCROSS_YELLOW,
    MAPPIC_CIRCLE_YELLOW,
    MAPPIC_ARROWCROSS_RED,
    MAPPIC_ARROWCROSS_ORANGE,
    MAPPIC_ARROWCROSS_RED_FLAG,
    MAPPIC_ARROWCROSS_RED_MINE,
    MAPPIC_ARROWCROSS_RED_HOUSE_SMALL,
    MAPPIC_ARROWCROSS_RED_HOUSE_MIDDLE,
    MAPPIC_ARROWCROSS_RED_HOUSE_BIG,
    MAPPIC_ARROWCROSS_RED_HOUSE_HARBOUR,
    MAPPIC_PAPER_RED_CROSS,
    MAPPIC_FLAG,
    MAPPIC_HOUSE_SMALL,
    MAPPIC_HOUSE_MIDDLE,
    MAPPIC_HOUSE_BIG,
    MAPPIC_MINE,
    MAPPIC_HOUSE_HARBOUR,
    // some pictures missing here
    MAPPIC_TREE_PINE = MAPPIC_HOUSE_HARBOUR + 10,
    MAPPIC_TREE_BIRCH = MAPPIC_TREE_PINE + 15,
    MAPPIC_TREE_OAK = MAPPIC_TREE_PINE + 30,
    MAPPIC_TREE_PALM1 = MAPPIC_TREE_PINE + 45,
    MAPPIC_TREE_PALM2 = MAPPIC_TREE_PINE + 60,
    MAPPIC_TREE_PINEAPPLE = MAPPIC_TREE_PINE + 75,
    MAPPIC_TREE_CYPRESS = MAPPIC_TREE_PINE + 83,
    MAPPIC_TREE_CHERRY = MAPPIC_TREE_PINE + 98,
    MAPPIC_TREE_FIR = MAPPIC_TREE_PINE + 113,
    MAPPIC_MUSHROOM1 = MAPPIC_TREE_FIR + 15,
    MAPPIC_MUSHROOM2,
    MAPPIC_STONE1,
    MAPPIC_STONE2,
    MAPPIC_STONE3,
    MAPPIC_TREE_TRUNK_DEAD,
    MAPPIC_TREE_DEAD,
    MAPPIC_BONE1,
    MAPPIC_BONE2,
    MAPPIC_FLOWERS,
    MAPPIC_BUSH1,
    MAPPIC_ROCK4,
    MAPPIC_CACTUS1,
    MAPPIC_CACTUS2,
    MAPPIC_SHRUB1,
    MAPPIC_SHRUB2,
    MAPPIC_GRANITE_1_1,
    MAPPIC_GRANITE_1_2,
    MAPPIC_GRANITE_1_3,
    MAPPIC_GRANITE_1_4,
    MAPPIC_GRANITE_1_5,
    MAPPIC_GRANITE_1_6,
    MAPPIC_GRANITE_2_1,
    MAPPIC_GRANITE_2_2,
    MAPPIC_GRANITE_2_3,
    MAPPIC_GRANITE_2_4,
    MAPPIC_GRANITE_2_5,
    MAPPIC_GRANITE_2_6,
    MAPPIC_UNKNOWN_PICTURE,
    MAPPIC_FIELD_1_1,
    MAPPIC_FIELD_1_2,
    MAPPIC_FIELD_1_3,
    MAPPIC_FIELD_1_4,
    MAPPIC_FIELD_1_5,
    MAPPIC_FIELD_2_1,
    MAPPIC_FIELD_2_2,
    MAPPIC_FIELD_2_3,
    MAPPIC_FIELD_2_4,
    MAPPIC_FIELD_2_5,
    MAPPIC_BUSH2,
    MAPPIC_BUSH3,
    MAPPIC_BUSH4,
    MAPPIC_SHRUB3,
    MAPPIC_SHRUB4,
    MAPPIC_BONE3,
    MAPPIC_BONE4,
    MAPPIC_MUSHROOM3,
    MAPPIC_STONE4,
    MAPPIC_STONE5,
    MAPPIC_PEBBLE1,
    MAPPIC_PEBBLE2,
    MAPPIC_PEBBLE3,
    MAPPIC_SHRUB5,
    MAPPIC_SHRUB6,
    MAPPIC_SHRUB7,
    MAPPIC_SNOWMAN,
    MAPPIC_DOOR,
    // some pictures missing here
    MAPPIC_LAST_ENTRY = 1432 + 2070
    // END: /DATA/MAP00.LST
};

// enumeration for BobtypeSHADOW (shadows for the pics)
enum
{
    MIS0BOBS_SHIP_SHADOW = 0,
    MIS0BOBS_TENT_SHADOW,
    MIS1BOBS_STONE1_SHADOW,
    MIS1BOBS_STONE2_SHADOW,
    MIS1BOBS_STONE3_SHADOW,
    MIS1BOBS_STONE4_SHADOW,
    MIS1BOBS_STONE5_SHADOW,
    MIS1BOBS_STONE6_SHADOW,
    MIS1BOBS_STONE7_SHADOW,
    MIS1BOBS_TREE1_SHADOW,
    MIS1BOBS_TREE2_SHADOW,
    MIS1BOBS_SKELETON_SHADOW,
    MIS2BOBS_TENT_SHADOW,
    MIS2BOBS_GUARDHOUSE_SHADOW,
    MIS2BOBS_GUARDTOWER_SHADOW,
    MIS2BOBS_FORTRESS_SHADOW,
    MIS2BOBS_PUPPY_SHADOW,
    MIS3BOBS_VIKING_SHADOW,
    MIS4BOBS_SCROLLS_SHADOW,
    MIS5BOBS_SKELETON1_SHADOW,
    MIS5BOBS_SKELETON2_SHADOW,
    MIS5BOBS_CAVE_SHADOW,
    MIS5BOBS_VIKING_SHADOW
};

// enumeration for BobtypePAL (palettes for the pics)
enum
{
    PAL_RESOURCE = 0,
    PAL_IO,
    PAL_MAPxx,
    PAL_xBBM
};

// Button-Colors (after all used by CButton and other Objects using CButton)
enum
{
    BUTTON_GREY = BUTTON_GREY_BRIGHT,
    BUTTON_RED1 = BUTTON_RED1_BRIGHT,
    BUTTON_GREEN1 = BUTTON_GREEN1_BRIGHT,
    BUTTON_GREEN2 = BUTTON_GREEN2_BRIGHT,
    BUTTON_RED2 = BUTTON_RED2_BRIGHT,
    BUTTON_STONE = BUTTON_STONE_BRIGHT
};

// some necessary data for CWindow
// background color
enum
{
    WINDOW_NOTHING = -1,
    WINDOW_GREEN1 = WINDOW_BACKGROUND,
    WINDOW_GREEN2 = BUTTON_GREEN1_DARK,
    WINDOW_GREEN3 = BUTTON_GREEN1_BRIGHT,
    WINDOW_GREEN4 = BUTTON_GREEN1_BACKGROUND,
    WINDOW_GREEN5 = BUTTON_GREEN2_DARK,
    WINDOW_GREEN6 = BUTTON_GREEN2_BRIGHT,
    WINDOW_GREEN7 = BUTTON_GREEN2_BACKGROUND,
    WINDOW_GREY1 = BUTTON_GREY_DARK,
    WINDOW_GREY2 = BUTTON_GREY_BRIGHT,
    WINDOW_GREY3 = BUTTON_GREY_BACKGROUND,
    WINDOW_RED1 = BUTTON_RED1_DARK,
    WINDOW_RED2 = BUTTON_RED1_BRIGHT,
    WINDOW_RED3 = BUTTON_RED1_BACKGROUND,
    WINDOW_RED4 = BUTTON_RED2_DARK,
    WINDOW_RED5 = BUTTON_RED2_BRIGHT,
    WINDOW_RED6 = BUTTON_RED2_BACKGROUND,
    WINDOW_STONE1 = BUTTON_STONE_DARK,
    WINDOW_STONE2 = BUTTON_STONE_BRIGHT,
    WINDOW_STONE3 = BUTTON_STONE_BACKGROUND
};
// flags
enum
{
    WINDOW_MOVE = 1,
    WINDOW_CLOSE = 2,
    WINDOW_MINIMIZE = 4,
    WINDOW_RESIZE = 8
};

// modes for editor (what will happen if user clicks somewhere on the map in editor mode)
enum
{
    EDITOR_MODE_CUT = 0,
    EDITOR_MODE_TREE,
    EDITOR_MODE_HEIGHT_RAISE,
    EDITOR_MODE_HEIGHT_REDUCE,
    EDITOR_MODE_HEIGHT_PLANE,
    EDITOR_MODE_HEIGHT_MAKE_BIG_HOUSE,
    EDITOR_MODE_TEXTURE,
    EDITOR_MODE_TEXTURE_MAKE_HARBOUR,
    EDITOR_MODE_LANDSCAPE,
    EDITOR_MODE_FLAG,
    EDITOR_MODE_FLAG_DELETE,
    EDITOR_MODE_RESOURCE_RAISE,
    EDITOR_MODE_RESOURCE_REDUCE,
    EDITOR_MODE_ANIMAL
};

// maximum range for the cursor in editor mode (user can increase or decrease by pressing '+' or '-') --> Must be >= 0.
#define MAX_CHANGE_SECTION 10

// maximum values for global arrays
// maximum pics
#define MAXBOBBMP 5000
// maximum shadows
#define MAXBOBSHADOW 5000
// maximum palettes
#define MAXBOBPAL 100

// maximum values for GUI stuff
// maximum number of menus
#define MAXMENUS 20
// maximum number of windows
#define MAXWINDOWS 100
// maximum number of callbacks
#define MAXCALLBACKS 100
// maximum number of buttons that can be created WITHIN a menu or window
#define MAXBUTTONS 50
// maximum number of texts that can be written WITHIN a menu or window
#define MAXTEXTS 100
// maximum number of pictures that can be shown WITHIN a menu or window
#define MAXPICTURES 100
// maximum number of textfields that can be created WITHIN a menu or window
#define MAXTEXTFIELDS 20
// maximum number of selectboxes that can be created WITHIN a menu or window
#define MAXSELECTBOXES 20

// maximum players for a map
#define MAXPLAYERS 16
// maximum map size
#define MAXMAPWIDTH 1024
#define MAXMAPHEIGHT 1024

// triangle values
// these values are now handled in globals.h and globals.cpp, cause they must be changeable for the zoom mode
//#define TRIANGLE_HEIGHT             28  //30 --> old value, 28 is the right
//#define TRIANGLE_WIDTH              56  //54 --> old value, 56 is the right
//#define TRIANGLE_INCREASE            5  //depends on TRIANGLE_HEIGHT --> TRIANGLE_HEIGHT/TRIANGLE_INCREASE must be greater than 5

enum TriangleTerrainType
{
    TRIANGLE_TEXTURE_STEPPE_MEADOW1 = 0x00,
    TRIANGLE_TEXTURE_STEPPE_MEADOW1_HARBOUR = 0x40,
    TRIANGLE_TEXTURE_MINING1 = 0x01,
    TRIANGLE_TEXTURE_SNOW = 0x02,
    TRIANGLE_TEXTURE_SWAMP = 0x03,
    TRIANGLE_TEXTURE_STEPPE = 0x04,
    TRIANGLE_TEXTURE_WATER = 0x05,
    TRIANGLE_TEXTURE_WATER_ = 0x06,
    TRIANGLE_TEXTURE_STEPPE_ = 0x07,
    TRIANGLE_TEXTURE_MEADOW1 = 0x08,
    TRIANGLE_TEXTURE_MEADOW1_HARBOUR = 0x48,
    TRIANGLE_TEXTURE_MEADOW2 = 0x09,
    TRIANGLE_TEXTURE_MEADOW2_HARBOUR = 0x49,
    TRIANGLE_TEXTURE_MEADOW3 = 0x0A,
    TRIANGLE_TEXTURE_MEADOW3_HARBOUR = 0x4A,
    TRIANGLE_TEXTURE_MINING2 = 0x0B,
    TRIANGLE_TEXTURE_MINING3 = 0x0C,
    TRIANGLE_TEXTURE_MINING4 = 0x0D,
    TRIANGLE_TEXTURE_STEPPE_MEADOW2 = 0x0E,
    TRIANGLE_TEXTURE_STEPPE_MEADOW2_HARBOUR = 0x4E,
    TRIANGLE_TEXTURE_FLOWER = 0x0F,
    TRIANGLE_TEXTURE_FLOWER_HARBOUR = 0x4F,
    TRIANGLE_TEXTURE_LAVA = 0x10,
    TRIANGLE_TEXTURE_COLOR = 0x11,
    TRIANGLE_TEXTURE_MINING_MEADOW = 0x12,
    TRIANGLE_TEXTURE_MINING_MEADOW_HARBOUR = 0x52,
    TRIANGLE_TEXTURE_WATER__ = 0x13,
    TRIANGLE_TEXTURE_STEPPE__ = 0x80,
    TRIANGLE_TEXTURE_STEPPE___ = 0x84,
    TRIANGLE_TEXTURE_MEADOW_MIXED = 0xBF, // this will not be written to map-files, it is only a indicator for mixed meadow in editor mode
    TRIANGLE_TEXTURE_MEADOW_MIXED_HARBOUR =
      0xFF, // this will not be written to map-files, it is only a indicator for mixed meadow in editor mode
};

#endif
