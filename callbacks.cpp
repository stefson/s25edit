//NOTE: negative callbackParams are reserved: -1 = callback is called first time, -2 = used by gameloop for registered
//callbacks (callbacks that will additionally execute WITHIN the gameloop)

#include "callbacks.h"

void callback::mainmenu(int Param)
{
    static CMenu *MainMenu = NULL;
    static CMap *Map = NULL;

    enum
    {
        ENDGAME = 1,
        LOADMAP
    };

    switch (Param)
    {
        case INITIALIZING_CALL:
                    Map = NULL;
                    MainMenu = new CMenu(SPLASHSCREEN_MAINMENU);
                    if (!global::s2->RegisterMenu(MainMenu))
                    {
                        delete MainMenu;
                        MainMenu = NULL;
                        return;
                    }
                    MainMenu->addButton(callback::mainmenu, ENDGAME, 50, 400, 200, 20, BUTTON_RED1, "Programm Verlassen");
                    #ifdef _ADMINMODE
                        MainMenu->addButton(callback::submenu1, INITIALIZING_CALL, 50, 200, 200, 20, BUTTON_GREY, "Submenu_1");
                        MainMenu->addButton(callback::mainmenu, LOADMAP, 50, 160, 200, 20, BUTTON_STONE, "Testmap laden");
                    #endif
                    MainMenu->addButton(callback::submenuOptions, INITIALIZING_CALL, 50, 370, 200, 20, BUTTON_GREEN2, "Optionen");
                    break;

        case CALL_FROM_GAMELOOP:
                    break;

        case ENDGAME:
                    MainMenu->setWaste();
                    MainMenu = NULL;
                    global::s2->Running = false;
                    break;

        case LOADMAP:
                    //Map = new CMap("BERG_.SWD");
                    Map = new CMap("MISS208.WLD");
                    //Map = new CMap(NULL);
                    global::s2->setMap(Map);
                    MainMenu->setWaste();
                    MainMenu = NULL;
                    break;

        default:    break;
    }
}

void callback::submenuOptions(int Param)
{
    static CMenu *SubMenu = NULL;
    static CFont *TextResolution = NULL;
    static CButton *ButtonFullscreen = NULL;
    char puffer[80];
    char puffer2[80];
    const SDL_VideoInfo* videoinfo = SDL_GetVideoInfo();

    enum
    {
        MAINMENU = 1,
        RES_640,
        RES_800,
        RES_1024,
        RES_1280,
        RES_1440,
        FULLSCREEN
    };

    switch (Param)
    {
        case INITIALIZING_CALL:
                SubMenu = new CMenu(SPLASHSCREEN_SUBMENU3);
                if (!global::s2->RegisterMenu(SubMenu))
                {
                    delete SubMenu;
                    SubMenu = NULL;
                    return;
                }
                //add button for "back to main menu"
                SubMenu->addButton(callback::submenuOptions, MAINMENU, (int)(global::s2->MenuResolutionX/2-100), 440, 200, 20, BUTTON_RED1, "zur�ck");
                //add menu title
                SubMenu->addText("Optionen", (int)(global::s2->MenuResolutionX/2-20), 10, 14);
                //add screen resolution
                if (TextResolution != NULL)
                    SubMenu->delText(TextResolution);
                sprintf(puffer, "Game Resolution: %d*%d / %s", global::s2->GameResolutionX, global::s2->GameResolutionY, (global::s2->fullscreen ? "Fullscreen" : "Window"));
                TextResolution = SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-110), 50, 11);
                //add buttons for resolutions
                SubMenu->addButton(callback::submenuOptions, RES_640, (int)(global::s2->MenuResolutionX/2-100), 70, 200, 20, BUTTON_RED1, "640*480");
                SubMenu->addButton(callback::submenuOptions, RES_800, (int)(global::s2->MenuResolutionX/2-100), 90, 200, 20, BUTTON_RED1, "800*600");
                SubMenu->addButton(callback::submenuOptions, RES_1024, (int)(global::s2->MenuResolutionX/2-100), 110, 200, 20, BUTTON_RED1, "1024*768");
                SubMenu->addButton(callback::submenuOptions, RES_1280, (int)(global::s2->MenuResolutionX/2-100), 130, 200, 20, BUTTON_RED1, "1280*1024");
                SubMenu->addButton(callback::submenuOptions, RES_1440, (int)(global::s2->MenuResolutionX/2-100), 150, 200, 20, BUTTON_RED1, "1440*900");
                if (ButtonFullscreen != NULL)
                    SubMenu->delButton(ButtonFullscreen);
                ButtonFullscreen = SubMenu->addButton(callback::submenuOptions, FULLSCREEN, (int)(global::s2->MenuResolutionX/2-100), 170, 200, 20, BUTTON_RED1, (global::s2->fullscreen ? "WINDOW" : "FULLSCREEN"));
                //add video driver name
                SDL_VideoDriverName(puffer, 80);
                sprintf(puffer2, "Video-Treiber: %s", puffer);
                SubMenu->addText(puffer2, (int)(global::s2->MenuResolutionX/2-70), 195, 11);
                //add video memory
                sprintf(puffer, "Grafik-Speicher: %d MB", videoinfo->video_mem);
                SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-70), 210, 11);
                break;

        case MAINMENU:  SubMenu->setWaste();
                        SubMenu = NULL;
                        break;
        case RES_640:   global::s2->GameResolutionX = 640;
                        global::s2->GameResolutionY = 480;
                        if (TextResolution != NULL)
                            SubMenu->delText(TextResolution);
                        sprintf(puffer, "Game Resolution: %d*%d / %s", global::s2->GameResolutionX, global::s2->GameResolutionY, (global::s2->fullscreen ? "Fullscreen" : "Window"));
                        TextResolution = SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-110), 50, 11);
                        break;
        case RES_800:   global::s2->GameResolutionX = 800;
                        global::s2->GameResolutionY = 600;
                        if (TextResolution != NULL)
                            SubMenu->delText(TextResolution);
                        sprintf(puffer, "Game Resolution: %d*%d / %s", global::s2->GameResolutionX, global::s2->GameResolutionY, (global::s2->fullscreen ? "Fullscreen" : "Window"));
                        TextResolution = SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-110), 50, 11);
                        break;
        case RES_1024:  global::s2->GameResolutionX = 1024;
                        global::s2->GameResolutionY = 768;
                        if (TextResolution != NULL)
                            SubMenu->delText(TextResolution);
                        sprintf(puffer, "Game Resolution: %d*%d / %s", global::s2->GameResolutionX, global::s2->GameResolutionY, (global::s2->fullscreen ? "Fullscreen" : "Window"));
                        TextResolution = SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-110), 50, 11);
                        break;
        case RES_1280:  global::s2->GameResolutionX = 1280;
                        global::s2->GameResolutionY = 1024;
                        if (TextResolution != NULL)
                            SubMenu->delText(TextResolution);
                        sprintf(puffer, "Game Resolution: %d*%d / %s", global::s2->GameResolutionX, global::s2->GameResolutionY, (global::s2->fullscreen ? "Fullscreen" : "Window"));
                        TextResolution = SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-110), 50, 11);
                        break;
        case RES_1440:  global::s2->GameResolutionX = 1440;
                        global::s2->GameResolutionY = 900;
                        if (TextResolution != NULL)
                            SubMenu->delText(TextResolution);
                        sprintf(puffer, "Game Resolution: %d*%d / %s", global::s2->GameResolutionX, global::s2->GameResolutionY, (global::s2->fullscreen ? "Fullscreen" : "Window"));
                        TextResolution = SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-110), 50, 11);
                        break;
        case FULLSCREEN:if (global::s2->fullscreen)
                            global::s2->fullscreen = false;
                        else
                            global::s2->fullscreen = true;
                        if (TextResolution != NULL)
                            SubMenu->delText(TextResolution);
                        sprintf(puffer, "Game Resolution: %d*%d / %s", global::s2->GameResolutionX, global::s2->GameResolutionY, (global::s2->fullscreen ? "Fullscreen" : "Window"));
                        TextResolution = SubMenu->addText(puffer, (int)(global::s2->MenuResolutionX/2-110), 50, 11);
                        if (ButtonFullscreen != NULL)
                            SubMenu->delButton(ButtonFullscreen);
                        ButtonFullscreen = SubMenu->addButton(callback::submenuOptions, FULLSCREEN, (int)(global::s2->MenuResolutionX/2-100), 170, 200, 20, BUTTON_RED1, (global::s2->fullscreen ? "WINDOW" : "FULLSCREEN"));
                        break;

        default:    break;
    }
}

//now the 4 game callbacks from the menubar will follow

//first
void callback::GameMenu(int Param)
{
    static CWindow *WNDBackToMainMenu = NULL;

    enum
    {
        BACKTOMAIN = 1,
        WINDOWQUIT
    };

    switch (Param)
    {
        case INITIALIZING_CALL:
                    if (WNDBackToMainMenu != NULL)
                        break;
                    WNDBackToMainMenu = new CWindow(GameMenu, WINDOWQUIT, global::s2->GameResolutionX/2-125, global::s2->GameResolutionY/2-60, 250, 140, "Zurueck zum Hauptmenu?", WINDOW_GREEN1, WINDOW_CLOSE);
                    if (global::s2->RegisterWindow(WNDBackToMainMenu))
                    {
                        WNDBackToMainMenu->addButton(GameMenu, BACKTOMAIN, 20, 20, 200, 80, BUTTON_RED2, "Ja");
                    }
                    else
                    {
                        delete WNDBackToMainMenu;
                        WNDBackToMainMenu = NULL;
                        return;
                    }
                    break;

        case BACKTOMAIN:
                    if (global::s2->getMap() != NULL)
                        global::s2->delMap();
                    WNDBackToMainMenu->setWaste();
                    WNDBackToMainMenu = NULL;
                    mainmenu(INITIALIZING_CALL);
                    break;

        case WINDOWQUIT:
                    if (WNDBackToMainMenu != NULL)
                    {
                        WNDBackToMainMenu->setWaste();
                        WNDBackToMainMenu = NULL;
                    }
                    break;

        default:    break;
    }
}

//second

//third

//fourth


//now the editor callbacks from the menubar will follow (editor mode)

void callback::EditorMainMenu(int Param)
{
    ;
}

void callback::EditorQuitMenu(int Param)
{
    static CWindow *WNDBackToMainMenu = NULL;

    enum
    {
        BACKTOMAIN = 1,
        NOTBACKTOMAIN,
        WINDOWQUIT
    };

    switch (Param)
    {
        case INITIALIZING_CALL:
                    if (WNDBackToMainMenu != NULL)
                        break;
                    WNDBackToMainMenu = new CWindow(EditorQuitMenu, WINDOWQUIT, global::s2->GameResolutionX/2-106, global::s2->GameResolutionY/2-55, 212, 110, "Beenden?");
                    if (global::s2->RegisterWindow(WNDBackToMainMenu))
                    {
                        WNDBackToMainMenu->addButton(EditorQuitMenu, BACKTOMAIN, 0, 0, 100, 80, BUTTON_GREEN2, NULL, PICTURE_SMALL_TICK);
                        WNDBackToMainMenu->addButton(EditorQuitMenu, NOTBACKTOMAIN, 100, 0, 100, 80, BUTTON_RED1, NULL, PICTURE_SMALL_CROSS);
                    }
                    else
                    {
                        delete WNDBackToMainMenu;
                        WNDBackToMainMenu = NULL;
                        return;
                    }
                    break;

        case BACKTOMAIN:
                    if (global::s2->getMap() != NULL)
                        global::s2->delMap();
                    WNDBackToMainMenu->setWaste();
                    WNDBackToMainMenu = NULL;
                    mainmenu(INITIALIZING_CALL);
                    break;

        case NOTBACKTOMAIN:
                    if (WNDBackToMainMenu != NULL)
                    {
                        WNDBackToMainMenu->setWaste();
                        WNDBackToMainMenu = NULL;
                    }
                    break;

        default:    break;
    }
}

void callback::EditorTextureMenu(int Param)
{
    static CWindow *WNDTexture = NULL;
    static CPicture *picSNOW = NULL;
    static CPicture *picSTEPPE = NULL;
    static CPicture *picSWAMP = NULL;
    static CPicture *picFLOWER = NULL;
    static CPicture *picMINING1 = NULL;
    static CPicture *picMINING2 = NULL;
    static CPicture *picMINING3 = NULL;
    static CPicture *picMINING4 = NULL;
    static CPicture *picSTEPPE_MEADOW1 = NULL;
    static CPicture *picMEADOW1 = NULL;
    static CPicture *picMEADOW2 = NULL;
    static CPicture *picMEADOW3 = NULL;
    static CPicture *picSTEPPE_MEADOW2 = NULL;
    static CPicture *picMINING_MEADOW = NULL;
    static CPicture *picWATER = NULL;
    static CPicture *picLAVA = NULL;
    static CPicture *picMEADOW_MIXED = NULL;
    static CMap* MapObj = NULL;
    static bobMAP *map = NULL;
    static int textureIndex = 0;

    enum
    {
        WINDOWQUIT,
        HARBOUR,
        PICSNOW,
        PICSTEPPE,
        PICSWAMP,
        PICFLOWER,
        PICMINING1,
        PICMINING2,
        PICMINING3,
        PICMINING4,
        PICSTEPPE_MEADOW1,
        PICMEADOW1,
        PICMEADOW2,
        PICMEADOW3,
        PICSTEPPE_MEADOW2,
        PICMINING_MEADOW,
        PICWATER,
        PICLAVA,
        PICMEADOW_MIXED
    };

    switch (Param)
    {
        case INITIALIZING_CALL:
                    if (WNDTexture != NULL)
                        break;
                    WNDTexture = new CWindow(EditorTextureMenu, WINDOWQUIT, 0, 0, 220, 135, "Terrain", WINDOW_GREEN1, WINDOW_CLOSE | WINDOW_MINIMIZE | WINDOW_MOVE);
                    if (global::s2->RegisterWindow(WNDTexture))
                    {
                        MapObj = global::s2->getMap();
                        map = MapObj->getMap();
                        switch (map->type)
                        {
                            case MAP_GREENLAND: textureIndex = PICTURE_GREENLAND_TEXTURE_SNOW;
                                                break;
                            case MAP_WASTELAND: textureIndex = PICTURE_WASTELAND_TEXTURE_SNOW;
                                                break;
                            case MAP_WINTERLAND: textureIndex = PICTURE_WINTERLAND_TEXTURE_SNOW;
                                                break;
                            default:            textureIndex = PICTURE_GREENLAND_TEXTURE_SNOW;
                                                break;
                        }
                        MapObj->setModeContent(TRIANGLE_TEXTURE_SNOW);

                        //WNDTexture->addButton(EditorTextureMenu, HARBOUR, 0, 0, 100, 80, BUTTON_GREEN2, NULL, PICTURE_SMALL_TICK);

                        picSNOW = WNDTexture->addPicture(EditorTextureMenu, PICSNOW, 2, 2, textureIndex++);
                        picSTEPPE = WNDTexture->addPicture(EditorTextureMenu, PICSTEPPE, 36, 2, textureIndex++);
                        picSWAMP = WNDTexture->addPicture(EditorTextureMenu, PICSWAMP, 70, 2, textureIndex++);
                        picFLOWER = WNDTexture->addPicture(EditorTextureMenu, PICFLOWER, 104, 2, textureIndex++);
                        picMINING1 = WNDTexture->addPicture(EditorTextureMenu, PICMINING1, 138, 2, textureIndex++);
                        picMINING2 = WNDTexture->addPicture(EditorTextureMenu, PICMINING2, 172, 2, textureIndex++);
                        picMINING3 = WNDTexture->addPicture(EditorTextureMenu, PICMINING3, 2, 36, textureIndex++);
                        picMINING4 = WNDTexture->addPicture(EditorTextureMenu, PICMINING4, 36, 36, textureIndex++);
                        picSTEPPE_MEADOW1 = WNDTexture->addPicture(EditorTextureMenu, PICSTEPPE_MEADOW1, 70, 36, textureIndex++);
                        picMEADOW1 = WNDTexture->addPicture(EditorTextureMenu, PICMEADOW1, 104, 36, textureIndex++);
                        picMEADOW2 = WNDTexture->addPicture(EditorTextureMenu, PICMEADOW2, 138, 36, textureIndex++);
                        picMEADOW3 = WNDTexture->addPicture(EditorTextureMenu, PICMEADOW3, 172, 36, textureIndex++);
                        picSTEPPE_MEADOW2 = WNDTexture->addPicture(EditorTextureMenu, PICSTEPPE_MEADOW2, 2, 70, textureIndex++);
                        picMINING_MEADOW = WNDTexture->addPicture(EditorTextureMenu, PICMINING_MEADOW, 36, 70, textureIndex++);
                        picWATER = WNDTexture->addPicture(EditorTextureMenu, PICWATER, 70, 70, textureIndex++);
                        picLAVA = WNDTexture->addPicture(EditorTextureMenu, PICLAVA, 104, 70, textureIndex++);
                        //picMEADOW_MIXED = WNDTexture->addPicture(EditorTextureMenu, PICMEADOW_MIXED, 138, 70, textureIndex++);
                    }
                    else
                    {
                        delete WNDTexture;
                        WNDTexture = NULL;
                        return;
                    }
                    break;

        case HARBOUR:
                    break;

        case PICSNOW:           MapObj->setModeContent(TRIANGLE_TEXTURE_SNOW);
                                break;
        case PICSTEPPE:         MapObj->setModeContent(TRIANGLE_TEXTURE_STEPPE);
                                break;
        case PICSWAMP:          MapObj->setModeContent(TRIANGLE_TEXTURE_SWAMP);
                                break;
        case PICFLOWER:         MapObj->setModeContent(TRIANGLE_TEXTURE_FLOWER);
                                break;
        case PICMINING1:        MapObj->setModeContent(TRIANGLE_TEXTURE_MINING1);
                                break;
        case PICMINING2:        MapObj->setModeContent(TRIANGLE_TEXTURE_MINING2);
                                break;
        case PICMINING3:        MapObj->setModeContent(TRIANGLE_TEXTURE_MINING3);
                                break;
        case PICMINING4:        MapObj->setModeContent(TRIANGLE_TEXTURE_MINING4);
                                break;
        case PICSTEPPE_MEADOW1: MapObj->setModeContent(TRIANGLE_TEXTURE_STEPPE_MEADOW1);
                                break;
        case PICMEADOW1:        MapObj->setModeContent(TRIANGLE_TEXTURE_MEADOW1);
                                break;
        case PICMEADOW2:        MapObj->setModeContent(TRIANGLE_TEXTURE_MEADOW2);
                                break;
        case PICMEADOW3:        MapObj->setModeContent(TRIANGLE_TEXTURE_MEADOW3);
                                break;
        case PICSTEPPE_MEADOW2: MapObj->setModeContent(TRIANGLE_TEXTURE_STEPPE_MEADOW2);
                                break;
        case PICMINING_MEADOW:  MapObj->setModeContent(TRIANGLE_TEXTURE_MINING_MEADOW);
                                break;
        case PICWATER:          MapObj->setModeContent(TRIANGLE_TEXTURE_WATER);
                                break;
        case PICLAVA:           MapObj->setModeContent(TRIANGLE_TEXTURE_LAVA);
                                break;
        //case PICMEADOW_MIXED: MapObj->setModeContent(TRIANGLE_TEXTURE_MEADOW_MIXED);
        //                        break;

        case WINDOWQUIT:
                    if (WNDTexture != NULL)
                    {
                        WNDTexture->setWaste();
                        WNDTexture = NULL;
                    }
                    MapObj->setMode(EDITOR_MODE_RAISE);
                    picSNOW = NULL;
                    picSTEPPE = NULL;
                    picSWAMP = NULL;
                    picFLOWER = NULL;
                    picMINING1 = NULL;
                    picMINING2 = NULL;
                    picMINING3 = NULL;
                    picMINING4 = NULL;
                    picSTEPPE_MEADOW1 = NULL;
                    picMEADOW1 = NULL;
                    picMEADOW2 = NULL;
                    picMEADOW3 = NULL;
                    picSTEPPE_MEADOW2 = NULL;
                    picMINING_MEADOW = NULL;
                    picWATER = NULL;
                    picLAVA = NULL;
                    picMEADOW_MIXED = NULL;
                    MapObj = NULL;
                    map = NULL;
                    textureIndex = 0;
                    break;

        default:    break;
    }
}

#ifdef _ADMINMODE
//the debugger is an object and a friend class of all other classes
//debugger-function only will construct a new debugger and if debugger-function gets a window-quit-message
//then the debugger-function will destruct the object

void callback::debugger(int Param)
{
    static CDebug* Debugger = NULL;

    switch (Param)
    {
        case INITIALIZING_CALL:
                    if (Debugger != NULL)
                        break;
                    Debugger = new CDebug(debugger, -3);    //-3 is parameter for closing the debugger window
                    break;

        case -3:    delete Debugger;
                    Debugger = NULL;
                    break;

        default:    if (Debugger != NULL)
                        Debugger->sendParam(Param);
                    break;
    }
}
//this is a submenu for testing
void callback::submenu1(int Param)
{
    static CMenu *SubMenu = NULL;
    static CButton *toMain = NULL;
    static CButton *greatMoon = NULL;
    static CButton *smallMoon = NULL;
    static CButton *toosmall = NULL;
    static CButton *createWindow = NULL;
    static CFont *ueberschrift = NULL;
    static CFont *resolution = NULL;
    static CFont *greatMoonText = NULL;
    static CFont *counterText = NULL;
    static CPicture *picObject = NULL;
    static int counter = 0;
    static CWindow *testWindow = NULL;
    static CWindow *testWindow2 = NULL;
    static CPicture *testWindowPicture = NULL;
    static CFont *testWindowText = NULL;
    static CFont *testWindowText2 = NULL;

    static int picIndex = -1;
    char puffer[80];

    //if this is the first time the function is called
    if (Param == INITIALIZING_CALL)
        global::s2->RegisterCallback(submenu1);

    enum
    {
        MAINMENU = 1,
        GREATMOON,
        SMALLMOON,
        TOOSMALL,
        CREATEWINDOW,
        GREATMOONENTRY,
        GREATMOONLEAVE,
        PICOBJECT,
        PICOBJECTENTRY,
        PICOBJECTLEAVE,
        TESTWINDOWPICTURE,
        TESTWINDOWPICTUREENTRY,
        TESTWINDOWPICTURELEAVE,
        TESTWINDOWQUITMESSAGE,
        TESTWINDOW2QUITMESSAGE
    };

    switch (Param)
    {
        case INITIALIZING_CALL:
                SubMenu = new CMenu(SPLASHSCREEN_SUBMENU1);
                if (!global::s2->RegisterMenu(SubMenu))
                {
                    delete SubMenu;
                    SubMenu = NULL;
                    return;
                }
                toMain = SubMenu->addButton(submenu1, MAINMENU, 400, 440, 200, 20, BUTTON_RED1, "zur�ck");
                greatMoon = SubMenu->addButton(submenu1, GREATMOON, 100, 100, 200, 200, BUTTON_STONE, NULL, MOON);
                greatMoon->setMotionParams(GREATMOONENTRY, GREATMOONLEAVE);
                smallMoon = SubMenu->addButton(submenu1, SMALLMOON, 100, 350, global::bmpArray[MOON].w, global::bmpArray[MOON].h, BUTTON_STONE, NULL, MOON);
                toosmall = SubMenu->addButton(submenu1, TOOSMALL, 100, 400, global::bmpArray[MOON].w-1, global::bmpArray[MOON].h-1, BUTTON_STONE, NULL, MOON);
                createWindow = SubMenu->addButton(submenu1, CREATEWINDOW, 500, 10, 130, 30, BUTTON_GREEN1, "Fenster erzeugen");
                picObject = SubMenu->addPicture(submenu1, PICOBJECT, 200, 30, MIS0BOBS_SHIP);
                picObject->setMotionParams(PICOBJECTENTRY, PICOBJECTLEAVE);
                break;

        case MAINMENU:  SubMenu->setWaste();
                        SubMenu = NULL;
                        toMain = NULL;
                        greatMoon = NULL;
                        smallMoon = NULL;
                        toosmall = NULL;
                        createWindow = NULL;
                        ueberschrift = NULL;
                        resolution = NULL;
                        greatMoonText = NULL;
                        counterText = NULL;
                        testWindowPicture = NULL;
                        testWindowText = NULL;
                        testWindowText2 = NULL;
                        global::s2->UnregisterCallback(submenu1);
                        if (testWindow != NULL)
                        {
                            testWindow->setWaste();
                            testWindow = NULL;
                        }
                        if (testWindow2 != NULL)
                        {
                            testWindow2->setWaste();
                            testWindow2 = NULL;
                        }
                        picIndex = -1;
                        break;

        case GREATMOON: ueberschrift = SubMenu->addText("�berschrift!", 300, 10, 14);
                        sprintf(puffer, "Fenster X: %d Fenster Y: %d", global::s2->MenuResolutionX, global::s2->MenuResolutionY);
                        resolution = SubMenu->addText(puffer, 10, 10, 14);
                        break;

        case SMALLMOON: SubMenu->delButton(greatMoon);
                        SubMenu->delStaticPicture(picIndex);
                        picIndex = -1;
                        break;

        case TOOSMALL:  if (picIndex == -1)
                            picIndex = SubMenu->addStaticPicture(0, 0, MAINFRAME_640_480);
                        break;

        case CREATEWINDOW:  if (testWindow == NULL)
                            {
                                testWindow = new CWindow(submenu1, TESTWINDOWQUITMESSAGE, 5, 5, 350, 240, "Fenster", WINDOW_GREEN1, WINDOW_CLOSE | WINDOW_MOVE | WINDOW_MINIMIZE | WINDOW_RESIZE);
                                if (global::s2->RegisterWindow(testWindow))
                                {
                                    testWindow->addText("Text innerhalb des Fensters", 10, 10, 14);
                                    testWindow->addButton(submenu1, -10, 150, 100, 210, 30, BUTTON_GREEN2, "Button innerhalb des Fensters");
                                    testWindowPicture = testWindow->addPicture(submenu1, TESTWINDOWPICTURE, 10, 60, MIS2BOBS_FORTRESS);
                                    testWindowPicture->setMotionParams(TESTWINDOWPICTUREENTRY, TESTWINDOWPICTURELEAVE);
                                }
                                else
                                {
                                    delete testWindow;
                                    testWindow = NULL;
                                    return;
                                }
                            }
                            if (testWindow2 == NULL)
                            {
                                testWindow2 = new CWindow(submenu1, TESTWINDOW2QUITMESSAGE, 200, 5, 350, 240, "Noch ein Fenster", WINDOW_GREEN1, WINDOW_CLOSE | WINDOW_MOVE | WINDOW_MINIMIZE | WINDOW_RESIZE);
                                if (global::s2->RegisterWindow(testWindow2))
                                {
                                    testWindow2->addText("Text innerhalb des Fensters", 50, 40, 9);
                                    testWindow2->addButton(submenu1, -10, 100, 100, 100, 20, BUTTON_GREEN2, "Button");
                                }
                                else
                                {
                                    delete testWindow2;
                                    testWindow2 = NULL;
                                    return;
                                }
                            }
                            break;

        case GREATMOONENTRY:    if (greatMoonText == NULL)
                                    greatMoonText = SubMenu->addText("Test-Text����", 100, 10, 14);
                                break;

        case GREATMOONLEAVE:    if (greatMoonText != NULL)
                                {
                                    SubMenu->delText(greatMoonText);
                                    greatMoonText = NULL;
                                }
                                break;

        case PICOBJECT: if (greatMoon != NULL)
                        {
                            SubMenu->delButton(greatMoon);
                            greatMoon = NULL;
                        }
                        break;

        case PICOBJECTENTRY:    if (greatMoonText == NULL)
                                    greatMoonText = SubMenu->addText("Test-Text����", 100, 10, 14);
                                break;

        case PICOBJECTLEAVE:    if (greatMoonText != NULL)
                                {
                                    SubMenu->delText(greatMoonText);
                                    greatMoonText = NULL;
                                }
                                break;

        case TESTWINDOWPICTURE: if (testWindowText == NULL)
                                    testWindowText = testWindow->addText("Auf Festung geklickt", 10, 200, 11);
                                else
                                {
                                    testWindow->delText(testWindowText);
                                    testWindowText = NULL;
                                }
                                break;

        case TESTWINDOWPICTUREENTRY:    if (testWindowText2 != NULL)
                                        {
                                            testWindow->delText(testWindowText2);
                                            testWindowText2 = NULL;
                                        }
                                        testWindowText2 = testWindow->addText("Bildbereich betreten", 10, 220, 11);
                                        break;

        case TESTWINDOWPICTURELEAVE:    if (testWindowText2 != NULL)
                                        {
                                            testWindow->delText(testWindowText2);
                                            testWindowText2 = NULL;
                                        }
                                        testWindowText2 = testWindow->addText("Bildbereich verlassen", 10, 220, 11);
                                        break;

        case TESTWINDOWQUITMESSAGE:     testWindow->setWaste();
                                        testWindow = NULL;
                                        break;

        case TESTWINDOW2QUITMESSAGE:    testWindow2->setWaste();
                                        testWindow2 = NULL;
                                        break;

        case CALL_FROM_GAMELOOP:    if (counter % 10 == 0)
                    {
                        if (counterText != NULL)
                        {
                            SubMenu->delText(counterText);
                            counterText = NULL;
                        }
                        if (counterText == NULL)
                        {
                            sprintf(puffer, "zaehler: %d", counter);
                            counterText = SubMenu->addText(puffer, 100, 20, 9);
                        }
                    }
                    counter++;
                    break;

        default:    break;
    }
}
#endif
