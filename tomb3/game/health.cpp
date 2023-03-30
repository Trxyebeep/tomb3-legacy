#include "../tomb3/pch.h"
#include "health.h"
#include "objects.h"
#include "../3dsystem/scalespr.h"
#include "../specific/drawbars.h"
#include "text.h"
#include "../specific/specific.h"
#include "gameflow.h"
#include "../3dsystem/3d_gen.h"
#include "control.h"
#include "savegame.h"
#include "lara.h"
#include "effects.h"

#define AMMO_XPOS_PC	-10
#define AMMO_YPOS_PC	50

#define AMMO_XPOS_PS	-24
#define AMMO_YPOS_PS	-24

long health_bar_timer;
TEXTSTRING* ammotext;
static TEXTSTRING* LpModeTS;
static long LnModeTSLife;
DISPLAYPU pickups[1];

long FlashIt()
{
	static long flash_state = 0, flash_count = 0;

	if (flash_count)
		flash_count--;
	else
	{
		flash_state ^= 1;
		flash_count = 5;
	}

	return flash_state;
}

void DrawAssaultTimer()
{
	char* txt;
	long timer, x, y, h, v, d0, d1, d2;
	char buffer[8];

	if (CurrentLevel == LV_GYM && assault_timer_display && !QuadbikeLapTimeDisplayTimer)
	{
		if (savegame.timer > 0x1A5DD)
			savegame.timer = 0x1A5DD;

		timer = savegame.timer;

		if (timer < 2)
			timer = 0;

		sprintf(buffer, "%d:%02d.%02d", timer / 30 / 60, timer / 30 % 60, 334 * (timer % 30) / 100);
		x = (phd_winxmax >> 1) - 50;
		y = 36;
		h = 0x10000;
		v = 0x10000;
		d0 = -6;
		d1 = 14;
		d2 = 20;

		for (txt = buffer; *txt; txt++)
		{
			if (*txt == ':')
			{
				x += d0;
				S_DrawScreenSprite2d(x, y, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 10, 2, 0);
				x += d1;
			}
			else if (*txt == '.')
			{
				x += d0;
				S_DrawScreenSprite2d(x, y, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 11, 2, 0);
				x += d1;
			}
			else
			{
				S_DrawScreenSprite2d(x, y, 0, h, v, *txt + objects[ASSAULT_NUMBERS].mesh_index - '0', 2, 0);
				x += d2;
			}
		}
	}
}

void DrawAssaultPenalties(long type)
{
	char* txt;
	long timer, x, y, h, v, p, d0, d1, d2, d3;
	char buffer[12];

	if (CurrentLevel == LV_GYM && assault_penalty_display_timer && assault_timer_display)
	{
		if (type)
		{
			if (!assault_target_penalties)
				return;

			if (assault_target_penalties > 0x1A5DD)
				assault_target_penalties = 0x1A5DD;

			x = (phd_winxmax >> 1) - 193;
			y = !assault_penalties ? 36 : 64;
			timer = assault_target_penalties;
			sprintf(buffer, "T %d:%02d s", timer / 30 / 60, timer / 30 % 60);
		}
		else
		{
			if (!assault_penalties)
				return;

			if (assault_penalties > 0x1A5DD)
				assault_penalties = 0x1A5DD;

			x = (phd_winxmax >> 1) - 175;
			y = 36;
			timer = assault_penalties;
			sprintf(buffer, "%d:%02d s", timer / 30 / 60, timer / 30 % 60);
		}

		h = 0x10000;
		v = 0x10000;
		p = 1;
		d0 = -6;
		d1 = 14;
		d2 = 20;
		d3 = 8;

		for (txt = buffer; *txt; txt++)
		{
			if (*txt == ' ')
				x += d3;
			else if (*txt == 'T')
			{
				x += d0;
				S_DrawScreenSprite2d(x, y + p, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 12, 0, 0);
				x += d1 + (p * 2);
			}
			else if (*txt == 's')
			{
				x += d0;
				S_DrawScreenSprite2d(x - (p * 4), y, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 13, 9, 0);
				x += d1;
			}
			else if (*txt == ':')
			{
				x += d0;
				S_DrawScreenSprite2d(x, y, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 10, 9, 0);
				x += d1;
			}
			else if (*txt == '.')
			{
				x += d0;
				S_DrawScreenSprite2d(x, y, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 11, 9, 0);
				x += d1;
			}
			else
			{
				S_DrawScreenSprite2d(x, y, 0, h, v, *txt + objects[ASSAULT_NUMBERS].mesh_index - '0', 9, 0);
				x += d2;
			}
		}
	}
}

void DrawQuadbikeLapTime()
{
	char* txt;
	long timer, x, y, h, v, d0, d1, d2, hundredth;
	short col;
	char buffer[8];

	if (CurrentLevel == LV_GYM && QuadbikeLapTimeDisplayTimer)
	{
		for (int i = 0; i < 2; i++)
		{
			if (i)
			{
				timer = savegame.best_quadbike_times[0] / 0x1E;
				col = 10;
				hundredth = 334 * (savegame.best_quadbike_times[0] % 0x1E) / 0x64;
				x = (phd_winxmax >> 1) + 100;
			}
			else
			{
				col = 9;
				timer = QuadbikeLapTime / 30;
				hundredth = 334 * (QuadbikeLapTime % 30) / 100;
				x = (phd_winxmax >> 1) - 50;
			}

			y = 36;
			h = 0x10000;
			v = 0x10000;
			d0 = -6;
			d1 = 14;
			d2 = 20;
			sprintf(buffer, "%d:%02d.%02d", timer / 60, timer % 60, hundredth);

			for (txt = buffer; *txt; txt++)
			{
				if (*txt == ':')
				{
					x += d0;
					S_DrawScreenSprite2d(x, y, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 10, col, 0);
					x += d1;
				}
				else if (*txt == '.')
				{
					x += d0;
					S_DrawScreenSprite2d(x, y, 0, h, v, objects[ASSAULT_NUMBERS].mesh_index + 11, col, 0);
					x += d1;
				}
				else
				{
					S_DrawScreenSprite2d(x, y, 0, h, v, *txt + objects[ASSAULT_NUMBERS].mesh_index - '0', col, 0);
					x += d2;
				}
			}
		}
	}
}

void DrawHealthBar(long flash_state)
{
	static long old_hitpoints = 0;
	long hitpoints;

	hitpoints = lara_item->hit_points;

	if (hitpoints < 0)
		hitpoints = 0;
	else if (hitpoints > 1000)
		hitpoints = 1000;

	if (old_hitpoints != hitpoints)
	{
		old_hitpoints = hitpoints;
		health_bar_timer = 40;
	}

	if (health_bar_timer < 0)
		health_bar_timer = 0;

	if (hitpoints <= 250)
	{
		if (flash_state)
			S_DrawHealthBar(hitpoints / 10);
		else
			S_DrawHealthBar(0);
	}
	else if (health_bar_timer > 0 || hitpoints <= 0 || lara.gun_status == LG_READY || lara.poisoned)
		S_DrawHealthBar(hitpoints / 10);

	if (PoisonFlag)
		PoisonFlag--;
}

void DrawAirBar(long flash_state)
{
	long air;

	if (lara.skidoo != NO_ITEM && items[lara.skidoo].object_number == UPV || lara.water_status == LARA_UNDERWATER ||
		lara.water_status == LARA_SURFACE || room[lara_item->room_number].flags & ROOM_SWAMP && lara.water_surface_dist < -775)
	{
		air = lara.air;

		if (lara.air < 0)
			air = 0;
		else if (lara.air > 1800)
			air = 1800;

		if (air > 450 || flash_state)
			S_DrawAirBar(100 * air / 1800);
		else
			S_DrawAirBar(0);
	}
}

void RemoveAmmoText()
{
	if (ammotext)
	{
		T_RemovePrint(ammotext);
		ammotext = 0;
	}
}

void DrawAmmoInfo()
{
	char txt[80];

	txt[0] = 0;

	if (items[lara.skidoo].object_number == UPV)
		wsprintf(txt, "%5d", lara.harpoon.ammo);
	else
	{
		if (lara.gun_status != LG_READY || overlay_flag <= 0 || savegame.bonus_flag)
		{
			RemoveAmmoText();
			return;
		}

		switch (lara.gun_type)
		{
		case LG_MAGNUMS:
			wsprintf(txt, "%5d", lara.magnums.ammo);
			break;

		case LG_UZIS:
			wsprintf(txt, "%5d", lara.uzis.ammo);
			break;

		case LG_SHOTGUN:
			wsprintf(txt, "%5d", lara.shotgun.ammo / 6);
			break;

		case LG_M16:
			wsprintf(txt, "%5d", lara.m16.ammo);
			break;

		case LG_ROCKET:
			wsprintf(txt, "%5d", lara.rocket.ammo);
			break;

		case LG_GRENADE:
			wsprintf(txt, "%5d", lara.grenade.ammo);
			break;

		case LG_HARPOON:
			wsprintf(txt, "%5d", lara.harpoon.ammo);
			break;

		default:
			return;
		}
	}

	RemoveAmmoText();
	ammotext = T_Print(AMMO_XPOS_PC, AMMO_YPOS_PC, 0, txt);
	T_RightAlign(ammotext, 1);
}

void DrawModeInfo()
{
	if (LpModeTS)
	{
		LnModeTSLife--;

		if (!LnModeTSLife)
		{
			T_RemovePrint(LpModeTS);
			LpModeTS = 0;
		}
	}
}

void DisplayModeInfo(char* string)
{
	if (string)
	{
		if (LpModeTS)
			T_ChangeText(LpModeTS, string);
		else
		{
			LpModeTS = T_Print(-16, -16, 0, string);
			T_RightAlign(LpModeTS, 1);
			T_BottomAlign(LpModeTS, 1);
		}

		LnModeTSLife = 75;
	}
	else
	{
		T_RemovePrint(LpModeTS);
		LpModeTS = 0;
	}
}

void DrawGameInfo(long timed)
{
	long flash_state;

	DrawAmmoInfo();
	DrawModeInfo();

	if (overlay_flag > 0)
	{
		flash_state = FlashIt();
		DrawAssaultTimer();
		DrawAssaultPenalties(0);
		DrawAssaultPenalties(1);
		DrawQuadbikeLapTime();
		DrawHealthBar(flash_state);
		DrawAirBar(flash_state);

		if (DashTimer < 120)
			S_DrawDashBar(100 * DashTimer / 120);

		if (ExposureMeter < 600)
		{
			if (ExposureMeter < 0)
				S_DrawColdBar(0);
			else if (ExposureMeter >= 150 || !flash_state)
				S_DrawColdBar(100 * ExposureMeter / 600);
		}
	}

	T_DrawText();
}

void InitialisePickUpDisplay()
{
	for (int i = 0; i < 1; i++)
		pickups[i].duration = 0;
}

void AddDisplayPickup(short objnum)
{
	if (objnum == SECRET_ITEM1 || objnum == SECRET_ITEM2 || objnum == SECRET_ITEM3)
		S_CDPlay(gameflow.secret_track, 0);

	pickups[0].sprnum = objnum;
	pickups[0].duration = 75;
}
