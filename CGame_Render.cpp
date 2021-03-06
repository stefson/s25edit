#include "CGame.h"
#include "CIO/CMenu.h"
#include "CIO/CWindow.h"
#include "CMap.h"
#include "CSurface.h"
#include "SGE/sge_blib.h"
#include "globals.h"
#ifdef _WIN32
#include "s25editResource.h"
#undef WIN32_LEAN_AND_MEAN
#include <SDL_syswm.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

void CGame::SetAppIcon()
{
#ifdef _WIN32
    LPARAM icon = (LPARAM)LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SYMBOL));
    SendMessage(GetConsoleWindow(), WM_SETICON, ICON_BIG, icon);
    SendMessage(GetConsoleWindow(), WM_SETICON, ICON_SMALL, icon);

    SDL_SysWMinfo info;
    // get window handle from SDL
    SDL_VERSION(&info.version);
    if(SDL_GetWMInfo(&info) != 1)
        return;
    SendMessage(info.window, WM_SETICON, ICON_BIG, icon);
    SendMessage(info.window, WM_SETICON, ICON_SMALL, icon);
#endif // _WIN32
}

void CGame::Render()
{
    if(Extent(Surf_Display->w, Surf_Display->h) != GameResolution || fullscreen != ((Surf_Display->flags & SDL_FULLSCREEN) != 0))
    {
        ReCreateWindow();
    }

    // if the S2 loading screen is shown, render only this until user clicks a mouse button
    if(showLoadScreen)
    {
        // CSurface::Draw(Surf_Display, global::bmpArray[SPLASHSCREEN_LOADING_S2SCREEN].surface, 0, 0);
        SDL_Surface* surfLoadScreen = global::bmpArray[SPLASHSCREEN_LOADING_S2SCREEN].surface;
        sge_TexturedRect(Surf_Display, 0, 0, Surf_Display->w - 1, 0, 0, Surf_Display->h - 1, Surf_Display->w - 1, Surf_Display->h - 1,
                         surfLoadScreen, 0, 0, surfLoadScreen->w - 1, 0, 0, surfLoadScreen->h - 1, surfLoadScreen->w - 1,
                         surfLoadScreen->h - 1);

        if(CSurface::useOpenGL)
            SDL_GL_SwapBuffers();
        else
            SDL_Flip(Surf_Display);
        return;
    }

    // render the map if active
    if(MapObj && MapObj->isActive())
        CSurface::Draw(Surf_Display, MapObj->getSurface(), 0, 0);

    // render active menus
    for(auto& Menu : Menus)
    {
        if(Menu && Menu->isActive())
            CSurface::Draw(Surf_Display, Menu->getSurface(), 0, 0);
    }

    // render windows ordered by priority
    int highestPriority = 0;
    // first find the highest priority
    for(auto& Window : Windows)
    {
        if(Window && Window->getPriority() > highestPriority)
            highestPriority = Window->getPriority();
    }
    // render from lowest priority to highest
    for(int actualPriority = 0; actualPriority <= highestPriority; actualPriority++)
    {
        for(auto& Window : Windows)
        {
            if(Window && Window->getPriority() == actualPriority)
                CSurface::Draw(Surf_Display, Window->getSurface(), Window->getX(), Window->getY());
        }
    }

    // render mouse cursor
    if(Cursor.clicked)
    {
        if(Cursor.button.right)
            CSurface::Draw(Surf_Display, global::bmpArray[CROSS].surface, Cursor.x, Cursor.y);
        else
            CSurface::Draw(Surf_Display, global::bmpArray[CURSOR_CLICKED].surface, Cursor.x, Cursor.y);
    } else
        CSurface::Draw(Surf_Display, global::bmpArray[CURSOR].surface, Cursor.x, Cursor.y);

#ifdef _ADMINMODE
    FrameCounter++;
#endif

    if(CSurface::useOpenGL)
    {
        SDL_BlitSurface(Surf_Display, nullptr, Surf_DisplayGL, nullptr);
        SDL_Flip(Surf_DisplayGL);
        SDL_GL_SwapBuffers();
    } else
        SDL_Flip(Surf_Display);

    SDL_Delay(msWait);
}
