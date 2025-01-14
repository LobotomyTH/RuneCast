/* 

 SDL_gfxPrimitives - Graphics primitives for SDL surfaces

 LGPL (c) A. Schiffler

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <math.h>
#include "include/math-sll.h"

#ifdef USE_FIXED_POINT
#define SLL_CONST_2PI360 0x00000000477D1A8LL
#else
#define SLL_CONST_2PI360 0.017453292519943295769236907684883
#endif

#include "include/SDL_gfxPrimitives.h"
#include "include/SDL_gfxPrimitives_font.h"

/* -===================- */

/* ----- Defines for pixel clipping tests */

#define clip_xmin(surface) surface->clip_rect.x
#define clip_xmax(surface) surface->clip_rect.x+surface->clip_rect.w-1
#define clip_ymin(surface) surface->clip_rect.y
#define clip_ymax(surface) surface->clip_rect.y+surface->clip_rect.h-1

/* ----- Pixel - fast, no blending, no locking, clipping */

int fastPixelColorNolock(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color)
{
    int bpp;
    Uint8 *p;

    /*
     * Honor clipping setup at pixel level 
     */
    if ((x >= clip_xmin(dst)) && (x <= clip_xmax(dst)) && (y >= clip_ymin(dst)) && (y <= clip_ymax(dst))) {

	/*
	 * Get destination format 
	 */
	bpp = dst->format->BytesPerPixel;
	p = (Uint8 *) dst->pixels + y * dst->pitch + x * bpp;
	switch (bpp) {
	case 1:
	    *p = color;
	    break;
	case 2:
	    *(Uint16 *) p = color;
	    break;
	case 3:
	    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		p[0] = (color >> 16) & 0xff;
		p[1] = (color >> 8) & 0xff;
		p[2] = color & 0xff;
	    } else {
		p[0] = color & 0xff;
		p[1] = (color >> 8) & 0xff;
		p[2] = (color >> 16) & 0xff;
	    }
	    break;
	case 4:
	    *(Uint32 *) p = color;
	    break;
	}			/* switch */


    }

    return (0);
}

/* ----- Pixel - fast, no blending, no locking, no clipping */

/* (faster but dangerous, make sure we stay in surface bounds) */

int fastPixelColorNolockNoclip(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color)
{
    int bpp;
    Uint8 *p;

    /*
     * Get destination format 
     */
    bpp = dst->format->BytesPerPixel;
    p = (Uint8 *) dst->pixels + y * dst->pitch + x * bpp;
    switch (bpp) {
    case 1:
	*p = color;
	break;
    case 2:
	*(Uint16 *) p = color;
	break;
    case 3:
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    p[0] = (color >> 16) & 0xff;
	    p[1] = (color >> 8) & 0xff;
	    p[2] = color & 0xff;
	} else {
	    p[0] = color & 0xff;
	    p[1] = (color >> 8) & 0xff;
	    p[2] = (color >> 16) & 0xff;
	}
	break;
    case 4:
	*(Uint32 *) p = color;
	break;
    }				/* switch */

    return (0);
}

/* ----- Pixel - fast, no blending, locking, clipping */

int fastPixelColor(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color)
{
    int result;

    /*
     * Lock the surface 
     */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    result = fastPixelColorNolock(dst, x, y, color);

    /*
     * Unlock surface 
     */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (result);
}

/* ----- Pixel - fast, no blending, locking, RGB input */

int fastPixelRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    Uint32 color;

    /*
     * Setup color 
     */
    color = SDL_MapRGBA(dst->format, r, g, b, a);

    /*
     * Draw 
     */
    return (fastPixelColor(dst, x, y, color));

}

/* ----- Pixel - fast, no blending, no locking RGB input */

int fastPixelRGBANolock(SDL_Surface * dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    Uint32 color;

    /*
     * Setup color 
     */
    color = SDL_MapRGBA(dst->format, r, g, b, a);

    /*
     * Draw 
     */
    return (fastPixelColorNolock(dst, x, y, color));
}

/* PutPixel routine with alpha blending, input color in destination format */

/* New, faster routine - default blending pixel */

int _putPixelAlpha(SDL_Surface * surface, Sint16 x, Sint16 y, Uint32 color, Uint8 alpha)
{
    Uint32 Rmask = surface->format->Rmask, Gmask =
	surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
    Uint32 R, G, B, A = 0;

    if (x >= clip_xmin(surface) && x <= clip_xmax(surface)
	&& y >= clip_ymin(surface) && y <= clip_ymax(surface)) {

	switch (surface->format->BytesPerPixel) {
	case 1:{		/* Assuming 8-bpp */
		if (alpha == 255) {
		    *((Uint8 *) surface->pixels + y * surface->pitch + x) = color;
		} else {
		    Uint8 *pixel = (Uint8 *) surface->pixels + y * surface->pitch + x;

		    Uint8 dR = surface->format->palette->colors[*pixel].r;
		    Uint8 dG = surface->format->palette->colors[*pixel].g;
		    Uint8 dB = surface->format->palette->colors[*pixel].b;
		    Uint8 sR = surface->format->palette->colors[color].r;
		    Uint8 sG = surface->format->palette->colors[color].g;
		    Uint8 sB = surface->format->palette->colors[color].b;

		    dR = dR + ((sR - dR) * alpha >> 8);
		    dG = dG + ((sG - dG) * alpha >> 8);
		    dB = dB + ((sB - dB) * alpha >> 8);

		    *pixel = SDL_MapRGB(surface->format, dR, dG, dB);
		}
	    }
	    break;

	case 2:{		/* Probably 15-bpp or 16-bpp */
		if (alpha == 255) {
		    *((Uint16 *) surface->pixels + y * surface->pitch / 2 + x) = color;
		} else {
		    Uint16 *pixel = (Uint16 *) surface->pixels + y * surface->pitch / 2 + x;
		    Uint32 dc = *pixel;

		    R = ((dc & Rmask) + (((color & Rmask) - (dc & Rmask)) * alpha >> 8)) & Rmask;
		    G = ((dc & Gmask) + (((color & Gmask) - (dc & Gmask)) * alpha >> 8)) & Gmask;
		    B = ((dc & Bmask) + (((color & Bmask) - (dc & Bmask)) * alpha >> 8)) & Bmask;
		    if (Amask)
			A = ((dc & Amask) + (((color & Amask) - (dc & Amask)) * alpha >> 8)) & Amask;

		    *pixel = R | G | B | A;
		}
	    }
	    break;

	case 3:{		/* Slow 24-bpp mode, usually not used */
		Uint8 *pix = (Uint8 *) surface->pixels + y * surface->pitch + x * 3;
		Uint8 rshift8 = surface->format->Rshift / 8;
		Uint8 gshift8 = surface->format->Gshift / 8;
		Uint8 bshift8 = surface->format->Bshift / 8;
		Uint8 ashift8 = surface->format->Ashift / 8;


		if (alpha == 255) {
		    *(pix + rshift8) = color >> surface->format->Rshift;
		    *(pix + gshift8) = color >> surface->format->Gshift;
		    *(pix + bshift8) = color >> surface->format->Bshift;
		    *(pix + ashift8) = color >> surface->format->Ashift;
		} else {
		    Uint8 dR, dG, dB, dA = 0;
		    Uint8 sR, sG, sB, sA = 0;

		    pix = (Uint8 *) surface->pixels + y * surface->pitch + x * 3;

		    dR = *((pix) + rshift8);
		    dG = *((pix) + gshift8);
		    dB = *((pix) + bshift8);
		    dA = *((pix) + ashift8);

		    sR = (color >> surface->format->Rshift) & 0xff;
		    sG = (color >> surface->format->Gshift) & 0xff;
		    sB = (color >> surface->format->Bshift) & 0xff;
		    sA = (color >> surface->format->Ashift) & 0xff;

		    dR = dR + ((sR - dR) * alpha >> 8);
		    dG = dG + ((sG - dG) * alpha >> 8);
		    dB = dB + ((sB - dB) * alpha >> 8);
		    dA = dA + ((sA - dA) * alpha >> 8);

		    *((pix) + rshift8) = dR;
		    *((pix) + gshift8) = dG;
		    *((pix) + bshift8) = dB;
		    *((pix) + ashift8) = dA;
		}
	    }
	    break;

	case 4:{		/* Probably 32-bpp */
		if (alpha == 255) {
		    *((Uint32 *) surface->pixels + y * surface->pitch / 4 + x) = color;
		} else {
		    Uint32 *pixel = (Uint32 *) surface->pixels + y * surface->pitch / 4 + x;
		    Uint32 dc = *pixel;

		    R = ((dc & Rmask) + (((color & Rmask) - (dc & Rmask)) * alpha >> 8)) & Rmask;
		    G = ((dc & Gmask) + (((color & Gmask) - (dc & Gmask)) * alpha >> 8)) & Gmask;
		    B = ((dc & Bmask) + (((color & Bmask) - (dc & Bmask)) * alpha >> 8)) & Bmask;
		    if (Amask)
			A = ((dc & Amask) + (((color & Amask) - (dc & Amask)) * alpha >> 8)) & Amask;

		    *pixel = R | G | B | A;
		}
	    }
	    break;
	}
    }

    return (0);
}

/* ----- Pixel - pixel draw with blending enabled if a<255 */

int pixelColor(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color)
{
    Uint8 alpha;
    Uint32 mcolor;
    int result = 0;

    /*
     * Lock the surface 
     */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    /*
     * Setup color 
     */
    alpha = color & 0x000000ff;
    mcolor =
	SDL_MapRGBA(dst->format, (color & 0xff000000) >> 24,
		    (color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, alpha);

    /*
     * Draw 
     */
    result = _putPixelAlpha(dst, x, y, mcolor, alpha);

    /*
     * Unlock the surface 
     */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (result);
}

int pixelColorNolock(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color)
{
    Uint8 alpha;
    Uint32 mcolor;
    int result = 0;

    /*
     * Setup color 
     */
    alpha = color & 0x000000ff;
    mcolor =
	SDL_MapRGBA(dst->format, (color & 0xff000000) >> 24,
		    (color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, alpha);

    /*
     * Draw 
     */
    result = _putPixelAlpha(dst, x, y, mcolor, alpha);

    return (result);
}


/* Filled rectangle with alpha blending, color in destination format */

int _filledRectAlpha(SDL_Surface * surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, Uint8 alpha)
{
    Uint32 Rmask = surface->format->Rmask, Gmask =
	surface->format->Gmask, Bmask = surface->format->Bmask, Amask = surface->format->Amask;
    Uint32 R, G, B, A = 0;
    Sint16 x, y;

    switch (surface->format->BytesPerPixel) {
    case 1:{			/* Assuming 8-bpp */
	    Uint8 *row, *pixel;
	    Uint8 dR, dG, dB;

	    Uint8 sR = surface->format->palette->colors[color].r;
	    Uint8 sG = surface->format->palette->colors[color].g;
	    Uint8 sB = surface->format->palette->colors[color].b;

	    for (y = y1; y <= y2; y++) {
		row = (Uint8 *) surface->pixels + y * surface->pitch;
		for (x = x1; x <= x2; x++) {
		    pixel = row + x;

		    dR = surface->format->palette->colors[*pixel].r;
		    dG = surface->format->palette->colors[*pixel].g;
		    dB = surface->format->palette->colors[*pixel].b;

		    dR = dR + ((sR - dR) * alpha >> 8);
		    dG = dG + ((sG - dG) * alpha >> 8);
		    dB = dB + ((sB - dB) * alpha >> 8);

		    *pixel = SDL_MapRGB(surface->format, dR, dG, dB);
		}
	    }
	}
	break;

    case 2:{			/* Probably 15-bpp or 16-bpp */
	    Uint16 *row, *pixel;
	    Uint32 dR = (color & Rmask), dG = (color & Gmask), dB = (color & Bmask), dA = (color & Amask);

	    for (y = y1; y <= y2; y++) {
		row = (Uint16 *) surface->pixels + y * surface->pitch / 2;
		for (x = x1; x <= x2; x++) {
		    pixel = row + x;

		    R = ((*pixel & Rmask) + ((dR - (*pixel & Rmask)) * alpha >> 8)) & Rmask;
		    G = ((*pixel & Gmask) + ((dG - (*pixel & Gmask)) * alpha >> 8)) & Gmask;
		    B = ((*pixel & Bmask) + ((dB - (*pixel & Bmask)) * alpha >> 8)) & Bmask;
		    if (Amask)
			A = ((*pixel & Amask) + ((dA - (*pixel & Amask)) * alpha >> 8)) & Amask;

		    *pixel = R | G | B | A;
		}
	    }
	}
	break;

    case 3:{			/* Slow 24-bpp mode, usually not used */
	    Uint8 *row, *pix;
	    Uint8 dR, dG, dB, dA;
	    Uint8 rshift8 = surface->format->Rshift / 8;
	    Uint8 gshift8 = surface->format->Gshift / 8;
	    Uint8 bshift8 = surface->format->Bshift / 8;
	    Uint8 ashift8 = surface->format->Ashift / 8;

	    Uint8 sR = (color >> surface->format->Rshift) & 0xff;
	    Uint8 sG = (color >> surface->format->Gshift) & 0xff;
	    Uint8 sB = (color >> surface->format->Bshift) & 0xff;
	    Uint8 sA = (color >> surface->format->Ashift) & 0xff;

	    for (y = y1; y <= y2; y++) {
		row = (Uint8 *) surface->pixels + y * surface->pitch;
		for (x = x1; x <= x2; x++) {
		    pix = row + x * 3;

		    dR = *((pix) + rshift8);
		    dG = *((pix) + gshift8);
		    dB = *((pix) + bshift8);
		    dA = *((pix) + ashift8);

		    dR = dR + ((sR - dR) * alpha >> 8);
		    dG = dG + ((sG - dG) * alpha >> 8);
		    dB = dB + ((sB - dB) * alpha >> 8);
		    dA = dA + ((sA - dA) * alpha >> 8);

		    *((pix) + rshift8) = dR;
		    *((pix) + gshift8) = dG;
		    *((pix) + bshift8) = dB;
		    *((pix) + ashift8) = dA;
		}
	    }

	}
	break;

    case 4:{			/* Probably 32-bpp */
	    Uint32 *row, *pixel;
	    Uint32 dR = (color & Rmask), dG = (color & Gmask), dB = (color & Bmask), dA = (color & Amask);

	    for (y = y1; y <= y2; y++) {
		row = (Uint32 *) surface->pixels + y * surface->pitch / 4;
		for (x = x1; x <= x2; x++) {
		    pixel = row + x;

		    R = ((*pixel & Rmask) + ((dR - (*pixel & Rmask)) * alpha >> 8)) & Rmask;
		    G = ((*pixel & Gmask) + ((dG - (*pixel & Gmask)) * alpha >> 8)) & Gmask;
		    B = ((*pixel & Bmask) + ((dB - (*pixel & Bmask)) * alpha >> 8)) & Bmask;
		    if (Amask)
			A = ((*pixel & Amask) + ((dA - (*pixel & Amask)) * alpha >> 8)) & Amask;

		    *pixel = R | G | B | A;
		}
	    }
	}
	break;
    }

    return (0);
}

/* Draw rectangle with alpha enabled from RGBA color. */

int filledRectAlpha(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    Uint8 alpha;
    Uint32 mcolor;
    int result = 0;

    /*
     * Lock the surface 
     */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    /*
     * Setup color 
     */
    alpha = color & 0x000000ff;
    mcolor =
	SDL_MapRGBA(dst->format, (color & 0xff000000) >> 24,
		    (color & 0x00ff0000) >> 16, (color & 0x0000ff00) >> 8, alpha);

    /*
     * Draw 
     */
    result = _filledRectAlpha(dst, x1, y1, x2, y2, mcolor, alpha);

    /*
     * Unlock the surface 
     */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (result);
}

/* Draw horizontal line with alpha enabled from RGBA color */

int HLineAlpha(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color)
{
    return (filledRectAlpha(dst, x1, y, x2, y, color));
}


/* Draw vertical line with alpha enabled from RGBA color */

int VLineAlpha(SDL_Surface * dst, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color)
{
    return (filledRectAlpha(dst, x, y1, x, y2, color));
}


/* Pixel - using alpha weight on color for AA-drawing */

int pixelColorWeight(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color, Uint32 weight)
{
    Uint32 a;

    /*
     * Get alpha 
     */
    a = (color & (Uint32) 0x000000ff);

    /*
     * Modify Alpha by weight 
     */
    a = ((a * weight) >> 8);

    return (pixelColor(dst, x, y, (color & (Uint32) 0xffffff00) | (Uint32) a));
}

/* Pixel - using alpha weight on color for AA-drawing - no locking */

int pixelColorWeightNolock(SDL_Surface * dst, Sint16 x, Sint16 y, Uint32 color, Uint32 weight)
{
    Uint32 a;

    /*
     * Get alpha 
     */
    a = (color & (Uint32) 0x000000ff);

    /*
     * Modify Alpha by weight 
     */
    a = ((a * weight) >> 8);

    return (pixelColorNolock(dst, x, y, (color & (Uint32) 0xffffff00) | (Uint32) a));
}

int pixelRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    Uint32 color;

    /*
     * Check Alpha 
     */
    if (a == 255) {
	/*
	 * No alpha blending required 
	 */
	/*
	 * Setup color 
	 */
	color = SDL_MapRGBA(dst->format, r, g, b, a);
	/*
	 * Draw 
	 */
	return (fastPixelColor(dst, x, y, color));
    } else {
	/*
	 * Alpha blending required 
	 */
	/*
	 * Draw 
	 */
	return (pixelColor(dst, x, y, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
    }
}

/* ----- Horizontal line */

/* Just store color including alpha, no blending */

int hlineColorStore(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color)
{
    Sint16 left, right, top, bottom;
    Uint8 *pixel, *pixellast;
    int dx;
    int pixx, pixy;
    Sint16 w;
    Sint16 xtmp;
    int result = -1;

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Check visibility of hline 
     */
    if ((x1<left) && (x2<left)) {
     return(0);
    }
    if ((x1>right) && (x2>right)) {
     return(0);
    }
    if ((y<top) || (y>bottom)) {
     return (0);
    }

    /*
     * Clip x 
     */
    if (x1 < left) {
	x1 = left;
    }
    if (x2 > right) {
	x2 = right;
    }

    /*
     * Swap x1, x2 if required 
     */
    if (x1 > x2) {
	xtmp = x1;
	x1 = x2;
	x2 = xtmp;
    }

    /*
     * Calculate width 
     */
    w = x2 - x1;

    /*
     * Sanity check on width 
     */
    if (w < 0) {
	return (0);
    }

    /*
     * Lock surface 
     */
    SDL_LockSurface(dst);

    /*
     * More variable setup 
     */
    dx = w;
    pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x1 + pixy * (int) y;

	/*
	 * Draw 
	 */
	switch (dst->format->BytesPerPixel) {
	case 1:
	    memset(pixel, color, dx);
	    break;
	case 2:
	    pixellast = pixel + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		*(Uint16 *) pixel = color;
	    }
	    break;
	case 3:
	    pixellast = pixel + dx + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		    pixel[0] = (color >> 16) & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = color & 0xff;
		} else {
		    pixel[0] = color & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = (color >> 16) & 0xff;
		}
	    }
	    break;
	default:		/* case 4 */
	    dx = dx + dx;
	    pixellast = pixel + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		*(Uint32 *) pixel = color;
	    }
	    break;
	}

	/*
	 * Unlock surface 
	 */
	SDL_UnlockSurface(dst);

	/*
	 * Set result code 
	 */
	result = 0;

    return (result);
}

int hlineRGBAStore(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (hlineColorStore(dst, x1, x2, y, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

int hlineColor(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color)
{
    Sint16 left, right, top, bottom;
    Uint8 *pixel, *pixellast;
    int dx;
    int pixx, pixy;
    Sint16 w;
    Sint16 xtmp;
    int result = -1;
    Uint8 *colorptr;

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Check visibility of hline 
     */
    if ((x1<left) && (x2<left)) {
     return(0);
    }
    if ((x1>right) && (x2>right)) {
     return(0);
    }
    if ((y<top) || (y>bottom)) {
     return (0);
    }

    /*
     * Clip x 
     */
    if (x1 < left) {
	x1 = left;
    }
    if (x2 > right) {
	x2 = right;
    }

    /*
     * Swap x1, x2 if required 
     */
    if (x1 > x2) {
	xtmp = x1;
	x1 = x2;
	x2 = xtmp;
    }

    /*
     * Calculate width 
     */
    w = x2 - x1;

    /*
     * Sanity check on width 
     */
    if (w < 0) {
	return (0);
    }

    /*
     * Alpha check 
     */
    if ((color & 255) == 255) {

	/*
	 * No alpha-blending required 
	 */

	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}

	/*
	 * Lock surface 
	 */
	SDL_LockSurface(dst);

	/*
	 * More variable setup 
	 */
	dx = w;
	pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x1 + pixy * (int) y;

	/*
	 * Draw 
	 */
	switch (dst->format->BytesPerPixel) {
	case 1:
	    memset(pixel, color, dx);
	    break;
	case 2:
	    pixellast = pixel + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		*(Uint16 *) pixel = color;
	    }
	    break;
	case 3:
	    pixellast = pixel + dx + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		    pixel[0] = (color >> 16) & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = color & 0xff;
		} else {
		    pixel[0] = color & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = (color >> 16) & 0xff;
		}
	    }
	    break;
	default:		/* case 4 */
	    dx = dx + dx;
	    pixellast = pixel + dx + dx;
	    for (; pixel <= pixellast; pixel += pixx) {
		*(Uint32 *) pixel = color;
	    }
	    break;
	}

	/*
	 * Unlock surface 
	 */
	SDL_UnlockSurface(dst);

	/*
	 * Set result code 
	 */
	result = 0;

    } else {

	/*
	 * Alpha blending blit 
	 */

	result = HLineAlpha(dst, x1, x1 + w, y, color);

    }

    return (result);
}

int hlineRGBA(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (hlineColor(dst, x1, x2, y, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ----- Vertical line */

int vlineColor(SDL_Surface * dst, Sint16 x, Sint16 y1, Sint16 y2, Uint32 color)
{
    Sint16 left, right, top, bottom;
    Uint8 *pixel, *pixellast;
    int dy;
    int pixx, pixy;
    Sint16 h;
    Sint16 ytmp;
    int result = -1;
    Uint8 *colorptr;

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Check visibility of vline 
     */
    if ((x<left) || (x>right)) {
     return (0);
    }
    if ((y1<top) && (y2<top)) {
     return(0);
    }
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    }

    /*
     * Clip y 
     */
    if (y1 < top) {
	y1 = top;
    }
    if (y2 > bottom) {
	y2 = bottom;
    }

    /*
     * Swap y1, y2 if required 
     */
    if (y1 > y2) {
	ytmp = y1;
	y1 = y2;
	y2 = ytmp;
    }

    /*
     * Calculate height 
     */
    h = y2 - y1;

    /*
     * Sanity check on height 
     */
    if (h < 0) {
	return (0);
    }

    /*
     * Alpha check 
     */
    if ((color & 255) == 255) {

	/*
	 * No alpha-blending required 
	 */

	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}

	/*
	 * Lock surface 
	 */
	SDL_LockSurface(dst);

	/*
	 * More variable setup 
	 */
	dy = h;
	pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x + pixy * (int) y1;
	pixellast = pixel + pixy * dy;

	/*
	 * Draw 
	 */
	switch (dst->format->BytesPerPixel) {
	case 1:
	    for (; pixel <= pixellast; pixel += pixy) {
		*(Uint8 *) pixel = color;
	    }
	    break;
	case 2:
	    for (; pixel <= pixellast; pixel += pixy) {
		*(Uint16 *) pixel = color;
	    }
	    break;
	case 3:
	    for (; pixel <= pixellast; pixel += pixy) {
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		    pixel[0] = (color >> 16) & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = color & 0xff;
		} else {
		    pixel[0] = color & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = (color >> 16) & 0xff;
		}
	    }
	    break;
	default:		/* case 4 */
	    for (; pixel <= pixellast; pixel += pixy) {
		*(Uint32 *) pixel = color;
	    }
	    break;
	}

	/*
	 * Unlock surface 
	 */
	SDL_UnlockSurface(dst);

	/*
	 * Set result code 
	 */
	result = 0;

    } else {

	/*
	 * Alpha blending blit 
	 */

	result = VLineAlpha(dst, x, y1, y1 + h, color);

    }

    return (result);
}

int vlineRGBA(SDL_Surface * dst, Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (vlineColor(dst, x, y1, y2, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ----- Rectangle */

int rectangleColor(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    int result;
    Sint16 w, h, xtmp, ytmp;

    /*
     * Swap x1, x2 if required 
     */
    if (x1 > x2) {
	xtmp = x1;
	x1 = x2;
	x2 = xtmp;
    }

    /*
     * Swap y1, y2 if required 
     */
    if (y1 > y2) {
	ytmp = y1;
	y1 = y2;
	y2 = ytmp;
    }

    /*
     * Calculate width&height 
     */
    w = x2 - x1;
    h = y2 - y1;

    /*
     * Sanity check 
     */
    if ((w < 0) || (h < 0)) {
	return (0);
    }

    /*
     * Test for special cases of straight lines or single point 
     */
    if (x1 == x2) {
	if (y1 == y2) {
	    return (pixelColor(dst, x1, y1, color));
	} else {
	    return (vlineColor(dst, x1, y1, y2, color));
	}
    } else {
	if (y1 == y2) {
	    return (hlineColor(dst, x1, x2, y1, color));
	}
    }

    /*
     * Draw rectangle 
     */
    result = 0;
    result |= hlineColor(dst, x1, x2, y1, color);
    result |= hlineColor(dst, x1, x2, y2, color);
    y1 += 1;
    y2 -= 1;
    if (y1<=y2) {
     result |= vlineColor(dst, x1, y1, y2, color);
     result |= vlineColor(dst, x2, y1, y2, color);
    }
    return (result);

}

int rectangleRGBA(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (rectangleColor
	    (dst, x1, y1, x2, y2, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* --------- Clipping routines for line */

/* Clipping based heavily on code from                       */
/* http://www.ncsa.uiuc.edu/Vis/Graphics/src/clipCohSuth.c   */

#define CLIP_LEFT_EDGE   0x1
#define CLIP_RIGHT_EDGE  0x2
#define CLIP_BOTTOM_EDGE 0x4
#define CLIP_TOP_EDGE    0x8
#define CLIP_INSIDE(a)   (!a)
#define CLIP_REJECT(a,b) (a&b)
#define CLIP_ACCEPT(a,b) (!(a|b))

static int clipEncode(Sint16 x, Sint16 y, Sint16 left, Sint16 top, Sint16 right, Sint16 bottom)
{
    int code = 0;

    if (x < left) {
	code |= CLIP_LEFT_EDGE;
    } else if (x > right) {
	code |= CLIP_RIGHT_EDGE;
    }
    if (y < top) {
	code |= CLIP_TOP_EDGE;
    } else if (y > bottom) {
	code |= CLIP_BOTTOM_EDGE;
    }
    return code;
}

static int clipLine(SDL_Surface * dst, Sint16 * x1, Sint16 * y1, Sint16 * x2, Sint16 * y2)
{
    Sint16 left, right, top, bottom;
    int code1, code2;
    int draw = 0;
    Sint16 swaptmp;
    sll m;

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    while (1) {
	code1 = clipEncode(*x1, *y1, left, top, right, bottom);
	code2 = clipEncode(*x2, *y2, left, top, right, bottom);
	if (CLIP_ACCEPT(code1, code2)) {
	    draw = 1;
	    break;
	} else if (CLIP_REJECT(code1, code2))
	    break;
	else {
	    if (CLIP_INSIDE(code1)) {
		swaptmp = *x2;
		*x2 = *x1;
		*x1 = swaptmp;
		swaptmp = *y2;
		*y2 = *y1;
		*y1 = swaptmp;
		swaptmp = code2;
		code2 = code1;
		code1 = swaptmp;
	    }
	    if (*x2 != *x1) {
		m = slldiv(int2sll(*y2 - *y1), int2sll(*x2 - *x1));
	    } else {
		m = SLL_CONST_1;
	    }
	    if (code1 & CLIP_LEFT_EDGE) {
		*y1 += (Sint16) sll2int(sllmul(int2sll(left - *x1), m));
		*x1 = left;
	    } else if (code1 & CLIP_RIGHT_EDGE) {
		*y1 += (Sint16) sll2int(sllmul(int2sll(right - *x1), m));
		*x1 = right;
	    } else if (code1 & CLIP_BOTTOM_EDGE) {
		if (*x2 != *x1) {
		    *x1 += (Sint16) sll2int(slldiv(int2sll(bottom - *y1), m));
		}
		*y1 = bottom;
	    } else if (code1 & CLIP_TOP_EDGE) {
		if (*x2 != *x1) {
		    *x1 += (Sint16) sll2int(slldiv(int2sll(top - *y1), m));
		}
		*y1 = top;
	    }
	}
    }

    return draw;
}

/* ----- Filled rectangle (Box) */

int boxColor(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    Sint16 left, right, top, bottom;
    Uint8 *pixel, *pixellast;
    int x, dx;
    int dy;
    int pixx, pixy;
    Sint16 w, h, tmp;
    int result;
    Uint8 *colorptr;

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
    
    /* Check visibility */
    if ((x1<left) && (x2<left)) {
     return(0);
    }
    if ((x1>right) && (x2>right)) {
     return(0);
    }
    if ((y1<top) && (y2<top)) {
     return(0);
    }
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    }
     
    /* Clip all points */
    if (x1<left) { 
     x1=left; 
    } else if (x1>right) {
     x1=right;
    }
    if (x2<left) { 
     x2=left; 
    } else if (x2>right) {
     x2=right;
    }
    if (y1<top) { 
     y1=top; 
    } else if (y1>bottom) {
     y1=bottom;
    }
    if (y2<top) { 
     y2=top; 
    } else if (y2>bottom) {
     y2=bottom;
    }

    /*
     * Order coordinates 
     */
    if (x1 > x2) {
	tmp = x1;
	x1 = x2;
	x2 = tmp;
    }
    if (y1 > y2) {
	tmp = y1;
	y1 = y2;
	y2 = tmp;
    }

    /*
     * Test for special cases of straight line or single point 
     */
    if (x1 == x2) {
	if (y1 == y2) {
	    return (pixelColor(dst, x1, y1, color));
	} else { 
	    return (vlineColor(dst, x1, y1, y2, color));
	}
    }
    if (y1 == y2) {
	return (hlineColor(dst, x1, x2, y1, color));
    }


    /*
     * Calculate width&height 
     */
    w = x2 - x1;
    h = y2 - y1;

    /*
     * Alpha check 
     */
    if ((color & 255) == 255) {

	/*
	 * No alpha-blending required 
	 */

	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}

	/*
	 * Lock surface 
	 */
	SDL_LockSurface(dst);

	/*
	 * More variable setup 
	 */
	dx = w;
	dy = h;
	pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x1 + pixy * (int) y1;
	pixellast = pixel + pixx * dx + pixy * dy;
	dx++;
	
	/*
	 * Draw 
	 */
	switch (dst->format->BytesPerPixel) {
	case 1:
	    for (; pixel <= pixellast; pixel += pixy) {
		memset(pixel, (Uint8) color, dx);
	    }
	    break;
	case 2:
	    pixy -= (pixx * dx);
	    for (; pixel <= pixellast; pixel += pixy) {
		for (x = 0; x < dx; x++) {
		    *(Uint16 *) pixel = color;
		    pixel += pixx;
		}
	    }
	    break;
	case 3:
	    pixy -= (pixx * dx);
	    for (; pixel <= pixellast; pixel += pixy) {
		for (x = 0; x < dx; x++) {
		    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			pixel[0] = (color >> 16) & 0xff;
			pixel[1] = (color >> 8) & 0xff;
			pixel[2] = color & 0xff;
		    } else {
			pixel[0] = color & 0xff;
			pixel[1] = (color >> 8) & 0xff;
			pixel[2] = (color >> 16) & 0xff;
		    }
		    pixel += pixx;
		}
	    }
	    break;
	default:		/* case 4 */
	    pixy -= (pixx * dx);
	    for (; pixel <= pixellast; pixel += pixy) {
		for (x = 0; x < dx; x++) {
		    *(Uint32 *) pixel = color;
		    pixel += pixx;
		}
	    }
	    break;
	}

	/*
	 * Unlock surface 
	 */
	SDL_UnlockSurface(dst);

	result = 0;

    } else {

	result = filledRectAlpha(dst, x1, y1, x1 + w, y1 + h, color);

    }

    return (result);
}

int boxRGBA(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (boxColor(dst, x1, y1, x2, y2, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ----- Line */

/* Non-alpha line drawing code adapted from routine          */
/* by Pete Shinners, pete@shinners.org                       */
/* Originally from pygame, http://pygame.seul.org            */

#define ABS(a) (((a)<0) ? -(a) : (a))

int lineColor(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    int pixx, pixy;
    int x, y;
    int dx, dy;
    int ax, ay;
    int sx, sy;
    int swaptmp;
    Uint8 *pixel;
    Uint8 *colorptr;

    /*
     * Clip line and test if we have to draw 
     */
    if (!(clipLine(dst, &x1, &y1, &x2, &y2))) {
	return (0);
    }

    /*
     * Test for special cases of straight lines or single point 
     */
    if (x1 == x2) {
	if (y1 < y2) {
	    return (vlineColor(dst, x1, y1, y2, color));
	} else if (y1 > y2) {
	    return (vlineColor(dst, x1, y2, y1, color));
	} else {
	    return (pixelColor(dst, x1, y1, color));
	}
    }
    if (y1 == y2) {
	if (x1 < x2) {
	    return (hlineColor(dst, x1, x2, y1, color));
	} else if (x1 > x2) {
	    return (hlineColor(dst, x2, x1, y1, color));
	}
    }

    /*
     * Variable setup 
     */
    dx = x2 - x1;
    dy = y2 - y1;
    sx = (dx >= 0) ? 1 : -1;
    sy = (dy >= 0) ? 1 : -1;

    /* Lock surface */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    /*
     * Check for alpha blending 
     */
    if ((color & 255) == 255) {

	/*
	 * No alpha blending - use fast pixel routines 
	 */

	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}

	/*
	 * More variable setup 
	 */
	dx = sx * dx + 1;
	dy = sy * dy + 1;
	pixx = dst->format->BytesPerPixel;
	pixy = dst->pitch;
	pixel = ((Uint8 *) dst->pixels) + pixx * (int) x1 + pixy * (int) y1;
	pixx *= sx;
	pixy *= sy;
	if (dx < dy) {
	    swaptmp = dx;
	    dx = dy;
	    dy = swaptmp;
	    swaptmp = pixx;
	    pixx = pixy;
	    pixy = swaptmp;
	}

	/*
	 * Draw 
	 */
	x = 0;
	y = 0;
	switch (dst->format->BytesPerPixel) {
	case 1:
	    for (; x < dx; x++, pixel += pixx) {
		*pixel = color;
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	case 2:
	    for (; x < dx; x++, pixel += pixx) {
		*(Uint16 *) pixel = color;
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	case 3:
	    for (; x < dx; x++, pixel += pixx) {
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		    pixel[0] = (color >> 16) & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = color & 0xff;
		} else {
		    pixel[0] = color & 0xff;
		    pixel[1] = (color >> 8) & 0xff;
		    pixel[2] = (color >> 16) & 0xff;
		}
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	default:		/* case 4 */
	    for (; x < dx; x++, pixel += pixx) {
		*(Uint32 *) pixel = color;
		y += dy;
		if (y >= dx) {
		    y -= dx;
		    pixel += pixy;
		}
	    }
	    break;
	}

    } else {

	/*
	 * Alpha blending required - use single-pixel blits 
	 */

	ax = ABS(dx) << 1;
	ay = ABS(dy) << 1;
	x = x1;
	y = y1;
	if (ax > ay) {
	    int d = ay - (ax >> 1);

	    while (x != x2) {
		pixelColorNolock (dst, x, y, color);
		if (d > 0 || (d == 0 && sx == 1)) {
		    y += sy;
		    d -= ax;
		}
		x += sx;
		d += ay;
	    }
	} else {
	    int d = ax - (ay >> 1);

	    while (y != y2) {
		pixelColorNolock (dst, x, y, color);
		if (d > 0 || ((d == 0) && (sy == 1))) {
		    x += sx;
		    d -= ay;
		}
		y += sy;
		d += ax;
	    }
	}
	pixelColorNolock (dst, x, y, color);

    }

    /* Unlock surface */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (0);
}

int lineRGBA(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (lineColor(dst, x1, y1, x2, y2, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* AA Line */

#define AAlevels 256
#define AAbits 8

/* 

This implementation of the Wu antialiasing code is based on Mike Abrash's
DDJ article which was reprinted as Chapter 42 of his Graphics Programming
Black Book, but has been optimized to work with SDL and utilizes 32-bit
fixed-point arithmetic. (A. Schiffler).

*/

int aalineColorInt(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color, int draw_endpoint)
{
    Sint32 xx0, yy0, xx1, yy1;
    int result;
    Uint32 intshift, erracc, erradj;
    Uint32 erracctmp, wgt, wgtcompmask;
    int dx, dy, tmp, xdir, y0p1, x0pxdir;

    /*
     * Clip line and test if we have to draw 
     */
    if (!(clipLine(dst, &x1, &y1, &x2, &y2))) {
	return (0);
    }

    /*
     * Keep on working with 32bit numbers 
     */
    xx0 = x1;
    yy0 = y1;
    xx1 = x2;
    yy1 = y2;

    /*
     * Reorder points if required 
     */
    if (yy0 > yy1) {
	tmp = yy0;
	yy0 = yy1;
	yy1 = tmp;
	tmp = xx0;
	xx0 = xx1;
	xx1 = tmp;
    }

    /*
     * Calculate distance 
     */
    dx = xx1 - xx0;
    dy = yy1 - yy0;

    /*
     * Adjust for negative dx and set xdir 
     */
    if (dx >= 0) {
	xdir = 1;
    } else {
	xdir = -1;
	dx = (-dx);
    }

    /*
     * Check for special cases 
     */
    if (dx == 0) {
	/*
	 * Vertical line 
	 */
	return (vlineColor(dst, x1, y1, y2, color));
    } else if (dy == 0) {
	/*
	 * Horizontal line 
	 */
	return (hlineColor(dst, x1, x2, y1, color));
    } else if (dx == dy) {
	/*
	 * Diagonal line 
	 */
	return (lineColor(dst, x1, y1, x2, y2, color));
    }

    /*
     * Line is not horizontal, vertical or diagonal 
     */
    result = 0;

    /*
     * Zero accumulator 
     */
    erracc = 0;

    /*
     * # of bits by which to shift erracc to get intensity level 
     */
    intshift = 32 - AAbits;
    /*
     * Mask used to flip all bits in an intensity weighting 
     */
    wgtcompmask = AAlevels - 1;

    /* Lock surface */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    /*
     * Draw the initial pixel in the foreground color 
     */
    result |= pixelColorNolock(dst, x1, y1, color);

    /*
     * x-major or y-major? 
     */
    if (dy > dx) {

	/*
	 * y-major.  Calculate 16-bit fixed point fractional part of a pixel that
	 * X advances every time Y advances 1 pixel, truncating the result so that
	 * we won't overrun the endpoint along the X axis 
	 */
	/*
	 * Not-so-portable version: erradj = ((Uint64)dx << 32) / (Uint64)dy; 
	 */
	erradj = ((dx << 16) / dy) << 16;

	/*
	 * draw all pixels other than the first and last 
	 */
	x0pxdir = xx0 + xdir;
	while (--dy) {
	    erracctmp = erracc;
	    erracc += erradj;
	    if (erracc <= erracctmp) {
		/*
		 * rollover in error accumulator, x coord advances 
		 */
		xx0 = x0pxdir;
		x0pxdir += xdir;
	    }
	    yy0++;		/* y-major so always advance Y */

	    /*
	     * the AAbits most significant bits of erracc give us the intensity
	     * weighting for this pixel, and the complement of the weighting for
	     * the paired pixel. 
	     */
	    wgt = (erracc >> intshift) & 255;
	    result |= pixelColorWeightNolock (dst, xx0, yy0, color, 255 - wgt);
	    result |= pixelColorWeightNolock (dst, x0pxdir, yy0, color, wgt);
	}

    } else {

	/*
	 * x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
	 * that Y advances each time X advances 1 pixel, truncating the result so
	 * that we won't overrun the endpoint along the X axis. 
	 */
	/*
	 * Not-so-portable version: erradj = ((Uint64)dy << 32) / (Uint64)dx; 
	 */
	erradj = ((dy << 16) / dx) << 16;

	/*
	 * draw all pixels other than the first and last 
	 */
	y0p1 = yy0 + 1;
	while (--dx) {

	    erracctmp = erracc;
	    erracc += erradj;
	    if (erracc <= erracctmp) {
		/*
		 * Accumulator turned over, advance y 
		 */
		yy0 = y0p1;
		y0p1++;
	    }
	    xx0 += xdir;	/* x-major so always advance X */
	    /*
	     * the AAbits most significant bits of erracc give us the intensity
	     * weighting for this pixel, and the complement of the weighting for
	     * the paired pixel. 
	     */
	    wgt = (erracc >> intshift) & 255;
	    result |= pixelColorWeightNolock (dst, xx0, yy0, color, 255 - wgt);
	    result |= pixelColorWeightNolock (dst, xx0, y0p1, color, wgt);
	}
    }

    /*
     * Do we have to draw the endpoint 
     */
    if (draw_endpoint) {
	/*
	 * Draw final pixel, always exactly intersected by the line and doesn't
	 * need to be weighted. 
	 */
	result |= pixelColorNolock (dst, x2, y2, color);
    }

    /* Unlock surface */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (result);
}

int aalineColor(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
    return (aalineColorInt(dst, x1, y1, x2, y2, color, 1));
}

int aalineRGBA(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    return (aalineColorInt
	    (dst, x1, y1, x2, y2, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a, 1));
}


/* ----- Circle */

/* Note: Based on algorithm from sge library, modified by A. Schiffler */
/* with multiple pixel-draw removal and other minor speedup changes.   */

int circleColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color)
{
    Sint16 left, right, top, bottom;
    int result;
    Sint16 x1, y1, x2, y2;
    Sint16 cx = 0;
    Sint16 cy = r;
    Sint16 ocx = (Sint16) 0xffff;
    Sint16 ocy = (Sint16) 0xffff;
    Sint16 df = 1 - r;
    Sint16 d_e = 3;
    Sint16 d_se = -2 * r + 5;
    Sint16 xpcx, xmcx, xpcy, xmcy;
    Sint16 ypcy, ymcy, ypcx, ymcx;
    Uint8 *colorptr;

    /*
     * Sanity check radius 
     */
    if (r < 0) {
	return (-1);
    }

    /*
     * Special case for r=0 - draw a point 
     */
    if (r == 0) {
	return (pixelColor(dst, x, y, color));
    }

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Test if bounding box of circle is visible 
     */
    x1 = x - r;
    x2 = x + r;
    y1 = y - r;
    y2 = y + r;
    if ((x1<left) && (x2<left)) {
     return(0);
    } 
    if ((x1>right) && (x2>right)) {
     return(0);
    } 
    if ((y1<top) && (y2<top)) {
     return(0);
    } 
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    } 

    /*
     * Draw circle 
     */
    result = 0;

    /* Lock surface */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    /*
     * Alpha Check 
     */
    if ((color & 255) == 255) {

	/*
	 * No Alpha - direct memory writes 
	 */

	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}

	/*
	 * Draw 
	 */
	do {
	    if ((ocy != cy) || (ocx != cx)) {
		xpcx = x + cx;
		xmcx = x - cx;
		if (cy > 0) {
		    ypcy = y + cy;
		    ymcy = y - cy;
		    result |= fastPixelColorNolock(dst, xmcx, ypcy, color);
		    result |= fastPixelColorNolock(dst, xpcx, ypcy, color);
		    result |= fastPixelColorNolock(dst, xmcx, ymcy, color);
		    result |= fastPixelColorNolock(dst, xpcx, ymcy, color);
		} else {
		    result |= fastPixelColorNolock(dst, xmcx, y, color);
		    result |= fastPixelColorNolock(dst, xpcx, y, color);
		}
		ocy = cy;
		xpcy = x + cy;
		xmcy = x - cy;
		if (cx > 0) {
		    ypcx = y + cx;
		    ymcx = y - cx;
		    result |= fastPixelColorNolock(dst, xmcy, ypcx, color);
		    result |= fastPixelColorNolock(dst, xpcy, ypcx, color);
		    result |= fastPixelColorNolock(dst, xmcy, ymcx, color);
		    result |= fastPixelColorNolock(dst, xpcy, ymcx, color);
		} else {
		    result |= fastPixelColorNolock(dst, xmcy, y, color);
		    result |= fastPixelColorNolock(dst, xpcy, y, color);
		}
		ocx = cx;
	    }
	    /*
	     * Update 
	     */
	    if (df < 0) {
		df += d_e;
		d_e += 2;
		d_se += 2;
	    } else {
		df += d_se;
		d_e += 2;
		d_se += 4;
		cy--;
	    }
	    cx++;
	} while (cx <= cy);

	/*
	 * Unlock surface 
	 */
	SDL_UnlockSurface(dst);

    } else {

	/*
	 * Using Alpha - blended pixel blits 
	 */

	do {
	    /*
	     * Draw 
	     */
	    if ((ocy != cy) || (ocx != cx)) {
		xpcx = x + cx;
		xmcx = x - cx;
		if (cy > 0) {
		    ypcy = y + cy;
		    ymcy = y - cy;
		    result |= pixelColorNolock (dst, xmcx, ypcy, color);
		    result |= pixelColorNolock (dst, xpcx, ypcy, color);
		    result |= pixelColorNolock (dst, xmcx, ymcy, color);
		    result |= pixelColorNolock (dst, xpcx, ymcy, color);
		} else {
		    result |= pixelColorNolock (dst, xmcx, y, color);
		    result |= pixelColorNolock (dst, xpcx, y, color);
		}
		ocy = cy;
		xpcy = x + cy;
		xmcy = x - cy;
		if (cx > 0) {
		    ypcx = y + cx;
		    ymcx = y - cx;
		    result |= pixelColorNolock (dst, xmcy, ypcx, color);
		    result |= pixelColorNolock (dst, xpcy, ypcx, color);
		    result |= pixelColorNolock (dst, xmcy, ymcx, color);
		    result |= pixelColorNolock (dst, xpcy, ymcx, color);
		} else {
		    result |= pixelColorNolock (dst, xmcy, y, color);
		    result |= pixelColorNolock (dst, xpcy, y, color);
		}
		ocx = cx;
	    }
	    /*
	     * Update 
	     */
	    if (df < 0) {
		df += d_e;
		d_e += 2;
		d_se += 2;
	    } else {
		df += d_se;
		d_e += 2;
		d_se += 4;
		cy--;
	    }
	    cx++;
	} while (cx <= cy);

    }				/* Alpha check */

    /* Unlock surface */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (result);
}

int circleRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (circleColor(dst, x, y, rad, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ----- AA Circle */

/* AA circle is based on AAellipse  */

int aacircleColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color)
{
    return (aaellipseColor(dst, x, y, r, r, color));
}

int aacircleRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (aaellipseColor
	    (dst, x, y, rad, rad, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ----- Filled Circle */

/* Note: Based on algorithm from sge library with multiple-hline draw removal */

/* and other speedup changes. */

int filledCircleColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 r, Uint32 color)
{
    Sint16 left, right, top, bottom;
    int result;
    Sint16 x1, y1, x2, y2;
    Sint16 cx = 0;
    Sint16 cy = r;
    Sint16 ocx = (Sint16) 0xffff;
    Sint16 ocy = (Sint16) 0xffff;
    Sint16 df = 1 - r;
    Sint16 d_e = 3;
    Sint16 d_se = -2 * r + 5;
    Sint16 xpcx, xmcx, xpcy, xmcy;
    Sint16 ypcy, ymcy, ypcx, ymcx;

    /*
     * Sanity check radius 
     */
    if (r < 0) {
	return (-1);
    }

    /*
     * Special case for r=0 - draw a point 
     */
    if (r == 0) {
	return (pixelColor(dst, x, y, color));
    }

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Test if bounding box of circle is visible 
     */
    x1 = x - r;
    x2 = x + r;
    y1 = y - r;
    y2 = y + r;
    if ((x1<left) && (x2<left)) {
     return(0);
    } 
    if ((x1>right) && (x2>right)) {
     return(0);
    } 
    if ((y1<top) && (y2<top)) {
     return(0);
    } 
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    } 

    /*
     * Draw 
     */
    result = 0;
    do {
	xpcx = x + cx;
	xmcx = x - cx;
	xpcy = x + cy;
	xmcy = x - cy;
	if (ocy != cy) {
	    if (cy > 0) {
		ypcy = y + cy;
		ymcy = y - cy;
		result |= hlineColor(dst, xmcx, xpcx, ypcy, color);
		result |= hlineColor(dst, xmcx, xpcx, ymcy, color);
	    } else {
		result |= hlineColor(dst, xmcx, xpcx, y, color);
	    }
	    ocy = cy;
	}
	if (ocx != cx) {
	    if (cx != cy) {
		if (cx > 0) {
		    ypcx = y + cx;
		    ymcx = y - cx;
		    result |= hlineColor(dst, xmcy, xpcy, ymcx, color);
		    result |= hlineColor(dst, xmcy, xpcy, ypcx, color);
		} else {
		    result |= hlineColor(dst, xmcy, xpcy, y, color);
		}
	    }
	    ocx = cx;
	}
	/*
	 * Update 
	 */
	if (df < 0) {
	    df += d_e;
	    d_e += 2;
	    d_se += 2;
	} else {
	    df += d_se;
	    d_e += 2;
	    d_se += 4;
	    cy--;
	}
	cx++;
    } while (cx <= cy);

    return (result);
}

int filledCircleRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (filledCircleColor
	    (dst, x, y, rad, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}


/* ----- Ellipse */

/* Note: Based on algorithm from sge library with multiple-hline draw removal */
/* and other speedup changes. */

int ellipseColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color)
{
    Sint16 left, right, top, bottom;
    int result;
    Sint16 x1, y1, x2, y2;
    int ix, iy;
    int h, i, j, k;
    int oh, oi, oj, ok;
    int xmh, xph, ypk, ymk;
    int xmi, xpi, ymj, ypj;
    int xmj, xpj, ymi, ypi;
    int xmk, xpk, ymh, yph;
    Uint8 *colorptr;

    /*
     * Sanity check radii 
     */
    if ((rx < 0) || (ry < 0)) {
	return (-1);
    }

    /*
     * Special case for rx=0 - draw a vline 
     */
    if (rx == 0) {
	return (vlineColor(dst, x, y - ry, y + ry, color));
    }
    /*
     * Special case for ry=0 - draw a hline 
     */
    if (ry == 0) {
	return (hlineColor(dst, x - rx, x + rx, y, color));
    }

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Test if bounding box of ellipse is visible 
     */
    x1 = x - rx;
    x2 = x + rx;
    y1 = y - ry;
    y2 = y + ry;
    if ((x1<left) && (x2<left)) {
     return(0);
    } 
    if ((x1>right) && (x2>right)) {
     return(0);
    } 
    if ((y1<top) && (y2<top)) {
     return(0);
    } 
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    } 

    /*
     * Init vars 
     */
    oh = oi = oj = ok = 0xFFFF;

    /*
     * Draw 
     */
    result = 0;

    /* Lock surface */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    /*
     * Check alpha 
     */
    if ((color & 255) == 255) {

	/*
	 * No Alpha - direct memory writes 
	 */

	/*
	 * Setup color 
	 */
	colorptr = (Uint8 *) & color;
	if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    color = SDL_MapRGBA(dst->format, colorptr[0], colorptr[1], colorptr[2], colorptr[3]);
	} else {
	    color = SDL_MapRGBA(dst->format, colorptr[3], colorptr[2], colorptr[1], colorptr[0]);
	}


	if (rx > ry) {
	    ix = 0;
	    iy = rx * 64;

	    do {
		h = (ix + 32) >> 6;
		i = (iy + 32) >> 6;
		j = (h * ry) / rx;
		k = (i * ry) / rx;

		if (((ok != k) && (oj != k)) || ((oj != j) && (ok != j)) || (k != j)) {
		    xph = x + h;
		    xmh = x - h;
		    if (k > 0) {
			ypk = y + k;
			ymk = y - k;
			result |= fastPixelColorNolock(dst, xmh, ypk, color);
			result |= fastPixelColorNolock(dst, xph, ypk, color);
			result |= fastPixelColorNolock(dst, xmh, ymk, color);
			result |= fastPixelColorNolock(dst, xph, ymk, color);
		    } else {
			result |= fastPixelColorNolock(dst, xmh, y, color);
			result |= fastPixelColorNolock(dst, xph, y, color);
		    }
		    ok = k;
		    xpi = x + i;
		    xmi = x - i;
		    if (j > 0) {
			ypj = y + j;
			ymj = y - j;
			result |= fastPixelColorNolock(dst, xmi, ypj, color);
			result |= fastPixelColorNolock(dst, xpi, ypj, color);
			result |= fastPixelColorNolock(dst, xmi, ymj, color);
			result |= fastPixelColorNolock(dst, xpi, ymj, color);
		    } else {
			result |= fastPixelColorNolock(dst, xmi, y, color);
			result |= fastPixelColorNolock(dst, xpi, y, color);
		    }
		    oj = j;
		}

		ix = ix + iy / rx;
		iy = iy - ix / rx;

	    } while (i > h);
	} else {
	    ix = 0;
	    iy = ry * 64;

	    do {
		h = (ix + 32) >> 6;
		i = (iy + 32) >> 6;
		j = (h * rx) / ry;
		k = (i * rx) / ry;

		if (((oi != i) && (oh != i)) || ((oh != h) && (oi != h) && (i != h))) {
		    xmj = x - j;
		    xpj = x + j;
		    if (i > 0) {
			ypi = y + i;
			ymi = y - i;
			result |= fastPixelColorNolock(dst, xmj, ypi, color);
			result |= fastPixelColorNolock(dst, xpj, ypi, color);
			result |= fastPixelColorNolock(dst, xmj, ymi, color);
			result |= fastPixelColorNolock(dst, xpj, ymi, color);
		    } else {
			result |= fastPixelColorNolock(dst, xmj, y, color);
			result |= fastPixelColorNolock(dst, xpj, y, color);
		    }
		    oi = i;
		    xmk = x - k;
		    xpk = x + k;
		    if (h > 0) {
			yph = y + h;
			ymh = y - h;
			result |= fastPixelColorNolock(dst, xmk, yph, color);
			result |= fastPixelColorNolock(dst, xpk, yph, color);
			result |= fastPixelColorNolock(dst, xmk, ymh, color);
			result |= fastPixelColorNolock(dst, xpk, ymh, color);
		    } else {
			result |= fastPixelColorNolock(dst, xmk, y, color);
			result |= fastPixelColorNolock(dst, xpk, y, color);
		    }
		    oh = h;
		}

		ix = ix + iy / ry;
		iy = iy - ix / ry;

	    } while (i > h);
	}

    } else {

	if (rx > ry) {
	    ix = 0;
	    iy = rx * 64;

	    do {
		h = (ix + 32) >> 6;
		i = (iy + 32) >> 6;
		j = (h * ry) / rx;
		k = (i * ry) / rx;

		if (((ok != k) && (oj != k)) || ((oj != j) && (ok != j)) || (k != j)) {
		    xph = x + h;
		    xmh = x - h;
		    if (k > 0) {
			ypk = y + k;
			ymk = y - k;
			result |= pixelColorNolock (dst, xmh, ypk, color);
			result |= pixelColorNolock (dst, xph, ypk, color);
			result |= pixelColorNolock (dst, xmh, ymk, color);
			result |= pixelColorNolock (dst, xph, ymk, color);
		    } else {
			result |= pixelColorNolock (dst, xmh, y, color);
			result |= pixelColorNolock (dst, xph, y, color);
		    }
		    ok = k;
		    xpi = x + i;
		    xmi = x - i;
		    if (j > 0) {
			ypj = y + j;
			ymj = y - j;
			result |= pixelColorNolock (dst, xmi, ypj, color);
			result |= pixelColorNolock (dst, xpi, ypj, color);
			result |= pixelColorNolock (dst, xmi, ymj, color);
			result |= pixelColor(dst, xpi, ymj, color);
		    } else {
			result |= pixelColorNolock (dst, xmi, y, color);
			result |= pixelColorNolock (dst, xpi, y, color);
		    }
		    oj = j;
		}

		ix = ix + iy / rx;
		iy = iy - ix / rx;

	    } while (i > h);
	} else {
	    ix = 0;
	    iy = ry * 64;

	    do {
		h = (ix + 32) >> 6;
		i = (iy + 32) >> 6;
		j = (h * rx) / ry;
		k = (i * rx) / ry;

		if (((oi != i) && (oh != i)) || ((oh != h) && (oi != h) && (i != h))) {
		    xmj = x - j;
		    xpj = x + j;
		    if (i > 0) {
			ypi = y + i;
			ymi = y - i;
			result |= pixelColorNolock (dst, xmj, ypi, color);
			result |= pixelColorNolock (dst, xpj, ypi, color);
			result |= pixelColorNolock (dst, xmj, ymi, color);
			result |= pixelColorNolock (dst, xpj, ymi, color);
		    } else {
			result |= pixelColorNolock (dst, xmj, y, color);
			result |= pixelColorNolock (dst, xpj, y, color);
		    }
		    oi = i;
		    xmk = x - k;
		    xpk = x + k;
		    if (h > 0) {
			yph = y + h;
			ymh = y - h;
			result |= pixelColorNolock (dst, xmk, yph, color);
			result |= pixelColorNolock (dst, xpk, yph, color);
			result |= pixelColorNolock (dst, xmk, ymh, color);
			result |= pixelColorNolock (dst, xpk, ymh, color);
		    } else {
			result |= pixelColorNolock (dst, xmk, y, color);
			result |= pixelColorNolock (dst, xpk, y, color);
		    }
		    oh = h;
		}

		ix = ix + iy / ry;
		iy = iy - ix / ry;

	    } while (i > h);
	}

    }				/* Alpha check */

    /* Unlock surface */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (result);
}

int ellipseRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (ellipseColor(dst, x, y, rx, ry, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ----- AA Ellipse */

/* Based on code from Anders Lindstroem, based on code from SGE, based on code from TwinLib */

int aaellipseColor(SDL_Surface * dst, Sint16 xc, Sint16 yc, Sint16 rx, Sint16 ry, Uint32 color)
{
    Sint16 left, right, top, bottom;
    Sint16 x1,y1,x2,y2;
    int i;
    int a2, b2, ds, dt, dxt, t, s, d;
    Sint16 x, y, xs, ys, dyt, xx, yy, xc2, yc2;
    sll cp;
    Uint8 weight, iweight;
    int result;

    /*
     * Sanity check radii 
     */
    if ((rx < 0) || (ry < 0)) {
	return (-1);
    }

    /*
     * Special case for rx=0 - draw a vline 
     */
    if (rx == 0) {
	return (vlineColor(dst, xc, yc - ry, yc + ry, color));
    }
    /*
     * Special case for ry=0 - draw a hline 
     */
    if (ry == 0) {
	return (hlineColor(dst, xc - rx, xc + rx, yc, color));
    }

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Test if bounding box of ellipse is visible 
     */
    x1 = xc - rx;
    x2 = xc + rx;
    y1 = yc - ry;
    y2 = yc + ry;
    if ((x1<left) && (x2<left)) {
     return(0);
    } 
    if ((x1>right) && (x2>right)) {
     return(0);
    } 
    if ((y1<top) && (y2<top)) {
     return(0);
    } 
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    } 
    
    /* Variable setup */
    a2 = rx * rx;
    b2 = ry * ry;

    ds = 2 * a2;
    dt = 2 * b2;

    xc2 = 2 * xc;
    yc2 = 2 * yc;

    dxt = sll2int(slldiv(a2, sllsqrt(int2sll(a2+b2))));

    t = 0;
    s = -2 * a2 * ry;
    d = 0;

    x = xc;
    y = yc - ry;

    /* Draw */
    result = 0;

    /* Lock surface */
    if (SDL_MUSTLOCK(dst)) {
	if (SDL_LockSurface(dst) < 0) {
	    return (-1);
	}
    }

    /* "End points" */
    result |= pixelColorNolock(dst, x, y, color);
    result |= pixelColorNolock(dst, xc2 - x, y, color);
    result |= pixelColorNolock(dst, x, yc2 - y, color);
    result |= pixelColorNolock(dst, xc2 - x, yc2 - y, color);

    for (i = 1; i <= dxt; i++) {
	x--;
	d += t - b2;

	if (d >= 0)
	    ys = y - 1;
	else if ((d - s - a2) > 0) {
	    if ((2 * d - s - a2) >= 0)
		ys = y + 1;
	    else {
		ys = y;
		y++;
		d -= s + a2;
		s += ds;
	    }
	} else {
	    y++;
	    ys = y + 1;
	    d -= s + a2;
	    s += ds;
	}

	t -= dt;

	/* Calculate alpha */
	if (s != 0.0) {
	    cp = slldiv(int2sll(abs(d)), int2sll(abs(s)));
	    if (cp > SLL_CONST_1) {
		cp = SLL_CONST_1;
	    }
	} else {
	    cp = SLL_CONST_1;
	}

	/* Calculate weights */
	weight = (Uint8) sll2int(sllmul(cp, int2sll(255)));
	iweight = 255 - weight;

	/* Upper half */
	xx = xc2 - x;
	result |= pixelColorWeightNolock(dst, x, y, color, iweight);
	result |= pixelColorWeightNolock(dst, xx, y, color, iweight);

	result |= pixelColorWeightNolock(dst, x, ys, color, weight);
	result |= pixelColorWeightNolock(dst, xx, ys, color, weight);

	/* Lower half */
	yy = yc2 - y;
	result |= pixelColorWeightNolock(dst, x, yy, color, iweight);
	result |= pixelColorWeightNolock(dst, xx, yy, color, iweight);

	yy = yc2 - ys;
	result |= pixelColorWeightNolock(dst, x, yy, color, weight);
	result |= pixelColorWeightNolock(dst, xx, yy, color, weight);
    }

    dyt = abs(y - yc);

    for (i = 1; i <= dyt; i++) {
	y++;
	d -= s + a2;

	if (d <= 0)
	    xs = x + 1;
	else if ((d + t - b2) < 0) {
	    if ((2 * d + t - b2) <= 0)
		xs = x - 1;
	    else {
		xs = x;
		x--;
		d += t - b2;
		t -= dt;
	    }
	} else {
	    x--;
	    xs = x - 1;
	    d += t - b2;
	    t -= dt;
	}

	s += ds;

	/* Calculate alpha */
	if (t != 0.0) {
	    cp = slldiv(int2sll(abs(d)), int2sll(abs(t)));
	    if (cp > SLL_CONST_1) {
		cp = SLL_CONST_1;
	    }
	} else {
	    cp = SLL_CONST_1;
	}

	/* Calculate weight */
	weight = (Uint8) sll2int(sllmul(cp, int2sll(255)));
	iweight = 255 - weight;

	/* Left half */
	xx = xc2 - x;
	yy = yc2 - y;
	result |= pixelColorWeightNolock(dst, x, y, color, iweight);
	result |= pixelColorWeightNolock(dst, xx, y, color, iweight);

	result |= pixelColorWeightNolock(dst, x, yy, color, iweight);
	result |= pixelColorWeightNolock(dst, xx, yy, color, iweight);

	/* Right half */
	xx = 2 * xc - xs;
	result |= pixelColorWeightNolock(dst, xs, y, color, weight);
	result |= pixelColorWeightNolock(dst, xx, y, color, weight);

	result |= pixelColorWeightNolock(dst, xs, yy, color, weight);
	result |= pixelColorWeightNolock(dst, xx, yy, color, weight);


    }

    /* Unlock surface */
    if (SDL_MUSTLOCK(dst)) {
	SDL_UnlockSurface(dst);
    }

    return (result);
}

int aaellipseRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (aaellipseColor
	    (dst, x, y, rx, ry, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ---- Filled Ellipse */

/* Note: */
/* Based on algorithm from sge library with multiple-hline draw removal */
/* and other speedup changes. */

int filledEllipseColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 color)
{
    Sint16 left, right, top, bottom;
    int result;
    Sint16 x1, y1, x2, y2;
    int ix, iy;
    int h, i, j, k;
    int oh, oi, oj, ok;
    int xmh, xph;
    int xmi, xpi;
    int xmj, xpj;
    int xmk, xpk;

    /*
     * Sanity check radii 
     */
    if ((rx < 0) || (ry < 0)) {
	return (-1);
    }

    /*
     * Special case for rx=0 - draw a vline 
     */
    if (rx == 0) {
	return (vlineColor(dst, x, y - ry, y + ry, color));
    }
    /*
     * Special case for ry=0 - draw a hline 
     */
    if (ry == 0) {
	return (hlineColor(dst, x - rx, x + rx, y, color));
    }

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Test if bounding box of ellipse is visible 
     */
    x1 = x - rx;
    x2 = x + rx;
    y1 = y - ry;
    y2 = y + ry;
    if ((x1<left) && (x2<left)) {
     return(0);
    } 
    if ((x1>right) && (x2>right)) {
     return(0);
    } 
    if ((y1<top) && (y2<top)) {
     return(0);
    } 
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    } 

    /*
     * Init vars 
     */
    oh = oi = oj = ok = 0xFFFF;

    /*
     * Draw 
     */
    result = 0;
    if (rx > ry) {
	ix = 0;
	iy = rx * 64;

	do {
	    h = (ix + 32) >> 6;
	    i = (iy + 32) >> 6;
	    j = (h * ry) / rx;
	    k = (i * ry) / rx;

	    if ((ok != k) && (oj != k)) {
		xph = x + h;
		xmh = x - h;
		if (k > 0) {
		    result |= hlineColor(dst, xmh, xph, y + k, color);
		    result |= hlineColor(dst, xmh, xph, y - k, color);
		} else {
		    result |= hlineColor(dst, xmh, xph, y, color);
		}
		ok = k;
	    }
	    if ((oj != j) && (ok != j) && (k != j)) {
		xmi = x - i;
		xpi = x + i;
		if (j > 0) {
		    result |= hlineColor(dst, xmi, xpi, y + j, color);
		    result |= hlineColor(dst, xmi, xpi, y - j, color);
		} else {
		    result |= hlineColor(dst, xmi, xpi, y, color);
		}
		oj = j;
	    }

	    ix = ix + iy / rx;
	    iy = iy - ix / rx;

	} while (i > h);
    } else {
	ix = 0;
	iy = ry * 64;

	do {
	    h = (ix + 32) >> 6;
	    i = (iy + 32) >> 6;
	    j = (h * rx) / ry;
	    k = (i * rx) / ry;

	    if ((oi != i) && (oh != i)) {
		xmj = x - j;
		xpj = x + j;
		if (i > 0) {
		    result |= hlineColor(dst, xmj, xpj, y + i, color);
		    result |= hlineColor(dst, xmj, xpj, y - i, color);
		} else {
		    result |= hlineColor(dst, xmj, xpj, y, color);
		}
		oi = i;
	    }
	    if ((oh != h) && (oi != h) && (i != h)) {
		xmk = x - k;
		xpk = x + k;
		if (h > 0) {
		    result |= hlineColor(dst, xmk, xpk, y + h, color);
		    result |= hlineColor(dst, xmk, xpk, y - h, color);
		} else {
		    result |= hlineColor(dst, xmk, xpk, y, color);
		}
		oh = h;
	    }

	    ix = ix + iy / ry;
	    iy = iy - ix / ry;

	} while (i > h);
    }

    return (result);
}


int filledEllipseRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (filledEllipseColor
	    (dst, x, y, rx, ry, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ----- filled pie */

/* Low-speed float pie-calc implementation by drawing polygons/lines. */

int doPieColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 color, Uint8 filled)
{
    Sint16 left, right, top, bottom;
    Sint16 x1, y1, x2, y2;
    int result;
    sll angle, start_angle, end_angle;
    sll deltaAngle;
    sll dr;
    int posX, posY;
    int numpoints, i;
    Sint16 *vx, *vy;

    /*
     * Sanity check radii 
     */
    if (rad < 0) {
	return (-1);
    }

    /*
     * Fixup angles
     */
    start = start % 360;
    end = end % 360;

    /*
     * Special case for rad=0 - draw a point 
     */
    if (rad == 0) {
	return (pixelColor(dst, x, y, color));
    }

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Test if bounding box of pie's circle is visible 
     */
    x1 = x - rad;
    x2 = x + rad;
    y1 = y - rad;
    y2 = y + rad;
    if ((x1<left) && (x2<left)) {
     return(0);
    } 
    if ((x1>right) && (x2>right)) {
     return(0);
    } 
    if ((y1<top) && (y2<top)) {
     return(0);
    } 
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    } 

    /*
     * Variable setup 
     */
    dr = int2sll(rad);
    deltaAngle = slldiv(SLL_CONST_3, dr);
    start_angle = sllmul(int2sll(start), SLL_CONST_2PI360);
    end_angle = sllmul(int2sll(end), SLL_CONST_2PI360);
    if (start > end) {
	end_angle = slladd(end_angle, SLL_CONST_2PI);
    }

    /* Count points (rather than calculate it) */
    numpoints = 1;
    angle = start_angle;
    while (angle <= end_angle) {
	angle = slladd(angle,deltaAngle);
	numpoints++;
    }

    /* Check size of array */
    if (numpoints == 1) {
	return (pixelColor(dst, x, y, color));
    } else if (numpoints == 2) {
	posX = x + sll2int(sllmul(dr, sllcos(start_angle)));
	posY = y + sll2int(sllmul(dr, sllsin(start_angle)));
	return (lineColor(dst, x, y, posX, posY, color));
    }

    /* Allocate vertex array */
    vx = vy = (Uint16 *) malloc(2 * sizeof(Uint16) * numpoints);
    if (vx == NULL) {
	return (-1);
    }
    vy += numpoints;

    /* Center */
    vx[0] = x;
    vy[0] = y;

    /* Calculate and store vertices */
    i = 1;
    angle = start_angle;
    while (angle <= end_angle) {
	vx[i] = x + sll2int(sllmul(dr, sllcos(angle)));
	vy[i] = y + sll2int(sllmul(dr, sllsin(angle)));
	angle = slladd(angle,deltaAngle);
	i++;
    }

    /* Draw */
    if (filled) {
     result = filledPolygonColor(dst, vx, vy, numpoints, color);
    } else {
     result = polygonColor(dst, vx, vy, numpoints, color);
    }

    /* Free vertex array */
    free(vx);

    return (result);
}

int pieColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, 
		Sint16 start, Sint16 end, Uint32 color) 
{
    return (doPieColor(dst, x, y, rad, start, end, color, 0));

}

int pieRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad,
	    Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    return (doPieColor(dst, x, y, rad, start, end,
			   ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a, 0));

}

int filledPieColor(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 color)
{
    return (doPieColor(dst, x, y, rad, start, end, color, 1));
}

int filledPieRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, Sint16 rad,
		  Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    return (doPieColor(dst, x, y, rad, start, end,
			   ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a, 1));
}

/* Trigon */

int trigonColor(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color)
{
 Sint16 vx[3]; 
 Sint16 vy[3];
 
 vx[0]=x1;
 vx[1]=x2;
 vx[2]=x3;
 vy[0]=y1;
 vy[1]=y2;
 vy[2]=y3;
 
 return(polygonColor(dst,vx,vy,3,color));
}

int trigonRGBA(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
				 Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
 Sint16 vx[3]; 
 Sint16 vy[3];
 
 vx[0]=x1;
 vx[1]=x2;
 vx[2]=x3;
 vy[0]=y1;
 vy[1]=y2;
 vy[2]=y3;
 
 return(polygonRGBA(dst,vx,vy,3,r,g,b,a));
}				 

/* AA-Trigon */

int aatrigonColor(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color)
{
 Sint16 vx[3]; 
 Sint16 vy[3];
 
 vx[0]=x1;
 vx[1]=x2;
 vx[2]=x3;
 vy[0]=y1;
 vy[1]=y2;
 vy[2]=y3;
 
 return(aapolygonColor(dst,vx,vy,3,color));
}

int aatrigonRGBA(SDL_Surface * dst,  Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
				   Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
 Sint16 vx[3]; 
 Sint16 vy[3];
 
 vx[0]=x1;
 vx[1]=x2;
 vx[2]=x3;
 vy[0]=y1;
 vy[1]=y2;
 vy[2]=y3;
 
 return(aapolygonRGBA(dst,vx,vy,3,r,g,b,a));
}				   

/* Filled Trigon */

int filledTrigonColor(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 color)
{
 Sint16 vx[3]; 
 Sint16 vy[3];
 
 vx[0]=x1;
 vx[1]=x2;
 vx[2]=x3;
 vy[0]=y1;
 vy[1]=y2;
 vy[2]=y3;
 
 return(filledPolygonColor(dst,vx,vy,3,color));
}

int filledTrigonRGBA(SDL_Surface * dst, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
				       Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
 Sint16 vx[3]; 
 Sint16 vy[3];
 
 vx[0]=x1;
 vx[1]=x2;
 vx[2]=x3;
 vy[0]=y1;
 vy[1]=y2;
 vy[2]=y3;
 
 return(filledPolygonRGBA(dst,vx,vy,3,r,g,b,a));
}

/* ---- Polygon */

int polygonColor(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color)
{
    int result;
    int i;
    const Sint16 *x1, *y1, *x2, *y2;

    /*
     * Sanity check 
     */
    if (n < 3) {
	return (-1);
    }

    /*
     * Pointer setup 
     */
    x1 = x2 = vx;
    y1 = y2 = vy;
    x2++;
    y2++;

    /*
     * Draw 
     */
    result = 0;
    for (i = 1; i < n; i++) {
	result |= lineColor(dst, *x1, *y1, *x2, *y2, color);
	x1 = x2;
	y1 = y2;
	x2++;
	y2++;
    }
    result |= lineColor(dst, *x1, *y1, *vx, *vy, color);

    return (result);
}

int polygonRGBA(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (polygonColor(dst, vx, vy, n, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ---- AA-Polygon */

int aapolygonColor(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color)
{
    int result;
    int i;
    const Sint16 *x1, *y1, *x2, *y2;

    /*
     * Sanity check 
     */
    if (n < 3) {
	return (-1);
    }

    /*
     * Pointer setup 
     */
    x1 = x2 = vx;
    y1 = y2 = vy;
    x2++;
    y2++;

    /*
     * Draw 
     */
    result = 0;
    for (i = 1; i < n; i++) {
	result |= aalineColorInt(dst, *x1, *y1, *x2, *y2, color, 0);
	x1 = x2;
	y1 = y2;
	x2++;
	y2++;
    }
    result |= aalineColorInt(dst, *x1, *y1, *vx, *vy, color, 0);

    return (result);
}

int aapolygonRGBA(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (aapolygonColor(dst, vx, vy, n, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ---- Filled Polygon */

int gfxPrimitivesCompareInt(const void *a, const void *b);

static int *gfxPrimitivesPolyInts = NULL;
static int gfxPrimitivesPolyAllocated = 0;

int filledPolygonColor(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint32 color)
{
    int result;
    int i;
    int y, xa, xb;
    int miny, maxy;
    int x1, y1;
    int x2, y2;
    int ind1, ind2;
    int ints;

    /*
     * Sanity check 
     */
    if (n < 3) {
	return -1;
    }

    /*
     * Allocate temp array, only grow array 
     */
    if (!gfxPrimitivesPolyAllocated) {
	gfxPrimitivesPolyInts = (int *) malloc(sizeof(int) * n);
	gfxPrimitivesPolyAllocated = n;
    } else {
	if (gfxPrimitivesPolyAllocated < n) {
	    gfxPrimitivesPolyInts = (int *) realloc(gfxPrimitivesPolyInts, sizeof(int) * n);
	    gfxPrimitivesPolyAllocated = n;
	}
    }

    /*
     * Determine Y maxima 
     */
    miny = vy[0];
    maxy = vy[0];
    for (i = 1; (i < n); i++) {
	if (vy[i] < miny) {
	    miny = vy[i];
	} else if (vy[i] > maxy) {
	    maxy = vy[i];
	}
    }

    /*
     * Draw, scanning y 
     */
    result = 0;
    for (y = miny; (y <= maxy); y++) {
	ints = 0;
	for (i = 0; (i < n); i++) {
	    if (!i) {
		ind1 = n - 1;
		ind2 = 0;
	    } else {
		ind1 = i - 1;
		ind2 = i;
	    }
	    y1 = vy[ind1];
	    y2 = vy[ind2];
	    if (y1 < y2) {
		x1 = vx[ind1];
		x2 = vx[ind2];
	    } else if (y1 > y2) {
		y2 = vy[ind1];
		y1 = vy[ind2];
		x2 = vx[ind1];
		x1 = vx[ind2];
	    } else {
		continue;
	    }
	    if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
		gfxPrimitivesPolyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
	    } 
	    
	}
	
	qsort(gfxPrimitivesPolyInts, ints, sizeof(int), gfxPrimitivesCompareInt);

	for (i = 0; (i < ints); i += 2) {
	    xa = gfxPrimitivesPolyInts[i] + 1;
	    xa = (xa >> 16) + ((xa & 32768) >> 15);
	    xb = gfxPrimitivesPolyInts[i+1] - 1;
	    xb = (xb >> 16) + ((xb & 32768) >> 15);
	    result |= hlineColor(dst, xa, xb, y, color);
	}
    }

    return (result);
}

int filledPolygonRGBA(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (filledPolygonColor
	    (dst, vx, vy, n, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

int gfxPrimitivesCompareInt(const void *a, const void *b)
{
    return (*(const int *) a) - (*(const int *) b);
}

/* ---- Character */

static SDL_Surface *gfxPrimitivesFont[256];
static Uint32 gfxPrimitivesFontColor[256];

/* Default is to use 8x8 internal font */
static const unsigned char *currentFontdata = gfxPrimitivesFontdata;

static int charWidth = 8, charHeight = 8;
static int charPitch = 1;
static int charSize = 8;	/* character data size in bytes */

void gfxPrimitivesSetFont(const void *fontdata, int cw, int ch)
{
    int i;

    if (fontdata) {
        currentFontdata = fontdata;
        charWidth = cw;
        charHeight = ch;
    } else {
        currentFontdata = gfxPrimitivesFontdata;
        charWidth = 8;
        charHeight = 8;
    }

    charPitch = (charWidth+7)/8;
    charSize = charPitch * charHeight;

    for (i = 0; i < 256; i++) {
	if (gfxPrimitivesFont[i]) {
	    SDL_FreeSurface(gfxPrimitivesFont[i]);
	    gfxPrimitivesFont[i] = NULL;
	}
    }
}

int characterColor(SDL_Surface * dst, Sint16 x, Sint16 y, char c, Uint32 color)
{
    Sint16 left, right, top, bottom;
    Sint16 x1, y1, x2, y2;
    SDL_Rect srect;
    SDL_Rect drect;
    int result;
    int ix, iy;
    const unsigned char *charpos;
    Uint8 *curpos;
    int forced_redraw;
    Uint8 patt, mask;

    /*
     * Get clipping boundary 
     */
    left = dst->clip_rect.x;
    right = dst->clip_rect.x + dst->clip_rect.w - 1;
    top = dst->clip_rect.y;
    bottom = dst->clip_rect.y + dst->clip_rect.h - 1;

    /*
     * Test if bounding box of character is visible 
     */
    x1 = x;
    x2 = x + charWidth;
    y1 = y;
    y2 = y + charHeight;
    if ((x1<left) && (x2<left)) {
     return(0);
    } 
    if ((x1>right) && (x2>right)) {
     return(0);
    } 
    if ((y1<top) && (y2<top)) {
     return(0);
    } 
    if ((y1>bottom) && (y2>bottom)) {
     return(0);
    } 

    /*
     * Setup source rectangle
     */
    srect.x = 0;
    srect.y = 0;
    srect.w = charWidth;
    srect.h = charHeight;

    /*
     * Setup destination rectangle
     */
    drect.x = x;
    drect.y = y;
    drect.w = charWidth;
    drect.h = charHeight;

    /*
     * Create new charWidth x charHeight bitmap surface if not already present 
     */
    if (gfxPrimitivesFont[(unsigned char) c] == NULL) {
	gfxPrimitivesFont[(unsigned char) c] =
	    SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_HWSURFACE | SDL_SRCALPHA,
                                 charWidth, charHeight, 32,
				 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	/*
	 * Check pointer 
	 */
	if (gfxPrimitivesFont[(unsigned char) c] == NULL) {
	    return (-1);
	}
	/*
	 * Definitely redraw 
	 */
	forced_redraw = 1;
    } else {
	forced_redraw = 0;
    }

    /*
     * Check if color has changed 
     */
    if ((gfxPrimitivesFontColor[(unsigned char) c] != color) || (forced_redraw)) {
	/*
	 * Redraw character 
	 */
	SDL_SetAlpha(gfxPrimitivesFont[(unsigned char) c], SDL_SRCALPHA, 255);
	gfxPrimitivesFontColor[(unsigned char) c] = color;

	/*
	 * Variable setup 
	 */
	charpos = currentFontdata + (unsigned char) c * charSize;
	curpos = (Uint8 *) gfxPrimitivesFont[(unsigned char) c]->pixels;

	/*
	 * Drawing loop 
	 */
        patt = 0;
	for (iy = 0; iy < charHeight; iy++) {
            mask = 0x00;
	    for (ix = 0; ix < charWidth; ix++) {
		if (!(mask >>= 1)) {
		    patt = *charpos++;
		    mask = 0x80;
		}

		if (patt & mask)
		    *(Uint32 *)curpos = color;
		else
		    *(Uint32 *)curpos = 0;
		curpos += 4;;
	    }
	}
    }

    /*
     * Draw bitmap onto destination surface 
     */
    result = SDL_BlitSurface(gfxPrimitivesFont[(unsigned char) c], &srect, dst, &drect);

    return (result);
}

int characterRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, char c, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (characterColor(dst, x, y, c, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

int stringColor(SDL_Surface * dst, Sint16 x, Sint16 y, const char *c, Uint32 color)
{
    int result = 0;
    int curx = x;
    const char *curchar = c;
 
    while (*curchar) {
	result |= characterColor(dst, curx, y, *curchar, color);
	curx += charWidth;
	curchar++;
    }

    return (result);
}

int stringRGBA(SDL_Surface * dst, Sint16 x, Sint16 y, const char *c, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (stringColor(dst, x, y, c, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}

/* ---- Bezier curve */

/*
 Calculate bezier interpolator of data array with ndata values at position 't'
*/

static sll evaluateBezier (sll *data, int ndata, sll t) 
{
 sll mu, result;
 int n,k,kn,nn,nkn;
 sll blend,muk,munk;
     
 /* Sanity check bounds */
 if (t<SLL_CONST_0) {
  return(data[0]);
 }
 if (t>=int2sll(ndata)) {
  return(data[ndata-1]);
 }
  
 /* Adjust t to the range 0.0 to 1.0 */ 
 mu=slldiv(t,int2sll(ndata));
 
 /* Calculate interpolate */
 n=ndata-1;
 result=SLL_CONST_0;
 muk = SLL_CONST_1;
#ifdef USE_FIXED_POINT
 munk = sllpow(sllsub(SLL_CONST_1,mu),int2sll(n));
#else
 munk = pow(sllsub(SLL_CONST_1,mu),int2sll(n));
#endif
 for (k=0;k<=n;k++) {
  nn = n;
  kn = k;
  nkn = n - k;
  blend = sllmul(muk, munk);
  muk = sllmul(muk,mu);
  munk = slldiv(munk,(sllsub(SLL_CONST_1,mu)));
  while (nn >= 1) {
   blend = sllmul(blend,int2sll(nn));
   nn--;
   if (kn > 1) {
    blend = slldiv(blend,int2sll(kn));
    kn--;
   }
   if (nkn > 1) {
    blend = slldiv(blend,int2sll(nkn));
    nkn--;
   }
  }
  result = slladd(result, sllmul(data[k], blend));
 }
                                                          
 return(result);
}

int bezierColor(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, int s, Uint32 color)
{
    int result;
    int i;
    sll *x, *y, t, stepsize;
    Sint16 x1, y1, x2, y2;

    /*
     * Sanity check 
     */
    if (n < 3) {
	return (-1);
    }
    if (s < 2) {
        return (-1);
    }
    
    /*
     * Variable setup 
     */
    stepsize=slldiv(SLL_CONST_1,int2sll(s));
    
    /* Transfer vertices into float arrays */
    if ((x=(sll *)malloc(sizeof(sll)*(n+1)))==NULL) {
     return(-1);
    }
    if ((y=(sll *)malloc(sizeof(sll)*(n+1)))==NULL) {
     free(x);
     return(-1);
    }    
    for (i=0; i<n; i++) {
     x[i]=int2sll(vx[i]);
     y[i]=int2sll(vy[i]);
    }      
    x[n]=int2sll(vx[0]);
    y[n]=int2sll(vy[0]);
    
    /*
     * Draw 
     */
    result = 0;
    t=SLL_CONST_0;
    x1=evaluateBezier(x,n+1,t);
    y1=evaluateBezier(y,n+1,t);
    for (i = 0; i <= (n*s); i++) {
	t = slladd(t,stepsize);
	x2=(Sint16)evaluateBezier(x,n,t);
	y2=(Sint16)evaluateBezier(y,n,t);
	result |= lineColor(dst, x1, y1, x2, y2, color);
	x1 = x2;
	y1 = y2;
    }

    /* Clean up temporary array */
    free(x);
    free(y);
    
    return (result);
}

int bezierRGBA(SDL_Surface * dst, const Sint16 * vx, const Sint16 * vy, int n, int s, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    /*
     * Draw 
     */
    return (bezierColor(dst, vx, vy, n, s, ((Uint32) r << 24) | ((Uint32) g << 16) | ((Uint32) b << 8) | (Uint32) a));
}
