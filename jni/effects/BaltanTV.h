/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BaltanTV.h :
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
 * BaltanTV - like StreakTV, but following for a long time
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#ifndef __BALTANTV__
#define __BALTANTV__

#include "BaseEffecTV.h"

class BaltanTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int plane;
	RGB32* buffer;
	RGB32** planetable;

	virtual void intialize(bool reset);
	virtual int readConfig() { return CONFIG_ERROR; };
	virtual int writeConfig() { return CONFIG_ERROR; };

public:
	BaltanTV(void);
	virtual ~BaltanTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);
};

#endif // __BALTANTV__
