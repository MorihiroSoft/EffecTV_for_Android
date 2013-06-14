/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * WarpTV.cpp :
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

#include "WarpTV.h"

#define  LOG_TAG "WarpTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

//static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "WarpTV";
static const char* EFFECT_TITLE = "WarpTV";
static const char* FUNC_LIST[]  = {
		NULL
};

#define MaxColor 120
#define Decay 15

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void WarpTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	offstable = NULL;
	disttable = NULL;
	ctable    = NULL;
	sintable  = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
		}
	} else {
		writeConfig();
	}
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
WarpTV::WarpTV(void)
: offstable(NULL)
, disttable(NULL)
, ctable(NULL)
, sintable(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
WarpTV::~WarpTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* WarpTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* WarpTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** WarpTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int WarpTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	offstable = new int[video_height];
	disttable = new int[video_area];
	ctable    = new int[1024];
	sintable  = new int[1024+256];
	if (offstable == NULL || disttable == NULL || ctable == NULL || sintable == NULL) {
		return -1;
	}

	initSinTable();
	initOffsTable();
	initDistTable();

	return 0;
}

// Finalize
int WarpTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (offstable != NULL) {
		delete[] offstable;
	}
	if (disttable != NULL) {
		delete[] disttable;
	}
	if (ctable != NULL) {
		delete[] ctable;
	}
	if (sintable != NULL) {
		delete[] sintable;
	}

	//
	return super::stop();
}

// Convert
int WarpTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	static int tval = 0;
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	int xw  = (int)(sin((tval+100)*M_PI/128) *  30);
	int yw  = (int)(sin((tval    )*M_PI/256) * -35);
	int cw  = (int)(sin((tval- 70)*M_PI/ 64) *  50);

	xw += (int)(sin((tval- 10)*M_PI/512) *  40);
	yw += (int)(sin((tval+ 30)*M_PI/512) *  40);

	doWarp(xw, yw, cw, src_rgb, dst_rgb);
	tval = (tval + 1) & 511;

	return 0;
}

// Key functions
int WarpTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	return 0;
}

// Touch action
int WarpTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}


//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void WarpTV::initSinTable(void)
{
	int* tsinptr = sintable;
	int* tptr    = sintable;

	for (int i=0; i<1024; i++) {
		*tptr++ = (int)(sin(i*M_PI/512) * 32767);
	}

	for (int i=0; i<256; i++) {
		*tptr++ = *tsinptr++;
	}
}

//
void WarpTV::initOffsTable(void)
{
	for (int y=0; y<video_height; y++) {
		offstable[y] = y * video_width;
	}
}

//
void WarpTV::initDistTable(void)
{
	int halfw = video_width>> 1;
	int halfh = video_height >> 1;

	int* distptr = disttable;

	double m = sqrt((double)(halfw * halfw + halfh * halfh));

	for (int y=-halfh; y<halfh; y++) {
		for (int x=-halfw; x<halfw; x++) {
			*distptr++ = ((int)((sqrt(x*x+y*y) * 511.9999) / m)) << 1;
		}
	}
}

//
void WarpTV::doWarp(int xw, int yw, int cw, RGB32* src, RGB32* dst)
{
	const int vw = video_width;
	const int vh = video_height;
	int*   ctptr   = ctable;
	int*   distptr = disttable;
	RGB32* dstptr  = dst;
	int    skip    = 0 ; /* video_width*sizeof(RGB32)/4 - video_width;; */

	int c = 0;
	for (int x=0; x<512; x++) {
		int i = (c >> 3) & 0x3FE;
		*ctptr++ = ((sintable[i] * yw) >> 15);
		*ctptr++ = ((sintable[i+256] * xw) >> 15);
		c += cw;
	}

	int maxx = vw - 2;
	int maxy = vh - 2;
	for (int y=0; y<vh-1; y++) {
		for (int x=0; x<vw; x++) {
			int i = *distptr++;
			int dx = ctable [i+1] + x;
			int dy = ctable [i] + y;


			if (dx < 0) dx = 0;
			else if (dx > maxx) dx = maxx;

			if (dy < 0) dy = 0;
			else if (dy > maxy) dy = maxy;
			*dstptr++ = src[offstable[dy] + dx];
		}
		dstptr += skip;
	}
	memset(dstptr, 0, vw * PIXEL_SIZE);
}
