/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#ifndef CONSOLE
#include "screengrab.h"
#include "Paths.h"
#include <string>
#include "Zeven.h"
#include "GameVar.h"
#include "Game.h"
#include "Player.h"
#include "Scene.h"
#include "glheaders.h"
#include <time.h>
#include <stdio.h>


extern Scene* scene;

// Writes a bottom-up 32 bpp BMP (BITMAPFILEHEADER + BITMAPINFOHEADER, little endian) with plain stdio.
static void put16(FILE * f, unsigned int v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }
static void put32(FILE * f, unsigned int v) { put16(f, v & 0xFFFF); put16(f, v >> 16); }

void SaveBitmapToFile( const unsigned char* pBitmapBits, int lWidth, int lHeight, int wBitsPerPixel, const char* lpszFileName )
{
	const unsigned int headers = 14 + 40;
	const unsigned int imageSize = (unsigned int)(lWidth * lHeight * (wBitsPerPixel / 8));

	FILE * f = fopen(lpszFileName, "wb");
	if (!f)
		return;

	// BITMAPFILEHEADER
	put16(f, 0x4D42); // "BM"
	put32(f, headers + imageSize);
	put32(f, 0);
	put32(f, headers); // offset to the pixels
	// BITMAPINFOHEADER
	put32(f, 40);
	put32(f, (unsigned int)lWidth);
	put32(f, (unsigned int)lHeight);
	put16(f, 1); // planes
	put16(f, (unsigned int)wBitsPerPixel);
	put32(f, 0); // BI_RGB
	put32(f, imageSize);
	put32(f, 0);
	put32(f, 0);
	put32(f, 0);
	put32(f, 0);

	fwrite(pBitmapBits, 1, imageSize, f);
	fclose(f);
}

bool SaveScreenGrabAuto() 
{
   char path[512];
   time_t time;
   ::time(&time);
   snprintf(path, sizeof(path), "%s", bv2::userFile("screenshots/SS_" + std::to_string((long long)time) + ".bmp").c_str());
   return SaveScreenGrab(path);
}

bool SaveStatsAuto()
{
   char path[512];
   time_t time;
   ::time(&time);
   snprintf(path, sizeof(path), "%s", bv2::userFile("screenshots/SS_" + std::to_string((long long)time) + ".bmp").c_str());
   SaveScreenGrab(path);
   snprintf(path, sizeof(path), "%s", bv2::userFile("screenshots/SS_" + std::to_string((long long)time) + ".txt").c_str());

  FILE * pFile;
  pFile = fopen (path,"w");
  if (pFile!=NULL)
  {
/*
	CString mapName;

	// Le type de jeu et les scores
	int gameType;
   int spawnType;
   int subGameType;
	int blueScore;
	int redScore;
	int blueWin;
	int redWin;
   */
    Game* pGame = scene->client->game;


    fputs (pGame->mapName.s,pFile);
    fprintf(pFile, "\nBlue:%d\nRed:%d\n", pGame->blueScore, pGame->redScore);
   
	 for (int i=0;i<MAX_PLAYER;++i)
	 {
       if (pGame->players[i])
		 {         
       Player* pPlayer = pGame->players[i];       
       fprintf(pFile, "PlayerName:%s\n", textColorLess(pPlayer->name).s);       
       fprintf(pFile, "PlayerId:%d\nTeamId:%d\n", pPlayer->playerID, pPlayer->teamID);              
       fprintf(pFile, "Kills:%d\n", pPlayer->kills);
       fprintf(pFile, "Deaths:%d\n", pPlayer->deaths);
       fprintf(pFile, "Damage:%f\n", pPlayer->dmg);
       fprintf(pFile, "FlagAttempts:%d\n", pPlayer->flagAttempts);
       fprintf(pFile, "Returns:%d\n", pPlayer->returns);
       fprintf(pFile, "Score:%d\n", pPlayer->score);       
		 }
	 }
   
    fclose(pFile);
  }
 
   return true;
}



bool SaveScreenGrab(const char* filename) {

	// get some info about the screen size
   CVector2i res = dkwGetResolution();
   
   int sw           =  res.x();
	int sh           =  res.y();
   int bitdepth     =  32;
   GLenum   format  =  GL_BGRA_EXT;
   int bpp          =  4;
	//int bitdepth     =  gameVar.r_bitdepth;
	//GLenum   format  =  (bitdepth==32) ? GL_BGRA_EXT : GL_BGR_EXT;
   //int bpp          =  (bitdepth==32) ? 4 : 3;

	// allocate memory to store image data
	unsigned char* pdata = new unsigned char[sw*sh*bpp];
	// read from front buffer
	glReadBuffer(GL_FRONT);

	// read pixel data
	glReadPixels(0,0,sw,sh,format,GL_UNSIGNED_BYTE,pdata);

	// write data as a tga file
   SaveBitmapToFile(pdata,sw,sh,bitdepth,filename);

	// clean up
	delete [] pdata;

	// done
	return true;
}

#endif
