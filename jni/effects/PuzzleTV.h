/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * PuzzleTV.h :
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
 * PuzzleTV - separates the image into blocks and scrambles them.
 *            The blocks can be moved interactively.
 *
 * The origin of PuzzleTV is ``Video Puzzle'' by Suutarou in 1993.
 * It runs on Fujitsu FM-TOWNS.
 */

#ifndef __PUZZLETV__
#define __PUZZLETV__

#include "BaseEffecTV.h"

class PuzzleTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	struct Block {
		int position;
		int srcOffset;
		int destOffset;
	};

	int hint;
	int blockSizeW;
	int blockSizeH;
	int blockW;
	int blockH;
	int blockNum;
	int marginW;
	int marginH;
	int phase;
	int movingBlock;
	int spaceBlock;
	int autoSolveTimer;
	struct Block* blocks;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	PuzzleTV(void);
	virtual ~PuzzleTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);

protected:
	int setBlockNum(int numW);
	int orderMotion(int dir);
	void copyBlockImage(RGB32*src, RGB32* dst);
	void copyHintImage(RGB32*src, RGB32* dst, int offset);
	void blockSetSrcOffset(int i);
	void moveBlock(RGB32* src, RGB32* dst);
	void autoSolve(void);
};

#endif // __PUZZLETV__
