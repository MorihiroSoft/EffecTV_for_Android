/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BlueScreenTV.cpp :
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
 * BlueScreenTV - blue sceen effect: changes scene background
 * Copyright (C) 2005-2006 Nicolas Argyrou
 *
 * s: take 4-frames background snapshot
 * d: take delayed background snapshot after 3 seconds
 * space: get 4-frames blue screen background and preset tolerance to 30
 * b: get 24-frames blue screen background and preset tolerance to 20
 * c: decrease tolerance by 1 (0-255)
 * v: increase tolerance by 1 (0-255)
 *
 */

/*
 * Developper's notes:
 * The above filter computes color difference between the current frame
 * the pre-defined bluesceen background, and replaces differences less than
 * the threshold level by another background image.
 * Most webcams do not have a very clean image and the threshold is not enough
 * to avoid noise, so the bluescreen is recorded for 4 or 24 frames and the
 * minimum and maximum colors are saved (this method is better than averaging).
 * To avoid noise the replacement background is averaged over 4 frames. It
 * can be taken after a 3 seconds delay to be able to shoot the screen with
 * a webcam.
 * The color difference algorithm is quite different from the other algorithms
 * included in the effectv package. It uses a max(diff(rgv)) formulae with
 * anitaliasing like high quality photo editors do. Moreover it uses it twice
 * for the maximum and minimum blue screen.
 * To have even less noise a fast blur routine blurs the current frame
 * so that noisy lonely pixels are diluted. This blurring routine may be
 * overriden at compilation time by commenting out the "#define USE_BLUR" line.
 * The "#define PROFILING" line in the source code may be uncommented to try
 * other algorithm optimisations, although a lot has been done to allow a
 * maximum speed on 32bits+ processors.
 */

#include "BlueScreenTV.h"

#define  LOG_TAG "BlueScreenTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 1
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "BlueScreenTV";
static const char* EFFECT_TITLE = "Blue\nScreen\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Get BG\n(Now)",
		"Get BG\n(Now+3s)",
		"Get\nBlue",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};

#define MAX_THRESHOLD 127
#define MIN_THRESHOLD 0
#define DEF_THRESHOLD 30
#define DLT_THRESHOLD 1

#define SNAPSHOT_DELAY  3
#define BLUESCREEN_FRAMES 4
#define USE_BLUR

#define BG_CNT_START 6

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void BlueScreenTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgSetCnt       = BG_CNT_START;
	bgSetCntTime   = 0;
	bcSetCnt       = BLUESCREEN_FRAMES;
	bgimage        = NULL;
	bgimageTmp     = NULL;
	bluescreen_min = NULL;
	bluescreen_max = NULL;

	// set default parameters. (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			threshold = DEF_THRESHOLD;
		}
		threshold2 = threshold << 1; /* pre-computation */
	} else {
		writeConfig();
	}
}

int BlueScreenTV::readConfig()
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
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int BlueScreenTV::writeConfig()
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
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
BlueScreenTV::BlueScreenTV(void)
: show_info(0)
, threshold(0)
, threshold2(0)
, bgSetCnt(0)
, bgSetCntTime(0)
, bcSetCnt(0)
, bgimage(NULL)
, bgimageTmp(NULL)
, bluescreen_min(NULL)
, bluescreen_max(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
BlueScreenTV::~BlueScreenTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* BlueScreenTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* BlueScreenTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** BlueScreenTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int BlueScreenTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	bgimage        = new RGB32[video_area];
	bgimageTmp     = new RGB32[video_area];
	bluescreen_min = new RGB32[video_area];
	bluescreen_max = new RGB32[video_area];
	if (bgimage == NULL || bgimageTmp == NULL || bluescreen_min == NULL || bluescreen_max == NULL) {
		return -1;
	}
	memset(bluescreen_min, 0xFF, video_area * PIXEL_SIZE);
	memset(bluescreen_max, 0,    video_area * PIXEL_SIZE);

	return 0;
}

// Finalize
int BlueScreenTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (bgimage != NULL) {
		delete[] bgimage;
	}
	if (bgimageTmp != NULL) {
		delete[] bgimageTmp;
	}
	if (bluescreen_min != NULL) {
		delete[] bluescreen_min;
	}
	if (bluescreen_max != NULL) {
		delete[] bluescreen_max;
	}

	//
	return super::stop();
}

// Convert
int BlueScreenTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);
	RGB32* bg     = bgimage;
	RGB32* bs_min = bluescreen_min;
	RGB32* bs_max = bluescreen_max;
#ifdef USE_BLUR
	unsigned char rold = 0;
	unsigned char gold = 0;
	unsigned char bold = 0;
#endif

	if (bgSetCnt > 0) {
		if (bgSetCntTime != 0) {
			time_t now = time(NULL);
			if (now < bgSetCntTime) {
				memcpy(dst_rgb, src_rgb, video_area * PIXEL_SIZE);
				return 0;
			}
			bgSetCntTime = 0;
		}
		if (setBackground(src_rgb) != 0) {
			memcpy(dst_rgb, src_rgb, video_area * PIXEL_SIZE);
			return 0;
		}
		memset(dst_rgb, 0, video_area * PIXEL_SIZE);
	}

	if (bcSetCnt > 0) {
		if (setBlueScreen(src_rgb) != 0) {
			return 0;
		}
	}

	for (int k=0; k<video_area; k++) {
		/* retreive source */
#ifdef USE_BLUR
		unsigned char r0 = *src_rgb >> 16;
		unsigned char g0 = *src_rgb >>  8;
		unsigned char b0 = *src_rgb;
		/* blur */
		unsigned char r = (rold >> 1) + (r0 >> 1);
		unsigned char g = (gold >> 1) + (g0 >> 1);
		unsigned char b = (bold >> 1) + (b0 >> 1);
#else
		unsigned char r = *src_rgb >> 16;
		unsigned char g = *src_rgb >>  8;
		unsigned char b = *src_rgb;
#endif

		/* use max and min bluescreen to avoid noise */
		unsigned char rmin = *bs_min >> 16;
		unsigned char gmin = *bs_min >>  8;
		unsigned char bmin = *bs_min;
		unsigned char rmax = *bs_max >> 16;
		unsigned char gmax = *bs_max >>  8;
		unsigned char bmax = *bs_max;

		/* max(dr,dg,db)) method (optimized for 32bits+ processors) */
		int d = (r<rmin ? rmin-r : (r>rmax ? r-rmax : 0));
		unsigned char dgmin = (g<gmin ? gmin-g : (g>gmax ? g-gmax : 0));
		if (d < dgmin) d = dgmin;
		unsigned char dbmin = (b<bmin ? bmin-b : (b>bmax ? b-bmax : 0));
		if (d < dbmin) d = dbmin;

		/* with antialiasing */
		if (d * 2 > threshold * 3) {
			*dst_rgb = *src_rgb;
		} else if (d * 2 < threshold) {
			*dst_rgb = *bg;
		} else {
			unsigned char rbg = *bg >> 16;
			unsigned char gbg = *bg >>  8;
			unsigned char bbg = *bg;
			/* oh yeah, that's it */
			int m1 = (d<<1) - threshold;
			int m2 = m1 - threshold2;
			float k1 = (float)m1 / (float)threshold2;
			float k2 = (float)m2 / (float)threshold2;
#ifdef USE_BLUR
			*dst_rgb =
					(((int)(r0 * k1 - rbg * k2)) << 16) |
					(((int)(g0 * k1 - gbg * k2)) <<  8) |
					(((int)(b0 * k1 - bbg * k2))      );
#else
			*dst_rgb =
					(((int)(r * k1 - rbg * k2)) << 16) |
					(((int)(g * k1 - gbg * k2)) <<  8) |
					(((int)(b * k1 - bbg * k2))      );
#endif
		}

#ifdef USE_BLUR
		rold = r0;
		gold = g0;
		bold = b0;
#endif

		src_rgb++;
		dst_rgb++;
		bg++;
		bs_min++;
		bs_max++;
	}

	if (show_info) {
		sprintf(dst_msg, "Threshold: %1d",
				threshold);
	}

	return 0;
}

// Key functions
int BlueScreenTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Get BG (Now)
		bgSetCnt = BG_CNT_START;
		bgSetCntTime = 0;
		break;
	case 2: // Get BG (Now+3s)
		bgSetCnt = BG_CNT_START;
		bgSetCntTime = time(NULL) + 3;
		break;
	case 3: // Get Blue
		bcSetCnt = BLUESCREEN_FRAMES;
		memset(bluescreen_min, 0xFF, video_area * PIXEL_SIZE);
		memset(bluescreen_max, 0x00, video_area * PIXEL_SIZE);
		break;
	case 4: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		threshold2 = threshold << 1;
		break;
	case 5: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		threshold2 = threshold << 1;
		break;
	}
	return 0;
}

// Touch action
int BlueScreenTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int BlueScreenTV::setBackground(RGB32* src)
{
	int i;

	/*
	 * grabs 4 frames and composites them to get a quality background image
	 * (original code by FUKUCHI Kentaro, debugged by Nicolas Argyrou)
	 */
	switch(bgSetCnt) {
	case 6: // BG_CNT_START
		/* step 1: grab frame-1 to buffer-1 */
		memcpy(bgimage, src, video_area * PIXEL_SIZE);
		bgSetCnt--;
		return 1;
	case 5:
		/* step 2: add frame-2 to buffer-1 */
		for (i=0; i<video_area; i++) {
			bgimage[i] = (src[i]&bgimage[i]) + (((src[i]^bgimage[i])&0xfefefe)>>1);
		}
		bgSetCnt--;
		return 1;
	case 4:
		/* step 3: grab frame-3 to buffer-2 */
		memcpy(bgimageTmp, src, video_area * PIXEL_SIZE);
		bgSetCnt--;
		return 1;
	case 3:
		/* step 4: add frame-4 to buffer-2 */
		for (i=0; i<video_area; i++) {
			bgimageTmp[i] = (src[i]&bgimageTmp[i]) + (((src[i]^bgimageTmp[i])&0xfefefe)>>1);
		}
		bgSetCnt--;
		return 1;
	case 2:
		/* step 5: add buffer-3 to buffer-1 */
		for (i=0; i<video_area; i++) {
			bgimage[i] = ((bgimage[i]&bgimageTmp[i]) + (((bgimage[i]^bgimageTmp[i])&0xfefefe)>>1))&0xfefefe;
		}
		mUtils->image_bgset_y(bgimage);
		bgSetCnt--;
		return 0;
	case 1:
		// for double buffer
		bgSetCnt--;
		return 0;
	}
	return 0;
}

//
int BlueScreenTV::setBlueScreen(RGB32* src)
{
	int i;

	/* grabs frames, keep min and max to get a bluescreen image */
	if (bcSetCnt > 0) {
		for (i=0; i<video_area; i++) {
			if (RED(bluescreen_min[i]) > RED(src[i])) {
				bluescreen_min[i] = (bluescreen_min[i] & 0x00FFFF) | (src[i] & 0xFF0000);
			}
			if (GREEN(bluescreen_min[i]) > GREEN(src[i])) {
				bluescreen_min[i] = (bluescreen_min[i] & 0xFF00FF) | (src[i] & 0x00FF00);
			}
			if (BLUE(bluescreen_min[i]) > BLUE(src[i])) {
				bluescreen_min[i] = (bluescreen_min[i] & 0xFFFF00) | (src[i] & 0x0000FF);
			}
			if (RED(bluescreen_max[i]) < RED(src[i])) {
				bluescreen_max[i] = (bluescreen_max[i] & 0x00FFFF) | (src[i] & 0xFF0000);
			}
			if (GREEN(bluescreen_max[i]) < GREEN(src[i])) {
				bluescreen_max[i] = (bluescreen_max[i] & 0xFF00FF) | (src[i] & 0x00FF00);
			}
			if (BLUE(bluescreen_max[i]) < BLUE(src[i])) {
				bluescreen_max[i] = (bluescreen_max[i] & 0xFFFF00) | (src[i] & 0x0000FF);
			}
		}
		bcSetCnt--;
		return 1;
	}

	return 0;
}
