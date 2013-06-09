/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * AgingTV.h :
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
 * AgingTV - film-aging effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#ifndef __AGINGTV__
#define __AGINGTV__

#include "BaseEffecTV.h"

class AgingTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	struct scratch {
		int life;
		int x;
		int dx;
		int init;
	};

	int aging_mode;
	int scratch_lines;
	scratch* scratches;
	int area_scale;
	int pits_interval;
	int dust_interval;

	virtual void intialize(bool reset);
	virtual int readConfig() { return CONFIG_ERROR; };
	virtual int writeConfig() { return CONFIG_ERROR; };

public:
	AgingTV(void);
	virtual ~AgingTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);

protected:
	void coloraging(RGB32* src, RGB32* dst);
	void scratching(RGB32* dst);
	void dusts(RGB32* dst);
	void pits(RGB32* dst);
	void aging_mode_switch(void);
};

#endif // __AGINGTV__
