/*
 *	SDL Graphics Extension
 *	Triangles of every sort
 *
 *	Started 000428
 *
 *	License: LGPL v2+ (see the file LICENSE)
 *	(c)2000-2003 Anders Lindström & Johan E. Thelin
 */

/*********************************************************************
 *  This library is free software; you can redistribute it and/or    *
 *  modify it under the terms of the GNU Library General Public      *
 *  License as published by the Free Software Foundation; either     *
 *  version 2 of the License, or (at your option) any later version. *
 *********************************************************************/

/*
 *  Written with some help from Johan E. Thelin.
 */

#include "sge_blib.h"
#include "sge_primitives.h"
#include "sge_primitives_int.h"
#include "sge_surface.h"
#include <boost/numeric/conversion/cast.hpp>
#include <array>

using boost::numeric_cast;

#define SWAP(x, y, temp) \
    (temp) = x;          \
    (x) = y;             \
    (y) = temp

/* Globals used for sge_Update/sge_Lock (defined in sge_surface) */
extern Uint8 _sge_update;
extern Uint8 _sge_lock;
extern Uint8 _sge_alpha_hack;

/* Macro to inline RGB mapping */
static constexpr Uint32 MapRGB(const SDL_PixelFormat& format, Uint8 r, Uint8 g, Uint8 b)
{
    return ((Uint32)r >> format.Rloss) << format.Rshift | ((Uint32)g >> format.Gloss) << format.Gshift
           | ((Uint32)b >> format.Bloss) << format.Bshift | format.Amask;
}

static constexpr Uint32 MapRGBFixPoint(const SDL_PixelFormat& format, Sint32 R, Sint32 G, Sint32 B)
{
    return MapRGB(format, static_cast<Uint8>(R >> 16), static_cast<Uint8>(G >> 16), static_cast<Uint8>(B >> 16));
}

static Uint32 ScaleRGB(const SDL_PixelFormat& format, Uint32 value, Sint32 factor)
{
    const auto r = ((static_cast<Uint8>((value & format.Rmask) >> format.Rshift) * factor) >> 16);
    const auto g = ((static_cast<Uint8>((value & format.Gmask) >> format.Gshift) * factor) >> 16);
    const auto b = ((static_cast<Uint8>((value & format.Bmask) >> format.Bshift) * factor) >> 16);
    const auto r8 = (Uint8)(r > 255 ? 255 : (r < 0 ? 0 : r));
    const auto g8 = (Uint8)(g > 255 ? 255 : (g < 0 ? 0 : g));
    const auto b8 = (Uint8)(b > 255 ? 255 : (b < 0 ? 0 : b));
    return MapRGB(format, r8, g8, b8);
}
//==================================================================================
// Draws a horisontal line, fading the colors
//==================================================================================
static void _FadedLine(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r1, Uint8 g1, Uint8 b1, Uint8 r2, Uint8 g2, Uint8 b2)
{
    Sint16 x;
    Uint8 t;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(r1, r2, t);
        SWAP(g1, g2, t);
        SWAP(b1, b2, t);
    }

    /* We use fixedpoint math */
    Sint32 R = r1 << 16;
    Sint32 G = g1 << 16;
    Sint32 B = b1 << 16;

    /* Color step value */
    Sint32 rstep = Sint32((r2 - r1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 gstep = Sint32((g2 - g1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 bstep = Sint32((b2 - b1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Update start colors */
        R += (sge_clip_xmin(dest) - x1) * rstep;
        G += (sge_clip_xmin(dest) - x1) * gstep;
        B += (sge_clip_xmin(dest) - x1) * bstep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto dstFormat = *dest->format;
    switch(dstFormat.BytesPerPixel)
    {
        case 1:
        { /* Assuming 8-bpp */
            Uint8* pixel;
            Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

            for(x = x1; x <= x2; x++)
            {
                pixel = row + x;

                *pixel = SDL_MapRGB(dest->format, R >> 16, G >> 16, B >> 16);

                R += rstep;
                G += gstep;
                B += bstep;
            }
        }
        break;

        case 2:
        { /* Probably 15-bpp or 16-bpp */
            Uint16* pixel;
            Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

            for(x = x1; x <= x2; x++)
            {
                pixel = row + x;

                *pixel = MapRGBFixPoint(*dest->format, R, G, B);

                R += rstep;
                G += gstep;
                B += bstep;
            }
        }
        break;

        case 3:
        { /* Slow 24-bpp mode, usually not used */
            Uint8* pixel;
            Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

            Uint8 rshift8 = dstFormat.Rshift / 8;
            Uint8 gshift8 = dstFormat.Gshift / 8;
            Uint8 bshift8 = dstFormat.Bshift / 8;

            for(x = x1; x <= x2; x++)
            {
                pixel = row + x * 3;

                *(pixel + rshift8) = R >> 16;
                *(pixel + gshift8) = G >> 16;
                *(pixel + bshift8) = B >> 16;

                R += rstep;
                G += gstep;
                B += bstep;
            }
        }
        break;

        case 4:
        { /* Probably 32-bpp */
            Uint32* pixel;
            Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

            for(x = x1; x <= x2; x++)
            {
                pixel = row + x;

                *pixel = MapRGBFixPoint(*dest->format, R, G, B);

                R += rstep;
                G += gstep;
                B += bstep;
            }
        }
        break;
    }
}

void sge_FadedLine(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r1, Uint8 g1, Uint8 b1, Uint8 r2, Uint8 g2, Uint8 b2)
{
    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    _FadedLine(dest, x1, x2, y, r1, g1, b1, r2, g2, b2);

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }
    sge_UpdateRect(dest, x1, y, absDiff(x1, x2) + 1, 1);
}

//==================================================================================
// Draws a horisontal, textured line
//==================================================================================
static void _TexturedLine(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2,
                          Sint16 sy2)
{
    Sint16 x;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(sx1, sx2, x);
        SWAP(sy1, sy2, x);
    }

    /* Fixed point texture starting coords */
    Sint32 srcx = sx1 << 16;
    Sint32 srcy = sy1 << 16;

    /* Texture coords stepping value */
    Sint32 xstep = Sint32((sx2 - sx1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 ystep = Sint32((sy2 - sy1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Fix texture starting coord */
        srcx += (sge_clip_xmin(dest) - x1) * xstep;
        srcy += (sge_clip_xmin(dest) - x1) * ystep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto dstFormat = *dest->format;
    if(dstFormat.BytesPerPixel == source->format->BytesPerPixel)
    {
        /* Fast mode. Just copy the pixel */

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8 pixel_value;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    pixel_value = *((Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16));

                    if(pixel_value != source->format->colorkey)
                        *pixel = pixel_value;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                Uint16 pitch = source->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint16*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8 *pixel, *srcpixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;
                    srcpixel = (Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16) * 3;

                    *(pixel + rshift8) = *(srcpixel + rshift8);
                    *(pixel + gshift8) = *(srcpixel + gshift8);
                    *(pixel + bshift8) = *(srcpixel + bshift8);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32 pixel_value;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                Uint16 pitch = source->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    pixel_value = *((Uint32*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));
                    if(pixel_value != source->format->colorkey)
                        *pixel = pixel_value;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    } else
    {
        /* Slow mode. We must translate every pixel color! */

        Uint8 r = 0, g = 0, b = 0;

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = SDL_MapRGB(dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);

                    *(pixel + rshift8) = r;
                    *(pixel + gshift8) = g;
                    *(pixel + bshift8) = b;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    }
}

//==================================================================================
// Draws a horisontal, gouraud shaded and textured line
//==================================================================================
static void _FadedTexturedLine(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2,
                               Sint16 sy2, Sint32 i1, Sint32 i2)
{
    Sint16 x;
    Sint32 i;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(sx1, sx2, x);
        SWAP(sy1, sy2, x);
        SWAP(i1, i2, i);
    }

    /* We use fixedpoint math */
    Sint32 I = i1;

    /* Color step value */
    Sint32 istep = (i2 - i1) / Sint32(x2 - x1 + 1);

    /* Fixed point texture starting coords */
    Sint32 srcx = sx1 << 16;
    Sint32 srcy = sy1 << 16;

    /* Texture coords stepping value */
    Sint32 xstep = Sint32((sx2 - sx1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 ystep = Sint32((sy2 - sy1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Update start colors */
        I += (sge_clip_xmin(dest) - x1) * istep;
        /* Fix texture starting coord */
        srcx += (sge_clip_xmin(dest) - x1) * xstep;
        srcy += (sge_clip_xmin(dest) - x1) * ystep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto srcFormat = *source->format;
    const auto dstFormat = *dest->format;

    if(dstFormat.BytesPerPixel == srcFormat.BytesPerPixel)
    {
        /* Fast mode. Just copy the pixel */

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                Uint16 pitch = source->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint16*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8 *pixel, *srcpixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;
                    srcpixel = (Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16) * 3;

                    *(pixel + rshift8) = *(srcpixel + rshift8);
                    *(pixel + gshift8) = *(srcpixel + gshift8);
                    *(pixel + bshift8) = *(srcpixel + bshift8);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* row = (Uint32*)dest->pixels + (Uint32)y * dest->pitch / sizeof(Uint32);

                Uint16 pitch = source->pitch / sizeof(Uint32);

                for(x = x1; x <= x2; x++)
                {
                    Uint32* pixel = row + x;

                    // SDL_GetRGB(*((Uint32 *)source->pixels + (srcy>>16)*pitch + (srcx>>16)), srcFormat, &r, &g, &b);
                    // r1=r*I;
                    // g1=g*I;
                    // b1=b*I;

                    /*
                    //32BPP: 0x FF FF FF FF
                    //          R  G  B  A
                    pixel_ptr = (Uint8*)((Uint32 *)source->pixels + (srcy>>16)*pitch + (srcx>>16));
                    r = (*(pixel_ptr++) * I) >> 16;
                    g = (*(pixel_ptr++) * I) >> 16;
                    b = (*(pixel_ptr) * I) >> 16;
                    r8 = (Uint8)(r > 255 ? 255 : (r < 0 ? 0 : r));
                    g8 = (Uint8)(g > 255 ? 255 : (g < 0 ? 0 : g));
                    b8 = (Uint8)(b > 255 ? 255 : (b < 0 ? 0 : b));
                    pixel_ptr = (Uint8*)pixel;
                    *(pixel_ptr++) = r8;
                    *(pixel_ptr++) = g8;
                    *(pixel_ptr) = b8;
                    */

                    Uint32 pixel_value = *((Uint32*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));
                    *pixel = ScaleRGB(dstFormat, pixel_value, I);

                    I += istep;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    } else
    {
        /* Slow mode. We must translate every pixel color! */

        Uint8 r = 0, g = 0, b = 0;

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), &srcFormat, &r, &g, &b);
                    *pixel = SDL_MapRGB(dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), &srcFormat, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), &srcFormat, &r, &g, &b);

                    *(pixel + rshift8) = r;
                    *(pixel + gshift8) = g;
                    *(pixel + bshift8) = b;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), &srcFormat, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    }
}

//==================================================================================
// Draws a horisontal, gouraud shaded and textured line respecting the color key
//==================================================================================
static void _FadedTexturedLineColorKey(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1, Sint16 sy1,
                                       Sint16 sx2, Sint16 sy2, Sint32 i1, Sint32 i2)
{
    Sint16 x;
    Sint32 i;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(sx1, sx2, x);
        SWAP(sy1, sy2, x);
        SWAP(i1, i2, i);
    }

    /* We use fixedpoint math */
    Sint32 I = i1;

    /* Color step value */
    Sint32 istep = (i2 - i1) / Sint32(x2 - x1 + 1);

    /* Fixed point texture starting coords */
    Sint32 srcx = sx1 << 16;
    Sint32 srcy = sy1 << 16;

    /* Texture coords stepping value */
    Sint32 xstep = Sint32((sx2 - sx1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 ystep = Sint32((sy2 - sy1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Update start colors */
        I += (sge_clip_xmin(dest) - x1) * istep;
        /* Fix texture starting coord */
        srcx += (sge_clip_xmin(dest) - x1) * xstep;
        srcy += (sge_clip_xmin(dest) - x1) * ystep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto dstFormat = *dest->format;
    if(dstFormat.BytesPerPixel == source->format->BytesPerPixel)
    {
        /* Fast mode. Just copy the pixel */

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                Uint16 pitch = source->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint16*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8 *pixel, *srcpixel;
                Uint8* row = (Uint8*)dest->pixels + (Uint32)y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;
                    srcpixel = (Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16) * 3;

                    *(pixel + rshift8) = *(srcpixel + rshift8);
                    *(pixel + gshift8) = *(srcpixel + gshift8);
                    *(pixel + bshift8) = *(srcpixel + bshift8);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32 pixel_value;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                Uint16 pitch = source->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    // SDL_GetRGB(*((Uint32 *)source->pixels + (srcy>>16)*pitch + (srcx>>16)), source->format, &r, &g, &b);
                    // r1=r*I;
                    // g1=g*I;
                    // b1=b*I;
                    // test for colorkey
                    pixel_value = *((Uint32*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));
                    if(pixel_value != source->format->colorkey)
                    {
                        *pixel = ScaleRGB(dstFormat, pixel_value, I);
                    }

                    I += istep;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    } else
    {
        /* Slow mode. We must translate every pixel color! */

        Uint8 r = 0, g = 0, b = 0;

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = SDL_MapRGB(dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);

                    *(pixel + rshift8) = r;
                    *(pixel + gshift8) = g;
                    *(pixel + bshift8) = b;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    }
}

//==================================================================================
// Draws a horisontal, textured line with precalculated gouraud shading
//==================================================================================
static void _PreCalcFadedTexturedLine(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1, Sint16 sy1,
                                      Sint16 sx2, Sint16 sy2, Uint16 i1, Uint16 i2, Uint8 PreCalcPalettes[][256])
{
    Sint16 x;
    Uint16 i;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(sx1, sx2, x);
        SWAP(sy1, sy2, x);
        SWAP(i1, i2, i);
    }

    /* We use fixedpoint math */
    Uint16 I = i1;

    /* Color step value */
    Sint16 istep = (i2 - i1) / (x2 - x1 + 1);

    /* Fixed point texture starting coords */
    Sint32 srcx = sx1 << 16;
    Sint32 srcy = sy1 << 16;

    /* Texture coords stepping value */
    Sint32 xstep = Sint32((sx2 - sx1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 ystep = Sint32((sy2 - sy1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Update start colors */
        I += (sge_clip_xmin(dest) - x1) * istep;
        /* Fix texture starting coord */
        srcx += (sge_clip_xmin(dest) - x1) * xstep;
        srcy += (sge_clip_xmin(dest) - x1) * ystep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto dstFormat = *dest->format;
    if(dstFormat.BytesPerPixel == source->format->BytesPerPixel)
    {
        /* Fast mode. Just copy the pixel */

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = PreCalcPalettes[(Uint8)(I >> 8)][*((Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16))];
                    // if (I>>8 < 0x00)
                    //    printf("\n(i1>>8):%d (i2>>8):%d I:%d (I>>8):%d (Uint8)(I>>8):%d istep:%d", i1>>8, i2>>8, I, I>>8, (Uint8)(I>>8),
                    //    istep);

                    srcx += xstep;
                    srcy += ystep;

                    I += istep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                Uint16 pitch = source->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint16*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8 *pixel, *srcpixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;
                    srcpixel = (Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16) * 3;

                    *(pixel + rshift8) = *(srcpixel + rshift8);
                    *(pixel + gshift8) = *(srcpixel + gshift8);
                    *(pixel + bshift8) = *(srcpixel + bshift8);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                Uint16 pitch = source->pitch / 4;

                // Uint32 r1,g1,b1;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    // SDL_GetRGB(*((Uint32 *)source->pixels + (srcy>>16)*pitch + (srcx>>16)), source->format, &r, &g, &b);
                    // r1=r*I;
                    // g1=g*I;
                    // b1=b*I;
                    const auto pixel_value = *((Uint32*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));
                    *pixel = ScaleRGB(dstFormat, pixel_value, I);

                    I += istep;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    } else
    {
        /* Slow mode. We must translate every pixel color! */

        Uint8 r = 0, g = 0, b = 0;

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = SDL_MapRGB(dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);

                    *(pixel + rshift8) = r;
                    *(pixel + gshift8) = g;
                    *(pixel + bshift8) = b;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    }
}

//==================================================================================
// Draws a horisontal, textured line with precalculated gouraud shading (and respecting the colorkey)
//==================================================================================
static void _PreCalcFadedTexturedLineColorKey(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1,
                                              Sint16 sy1, Sint16 sx2, Sint16 sy2, Uint16 i1, Uint16 i2, Uint8 PreCalcPalettes[][256])
{
    Sint16 x;
    Uint16 i;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(sx1, sx2, x);
        SWAP(sy1, sy2, x);
        SWAP(i1, i2, i);
    }

    /* We use fixedpoint math */
    Uint16 I = i1;

    /* Color step value */
    Sint16 istep = (i2 - i1) / (x2 - x1 + 1);

    /* Fixed point texture starting coords */
    Sint32 srcx = sx1 << 16;
    Sint32 srcy = sy1 << 16;

    /* Texture coords stepping value */
    Sint32 xstep = Sint32((sx2 - sx1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 ystep = Sint32((sy2 - sy1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Update start colors */
        I += (sge_clip_xmin(dest) - x1) * istep;
        /* Fix texture starting coord */
        srcx += (sge_clip_xmin(dest) - x1) * xstep;
        srcy += (sge_clip_xmin(dest) - x1) * ystep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto dstFormat = *dest->format;
    if(dstFormat.BytesPerPixel == source->format->BytesPerPixel)
    {
        /* Fast mode. Just copy the pixel */

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8 pixel_value;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    pixel_value = *((Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16));

                    if(pixel_value != source->format->colorkey)
                    {
                        *pixel = PreCalcPalettes[(Uint8)(I >> 8)][pixel_value];
                        // if (I>>8 < 0x00)
                        //    printf("\n(i1>>8):%d (i2>>8):%d I:%d (I>>8):%d (Uint8)(I>>8):%d istep:%d", i1>>8, i2>>8, I, I>>8,
                        //    (Uint8)(I>>8), istep);
                    }

                    srcx += xstep;
                    srcy += ystep;

                    I += istep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                Uint16 pitch = source->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint16*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8 *pixel, *srcpixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;
                    srcpixel = (Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16) * 3;

                    *(pixel + rshift8) = *(srcpixel + rshift8);
                    *(pixel + gshift8) = *(srcpixel + gshift8);
                    *(pixel + bshift8) = *(srcpixel + bshift8);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                Uint16 pitch = source->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    // SDL_GetRGB(*((Uint32 *)source->pixels + (srcy>>16)*pitch + (srcx>>16)), source->format, &r, &g, &b);
                    // r1=r*I;
                    // g1=g*I;
                    // b1=b*I;
                    const auto pixel_value = *((Uint32*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));
                    *pixel = ScaleRGB(dstFormat, pixel_value, I);

                    I += istep;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    } else
    {
        /* Slow mode. We must translate every pixel color! */

        Uint8 r = 0, g = 0, b = 0;

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = SDL_MapRGB(dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);

                    *(pixel + rshift8) = r;
                    *(pixel + gshift8) = g;
                    *(pixel + bshift8) = b;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    }
}

//==================================================================================
// Draws a horisontal, textured line with precalculated gouraud shading (respecting colorkeys)
//==================================================================================
static void _PreCalcFadedTexturedLineColorKeys(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1,
                                               Sint16 sy1, Sint16 sx2, Sint16 sy2, Uint16 i1, Uint16 i2, Uint8 PreCalcPalettes[][256],
                                               const Uint32 keys[], int keycount)
{
    Sint16 x;
    Uint16 i;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(sx1, sx2, x);
        SWAP(sy1, sy2, x);
        SWAP(i1, i2, i);
    }

    /* We use fixedpoint math */
    Uint16 I = i1;

    /* Color step value */
    Sint16 istep = (i2 - i1) / (x2 - x1 + 1);

    /* Fixed point texture starting coords */
    Sint32 srcx = sx1 << 16;
    Sint32 srcy = sy1 << 16;

    /* Texture coords stepping value */
    Sint32 xstep = Sint32((sx2 - sx1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 ystep = Sint32((sy2 - sy1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Update start colors */
        I += (sge_clip_xmin(dest) - x1) * istep;
        /* Fix texture starting coord */
        srcx += (sge_clip_xmin(dest) - x1) * xstep;
        srcy += (sge_clip_xmin(dest) - x1) * ystep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto dstFormat = *dest->format;
    if(dstFormat.BytesPerPixel == source->format->BytesPerPixel)
    {
        /* Fast mode. Just copy the pixel */

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8 pixel_value;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;
                bool isColorKey;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    isColorKey = false;
                    pixel_value = *((Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16));
                    // test for colorkey
                    for(int i = 0; i < keycount; i++)
                    {
                        if(pixel_value == (Uint8)keys[i])
                        {
                            isColorKey = true;
                            break;
                        }
                    }

                    if(!isColorKey)
                    {
                        *pixel = PreCalcPalettes[(Uint8)(I >> 8)][pixel_value];
                        // printf("I:%d\nUint8 I:%d\nUint8 I>>8:%d\nUint8 (I>>8):%d\n", I, (Uint8)I, (Uint8)I>>8, (Uint8)(I>>8));
                    }

                    srcx += xstep;
                    srcy += ystep;

                    I += istep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                Uint16 pitch = source->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint16*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8 *pixel, *srcpixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;
                    srcpixel = (Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16) * 3;

                    *(pixel + rshift8) = *(srcpixel + rshift8);
                    *(pixel + gshift8) = *(srcpixel + gshift8);
                    *(pixel + bshift8) = *(srcpixel + bshift8);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                Uint16 pitch = source->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    const auto pixel_value = *((Uint32*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));
                    *pixel = ScaleRGB(dstFormat, pixel_value, I);

                    I += istep;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    } else
    {
        /* Slow mode. We must translate every pixel color! */

        Uint8 r = 0, g = 0, b = 0;

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = SDL_MapRGB(dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);

                    *(pixel + rshift8) = r;
                    *(pixel + gshift8) = g;
                    *(pixel + bshift8) = b;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    }
}

//==================================================================================
// Draws a horisontal, gouraud shaded and textured line (respecting colorkeys)
//==================================================================================
static void _FadedTexturedLineColorKeys(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1, Sint16 sy1,
                                        Sint16 sx2, Sint16 sy2, Sint32 i1, Sint32 i2, const Uint32 keys[], int keycount)
{
    Sint16 x;
    Sint32 i;

    /* Fix coords */
    if(x1 > x2)
    {
        SWAP(x1, x2, x);
        SWAP(sx1, sx2, x);
        SWAP(sy1, sy2, x);
        SWAP(i1, i2, i);
    }

    /* We use fixedpoint math */
    Sint32 I = i1;

    /* Color step value */
    Sint32 istep = (i2 - i1) / Sint32(x2 - x1 + 1);

    /* Fixed point texture starting coords */
    Sint32 srcx = sx1 << 16;
    Sint32 srcy = sy1 << 16;

    /* Texture coords stepping value */
    Sint32 xstep = Sint32((sx2 - sx1) << 16) / Sint32(x2 - x1 + 1);
    Sint32 ystep = Sint32((sy2 - sy1) << 16) / Sint32(x2 - x1 + 1);

    /* Clipping */
    if(x2 < sge_clip_xmin(dest) || x1 > sge_clip_xmax(dest) || y < sge_clip_ymin(dest) || y > sge_clip_ymax(dest))
        return;
    if(x1 < sge_clip_xmin(dest))
    {
        /* Update start colors */
        I += (sge_clip_xmin(dest) - x1) * istep;
        /* Fix texture starting coord */
        srcx += (sge_clip_xmin(dest) - x1) * xstep;
        srcy += (sge_clip_xmin(dest) - x1) * ystep;
        x1 = sge_clip_xmin(dest);
    }
    if(x2 > sge_clip_xmax(dest))
        x2 = sge_clip_xmax(dest);

    const auto dstFormat = *dest->format;
    if(dstFormat.BytesPerPixel == source->format->BytesPerPixel)
    {
        /* Fast mode. Just copy the pixel */

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                Uint16 pitch = source->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    *pixel = *((Uint16*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8 *pixel, *srcpixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;
                    srcpixel = (Uint8*)source->pixels + (srcy >> 16) * source->pitch + (srcx >> 16) * 3;

                    *(pixel + rshift8) = *(srcpixel + rshift8);
                    *(pixel + gshift8) = *(srcpixel + gshift8);
                    *(pixel + bshift8) = *(srcpixel + bshift8);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32 pixel_value;
                bool isColorKey;
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                Uint16 pitch = source->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    isColorKey = false;

                    pixel = row + x;

                    pixel_value = *((Uint32*)source->pixels + (srcy >> 16) * pitch + (srcx >> 16));
                    // test for colorkey
                    for(int i = 0; i < keycount; i++)
                    {
                        if(pixel_value == keys[i])
                        {
                            isColorKey = true;
                            break;
                        }
                    }

                    if(!isColorKey)
                    {
                        *pixel = ScaleRGB(dstFormat, pixel_value, I);
                    }

                    I += istep;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    } else
    {
        /* Slow mode. We must translate every pixel color! */

        Uint8 r = 0, g = 0, b = 0;

        switch(dstFormat.BytesPerPixel)
        {
            case 1:
            { /* Assuming 8-bpp */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = SDL_MapRGB(dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 2:
            { /* Probably 15-bpp or 16-bpp */
                Uint16* pixel;
                Uint16* row = (Uint16*)dest->pixels + y * dest->pitch / 2;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 3:
            { /* Slow 24-bpp mode, usually not used */
                Uint8* pixel;
                Uint8* row = (Uint8*)dest->pixels + y * dest->pitch;

                Uint8 rshift8 = dstFormat.Rshift / 8;
                Uint8 gshift8 = dstFormat.Gshift / 8;
                Uint8 bshift8 = dstFormat.Bshift / 8;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x * 3;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);

                    *(pixel + rshift8) = r;
                    *(pixel + gshift8) = g;
                    *(pixel + bshift8) = b;

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;

            case 4:
            { /* Probably 32-bpp */
                Uint32* pixel;
                Uint32* row = (Uint32*)dest->pixels + y * dest->pitch / 4;

                for(x = x1; x <= x2; x++)
                {
                    pixel = row + x;

                    SDL_GetRGB(sge_GetPixel(source, srcx >> 16, srcy >> 16), source->format, &r, &g, &b);
                    *pixel = MapRGB(*dest->format, r, g, b);

                    srcx += xstep;
                    srcy += ystep;
                }
            }
            break;
        }
    }
}

void sge_TexturedLine(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2,
                      Sint16 sy2)
{
    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;
    if(SDL_MUSTLOCK(source) && _sge_lock)
        if(SDL_LockSurface(source) < 0)
            return;

    _TexturedLine(dest, x1, x2, y, source, sx1, sy1, sx2, sy2);

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);
    if(SDL_MUSTLOCK(source) && _sge_lock)
        SDL_UnlockSurface(source);

    if(_sge_update != 1)
    {
        return;
    }
    sge_UpdateRect(dest, x1, y, absDiff(x1, x2) + 1, 1);
}

void sge_FadedTexturedLine(SDL_Surface* dest, Sint16 x1, Sint16 x2, Sint16 y, SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2,
                           Sint16 sy2, Sint32 i1, Sint32 i2)
{
    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;
    if(SDL_MUSTLOCK(source) && _sge_lock)
        if(SDL_LockSurface(source) < 0)
            return;

    _FadedTexturedLine(dest, x1, x2, y, source, sx1, sy1, sx2, sy2, i1, i2);

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);
    if(SDL_MUSTLOCK(source) && _sge_lock)
        SDL_UnlockSurface(source);

    if(_sge_update != 1)
    {
        return;
    }
    sge_UpdateRect(dest, x1, y, absDiff(x1, x2) + 1, 1);
}

//==================================================================================
// Draws a trigon
//==================================================================================
void sge_Trigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color)
{
    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    _Line(dest, x1, y1, x2, y2, color);
    _Line(dest, x1, y1, x3, y3, color);
    _Line(dest, x3, y3, x2, y2, color);

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, ymax = y1, xmin = x1, ymin = y1;
    xmax = (xmax > x2) ? xmax : x2;
    ymax = (ymax > y2) ? ymax : y2;
    xmin = (xmin < x2) ? xmin : x2;
    ymin = (ymin < y2) ? ymin : y2;
    xmax = (xmax > x3) ? xmax : x3;
    ymax = (ymax > y3) ? ymax : y3;
    xmin = (xmin < x3) ? xmin : x3;
    ymin = (ymin < y3) ? ymin : y3;

    sge_UpdateRect(dest, xmin, ymin, numeric_cast<Uint16>(xmax - xmin + 1), numeric_cast<Uint16>(ymax - ymin + 1));
}

void sge_Trigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint8 R, Uint8 G, Uint8 B)
{
    sge_Trigon(dest, x1, y1, x2, y2, x3, y3, SDL_MapRGB(dest->format, R, G, B));
}

//==================================================================================
// Draws a trigon (alpha)
//==================================================================================
void sge_TrigonAlpha(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color, Uint8 alpha)
{
    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    _LineAlpha(dest, x1, y1, x2, y2, color, alpha);
    _LineAlpha(dest, x1, y1, x3, y3, color, alpha);
    _LineAlpha(dest, x3, y3, x2, y2, color, alpha);

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, ymax = y1, xmin = x1, ymin = y1;
    xmax = (xmax > x2) ? xmax : x2;
    ymax = (ymax > y2) ? ymax : y2;
    xmin = (xmin < x2) ? xmin : x2;
    ymin = (ymin < y2) ? ymin : y2;
    xmax = (xmax > x3) ? xmax : x3;
    ymax = (ymax > y3) ? ymax : y3;
    xmin = (xmin < x3) ? xmin : x3;
    ymin = (ymin < y3) ? ymin : y3;

    sge_UpdateRect(dest, xmin, ymin, numeric_cast<Uint16>(xmax - xmin + 1), numeric_cast<Uint16>(ymax - ymin + 1));
}

void sge_TrigonAlpha(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint8 R, Uint8 G, Uint8 B,
                     Uint8 alpha)
{
    sge_TrigonAlpha(dest, x1, y1, x2, y2, x3, y3, SDL_MapRGB(dest->format, R, G, B), alpha);
}

//==================================================================================
// Draws an AA trigon (alpha)
//==================================================================================
void sge_AATrigonAlpha(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color, Uint8 alpha)
{
    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    _AALineAlpha(dest, x1, y1, x2, y2, color, alpha);
    _AALineAlpha(dest, x1, y1, x3, y3, color, alpha);
    _AALineAlpha(dest, x3, y3, x2, y2, color, alpha);

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, ymax = y1, xmin = x1, ymin = y1;
    xmax = (xmax > x2) ? xmax : x2;
    ymax = (ymax > y2) ? ymax : y2;
    xmin = (xmin < x2) ? xmin : x2;
    ymin = (ymin < y2) ? ymin : y2;
    xmax = (xmax > x3) ? xmax : x3;
    ymax = (ymax > y3) ? ymax : y3;
    xmin = (xmin < x3) ? xmin : x3;
    ymin = (ymin < y3) ? ymin : y3;

    sge_UpdateRect(dest, xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
}

void sge_AATrigonAlpha(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint8 R, Uint8 G, Uint8 B,
                       Uint8 alpha)
{
    sge_AATrigonAlpha(dest, x1, y1, x2, y2, x3, y3, SDL_MapRGB(dest->format, R, G, B), alpha);
}

void sge_AATrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color)
{
    sge_AATrigonAlpha(dest, x1, y1, x2, y2, x3, y3, color, 255);
}

void sge_AATrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint8 R, Uint8 G, Uint8 B)
{
    sge_AATrigonAlpha(dest, x1, y1, x2, y2, x3, y3, SDL_MapRGB(dest->format, R, G, B), 255);
}

//==================================================================================
// Draws a filled trigon
//==================================================================================
void sge_FilledTrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color)
{
    Sint16 y;

    if(y1 == y3)
        return;

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
    }

    /*
     * How do we calculate the starting and ending x coordinate of the horizontal line
     * on each y coordinate?  We can do this by using a standard line algorithm but
     * instead of plotting pixels, use the x coordinates as start and stop
     * coordinates for the horizontal line.
     * So we will simply trace the outlining of the triangle; this will require 3 lines.
     * Line 1 is the line between (x1,y1) and (x2,y2)
     * Line 2 is the line between (x1,y1) and (x3,y3)
     * Line 3 is the line between (x2,y2) and (x3,y3)
     *
     * We can divide the triangle into 2 halfs. The upper half will be outlined by line
     * 1 and 2. The lower half will be outlined by line line 2 and 3.
     */

    /* Starting coords for the three lines */
    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    /* Lines step values */
    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    /* Upper half of the triangle */
    if(y1 == y2)
        _HLine(dest, x1, x2, y1, color);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            _HLine(dest, xa >> 16, xb >> 16, y, color);

            xa += m1;
            xb += m2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        _HLine(dest, x2, x3, y2, color);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            _HLine(dest, xb >> 16, xc >> 16, y, color);

            xb += m2;
            xc += m3;
        }
    }

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, numeric_cast<Uint16>(xmax - xmin + 1), numeric_cast<Uint16>(y3 - y1 + 1));
}

void sge_FilledTrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint8 R, Uint8 G, Uint8 B)
{
    sge_FilledTrigon(dest, x1, y1, x2, y2, x3, y3, SDL_MapRGB(dest->format, R, G, B));
}

//==================================================================================
// Draws a filled trigon (alpha)
//==================================================================================
void sge_FilledTrigonAlpha(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color, Uint8 alpha)
{
    Sint16 y;

    if(y1 == y3)
        return;

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
    }

    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    /* Upper half of the triangle */
    if(y1 == y2)
        _HLineAlpha(dest, x1, x2, y1, color, alpha);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            _HLineAlpha(dest, xa >> 16, xb >> 16, y, color, alpha);

            xa += m1;
            xb += m2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        _HLineAlpha(dest, x2, x3, y2, color, alpha);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            _HLineAlpha(dest, xb >> 16, xc >> 16, y, color, alpha);

            xb += m2;
            xc += m3;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, numeric_cast<Uint16>(xmax - xmin + 1), numeric_cast<Uint16>(y3 - y1 + 1));
}

void sge_FilledTrigonAlpha(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint8 R, Uint8 G, Uint8 B,
                           Uint8 alpha)
{
    sge_FilledTrigonAlpha(dest, x1, y1, x2, y2, x3, y3, SDL_MapRGB(dest->format, R, G, B), alpha);
}

//==================================================================================
// Draws a gourand shaded trigon
//==================================================================================
void sge_FadedTrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c1, Uint32 c2, Uint32 c3)
{
    Sint16 y;

    if(y1 == y3)
        return;

    Uint8 c = 0;
    SDL_Color col1;
    SDL_Color col2;
    SDL_Color col3;

    col1 = sge_GetRGB(dest, c1);
    col2 = sge_GetRGB(dest, c2);
    col3 = sge_GetRGB(dest, c3);

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(col1.r, col2.r, c);
        SWAP(col1.g, col2.g, c);
        SWAP(col1.b, col2.b, c);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
        SWAP(col2.r, col3.r, c);
        SWAP(col2.g, col3.g, c);
        SWAP(col2.b, col3.b, c);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(col1.r, col2.r, c);
        SWAP(col1.g, col2.g, c);
        SWAP(col1.b, col2.b, c);
    }

    /*
     * We trace three lines exactly like in sge_FilledTrigon(), but here we
     * must also keep track of the colors. We simply calculate how the color
     * will change along the three lines.
     */

    /* Starting coords for the three lines */
    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    /* Starting colors (rgb) for the three lines */
    auto r1 = Sint32(col1.r << 16);
    Sint32 r2 = r1;
    auto r3 = Sint32(col2.r << 16);

    auto g1 = Sint32(col1.g << 16);
    Sint32 g2 = g1;
    auto g3 = Sint32(col2.g << 16);

    auto b1 = Sint32(col1.b << 16);
    Sint32 b2 = b1;
    auto b3 = Sint32(col2.b << 16);

    /* Lines step values */
    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    /* Colors step values */
    Sint32 rstep1 = 0;
    Sint32 rstep2 = Sint32((col3.r - col1.r) << 16) / Sint32(y3 - y1);
    Sint32 rstep3 = 0;

    Sint32 gstep1 = 0;
    Sint32 gstep2 = Sint32((col3.g - col1.g) << 16) / Sint32(y3 - y1);
    Sint32 gstep3 = 0;

    Sint32 bstep1 = 0;
    Sint32 bstep2 = Sint32((col3.b - col1.b) << 16) / Sint32(y3 - y1);
    Sint32 bstep3 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    /* Upper half of the triangle */
    if(y1 == y2)
        _FadedLine(dest, x1, x2, y1, col1.r, col1.g, col1.b, col2.r, col2.g, col2.b);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        rstep1 = Sint32((col2.r - col1.r) << 16) / Sint32(y2 - y1);
        gstep1 = Sint32((col2.g - col1.g) << 16) / Sint32(y2 - y1);
        bstep1 = Sint32((col2.b - col1.b) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            _FadedLine(dest, xa >> 16, xb >> 16, y, r1 >> 16, g1 >> 16, b1 >> 16, r2 >> 16, g2 >> 16, b2 >> 16);

            xa += m1;
            xb += m2;

            r1 += rstep1;
            g1 += gstep1;
            b1 += bstep1;

            r2 += rstep2;
            g2 += gstep2;
            b2 += bstep2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        _FadedLine(dest, x2, x3, y2, col2.r, col2.g, col2.b, col3.r, col3.g, col3.b);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        rstep3 = Sint32((col3.r - col2.r) << 16) / Sint32(y3 - y2);
        gstep3 = Sint32((col3.g - col2.g) << 16) / Sint32(y3 - y2);
        bstep3 = Sint32((col3.b - col2.b) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            _FadedLine(dest, xb >> 16, xc >> 16, y, r2 >> 16, g2 >> 16, b2 >> 16, r3 >> 16, g3 >> 16, b3 >> 16);

            xb += m2;
            xc += m3;

            r2 += rstep2;
            g2 += gstep2;
            b2 += bstep2;

            r3 += rstep3;
            g3 += gstep3;
            b3 += bstep3;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, xmax - xmin + 1, y3 - y1 + 1);
}

//==================================================================================
// Draws a texured trigon (fast)
//==================================================================================
void sge_TexturedTrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, SDL_Surface* source,
                        Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3)
{
    Sint16 y;

    if(y1 == y3)
        return;

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
        SWAP(sx2, sx3, y);
        SWAP(sy2, sy3, y);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }

    /*
     * Again we do the same thing as in sge_FilledTrigon(). But here we must keep track of how the
     * texture coords change along the lines.
     */

    /* Starting coords for the three lines */
    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    /* Lines step values */
    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    /* Starting texture coords for the three lines */
    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);

    /* Texture coords stepping value */
    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;
    if(SDL_MUSTLOCK(source) && _sge_lock)
        if(SDL_LockSurface(source) < 0)
            return;

    /* Upper half of the triangle */
    if(y1 == y2)
        _TexturedLine(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            _TexturedLine(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16);

            xa += m1;
            xb += m2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        _TexturedLine(dest, x2, x3, y2, source, sx2, sy2, sx3, sy3);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        xstep3 = Sint32((sx3 - sx2) << 16) / Sint32(y3 - y2);
        ystep3 = Sint32((sy3 - sy2) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            _TexturedLine(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16);

            xb += m2;
            xc += m3;

            srcx2 += xstep2;
            srcx3 += xstep3;
            srcy2 += ystep2;
            srcy3 += ystep3;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);
    if(SDL_MUSTLOCK(source) && _sge_lock)
        SDL_UnlockSurface(source);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, numeric_cast<Uint16>(xmax - xmin + 1), numeric_cast<Uint16>(y3 - y1 + 1));
}

//==================================================================================
// Draws a gouraud shaded and texured trigon (fast)
//==================================================================================
void sge_FadedTexturedTrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, SDL_Surface* source,
                             Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3, Sint32 I1, Sint32 I2, Sint32 I3)
{
    Sint16 y;

    if(y1 == y3)
        return;

    Sint32 i = 0;
    Sint32 i_orig1 = I1;
    Sint32 i_orig2 = I2;
    Sint32 i_orig3 = I3;

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
        SWAP(sx2, sx3, y);
        SWAP(sy2, sy3, y);
        SWAP(i_orig2, i_orig3, i);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }

    /*
     * Again we do the same thing as in sge_FilledTrigon(). But here we must keep track of how the
     * texture coords change along the lines.
     */

    /* Starting coords for the three lines */
    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    /* Starting colors (rgb) for the three lines */
    Sint32 i1 = i_orig1;
    Sint32 i2 = i1;
    Sint32 i3 = i_orig2;

    /* Lines step values */
    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    /* Colors step values */
    Sint32 istep1 = 0;
    Sint32 istep2 = (i_orig3 - i1) / Sint32(y3 - y1);
    Sint32 istep3 = 0;

    /* Starting texture coords for the three lines */
    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);

    /* Texture coords stepping value */
    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;
    if(SDL_MUSTLOCK(source) && _sge_lock)
        if(SDL_LockSurface(source) < 0)
            return;

    /* Upper half of the triangle */
    if(y1 == y2)
        //_TexturedLine(dest,x1,x2,y1,source,sx1,sy1,sx2,sy2);
        //_FadedLine(dest, x1, x2, y1, col1.r, col1.g, col1.b, col2.r, col2.g, col2.b);
        _FadedTexturedLine(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2, i_orig1, i_orig2);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        istep1 = (i_orig2 - i_orig1) / Sint32(y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            //_TexturedLine(dest, xa>>16, xb>>16, y, source, srcx1>>16, srcy1>>16, srcx2>>16, srcy2>>16);
            //_FadedLine(dest, xa>>16, xb>>16, y, r1>>16, g1>>16, b1>>16, r2>>16, g2>>16, b2>>16);
            _FadedTexturedLine(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16, i1, i2);

            xa += m1;
            xb += m2;

            i1 += istep1;

            i2 += istep2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        //_TexturedLine(dest,x2,x3,y2,source,sx2,sy2,sx3,sy3);
        //_FadedLine(dest, x2, x3, y2, col2.r, col2.g, col2.b, col3.r, col3.g, col3.b);
        _FadedTexturedLine(dest, x2, x3, y2, source, sx2, sy2, sx3, sy3, i_orig2, i_orig3);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        istep3 = (i_orig3 - i_orig2) / Sint32(y3 - y2);

        xstep3 = Sint32((sx3 - sx2) << 16) / Sint32(y3 - y2);
        ystep3 = Sint32((sy3 - sy2) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            //_TexturedLine(dest, xb>>16, xc>>16, y, source, srcx2>>16, srcy2>>16, srcx3>>16, srcy3>>16);
            //_FadedLine(dest, xb>>16, xc>>16, y, r2>>16, g2>>16, b2>>16, r3>>16, g3>>16, b3>>16);
            _FadedTexturedLine(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16, i2, i3);

            xb += m2;
            xc += m3;

            i2 += istep2;

            i3 += istep3;

            srcx2 += xstep2;
            srcx3 += xstep3;
            srcy2 += ystep2;
            srcy3 += ystep3;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);
    if(SDL_MUSTLOCK(source) && _sge_lock)
        SDL_UnlockSurface(source);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, numeric_cast<Uint16>(xmax - xmin + 1), numeric_cast<Uint16>(y3 - y1 + 1));
}

//==================================================================================
// Draws a texured trigon  with precalculated gouraud shading (fast)
//==================================================================================
void sge_PreCalcFadedTexturedTrigon(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
                                    SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3, Uint16 I1,
                                    Uint16 I2, Uint16 I3, Uint8 PreCalcPalettes[][256])
{
    Sint16 y;

    if(y1 == y3)
        return;

    Uint16 i = 0;
    Uint16 i_orig1 = I1;
    Uint16 i_orig2 = I2;
    Uint16 i_orig3 = I3;

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
        SWAP(sx2, sx3, y);
        SWAP(sy2, sy3, y);
        SWAP(i_orig2, i_orig3, i);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }

    /*
     * Again we do the same thing as in sge_FilledTrigon(). But here we must keep track of how the
     * texture coords change along the lines.
     */

    /* Starting coords for the three lines */
    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    /* Starting colors (rgb) for the three lines */
    Uint16 i1 = i_orig1;
    Uint16 i2 = i1;
    Uint16 i3 = i_orig2;

    /* Lines step values */
    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    /* Colors step values */
    Sint16 istep1 = 0;
    Sint16 istep2 = (i_orig3 - i1) / (y3 - y1);
    Sint16 istep3 = 0;

    /* Starting texture coords for the three lines */
    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);

    /* Texture coords stepping value */
    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;
    if(SDL_MUSTLOCK(source) && _sge_lock)
        if(SDL_LockSurface(source) < 0)
            return;

    /* Upper half of the triangle */
    if(y1 == y2)
        //_TexturedLine(dest,x1,x2,y1,source,sx1,sy1,sx2,sy2);
        //_FadedLine(dest, x1, x2, y1, col1.r, col1.g, col1.b, col2.r, col2.g, col2.b);
        _PreCalcFadedTexturedLine(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2, i_orig1, i_orig2, PreCalcPalettes);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        istep1 = (i_orig2 - i_orig1) / (y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            //_TexturedLine(dest, xa>>16, xb>>16, y, source, srcx1>>16, srcy1>>16, srcx2>>16, srcy2>>16);
            //_FadedLine(dest, xa>>16, xb>>16, y, r1>>16, g1>>16, b1>>16, r2>>16, g2>>16, b2>>16);
            _PreCalcFadedTexturedLine(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16, i1, i2,
                                      PreCalcPalettes);

            // if (i1 < 0 || i2 < 0)
            // printf("\nx1:%d y1:%d x2:%d y2:%d x3:%d y3:%d i1>>8:%d i2>>8:%d istep1:%d istep2:%d I1>>8:%d I2>>8:%d I3>>8:%d", x1, y1, x2,
            // y2, x3, y3, i1>>8, i2>>8, istep1, istep2, I1>>7, I2>>8, I3>>8);

            xa += m1;
            xb += m2;

            i1 += istep1;

            i2 += istep2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        //_TexturedLine(dest,x2,x3,y2,source,sx2,sy2,sx3,sy3);
        //_FadedLine(dest, x2, x3, y2, col2.r, col2.g, col2.b, col3.r, col3.g, col3.b);
        _PreCalcFadedTexturedLine(dest, x2, x3, y2, source, sx2, sy2, sx3, sy3, i_orig2, i_orig3, PreCalcPalettes);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        istep3 = (i_orig3 - i_orig2) / (y3 - y2);

        xstep3 = Sint32((sx3 - sx2) << 16) / Sint32(y3 - y2);
        ystep3 = Sint32((sy3 - sy2) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            //_TexturedLine(dest, xb>>16, xc>>16, y, source, srcx2>>16, srcy2>>16, srcx3>>16, srcy3>>16);
            //_FadedLine(dest, xb>>16, xc>>16, y, r2>>16, g2>>16, b2>>16, r3>>16, g3>>16, b3>>16);
            _PreCalcFadedTexturedLine(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16, i2, i3,
                                      PreCalcPalettes);

            xb += m2;
            xc += m3;

            i2 += istep2;

            i3 += istep3;

            srcx2 += xstep2;
            srcx3 += xstep3;
            srcy2 += ystep2;
            srcy3 += ystep3;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);
    if(SDL_MUSTLOCK(source) && _sge_lock)
        SDL_UnlockSurface(source);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, numeric_cast<Uint16>(xmax - xmin + 1), numeric_cast<Uint16>(y3 - y1 + 1));
}

//==================================================================================
// Draws a gouraud shaded and texured trigon (fast) (respecting colorkeys)
//==================================================================================
void sge_FadedTexturedTrigonColorKeys(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
                                      SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3,
                                      Sint32 I1, Sint32 I2, Sint32 I3, Uint32 keys[], int keycount)
{
    Sint16 y;

    if(y1 == y3)
        return;

    Sint32 i = 0;
    Sint32 i_orig1 = I1;
    Sint32 i_orig2 = I2;
    Sint32 i_orig3 = I3;

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
        SWAP(sx2, sx3, y);
        SWAP(sy2, sy3, y);
        SWAP(i_orig2, i_orig3, i);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }

    /*
     * Again we do the same thing as in sge_FilledTrigon(). But here we must keep track of how the
     * texture coords change along the lines.
     */

    /* Starting coords for the three lines */
    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    /* Starting colors (rgb) for the three lines */
    Sint32 i1 = i_orig1;
    Sint32 i2 = i1;
    Sint32 i3 = i_orig2;

    /* Lines step values */
    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    /* Colors step values */
    Sint32 istep1 = 0;
    Sint32 istep2 = (i_orig3 - i1) / Sint32(y3 - y1);
    Sint32 istep3 = 0;

    /* Starting texture coords for the three lines */
    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);

    /* Texture coords stepping value */
    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;
    if(SDL_MUSTLOCK(source) && _sge_lock)
        if(SDL_LockSurface(source) < 0)
            return;

    /* Upper half of the triangle */
    if(y1 == y2)
        //_TexturedLine(dest,x1,x2,y1,source,sx1,sy1,sx2,sy2);
        //_FadedLine(dest, x1, x2, y1, col1.r, col1.g, col1.b, col2.r, col2.g, col2.b);
        _FadedTexturedLineColorKeys(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2, i_orig1, i_orig2, keys, keycount);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        istep1 = (i_orig2 - i_orig1) / Sint32(y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            //_TexturedLine(dest, xa>>16, xb>>16, y, source, srcx1>>16, srcy1>>16, srcx2>>16, srcy2>>16);
            //_FadedLine(dest, xa>>16, xb>>16, y, r1>>16, g1>>16, b1>>16, r2>>16, g2>>16, b2>>16);
            _FadedTexturedLineColorKeys(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16, i1, i2,
                                        keys, keycount);

            xa += m1;
            xb += m2;

            i1 += istep1;

            i2 += istep2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        //_TexturedLine(dest,x2,x3,y2,source,sx2,sy2,sx3,sy3);
        //_FadedLine(dest, x2, x3, y2, col2.r, col2.g, col2.b, col3.r, col3.g, col3.b);
        _FadedTexturedLineColorKeys(dest, x2, x3, y2, source, sx2, sy2, sx3, sy3, i_orig2, i_orig3, keys, keycount);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        istep3 = (i_orig3 - i_orig2) / Sint32(y3 - y2);

        xstep3 = Sint32((sx3 - sx2) << 16) / Sint32(y3 - y2);
        ystep3 = Sint32((sy3 - sy2) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            //_TexturedLine(dest, xb>>16, xc>>16, y, source, srcx2>>16, srcy2>>16, srcx3>>16, srcy3>>16);
            //_FadedLine(dest, xb>>16, xc>>16, y, r2>>16, g2>>16, b2>>16, r3>>16, g3>>16, b3>>16);
            _FadedTexturedLineColorKeys(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16, i2, i3,
                                        keys, keycount);

            xb += m2;
            xc += m3;

            i2 += istep2;

            i3 += istep3;

            srcx2 += xstep2;
            srcx3 += xstep3;
            srcy2 += ystep2;
            srcy3 += ystep3;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);
    if(SDL_MUSTLOCK(source) && _sge_lock)
        SDL_UnlockSurface(source);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, xmax - xmin + 1, y3 - y1 + 1);
}

//==================================================================================
// Draws a texured trigon  with precalculated gouraud shading (fast) respecting the color keys
//==================================================================================
void sge_PreCalcFadedTexturedTrigonColorKeys(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
                                             SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3,
                                             Uint16 I1, Uint16 I2, Uint16 I3, Uint8 PreCalcPalettes[][256], Uint32 keys[], int keycount)
{
    Sint16 y;

    if(y1 == y3)
        return;

    Uint16 i = 0;
    Uint16 i_orig1 = I1;
    Uint16 i_orig2 = I2;
    Uint16 i_orig3 = I3;

    /* Sort coords */
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }
    if(y2 > y3)
    {
        SWAP(y2, y3, y);
        SWAP(x2, x3, y);
        SWAP(sx2, sx3, y);
        SWAP(sy2, sy3, y);
        SWAP(i_orig2, i_orig3, i);
    }
    if(y1 > y2)
    {
        SWAP(y1, y2, y);
        SWAP(x1, x2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
        SWAP(i_orig1, i_orig2, i);
    }

    /*
     * Again we do the same thing as in sge_FilledTrigon(). But here we must keep track of how the
     * texture coords change along the lines.
     */

    /* Starting coords for the three lines */
    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);

    /* Starting colors (rgb) for the three lines */
    Uint16 i1 = i_orig1;
    Uint16 i2 = i1;
    Uint16 i3 = i_orig2;

    /* Lines step values */
    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = 0;

    /* Colors step values */
    Sint16 istep1 = 0;
    Sint16 istep2 = (i_orig3 - i1) / (y3 - y1);
    Sint16 istep3 = 0;

    /* Starting texture coords for the three lines */
    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);

    /* Texture coords stepping value */
    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;
    if(SDL_MUSTLOCK(source) && _sge_lock)
        if(SDL_LockSurface(source) < 0)
            return;

    /* Upper half of the triangle */
    if(y1 == y2)
        //_TexturedLine(dest,x1,x2,y1,source,sx1,sy1,sx2,sy2);
        //_FadedLine(dest, x1, x2, y1, col1.r, col1.g, col1.b, col2.r, col2.g, col2.b);
        _PreCalcFadedTexturedLineColorKeys(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2, i_orig1, i_orig2, PreCalcPalettes, keys, keycount);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        istep1 = (i_orig2 - i_orig1) / (y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            //_TexturedLine(dest, xa>>16, xb>>16, y, source, srcx1>>16, srcy1>>16, srcx2>>16, srcy2>>16);
            //_FadedLine(dest, xa>>16, xb>>16, y, r1>>16, g1>>16, b1>>16, r2>>16, g2>>16, b2>>16);
            _PreCalcFadedTexturedLineColorKeys(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16, i1,
                                               i2, PreCalcPalettes, keys, keycount);

            xa += m1;
            xb += m2;

            i1 += istep1;

            i2 += istep2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
        }
    }

    /* Lower half of the triangle */
    if(y2 == y3)
        //_TexturedLine(dest,x2,x3,y2,source,sx2,sy2,sx3,sy3);
        //_FadedLine(dest, x2, x3, y2, col2.r, col2.g, col2.b, col3.r, col3.g, col3.b);
        _PreCalcFadedTexturedLineColorKeys(dest, x2, x3, y2, source, sx2, sy2, sx3, sy3, i_orig2, i_orig3, PreCalcPalettes, keys, keycount);
    else
    {
        m3 = Sint32((x3 - x2) << 16) / Sint32(y3 - y2);

        istep3 = (i_orig3 - i_orig2) / (y3 - y2);

        xstep3 = Sint32((sx3 - sx2) << 16) / Sint32(y3 - y2);
        ystep3 = Sint32((sy3 - sy2) << 16) / Sint32(y3 - y2);

        for(y = y2 + 1; y <= y3; y++)
        {
            //_TexturedLine(dest, xb>>16, xc>>16, y, source, srcx2>>16, srcy2>>16, srcx3>>16, srcy3>>16);
            //_FadedLine(dest, xb>>16, xc>>16, y, r2>>16, g2>>16, b2>>16, r3>>16, g3>>16, b3>>16);
            _PreCalcFadedTexturedLineColorKeys(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16, i2,
                                               i3, PreCalcPalettes, keys, keycount);

            xb += m2;
            xc += m3;

            i2 += istep2;

            i3 += istep3;

            srcx2 += xstep2;
            srcx3 += xstep3;
            srcy2 += ystep2;
            srcy3 += ystep3;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);
    if(SDL_MUSTLOCK(source) && _sge_lock)
        SDL_UnlockSurface(source);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;

    sge_UpdateRect(dest, xmin, y1, xmax - xmin + 1, y3 - y1 + 1);
}

//==================================================================================
// Draws a texured *RECTANGLE*
//==================================================================================
void sge_TexturedRect(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Sint16 x4, Sint16 y4,
                      SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3, Sint16 sx4, Sint16 sy4)
{
    Sint16 y;

    if(y1 == y3 || y1 == y4 || y4 == y2)
        return;

    /* Sort the coords */
    if(y1 > y2)
    {
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }
    if(y2 > y3)
    {
        SWAP(x3, x2, y);
        SWAP(y3, y2, y);
        SWAP(sx3, sx2, y);
        SWAP(sy3, sy2, y);
    }
    if(y1 > y2)
    {
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }
    if(y3 > y4)
    {
        SWAP(x3, x4, y);
        SWAP(y3, y4, y);
        SWAP(sx3, sx4, y);
        SWAP(sy3, sy4, y);
    }
    if(y2 > y3)
    {
        SWAP(x3, x2, y);
        SWAP(y3, y2, y);
        SWAP(sx3, sx2, y);
        SWAP(sy3, sy2, y);
    }
    if(y1 > y2)
    {
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }

    /*
     * We do this exactly like sge_TexturedTrigon(), but here we must trace four lines.
     */

    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);
    auto xd = Sint32(x3 << 16);

    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = Sint32((x4 - x2) << 16) / Sint32(y4 - y2);
    Sint32 m4 = 0;

    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);
    auto srcx4 = Sint32(sx3 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);
    auto srcy4 = Sint32(sy3 << 16);

    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = Sint32((sx4 - sx2) << 16) / Sint32(y4 - y2);
    Sint32 xstep4 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = Sint32((sy4 - sy2) << 16) / Sint32(y4 - y2);
    Sint32 ystep4 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    /* Upper bit of the rectangle */
    if(y1 == y2)
        _TexturedLine(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            _TexturedLine(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16);

            xa += m1;
            xb += m2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
        }
    }

    /* Middle bit of the rectangle */
    for(y = y2 + 1; y <= y3; y++)
    {
        _TexturedLine(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16);

        xb += m2;
        xc += m3;

        srcx2 += xstep2;
        srcx3 += xstep3;
        srcy2 += ystep2;
        srcy3 += ystep3;
    }

    /* Lower bit of the rectangle */
    if(y3 == y4)
        _TexturedLine(dest, x3, x4, y3, source, sx3, sy3, sx4, sy4);
    else
    {
        m4 = Sint32((x4 - x3) << 16) / Sint32(y4 - y3);

        xstep4 = Sint32((sx4 - sx3) << 16) / Sint32(y4 - y3);
        ystep4 = Sint32((sy4 - sy3) << 16) / Sint32(y4 - y3);

        for(y = y3 + 1; y <= y4; y++)
        {
            _TexturedLine(dest, xc >> 16, xd >> 16, y, source, srcx3 >> 16, srcy3 >> 16, srcx4 >> 16, srcy4 >> 16);

            xc += m3;
            xd += m4;

            srcx3 += xstep3;
            srcx4 += xstep4;
            srcy3 += ystep3;
            srcy4 += ystep4;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;
    xmax = (xmax > x4) ? xmax : x4;
    xmin = (xmin < x4) ? xmin : x4;

    sge_UpdateRect(dest, xmin, y1, xmax - xmin + 1, y4 - y1 + 1);
}

//==================================================================================
// Draws a gouraud shaded and texured *RECTANGLE* (shaded only from left to right) - respecting the colorkey
//==================================================================================
void sge_FadedTexturedRect(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Sint16 x4, Sint16 y4,
                           SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3, Sint16 sx4,
                           Sint16 sy4, Sint32 I1, Sint32 I2)
{
    Sint16 y;
    Sint32 i, i_orig1 = I1, i_orig2 = I2, i_orig3 = I1, i_orig4 = I2;

    if(y1 == y3 || y1 == y4 || y4 == y2)
        return;

    /* Sort the coords */
    if(y1 > y2)
    {
        SWAP(i_orig1, i_orig2, i);
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }
    if(y2 > y3)
    {
        SWAP(i_orig3, i_orig2, i);
        SWAP(x3, x2, y);
        SWAP(y3, y2, y);
        SWAP(sx3, sx2, y);
        SWAP(sy3, sy2, y);
    }
    if(y1 > y2)
    {
        SWAP(i_orig1, i_orig2, i);
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }
    if(y3 > y4)
    {
        SWAP(i_orig3, i_orig4, i);
        SWAP(x3, x4, y);
        SWAP(y3, y4, y);
        SWAP(sx3, sx4, y);
        SWAP(sy3, sy4, y);
    }
    if(y2 > y3)
    {
        SWAP(i_orig3, i_orig2, i);
        SWAP(x3, x2, y);
        SWAP(y3, y2, y);
        SWAP(sx3, sx2, y);
        SWAP(sy3, sy2, y);
    }
    if(y1 > y2)
    {
        SWAP(i_orig1, i_orig2, i);
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }

    /*
     * We do this exactly like sge_TexturedTrigon(), but here we must trace four lines.
     */

    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);
    auto xd = Sint32(x3 << 16);

    /* Starting colors (rgb) for the three lines */
    Sint32 i1 = i_orig1;
    Sint32 i2 = i1;
    Sint32 i3 = i_orig3;
    Sint32 i4 = i_orig2;

    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = Sint32((x4 - x2) << 16) / Sint32(y4 - y2);
    Sint32 m4 = 0;

    /* Colors step values */
    Sint32 istep1 = (i_orig3 - i1) / Sint32(y3 - y1);
    Sint32 istep2 = 0;
    Sint32 istep3 = 0;
    Sint32 istep4 = (i_orig4 - i2) / Sint32(y4 - y2);

    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);
    auto srcx4 = Sint32(sx3 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);
    auto srcy4 = Sint32(sy3 << 16);

    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = Sint32((sx4 - sx2) << 16) / Sint32(y4 - y2);
    Sint32 xstep4 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = Sint32((sy4 - sy2) << 16) / Sint32(y4 - y2);
    Sint32 ystep4 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    /* Upper bit of the rectangle */
    if(y1 == y2)
        _FadedTexturedLineColorKey(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2, i1, i2);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);
        istep2 = (i_orig2 - i1) / Sint32(y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            _FadedTexturedLineColorKey(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16, i1, i2);

            xa += m1;
            xb += m2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
            i1 += istep1;
            i2 += istep2;
        }
    }

    /* Middle bit of the rectangle */
    for(y = y2 + 1; y <= y3; y++)
    {
        _FadedTexturedLineColorKey(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16, i1, i4);

        xb += m2;
        xc += m3;

        srcx2 += xstep2;
        srcx3 += xstep3;
        srcy2 += ystep2;
        srcy3 += ystep3;
        i1 += istep1;
        i4 += istep4;
    }

    /* Lower bit of the rectangle */
    if(y3 == y4)
        _FadedTexturedLineColorKey(dest, x3, x4, y3, source, sx3, sy3, sx4, sy4, i3, i4);
    else
    {
        m4 = Sint32((x4 - x3) << 16) / Sint32(y4 - y3);

        xstep4 = Sint32((sx4 - sx3) << 16) / Sint32(y4 - y3);
        ystep4 = Sint32((sy4 - sy3) << 16) / Sint32(y4 - y3);
        istep3 = (i_orig4 - i3) / Sint32(y4 - y3);

        for(y = y3 + 1; y <= y4; y++)
        {
            _FadedTexturedLineColorKey(dest, xc >> 16, xd >> 16, y, source, srcx3 >> 16, srcy3 >> 16, srcx4 >> 16, srcy4 >> 16, i3, i4);

            xc += m3;
            xd += m4;

            srcx3 += xstep3;
            srcx4 += xstep4;
            srcy3 += ystep3;
            srcy4 += ystep4;
            i3 += istep3;
            i4 += istep4;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;
    xmax = (xmax > x4) ? xmax : x4;
    xmin = (xmin < x4) ? xmin : x4;

    sge_UpdateRect(dest, xmin, y1, xmax - xmin + 1, y4 - y1 + 1);
}

//==================================================================================
// Draws a texured *RECTANGLE* with precalculated gouraud shading (shaded only from left to right) - respecting the colorkey
//==================================================================================
void sge_PreCalcFadedTexturedRect(SDL_Surface* dest, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Sint16 x4, Sint16 y4,
                                  SDL_Surface* source, Sint16 sx1, Sint16 sy1, Sint16 sx2, Sint16 sy2, Sint16 sx3, Sint16 sy3, Sint16 sx4,
                                  Sint16 sy4, Uint16 I1, Uint16 I2, Uint8 PreCalcPalettes[][256])
{
    Sint16 y;
    Uint16 i, i_orig1 = I1, i_orig2 = I2, i_orig3 = I1, i_orig4 = I2;

    if(y1 == y3 || y1 == y4 || y4 == y2)
        return;

    /* Sort the coords */
    if(y1 > y2)
    {
        SWAP(i_orig1, i_orig2, i);
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }
    if(y2 > y3)
    {
        SWAP(i_orig3, i_orig2, i);
        SWAP(x3, x2, y);
        SWAP(y3, y2, y);
        SWAP(sx3, sx2, y);
        SWAP(sy3, sy2, y);
    }
    if(y1 > y2)
    {
        SWAP(i_orig1, i_orig2, i);
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }
    if(y3 > y4)
    {
        SWAP(i_orig3, i_orig4, i);
        SWAP(x3, x4, y);
        SWAP(y3, y4, y);
        SWAP(sx3, sx4, y);
        SWAP(sy3, sy4, y);
    }
    if(y2 > y3)
    {
        SWAP(i_orig3, i_orig2, i);
        SWAP(x3, x2, y);
        SWAP(y3, y2, y);
        SWAP(sx3, sx2, y);
        SWAP(sy3, sy2, y);
    }
    if(y1 > y2)
    {
        SWAP(i_orig1, i_orig2, i);
        SWAP(x1, x2, y);
        SWAP(y1, y2, y);
        SWAP(sx1, sx2, y);
        SWAP(sy1, sy2, y);
    }

    /*
     * We do this exactly like sge_TexturedTrigon(), but here we must trace four lines.
     */

    auto xa = Sint32(x1 << 16);
    Sint32 xb = xa;
    auto xc = Sint32(x2 << 16);
    auto xd = Sint32(x3 << 16);

    /* Starting colors (rgb) for the three lines */
    Uint16 i1 = i_orig1;
    Uint16 i2 = i1;
    Uint16 i3 = i_orig3;
    Uint16 i4 = i_orig2;

    Sint32 m1 = 0;
    Sint32 m2 = Sint32((x3 - x1) << 16) / Sint32(y3 - y1);
    Sint32 m3 = Sint32((x4 - x2) << 16) / Sint32(y4 - y2);
    Sint32 m4 = 0;

    /* Colors step values */
    Sint16 istep1 = (i_orig3 - i1) / (y3 - y1);
    Sint16 istep2 = 0;
    Sint16 istep3 = 0;
    Sint16 istep4 = (i_orig4 - i2) / (y4 - y2);

    auto srcx1 = Sint32(sx1 << 16);
    Sint32 srcx2 = srcx1;
    auto srcx3 = Sint32(sx2 << 16);
    auto srcx4 = Sint32(sx3 << 16);

    auto srcy1 = Sint32(sy1 << 16);
    Sint32 srcy2 = srcy1;
    auto srcy3 = Sint32(sy2 << 16);
    auto srcy4 = Sint32(sy3 << 16);

    Sint32 xstep1 = 0;
    Sint32 xstep2 = Sint32((sx3 - sx1) << 16) / Sint32(y3 - y1);
    Sint32 xstep3 = Sint32((sx4 - sx2) << 16) / Sint32(y4 - y2);
    Sint32 xstep4 = 0;

    Sint32 ystep1 = 0;
    Sint32 ystep2 = Sint32((sy3 - sy1) << 16) / Sint32(y3 - y1);
    Sint32 ystep3 = Sint32((sy4 - sy2) << 16) / Sint32(y4 - y2);
    Sint32 ystep4 = 0;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return;

    /* Upper bit of the rectangle */
    if(y1 == y2)
        _PreCalcFadedTexturedLineColorKey(dest, x1, x2, y1, source, sx1, sy1, sx2, sy2, i1, i2, PreCalcPalettes);
    else
    {
        m1 = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

        xstep1 = Sint32((sx2 - sx1) << 16) / Sint32(y2 - y1);
        ystep1 = Sint32((sy2 - sy1) << 16) / Sint32(y2 - y1);
        istep2 = (i_orig2 - i1) / (y2 - y1);

        for(y = y1; y <= y2; y++)
        {
            _PreCalcFadedTexturedLineColorKey(dest, xa >> 16, xb >> 16, y, source, srcx1 >> 16, srcy1 >> 16, srcx2 >> 16, srcy2 >> 16, i1,
                                              i2, PreCalcPalettes);

            xa += m1;
            xb += m2;

            srcx1 += xstep1;
            srcx2 += xstep2;
            srcy1 += ystep1;
            srcy2 += ystep2;
            i1 += istep1;
            i2 += istep2;
        }
    }

    /* Middle bit of the rectangle */
    for(y = y2 + 1; y <= y3; y++)
    {
        _PreCalcFadedTexturedLineColorKey(dest, xb >> 16, xc >> 16, y, source, srcx2 >> 16, srcy2 >> 16, srcx3 >> 16, srcy3 >> 16, i1, i4,
                                          PreCalcPalettes);

        xb += m2;
        xc += m3;

        srcx2 += xstep2;
        srcx3 += xstep3;
        srcy2 += ystep2;
        srcy3 += ystep3;
        i1 += istep1;
        i4 += istep4;
    }

    /* Lower bit of the rectangle */
    if(y3 == y4)
        _PreCalcFadedTexturedLineColorKey(dest, x3, x4, y3, source, sx3, sy3, sx4, sy4, i3, i4, PreCalcPalettes);
    else
    {
        m4 = Sint32((x4 - x3) << 16) / Sint32(y4 - y3);

        xstep4 = Sint32((sx4 - sx3) << 16) / Sint32(y4 - y3);
        ystep4 = Sint32((sy4 - sy3) << 16) / Sint32(y4 - y3);
        istep3 = (i_orig4 - i3) / (y4 - y3);

        for(y = y3 + 1; y <= y4; y++)
        {
            _PreCalcFadedTexturedLineColorKey(dest, xc >> 16, xd >> 16, y, source, srcx3 >> 16, srcy3 >> 16, srcx4 >> 16, srcy4 >> 16, i3,
                                              i4, PreCalcPalettes);

            xc += m3;
            xd += m4;

            srcx3 += xstep3;
            srcx4 += xstep4;
            srcy3 += ystep3;
            srcy4 += ystep4;
            i3 += istep3;
            i4 += istep4;
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    if(_sge_update != 1)
    {
        return;
    }

    Sint16 xmax = x1, xmin = x1;
    xmax = (xmax > x2) ? xmax : x2;
    xmin = (xmin < x2) ? xmin : x2;
    xmax = (xmax > x3) ? xmax : x3;
    xmin = (xmin < x3) ? xmin : x3;
    xmax = (xmax > x4) ? xmax : x4;
    xmin = (xmin < x4) ? xmin : x4;

    sge_UpdateRect(dest, xmin, y1, xmax - xmin + 1, y4 - y1 + 1);
}

//==================================================================================
// And now to something completly different: Polygons!
//==================================================================================

/* Base polygon structure */
class pline
{
public:
    virtual ~pline() = default;
    pline* next;

    Sint16 x1, x2, y1, y2;

    Sint32 fx, fm;

    Sint16 x;

    virtual void update()
    {
        x = Sint16(fx >> 16);
        fx += fm;
    }
};

/* Pointer storage (to preserve polymorphism) */
struct pline_p
{
    pline* p;
};

/* Radix sort */
static pline* rsort(pline* inlist)
{
    if(!inlist)
        return nullptr;

    // 16 radix-buckets
    std::array<pline*, 16> bucket = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    std::array<pline*, 16> bi; // bucket itterator (points to last element in bucket)

    pline* plist = inlist;

    int i, k;
    pline* j;
    Uint8 nr;

    // Radix sort in 4 steps (16-bit numbers)
    for(i = 0; i < 4; i++)
    {
        for(j = plist; j; j = j->next)
        {
            nr = Uint8((j->x >> (4 * i)) & 0x000F); // Get bucket number

            if(!bucket[nr])
                bucket[nr] = j; // First in bucket
            else
                bi[nr]->next = j; // Put last in bucket

            bi[nr] = j; // Update bucket itterator
        }

        // Empty buckets (recombine list)
        j = nullptr;
        for(k = 0; k < 16; k++)
        {
            if(bucket[k])
            {
                if(j)
                    j->next = bucket[k]; // Connect elements in buckets
                else
                    plist = bucket[k]; // First element

                j = bi[k];
            }
            bucket[k] = nullptr; // Empty
        }
        j->next = nullptr; // Terminate list
    }

    return plist;
}

/* Calculate the scanline for y */
static pline* get_scanline(pline_p* plist, Uint16 n, Sint32 y)
{
    pline* p = nullptr;
    pline* list = nullptr;
    pline* li = nullptr;

    for(int i = 0; i < n; i++)
    {
        // Is polyline on this scanline?
        p = plist[i].p;
        if(p->y1 <= y && p->y2 >= y && (p->y1 != p->y2))
        {
            if(list)
                li->next = p; // Add last in list
            else
                list = p; // Add first in list

            li = p; // Update itterator

            // Calculate x
            p->update();
        }
    }

    if(li)
        li->next = nullptr; // terminate

    // Sort list
    return rsort(list);
}

/* Removes duplicates if needed */
inline void remove_dup(pline* li, Sint16 y)
{
    if(li->next)
        if((y == li->y1 || y == li->y2) && (y == li->next->y1 || y == li->next->y2))
            if(((y == li->y1) ? -1 : 1) != ((y == li->next->y1) ? -1 : 1))
                li->next = li->next->next;
}

//==================================================================================
// Draws a n-points filled polygon
//==================================================================================

int sge_FilledPolygonAlpha(SDL_Surface* dest, Uint16 n, const Sint16* x, const Sint16* y, Uint32 color, Uint8 alpha)
{
    if(n < 3)
        return -1;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return -2;

    auto* line = new pline[n];
    auto* plist = new pline_p[n];

    Sint16 y1, y2, x1, x2, tmp, sy;
    Sint16 ymin = y[1], ymax = y[1];
    Sint16 xmin = x[1], xmax = x[1];
    Uint16 i;

    /* Decompose polygon into straight lines */
    for(i = 0; i < n; i++)
    {
        y1 = y[i];
        x1 = x[i];

        if(i == n - 1)
        {
            // Last point == First point
            y2 = y[0];
            x2 = x[0];
        } else
        {
            y2 = y[i + 1];
            x2 = x[i + 1];
        }

        // Make sure y1 <= y2
        if(y1 > y2)
        {
            SWAP(y1, y2, tmp);
            SWAP(x1, x2, tmp);
        }

        // Reject polygons with negative coords
        if(y1 < 0 || x1 < 0 || x2 < 0)
        {
            if(SDL_MUSTLOCK(dest) && _sge_lock)
                SDL_UnlockSurface(dest);

            delete[] line;
            delete[] plist;
            return -1;
        }

        if(y1 < ymin)
            ymin = y1;
        if(y2 > ymax)
            ymax = y2;
        if(x1 < xmin)
            xmin = x1;
        else if(x1 > xmax)
            xmax = x1;
        if(x2 < xmin)
            xmin = x2;
        else if(x2 > xmax)
            xmax = x2;

        // Fill structure
        line[i].y1 = y1;
        line[i].y2 = y2;
        line[i].x1 = x1;
        line[i].x2 = x2;

        // Start x-value (fixed point)
        line[i].fx = Sint32(x1 << 16);

        // Lines step value (fixed point)
        if(y1 != y2)
            line[i].fm = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);
        else
            line[i].fm = 0;

        line[i].next = nullptr;

        // Add to list
        plist[i].p = &line[i];

        // Draw the polygon outline (looks nicer)
        if(alpha == SDL_ALPHA_OPAQUE)
            _Line(dest, x1, y1, x2, y2, color); // Can't do this with alpha, might overlap with the filling
    }

    /* Remove surface lock if _HLine() is to be used */
    if(SDL_MUSTLOCK(dest) && _sge_lock && alpha == SDL_ALPHA_OPAQUE)
        SDL_UnlockSurface(dest);

    pline* list = nullptr;
    pline* li = nullptr; // list itterator

    // Scan y-lines
    for(sy = ymin; sy <= ymax; sy++)
    {
        list = get_scanline(plist, n, sy);

        if(!list)
            continue; // nothing in list... hmmmm

        x1 = x2 = -1;

        // Draw horizontal lines between pairs
        for(li = list; li; li = li->next)
        {
            remove_dup(li, sy);

            if(x1 < 0)
                x1 = li->x + 1;
            else if(x2 < 0)
                x2 = li->x;

            if(x1 >= 0 && x2 >= 0)
            {
                if(x2 - x1 < 0 && alpha == SDL_ALPHA_OPAQUE)
                {
                    // Already drawn by the outline
                    x1 = x2 = -1;
                    continue;
                }

                if(alpha == SDL_ALPHA_OPAQUE)
                    _HLine(dest, x1, x2, sy, color);
                else
                    _HLineAlpha(dest, x1 - 1, x2, sy, color, alpha);

                x1 = x2 = -1;
            }
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock && alpha != SDL_ALPHA_OPAQUE)
        SDL_UnlockSurface(dest);

    delete[] line;
    delete[] plist;

    if(_sge_update != 1)
    {
        return 0;
    }

    sge_UpdateRect(dest, xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);

    return 0;
}

int sge_FilledPolygonAlpha(SDL_Surface* dest, Uint16 n, Sint16* x, Sint16* y, Uint8 r, Uint8 g, Uint8 b, Uint8 alpha)
{
    return sge_FilledPolygonAlpha(dest, n, x, y, SDL_MapRGB(dest->format, r, g, b), alpha);
}

int sge_FilledPolygon(SDL_Surface* dest, Uint16 n, Sint16* x, Sint16* y, Uint32 color)
{
    return sge_FilledPolygonAlpha(dest, n, x, y, color, SDL_ALPHA_OPAQUE);
}

int sge_FilledPolygon(SDL_Surface* dest, Uint16 n, Sint16* x, Sint16* y, Uint8 r, Uint8 g, Uint8 b)
{
    return sge_FilledPolygonAlpha(dest, n, x, y, SDL_MapRGB(dest->format, r, g, b), SDL_ALPHA_OPAQUE);
}

//==================================================================================
// Draws a n-points (AA) filled polygon
//==================================================================================

int sge_AAFilledPolygon(SDL_Surface* dest, Uint16 n, const Sint16* x, const Sint16* y, Uint32 color)
{
    if(n < 3)
        return -1;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return -2;

    auto* line = new pline[n];
    auto* plist = new pline_p[n];

    Sint16 y1, y2, x1, x2, tmp, sy;
    Sint16 ymin = y[1], ymax = y[1];
    Sint16 xmin = x[1], xmax = x[1];
    Uint16 i;

    /* Decompose polygon into straight lines */
    for(i = 0; i < n; i++)
    {
        y1 = y[i];
        x1 = x[i];

        if(i == n - 1)
        {
            // Last point == First point
            y2 = y[0];
            x2 = x[0];
        } else
        {
            y2 = y[i + 1];
            x2 = x[i + 1];
        }

        // Make sure y1 <= y2
        if(y1 > y2)
        {
            SWAP(y1, y2, tmp);
            SWAP(x1, x2, tmp);
        }

        // Reject polygons with negative coords
        if(y1 < 0 || x1 < 0 || x2 < 0)
        {
            if(SDL_MUSTLOCK(dest) && _sge_lock)
                SDL_UnlockSurface(dest);

            delete[] line;
            delete[] plist;
            return -1;
        }

        if(y1 < ymin)
            ymin = y1;
        if(y2 > ymax)
            ymax = y2;
        if(x1 < xmin)
            xmin = x1;
        else if(x1 > xmax)
            xmax = x1;
        if(x2 < xmin)
            xmin = x2;
        else if(x2 > xmax)
            xmax = x2;

        // Fill structure
        line[i].y1 = y1;
        line[i].y2 = y2;
        line[i].x1 = x1;
        line[i].x2 = x2;

        // Start x-value (fixed point)
        line[i].fx = Sint32(x1 << 16);

        // Lines step value (fixed point)
        if(y1 != y2)
            line[i].fm = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);
        else
            line[i].fm = 0;

        line[i].next = nullptr;

        // Add to list
        plist[i].p = &line[i];

        // Draw AA Line
        _AALineAlpha(dest, x1, y1, x2, y2, color, SDL_ALPHA_OPAQUE);
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    pline* list = nullptr;
    pline* li = nullptr; // list itterator

    // Scan y-lines
    for(sy = ymin; sy <= ymax; sy++)
    {
        list = get_scanline(plist, n, sy);

        if(!list)
            continue; // nothing in list... hmmmm

        x1 = x2 = -1;

        // Draw horizontal lines between pairs
        for(li = list; li; li = li->next)
        {
            remove_dup(li, sy);

            if(x1 < 0)
                x1 = li->x + 1;
            else if(x2 < 0)
                x2 = li->x;

            if(x1 >= 0 && x2 >= 0)
            {
                if(x2 - x1 < 0)
                {
                    x1 = x2 = -1;
                    continue;
                }

                _HLine(dest, x1, x2, sy, color);

                x1 = x2 = -1;
            }
        }
    }

    delete[] line;
    delete[] plist;

    if(_sge_update != 1)
    {
        return 0;
    }

    sge_UpdateRect(dest, xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);

    return 0;
}

int sge_AAFilledPolygon(SDL_Surface* dest, Uint16 n, Sint16* x, Sint16* y, Uint8 r, Uint8 g, Uint8 b)
{
    return sge_AAFilledPolygon(dest, n, x, y, SDL_MapRGB(dest->format, r, g, b));
}

//==================================================================================
// Draws a n-points gourand shaded polygon
//==================================================================================

/* faded polygon structure */
class fpline : public pline
{
public:
    Uint8 r1, r2;
    Uint8 g1, g2;
    Uint8 b1, b2;

    Uint32 fr, fg, fb;
    Sint32 fmr, fmg, fmb;

    Uint8 r, g, b;

    void update() override
    {
        x = Sint16(fx >> 16);
        fx += fm;

        r = Uint8(fr >> 16);
        g = Uint8(fg >> 16);
        b = Uint8(fb >> 16);

        fr += fmr;
        fg += fmg;
        fb += fmb;
    }
};

int sge_FadedPolygonAlpha(SDL_Surface* dest, Uint16 n, const Sint16* x, const Sint16* y, const Uint8* R, const Uint8* G, const Uint8* B,
                          Uint8 alpha)
{
    if(n < 3)
        return -1;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return -2;

    auto* line = new fpline[n];
    auto* plist = new pline_p[n];

    Sint16 y1, y2, x1, x2, tmp, sy;
    Sint16 ymin = y[1], ymax = y[1];
    Sint16 xmin = x[1], xmax = x[1];
    Uint16 i;
    Uint8 r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0, t;

    // Decompose polygon into straight lines
    for(i = 0; i < n; i++)
    {
        y1 = y[i];
        x1 = x[i];
        r1 = R[i];
        g1 = G[i];
        b1 = B[i];

        if(i == n - 1)
        {
            // Last point == First point
            y2 = y[0];
            x2 = x[0];
            r2 = R[0];
            g2 = G[0];
            b2 = B[0];
        } else
        {
            y2 = y[i + 1];
            x2 = x[i + 1];
            r2 = R[i + 1];
            g2 = G[i + 1];
            b2 = B[i + 1];
        }

        // Make sure y1 <= y2
        if(y1 > y2)
        {
            SWAP(y1, y2, tmp);
            SWAP(x1, x2, tmp);
            SWAP(r1, r2, t);
            SWAP(g1, g2, t);
            SWAP(b1, b2, t);
        }

        // Reject polygons with negative coords
        if(y1 < 0 || x1 < 0 || x2 < 0)
        {
            if(SDL_MUSTLOCK(dest) && _sge_lock)
                SDL_UnlockSurface(dest);

            delete[] line;
            delete[] plist;
            return -1;
        }

        if(y1 < ymin)
            ymin = y1;
        if(y2 > ymax)
            ymax = y2;
        if(x1 < xmin)
            xmin = x1;
        else if(x1 > xmax)
            xmax = x1;
        if(x2 < xmin)
            xmin = x2;
        else if(x2 > xmax)
            xmax = x2;

        // Fill structure
        line[i].y1 = y1;
        line[i].y2 = y2;
        line[i].x1 = x1;
        line[i].x2 = x2;
        line[i].r1 = r1;
        line[i].g1 = g1;
        line[i].b1 = b1;
        line[i].r2 = r2;
        line[i].g2 = g2;
        line[i].b2 = b2;

        // Start x-value (fixed point)
        line[i].fx = Sint32(x1 << 16);

        line[i].fr = Uint32(r1 << 16);
        line[i].fg = Uint32(g1 << 16);
        line[i].fb = Uint32(b1 << 16);

        // Lines step value (fixed point)
        if(y1 != y2)
        {
            line[i].fm = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

            line[i].fmr = Sint32((r2 - r1) << 16) / Sint32(y2 - y1);
            line[i].fmg = Sint32((g2 - g1) << 16) / Sint32(y2 - y1);
            line[i].fmb = Sint32((b2 - b1) << 16) / Sint32(y2 - y1);
        } else
        {
            line[i].fm = 0;
            line[i].fmr = 0;
            line[i].fmg = 0;
            line[i].fmb = 0;
        }

        line[i].next = nullptr;

        // Add to list
        plist[i].p = &line[i];

        // Draw the polygon outline (looks nicer)
        if(alpha == SDL_ALPHA_OPAQUE)
            sge_DomcLine(dest, x1, y1, x2, y2, r1, g1, b1, r2, g2, b2,
                         _PutPixel); // Can't do this with alpha, might overlap with the filling
    }

    fpline* list = nullptr;
    fpline* li = nullptr; // list itterator

    // Scan y-lines
    for(sy = ymin; sy <= ymax; sy++)
    {
        list = (fpline*)get_scanline(plist, n, sy);

        if(!list)
            continue; // nothing in list... hmmmm

        x1 = x2 = -1;

        // Draw horizontal lines between pairs
        for(li = list; li; li = (fpline*)li->next)
        {
            remove_dup(li, sy);

            if(x1 < 0)
            {
                x1 = li->x + 1;
                r1 = li->r;
                g1 = li->g;
                b1 = li->b;
            } else if(x2 < 0)
            {
                x2 = li->x;
                r2 = li->r;
                g2 = li->g;
                b2 = li->b;
            }

            if(x1 >= 0 && x2 >= 0)
            {
                if(x2 - x1 < 0 && alpha == SDL_ALPHA_OPAQUE)
                {
                    x1 = x2 = -1;
                    continue;
                }

                if(alpha == SDL_ALPHA_OPAQUE)
                    _FadedLine(dest, x1, x2, sy, r1, g1, b1, r2, g2, b2);
                else
                {
                    _sge_alpha_hack = alpha;
                    sge_DomcLine(dest, x1 - 1, sy, x2, sy, r1, g1, b1, r2, g2, b2, callback_alpha_hack);
                }

                x1 = x2 = -1;
            }
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    delete[] line;
    delete[] plist;

    if(_sge_update != 1)
    {
        return 0;
    }

    sge_UpdateRect(dest, xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);

    return 0;
}

int sge_FadedPolygon(SDL_Surface* dest, Uint16 n, Sint16* x, Sint16* y, Uint8* R, Uint8* G, Uint8* B)
{
    return sge_FadedPolygonAlpha(dest, n, x, y, R, G, B, SDL_ALPHA_OPAQUE);
}

//==================================================================================
// Draws a n-points (AA) gourand shaded polygon
//==================================================================================
int sge_AAFadedPolygon(SDL_Surface* dest, Uint16 n, const Sint16* x, const Sint16* y, const Uint8* R, const Uint8* G, const Uint8* B)
{
    if(n < 3)
        return -1;

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        if(SDL_LockSurface(dest) < 0)
            return -2;

    auto* line = new fpline[n];
    auto* plist = new pline_p[n];

    Sint16 y1, y2, x1, x2, tmp, sy;
    Sint16 ymin = y[1], ymax = y[1];
    Sint16 xmin = x[1], xmax = x[1];
    Uint16 i;
    Uint8 r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0, t;

    // Decompose polygon into straight lines
    for(i = 0; i < n; i++)
    {
        y1 = y[i];
        x1 = x[i];
        r1 = R[i];
        g1 = G[i];
        b1 = B[i];

        if(i == n - 1)
        {
            // Last point == First point
            y2 = y[0];
            x2 = x[0];
            r2 = R[0];
            g2 = G[0];
            b2 = B[0];
        } else
        {
            y2 = y[i + 1];
            x2 = x[i + 1];
            r2 = R[i + 1];
            g2 = G[i + 1];
            b2 = B[i + 1];
        }

        // Make sure y1 <= y2
        if(y1 > y2)
        {
            SWAP(y1, y2, tmp);
            SWAP(x1, x2, tmp);
            SWAP(r1, r2, t);
            SWAP(g1, g2, t);
            SWAP(b1, b2, t);
        }

        // Reject polygons with negative coords
        if(y1 < 0 || x1 < 0 || x2 < 0)
        {
            if(SDL_MUSTLOCK(dest) && _sge_lock)
                SDL_UnlockSurface(dest);

            delete[] line;
            delete[] plist;
            return -1;
        }

        if(y1 < ymin)
            ymin = y1;
        if(y2 > ymax)
            ymax = y2;
        if(x1 < xmin)
            xmin = x1;
        else if(x1 > xmax)
            xmax = x1;
        if(x2 < xmin)
            xmin = x2;
        else if(x2 > xmax)
            xmax = x2;

        // Fill structure
        line[i].y1 = y1;
        line[i].y2 = y2;
        line[i].x1 = x1;
        line[i].x2 = x2;
        line[i].r1 = r1;
        line[i].g1 = g1;
        line[i].b1 = b1;
        line[i].r2 = r2;
        line[i].g2 = g2;
        line[i].b2 = b2;

        // Start x-value (fixed point)
        line[i].fx = Sint32(x1 << 16);

        line[i].fr = Uint32(r1 << 16);
        line[i].fg = Uint32(g1 << 16);
        line[i].fb = Uint32(b1 << 16);

        // Lines step value (fixed point)
        if(y1 != y2)
        {
            line[i].fm = Sint32((x2 - x1) << 16) / Sint32(y2 - y1);

            line[i].fmr = Sint32((r2 - r1) << 16) / Sint32(y2 - y1);
            line[i].fmg = Sint32((g2 - g1) << 16) / Sint32(y2 - y1);
            line[i].fmb = Sint32((b2 - b1) << 16) / Sint32(y2 - y1);
        } else
        {
            line[i].fm = 0;
            line[i].fmr = 0;
            line[i].fmg = 0;
            line[i].fmb = 0;
        }

        line[i].next = nullptr;

        // Add to list
        plist[i].p = &line[i];

        // Draw the polygon outline (AA)
        _AAmcLineAlpha(dest, x1, y1, x2, y2, r1, g1, b1, r2, g2, b2, SDL_ALPHA_OPAQUE);
    }

    fpline* list = nullptr;
    fpline* li = nullptr; // list itterator

    // Scan y-lines
    for(sy = ymin; sy <= ymax; sy++)
    {
        list = (fpline*)get_scanline(plist, n, sy);

        if(!list)
            continue; // nothing in list... hmmmm

        x1 = x2 = -1;

        // Draw horizontal lines between pairs
        for(li = list; li; li = (fpline*)li->next)
        {
            remove_dup(li, sy);

            if(x1 < 0)
            {
                x1 = li->x + 1;
                r1 = li->r;
                g1 = li->g;
                b1 = li->b;
            } else if(x2 < 0)
            {
                x2 = li->x;
                r2 = li->r;
                g2 = li->g;
                b2 = li->b;
            }

            if(x1 >= 0 && x2 >= 0)
            {
                if(x2 - x1 < 0)
                {
                    x1 = x2 = -1;
                    continue;
                }

                _FadedLine(dest, x1, x2, sy, r1, g1, b1, r2, g2, b2);

                x1 = x2 = -1;
            }
        }
    }

    if(SDL_MUSTLOCK(dest) && _sge_lock)
        SDL_UnlockSurface(dest);

    delete[] line;
    delete[] plist;

    if(_sge_update != 1)
    {
        return 0;
    }

    sge_UpdateRect(dest, xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);

    return 0;
}
