#include "../tomb3/pch.h"
#include "output.h"
#include "../3dsystem/3d_gen.h"
#include "transform.h"
#include "hwrender.h"
#include "../3dsystem/hwinsert.h"
#include "picture.h"
#include "dxshell.h"
#include "display.h"
#include "time.h"
#include "game.h"
#include "../3dsystem/phd_math.h"
#include "dd.h"
#include "file.h"
#include "winmain.h"
#include "../game/gameflow.h"
#include "../game/draw.h"
#include "../game/inventry.h"
#include "../game/invfunc.h"

static short shadow[6 + (3 * 8)] =
{
	0, 0, 0,		//x, y, z
	32767, 1, 8,	//size, nPolys, nVtx
	0, 0, 0,		//8 vtx-> x, y, z
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
};

long framedump;
long SunsetTimer;
long water_effect;
bool bBlueEffect;
static long shade_effect;
static long light_level[4];
static long objbcnt;

void S_PrintShadow(short size, short* box, ITEM_INFO* item)
{
	long x0, x1, z0, z1, midX, midZ, xAdd, zAdd;

	x0 = box[0];
	x1 = box[1];
	z0 = box[4];
	z1 = box[5];
	midX = (x0 + x1) / 2;
	xAdd = (x1 - x0) * size / 1024;
	midZ = (z0 + z1) / 2;
	zAdd = (z1 - z0) * size / 1024;

	shadow[6] = short(midX - xAdd);
	shadow[8] = short(midZ + zAdd * 2);

	shadow[9] = short(midX + xAdd);
	shadow[11] = short(midZ + zAdd * 2);

	shadow[12] = short(midX + xAdd * 2);
	shadow[14] = short(midZ + zAdd);

	shadow[15] = short(midX + xAdd * 2);
	shadow[17] = short(midZ - zAdd);

	shadow[18] = short(midX + xAdd);
	shadow[20] = short(midZ - zAdd * 2);

	shadow[21] = short(midX - xAdd);
	shadow[23] = short(midZ - zAdd * 2);

	shadow[24] = short(midX - xAdd * 2);
	shadow[26] = short(midZ - zAdd);

	shadow[27] = short(midX - xAdd * 2);
	shadow[29] = short(midZ + zAdd);

	phd_leftfloat = float(phd_winxmin + phd_left);
	phd_topfloat = float(phd_winymin + phd_top);
	phd_rightfloat = float(phd_right + phd_winxmin + 1);
	phd_bottomfloat = float(phd_bottom + phd_winymin + 1);
	f_centerx = float(phd_winxmin + phd_centerx);
	f_centery = float(phd_winymin + phd_centery);

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->floor, item->pos.z_pos);
	phd_RotY(item->pos.y_rot);

	if (calc_object_vertices(&shadow[4]))
		InsertTrans8(vbuf, 32);

	phd_PopMatrix();
}

void S_SetupAboveWater(long underwater)
{
	water_effect = underwater;
	shade_effect = 0;
	bBlueEffect = underwater;
}

void S_SetupBelowWater(long underwater)
{
	if (wet != underwater)
		wet = underwater;

	shade_effect = 1;
	water_effect = !underwater;
	bBlueEffect = 1;
}

void S_OutputPolyList()
{
	if (App.lpZBuffer)
	{
		HWR_EnableColorKey(0);
		HWR_EnableAlphaBlend(0);
		HWR_EnableColorAddition(0);
		HWR_EnableZBuffer(1, 1);
		HWR_SetCurrentTexture(0);

		if (bAlphaTesting)
		{
			SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, 0);
			DrawBuckets();
			SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, 1);
			phd_SortPolyList(surfacenumbf, sort3d_bufferbf);
			HWR_DrawPolyListBF(surfacenumbf, sort3d_bufferbf);
			HWR_EnableZBuffer(0, 1);
			SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, 0);
			phd_SortPolyList(surfacenumfb, sort3d_bufferfb);
			HWR_DrawPolyListBF(surfacenumfb, sort3d_bufferfb);
		}
		else
		{
			DrawBuckets();
			phd_SortPolyList(surfacenumbf, sort3d_bufferbf);
			HWR_DrawPolyListBF(surfacenumbf, sort3d_bufferbf);
		}
	}
	else
	{
		HWR_EnableColorKey(0);
		HWR_EnableAlphaBlend(0);
		phd_SortPolyList(surfacenumbf, sort3d_bufferbf);
		HWR_DrawPolyList(surfacenumbf, sort3d_bufferbf);
	}

	S_FadePicture();
	HWR_EndScene();
}

void S_LightRoom(ROOM_INFO* r)
{
	short* ptr;
	long level;

	if (!r->lighting)
		return;

	level = light_level[r->lighting];
	ptr = r->data;

	for (int i = (long)*ptr++; i > 0; i++, ptr += 6)
		((uchar*)ptr)[2] += uchar((level * (((uchar*)ptr)[3] & 0x1F)) >> 6);
}

void S_InsertBackPolygon(long xmin, long ymin, long xmax, long ymax, long col)
{
	InsertFlatRect(phd_winxmin + xmin, phd_winymin + ymin, phd_winxmin + xmax, phd_winymin + ymax, phd_zfar, inv_colours[0]);
}

long S_GetObjectBounds(short* box)
{
	long* v;
	long vtx[8][3];
	long xmin, xmax, ymin, ymax, zmin, zmax, nZ, x, y, z;

	if (phd_mxptr[M23] >= phd_zfar && !outside)
		return 0;

	objbcnt++;
	xmin = box[0];
	xmax = box[1];
	ymin = box[2];
	ymax = box[3];
	zmin = box[4];
	zmax = box[5];

	vtx[0][0] = xmin;
	vtx[0][1] = ymin;
	vtx[0][2] = zmin;

	vtx[1][0] = xmax;
	vtx[1][1] = ymin;
	vtx[1][2] = zmin;

	vtx[2][0] = xmax;
	vtx[2][1] = ymax;
	vtx[2][2] = zmin;

	vtx[3][0] = xmin;
	vtx[3][1] = ymax;
	vtx[3][2] = zmin;

	vtx[4][0] = xmin;
	vtx[4][1] = ymin;
	vtx[4][2] = zmax;

	vtx[5][0] = xmax;
	vtx[5][1] = ymin;
	vtx[5][2] = zmax;

	vtx[6][0] = xmax;
	vtx[6][1] = ymax;
	vtx[6][2] = zmax;

	vtx[7][0] = xmin;
	vtx[7][1] = ymax;
	vtx[7][2] = zmax;
	
	xmin = 0x3FFFFFFF;
	xmax = -0x3FFFFFFF;
	ymin = 0x3FFFFFFF;
	ymax = -0x3FFFFFFF;
	v = &vtx[0][0];
	nZ = 0;

	for (int i = 0; i < 8; i++)
	{
		z = v[0] * phd_mxptr[M20] + v[1] * phd_mxptr[M21] + v[2] * phd_mxptr[M22] + phd_mxptr[M23];

		if (z > phd_znear && z < phd_zfar)
		{
			nZ++;
			z /= phd_persp;
			x = (v[0] * phd_mxptr[M00] + v[1] * phd_mxptr[M01] + v[2] * phd_mxptr[M02] + phd_mxptr[M03]) / z;

			if (x < xmin)
				xmin = x;

			if (x > xmax)
				xmax = x;

			y = (v[0] * phd_mxptr[M10] + v[1] * phd_mxptr[M11] + v[2] * phd_mxptr[M12] + phd_mxptr[M13]) / z;

			if (y < ymin)
				ymin = y;

			if (y > ymax)
				ymax = y;
		}

		v += 3;
	}

	xmin += phd_centerx;
	xmax += phd_centerx;
	ymin += phd_centery;
	ymax += phd_centery;

	if (nZ < 8 || xmin < 0 || ymin < 0 || xmax > phd_winxmax || ymax > phd_winymax)
		return -1;

	if (xmin > phd_right || ymin > phd_bottom || xmax < phd_left || ymax < phd_top)
		return 0;

	return 1;
}

void mCalcPoint(long x, long y, long z, long* result)
{
	x -= w2v_matrix[M03];
	y -= w2v_matrix[M13];
	z -= w2v_matrix[M23];
	result[0] = (w2v_matrix[M00] * x + w2v_matrix[M01] * y + w2v_matrix[M02] * z) >> W2V_SHIFT;
	result[1] = (w2v_matrix[M10] * x + w2v_matrix[M11] * y + w2v_matrix[M12] * z) >> W2V_SHIFT;
	result[2] = (w2v_matrix[M20] * x + w2v_matrix[M21] * y + w2v_matrix[M22] * z) >> W2V_SHIFT;
}

void ProjectPCoord(long x, long y, long z, long* result, long cx, long cy, long fov)
{
	if (z > 0)
	{
		result[0] = cx + x * fov / z;
		result[1] = cy + y * fov / z;
		result[2] = z;
	}
	else if (z < 0)
	{
		result[0] = cx - x * fov / z;
		result[1] = cy - y * fov / z;
		result[2] = z;
	}
	else
	{
		result[0] = cx + x * fov;
		result[1] = cy + y * fov;
		result[2] = z;
	}
}

long S_DumpCine()
{
	static long nf = 0;

	if (framedump)
	{
		nf++;
		DXSaveScreen(App.lpFrontBuffer);
		return 1;
	}

	return 0;
}

void S_InitialiseScreen(long type)
{
	if (type > 0)
		TempVideoRemove();

	//Empty function call here

	if (App.nRenderMode)
		HWR_InitState();
}

void ScreenPartialDump()
{
	DXUpdateFrame(1, &phd_WindowRect);
}

long S_DumpScreen()
{
	long nFrames;

	if (framedump)
		nFrames = 2;
	else
	{
		nFrames = Sync();

		while (nFrames < 2)
		{
			while (!Sync());
			nFrames++;
		}
	}

	ScreenPartialDump();
	return nFrames;
}

void ScreenClear(bool a)
{
	DXClearBuffers(2, 0);
}

void S_ClearScreen()
{
	ScreenClear(0);
}

void AnimateTextures(long n)
{
	PHDTEXTURESTRUCT tex;
	short* range;
	static long comp;
	short nRanges, nRangeFrames;

	for (comp += n; comp > 5; comp -= 5)
	{
		nRanges = *aranges;
		range = aranges + 1;

		for (int i = 0; i < nRanges; i++)
		{
			nRangeFrames = *range++;

			tex = phdtextinfo[range[0]];

			while (nRangeFrames > 0)
			{
				phdtextinfo[range[0]] = phdtextinfo[range[1]];
				range++;
				nRangeFrames--;
			}

			phdtextinfo[range[0]] = tex;
			range++;
		}
	}
}

void S_AnimateTextures(long n)
{
	static long pulse;

	pulse = (pulse + n) & 0x1F;
	light_level[1] = GetRandomDraw() & 0x1F;
	light_level[2] = ((phd_sin((pulse << 16) / 32) + 0x4000) * 31) >> 15;

	if (GF_SunsetEnabled)
	{
		SunsetTimer += n;

		if (SunsetTimer < 72000)
			light_level[3] = 31 * SunsetTimer / 72000;
		else
			light_level[3] = 31;
	}

	AnimateTextures(n);
}

void S_InitialisePolyList(bool clearBackBuffer)
{
	ulong flags;

	nPolyType = 0;

	if (GtFullScreenClearNeeded)
	{
		DXCheckForLostSurfaces();
		DD_SpinMessageLoop(0);
		DXDoFlipWait();
		DXClearBuffers(3, 0);
		GtFullScreenClearNeeded = 0;
		clearBackBuffer = 0;
	}

	if (!App.nRenderMode)
		flags = 272;
	else
	{
		if (clearBackBuffer || HWConfig.nFillMode < D3DFILL_SOLID)
			flags = 258;
		else
			flags = 256;

		flags |= 8;
	}

	DXClearBuffers(flags, 0);
	HWR_BeginScene();
	phd_InitPolyList();
	objbcnt = 0;
}
