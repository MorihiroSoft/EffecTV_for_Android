/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BaseEffecTV.h :
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __BASEEFFECTV__
#define __BASEEFFECTV__

//---------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <time.h>
#include <android/log.h>

#include "../Utils.h"

//---------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0
#define VERSION_STRING "0.1.0"

#define CONFIG_SUCCESS  (0)
#define CONFIG_ERROR    (-1)
#define CONFIG_E_FOPEN  (-2)
#define CONFIG_E_FREAD  (-3)
#define CONFIG_E_FWRITE (-4)
#define CONFIG_W_VER    (1)

#define FREAD_1( FP,VAR) (fread( &(VAR), sizeof(VAR), 1, FP) == 1)
#define FWRITE_1(FP,VAR) (fwrite(&(VAR), sizeof(VAR), 1, FP) == 1)

#define FREAD_N( FP,VAR,N) (fread( VAR, sizeof(VAR[0]), N, FP) == N)
#define FWRITE_N(FP,VAR,N) (fwrite(VAR, sizeof(VAR[0]), N, FP) == N)

//---------------------------------------------------------------------
// Effect interface
//---------------------------------------------------------------------
class BaseEffecTV {
protected:
	char   mConfigPath[FILENAME_MAX];
	Utils* mUtils;
	int    video_width;
	int    video_height;
	int    video_area;

	virtual void intialize(bool reset);
	virtual int readConfig() = 0;
	virtual int writeConfig() = 0;

public:
	BaseEffecTV(void);
	virtual ~BaseEffecTV(void);

	/** Set configure path name. */
	virtual void setConfigPath(const char* dir, const char* file);
	/** Return "effect name". (for Settings key) */
	virtual const char* name(void) = 0;
	/** Return "effect title". (for Display) */
	virtual const char* title(void) = 0;
	/** Return "function label list(NULL TERMINATED)". */
	virtual const char** funcs(void) = 0;
	/** Initialize. */
	virtual int start(Utils* utils, int width, int height);
	/** Finalize. */
	virtual int stop(void);
	/** Convert. */
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg) = 0;
	/** Key functions. */
	virtual const char* event(int key_code) = 0;
	/** Touch action. */
	virtual const char* touch(int action, int x, int y) = 0;
};

#endif // __BASEEFFECTV__
