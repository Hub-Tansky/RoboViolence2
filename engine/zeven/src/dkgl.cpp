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

/* TCE (c) All rights reserved */


#include "dkgli.h"
#include "dkw.h"

#include <SDL3/SDL.h>

int CDkgl::colorDepth=16;
void * CDkgl::context = 0;


void dkglEnableVsync(bool vsync)
{
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

bool CheckExtension( char *m_szSupportedGLExtensions, char* szExtensionName )
{
	unsigned int uiNextExtension;
	char*		 szSupExt= m_szSupportedGLExtensions;
	char*		 cEndExtensions;

	//find the end of the extension list
	cEndExtensions= szSupExt+strlen( m_szSupportedGLExtensions );

	//search through the entire list
	while( szSupExt<cEndExtensions )
	{
		//find the next extension in the list
		uiNextExtension= (unsigned int)(strcspn( szSupExt, " " ));

		//check the extension to the one given in the argument list
		if( ( strlen( szExtensionName )==uiNextExtension ) && 
			( strncmp( szExtensionName, szSupExt, uiNextExtension )==0 ) )
		{
			return true;
		}

		//move to the nexte extension in the list
		szSupExt+= ( uiNextExtension+1 );
	}
	return false;
}


//
// Pour vérifier une extension de la carte
//
bool			 dkglCheckExtension(char * extension)
{
	char * exts = (char*)glGetString(GL_EXTENSIONS);
	return !(strstr(exts, extension) == 0);
}



//
// Pour créer le context openGL (rendering context)
//
int				 dkglCreateContext(int colorDepth)
{
	CDkgl::colorDepth = colorDepth;

	SDL_Window * window = (SDL_Window *)dkwGetWindow();
	if (!window)
		return 0;

	SDL_GLContext ctx = SDL_GL_CreateContext(window);
	if (!ctx)
		return 0;
	CDkgl::context = ctx;
	SDL_GL_MakeCurrent(window, ctx);

	// Entry points for OpenGL 2.1 plus the extensions listed in the glad build
	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
	{
		SDL_GL_DestroyContext(ctx);
		CDkgl::context = 0;
		return 0;
	}
	return 1;
}



//
// Pour afficher le back buffer
//
void			 dkglSwapBuffers()
{
	SDL_Window * window = (SDL_Window *)dkwGetWindow();
	if (window && CDkgl::context)
		SDL_GL_SwapWindow(window);
}



//
// Pour dessiner le system de coordonnée
//
void			 dkglDrawCoordSystem()
{
	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
		glLineWidth(2);
		glDisable(GL_CULL_FACE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glColor3f(1,0,0);
		glBegin(GL_LINES);
			glVertex3f(0,0,0);
			glVertex3f(1,0,0);

			glVertex3f(1,0,0);
			glVertex3f(.9f,.1f,.1f);
			glVertex3f(1,0,0);
			glVertex3f(.9f,-.1f,.1f);
			glVertex3f(1,0,0);
			glVertex3f(.9f,-.1f,-.1f);
			glVertex3f(1,0,0);
			glVertex3f(.9f,.1f,-.1f);
		glEnd();
		glColor3f(0,1,0);
		glBegin(GL_LINES);
			glVertex3f(0,0,0);
			glVertex3f(0,1,0);

			glVertex3f(0,1,0);
			glVertex3f(.1f,.9f,.1f);
			glVertex3f(0,1,0);
			glVertex3f(-.1f,.9f,.1f);
			glVertex3f(0,1,0);
			glVertex3f(-.1f,.9f,-.1f);
			glVertex3f(0,1,0);
			glVertex3f(.1f,.9f,-.1f);
		glEnd();
		glColor3f(0,0,1);
		glBegin(GL_LINES);
			glVertex3f(0,0,0);
			glVertex3f(0,0,1);

			glVertex3f(0,0,1);
			glVertex3f(.1f,.1f,.9f);
			glVertex3f(0,0,1);
			glVertex3f(-.1f,.1f,.9f);
			glVertex3f(0,0,1);
			glVertex3f(-.1f,-.1f,.9f);
			glVertex3f(0,0,1);
			glVertex3f(.1f,-.1f,.9f);
		glEnd();
	glPopAttrib();
}



//
// Draw wire cube
//
void			 dkglDrawWireCube()
{
	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
		glLineWidth(2);
		glDisable(GL_CULL_FACE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glColor3f(1,1,0);
		glBegin(GL_LINE_LOOP);
			glVertex3f(-1,-1,-1);
			glVertex3f(-1,1,-1);
			glVertex3f(1,1,-1);
			glVertex3f(1,-1,-1);
		glEnd();
		glBegin(GL_LINE_LOOP);
			glVertex3f(-1,1,1);
			glVertex3f(-1,-1,1);
			glVertex3f(1,-1,1);
			glVertex3f(1,1,1);
		glEnd();
		glBegin(GL_LINES);
			glVertex3f(-1,1,1);
			glVertex3f(-1,1,-1);
			glVertex3f(-1,-1,1);
			glVertex3f(-1,-1,-1);
			glVertex3f(1,-1,1);
			glVertex3f(1,-1,-1);
			glVertex3f(1,1,1);
			glVertex3f(1,1,-1);
		glEnd();
	glPopAttrib();
}



//
// Pour revenir en vue 3D
//
void			 dkglPopOrtho()
{
	// On pop nos matrice
			glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// On pop nos attribs
	glPopAttrib();
}



//
// Pour setter la vue en 2D
//
void			 dkglPushOrtho(float mWidth, float mHeight)
{
	// On push les attribs pour certaines modifications
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
//	glCullFace(GL_BACK);

	// En mode 2D on ne veux pas de z-buffer
	glDisable(GL_DEPTH_TEST);

	// On push nos matrice et on set la matrice ortho
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
		glLoadIdentity();
		glOrtho(0,mWidth,mHeight,0,-9999,9999);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();
}



//
// Pour setter la fonction de blending
//
void			 dkglSetBlendingFunc(int blending)
{
	switch (blending)
	{
	case DKGL_BLENDING_ADD_SATURATE:
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case DKGL_BLENDING_ADD:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case DKGL_BLENDING_MULTIPLY:
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		break;
	case DKGL_BLENDING_ALPHA:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}
}



//
// Pour rapidement créer une lumière dans votre scene (très basic)
//
void			 dkglSetPointLight(int ID, float x, float y, float z, float r, float g, float b)
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0 + ID);
	float pos[] = {x,y,z,1};
	float amb[] = {r/6,g/6,b/6,1};
	float diff[] = {r,g,b,1};
	float spec[] = {r,g,b,1};

	glLightfv(GL_LIGHT0 + ID, GL_POSITION, pos);
	glLightfv(GL_LIGHT0 + ID, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0 + ID, GL_DIFFUSE, diff);
	glLightfv(GL_LIGHT0 + ID, GL_SPECULAR, spec);
}



//
// On set la vue de la perspective là
//
void			 dkglSetProjection(float mFieldOfView, float mNear, float mFar, float mWidth, float mHeight)
{
	// On met la matrice de projection pour ce créer une vue perspective
	glMatrixMode(GL_PROJECTION);

	// On remet cette matrice à identity
	glLoadIdentity();

	// On ajuste la matrice de projection
	gluPerspective(mFieldOfView, mWidth/mHeight, mNear, mFar);

	// On remet cette de model view (qui est celle de la position et l'orientation)
	glMatrixMode(GL_MODELVIEW);

	// La model view à identity
	glLoadIdentity();
}



//
// Pour bien fermer tout ça
//
void			 dkglShutDown()
{
	if (CDkgl::context)
	{
		SDL_GL_DestroyContext((SDL_GLContext)CDkgl::context);
		CDkgl::context = 0;
	}
}



//
// Pour projeter la mouse ou un point 2D quelconque
//
CVector3f		 dkglUnProject(CVector2i & pos2D, float zRange)
{
	double x,y,z;
	CVector3f pos((float)pos2D[0], (float)pos2D[1], zRange);
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint    viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject (
		pos[0], 
		pos[1], 
		pos[2], 
		modelMatrix, 
		projMatrix, 
		viewport, 
		&x, 
		&y, 
		&z);

	return CVector3f((float)x,(float)y,(float)z);
}


CVector3f		 dkglProject(const CVector3f & pos3D)
{
	double x,y,z;
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint    viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluProject (
		pos3D[0], 
		pos3D[1], 
		pos3D[2], 
		modelMatrix, 
		projMatrix, 
		viewport, 
		&x, 
		&y, 
		&z);
	return CVector3f((float)x,(float)y,(float)z);
}
