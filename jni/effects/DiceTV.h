/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DiceTV.h :
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

/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * dice.c: a 'dicing' effect
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * I suppose this looks similar to PuzzleTV, but it's not. The screen is
 * divided into small squares, each of which is rotated either 0, 90, 180 or
 * 270 degrees.  The amount of rotation for each square is chosen at random.
 *
 * Controls:
 *      c   -   shrink the size of the squares, down to 1x1.
 *      v   -   enlarge the size of the squares, up to 32x32.
 *      space - generate a new random rotation map.
 *
 */

#ifndef __DICETV__
#define __DICETV__

#include "BaseEffecTV.h"

class DiceTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	enum DiceDir {
		Up = 0,
		Right = 1,
		Down = 2,
		Left = 3
	};

	int show_info;
	int g_cube_bits;
	int g_cube_size;
	int g_map_width;
	int g_map_height;
	char* dicemap;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	DiceTV(void);
	virtual ~DiceTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);

protected:
	void diceCreateMap(void);
};

#endif // __DICETV__
