/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * NervousHalf.h :
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
 * NervousHalf - Or your bitter half.
 * Copyright (C) 2002 TANNENBAUM Edo
 * Copyright (C) 2004 Kentaro Fukuchi
 *
 * 2004/11/27
 *  The most of this code has been taken from Edo's NervousTV.
 */

#ifndef __NERVOUSHALF__
#define __NERVOUSHALF__

#include "BaseEffecTV.h"

class NervousHalf : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int show_info;
	int mode;
	int dir;
	int mirror;
	int delay;
	int plane;
	int scratchTimer;
	int scratchStride;
	int scratchCurrent;
	RGB32* buffer;
	RGB32** planetable;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	NervousHalf(void);
	virtual ~NervousHalf(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	int nextDelay(void);
	int nextNervous(void);
	int nextScratch(void);
	void left(RGB32* src, RGB32* buf, RGB32* dst, int mirror);
	void right(RGB32* src, RGB32* buf, RGB32* dst, int mirror);
	void upper(RGB32* src, RGB32* buf, RGB32* dst, int mirror);
	void bottom(RGB32* src, RGB32* buf, RGB32* dst, int mirror);
};

#endif // __NERVOUSHALF__
