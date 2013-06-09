/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * WarpTV.h :
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
 * From main.c of warp-1.1:
 *
 *      Simple DirectMedia Layer demo
 *      Realtime picture 'gooing'
 *      Released under GPL
 *      by sam lantinga slouken@devolution.com
 */

#ifndef __WARPTV__
#define __WARPTV__

#include "BaseEffecTV.h"

class WarpTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int* offstable;
	int* disttable;
	int* ctable;
	int* sintable;

	virtual void intialize(bool reset);
	virtual int readConfig() { return CONFIG_ERROR; };
	virtual int writeConfig() { return CONFIG_ERROR; };

public:
	WarpTV(void);
	virtual ~WarpTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);

protected:
	void initSinTable(void);
	void initOffsTable(void);
	void initDistTable(void);
	void doWarp(int xw, int yw, int cw, RGB32* src, RGB32* dst);
};

#endif // __WARPTV__
