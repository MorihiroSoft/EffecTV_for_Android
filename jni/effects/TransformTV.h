/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * TransformTV.h :
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
 * Plugin for EffecTV by Fukuchi Kentarou
 * Written by clifford smith <nullset@dookie.net>
 *
 * TransForm.c: Performs positinal translations on images
 *
 * Space selects b/w the different transforms
 *
 * basicaly TransformList contains an array of
 * values indicating where pixels go.
 * This value could be stored here or generated. The ones i use
 * here are generated.
 * Special value: -1 = black, -2 = get values from mapFromT(x,y,t)
 * ToDo: support multiple functions ( -3 to -10 or something?)
 * Note: the functions CAN return -1 to mean black....
 *
 */

#ifndef __TRANSFORMTV__
#define __TRANSFORMTV__

#include "BaseEffecTV.h"

class TransformTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int show_info;
	int mode;
	int** TableList;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	TransformTV(void);
	virtual ~TransformTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);

protected:
	int mapFromT(int x,int y, int t);
	void SquareTableInit(void);
};

#endif // __TRANSFORMTV__
