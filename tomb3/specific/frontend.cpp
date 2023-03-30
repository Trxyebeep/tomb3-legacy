#include "../tomb3/pch.h"
#include "frontend.h"
#include "specific.h"
#include "../3dsystem/3d_gen.h"
#include "display.h"
#include "input.h"
#include "time.h"
#include "fmv.h"
#include "../game/inventry.h"
#include "file.h"

ushort S_COLOUR(long r, long g, long b)
{
	return SWR_FindNearestPaletteEntry(game_palette, r, g, b, 0);
}

void S_DrawScreenLine(long x, long y, long z, long w, long h, long c, ushort* grdptr, ushort f)
{
	InsertLine(x, y, x + w, y + h, phd_znear + 8 * z, (char)c, c);
}

void S_DrawScreenBox(long x, long y, long z, long w, long h, long sprnum, ushort* grdptr, ushort f)
{
	long sx, sy;

	sx = x + w;
	sy = y + h;
	S_DrawScreenLine(x,			y - 1,	z, w + 1,	0,		15, 0, f);
	S_DrawScreenLine(x + 1,		y,		z, w - 1,	0,		31, 0, f);
	S_DrawScreenLine(sx,		y + 1,	z, 0,		h - 1,	15, 0, f);
	S_DrawScreenLine(sx + 1,	y,		z, 0,		h + 1,	31, 0, f);
	S_DrawScreenLine(x - 1,		y - 1,	z, 0,		h + 1,	15, 0, f);
	S_DrawScreenLine(x,			y,		z, 0,		h - 1,	31, 0, f);
	S_DrawScreenLine(x,			sy,		z, w - 1,	0,		15, 0, f);
	S_DrawScreenLine(x - 1,		sy + 1, z, w + 1,	0,		31, 0, f);
}

void S_DrawScreenFBox(long x, long y, long z, long w, long h, long c, ushort* grdptr, ushort f)
{
	long adder;

	adder = 1;
	w += adder;
	h += adder;
	InsertTransQuad(phd_winxmin + x, phd_winymin + y, w, h, phd_znear + 0x50000);
}

void S_FinishInventory()
{
	if (Inventory_Mode != INV_TITLE_MODE)
		TempVideoRemove();
}

void S_Wait(long nFrames, long skip)
{
	long s;

	if (skip)
	{
		while (nFrames > 0)
		{
			if (!input)
				break;

			S_UpdateInput();

			do { s = Sync(); } while (!s);

			nFrames -= s;
		}
	}

	while (nFrames > 0)
	{
		S_UpdateInput();

		if (skip && input)
			break;

		do { s = Sync(); } while (!s);

		nFrames -= s;
	}
}

long S_PlayFMV(char* name)
{
	return FMV_Play(name);
}

long S_IntroFMV(char* name1, char* name2)
{
	return FMV_PlayIntro(name1, name2);
}
