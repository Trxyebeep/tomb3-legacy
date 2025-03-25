#include "../tomb3/pch.h"
#include "draweffects.h"
#include "../3dsystem/3d_gen.h"
#include "../game/sphere.h"
#include "../3dsystem/hwinsert.h"
#include "game.h"
#include "../game/objects.h"
#include "../3dsystem/phd_math.h"
#include "../game/control.h"
#include "../game/lasers.h"
#include "../game/triboss.h"
#include "../game/londboss.h"
#include "../game/tonyboss.h"
#include "../game/willboss.h"
#include "output.h"
#include "transform.h"
#include "../game/fish.h"
#include "../game/effect2.h"
#include "file.h"
#include "../game/laraelec.h"
#include "../game/setup.h"
#include "winmain.h"
#include "../game/effects.h"
#include "../game/lara.h"

static BITE_INFO NodeOffsets[16] =
{
	{0, 340, 64, 7},
	{0, 0, -96, 10},
	{16, 48, 320, 13},
	{0, -256, 0, 5},
	{0, 64, 0, 10},
	{0, 64, 0, 13},
	{-32, -16, -192, 13},
	{-64, 410, 0, 20},
	{64, 410, 0, 23},
	{-160, -8, 16, 5},
	{-160, -8, 16, 9},
	{-160, -8, 16, 13},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

static short BatMesh[5][3] =
{
	{-192, 0, -48},
	{-192, 0, 48},
	{96, 0, 0},
	{-144, 0, -192},
	{-144, 0, 192}
};

static uchar SplashLinks[32]
{
	32, 36, 0, 4,
	36, 40, 4, 8,
	40, 44, 8, 12,
	44, 48, 12, 16,
	48, 52, 16, 20,
	52, 56, 20, 24,
	56, 60, 24, 28,
	60, 32, 28, 0
};

static char BoltLinks[20][4] =
{
	{0, 0, 1, 2}, {0, 0, 2, 3}, {0, 0, 3, 4}, {0, 0, 4, 1},
	{1, 2, 5, 6}, {2, 3, 6, 7}, {3, 4, 7, 8}, {4, 1, 8, 5},
	{5, 6, 9, 10}, {6, 7, 10, 11}, {7, 8, 11, 12}, {8, 5, 12, 9},
	{9, 10, 13, 14}, {10, 11, 14, 15}, {11, 12, 15, 16}, {12, 9, 16, 13},
	{13, 14, 17, 17}, {14, 15, 17, 17}, {15, 16, 17, 17}, {16, 13, 17, 17}
};

static char BoltSummonLinks[8][4] =
{
	{0, 0, 1, 2}, {0, 0, 2, 3}, {0, 0, 3, 4}, {0, 0, 4, 1},
	{1, 2, 5, 5}, {2, 3, 5, 5}, {3, 4, 5, 5}, {4, 1, 5, 5}
};

static uchar BatLinks[9] = { 0, 2, 4, 6, 0, 4, 2, 8, 4 };

static RAINDROP raindrops[MAX_WEATHER];
static SNOWFLAKE snowflakes[MAX_WEATHER];

EXPLOSION_RING ExpRings[6];
WAKE_PTS WakePts[32][2];
uchar WakeShade;
uchar CurrentStartWake;

static __inline void ClipCheckPoint(PHD_VBUF* v, long x, long y, long z)
{
	char clipFlag;

	clipFlag = 0;

	if (x < phd_winxmin)
		clipFlag++;
	else if (x > phd_winxmax)
		clipFlag += 2;

	if (y < phd_winymin)
		clipFlag += 4;
	else if (y > phd_winymax)
		clipFlag += 8;

	v->clip = clipFlag;
	v->xs = (float)x;
	v->ys = (float)y;
	v->zv = (float)z;
	v->ooz = f_persp / (float)z * f_oneopersp;
}

static __inline void setColor(PHD_VBUF* v, long c, char flag)
{
	long r, g, b;

	if (!flag)
	{
		r = (c >> 3) & 0x1F;
		g = (c >> 11) & 0x1F;
		b = (c >> 19) & 0x1F;
		v->g = short(r << 10 | g << 5 | b);
	}
	else
		v->g = (short)c;
}

static __inline void setXYZ3(PHD_VBUF* v, char cFlag,
	long x1, long y1, long z1, long c1,
	long x2, long y2, long z2, long c2,
	long x3, long y3, long z3, long c3)

{
	ClipCheckPoint(&v[0], x1, y1, z1);
	ClipCheckPoint(&v[1], x2, y2, z2);
	ClipCheckPoint(&v[2], x3, y3, z3);
	setColor(&v[0], c1, cFlag);
	setColor(&v[1], c2, cFlag);
	setColor(&v[2], c3, cFlag);
}

static __inline void setXYZ4(PHD_VBUF* v, char cFlag,
	long x1, long y1, long z1, long c1,
	long x2, long y2, long z2, long c2,
	long x3, long y3, long z3, long c3,
	long x4, long y4, long z4, long c4)
{
	ClipCheckPoint(&v[0], x1, y1, z1);
	ClipCheckPoint(&v[1], x2, y2, z2);
	ClipCheckPoint(&v[2], x3, y3, z3);
	ClipCheckPoint(&v[3], x4, y4, z4);

	setColor(&v[0], c1, cFlag);
	setColor(&v[1], c2, cFlag);
	setColor(&v[3], c4, cFlag);
}

void LaraElectricDeath(long lr, ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	PHD_VECTOR pos;
	PHD_VECTOR pos1;
	PHD_VECTOR pos2;
	PHD_VECTOR point;
	short* points;
	float zv;
	long w, h, nDs, mesh1, mesh2, x, y, z, xStep, yStep, zStep, pX, pY, pZ;
	long x1, y1, x2, y2, c0, c1;
	long coords[600];
	short distances[200];

	item = lara_item;

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;
	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	nDs = 0;

	for (int i = 0; i < 14; i++)
	{
		mesh1 = lara_meshes[2 * i];
		mesh2 = lara_meshes[2 * i + 1];
		points = &electricity_points[(5 * i) & 0xF][0];

		pos1.y = 0;

		if (lr)
		{
			pos1.x = -48;
			pos1.z = -48;
		}
		else
		{
			pos1.x = 48;
			pos1.z = 48;
		}

		GetJointAbsPosition(item, &pos1, mesh1);
		pos2.y = 0;

		if (!lara_last_points[i] || i == 13)
		{
			if (lr)
			{
				pos2.z = -48;
				pos2.x = -48;
			}
			else
			{
				pos2.x = 48;
				pos2.z = 48;
			}

			if (i == 13)
				pos2.y = -64;
		}
		else
		{
			pos2.x = 0;
			pos2.z = 0;
		}

		GetJointAbsPosition(item, &pos2, mesh2);
		pos1.x -= item->pos.x_pos;
		pos1.y -= item->pos.y_pos;
		pos1.z -= item->pos.z_pos;
		pos2.x -= item->pos.x_pos;
		pos2.y -= item->pos.y_pos;
		pos2.z -= item->pos.z_pos;
		x = pos1.x;
		y = pos1.y;
		z = pos1.z;
		xStep = (pos2.x - pos1.x) >> 2;
		yStep = (pos2.y - pos1.y) >> 2;
		zStep = (pos2.z - pos1.z) >> 2;

		for (int j = 0; j < 5; j++)
		{
			if (j == 4 && !lara_last_points[i])
				break;

			if (j == 4 && lara_last_points[i])
			{
				pos.x = pos2.x;
				pos.y = pos2.y;
				pos.z = pos2.z;
			}
			else
			{
				pos.x = x;
				pos.y = y;
				pos.z = z;
			}

			if (!j || j == 4)
				distances[nDs] = 0;
			else
			{
				point.x = *points++;
				point.y = *points++;
				point.z = *points++;
				points += 3;	//dont need vels

				if (lr)
				{
					pos.x -= point.x >> 3;
					pos.y -= point.y >> 3;
					pos.z -= point.z >> 3;
				}
				else
				{
					pos.x += point.x >> 3;
					pos.y += point.y >> 3;
					pos.z += point.z >> 3;
				}

				pX = abs(point.x);
				pY = abs(point.y);
				pZ = abs(point.z);

				if (pY > pX)
					pX = pY;

				if (pZ > pX)
					pX = pZ;

				distances[nDs] = (short)pX;
			}

			pX = phd_mxptr[M00] * pos.x + phd_mxptr[M01] * pos.y + phd_mxptr[M02] * pos.z + phd_mxptr[M03];
			pY = phd_mxptr[M10] * pos.x + phd_mxptr[M11] * pos.y + phd_mxptr[M12] * pos.z + phd_mxptr[M13];
			pZ = phd_mxptr[M20] * pos.x + phd_mxptr[M21] * pos.y + phd_mxptr[M22] * pos.z + phd_mxptr[M23];
			zv = f_persp / (float)pZ;
			coords[3 * nDs] = short(float(pX * zv + f_centerx));
			coords[3 * nDs + 1] = short(float(pY * zv + f_centery));
			coords[3 * nDs + 2] = pZ >> W2V_SHIFT;
			nDs++;
			x += xStep;
			y += yStep;
			z += zStep;
		}
	}

	nDs = 0;

	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < lara_line_counts[i]; j++)
		{
			x1 = coords[3 * nDs];
			y1 = coords[3 * nDs + 1];
			z = coords[3 * nDs + 2];
			x2 = coords[3 * nDs + 3];
			y2 = coords[3 * nDs + 4];
			c0 = distances[nDs];
			c1 = distances[nDs + 1];
			nDs++;

			if (z < 32)
				continue;

			if (c0 > 255)
			{
				c0 = 511 - c0;

				if (c0 < 0)
					c0 = 0;
			}

			if (c1 > 255)
			{
				c1 = 511 - c1;

				if (c1 < 0)
					c1 = 0;
			}

			if (lr)
			{
				c0 >>= 1;
				c1 >>= 1;
			}

			if (ClipLine(x1, y1, x2, y2, w, h) && x1 >= 0 && x1 <= w && y1 >= 0 && y1 <= h && x2 >= 0 && x2 <= w && y2 >= 0 && y2 <= h)
			{
				c0 = c0 | (c0 << 8);
				c1 = c1 | (c1 << 8);
				z <<= W2V_SHIFT;
				xStep = GlobalAlpha;
				GlobalAlpha = 0x70000000;
				HWI_InsertLine_Sorted(x1 - phd_winxmin, y1 - phd_winymin, x2 - phd_winxmin, y2 - phd_winymin, z, c0, c1);
				GlobalAlpha = xStep;
			}
		}

		nDs++;
	}

	phd_PopMatrix();
}

bool ClipLine(long& x1, long& y1, long& x2, long& y2, long w, long h)
{
	float clip;
	long xMax, xMin, yMax, yMin;

	xMax = phd_winxmin + phd_winxmax;
	xMin = phd_winxmin;
	yMax = phd_winymin + phd_winymax;
	yMin = phd_winymin;

	if (x1 < xMin && x2 < xMin || y1 < yMin && y2 < yMin)
		return 0;

	if (x1 > xMax && x2 > xMax || y1 > yMax && y2 > yMax)
		return 0;

	if (x1 > xMax)
	{
		clip = ((float)xMax - x2) / float(x1 - x2);
		x1 = xMax;
		y1 = long((y1 - y2) * clip + y2);
	}

	if (x2 > xMax)
	{
		clip = ((float)xMax - x1) / float(x2 - x1);
		x2 = xMax;
		y2 = long((y2 - y1) * clip + y1);
	}

	if (x1 < xMin)
	{
		clip = ((float)xMin - x1) / float(x2 - x1);
		x1 = xMin;
		y1 = long((y2 - y1) * clip + y1);
	}

	if (x2 < xMin)
	{
		clip = ((float)xMin - x2) / float(x1 - x2);
		x2 = xMin;
		y2 = long((y1 - y2) * clip + y2);
	}

	if (y1 > yMax)
	{
		clip = ((float)yMax - y2) / float(y1 - y2);
		y1 = yMax;
		x1 = long((x1 - x2) * clip + x2);
	}

	if (y2 > yMax)
	{
		clip = ((float)yMax - y1) / float(y2 - y1);
		y2 = yMax;
		x2 = long((x2 - x1) * clip + x1);
	}

	if (y1 < yMin)
	{
		clip = ((float)yMin - y1) / float(y2 - y1);
		y1 = yMin;
		x1 = long((x2 - x1) * clip + x1);
	}

	if (y2 < yMin)
	{
		clip = ((float)yMin - y2) / float(y1 - y2);
		y2 = yMin;
		x2 = long((x1 - x2) * clip + x2);
	}

	return 1;
}

void S_DrawWakeFX(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	WAKE_PTS* pt;
	PHD_VECTOR pos;
	long* pZ;
	short* pXY;
	uchar* pRGBs;
	float zv;
	long w, h, s, c, current, nw, s1, s2, s3, s4;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c12, c34, cval;
	long offsets[2][2];
	long Z[64];
	short XY[128];
	uchar rgbs[128];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;
	offsets[0][0] = -128;
	offsets[0][1] = 0;
	offsets[1][0] = 128;
	offsets[1][1] = 0;

	phd_PushMatrix();
	phd_TranslateAbs(lara_item->pos.x_pos, lara_item->pos.y_pos, lara_item->pos.z_pos);

	for (int i = 0; i < 2; i++)
	{
		pXY = XY;
		pZ = Z;
		pRGBs = rgbs;
		s = phd_sin(item->pos.y_rot);
		c = phd_cos(item->pos.y_rot);
		x1 = (item->pos.x_pos + ((offsets[i][1] * s + offsets[i][0] * c) >> W2V_SHIFT)) - lara_item->pos.x_pos;
		y1 = item->pos.y_pos - lara_item->pos.y_pos;
		z1 = (item->pos.z_pos + ((offsets[i][1] * c - offsets[i][0] * s) >> W2V_SHIFT)) - lara_item->pos.z_pos;

		pos.x = phd_mxptr[M00] * x1 + phd_mxptr[M01] * y1 + phd_mxptr[M02] * z1 + phd_mxptr[M03];
		pos.y = phd_mxptr[M10] * x1 + phd_mxptr[M11] * y1 + phd_mxptr[M12] * z1 + phd_mxptr[M13];
		pos.z = phd_mxptr[M20] * x1 + phd_mxptr[M21] * y1 + phd_mxptr[M22] * z1 + phd_mxptr[M23];

		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));

		pXY[0] = (short)pos.x;
		pXY[1] = (short)pos.y;
		pXY += 2;
		*pZ++ = pos.z;
		pXY[0] = (short)pos.x;
		pXY[1] = (short)pos.y;
		pXY += 2;
		*pZ++ = pos.z;
		*pRGBs++ = 64;
		current = (CurrentStartWake - 1) & 0x1F;

		for (nw = 0; nw < 32; nw++)
		{
			pt = &WakePts[current][i];

			if (pt->life)
			{
				for (int k = 0; k < 2; k++)
				{
					x1 = pt->x[k] - lara_item->pos.x_pos;
					y1 = pt->y - lara_item->pos.y_pos;
					z1 = pt->z[k] - lara_item->pos.z_pos;
					pos.x = phd_mxptr[M00] * x1 + phd_mxptr[M01] * y1 + phd_mxptr[M02] * z1 + phd_mxptr[M03];
					pos.y = phd_mxptr[M10] * x1 + phd_mxptr[M11] * y1 + phd_mxptr[M12] * z1 + phd_mxptr[M13];
					pos.z = phd_mxptr[M20] * x1 + phd_mxptr[M21] * y1 + phd_mxptr[M22] * z1 + phd_mxptr[M23];

					zv = f_persp / (float)pos.z;
					pos.x = short(float(pos.x * zv + f_centerx));
					pos.y = short(float(pos.y * zv + f_centery));

					pXY[0] = (short)pos.x;
					pXY[1] = (short)pos.y;
					pXY += 2;
					*pZ++ = pos.z;
				}

				*pRGBs++ = pt->life;
			}
			else
			{
				*pZ = -5555;
				break;
			}

			current = (current - 1) & 0x1F;
		}

		if (!nw)
			break;

		pXY = XY;
		pZ = Z;
		pRGBs = rgbs;
		x1 = *pXY++;
		y1 = *pXY++;
		x2 = *pXY++;
		y2 = *pXY++;
		z1 = *pZ++;
		z2 = *pZ++;
		c12 = *pRGBs++;

		if (WakeShade < 16)
			c12 = (c12 * WakeShade) >> 4;

		for (;;)
		{
			z3 = *pZ++;

			if (z3 == -5555)
				break;

			x3 = *pXY++;
			y3 = *pXY++;
			x4 = *pXY++;
			y4 = *pXY++;
			z4 = *pZ++;
			c34 = *pRGBs++;

			if (WakeShade < 16)
				c34 = (c34 * WakeShade) >> 4;

			z1 = (z1 + z2 + z3 + z4) >> 2;

			if ((z1 >> W2V_SHIFT) > 32 &&
				x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 &&
				x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
				y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 &&
				y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
			{
				cval = c12 >> 2;
				s1 = cval << 10 | cval << 5 | cval;

				cval = c12 >> 1;
				s2 = cval << 10 | cval << 5 | cval;

				cval = c34 >> 1;
				s3 = cval << 10 | cval << 5 | cval;

				cval = c34 >> 2;
				s4 = cval << 10 | cval << 5 | cval;

				HWI_InsertAlphaSprite_Sorted(x1, y1, z1, s1, x2, y2, z2, s2, x3, y3, z3, s3, x4, y4, z4, s4, -1, 16, 1);
			}

			y1 = y4;
			x1 = x4;
			z1 = z4;
			x2 = x3;
			y2 = y3;
			z2 = z3;
			c12 = c34;
		}
	}

	phd_PopMatrix();
}

void DoRain()
{
	RAINDROP* rptr;
	PHD_VECTOR pos;
	long* pZ;
	short* pXY;
	float zv;
	long rad, angle, rnd, alpha;
	long tx, ty, tz, x1, y1, x2, y2, z;
	long Z[2];
	short XY[4];

	for (int i = 0, num_alive = 0; i < MAX_WEATHER; i++)
	{
		rptr = &raindrops[i];

		if (!rptr->x && num_alive < MAX_WEATHER_ALIVE)
		{
			num_alive++;
			rad = GetRandomDraw() & 0xFFF;
			angle = GetRandomDraw() & 0x1FFE;
			rptr->x = lara_item->pos.x_pos + (rad * rcossin_tbl[angle] >> 12);
			rptr->y = lara_item->pos.y_pos -1024 - (GetRandomDraw() & 0x7FF);
			rptr->z = lara_item->pos.z_pos + (rad * rcossin_tbl[angle + 1] >> 12);

			if (IsRoomOutside(rptr->x, rptr->y, rptr->z) < 0)
			{
				rptr->x = 0;
				continue;
			}

			rptr->yv = (GetRandomDraw() & 7) + 16;
			rptr->xv = (GetRandomDraw() & 7) - 4;
			rptr->zv = (GetRandomDraw() & 7) - 4;
			rptr->life = 88 - (rptr->yv << 1);
		}

		if (rptr->x)
		{
			if (IsRoomOutside(rptr->x, rptr->y, rptr->z) == -2 || (room[IsRoomOutsideNo].flags & ROOM_UNDERWATER) ||
				rptr->life > 240 || abs(CamPos.x - rptr->x) > 6000 || abs(CamPos.z - rptr->z) > 6000)
			{
				rptr->x = 0;
				continue;
			}

			rptr->x += rptr->xv + 4 * SmokeWindX;
			rptr->y += rptr->yv << 3;
			rptr->z += rptr->zv + 4 * SmokeWindZ;
			rnd = GetRandomDraw();

			if ((rnd & 3) != 3)
			{
				rptr->xv += (rnd & 3) - 1;

				if (rptr->xv < -4)
					rptr->xv = -4;
				else if (rptr->xv > 4)
					rptr->xv = 4;
			}

			rnd = (rnd >> 2) & 3;

			if (rnd != 3)
			{
				rptr->zv += char(rnd - 1);

				if (rptr->zv < -4)
					rptr->zv = -4;
				else if (rptr->zv > 4)
					rptr->zv = 4;
			}

			rptr->life -= 2;

			if (rptr->life > 240)
				rptr->x = 0;
		}
	}

	phd_PushMatrix();
	phd_TranslateAbs(lara_item->pos.x_pos, lara_item->pos.y_pos, lara_item->pos.z_pos);
	pXY = XY;
	pZ = Z;

	for (int i = 0; i < MAX_WEATHER; i++)
	{
		rptr = &raindrops[i];

		if (!rptr->x)
			continue;

		tx = rptr->x - lara_item->pos.x_pos - (SmokeWindX << 2);
		ty = rptr->y - (rptr->yv << 3) - lara_item->pos.y_pos;
		tz = rptr->z - lara_item->pos.z_pos - (SmokeWindZ << 2);
		pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
		pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
		pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));
		pXY[0] = (short)pos.x;
		pXY[1] = (short)pos.y;
		pZ[0] = pos.z;
		pXY += 2;
		pZ++;

		tx = rptr->x - lara_item->pos.x_pos;
		ty = rptr->y - lara_item->pos.y_pos;
		tz = rptr->z - lara_item->pos.z_pos;
		pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
		pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
		pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));
		pXY[0] = (short)pos.x;
		pXY[1] = (short)pos.y;
		pZ[0] = pos.z;
		pXY -= 2;
		pZ--;

		x1 = pXY[0];
		y1 = pXY[1];
		x2 = pXY[2];
		y2 = pXY[3];
		z = pZ[0];

		if (z < 32 ||
			x1 < phd_winxmin || x1 > phd_winxmin + phd_winxmax ||
			y1 < phd_winymin || y1 > phd_winymin + phd_winymax)
			continue;

		if (ClipLine(x1, y1, x2, y2, 0, 0))
		{
			alpha = GlobalAlpha;
			GlobalAlpha = 0x80000000;
			HWI_InsertLine_Sorted(x1 - phd_winxmin, y1 - phd_winymin, x2 - phd_winxmin, y2 - phd_winymin, z, 0x20, 0x304060);
			GlobalAlpha = alpha;
		}
	}

	phd_PopMatrix();
}

void DoSnow()
{
	DISPLAYMODE* dm;
	SNOWFLAKE* snow;
	PHDSPRITESTRUCT* sprite;
	PHDTEXTURESTRUCT tex;
	PHD_VECTOR pos;
	PHD_VBUF v[3];
	float zv;
	long w, h, rad, angle, ox, oy, oz, r, tx, ty, tz, x, y, z, size;
	ushort u1, v1, u2, v2;
	short c;
	char clipFlag;

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w;
	h = dm->h;
	bBlueEffect = 0;

	for (int i = 0, num_alive = 0; i < MAX_WEATHER; i++)
	{
		snow = &snowflakes[i];

		if (!snow->x && num_alive < MAX_WEATHER_ALIVE)
		{
			num_alive++;
			rad = GetRandomDraw() & 0xFFF;
			angle = GetRandomDraw() & 0x1FFE;
			snow->x = lara_item->pos.x_pos + (rad * rcossin_tbl[angle] >> 12);
			snow->y = lara_item->pos.y_pos - 1024 - (GetRandomDraw() & 0x7FF);
			snow->z = lara_item->pos.z_pos + (rad * rcossin_tbl[angle + 1] >> 12);

			if (IsRoomOutside(snow->x, snow->y, snow->z) < 0)
			{
				snow->x = 0;
				continue;
			}

			snow->stopped = 0;
			snow->xv = (GetRandomDraw() & 7) - 4;
			snow->yv = (GetRandomDraw() % 24 + 8) << 3;
			snow->zv = (GetRandomDraw() & 7) - 4;
			snow->life = 96 - (snow->yv << 1);
		}

		ox = snow->x;
		oy = snow->y;
		oz = snow->z;

		if (!snow->stopped)
		{
			snow->x += snow->xv;
			snow->y += (snow->yv & 0xF8) >> 2;
			snow->z += snow->zv;
			r = IsRoomOutside(snow->x, snow->y, snow->z);

			if (r == -3)
			{
				snow->x = 0;
				continue;
			}

			if (r == -2 || room[IsRoomOutsideNo].flags & ROOM_UNDERWATER)
			{
				snow->stopped = 1;
				snow->x = ox;
				snow->y = oy;
				snow->z = oz;

				if (snow->life > 16)
					snow->life = 16;

				if (snow->yv > 16)
					snow->yv -= 16;
			}
		}

		if (!snow->life)
		{
			snow->x = 0;
			continue;
		}

		if ((abs(CamPos.x - snow->x) > 6000 || abs(CamPos.z - snow->z) > 6000) && snow->life > 16)
			snow->life = 16;

		if (snow->xv < SmokeWindX << 1)
			snow->xv++;
		else if (snow->xv > SmokeWindX << 1)
			snow->xv--;

		if (snow->zv < SmokeWindZ << 1)
			snow->zv++;
		else if (snow->zv > SmokeWindZ << 1)
			snow->zv--;

		snow->life -= 2;

		if ((snow->yv & 7) != 7)
			snow->yv++;
	}

	sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 17];
	u1 = (sprite->offset << 8) & 0xFF00;
	v1 = sprite->offset & 0xFF00;
	u2 = ushort(u1 + sprite->width - App.nUVAdd);
	v2 = ushort(v1 + sprite->height - App.nUVAdd);
	u1 += (ushort)App.nUVAdd;
	v1 += (ushort)App.nUVAdd;

	phd_PushMatrix();
	phd_TranslateAbs(lara_item->pos.x_pos, lara_item->pos.y_pos, lara_item->pos.z_pos);

	for (int i = 0; i < MAX_WEATHER; i++)
	{
		snow = &snowflakes[i];

		if (!snow->x)
			continue;

		tx = snow->x - lara_item->pos.x_pos;
		ty = snow->y - lara_item->pos.y_pos;
		tz = snow->z - lara_item->pos.z_pos;
		pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
		pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
		pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));

		x = pos.x;
		y = pos.y;
		z = pos.z;

		if ((z >> W2V_SHIFT) < 128)
		{
			if (snow->life > 16)
				snow->life = 16;

			continue;
		}

		if (x < 0 || x > w || y < 0 || y > h)
			continue;

		size = phd_persp * (snow->yv >> 3) / (z >> 16);

		if (size < 4)
			size = 4;
		else if (size > 16)
			size = 16;

		size = (size * 0x2AAB) >> 15;
		v[0].xs = float(x + size);
		v[0].ys = float(y - (size << 1));
		v[0].zv = (float)z;
		v[0].ooz = f_oneopersp * zv;
		v[0].u = u2;
		v[0].v = v1;
		clipFlag = 0;

		if (v[0].xs < phd_winxmin)
			clipFlag++;
		else if (v[0].xs > phd_winxmin + phd_winxmax)
			clipFlag += 2;

		if (v[0].ys < phd_winymin)
			clipFlag += 4;
		else if (v[0].ys > phd_winymin + phd_winymax)
			clipFlag += 8;

		v[0].clip = clipFlag;

		v[1].xs = float(x + size);
		v[1].ys = float(y + size);
		v[1].zv = (float)z;
		v[1].ooz = f_oneopersp * zv;
		v[1].u = u2;
		v[1].v = v2;
		clipFlag = 0;

		if (v[1].xs < phd_winxmin)
			clipFlag++;
		else if (v[1].xs > phd_winxmin + phd_winxmax)
			clipFlag += 2;

		if (v[1].ys < phd_winymin)
			clipFlag += 4;
		else if (v[1].ys > phd_winymin + phd_winymax)
			clipFlag += 8;

		v[1].clip = clipFlag;

		v[2].xs = float(x - (size << 1));
		v[2].ys = float(y + size);
		v[2].zv = (float)z;
		v[2].ooz = f_oneopersp * zv;
		v[2].u = u1;
		v[2].v = v2;
		clipFlag = 0;

		if (v[2].xs < phd_winxmin)
			clipFlag++;
		else if (v[2].xs > phd_winxmin + phd_winxmax)
			clipFlag += 2;

		if (v[2].ys < phd_winymin)
			clipFlag += 4;
		else if (v[2].ys > phd_winymin + phd_winymax)
			clipFlag += 8;

		v[2].clip = clipFlag;

		tex.drawtype = 2;
		tex.tpage = sprite->tpage;

		if ((snow->yv & 7) < 7)
		{
			c = snow->yv & 7;
			c = c << 10 | c << 5 | c;
		}
		else if (snow->life > 18)
			c = 0x3DEF;
		else
		{
			c = snow->life;
			c = c << 10 | c << 5 | c;
		}

		v[0].g = c;
		v[1].g = c;
		v[2].g = c;
		HWI_InsertGT3_Poly(&v[0], &v[1], &v[2], &tex, &v[0].u, &v[1].u, &v[2].u, MID_SORT, 0);
	}

	phd_PopMatrix();
}

void DrawExplosionRings()
{
	DISPLAYMODE* dm;
	EXPLOSION_RING* ring;
	EXPLOSION_VERTS* vtx;
	EXPLOSION_VERTS* vtx2;
	PHDSPRITESTRUCT* sprite;
	PHD_VBUF v[4];
	PHD_VECTOR pos;
	PHDTEXTURESTRUCT tex;
	long* pZ;
	long* pZ2;
	short* pXY;
	short* pXY2;
	float zv;
	long w, h, rad, ang, r, g, b, x, y, z;
	long x1, x2, x3, x4, y1, y2, y3, y4, z1, z2, z3, z4, col1, col2, col3, col4;
	long Z[16];
	short XY[32];
	ushort u1, u2, v1, v2;

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	for (int i = 0; i < 6; i++)
	{
		ring = &ExpRings[i];

		if (!ring->on)
			continue;

		ring->life--;

		if (!ring->life)
		{
			ring->on = 0;
			continue;
		}

		phd_PushMatrix();
		phd_TranslateAbs(ring->x, ring->y, ring->z);
		phd_RotZ(ring->zrot << 4);
		phd_RotX(ring->xrot << 4);
		ring->radius += ring->speed;
		rad = ring->radius;
		vtx = ring->verts;
		pXY = XY;
		pZ = Z;

		for (int j = 0; j < 2; j++)
		{
			ang = (wibble & 0x3F) << 3;

			for (int k = 0; k < 8; k++)
			{
				vtx->x = short((rad * rcossin_tbl[ang << 1]) >> (W2V_SHIFT - 2));
				vtx->z = short((rad * rcossin_tbl[(ang << 1) + 1]) >> (W2V_SHIFT - 2));

				r = GetRandomDraw() & 0x1F;
				g = (GetRandomDraw() & 0x3F) + 224;
				b = (g >> 2) + (GetRandomDraw() & 0x3F);
				r = (r * ring->life) >> 5;
				g = (g * ring->life) >> 5;
				b = (b * ring->life) >> 5;
				vtx->rgb = r | (g << 8) | (b << 16);

				ang = (ang + 512) & 0xFFF;

				x = vtx->x;
				y = 0;
				z = vtx->z;

				pos.x = phd_mxptr[M00] * x + phd_mxptr[M01] * y + phd_mxptr[M02] * z + phd_mxptr[M03];
				pos.y = phd_mxptr[M10] * x + phd_mxptr[M11] * y + phd_mxptr[M12] * z + phd_mxptr[M13];
				pos.z = phd_mxptr[M20] * x + phd_mxptr[M21] * y + phd_mxptr[M22] * z + phd_mxptr[M23];

				zv = f_persp / (float)pos.z;
				pos.x = short(float(pos.x * zv + f_centerx));
				pos.y = short(float(pos.y * zv + f_centery));

				pXY[0] = (short)pos.x;
				pXY[1] = (short)pos.y;
				pZ[0] = pos.z;

				vtx++;
				pXY += 2;
				pZ++;
			}

			rad >>= 1;
		}

		vtx = ring->verts;
		vtx2 = &ring->verts[8];
		pXY = XY;
		pZ = Z;
		pXY2 = &XY[16];
		pZ2 = &Z[8];

		x1 = *pXY++;
		y1 = *pXY++;
		z1 = *pZ++;
		x3 = *pXY2++;
		y3 = *pXY2++;
		z3 = *pZ2++;

		col1 = vtx->rgb;
		col3 = vtx2->rgb;
		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 4 + ((wibble >> 4) & 3)];
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;

		for (int j = 0; j < 8; j++)
		{
			if (j == 7)
			{
				x2 = pXY[-16];
				y2 = pXY[-15];
				z2 = pZ[-8];
				x4 = pXY2[-16];
				y4 = pXY2[-15];
				z4 = pZ2[-8];
				col2 = vtx[-8].rgb;
				col4 = vtx->rgb;
			}
			else
			{
				x2 = *pXY++;
				y2 = *pXY++;
				z2 = *pZ++;
				x4 = *pXY2++;
				y4 = *pXY2++;
				z4 = *pZ2++;
				col2 = vtx->rgb;
				col4 = vtx2->rgb;
				vtx++;
				vtx2++;
			}

			if (((z1 + z2 + z3 + z4) >> 2) > phd_znear && (col1 || col2 || col3 || col4) &&
				x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 && x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
				y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 && y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
			{
				setXYZ4(v, 0, x1, y1, z1, col1, x2, y2, z2, col2, x4, y4, z4, col4, x3, y3, z3, col3);
				tex.u1 = u1;
				tex.u2 = u2;
				tex.u3 = u1;
				tex.u4 = u2;
				tex.v1 = v1;
				tex.v2 = v1;
				tex.v3 = v2;
				tex.v4 = v2;
				tex.tpage = sprite->tpage;
				tex.drawtype = 2;
				HWI_InsertGT4_Sorted(&v[0], &v[1], &v[2], &v[3], &tex, MID_SORT, 1);
			}

			x1 = x2;
			y1 = y2;
			z1 = z2;
			x3 = x4;
			y3 = y4;
			z3 = z4;
			col1 = col2;
			col3 = col4;
		}

		phd_PopMatrix();
	}
}

void DrawSummonRings()
{
	DISPLAYMODE* dm;
	EXPLOSION_RING* ring;
	EXPLOSION_VERTS* vtx;
	EXPLOSION_VERTS* vtx2;
	PHDSPRITESTRUCT* sprite;
	PHD_VBUF v[4];
	PHD_VECTOR pos;
	PHDTEXTURESTRUCT tex;
	long* pZ;
	long* pZ2;
	short* pXY;
	short* pXY2;
	float zv;
	long w, h, rad, ang, cval, r, g, b, x, y, z;
	long x1, x2, x3, x4, y1, y2, y3, y4, z1, z2, z3, z4, col1, col2, col3, col4;
	long Z[16];
	ushort u1, u2, v1, v2;
	short XY[32];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	for (int i = 0; i < 6; i++)
	{
		ring = &ExpRings[i];

		if (!ring->on)
			continue;

		ring->life--;
		ring->radius -= ring->speed;

		if (!ring->life || ring->radius <= 0)
		{
			ring->on = 0;
			continue;
		}

		ring->speed += 2;
		phd_PushMatrix();
		phd_TranslateAbs(ring->x, ring->y, ring->z);
		phd_RotZ(ring->zrot << 4);
		phd_RotX(ring->xrot << 4);

		rad = ring->radius;
		vtx = ring->verts;
		pXY = XY;
		pZ = Z;

		if (ring->life > 32)
			cval = (64 - ring->life) << 1;
		else if (ring->life < 8)
			cval = ring->life << 3;
		else
			cval = 64;

		for (int j = 0; j < 2; j++)
		{
			ang = (wibble & 0x3F) << 3;

			for (int k = 0; k < 8; k++)
			{
				vtx->x = short((rad * rcossin_tbl[ang << 1]) >> (W2V_SHIFT - 2));
				vtx->z = short((rad * rcossin_tbl[(ang << 1) + 1]) >> (W2V_SHIFT - 2));

				if (j)
					vtx->rgb = 0;
				else
				{
					r = GetRandomDraw() & 0x3F;
					g = (GetRandomDraw() & 0x1F) + 224;
					b = (g >> 2) + (GetRandomDraw() & 0x3F);
					r = (r * cval) >> 7;
					g = (g * cval) >> 7;
					b = (b * cval) >> 7;
					vtx->rgb = r | (g << 8) | (b << 16);
				}

				ang = (ang + 512) & 0xFFF;

				x = vtx->x;
				y = 0;
				z = vtx->z;

				pos.x = phd_mxptr[M00] * x + phd_mxptr[M01] * y + phd_mxptr[M02] * z + phd_mxptr[M03];
				pos.y = phd_mxptr[M10] * x + phd_mxptr[M11] * y + phd_mxptr[M12] * z + phd_mxptr[M13];
				pos.z = phd_mxptr[M20] * x + phd_mxptr[M21] * y + phd_mxptr[M22] * z + phd_mxptr[M23];
				zv = f_persp / (float)pos.z;
				pos.x = short(float(pos.x * zv + f_centerx));
				pos.y = short(float(pos.y * zv + f_centery));

				pXY[0] = (short)pos.x;
				pXY[1] = (short)pos.y;
				pZ[0] = pos.z;

				vtx++;
				pXY += 2;
				pZ++;
			}

			rad >>= 1;
		}

		vtx = ring->verts;
		vtx2 = &ring->verts[8];
		pXY = XY;
		pZ = Z;
		pXY2 = &XY[16];
		pZ2 = &Z[8];

		x1 = *pXY++;
		y1 = *pXY++;
		z1 = *pZ++;
		x3 = *pXY2++;
		y3 = *pXY2++;
		z3 = *pZ2++;
		col1 = vtx->rgb;
		col3 = vtx2->rgb;
		vtx++;
		vtx2++;
		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 4 + ((wibble >> 4) & 3)];
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;

		for (int j = 0; j < 8; j++)
		{
			if (j == 7)
			{
				x2 = pXY[-16];
				y2 = pXY[-15];
				z2 = pZ[-8];
				x4 = pXY2[-16];
				y4 = pXY2[-15];
				z4 = pZ2[-8];
				col2 = vtx[-8].rgb;
				col4 = vtx->rgb;
			}
			else
			{
				x2 = *pXY++;
				y2 = *pXY++;
				z2 = *pZ++;
				x4 = *pXY2++;
				y4 = *pXY2++;
				z4 = *pZ2++;
				col2 = vtx->rgb;
				col4 = vtx2->rgb;
				vtx++;
				vtx2++;
			}

			z1 = (z1 + z2 + z3 + z4) >> 4;

			if (z1 > phd_znear)
			{
				if (x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 && x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
					y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 && y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
				{
					setXYZ4(v, 0, x1, y1, z1, col1, x2, y2, z2, col2, x4, y4, z4, col4, x3, y3, z3, col3);
					tex.u1 = u1;
					tex.u2 = u2;
					tex.u3 = u1;
					tex.u4 = u2;
					tex.v1 = v1;
					tex.v2 = v1;
					tex.v3 = v2;
					tex.v4 = v2;
					tex.tpage = sprite->tpage;
					tex.drawtype = 2;
					HWI_InsertGT4_Sorted(&v[0], &v[1], &v[2], &v[3], &tex, MID_SORT, 1);
				}
			}

			x1 = x2;
			y1 = y2;
			z1 = z2;
			x3 = x4;
			y3 = y4;
			z3 = z4;
			col1 = col2;
			col3 = col4;
		}

		phd_PopMatrix();
	}
}

void DrawKnockBackRings()
{
	DISPLAYMODE* dm;
	EXPLOSION_RING* ring;
	EXPLOSION_VERTS* vtx;
	EXPLOSION_VERTS* vtx2;
	PHDSPRITESTRUCT* sprite;
	PHD_VBUF v[4];
	PHD_VECTOR pos;
	PHDTEXTURESTRUCT tex;
	long* pZ;
	long* pZ2;
	short* pXY;
	short* pXY2;
	float zv;
	long w, h, rad, ang, cval, r, g, b, x, y, z;
	long x1, x2, x3, x4, y1, y2, y3, y4, z1, z2, z3, z4, col1, col2, col3, col4;
	long Z[16];
	ushort u1, u2, v1, v2;
	short XY[32];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	if (KBRings[1].speed >= 0 && KnockBackCollision(&KBRings[1]))
	{
		for (int i = 0; i < 3; i++)
			KBRings[i].speed = -KBRings[i].speed >> 2;
	}

	for (int i = 0; i < 3; i++)
	{
		ring = &KBRings[i];

		if (!ring->on)
			continue;

		ring->life--;

		if (!ring->life)
		{
			ring->on = 0;
			continue;
		}

		phd_PushMatrix();
		phd_TranslateAbs(ring->x, ring->y, ring->z);
		ring->radius += ring->speed;

		if (ring->speed < 0)
			ring->speed--;
		else
			ring->speed += 3 - (i != 1);

		rad = ring->radius;
		vtx = ring->verts;
		pXY = XY;
		pZ = Z;

		if (ring->life > 24)
			cval = (32 - ring->life) << 2;
		else if (ring->life < 16)
			cval = ring->life << 1;
		else
			cval = 32;

		for (int j = 0; j < 2; j++)
		{
			ang = (wibble & 0x3F) << 3;

			for (int k = 0; k < 8; k++)
			{
				vtx->x = short((rad * rcossin_tbl[ang << 1]) >> (W2V_SHIFT - 2));
				vtx->z = short((rad * rcossin_tbl[(ang << 1) + 1]) >> (W2V_SHIFT - 2));

				r = GetRandomDraw() & 0x3F;
				g = (GetRandomDraw() & 0x1F) + 224;
				b = (g >> 2) + (GetRandomDraw() & 0x3F);
				r = (r * cval) >> 5;
				g = (g * cval) >> 5;
				b = (b * cval) >> 5;
				vtx->rgb = r | (g << 8) | (b << 16);

				ang = (ang + 512) & 0xFFF;

				x = vtx->x;
				y = 0;
				z = vtx->z;

				pos.x = phd_mxptr[M00] * x + phd_mxptr[M01] * y + phd_mxptr[M02] * z + phd_mxptr[M03];
				pos.y = phd_mxptr[M10] * x + phd_mxptr[M11] * y + phd_mxptr[M12] * z + phd_mxptr[M13];
				pos.z = phd_mxptr[M20] * x + phd_mxptr[M21] * y + phd_mxptr[M22] * z + phd_mxptr[M23];

				zv = f_persp / (float)pos.z;
				pos.x = short(float(pos.x * zv + f_centerx));
				pos.y = short(float(pos.y * zv + f_centery));

				pXY[0] = (short)pos.x;
				pXY[1] = (short)pos.y;
				pZ[0] = pos.z;

				vtx++;
				pXY += 2;
				pZ++;
			}

			rad >>= 1;
			cval >>= 1;
		}

		vtx = ring->verts;
		vtx2 = &ring->verts[8];
		pXY = XY;
		pZ = Z;
		pXY2 = &XY[16];
		pZ2 = &Z[8];

		x1 = *pXY++;
		y1 = *pXY++;
		z1 = *pZ++;
		x3 = *pXY2++;
		y3 = *pXY2++;
		z3 = *pZ2++;
		col1 = vtx->rgb;
		col3 = vtx2->rgb;
		vtx++;
		vtx2++;
		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 4 + ((wibble >> 4) & 3)];
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;

		for (int j = 0; j < 8; j++)
		{
			if (j == 7)
			{
				x2 = pXY[-16];
				y2 = pXY[-15];
				z2 = pZ[-8];
				x4 = pXY2[-16];
				y4 = pXY2[-15];
				z4 = pZ2[-8];
				col2 = vtx[-8].rgb;
				col4 = vtx->rgb;
			}
			else
			{
				x2 = *pXY++;
				y2 = *pXY++;
				z2 = *pZ++;
				x4 = *pXY2++;
				y4 = *pXY2++;
				z4 = *pZ2++;
				col2 = vtx->rgb;
				col4 = vtx2->rgb;
				vtx++;
				vtx2++;
			}

			z1 = (z1 + z2 + z3 + z4) >> 4;

			if (z1 > phd_znear && (col1 | col2 | col3 | col4))
			{
				if (x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 && x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
					y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 && y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
				{
					setXYZ4(v, 0, x1, y1, z1, col1, x2, y2, z2, col2, x4, y4, z4, col4, x3, y3, z4, col3);
					tex.u1 = u1;
					tex.v1 = v1;
					tex.u2 = u2;
					tex.v2 = v1;
					tex.u3 = u1;
					tex.v3 = v2;
					tex.u4 = u2;
					tex.v4 = v2;
					tex.tpage = sprite->tpage;
					tex.drawtype = 2;
					HWI_InsertGT4_Sorted(&v[0], &v[1], &v[2], &v[3], &tex, MID_SORT, 1);
				}
			}

			x1 = x2;
			y1 = y2;
			z1 = z2;
			x3 = x4;
			y3 = y4;
			z3 = z4;
			col1 = col2;
			col3 = col4;
		}

		phd_PopMatrix();
	}
}

void TriggerElectricBeam(ITEM_INFO* item, GAME_VECTOR* src, long copy)
{
	DISPLAYMODE* dm;
	GAME_VECTOR target;
	PHD_VECTOR pos;
	long* pZ;
	short* pXY;
	short* points;	//electricity
	float zv;
	long w, h, x, y, z, dx, dy, dz, longest, nSegments, xOff, zOff, yOff1, yOff2;
	long xs, ys, zs, tx, ty, tz;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c1, c2, c3, c4, r, g, b;
	long Z[120];
	short XY[120];
	short angle;

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	angle = (item->pos.y_rot >> 4) & 0xFFF;
	src->room_number = item->room_number;
	dx = bossdata.BeamTarget.x - src->x;
	dy = bossdata.BeamTarget.y - src->y;
	dz = bossdata.BeamTarget.z - src->z;

	longest = abs(dx);

	if (abs(dy) > longest)
		longest = abs(dy);

	if (abs(dz) > longest)
		longest = abs(dz);

	if (longest < 20480)
	{
		longest = 20480 / longest + 1;
		dx *= longest;
		dy *= longest;
		dz *= longest;
	}

	target.x = src->x + dx;
	target.y = src->y + dy;
	target.z = src->z + dz;
	LOS(src, &target);

	if (!lara.electric && !copy && !bossdata.attack_type && LaraOnLOS(src, &target))
	{
		lara_item->hit_points = 0;
		lara.electric = 1;
	}

	TriggerElectricSparks(&target, 0);
	dx = abs(target.x - src->x);
	dz = abs(target.x - src->z);

	if (dx >= dz)
		nSegments = dx >> 8;
	else
		nSegments = dz >> 8;

	if (nSegments < 8)
		nSegments = 8;
	else if (nSegments > 24)
		nSegments = 24;

	dx = (target.x - src->x) / nSegments;
	dy = (target.y - src->y) / nSegments;
	dz = (target.z - src->z) / nSegments;
	x = src->x - item->pos.x_pos;
	y = src->y - item->pos.y_pos;
	z = src->z - item->pos.z_pos;
	angle = (angle + 1024) & 0xFFF;

	if (bossdata.attack_type == 1 || bossdata.attack_type == 2)
	{
		xOff = rcossin_tbl[angle << 1] >> 8;
		zOff = rcossin_tbl[(angle << 1) + 1] >> 8;
	}
	else
	{
		xOff = rcossin_tbl[angle << 1] >> 7;
		zOff = rcossin_tbl[(angle << 1) + 1] >> 7;
	}

	yOff1 = 0;
	yOff2 = 0;
	pXY = XY;
	pZ = Z;

	pos.x = x * phd_mxptr[M00] + y * phd_mxptr[M01] + z * phd_mxptr[M02] + phd_mxptr[M03];
	pos.y = x * phd_mxptr[M10] + y * phd_mxptr[M11] + z * phd_mxptr[M12] + phd_mxptr[M13];
	pos.z = x * phd_mxptr[M20] + y * phd_mxptr[M21] + z * phd_mxptr[M22] + phd_mxptr[M23];
	zv = f_persp / (float)pos.z;
	pos.x = short(float(pos.x * zv + f_centerx));
	pos.y = short(float(pos.y * zv + f_centery));
	pXY[0] = (short)pos.x;
	pXY[1] = (short)pos.y;
	pXY[2] = (short)pos.x;
	pXY[3] = (short)pos.y;
	pZ[0] = pos.z;
	pZ[1] = pos.z;
	pXY += 4;
	pZ += 2;
	points = &electricity_points[copy * 4][0];

	for (int i = 0; i < nSegments - 1; i++)
	{
		if (copy)
		{
			xs = -*points++;
			ys = -*points++ >> 1;
			zs = -*points++;
		}
		else
		{
			xs = *points++;
			ys = *points++ >> 1;
			zs = *points++;
		}

		points += 3;
		x += dx;
		y += dy;
		z += dz;
		yOff1 += (GetRandomControl() & 0x1F) - 16;
		yOff2 += (GetRandomControl() & 0x1F) - 16;

		if (yOff1 < -256)
			yOff1 = -192;
		else if (yOff1 > 256)
			yOff1 = 192;

		if (yOff2 < -256)
			yOff2 = -192;
		else if (yOff2 > 256)
			yOff2 = 192;

		tx = x + xs + xOff;
		ty = y + ys + yOff1;
		tz = z + zs + zOff;
		pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
		pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
		pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));
		pXY[0] = (short)pos.x;
		pXY[1] = (short)pos.y;
		pZ[0] = pos.z;

		tx = x + xs - xOff;
		ty = y + ys + yOff2;
		tz = z + zs - zOff;
		pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
		pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
		pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));
		pXY[2] = (short)pos.x;
		pXY[3] = (short)pos.y;
		pZ[1] = pos.z;

		pXY += 4;
		pZ += 2;
	}

	tx = target.x - item->pos.x_pos;
	ty = target.y - item->pos.y_pos;
	tz = target.z - item->pos.z_pos;
	pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
	pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
	pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
	zv = f_persp / (float)pos.z;
	pos.x = short(float(pos.x * zv + f_centerx));
	pos.y = short(float(pos.y * zv + f_centery));
	pXY[0] = (short)pos.x;
	pXY[1] = (short)pos.y;
	pXY[2] = (short)pos.x;
	pXY[3] = (short)pos.y;
	pZ[0] = pos.z;
	pZ[1] = pos.z;

	pXY = XY;
	pZ = Z;

	x1 = *pXY++;
	y1 = *pXY++;
	z1 = *pZ++;
	x2 = *pXY++;
	y2 = *pXY++;
	z2 = *pZ++;
	c1 = 0;
	c2 = 0;

	for (int i = 0; i < nSegments; i++)
	{
		if (i == nSegments - 1)
		{
			c3 = 0;
			c4 = 0;
		}
		else
		{
			c3 = (GetRandomControl() & 0xFF) >> copy;	//nice
			c4 = (GetRandomControl() & 0xFF) >> copy;
		}

		x3 = *pXY++;
		y3 = *pXY++;
		z3 = *pZ++;
		x4 = *pXY++;
		y4 = *pXY++;
		z4 = *pZ++;

		if ((z1 + z2 + z3 + z4) >> 2 > phd_znear)
		{
			if (c1 || c2 || c3 || c4)
			{
				if (x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 &&
					y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128)
				{
					if (x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
						y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
					{
						if (bossdata.attack_type)
						{
							r = (c1 & 0xC0) >> 5;
							g = c1 >> 4;
							b = (c1 & 0xC0) >> 5;
							c1 = r << 10 | g << 5 | b;

							r = (c2 & 0xC0) >> 5;
							g = c2 >> 4;
							b = (c2 & 0xC0) >> 5;
							c2 = r << 10 | g << 5 | b;

							r = (c3 & 0xC0) >> 5;
							g = c3 >> 4;
							b = (c3 & 0xC0) >> 5;
							c3 = r << 10 | g << 5 | b;

							r = (c4 & 0xC0) >> 5;
							g = c4 >> 4;
							b = (c4 & 0xC0) >> 5;
							c4 = r << 10 | g << 5 | b;
						}
						else
						{
							r = (c1 & 0xC0) >> 5;
							g = c1 >> 4;
							b = c1 >> 4;
							c1 = r << 10 | g << 5 | b;

							r = (c2 & 0xC0) >> 5;
							g = c2 >> 4;
							b = c2 >> 4;
							c2 = r << 10 | g << 5 | b;

							r = (c3 & 0xC0) >> 5;
							g = c3 >> 4;
							b = c3 >> 4;
							c3 = r << 10 | g << 5 | b;

							r = (c4 & 0xC0) >> 5;
							g = c4 >> 4;
							b = c4 >> 4;
							c4 = r << 10 | g << 5 | b;
						}

						HWI_InsertAlphaSprite_Sorted(x2, y2, z2, c2, x1, y1, z1, c2, x4, y4, z4, c3, x3, y3, z3, c4, -1, 16, 1);
					}
				}
			}
		}

		x1 = x4;
		y1 = y4;
		z1 = z4;
		c1 = c3;
		x2 = x3;
		y2 = y3;
		z2 = z3;
		c2 = c4;
	}
}

void TriggerTribeBossHeadElectricity(ITEM_INFO* item, long copy)
{
	DISPLAYMODE* dm;
	PHD_VECTOR pos;
	PHD_VECTOR pos1;
	PHD_VECTOR pos2;
	GAME_VECTOR src;
	long* pZ;
	short* points;
	short* pDists;
	short* pXY;
	float zv;
	long w, h, dx, dy, dz, s, x, y, z, tx, ty, tz, ex, ey, ez, vx, vy, vz;
	long x1, y1, x2, y2, c1, c2, alpha;
	long Z[128];
	short XY[128];
	short dists[128];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;
	dx = lara_item->pos.x_pos - item->pos.x_pos;
	dz = lara_item->pos.z_pos - item->pos.z_pos;

	if (dx < -0x4800 || dx > 0x4800 || dz < -0x4800 || dz > 0x4800)
		return;

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	s = (rcossin_tbl[wibble << 5] >> 7) + 64;
	points = &electricity_points[0][0];
	pDists = dists;
	pXY = XY;
	pZ = Z;

	for (int i = 0; i < 4; i++)
	{
		pos1.x = tribeboss_hit[i].x;
		pos1.y = tribeboss_hit[i].y;
		pos1.z = tribeboss_hit[i].z;
		GetJointAbsPosition(item, &pos1, tribeboss_hit[i].mesh_num);

		pos2.x = tribeboss_hit[i + 1].x;
		pos2.y = tribeboss_hit[i + 1].y;
		pos2.z = tribeboss_hit[i + 1].z;
		GetJointAbsPosition(item, &pos2, tribeboss_hit[i + 1].mesh_num);

		if (i == 2)
			TrigDynamics[0] = pos1;

		pos1.x -= item->pos.x_pos;
		pos1.y -= item->pos.y_pos;
		pos1.z -= item->pos.z_pos;
		pos2.x -= item->pos.x_pos;
		pos2.y -= item->pos.y_pos;
		pos2.z -= item->pos.z_pos;
		x = pos1.x;
		y = pos1.y;
		z = pos1.z;
		dx = (pos2.x - x) >> 2;
		dy = (pos2.y - y) >> 2;
		dz = (pos2.z - z) >> 2;

		for (int j = 0; j < 5; j++)
		{
			if (j == 4)
			{
				if (i != 3)
					break;

				tx = pos2.x;
				ty = pos2.y;
				tz = pos2.z;
			}
			else
			{
				tx = x;
				ty = y;
				tz = z;
			}

			if (!j || j == 4)
				*pDists++ = 0;
			else
			{
				ex = *points++;
				ey = *points++;
				ez = *points++;

				if (copy)
				{
					tx -= ex >> 3;
					ty -= ey >> 3;
					tz -= ez >> 3;
				}
				else
				{
					tx += ex >> 3;
					ty += ey >> 3;
					tz += ez >> 3;
				}

				points += 3;
				vx = abs(ex);
				vy = abs(ey);
				vz = abs(ez);

				if (vy > vx)
					vx = vy;

				if (vz > vx)
					vx = vz;

				*pDists++ = (short)vx;
			}

			pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
			pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
			pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
			zv = f_persp / (float)pos.z;
			pos.x = short(float(pos.x * zv + f_centerx));
			pos.y = short(float(pos.y * zv + f_centery));
			*pXY++ = (short)pos.x;
			*pXY++ = (short)pos.y;
			*pZ++ = pos.z;

			x += dx;
			y += dy;
			z += dz;
		}
	}

	points = &electricity_points[16][0];

	if (bossdata.attack_count && !bossdata.death_count && !bossdata.attack_type)
	{
		for (int i = 0, n = 0; i < 5; i++)
		{
			pos1.x = tribeboss_hit[i].x;
			pos1.y = tribeboss_hit[i].y;
			pos1.z = tribeboss_hit[i].z;
			GetJointAbsPosition(item, &pos1, tribeboss_hit[i].mesh_num);

			pos2.x = tribeboss_hit[5].x;
			pos2.y = tribeboss_hit[5].y;
			pos2.z = tribeboss_hit[5].z;
			GetJointAbsPosition(item, &pos2, tribeboss_hit[5].mesh_num);

			pos1.x -= item->pos.x_pos;
			pos1.y -= item->pos.y_pos;
			pos1.z -= item->pos.z_pos;
			pos2.x -= item->pos.x_pos;
			pos2.y -= item->pos.y_pos;
			pos2.z -= item->pos.z_pos;
			x = pos1.x;
			y = pos1.y;
			z = pos1.z;
			dx = (pos2.x - x) >> 2;
			dy = (pos2.y - y) >> 2;
			dz = (pos2.z - z) >> 2;

			for (int j = 0; j < 5; j++)
			{
				if (j == 4)
				{
					tx = pos2.x;
					ty = pos2.y;
					tz = pos2.z;

					if (i == 4)
					{
						src.x = tx + item->pos.x_pos;
						src.y = ty + item->pos.y_pos;
						src.z = tz + item->pos.z_pos;

						if (bossdata.attack_count >= 64 && bossdata.attack_count <= 128)
						{
							TrigDynamics[2].x = src.x;
							TrigDynamics[2].y = src.y;
							TrigDynamics[2].z = src.z;
						}
					}
				}
				else
				{
					tx = x;
					ty = y;
					tz = z;
				}

				if (!j || j == 4)
					*pDists++ = 0;
				else
				{
					n++;

					if (n > 15)
					{
						points = &electricity_points[0][0];
						n = 0;
					}

					ex = *points++;
					ey = *points++;

					if (copy)
					{
						tx -= ex >> 4;
						ty -= ey >> 4;
					}
					else
					{
						tx += ex >> 4;
						ty += ey >> 4;
					}

					points += 4;
					vx = abs(ex);
					vy = abs(ey);

					if (vy > vx)
						vx = vy;

					*pDists++ = (short)vx;
				}

				pos.x = tx * phd_mxptr[M00] + ty * phd_mxptr[M01] + tz * phd_mxptr[M02] + phd_mxptr[M03];
				pos.y = tx * phd_mxptr[M10] + ty * phd_mxptr[M11] + tz * phd_mxptr[M12] + phd_mxptr[M13];
				pos.z = tx * phd_mxptr[M20] + ty * phd_mxptr[M21] + tz * phd_mxptr[M22] + phd_mxptr[M23];
				zv = f_persp / (float)pos.z;
				pos.x = short(float(pos.x * zv + f_centerx));
				pos.y = short(float(pos.y * zv + f_centery));
				*pXY++ = (short)pos.x;
				*pXY++ = (short)pos.y;
				*pZ++ = pos.z;

				x += dx;
				y += dy;
				z += dz;
			}
		}
	}

	pDists = dists;
	pXY = XY;
	pZ = Z;

	for (int i = 0; i < 16; i++)
	{
		x1 = pXY[0];
		y1 = pXY[1];
		x2 = pXY[2];
		y2 = pXY[3];
		z = pZ[0];
		c1 = pDists[0];
		c2 = pDists[1];

		pDists++;
		pXY += 2;
		pZ++;

		if (z < phd_znear)
			continue;

		if (c1 > 255)
		{
			c1 = 511 - c1;

			if (c1 < 0)
				c1 = 0;
		}

		if (c2 > 255)
		{
			c2 = 511 - c2;

			if (c2 < 0)
				c2 = 0;
		}

		if (copy)
		{
			c1 >>= 1;
			c2 >>= 1;
		}

		c1 = (s * c1) >> 6;
		c2 = (s * c2) >> 6;

		if (ClipLine(x1, y1, x2, y2, w, h) && x1 >= 0 && x1 <= w && x2 >= 0 && x2 <= w && y1 >= 0 && y1 <= h && y2 >= 0 && y2 <= h)
		{
			c1 = c1 | (c1 << 8);
			c2 = c2 | (c2 << 8);

			alpha = GlobalAlpha;
			GlobalAlpha = 0x70000000;
			HWI_InsertLine_Sorted(x1 - phd_winxmin, y1 - phd_winymin, x2 - phd_winxmin, y2 - phd_winymin, z, c1, c2);
			GlobalAlpha = alpha;
		}
	}

	if (bossdata.attack_count && !bossdata.death_count && !bossdata.attack_type)
	{
		for (int i = 0; i < 5; i++)
		{
			pXY += 2;
			pZ++;
			pDists++;

			for (int j = 0; j < 4; j++)
			{
				x1 = pXY[0];
				y1 = pXY[1];
				x2 = pXY[2];
				y2 = pXY[3];
				z = pZ[0];
				c1 = pDists[0];
				c2 = pDists[1];

				pDists++;
				pXY += 2;
				pZ++;

				if (z < phd_znear)
					continue;

				if (c1 > 255)
				{
					c1 = 511 - c1;

					if (c1 < 0)
						c1 = 0;
				}

				if (c2 > 255)
				{
					c2 = 511 - c2;

					if (c2 < 0)
						c2 = 0;
				}

				if (copy)
				{
					c1 >>= 1;
					c2 >>= 1;
				}

				if (bossdata.attack_count < 64)
				{
					c1 = (bossdata.attack_count * c1) >> 6;
					c2 = (bossdata.attack_count * c2) >> 6;
				}

				if (ClipLine(x1, y1, x2, y2, w, h) && x1 >= 0 && x1 <= w && y1 >= 0 && y1 <= h && x2 >= 0 && x2 <= w && y2 >= 0 && y2 <= h)
				{
					c1 = c1 | (c1 << 8);
					c2 = c2 | (c2 << 8);

					alpha = GlobalAlpha;
					GlobalAlpha = 0x70000000;
					HWI_InsertLine_Sorted(x1 - phd_winxmin, y1 - phd_winymin, x2 - phd_winxmin, y2 - phd_winymin, z, c1, c2);
					GlobalAlpha = alpha;
				}
			}
		}
	}

	if (bossdata.attack_count && !bossdata.death_count && (bossdata.attack_type == 1 || bossdata.attack_type == 2))
	{
		pos1.x = 0;
		pos1.y = 0;
		pos1.z = 0;
		GetJointAbsPosition(item, &pos1, 14);

		src.x = pos1.x;
		src.y = pos1.y;
		src.z = pos1.z;

		if (bossdata.attack_count >= 64)
		{
			if (bossdata.attack_count <= 128)
				TrigDynamics[2] = pos1;

			if (bossdata.attack_count >= 64 && bossdata.attack_count <= 96)
			{
				if (bossdata.attack_count > 90 && !lizard_man_active)
					TriggerLizardMan();

				TriggerElectricBeam(item, &src, copy);

				for (int i = 0; i < 3; i++)
					TriggerSummonSmoke(bossdata.BeamTarget.x, bossdata.BeamTarget.y, bossdata.BeamTarget.z);
			}
		}
	}
	else if (bossdata.attack_count > 64 && !bossdata.death_count && !bossdata.attack_type)
		TriggerElectricBeam(item, &src, copy);

	phd_PopMatrix();
}

void DrawTonyBossShield(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	SHIELD_POINTS* s0;
	SHIELD_POINTS* s1;
	PHDSPRITESTRUCT* sprite;
	PHD_VECTOR pos;
	PHD_VBUF v[4];
	PHDTEXTURESTRUCT tex;
	long* pZ0;
	long* pZ1;
	short* pXY0;
	short* pXY1;
	float zv;
	long w, h, r, g, b, rgb;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c1, c2, c3, c4;
	long Z[150];
	ushort u1, v1, u2, v2;
	short XY[150];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	pXY0 = XY;
	pZ0 = Z;
	s0 = &TonyBossShield[0];

	for (int i = 0; i < 40; i++, s0++)
	{
		x1 = s0->x;
		y1 = s0->y;
		z1 = s0->z;

		if (i >= 8 && i <= 31)
		{
			rgb = s0->rgb;

			if (rgb)
			{
				r = (rgb & 0xFF) - s0->rsub;;
				g = ((rgb >> 8) & 0xFF) - s0->gsub;
				b = ((rgb >> 16) & 0xFF) - s0->bsub;

				if (r < 0)
					r = 0;

				if (g < 0)
					g = 0;

				if (b < 0)
					b = 0;

				s0->rgb = r | (g << 8) | (b << 16);
			}
		}

		pos.x = phd_mxptr[M00] * x1 + phd_mxptr[M01] * y1 + phd_mxptr[M02] * z1 + phd_mxptr[M03];
		pos.y = phd_mxptr[M10] * x1 + phd_mxptr[M11] * y1 + phd_mxptr[M12] * z1 + phd_mxptr[M13];
		pos.z = phd_mxptr[M20] * x1 + phd_mxptr[M21] * y1 + phd_mxptr[M22] * z1 + phd_mxptr[M23];

		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));

		pXY0[0] = (short)pos.x;
		pXY0[1] = (short)pos.y;
		pXY0 += 2;
		*pZ0++ = pos.z;
	}

	pXY0 = &XY[0];
	pZ0 = &Z[0];
	pXY1 = &XY[16];
	pZ1 = &Z[8];
	s0 = &TonyBossShield[0];
	s1 = &TonyBossShield[8];

	for (int i = 0; i < 4; i++)
	{
		x1 = *pXY0++;
		y1 = *pXY0++;
		z1 = *pZ0++;
		x3 = *pXY1++;
		y3 = *pXY1++;
		z3 = *pZ1++;
		c1 = s0->rgb;
		c3 = s1->rgb;
		s0++;
		s1++;

		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 18 + ((i + (wibble >> 3)) & 7)];
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;

		for (int j = 0; j < 8; j++)
		{
			if (j == 7)
			{
				x2 = pXY0[-16];
				y2 = pXY0[-15];
				z2 = pZ0[-8];
				x4 = pXY1[-16];
				y4 = pXY1[-15];
				z4 = pZ1[-8];
				c2 = s0[-8].rgb;
				c4 = s1->rgb;
			}
			else
			{
				x2 = *pXY0++;
				y2 = *pXY0++;
				z2 = *pZ0++;
				x4 = *pXY1++;
				y4 = *pXY1++;
				z4 = *pZ1++;
				c2 = s0->rgb;
				c4 = s1->rgb;
				s0++;
				s1++;

#if 1
				if (s1 - TonyBossShield >= sizeof(TonyBossShield) / sizeof(SHIELD_POINTS))
					s1 = &TonyBossShield[39];
#endif
			}

			if ((z1 + z2 + z3 + z4) >> 2 > phd_znear && (c1 || c2 || c3 || c4) &&
				x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 && x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
				y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 && y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
			{
				setXYZ4(v, 0, x1, y1, z1, c1, x2, y2, z2, c2, x4, y4, z4, c4, x3, y3, z3, c3);
				tex.u1 = u1;
				tex.u2 = u2;
				tex.u3 = u2;
				tex.u4 = u1;
				tex.v1 = v1;
				tex.v2 = v1;
				tex.v3 = v2;
				tex.v4 = v2;
				tex.tpage = sprite->tpage;
				tex.drawtype = 2;
				HWI_InsertGT4_Sorted(&v[0], &v[1], &v[2], &v[3], &tex, MID_SORT, 1);
			}

			x1 = x2;
			y1 = y2;
			z1 = z2;
			x3 = x4;
			y3 = y4;
			z3 = z4;
			c1 = c2;
			c3 = c4;
		}
	}

	phd_PopMatrix();
}

void DrawTribeBossShield(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	SHIELD_POINTS* s0;
	SHIELD_POINTS* s1;
	PHDSPRITESTRUCT* sprite;
	PHD_VECTOR pos;
	PHD_VBUF v[4];
	PHDTEXTURESTRUCT tex;
	long* pZ0;
	long* pZ1;
	short* pXY0;
	short* pXY1;
	float zv;
	long w, h, r, g, b, rgb;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c1, c2, c3, c4;
	long Z[150];
	ushort u1, v1, u2, v2;
	short XY[150];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	pXY0 = XY;
	pZ0 = Z;
	shield_active = 0;

	for (int i = 0; i < 40; i++)
	{
		s0 = &TribeBossShield[i];
		x1 = s0->x;
		y1 = s0->y;
		z1 = s0->z;

		if (i >= 8 && i <= 31)
		{
			rgb = s0->rgb;

			if (rgb)
			{
				r = (rgb & 0xFF) - s0->rsub;;
				g = ((rgb >> 8) & 0xFF) - s0->gsub;
				b = ((rgb >> 16) & 0xFF) - s0->bsub;

				if (r < 0)
					r = 0;

				if (g < 0)
					g = 0;

				if (b < 0)
					b = 0;

				s0->rgb = r | (g << 8) | (b << 16);
			}
		}

		pos.x = phd_mxptr[M00] * x1 + phd_mxptr[M01] * y1 + phd_mxptr[M02] * z1 + phd_mxptr[M03];
		pos.y = phd_mxptr[M10] * x1 + phd_mxptr[M11] * y1 + phd_mxptr[M12] * z1 + phd_mxptr[M13];
		pos.z = phd_mxptr[M20] * x1 + phd_mxptr[M21] * y1 + phd_mxptr[M22] * z1 + phd_mxptr[M23];

		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));
		*pXY0++ = (short)pos.x;
		*pXY0++ = (short)pos.y;
		*pZ0++ = pos.z;
	}

	pXY0 = &XY[0];
	pZ0 = &Z[0];
	pXY1 = &XY[16];
	pZ1 = &Z[8];
	s0 = &TribeBossShield[0];
	s1 = &TribeBossShield[8];

	for (int i = 0; i < 4; i++)
	{
		x1 = *pXY0++;
		y1 = *pXY0++;
		z1 = *pZ0++;
		x3 = *pXY1++;
		y3 = *pXY1++;
		z3 = *pZ1++;
		c1 = s0->rgb;
		c3 = s1->rgb;
		s0++;
		s1++;

		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 18 + ((i + (wibble >> 3)) & 7)];
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;

		for (int j = 0; j < 8; j++)
		{
			if (j == 7)
			{
				x2 = pXY0[-16];
				y2 = pXY0[-15];
				z2 = pZ0[-8];
				x4 = pXY1[-16];
				y4 = pXY1[-15];
				z4 = pZ1[-8];
				c2 = s0[-8].rgb;
				c4 = s1->rgb;
			}
			else
			{
				x2 = *pXY0++;
				y2 = *pXY0++;
				z2 = *pZ0++;
				x4 = *pXY1++;
				y4 = *pXY1++;
				z4 = *pZ1++;
				c2 = s0->rgb;
				c4 = s1->rgb;
				s0++;
				s1++;

#if 1
				if (s1 - TribeBossShield >= sizeof(TribeBossShield) / sizeof(SHIELD_POINTS))
					s1 = &TribeBossShield[39];
#endif
			}

			if ((z1 + z2 + z3 + z4) >> 2 > phd_znear && (c1 || c2 || c3 || c4) &&
				x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 && x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
				y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 && y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
			{
				setXYZ4(v, 0, x1, y1, z1, c1, x2, y2, z2, c2, x4, y4, z4, c4, x3, y3, z3, c3);
				tex.u1 = u1;
				tex.u2 = u2;
				tex.u3 = u2;
				tex.u4 = u1;
				tex.v1 = v1;
				tex.v2 = v1;
				tex.v3 = v2;
				tex.v4 = v2;
				tex.tpage = sprite->tpage;
				tex.drawtype = 2;
				HWI_InsertGT4_Sorted(&v[0], &v[1], &v[2], &v[3], &tex, MID_SORT, 1);
			}

			x1 = x2;
			y1 = y2;
			z1 = z2;
			x3 = x4;
			y3 = y4;
			z3 = z4;
			c1 = c2;
			c3 = c4;
		}
	}

	phd_PopMatrix();
}

void DrawLondonBossShield(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	SHIELD_POINTS* s0;
	SHIELD_POINTS* s1;
	PHDSPRITESTRUCT* sprite;
	PHD_VECTOR pos;
	PHD_VBUF v[4];
	PHDTEXTURESTRUCT tex;
	long* pZ0;
	long* pZ1;
	short* pXY0;
	short* pXY1;
	float zv;
	long w, h, r, g, b, rgb;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c1, c2, c3, c4;
	long Z[150];
	ushort u1, v1, u2, v2;
	short XY[150];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	pXY0 = XY;
	pZ0 = Z;

	for (int i = 0; i < 40; i++)
	{
		s0 = &LondonBossShield[i];
		x1 = s0->x;
		y1 = s0->y;
		z1 = s0->z;

		if (i >= 8 && i <= 31)
		{
			rgb = s0->rgb;

			if (rgb)
			{
				r = (rgb & 0xFF) - s0->rsub;;
				g = ((rgb >> 8) & 0xFF) - s0->gsub;
				b = ((rgb >> 16) & 0xFF) - s0->bsub;

				if (r < 0)
					r = 0;

				if (g < 0)
					g = 0;

				if (b < 0)
					b = 0;

				s0->rgb = r | (g << 8) | (b << 16);
			}
		}

		pos.x = phd_mxptr[M00] * x1 + phd_mxptr[M01] * y1 + phd_mxptr[M02] * z1 + phd_mxptr[M03];
		pos.y = phd_mxptr[M10] * x1 + phd_mxptr[M11] * y1 + phd_mxptr[M12] * z1 + phd_mxptr[M13];
		pos.z = phd_mxptr[M20] * x1 + phd_mxptr[M21] * y1 + phd_mxptr[M22] * z1 + phd_mxptr[M23];

		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));
		*pXY0++ = (short)pos.x;
		*pXY0++ = (short)pos.y;
		*pZ0++ = pos.z;
	}

	pXY0 = &XY[0];
	pZ0 = &Z[0];
	pXY1 = &XY[16];
	pZ1 = &Z[8];
	s0 = &LondonBossShield[0];
	s1 = &LondonBossShield[8];

	for (int i = 0; i < 4; i++)
	{
		x1 = *pXY0++;
		y1 = *pXY0++;
		z1 = *pZ0++;
		x3 = *pXY1++;
		y3 = *pXY1++;
		z3 = *pZ1++;
		c1 = s0->rgb;
		c3 = s1->rgb;
		s0++;
		s1++;

		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 18 + ((i + (wibble >> 3)) & 7)];
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;

		for (int j = 0; j < 8; j++)
		{
			if (j == 7)
			{
				x2 = pXY0[-16];
				y2 = pXY0[-15];
				z2 = pZ0[-8];
				x4 = pXY1[-16];
				y4 = pXY1[-15];
				z4 = pZ1[-8];
				c2 = s0[-8].rgb;
				c4 = s1->rgb;
			}
			else
			{
				x2 = *pXY0++;
				y2 = *pXY0++;
				z2 = *pZ0++;
				x4 = *pXY1++;
				y4 = *pXY1++;
				z4 = *pZ1++;
				c2 = s0->rgb;
				c4 = s1->rgb;
				s0++;
				s1++;

#if 1
				if (s1 - LondonBossShield >= sizeof(LondonBossShield) / sizeof(SHIELD_POINTS))
					s1 = &LondonBossShield[39];
#endif
			}

			if ((z1 + z2 + z3 + z4) >> 2 > phd_znear && (c1 || c2 || c3 || c4) &&
				x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 && x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
				y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 && y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
			{
				setXYZ4(v, 0, x1, y1, z1, c1, x2, y2, z2, c2, x4, y4, z4, c4, x3, y3, z3, c3);
				tex.u1 = u1;
				tex.u2 = u2;
				tex.u3 = u2;
				tex.u4 = u1;
				tex.v1 = v1;
				tex.v2 = v1;
				tex.v3 = v2;
				tex.v4 = v2;
				tex.tpage = sprite->tpage;
				tex.drawtype = 2;
				HWI_InsertGT4_Sorted(&v[0], &v[1], &v[2], &v[3], &tex, MID_SORT, 1);
			}

			x1 = x2;
			y1 = y2;
			z1 = z2;
			x3 = x4;
			y3 = y4;
			z3 = z4;
			c1 = c2;
			c3 = c4;
		}
	}

	phd_PopMatrix();
}

void DrawWillBossShield(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	SHIELD_POINTS* s0;
	SHIELD_POINTS* s1;
	PHDSPRITESTRUCT* sprite;
	PHD_VECTOR pos;
	PHD_VBUF v[4];
	PHDTEXTURESTRUCT tex;
	long* pZ0;
	long* pZ1;
	short* pXY0;
	short* pXY1;
	float zv;
	long w, h, r, g, b, rgb;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c1, c2, c3, c4;
	long Z[150];
	ushort u1, v1, u2, v2;
	short XY[150];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);
	pXY0 = XY;
	pZ0 = Z;

	for (int i = 0; i < 40; i++)
	{
		s0 = &WillBossShield[i];
		x1 = s0->x;
		y1 = s0->y;
		z1 = s0->z;

		if (i >= 8 && i <= 31)
		{
			rgb = s0->rgb;

			if (rgb)
			{
				r = (rgb & 0xFF) - s0->rsub;;
				g = ((rgb >> 8) & 0xFF) - s0->gsub;
				b = ((rgb >> 16) & 0xFF) - s0->bsub;

				if (r < 0)
					r = 0;

				if (g < 0)
					g = 0;

				if (b < 0)
					b = 0;

				s0->rgb = r | (g << 8) | (b << 16);
			}
		}

		pos.x = phd_mxptr[M00] * x1 + phd_mxptr[M01] * y1 + phd_mxptr[M02] * z1 + phd_mxptr[M03];
		pos.y = phd_mxptr[M10] * x1 + phd_mxptr[M11] * y1 + phd_mxptr[M12] * z1 + phd_mxptr[M13];
		pos.z = phd_mxptr[M20] * x1 + phd_mxptr[M21] * y1 + phd_mxptr[M22] * z1 + phd_mxptr[M23];

		zv = f_persp / (float)pos.z;
		pos.x = short(float(pos.x * zv + f_centerx));
		pos.y = short(float(pos.y * zv + f_centery));
		*pXY0++ = (short)pos.x;
		*pXY0++ = (short)pos.y;
		*pZ0++ = pos.z;
	}

	pXY0 = &XY[0];
	pZ0 = &Z[0];
	pXY1 = &XY[16];
	pZ1 = &Z[8];
	s0 = &WillBossShield[0];
	s1 = &WillBossShield[8];

	for (int i = 0; i < 4; i++)
	{
		x1 = *pXY0++;
		y1 = *pXY0++;
		z1 = *pZ0++;
		x3 = *pXY1++;
		y3 = *pXY1++;
		z3 = *pZ1++;
		c1 = s0->rgb;
		c3 = s1->rgb;
		s0++;
		s1++;

		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 18 + ((i + (wibble >> 3)) & 7)];
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;

		for (int j = 0; j < 8; j++)
		{
			if (j == 7)
			{
				x2 = pXY0[-16];
				y2 = pXY0[-15];
				z2 = pZ0[-8];
				x4 = pXY1[-16];
				y4 = pXY1[-15];
				z4 = pZ1[-8];
				c2 = s0[-8].rgb;
				c4 = s1->rgb;
			}
			else
			{
				x2 = *pXY0++;
				y2 = *pXY0++;
				z2 = *pZ0++;
				x4 = *pXY1++;
				y4 = *pXY1++;
				z4 = *pZ1++;
				c2 = s0->rgb;
				c4 = s1->rgb;
				s0++;
				s1++;

#if 1
				if (s1 - WillBossShield >= sizeof(WillBossShield) / sizeof(SHIELD_POINTS))
					s1 = &WillBossShield[39];
#endif
			}

			if ((z1 + z2 + z3 + z4) >> 2 > phd_znear && (c1 || c2 || c3 || c4) &&
				x1 > -128 && x2 > -128 && x3 > -128 && x4 > -128 && x1 < w + 128 && x2 < w + 128 && x3 < w + 128 && x4 < w + 128 &&
				y1 > -128 && y2 > -128 && y3 > -128 && y4 > -128 && y1 < h + 128 && y2 < h + 128 && y3 < h + 128 && y4 < h + 128)
			{
				setXYZ4(v, 0, x1, y1, z1, c1, x2, y2, z2, c2, x4, y4, z4, c4, x3, y3, z3, c3);
				tex.u1 = u1;
				tex.u2 = u2;
				tex.u3 = u2;
				tex.u4 = u1;
				tex.v1 = v1;
				tex.v2 = v1;
				tex.v3 = v2;
				tex.v4 = v2;
				tex.tpage = sprite->tpage;
				tex.drawtype = 2;
				HWI_InsertGT4_Sorted(&v[0], &v[1], &v[2], &v[3], &tex, MID_SORT, 1);
			}

			x1 = x2;
			y1 = y2;
			z1 = z2;
			x3 = x4;
			y3 = y4;
			z3 = z4;
			c1 = c2;
			c3 = c4;
		}
	}

	phd_PopMatrix();
}

void S_DrawLaserBeam(GAME_VECTOR* src, GAME_VECTOR* dest, uchar cr, uchar cg, uchar cb)
{
	DISPLAYMODE* dm;
	long* p;
	long* c;
	long w, h, dx, dy, dz, dist, nSegments, x, y, z, s;
	long x1, y1, z1, x2, y2, z2, r1, g1, b1, r2, g2, b2, alpha, c1, c2;
	long coords[600];
	long cols[600];
	long XYZ[3];

	UpdateLaserShades();
	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	dx = src->x - dest->x;
	dz = src->z - dest->z;
	dist = phd_sqrt(SQUARE(dx) + SQUARE(dz));
	nSegments = dist >> 9;

	if (nSegments < 8)
		nSegments = 8;
	else if (nSegments > 32)
		nSegments = 32;

	dx = (dest->x - src->x) / nSegments;
	dy = (dest->y - src->y) / nSegments;
	dz = (dest->z - src->z) / nSegments;
	x = 0;
	y = 0;
	z = 0;
	p = coords;
	c = cols;

	for (int i = 0; i < nSegments + 1; i++)
	{
		mCalcPoint(src->x + x, src->y + y, src->z + z, XYZ);
		ProjectPCoord(XYZ[0], XYZ[1], XYZ[2], p, w >> 1, h >> 1, phd_persp);

		p += 3;
		x += dx;
		y += dy;
		z += dz;

		if (!i || i == nSegments)
		{
			c[0] = 0;
			c[1] = 0;
			c[2] = 0;
		}
		else
		{
			s = LaserShades[i];
			c[0] = cr == 0xFF ? s + 32 : s >> cr;
			c[1] = cg == 0xFF ? s + 32 : s >> cg;
			c[2] = cb == 0xFF ? s + 32 : s >> cb;
		}
		
		c += 3;
	}

	p = coords;
	c = cols;
	x1 = *p++;
	y1 = *p++;
	z1 = *p++;
	r1 = *c++;
	g1 = *c++;
	b1 = *c++;

	for (int i = 0; i < nSegments; i++)
	{
		x2 = *p++;
		y2 = *p++;
		z2 = *p++;
		r2 = *c++;
		g2 = *c++;
		b2 = *c++;
		r2 <<= 1;
		g2 <<= 1;
		b2 <<= 1;
			
		if (r2 > 255)
			r2 = 255;

		if (g2 > 255)
			g2 = 255;

		if (b2 > 255)
			b2 = 255;

		if (z1 > 32 && z2 > 32 && ClipLine(x1, y1, x2, y2, w, h))
		{
			c1 = (r1 << 16) | (g1 << 8) | b1;
			c2 = (r2 << 16) | (g2 << 8) | b2;

			alpha = GlobalAlpha;
			GlobalAlpha = 0xB0000000;
			HWI_InsertLine_Sorted(x1 - phd_winxmin, y1 - phd_winymin, x2 - phd_winxmin, y2 - phd_winymin, z1 << W2V_SHIFT, c1, c2);
			GlobalAlpha = alpha;
		}

		y1 = y2;
		x1 = x2;
		z1 = z2;
		r1 = r2;
		g1 = g2;
		b1 = b2;
	}
}

void S_DrawBat()
{
	DISPLAYMODE* dm;
	BAT_STRUCT* bat;
	PHDSPRITESTRUCT* sprite;
	PHD_VECTOR pos;
	PHD_VBUF v[4];
	PHDTEXTURESTRUCT tex;
	long* pZ;
	short* pXY;
	uchar* links;
	float zv;
	long w, h, x, y, z;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3;
	long Z[10];
	ushort u1, v1, u2, v2;
	short XY[10];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w;
	h = dm->h;
	pXY = XY;
	pZ = Z;

	for (int i = 0; i < 32; i++)
	{
		bat = &bats[i];

		if (!(bat->flags & 1))
			continue;

		phd_PushMatrix();
		phd_TranslateAbs(bat->x, bat->y, bat->z);
		phd_RotY(bat->angle << 4);

		for (int j = 0; j < 5; j++)
		{
			x = BatMesh[j][0];

			if (j < 3)
				y = (rcossin_tbl[((bat->WingYoff - 32) & 0x3F) << 7] >> 8) + BatMesh[j][1] - 512;
			else
				y = (rcossin_tbl[bat->WingYoff << 7] >> 4) + BatMesh[j][1] - 512;

			z = BatMesh[j][2];

			pos.x = phd_mxptr[M00] * x + phd_mxptr[M01] * y + phd_mxptr[M02] * z + phd_mxptr[M03];
			pos.y = phd_mxptr[M10] * x + phd_mxptr[M11] * y + phd_mxptr[M12] * z + phd_mxptr[M13];
			pos.z = phd_mxptr[M20] * x + phd_mxptr[M21] * y + phd_mxptr[M22] * z + phd_mxptr[M23];

			zv = f_persp / (float)pos.z;
			pos.x = short(float(pos.x * zv + f_centerx));
			pos.y = short(float(pos.y * zv + f_centery));

			pXY[0] = (short)pos.x;
			pXY[1] = (short)pos.y;
			pZ[0] = pos.z;
			pXY += 2;
			pZ += 2;	//?
		}

		phd_PopMatrix();
		pXY = XY;
		pZ = Z;
		links = BatLinks;

		for (int j = 0; j < 3; j++)
		{
			x1 = pXY[links[0]];
			y1 = pXY[links[0] + 1];
			z1 = pZ[links[1]];
			links++;

			x2 = pXY[links[0]];
			y2 = pXY[links[0] + 1];
			z2 = pZ[links[1]];
			links++;

			x3 = pXY[links[0]];
			y3 = pXY[links[0] + 1];
			z3 = pZ[links[1]];
			links++;

			if (z1 >> W2V_SHIFT < 32 || z2 >> W2V_SHIFT < 32 || z3 >> W2V_SHIFT < 32 ||
				(x1 < 0 && x2 < 0 && x3 < 0) || (x1 >= w && x2 >= w && x3 >= w) ||
				(y1 < 0 && y2 < 0 && y3 < 0) || (y1 >= h && y2 >= h && y3 >= h))
				continue;

			setXYZ3(v, 1, x1, y1, z1, 0x7E8C, x2, y2, z2, 0x7E8C, x3, y3, z3, 0x7E8C);
			sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 12];
			u1 = (sprite->offset << 8) & 0xFF00;
			v1 = sprite->offset & 0xFF00;
			u2 = ushort(u1 + sprite->width - App.nUVAdd);
			v2 = ushort(v1 + sprite->height - App.nUVAdd);
			u1 += (ushort)App.nUVAdd;
			v1 += (ushort)App.nUVAdd;

			tex.drawtype = 1;
			tex.tpage = sprite->tpage;

			if (!j)
			{
				v[0].u = u1;
				v[0].v = v1 + (ushort)App.nUVAdd;
				v[1].u = u2 - (ushort)App.nUVAdd;
				v[1].v = v2;
				v[2].u = u1;
			}
			else
			{
				if (j == 1)
				{
					v[0].u = u2;
					v[0].v = v1;
					v[1].u = u1;
				}
				else
				{
					v[0].u = u1;
					v[0].v = v1;
					v[1].u = u2;
				}

				v[1].v = v1;
				v[2].u = u2;
			}

			v[2].v = v2;
			HWI_InsertGT3_Poly(v, &v[1], &v[2], &tex, &v[0].u, &v[1].u, &v[2].u, MID_SORT, 1);
		}
	}
}

void S_DrawSparks()
{
	DISPLAYMODE* dm;
	SPARKS* sptr;
	FX_INFO* fx;
	ITEM_INFO* item;
	PHD_VECTOR pos;
	long w, h, x, y, z, sw, sh;
	long x1, y1, x2, y2, x3, y3, x4, y4, r, g, b, f, c, drawType;
	long sin, cos, sx1, sx2, sy1, sy2, cx1, cx2, cy1, cy2;
	long vpos[3][3];
	long point[3];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w;
	h = dm->h;

	for (int i = 0; i < 192; i++)
	{
		sptr = &spark[i];

		if (!sptr->On)
			continue;

		if (sptr->Flags & SF_FX)
		{
			fx = &effects[sptr->FxObj];

			if (sptr->Flags & SF_ATTACHEDPOS)
			{
				x = fx->pos.x_pos;
				y = fx->pos.y_pos;
				z = fx->pos.z_pos;
			}
			else
			{
				x = sptr->x + fx->pos.x_pos;
				y = sptr->y + fx->pos.y_pos;
				z = sptr->z + fx->pos.z_pos;
			}
		}
		else if (sptr->Flags & SF_ITEM)
		{
			item = &items[sptr->FxObj];

			if (sptr->Flags & SF_ATTACHEDPOS)
			{
				x = item->pos.x_pos;
				y = item->pos.y_pos;
				z = item->pos.z_pos;
			}
			else if (sptr->Flags & SF_ATTACHEDNODE)
			{
				pos.x = NodeOffsets[sptr->NodeNumber].x;
				pos.y = NodeOffsets[sptr->NodeNumber].y;
				pos.z = NodeOffsets[sptr->NodeNumber].z;
				GetJointAbsPosition(item, &pos, NodeOffsets[sptr->NodeNumber].mesh_num);
				x = sptr->x + pos.x;
				y = sptr->y + pos.y;
				z = sptr->z + pos.z;

				if (sptr->NodeNumber == 2 || sptr->NodeNumber == 3)
				{
					b = sptr->NodeNumber == 3 ? (GetRandomDraw() & 3) + 12 : (GetRandomDraw() & 3) + 8;

					if (sptr->sLife - sptr->Life > b)
					{
						sptr->Flags &= ~(SF_ATTACHEDNODE | SF_ITEM);
						sptr->x = x;
						sptr->y = y;
						sptr->z = z;
					}
				}
			}
			else
			{
				x = sptr->x + item->pos.x_pos;
				y = sptr->y + item->pos.y_pos;
				z = sptr->z + item->pos.z_pos;
			}
		}
		else
		{
			x = sptr->x;
			y = sptr->y;
			z = sptr->z;
		}

		mCalcPoint(x, y, z, point);
		ProjectPCoord(point[0], point[1], point[2], vpos[0], w >> 1, h >> 1, phd_persp);

		if (sptr->Flags & SF_DEF)
		{
			if (sptr->Flags & SF_SCALE)
			{
				if (!vpos[0][2])
					vpos[0][2] = 1;

				if (sptr->Flags & SF_ATTACHEDNODE && !sptr->NodeNumber)
					x = 2;
				else
					x = 4;

				sw = ((sptr->Width * phd_persp) << sptr->Scalar) / vpos[0][2];
				sh = ((sptr->Height * phd_persp) << sptr->Scalar) / vpos[0][2];

				if (sw > sptr->Width << sptr->Scalar)
					sw = sptr->Width << sptr->Scalar;
				else if (sw < x)
					sw = x;

				if (sh > sptr->Height << sptr->Scalar)
					sh = sptr->Height << sptr->Scalar;
				else if (sh < x)
					sh = x;
			}
			else
			{
				sw = sptr->Width;
				sh = sptr->Height;
			}

			z = vpos[0][2] << W2V_SHIFT;

			if (z < phd_znear || z > phd_zfar)
				continue;

			x = vpos[0][0];
			y = vpos[0][1];

			if (x + (sw >> 1) < 0 || x - (sw >> 1) > w ||
				y + (sh >> 1) < 0 || y - (sh >> 1) > h)
				continue;

			if (sptr->Flags & SF_ROTATE)
			{
				sin = rcossin_tbl[sptr->RotAng << 1];
				cos = rcossin_tbl[(sptr->RotAng << 1) + 1];
				sx1 = (-(sw >> 1) * sin) >> 12;
				sx2 = ((sw >> 1) * sin) >> 12;
				sy1 = (-(sh >> 1) * sin) >> 12;
				sy2 = ((sh >> 1) * sin) >> 12;
				cx1 = (-(sw >> 1) * cos) >> 12;
				cx2 = ((sw >> 1) * cos) >> 12;
				cy1 = (-(sh >> 1) * cos) >> 12;
				cy2 = ((sh >> 1) * cos) >> 12;

				x1 = x + sx1 - cy1;
				x2 = x + sx2 - cy1;
				x3 = x + sx2 - cy2;
				x4 = x + sx1 - cy2;
				y1 = y + sy1 + cx1;
				y2 = y + cx2 + sy1;
				y3 = y + sy2 + cx2;
				y4 = y + sy2 + cx1;

				r = sptr->R;
				g = sptr->G;
				b = sptr->B;
				c = (r >> 3) << 10 | (g >> 3) << 5 | (b >> 3);

				if (z > distanceFogValue << W2V_SHIFT)
				{
					f = 2048 - ((z - (distanceFogValue << W2V_SHIFT)) >> 16);
					r = (r * f) >> W2V_SHIFT;
					g = (g * f) >> W2V_SHIFT;
					b = (b * f) >> W2V_SHIFT;

					if (r < 0)
						r = 0;

					if (g < 0)
						g = 0;

					if (b < 0)
						b = 0;

					c = r << 10 | g << 5 | b;
				}

				if (sptr->TransType == 2 || sptr->TransType == 3)
					drawType = DT_POLY_WGTA;
				else
					drawType = DT_POLY_WGT;

				HWI_InsertAlphaSprite_Sorted(x1, y1, z, c, x2, y2, z, c, x3, y3, z, c, x4, y4, z, c, sptr->Def, drawType, 0);
				sptr->RotAng = (sptr->RotAng + sptr->RotAdd) & 0xFFF;
			}
			else
			{
				x1 = x - (sw >> 1);
				y1 = y - (sh >> 1);
				x2 = x1 + sw;
				y2 = y1 + sh;

				r = sptr->R;
				g = sptr->G;
				b = sptr->B;
				c = (r >> 3) << 10 | (g >> 3) << 5 | (b >> 3);

				if (z > distanceFogValue << W2V_SHIFT)
				{
					f = 2048 - ((z - (distanceFogValue << W2V_SHIFT)) >> 16);
					r = (r * f) >> W2V_SHIFT;
					g = (g * f) >> W2V_SHIFT;
					b = (b * f) >> W2V_SHIFT;

					if (r < 0)
						r = 0;

					if (g < 0)
						g = 0;

					if (b < 0)
						b = 0;

					c = r << 10 | g << 5 | b;
				}

				if (sptr->TransType == 2 || sptr->TransType == 3)
					drawType = DT_POLY_WGTA;
				else
					drawType = DT_POLY_WGT;

				HWI_InsertAlphaSprite_Sorted(x1, y1, z, c, x2, y1, z, c, x2, y2, z, c, x1, y2, z, c, sptr->Def, drawType, 0);
			}
		}
		else
		{
			if (sptr->Flags & SF_SCALE)
			{
				if (!vpos[0][2])
					vpos[0][2] = 1;

				sw = ((sptr->Width * phd_persp) << sptr->Scalar) / vpos[0][2];
				sh = ((sptr->Height * phd_persp) << sptr->Scalar) / vpos[0][2];

				if (sw > sptr->Width << 2)
					sw = sptr->Width << 2;
				else if (sw < 1)
					sw = 1;

				if (sh > sptr->Height << 2)
					sh = sptr->Height << 2;
				else if (sh < 1)
					sh = 1;
			}
			else
			{
				sw = sptr->Width;
				sh = sptr->Height;
			}

			z = vpos[0][2] << W2V_SHIFT;

			if (z < phd_znear || z > phd_zfar)
				continue;

			x = vpos[0][0];
			y = vpos[0][1];

			if (x + (sw >> 1) < 0 || x - (sw >> 1) > w ||
				y + (sh >> 1) < 0 || y - (sh >> 1) > h)
				continue;

			x1 = x - (sw >> 1);
			y1 = y - (sh >> 1);
			x2 = x1 + sw;
			y2 = y1 + sh;

			r = sptr->R;
			g = sptr->G;
			b = sptr->B;
			c = (r >> 3) << 10 | (g >> 3) << 5 | (b >> 3);

			if (z > distanceFogValue << W2V_SHIFT)
			{
				f = 2048 - ((z - (distanceFogValue << W2V_SHIFT)) >> 16);
				r = (r * f) >> W2V_SHIFT;
				g = (g * f) >> W2V_SHIFT;
				b = (b * f) >> W2V_SHIFT;

				if (r < 0)
					r = 0;

				if (g < 0)
					g = 0;

				if (b < 0)
					b = 0;

				c = r << 10 | g << 5 | b;
			}

			if (sptr->TransType == 2 || sptr->TransType == 3)
				drawType = DT_POLY_GA;
			else
				drawType = DT_POLY_G;

			HWI_InsertAlphaSprite_Sorted(x1, y1, z, c, x2, y1, z, c, x2, y2, z, c, x1, y2, z, c, -1, drawType, 0);
		}
	}
}

void S_DrawSplashes()
{
	DISPLAYMODE* dm;
	SPLASH_STRUCT* splash;
	RIPPLE_STRUCT* ripple;
	SPLASH_VERTS* sv;
	long* p;
	uchar* links;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c, c1, c2;
	long w, h, nSprite, linkNum, n;
	long coords[192];
	long XYZ[3];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w;
	h = dm->h;

	for (int i = 0; i < 4; i++)
	{
		splash = &splashes[i];

		if (!(splash->flags & 1))
			continue;

		p = coords;

		for (int j = 0; j < 48; j++)
		{
			sv = &splash->sv[j];
			mCalcPoint(splash->x + (sv->wx >> 4), splash->y + sv->wy, splash->z + (sv->wz >> 4), XYZ);
			ProjectPCoord(XYZ[0], XYZ[1], XYZ[2], p, w >> 1, h >> 1, phd_persp);
			p += 4;
		}

		p = coords;

		for (int j = 0; j < 3; j++)
		{
			if (j == 2 || !j && splash->flags & 4 || j == 1 && splash->flags & 8)
				nSprite = ((wibble >> 4) & 3) + objects[EXPLOSION1].mesh_index + 4;
			else
				nSprite = objects[EXPLOSION1].mesh_index + 8;

			links = SplashLinks;
			linkNum = j << 6;

			for (int k = 0; k < 8; k++)
			{
				n = *links++ + linkNum;
				x1 = p[n];
				y1 = p[n + 1];
				z1 = p[n + 2];

				n = *links++ + linkNum;
				x2 = p[n];
				y2 = p[n + 1];
				z2 = p[n + 2];

				n = *links++ + linkNum;
				x3 = p[n];
				y3 = p[n + 1];
				z3 = p[n + 2];

				n = *links++ + linkNum;
				x4 = p[n];
				y4 = p[n + 1];
				z4 = p[n + 2];

				if ((x1 < 0 && x2 < 0 && x3 < 0 && x4 < 0) || (x1 >= w && x2 >= w && x3 >= w && x4 >= w) ||
					(y1 < 0 && y2 < 0 && y3 < 0 && y4 < 0) || (y1 >= h && y2 >= h && y3 >= h && y4 >= h))
					continue;

				z1 <<= W2V_SHIFT;

				if (z1 < phd_znear || z1 > phd_zfar)
					continue;

				z2 <<= W2V_SHIFT;

				if (z2 < phd_znear || z2 > phd_zfar)
					continue;

				z3 <<= W2V_SHIFT;

				if (z3 < phd_znear || z3 > phd_zfar)
					continue;

				z4 <<= W2V_SHIFT;

				if (z4 < phd_znear || z4 > phd_zfar)
					continue;

				c = splash->life >> 2;
				c1 = c << 10 | c << 5 | c;

				c -= splash->life >> 4;
				c2 = c << 10 | c << 5 | c;

				if ((x3 - x2) * (y1 - y2) - (y3 - y2) * (x1 - x2) >= 0)
					HWI_InsertAlphaSprite_Sorted(x1, y1, z1, c1, x2, y2, z2, c1, x4, y4, z4, c2, x3, y3, z3, c2, nSprite, DT_POLY_WGTA, 0);
				else
					HWI_InsertAlphaSprite_Sorted(x1, y1, z1, c1, x3, y3, z3, c2, x4, y4, z4, c2, x2, y2, z2, c1, nSprite, DT_POLY_WGTA, 1);
			}
		}
	}

	for (int i = 0; i < 16; i++)
	{
		ripple = &ripples[i];

		if (!(ripple->flags & 1))
			continue;

		p = coords;
		n = ripple->size << 2;
		nSprite = objects[EXPLOSION1].mesh_index + 9;

		mCalcPoint(ripple->x - n, ripple->y, ripple->z - n, XYZ);
		ProjectPCoord(XYZ[0], XYZ[1], XYZ[2], p, w >> 1, h >> 1, phd_persp);
		p += 3;

		mCalcPoint(ripple->x + n, ripple->y, ripple->z - n, XYZ);
		ProjectPCoord(XYZ[0], XYZ[1], XYZ[2], p, w >> 1, h >> 1, phd_persp);
		p += 3;

		mCalcPoint(ripple->x - n, ripple->y, ripple->z + n, XYZ);
		ProjectPCoord(XYZ[0], XYZ[1], XYZ[2], p, w >> 1, h >> 1, phd_persp);
		p += 3;

		mCalcPoint(ripple->x + n, ripple->y, ripple->z + n, XYZ);
		ProjectPCoord(XYZ[0], XYZ[1], XYZ[2], p, w >> 1, h >> 1, phd_persp);

		p -= 9;	//back to the start

		x1 = *p++;
		y1 = *p++;
		z1 = *p++;

		x2 = *p++;
		y2 = *p++;
		z2 = *p++;

		x3 = *p++;
		y3 = *p++;
		z3 = *p++;

		x4 = *p++;
		y4 = *p++;
		z4 = *p++;

		if ((x1 < 0 && x2 < 0 && x3 < 0 && x4 < 0) || (x1 >= w && x2 >= w && x3 >= w && x4 >= w) ||
			(y1 < 0 && y2 < 0 && y3 < 0 && y4 < 0) || (y1 >= h && y2 >= h && y3 >= h && y4 >= h))
			continue;

		z1 <<= W2V_SHIFT;

		if (z1 < phd_znear || z1 > phd_zfar)
			continue;

		z2 <<= W2V_SHIFT;

		if (z2 < phd_znear || z2 > phd_zfar)
			continue;

		z3 <<= W2V_SHIFT;

		if (z3 < phd_znear || z3 > phd_zfar)
			continue;

		z4 <<= W2V_SHIFT;

		if (z4 < phd_znear || z4 > phd_zfar)
			continue;

		if (ripple->flags & 0x10)
		{
			if (ripple->flags & 0x20)
			{
				nSprite = objects[EXPLOSION1].mesh_index;

				if (ripple->init)
					c1 = ripple->init >> 1;
				else
					c1 = ripple->life >> 1;

				c = c1 << 10;	//only red
			}
			else
			{
				if (ripple->init)
					c1 = ripple->init >> 2;
				else
					c1 = ripple->life >> 2;

				c = c1 << 10 | c1 << 5 | c1;
			}
		}
		else
		{
			if (ripple->init)
				c1 = ripple->init >> 1;
			else
				c1 = ripple->life >> 1;

			c = c1 << 10 | c1 << 5 | c1;
		}

		if ((x3 - x2) * (y1 - y2) - (y3 - y2) * (x1 - x2) >= 0)
			HWI_InsertAlphaSprite_Sorted(x1, y1, z1, c, x2, y2, z2, c, x4, y4, z4, c, x3, y3, z3, c, nSprite, DT_POLY_WGTA, 0);
		else
			HWI_InsertAlphaSprite_Sorted(x1, y1, z1, c, x3, y3, z3, c, x4, y4, z4, c, x2, y2, z2, c, nSprite, DT_POLY_WGTA, 1);
	}
}

void S_DrawLaserBolts(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	PHD_VECTOR rad[4];
	PHD_VECTOR sub[4];
	PHD_VECTOR pos;
	long* pZ;
	short* pXY;
	uchar* pC;
	char* links;
	float zv;
	long w, h, speed, px, py, pz, d, x, y, z, xStep, yStep, zStep, num, linkNum;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, c12, c34, r, g, b, c0, c1;
	long Z[128];
	short XY[128];
	uchar cols[128];

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;
	speed = (item->speed * phd_cos(item->pos.x_rot)) >> W2V_SHIFT;
	px = (speed * phd_sin(item->pos.y_rot)) >> W2V_SHIFT;
	py = -((item->speed * phd_sin(item->pos.x_rot)) >> W2V_SHIFT);
	pz = (speed * phd_cos(item->pos.y_rot)) >> W2V_SHIFT;

	phd_PushUnitMatrix();
	phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

	d = item->item_flags[1];

	if (item->item_flags[2])
		d >>= 1;

	for (int i = 0; i < 4; i++)
	{
		if (!i)
		{
			x = -d;
			y = -d;
		}
		else if (i == 1)
		{
			x = d;
			y = -d;
		}
		else if (i == 2)
		{
			x = d;
			y = d;
		}
		else
		{
			x = -d;
			y = d;
		}

		rad[i].x = (x * phd_mxptr[M00] + y * phd_mxptr[M01]) >> W2V_SHIFT;	//z = 0
		rad[i].y = (x * phd_mxptr[M10] + y * phd_mxptr[M11]) >> W2V_SHIFT;
		rad[i].z = (x * phd_mxptr[M20] + y * phd_mxptr[M21]) >> W2V_SHIFT;
		sub[i].x = rad[i].x >> 2;
		sub[i].y = rad[i].y >> 2;
		sub[i].z = rad[i].z >> 2;
	}

	phd_PopMatrix();

	pXY = XY;
	pZ = Z;
	pC = cols;

	phd_PushMatrix();
	phd_TranslateAbs(lara_item->pos.x_pos, lara_item->pos.y_pos, lara_item->pos.z_pos);

	x = item->pos.x_pos - lara_item->pos.x_pos;
	y = item->pos.y_pos - lara_item->pos.y_pos;
	z = item->pos.z_pos - lara_item->pos.z_pos;
	pos.x = x * phd_mxptr[M00] + y * phd_mxptr[M01] + z * phd_mxptr[M02] + phd_mxptr[M03];
	pos.y = x * phd_mxptr[M10] + y * phd_mxptr[M11] + z * phd_mxptr[M12] + phd_mxptr[M13];
	pos.z = x * phd_mxptr[M20] + y * phd_mxptr[M21] + z * phd_mxptr[M22] + phd_mxptr[M23];
	zv = f_persp / pos.z;
	pos.x = short(float(pos.x * zv + f_centerx));
	pos.y = short(float(pos.y * zv + f_centery));
	pXY[0] = (short)pos.x;
	pXY[1] = (short)pos.y;
	pZ[0] = pos.z;
	pXY += 2;
	pZ++;
	*pC++ = 128;

	xStep = px << 1;
	yStep = py << 1;
	zStep = pz << 1;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			x = item->pos.x_pos + rad[j].x - xStep - lara_item->pos.x_pos;
			y = item->pos.y_pos + rad[j].y - yStep - lara_item->pos.y_pos;
			z = item->pos.z_pos + rad[j].z - zStep - lara_item->pos.z_pos;
			pos.x = x * phd_mxptr[M00] + y * phd_mxptr[M01] + z * phd_mxptr[M02] + phd_mxptr[M03];
			pos.y = x * phd_mxptr[M10] + y * phd_mxptr[M11] + z * phd_mxptr[M12] + phd_mxptr[M13];
			pos.z = x * phd_mxptr[M20] + y * phd_mxptr[M21] + z * phd_mxptr[M22] + phd_mxptr[M23];
			zv = f_persp / pos.z;
			pos.x = short(float(pos.x * zv + f_centerx));
			pos.y = short(float(pos.y * zv + f_centery));
			pXY[0] = (short)pos.x;
			pXY[1] = (short)pos.y;
			pZ[0] = pos.z;
			pXY += 2;
			pZ++;

			rad[j].x -= sub[j].x;
			rad[j].y -= sub[j].y;
			rad[j].z -= sub[j].z;
		}

		*pC++ = 64 - (i << 4);
		xStep += px;
		yStep += py;
		zStep += pz;

		if (!i && item->item_flags[2])
		{
			xStep += px << 1;
			yStep += py << 1;
			zStep += pz << 1;
			break;
		}
	}

	x = item->pos.x_pos - xStep - lara_item->pos.x_pos;
	y = item->pos.y_pos - yStep - lara_item->pos.y_pos;
	z = item->pos.z_pos - zStep - lara_item->pos.z_pos;
	pos.x = x * phd_mxptr[M00] + y * phd_mxptr[M01] + z * phd_mxptr[M02] + phd_mxptr[M03];
	pos.y = x * phd_mxptr[M10] + y * phd_mxptr[M11] + z * phd_mxptr[M12] + phd_mxptr[M13];
	pos.z = x * phd_mxptr[M20] + y * phd_mxptr[M21] + z * phd_mxptr[M22] + phd_mxptr[M23];
	zv = f_persp / pos.z;
	pos.x = short(float(pos.x * zv + f_centerx));
	pos.y = short(float(pos.y * zv + f_centery));
	pXY[0] = (short)pos.x;
	pXY[1] = (short)pos.y;
	pZ[0] = pos.z;
	*pC = 0;
	pC = cols;

	if (item->item_flags[2])
		links = &BoltSummonLinks[0][0];
	else
		links = &BoltLinks[0][0];

	num = 0;

	if (item->item_flags[2])
		c12 = (pC[0] * (16 - item->item_flags[3])) >> 4;
	else
		c12 = pC[0];

	pC++;

	for (int i = 0; i < 5; i++)
	{
		if (item->item_flags[2])
			c34 = (pC[0] * (16 - item->item_flags[3])) >> 4;
		else
			c34 = pC[0];

		pC++;

		for (int j = 0; j < 4; j++)
		{
			linkNum = num << 2;

			x1 = XY[links[linkNum + 1] << 1];
			y1 = XY[(links[linkNum + 1] << 1) + 1];
			z1 = Z[links[linkNum + 1]];

			x2 = XY[links[linkNum] << 1];
			y2 = XY[(links[linkNum] << 1) + 1];
			z2 = Z[links[linkNum]];

			x3 = XY[links[linkNum + 2] << 1];
			y3 = XY[(links[linkNum + 2] << 1) + 1];
			z3 = Z[links[linkNum + 2]];

			x4 = XY[links[linkNum + 3] << 1];
			y4 = XY[(links[linkNum + 3] << 1) + 1];
			z4 = Z[links[linkNum + 3]];

			z1 = (z1 + z2 + z3 + z4) >> 2;

			if (z1 <= phd_znear + 0x3E8000)
				continue;

			if (x1 <= -128 || x2 <= -128 || x3 <= -128 || x4 <= -128 ||
				x1 >= w + 128 || x2 >= w + 128 || x3 >= w + 128 || x4 >= w + 128)
				continue;

			if (y1 <= -128 || y2 <= -128 || y3 <= -128 || y4 <= -128 ||
				y1 >= h + 128 || y2 >= h + 128 || y3 >= h + 128 || y4 >= h + 128)
				continue;

			if (z1 < phd_znear)
				z1 = phd_znear;

			if (z2 < phd_znear)
				z2 = phd_znear;

			if (z3 < phd_znear)
				z3 = phd_znear;

			if (z4 < phd_znear)
				z4 = phd_znear;

			r = c12 >> 5;
			g = c12 >> 3;
			b = c12 >> 4;
			c0 = r << 10 | g << 5 | b;

			r = c34 >> 5;
			g = c34 >> 3;
			b = c34 >> 4;
			c1 = r << 10 | g << 5 | b;

			HWI_InsertAlphaSprite_Sorted(x1, y1, z1, c0, x2, y2, z2, c0, x3, y3, z3, c1, x4, y4, z4, c1, -1, DT_POLY_GTA, 1);
			num++;
		}

		if (i == 1 && item->item_flags[2])
			break;

		c12 = c34;
	}

	phd_PopMatrix();
}

void S_DrawFish(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	PHDSPRITESTRUCT* sprite;
	FISH_INFO* pFish;
	PHD_VBUF v[3];
	PHDTEXTURESTRUCT tex;
	long w, h, sx, sy, x, y, z, ang, size;
	long x1, y1, z1, x2, y2, z2, x3, y3, z3;
	long XYZ[3][3];	//3 fishies x 3 vertices
	long point[3];
	ushort u1, v1, u2, v2;
	short g;

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w;
	h = dm->h;
	sx = phd_winxmin + phd_winxmax;
	sy = phd_winymax + phd_winymin;

	if (!item->active || item->hit_points == NO_ITEM || !lead_info[item->hit_points].on)
		return;

	if (item->object_number == PIRAHNAS)
		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 10];
	else
		sprite = &phdspriteinfo[objects[EXPLOSION1].mesh_index + 11];

	pFish = &fish[24 * item->hit_points + 8];

	for (int i = 0; i < 24; i++, pFish++)
	{
		x = item->pos.x_pos + pFish->x;
		y = item->pos.y_pos + pFish->y;
		z = item->pos.z_pos + pFish->z;
		ang = ((rcossin_tbl[pFish->swim << 7] >> 5) + pFish->angle - 2048) & 0xFFF;

		mCalcPoint(x, y, z, point);
		ProjectPCoord(point[0], point[1], point[2], XYZ[0], w >> 1, h >> 1, phd_persp);

		size = (128 * rcossin_tbl[i << 7] >> 12) + 192;
		x -= (size * rcossin_tbl[ang << 1]) >> 12;
		y -= size;
		z += (size * rcossin_tbl[(ang << 1) + 1]) >> 12;

		mCalcPoint(x, y, z, point);
		ProjectPCoord(point[0], point[1], point[2], XYZ[1], w >> 1, h >> 1, phd_persp);

		y += size << 1;

		mCalcPoint(x, y, z, point);
		ProjectPCoord(point[0], point[1], point[2], XYZ[2], w >> 1, h >> 1, phd_persp);

		x1 = XYZ[0][0];
		y1 = XYZ[0][1];
		z1 = XYZ[0][2];
		x2 = XYZ[1][0];
		y2 = XYZ[1][1];
		z2 = XYZ[1][2];
		x3 = XYZ[2][0];
		y3 = XYZ[2][1];
		z3 = XYZ[2][2];

		if (z1 > 0x5000 || z1 < 32 || z2 < 32 || z3 < 32)
			continue;

		if ((x1 < phd_winxmin && x2 < phd_winxmin && x3 < phd_winxmin) || (x1 >= sx && x2 >= sx && x3 >= sx))
			continue;

		if ((y1 < phd_winymin && y2 < phd_winymin && y3 < phd_winymin) || (y1 >= sy && y2 >= sy && y3 >= sy))
			continue;

		if (ang < 1024)
			ang -= 512;
		else if (ang < 2048)
			ang -= 1536;
		else if (ang < 3072)
			ang -= 2560;
		else
			ang -= 3584;

		if (ang > 512 || ang < 0)
			ang = 0;
		else if (ang < 256)
			ang >>= 2;
		else
			ang = (512 - ang) >> 2;

		ang += i;

		if (ang > 128)
			ang = 128;

		ang += 80;
		ang >>= 3;
		g = short(ang << 10 | ang << 5 | ang);

		z1 <<= W2V_SHIFT;
		z2 <<= W2V_SHIFT;
		z3 <<= W2V_SHIFT;

		setXYZ3(v, 1, x1, y1, z1, g, x2, y2, z2, g, x3, y3, z3, g);
		u1 = (sprite->offset << 8) & 0xFF00;
		v1 = sprite->offset & 0xFF00;
		u2 = ushort(u1 + sprite->width - App.nUVAdd);
		v2 = ushort(v1 + sprite->height - App.nUVAdd);
		u1 += (ushort)App.nUVAdd;
		v1 += (ushort)App.nUVAdd;
		tex.drawtype = 1;
		tex.tpage = sprite->tpage;

		if (item->object_number == PIRAHNAS)
			g = (i & 1) != 0;
		else
			g = (item->hit_points & 1) != 0;

		if (g)
		{
			v[0].u = u1;
			v[0].v = v1;
			v[1].u = u2;
			v[1].v = v1;
			v[2].u = u1;
			v[2].v = v2;
		}
		else
		{
			v[0].u = u2;
			v[0].v = v2;
			v[1].u = u1;
			v[1].v = v2;
			v[2].u = u2;
			v[2].v = v1;
		}

		HWI_InsertGT3_Poly(v, &v[1], &v[2], &tex, &v[0].u, &v[1].u, &v[2].u, MID_SORT, 1);
	}
}

void S_DrawDarts(ITEM_INFO* item)
{
	DISPLAYMODE* dm;
	PHD_VECTOR pos;
	float zv;
	long w, h, size, x, y, z;
	long x1, y1, z1, x2, y2, z2;

	dm = &App.DeviceInfoPtr->DDInfo[App.DXConfigPtr->nDD].D3DInfo[App.DXConfigPtr->nD3D].DisplayMode[App.DXConfigPtr->nVMode];
	w = dm->w - 1;
	h = dm->h - 1;

	phd_PushMatrix();
	phd_TranslateAbs(item->pos.x_pos, item->pos.y_pos, item->pos.z_pos);

	zv = f_persp / (float)phd_mxptr[M23];
	x1 = short(float(phd_mxptr[M03] * zv + f_centerx));
	y1 = short(float(phd_mxptr[M13] * zv + f_centery));
	z1 = phd_mxptr[M23];

	size = (-96 * phd_cos(item->pos.x_rot)) >> W2V_SHIFT;
	x = (size * phd_sin(item->pos.y_rot)) >> W2V_SHIFT;
	y = (96 * phd_sin(item->pos.x_rot)) >> W2V_SHIFT;
	z = (size * phd_cos(item->pos.y_rot)) >> W2V_SHIFT;
	pos.x = x * phd_mxptr[M00] + y * phd_mxptr[M01] + z * phd_mxptr[M02] + phd_mxptr[M03];
	pos.y = x * phd_mxptr[M10] + y * phd_mxptr[M11] + z * phd_mxptr[M12] + phd_mxptr[M13];
	pos.z = x * phd_mxptr[M20] + y * phd_mxptr[M21] + z * phd_mxptr[M22] + phd_mxptr[M23];
	zv = f_persp / (float)pos.z;
	x2 = short(float(pos.x * zv + f_centerx));
	y2 = short(float(pos.y * zv + f_centery));
	z2 = pos.z;

	if (z1 > 32 && z2 > 32 && ClipLine(x1, y1, x2, y2, w, h))
		HWI_InsertLine_Sorted(x1 - phd_winxmin, y1 - phd_winymin, x2 - phd_winxmin, y2 - phd_winymin, z1, 0, 0x783C14);

	phd_PopMatrix();
}
