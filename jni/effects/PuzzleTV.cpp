/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * PuzzleTV.cpp :
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

#include "PuzzleTV.h"

#define  LOG_TAG "PuzzleTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "PuzzleTV";
static const char* EFFECT_TITLE = "Puzzle\nTV";
static const char* FUNC_LIST[]  = {
		"4 x 4",
		"5 x 5",
		"6 x 6",
		"Hint\nOn/Off",
		NULL
};

#define SLIDING_INTERVAL 10
#define AUTOSOLVE_WAIT 300
#define HINT_SIZE 2

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void PuzzleTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	blockSizeW     = 0;
	blockSizeH     = 0;
	blockW         = 0;
	blockH         = 0;
	blockNum       = 0;
	marginW        = 0;
	marginH        = 0;
	phase          = 0;
	movingBlock    = 0;
	spaceBlock     = 0;
	autoSolveTimer = 0;
	blocks         = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			hint = 0;
		}
	} else {
		writeConfig();
	}
}

int PuzzleTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, hint);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int PuzzleTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, hint);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
PuzzleTV::PuzzleTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
PuzzleTV::~PuzzleTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* PuzzleTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* PuzzleTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** PuzzleTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int PuzzleTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	if (setBlockNum(4) != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int PuzzleTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (blocks != NULL) {
		delete[] blocks;
	}

	//
	return super::stop();
}

// Convert
int PuzzleTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	memset(dst_rgb, 0, video_area * PIXEL_SIZE);

	if (autoSolveTimer == 0) {
		autoSolve();
	} else {
		autoSolveTimer--;
	}

	for (int i=0; i<blockNum; i++) {
		if (i == movingBlock || i == spaceBlock) {
			RGB32* q = dst_rgb + blocks[i].destOffset;
			for (int y=0; y<blockSizeH; y++) {
				memset(q, 0, blockSizeW * PIXEL_SIZE);
				q += video_width;
			}
		} else {
			copyBlockImage(src_rgb + blocks[i].srcOffset, dst_rgb + blocks[i].destOffset);
		}
		if (hint) {
			copyHintImage(src_rgb, dst_rgb, blocks[i].destOffset);
		}
	}

	if (movingBlock >= 0) {
		moveBlock(src_rgb, dst_rgb);
	}

	return 0;
}

// Key functions
int PuzzleTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // 4x4
	case 1: // 5x5
	case 2: // 6x6
		setBlockNum(key_code+4);
		break;
	case 3: // Hint On/Off
		hint = 1 - hint;
		break;
	}
	return 0;
}

// Touch action
int PuzzleTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: { // Down
		if (movingBlock >= 0) {
			return -1;
		}
		int bx = x / blockSizeW;
		int by = y / blockSizeH;
		if (bx < 0 || bx >= blockW || by < 0 || by >= blockH) {
			return -1;
		}
		int bn = bx + by * blockW;
		if (bn - blockW == spaceBlock) {
			orderMotion(0); // Down
			autoSolveTimer = AUTOSOLVE_WAIT;
		} else if (bn + blockW == spaceBlock) {
			orderMotion(1); // Up
			autoSolveTimer = AUTOSOLVE_WAIT;
		} else if (bn - 1 == spaceBlock) {
			orderMotion(2); // Left
			autoSolveTimer = AUTOSOLVE_WAIT;
		} else if (bn + 1 == spaceBlock) {
			orderMotion(3); // Right
			autoSolveTimer = AUTOSOLVE_WAIT;
		}
	} break;
	case 1: // Move
		break;
	case 2: // Up
		break;
	}
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int PuzzleTV::setBlockNum(int num)
{
	blockW     = num;
	blockH     = num;
	blockSizeW = video_width  / blockW;
	blockSizeH = video_height / blockH;
	blockNum   = blockW * blockH;
	marginW    = video_width  - blockW * blockSizeW;
	marginH    = video_height - blockH * blockSizeH;

	// Reallocate memory & setup
	if (blocks != NULL) {
		delete[] blocks;
	}
	blocks = new Block[blockNum];
	if (blocks == NULL) {
		return -1;
	}
	for (int y=0; y<blockH; y++) {
		for (int x=0; x<blockW; x++) {
			blocks[y * blockW + x].destOffset =
					blockSizeW * x +
					blockSizeH * y * video_width;
		}
	}
	for (int i=0; i<blockNum; i++) {
		blocks[i].position = i;
	}
	for (int i=0; i<20*blockW; i++) {
		/* the number of shuffling times is a rule of thumb. */
		int a = mUtils->fastrand() % (blockNum - 1);
		int b = mUtils->fastrand() % (blockNum - 1);
		if (a == b) {
			b = (b + 1) % (blockNum - 1);
		}
		int c = blocks[a].position;
		blocks[a].position = blocks[b].position;
		blocks[b].position = c;
	}
	for (int i=0; i<blockNum; i++) {
		blockSetSrcOffset(i);
	}

	//
	phase = 0;
	movingBlock = -1;
	spaceBlock = blockNum - 1;
	autoSolveTimer = AUTOSOLVE_WAIT;

	return 0;
}

//
int PuzzleTV::orderMotion(int dir)
{
	if (movingBlock >= 0) return -1;

	int x = spaceBlock % blockW;
	int y = spaceBlock / blockW;
	int dx, dy;
	switch(dir) {
	case 0:
		dx =  0; dy =  1;
		break;
	case 1:
		dx =  0; dy = -1;
		break;
	case 2:
		dx =  1; dy =  0;
		break;
	case 3:
		dx = -1; dy =  0;
		break;
	default:
		return -1;
	}
	if (x + dx < 0 || x + dx >= blockW) return -1;
	if (y + dy < 0 || y + dy >= blockH) return -1;

	movingBlock = (y + dy) * blockW + x + dx;
	phase = SLIDING_INTERVAL - 1;

	return 0;
}

//
void PuzzleTV::copyBlockImage(RGB32* src, RGB32* dst)
{
	if (hint) {
		src += video_width * HINT_SIZE + HINT_SIZE;
		dst += video_width * HINT_SIZE + HINT_SIZE;
		for (int y=blockSizeH-HINT_SIZE*2; y>0; y--) {
			memcpy(dst, src, (blockSizeW - HINT_SIZE * 2) * PIXEL_SIZE);
			src += video_width;
			dst += video_width;
		}
	} else {
		for (int y=blockSizeH; y>0; y--) {
			memcpy(dst, src, blockSizeW * PIXEL_SIZE);
			src += video_width;
			dst += video_width;
		}
	}
}

void PuzzleTV::copyHintImage(RGB32* src, RGB32* dst, int offset)
{
	int y = blockSizeH;
	int o = blockSizeW - HINT_SIZE;
	src += offset;
	dst += offset;
	for ( ; y>blockSizeH-HINT_SIZE; y--) {
		memcpy(dst, src, blockSizeW * PIXEL_SIZE);
		src += video_width;
		dst += video_width;
	}
	for ( ; y>HINT_SIZE; y--) {
		memcpy(dst,   src,   HINT_SIZE * PIXEL_SIZE);
		memcpy(dst+o, src+o, HINT_SIZE * PIXEL_SIZE);
		src += video_width;
		dst += video_width;
	}
	for ( ; y>0; y--) {
		memcpy(dst, src, blockSizeW * PIXEL_SIZE);
		src += video_width;
		dst += video_width;
	}
}

//
void PuzzleTV::blockSetSrcOffset(int i)
{
	int x = blocks[i].position % blockW;
	int y = blocks[i].position / blockW;

	blocks[i].srcOffset =
			blockSizeW * x +
			blockSizeH * y * video_width;
}

//
void PuzzleTV::moveBlock(RGB32* src, RGB32* dst)
{
	int sx = movingBlock % blockW;
	int sy = movingBlock / blockW;
	int dx = spaceBlock % blockW;
	int dy = spaceBlock / blockW;

	sx *= blockSizeW;
	sy *= blockSizeH;
	dx *= blockSizeW;
	dy *= blockSizeH;

	int x = dx + (sx - dx) * phase / SLIDING_INTERVAL;
	int y = dy + (sy - dy) * phase / SLIDING_INTERVAL;

	copyBlockImage(src + blocks[movingBlock].srcOffset, dst + y * video_width + x);

	if (autoSolveTimer == 0) {
		phase--;
	} else {
		phase-=2;
	}
	if (phase < 0) {
		int tmp;
		/* Exchanges positions of the moving block and the space */
		tmp = blocks[movingBlock].position;
		blocks[movingBlock].position = blocks[spaceBlock].position;
		blocks[spaceBlock].position = tmp;

		blockSetSrcOffset(movingBlock);
		blockSetSrcOffset(spaceBlock);

		spaceBlock = movingBlock;
		movingBlock = -1;
	}
}

//
void PuzzleTV::autoSolve(void)
{
#if 1
	return;
#else
	/* FIXME: IMPORTANT BUG: this functions does *NOT* solve the puzzle! */
	static int lastMove = 0;
	static char dir[4];

	if (movingBlock >= 0) return;
	for (int i=0; i<4; i++) {
		dir[i] = i;
	}
	dir[lastMove] = -1;
	int x = spaceBlock % blockW;
	int y = spaceBlock / blockW;
	if (x <= 0) dir[3] = -1;
	if (x >= blockW - 1) dir[2] = -1;
	if (y <= 0) dir[1] = -1;
	if (y >= blockH - 1) dir[0] = -1;

	int max = 0;
	for (int i=0; i<3; i++) {
		if (dir[i] == -1) {
			for (int j=i+1; j<4; j++) {
				if (dir[j] != -1) {
					dir[i] = dir[j];
					dir[j] = -1;
					max++;
					break;
				}
			}
		} else {
			max++;
		}
	}

	if (max > 0) {
		int i = dir[mUtils->fastrand() % max];
		if (orderMotion(i) == 0) {
			if (i < 2) {
				lastMove = 1 - i;
			} else {
				lastMove = 5 - i;
			}
		}
	}
#endif
}
