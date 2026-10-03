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

#include "platform.h"

#include "Zeven.h"
#include "Scene.h"
#include "Console.h"
#include <exception>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "CMaster.h"
#include "Paths.h"
#include <string>

// After main/bv2.cfg is loaded: BV2_NETLOG=1 forces c_netlog on, BV2_NETLOG=0 off.
static void bv2ApplyNetlogFromEnv()
{
	const char* e = std::getenv("BV2_NETLOG");
	if (e && e[0] != '\0')
		gameVar.c_netlog = (std::strcmp(e, "0") != 0);
}
#ifndef CONSOLE
	#include "CStatus.h"
	#include "CLobby.h"
#endif




// notre scene
Scene * scene = 0;


int resW = 800;
int resH = 600;
bool fullscreen = false;

char * bbNetVersion;

#ifdef CONSOLE
#include <atomic>
std::atomic<bool> quit(false);
void dkwForceQuit()
{
	quit = true;
}
#else
CVector2i mousePos_xbox;
CVector2f mousePos_xboxVel;
void updateXBoxMouse_main(float delay)
{
	CVector3f joy = dkiGetJoyR();

	joy[2] = 0;

	if (fabsf(joy[0]) < .25f) joy[0] = 0;
	if (fabsf(joy[1]) < .25f) joy[1] = 0;

	CVector2i res = dkwGetResolution();

	mousePos_xboxVel[0] += joy[0] * (30 + fabsf(mousePos_xboxVel[0]) * 0.0f) * delay;
	mousePos_xboxVel[1] += joy[1] * (30 + fabsf(mousePos_xboxVel[1]) * 0.0f) * delay;
	if (mousePos_xboxVel[0] > 2.5f) mousePos_xboxVel[0] = 2.5f;
	if (mousePos_xboxVel[0] < -2.5f) mousePos_xboxVel[0] = -2.5f;
	if (mousePos_xboxVel[1] > 2.5f) mousePos_xboxVel[1] = 2.5f;
	if (mousePos_xboxVel[1] < -2.5f) mousePos_xboxVel[1] = -2.5f;

	mousePos_xbox[0] += (int)(mousePos_xboxVel[0] * 600 * delay);
	mousePos_xbox[1] += (int)(mousePos_xboxVel[1] * 600 * delay);
	if (mousePos_xbox[0] < 0)
	{
		mousePos_xbox[0] = 0;
		mousePos_xboxVel[0] = 0;
	}
	if (mousePos_xbox[1] < 0)
	{
		mousePos_xbox[1] = 0;
		mousePos_xboxVel[1] = 0;
	}
	if (mousePos_xbox[0] > res[0])
	{
		mousePos_xbox[0] = res[0] - 1;
		mousePos_xboxVel[0] = 0;
	}
	if (mousePos_xbox[1] > res[1])
	{
		mousePos_xbox[1] = res[1] - 1;
		mousePos_xboxVel[1] = 0;
	}

	if (mousePos_xboxVel[0] > 0)
	{
		mousePos_xboxVel[0] -= delay * 25;
		if (mousePos_xboxVel[0] < 0) mousePos_xboxVel[0] = 0;
	}
	if (mousePos_xboxVel[0] < 0)
	{
		mousePos_xboxVel[0] += delay * 25;
		if (mousePos_xboxVel[0] > 0) mousePos_xboxVel[0] = 0;
	}
	if (mousePos_xboxVel[1] > 0)
	{
		mousePos_xboxVel[1] -= delay * 25;
		if (mousePos_xboxVel[1] < 0) mousePos_xboxVel[1] = 0;
	}
	if (mousePos_xboxVel[1] < 0)
	{
		mousePos_xboxVel[1] += delay * 25;
		if (mousePos_xboxVel[1] > 0) mousePos_xboxVel[1] = 0;
	}
}

CVector2i dkwGetCursorPos_main()
{
	if (gameVar.cl_enableXBox360Controller)
	{
		return mousePos_xbox;
	}
	else
	{
		return dkwGetCursorPos();
	}
}
#endif


#ifndef CONSOLE

class MainLoopInterface : public CMainLoopInterface
{
public:



	// Les fonctions obligatoire du MainLoopInterface de la dll dkw
	void paint()
	{

		// On va updater notre timer
		int nbFrameElapsed = dkcUpdateTimer();

#ifdef _DEBUG
		// LAG GENERATOR , use it to bind a key and test in lag conditions
		if (dkiGetState(DIK_BACK) == DKI_DOWN)
		{
			dkcSleep(300);
		}
#endif


		// On va chercher notre delay
		float delay = dkcGetElapsedf();

		// On passe le nombre de frame �animer
		while (nbFrameElapsed)
		{
			// On update nos input
			dkiUpdate(delay, resW, resH);

			// Xbox mouse pos
			updateXBoxMouse_main(delay);

			// On update le writing
			if (writting) writting->updateWritting(delay);

			// Update la console
			console->update(delay);

			// On appel nos fonction pour animer ici
			scene->update(delay);

			// On d�r�ente pour le prochain frame
			nbFrameElapsed--;
		}

		// On render le tout
		scene->render();

		// Present the frame
		dkglSwapBuffers();

      #ifdef NDEBUG
      #ifdef BV2_PLATFORM_WINDOWS
  //    if (IsDebuggerPresent() == TRUE)
  //       {
  //       throw(0);
  //       }
      #endif  
      #endif
	}

	void textWrite(unsigned int caracter)
	{
		// Voil�juste un writting peut avoir le focus �la fois
		if (writting) writting->writeText(caracter);
	}
} mainLoopInterface;
#endif



class StringInterface : public CStringInterface
{
public:
	virtual void updateString(CString* string, char * newValue)
	{
		*string = newValue;
	}
    StringInterface()
        {
            //printf("Constructor: 0x%x\n", this);
        }
    virtual ~StringInterface()
        {
        }
} stringInterface;



#ifdef CONSOLE

#include "CThread.h"
#include <mutex>
#include <thread>
#include <chrono>

// Runs the game loop on its own thread; the console thread takes `gameMutex` (lock/unlock) around anything that touches the game.
class CMainLoopConsole : public CThread
{
public:
	std::mutex gameMutex;

public:
	void execute(void* pArg)
	{
		while (!quit)
		{
			{
				std::lock_guard<std::mutex> guard(gameMutex);

				// On va updater notre timer
				int nbFrameElapsed = dkcUpdateTimer();

				// On va chercher notre delay
				float delay = dkcGetElapsedf();

				// On passe le nombre de frame a animer
				while (nbFrameElapsed)
				{
					// Update la console
					console->update(delay);

					// On appel nos fonction pour animer ici
					scene->update(delay);

					// On decremente pour le prochain frame
					nbFrameElapsed--;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void lock()
	{
		gameMutex.lock();
	}

	void unlock()
	{
		gameMutex.unlock();
	}
};


int main(int argc, const char* argv[])
{

	// lil print out so that people now know that its working

	printf("***************************************\n");
	printf("*   Babo Violent 2 Dedicated Server   *\n");
	printf("*   Version 2.11d                     *\n");
	printf("*                                     *\n");
	printf("* check the /main/LaunchScript files  *\n");
	printf("* to configure your server            *\n");
	printf("***************************************\n\n\n");

	
	// Example on how to get the mac adress
	//unsigned char mac[8];		// unsigned here is very important
	//bb_getMyMAC( mac );

	//char macString[20];
	//sprintf( macString , "%.2x-%.2x-%.2x-%.2x-%.2x-%.2x" , (int)mac[0], (int)mac[1],(int)mac[2],(int)mac[3],(int)mac[4],(int)mac[5] );
	//printf( " mac addr : %s " , macString );





	// PREMI�E CHOSE �FAIRE, on load les config
	bv2::pathsInit(argv[0]);
	dksvarInit(&stringInterface);
	std::string configPath = bv2::loadConfigLayers(argc, (const char* const*)argv);
	bv2ApplyNetlogFromEnv();
	FetchDBInfos();
	dksvarSaveConfig((char*)configPath.c_str()); // On cre8 le config file aussi

	// On init nos DLL qui vont �re utilis�dans ce jeu
	// On initialise quelque cossin important avant tout
	dkcInit(30); // 30 frame par seconde (m�e que 15 serait le best)

	// On init la network
	if (bb_init() == 1)
	{
		fprintf(stderr, "Error initiating baboNet\n");
		return 0;
	}
	bbNetVersion = bb_getVersion();
	if (CString("%s", bbNetVersion) != "4.0")
	{
		// Error
		bb_peerShutdown();
		bb_shutdown();
		fprintf(stderr, "Wrong version of BaboNet\n");
		return 0;
	}

	// On init la console
	console = new Console();
	console->init();

	//--- On cr�le master
	master = new CMaster();

	// On cr�notre scene
	scene = new Scene();

	// La loop principal
	CMainLoopConsole mainLoopConsole;

	//--- On start la thread
	mainLoopConsole.start();




	//--- Get the arguments and send that to console
//	int argc, const char* argv[]

	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--config") == 0)
		{
			++i; // its file was applied with the other config layers
			continue;
		}
		CString executeCmd = "execute ";
		executeCmd += (char*)(argv[i]);
		console->sendCommand(executeCmd);
		break;
	}


	char input[256];
	while (!quit)
	{
		std::cin.getline(input,256);


		mainLoopConsole.lock();
		if(std::cin.gcount())
			console->sendCommand(input);//CString("Execute CTF"));
		mainLoopConsole.unlock();


		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		//cin.ignore( 10000 , '\n');
		//input[0] = 0;
		//fflush(stdin);
	};

	//printf(" main console loop has quit \n");


//	while (dkwMainLoop());

	// Let the game loop finish its current iteration (it sees `quit` and stops) before the scene goes away
	mainLoopConsole.lock();
	mainLoopConsole.unlock();

	// On efface la scene et ses amis
	delete scene;
	delete master;
	delete console;
	console = 0;
	master = 0;
	scene = 0;

	dksvarSaveConfig((char*)bv2::configFile().c_str());

	// On shutdown le tout (L'ordre est assez important ici)
	bb_peerShutdown();
	bb_shutdown();

	// Tout c'est bien pass� on retourne 0
	return 0;
}

#else



//
// Fonction principal (client): one entry point for Windows, macOS and Linux
//
int main(int argc, char* argv[])
{
	// PREMIERE CHOSE A FAIRE, on load les config
	bv2::pathsInit(argv[0]);
	dksvarInit(&stringInterface);
	std::string configPath = bv2::loadConfigLayers(argc, (const char* const*)argv);
	bv2ApplyNetlogFromEnv();
	FetchDBInfos();
	dksvarSaveConfig((char*)configPath.c_str()); // On cre8 le config file aussi

	// On load tout suite le language utilise par le joueur
	if (!gameVar.isLanguageLoaded())
	{
		dkwShowMessage("Error", "Can not load language file\nTry deleting the config file.");
		return 0;
	}

	if (gameVar.r_bitdepth != 16 && gameVar.r_bitdepth != 32) gameVar.r_bitdepth = 32;

	// On initialise quelque cossin important avant tout
	dkcInit(30); // 30 frame par seconde

	// The window is created at the requested pixel size on every platform (SDL3 has no title-bar offset to compensate)
	if (!dkwInit(gameVar.r_resolution[0], gameVar.r_resolution[1], gameVar.r_bitdepth, gameVar.lang_gameName.s, &mainLoopInterface, gameVar.r_fullScreen, gameVar.r_refreshRate))
	{
		dkwShowMessage("Error", dkwGetLastError());
		return 0;
	}

	// On init les input
	if (!dkiInit())
	{
		dkwShutDown();
		dkwShowMessage("Error", "Error creating Input");
		return 0;
	}

#ifdef BV2_PLATFORM_WINDOWS
	// Set single CPU usage while the GL driver starts (some old drivers misbehave on many cores)
	if(gameVar.cl_affinityMode > 0)
	{
		::SetProcessAffinityMask(::GetCurrentProcess(), 0x1);
	}
#endif

	// On cree notre API openGL
	if (!dkglCreateContext(gameVar.r_bitdepth))
	{
		dkiShutDown();
		dkwShutDown();
		dkwShowMessage("Error", "Error creating openGL context");
		return 0;
	}

#ifdef BV2_PLATFORM_WINDOWS
	// Restore system settings
	if(gameVar.cl_affinityMode == 1)
	{
		DWORD_PTR procMask;
		DWORD_PTR sysMask;
		::GetProcessAffinityMask(::GetCurrentProcess(), &procMask, &sysMask);
		::SetProcessAffinityMask(::GetCurrentProcess(), sysMask);
	}
#endif

	// On init les textures
	dktInit();

	// On init les objets 3D
	dkoInit();

	// On init les particles
	dkpInit();

	// On init le son
	if (!dksInit(gameVar.s_mixRate, gameVar.s_maxSoftwareChannels))
	{
		dkpShutDown();
		dkoShutDown();
		dkiShutDown();
		dkglShutDown();
		dkwShutDown();
		dkwShowMessage("Error", "Error creating the audio engine");
		return 0;
	}

	// On init la network
	if (bb_init() == 1)
	{
		dksShutDown();
		dkpShutDown();
		dkoShutDown();
		dkiShutDown();
		dkglShutDown();
		dkwShutDown();
		dkwShowMessage("Error", "Error initiating baboNet");
		return 0;
	}

	bbNetVersion = bb_getVersion();
	if (CString("%s", bbNetVersion) != "4.0")
	{
		// Error
		bb_peerShutdown();
		bb_shutdown();
		dksShutDown();
		dkpShutDown();
		dkoShutDown();
		dkiShutDown();
		dkglShutDown();
		dkwShutDown();
		dkwShowMessage("Error", "Wrong version of BaboNet\nReinstalling the game may resolve this prolem");
		return 0;
	}

	// On cree le lobby
	lobby = new CLobby();

	// On init la console
	console = new Console();
	console->init();

	// Create the status manager
	status = new CStatus();

	//--- On cree le master
	master = new CMaster();

	// On cree notre scene
	scene = new Scene();

	// check command line options: everything after the program name is one console command
	CString str;
	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "--config") == 0)
		{
			++i; // handled by the config layers
			continue;
		}
		if (str.len() > 0) str += " ";
		str += argv[i];
	}
	if( str.len() > 1 )
	{
		console->sendCommand( str );
	}

	// La loop principal
	dkwMainLoop();

	// On efface la scene
	delete scene;
	scene = 0;

	// Delete master
	delete master;
	master = 0;

	delete console;
	console = 0;

	delete lobby;
	lobby = 0;

	dksvarSaveConfig((char*)bv2::configFile().c_str());

	// On shutdown le tout (L'ordre est assez important ici)
	bb_peerShutdown();
	bb_shutdown();
	dksShutDown();
	dkpShutDown();
	dkoShutDown();
	dkfShutDown();
	dktShutDown();
	dkglShutDown();
	dkiShutDown();
	dkwShutDown();

	// Do last update after window is closed
	delete status;

	// Tout c'est bien passe on retourne 0
	return 0;
}

#endif

