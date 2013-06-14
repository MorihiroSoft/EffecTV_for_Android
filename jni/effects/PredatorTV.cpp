/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * PredatorTV.cpp :
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
 * PredatorTV - makes incoming objects invisible like the Predator.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "PredatorTV.h"

#define  LOG_TAG "PredatorTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "PredatorTV";
static const char* EFFECT_TITLE = "Predator\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};

#define BG_CNT_START 6

#define MAX_THRESHOLD 235
#define MIN_THRESHOLD 15 //16
#define DEF_THRESHOLD 40
#define DLT_THRESHOLD 5

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void PredatorTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	bgSetCnt    = BG_CNT_START;
	bgimage     = NULL;
	bgimageY    = NULL;
	bgimageYTmp = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int PredatorTV::readConfig()
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

int PredatorTV::writeConfig()
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
PredatorTV::PredatorTV(void)
: show_info(0)
, threshold(0)
, bgSetCnt(0)
, bgimage(NULL)
, bgimageY(NULL)
, bgimageYTmp(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
PredatorTV::~PredatorTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* PredatorTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* PredatorTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** PredatorTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int PredatorTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	bgimage     = new RGB32[video_area];
	bgimageY    = new YUV[video_area];
	bgimageYTmp = new YUV[video_area];
	if (bgimage == NULL || bgimageY == NULL || bgimageYTmp == NULL) {
		return -1;
	}

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int PredatorTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (bgimage != NULL) {
		delete[] bgimage;
	}
	if (bgimageY != NULL) {
		delete[] bgimageY;
	}
	if (bgimageYTmp != NULL) {
		delete[] bgimageYTmp;
	}

	//
	return super::stop();
}

// Convert
int PredatorTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (bgSetCnt > 0) {
		if (setBackground(src_yuv, src_rgb) != 0) {
			memcpy(dst_rgb, src_rgb, video_area * PIXEL_SIZE);
			return 0;
		}
		memset(dst_rgb, 0, video_area * PIXEL_SIZE);
	}

	unsigned char* diff = mUtils->image_diff_filter(mUtils->image_bgsubtract_yuv_y(src_yuv));

	RGB32* q = dst_rgb;
	RGB32* r = bgimage;
	diff += video_width;
	for (int y=0; y<video_height-1; y++) {
		for (int x=0; x<video_width-4; x++) {
			if (*diff) {
				*q = *(r+4) & 0xfffcfcfc;
			} else {
				*q = *r;
			}
			q++;
			r++;
			diff++;
		}
		q    += 4;
		r    += 4;
		diff += 4;
	}

	if (show_info) {
		sprintf(dst_msg, "Threshold: %1d (Touch screen to update BG)",
				threshold);
	}

	return 0;
}

// Key functions
int PredatorTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 2: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	}
	return 0;
}

// Touch action
int PredatorTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		bgSetCnt = BG_CNT_START;
		break;
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
int PredatorTV::setBackground(YUV* src_yuv, RGB32* src_rgb)
{
	/*
	 * grabs 4 frames and composites them to get a quality background image
	 */
	switch(bgSetCnt) {
	case 6: // BG_CNT_START
		/* step 1: grab frame-1 to buffer-1 */
		memcpy(bgimageY, src_yuv, video_area);
		bgSetCnt--;
		return 1;
	case 5:
		/* step 2: add frame-2 to buffer-1 */
		for (int i=0; i<video_area; i++) {
			bgimageY[i] = (src_yuv[i]&bgimageY[i]) + (((src_yuv[i]^bgimageY[i])&0xfe)>>1);
		}
		bgSetCnt--;
		return 1;
	case 4:
		/* step 3: grab frame-3 to buffer-2 */
		memcpy(bgimageYTmp, src_yuv, video_area);
		bgSetCnt--;
		return 1;
	case 3:
		/* step 4: add frame-4 to buffer-2 */
		for (int i=0; i<video_area; i++) {
			bgimageYTmp[i] = (src_yuv[i]&bgimageYTmp[i]) + (((src_yuv[i]^bgimageYTmp[i])&0xfe)>>1);
		}
		bgSetCnt--;
		return 1;
	case 2:
		/* step 5: add buffer-3 to buffer-1 */
		for (int i=0; i<video_area; i++) {
			bgimageY[i] = ((bgimageY[i]&bgimageYTmp[i]) + (((bgimageY[i]^bgimageYTmp[i])&0xfe)>>1))&0xfe;
		}
		mUtils->image_bgset_yuv_y(bgimageY);
		bgSetCnt--;
		return 0;
	case 1:
		memcpy(bgimage, src_rgb, video_area * PIXEL_SIZE);
		bgSetCnt--;
		return 0;
	}
	return 0;
}
