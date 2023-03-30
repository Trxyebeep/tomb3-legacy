#pragma once
#include "../global/types.h"

void ModifyStartInfo(long level);
void InitialiseStartInfo();
void CreateStartInfo(long level);
void ResetSG();
void WriteSG(void* pointer, long size);
void ReadSG(void* pointer, long size);
void CreateSaveGameInfo();
void ExtractSaveGameInfo();

extern SAVEGAME_INFO savegame;
