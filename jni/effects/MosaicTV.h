/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * MosaicTV.h :
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
 * MosaicTV - censors incoming objects
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#ifndef __MOSAICTV__
#define __MOSAICTV__

#include "BaseEffecTV.h"

class MosaicTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int show_info;
	int threshold;
	int bgIsSet;
	int mosaic_size;
	int mosaic_half;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	MosaicTV(void);
	virtual ~MosaicTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	int setBackground(YUV* src);
};

#endif // __MOSAICTV__
