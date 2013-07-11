/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * RadioacTV.cpp :
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
 * RadioacTV - motion-enlightment effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * I referred to "DUNE!" by QuoVadis for this effect.
 */

#include "RadioacTV.h"

#define  LOG_TAG "RadioacTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "RadioacTV";
static const char* EFFECT_TITLE = "Radioac\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Normal",
		"Strobe1",
		"Strobe2",
		"Manual",
		"Red",
		"Green",
		"Blue",
		"White",
		"Interval\n+",
		"Interval\n-",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Normal",
		"Strobe1",
		"Strobe2",
		"Manual (Touch screen to effect)",
};

#define COLORS 32
#define PATTERN 4
#define RATIO 0.95

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 40
#define DLT_THRESHOLD 5

#define VIDEO_HWIDTH  (buf_width/2)
#define VIDEO_HHEIGHT (buf_height/2)

#define DELTA (255/(COLORS/2-1))

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void RadioacTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	snapTime     = 0;
	snapInterval = 3;
	palette      = NULL;
	palettes     = NULL;
	snapframe    = NULL;

	buf_width_blocks = 0;
	buf_width        = 0;
	buf_height       = 0;
	buf_area         = 0;
	buf_margin_right = 0;
	buf_margin_left  = 0;
	blurzoombuf      = NULL;
	blurzoomx        = NULL;
	blurzoomy        = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 0;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int RadioacTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, show_info) &&
			FREAD_1(fp, mode) &&
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int RadioacTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, show_info) &&
			FWRITE_1(fp, mode) &&
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
RadioacTV::RadioacTV(void)
: show_info(0)
, mode(0)
, threshold(0)
, snapTime(0)
, snapInterval(0)
, palette(NULL)
, palettes(NULL)
, snapframe(NULL)
, buf_width_blocks(0)
, buf_width(0)
, buf_height(0)
, buf_area(0)
, buf_margin_right(0)
, buf_margin_left(0)
, blurzoombuf(NULL)
, blurzoomx(NULL)
, blurzoomy(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
RadioacTV::~RadioacTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* RadioacTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* RadioacTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** RadioacTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int RadioacTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	buf_width_blocks = (video_width / 32);
	if (buf_width_blocks > 255) {
		return -1;
	}
	buf_width = buf_width_blocks * 32;
	buf_height = video_height;
	buf_area = buf_width * buf_height;
	buf_margin_left = (video_width - buf_width)/2;
	buf_margin_right = video_width - buf_width - buf_margin_left;

	// Allocate memory & setup
	blurzoombuf = new unsigned char[buf_area * 2];
	blurzoomx   = new int[buf_width];
	blurzoomy   = new int[buf_height];
	snapframe   = new RGB32[video_area];
	if (blurzoombuf == NULL || blurzoomx == NULL || blurzoomy == NULL || snapframe == NULL) {
		return -1;
	}
	memset(blurzoombuf, 0, buf_area * 2);

	if (setTable() != 0) {
		return -1;
	}

	if (makePalette() != 0) {
		return -1;
	}
	setPalette(2);  // Blue

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int RadioacTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (blurzoombuf != NULL) {
		delete[] blurzoombuf;
	}
	if (blurzoomx != NULL) {
		delete[] blurzoomx;
	}
	if (blurzoomy != NULL) {
		delete[] blurzoomy;
	}
	if (palettes != NULL) {
		delete[] palettes;
	}
	if (snapframe != NULL) {
		delete[] snapframe;
	}

	//
	return super::stop();
}

// Convert
int RadioacTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	unsigned char* p = blurzoombuf;
	RGB32*         q = dst_rgb;

	if (mode != 2 || snapTime <= 0) {
		unsigned char* diff = mUtils->image_bgsubtract_update_yuv_y(src_yuv);
		if (mode == 0 || snapTime <= 0) {
			diff += buf_margin_left;
			for (int y=0; y<buf_height; y++) {
				for (int x=0; x<buf_width; x++) {
					p[x] |= diff[x] >> 3;
				}
				diff += video_width;
				p += buf_width;
			}
			if (mode == 1 || mode == 2) {
				memcpy(snapframe, src_rgb, video_area * PIXEL_SIZE);
			}
			p = blurzoombuf;
		}
	}
	blurzoomcore();

	if (mode == 1 || mode == 2) {
		src_rgb = snapframe;
	}
	for (int y=0; y<video_height; y++) {
		for (int x=0; x<buf_margin_left; x++) {
			*q++ = *src_rgb++;
		}
		for (int x=0; x<buf_width; x++) {
			RGB32 a = *src_rgb++ & 0xfefeff;
			RGB32 b = palette[*p++];
			a += b;
			b = a & 0x1010100;
			*q++ = a | (b - (b >> 8));
		}
		for (int x=0; x<buf_margin_right; x++) {
			*q++ = *src_rgb++;
		}
	}

	if (mode == 1 || mode == 2) {
		snapTime--;
		if (snapTime < 0) {
			snapTime = snapInterval;
		}
	}

	if (show_info) {
		if (mode == 1 || mode == 2) {
			sprintf(dst_msg, "Mode: %s, Interval: %1d, Threshold: %1d",
					MODE_LIST[mode],
					snapInterval,
					threshold);
		} else {
			sprintf(dst_msg, "Mode: %s, Threshold: %1d",
					MODE_LIST[mode],
					threshold);
		}
	}

	return 0;
}

// Key functions
const char* RadioacTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Normal
	case 2: // Strobe1
	case 3: // Strobe2
	case 4: // Manual
		mode = key_code - 1;
		if (mode == 3) {
			snapTime = 1;
		} else {
			snapTime = 0;
		}
		break;
	case 5: // Red
	case 6: // Green
	case 7: // Blue
	case 8: // White
		setPalette(key_code - 5);
		break;
	case 9: // Interval +
		if (mode == 1 || mode == 2) {
			snapInterval++;
		}
		break;
	case 10: // Interval -
		if (mode == 1 || mode == 2) {
			snapInterval--;
			if (snapInterval < 1) {
				snapInterval = 1;
			}
		}
		break;
	case 11: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 12: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	}
	return NULL;
}

// Touch action
const char* RadioacTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		if (mode == 3) {
			snapTime = 0;
		}
		break;
	case 1: // Move
		break;
	case 2: // Up -> Space
		if (mode == 3) {
			snapTime = 1;
		}
		break;
	}
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int RadioacTV::makePalette(void)
{
	palettes = new RGB32[COLORS * PATTERN];
	if (palettes == NULL) {
		return -1;
	}

	RGB32* R = &palettes[COLORS*0];
	RGB32* G = &palettes[COLORS*1];
	RGB32* B = &palettes[COLORS*2];
	RGB32* W = &palettes[COLORS*3];

	for (int i=0; i<COLORS/2; i++) {
		*R++ = (i*DELTA)<<16;
		*G++ = (i*DELTA)<<8;
		*B++ = i*DELTA;
	}
	for (int i=0; i<COLORS/2; i++) {
		*R++ = (255<<16) | (i*DELTA)<< 8 |  i*DELTA;
		*G++ = (255<< 8) | (i*DELTA)<<16 |  i*DELTA;
		*B++ =  255      | (i*DELTA)<<16 | (i*DELTA)<<8;
	}
	for (int i=0; i<COLORS; i++) {
		*W++ = (255*i/COLORS) * 0x10101;
	}

	for (int i=0; i<COLORS*PATTERN; i++) {
		palettes[i] &= 0xfefeff;
	}

	return 0;
}

//
void RadioacTV::setPalette(int color)
{
	palette = &palettes[COLORS * color];
}

/* this table assumes that video_width is times of 32 */
int RadioacTV::setTable(void)
{
	int prevptr = (int)(0.5+RATIO*(-VIDEO_HWIDTH)+VIDEO_HWIDTH);
	for (int xb=0; xb<(buf_width_blocks); xb++) {
		unsigned int bits = 0;
		for (int x=0; x<32; x++) {
			int ptr = (int)(0.5+RATIO*(xb*32+x-VIDEO_HWIDTH)+VIDEO_HWIDTH);
#ifdef USE_NASM
			bits = bits<<1;
			if(ptr != prevptr)
				bits |= 1;
#else
			bits = bits>>1;
			if(ptr != prevptr)
				bits |= 0x80000000;
#endif /* USE_NASM */
			prevptr = ptr;
		}
		blurzoomx[xb] = bits;
	}

	int ty = (int)(0.5+RATIO*(-VIDEO_HHEIGHT)+VIDEO_HHEIGHT);
	int tx = (int)(0.5+RATIO*(-VIDEO_HWIDTH)+VIDEO_HWIDTH);
	int xx = (int)(0.5+RATIO*(buf_width-1-VIDEO_HWIDTH)+VIDEO_HWIDTH);
	blurzoomy[0] = ty * buf_width + tx;
	prevptr = ty * buf_width + xx;
	for (int y=1; y<buf_height; y++) {
		ty = (int)(0.5+RATIO*(y-VIDEO_HHEIGHT)+VIDEO_HHEIGHT);
		blurzoomy[y] = ty * buf_width + tx - prevptr;
		prevptr = ty * buf_width + xx;
	}

	return 0;
}

/* following code is a replacement of blurzoomcore.nas. */
void RadioacTV::blur(void)
{
	const int bw = buf_width;
	unsigned char* p = blurzoombuf + bw + 1;
	unsigned char* q = p + buf_area;

	for (int y=buf_height-2; y>0; y--) {
		for (int x=bw-2; x>0; x--) {
			unsigned char v = (*(p-bw) + *(p-1) + *(p+1) + *(p+bw))/4 - 1;
			if (v == 255) v = 0;
			*q = v;
			p++;
			q++;
		}
		p += 2;
		q += 2;
	}
}

//
void RadioacTV::zoom(void)
{
	const int bh  = buf_height;
	const int bwb = buf_width_blocks;
	unsigned char* p = blurzoombuf + buf_area;
	unsigned char* q = blurzoombuf;

	for (int y=0; y<bh; y++) {
		p += blurzoomy[y];
		for (int b=0; b<bwb; b++) {
			int dx = blurzoomx[b];
			for (int x=0; x<32; x++) {
				p += (dx & 1);
				*q++ = *p;
				dx = dx>>1;
			}
		}
	}
}

//
void RadioacTV::blurzoomcore(void)
{
	blur();
	zoom();
}

