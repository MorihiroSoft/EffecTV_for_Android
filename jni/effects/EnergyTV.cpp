/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * EnergyTV.cpp :
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
 * Lightning Effect
 * Author: sazzzzz's (http://wonderfl.net/c/tcGz)
 */

#include "EnergyTV.h"

#define  LOG_TAG "EnergyTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "EnergyTV";
static const char* EFFECT_TITLE = "Energy\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Mode",
		"Energy\ncolor",
		"Marker\ncolor",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};
static const char* MODE_LIST[] = {
		"Normal",
		"Debug",
};
static const char* MSG_1 = "TOUCH FOR SELECTING MARKER COLOR.";

#define COLORS 32
#define PATTERN 4

#define MAX_MARKER_BOX_NUM 5
#define BLOB_THRESHOLD 30
#define MAX_LINE_THICK 10

#define MAX_THRESHOLD 40
#define MIN_THRESHOLD 1
#define DEF_THRESHOLD 15
#define DLT_THRESHOLD 1

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void EnergyTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	get_buffer = false;
	pause = false;
	vw2 = 0;
	vh2 = 0;
	va2 = 0;
	palettes = NULL;
	palette = NULL;
	blur_buffer = NULL;
	blur_buffer_a = NULL;
	blur_buffer_b = NULL;
	img_buffer = NULL;
	blob_buffer = NULL;
	label_table = NULL;
	marker_box_num = 0;
	marker_box = NULL;
	memset(marker_center, 0, sizeof(marker_center));

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 0;
			color = 0;
			memset(marker_vu, 0, sizeof(marker_vu));
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int EnergyTV::readConfig()
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
			FREAD_1(fp, color) &&
			FREAD_N(fp, marker_vu, 2) &&
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int EnergyTV::writeConfig()
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
			FWRITE_1(fp, color) &&
			FWRITE_N(fp, marker_vu, 2) &&
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
EnergyTV::EnergyTV(void)
: show_info(0)
, mode(0)
, color(0)
, marker_vu()
, threshold(0)
, vw2(0)
, vh2(0)
, va2(0)
, get_buffer(false)
, pause(false)
, palettes(NULL)
, palette(NULL)
, blur_buffer(NULL)
, blur_buffer_a(NULL)
, blur_buffer_b(NULL)
, img_buffer(NULL)
, blob_buffer(NULL)
, label_table(NULL)
, marker_box_num(0)
, marker_box(NULL)
, marker_center()
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
EnergyTV::~EnergyTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* EnergyTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* EnergyTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** EnergyTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int EnergyTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	vw2 = video_width  >> 1;
	vh2 = video_height >> 1;
	va2 = vw2 * vh2;

	// Allocate memory & setup
	palettes    = new RGB32[COLORS * PATTERN];
	marker_box  = new Box[MAX_MARKER_BOX_NUM];
	blur_buffer = new unsigned char[video_area*2];
	img_buffer  = new YUV[video_area*3/2];
	blob_buffer = new int[va2];
	label_table = new int[va2];
	if (palettes == NULL || marker_box == NULL || blur_buffer == NULL || img_buffer == NULL || blob_buffer == NULL || label_table == NULL) {
		return -1;
	}
	memset(blur_buffer, 0, video_area*2);
	memset(img_buffer,  0, video_area*3/2);
	memset(blob_buffer, 0, va2 * sizeof(int));
	memset(label_table, 0, va2 * sizeof(int));

	blur_buffer_a = blur_buffer;
	blur_buffer_b = blur_buffer + video_area;

	makePalette();
	setPalette(color);

	return 0;
}

// Finalize
int EnergyTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (palettes != NULL) {
		delete[] palettes;
	}
	if (marker_box != NULL) {
		delete[] marker_box;
	}
	if (blur_buffer != NULL) {
		delete[] blur_buffer;
	}
	if (img_buffer != NULL) {
		delete[] img_buffer;
	}
	if (blob_buffer != NULL) {
		delete[] blob_buffer;
	}
	if (label_table != NULL) {
		delete[] label_table;
	}

	//
	return super::stop();
}

// Convert
int EnergyTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	if (get_buffer) {
		memcpy(img_buffer, src_yuv, video_area*3/2);
		get_buffer = false;
		pause = true;
	}
	if (pause) {
		mUtils->yuv_YUVtoRGB(img_buffer, dst_rgb);
	} else {
		blob(src_yuv);
		screen(src_yuv, dst_rgb);
		lightning();
		blur();
		blend(dst_rgb);
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s, Threshold: %1d",
				MODE_LIST[mode],
				threshold);
	}

	return 0;
}

// Key functions
const char* EnergyTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	const char* msg = NULL;
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Mode
		mode = (mode + 1) % 2;
		break;
	case 2: // Color pattern
		color = (color + 1) % PATTERN;
		setPalette(color);
		break;
	case 3: // Marker color
		if (pause) {
			pause = false;
			get_buffer = false;
		} else {
			get_buffer = true;
			msg = MSG_1;
		}
		break;
	case 4: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		break;
	case 5: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		break;
	}
	return msg;
}

// Touch action
const char* EnergyTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		if (pause) {
			YUV* p = img_buffer + video_area + ((x>>1) + (y>>1) * vw2) * 2;
			marker_vu[0] = p[0];
			marker_vu[1] = p[1];
			pause = false;
		}
		break;
	case 1: // Move
		break;
	case 2: // Up
		break;
	}
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
void EnergyTV::makePalette(void)
{
	const int KEY_NUM = 8;
	const int KEY_RGB[PATTERN][KEY_NUM] = {{
			// Yellow
			0x000000, 0x7F0000, 0xFF7F00, 0xFF7F00,
			0xFFFF00, 0xFFFF00, 0xFFFFFF, 0xFFFFFF,
	},{
			// Yellow2
			0x000000, 0x3F1F00, 0x9F7F1F, 0xCFAF1F,
			0xEFCF3F, 0xEFCF3F, 0xFFFFFF, 0xFFFFFF,
	},{
			// Blue
			0x000000, 0x00003F, 0x00007F, 0x1F4FFF,
			0x1F4FFF, 0x7FCFFF, 0xFFFFFF, 0xFFFFFF,
	},{
			// Purple
			0x000000, 0x1F007F, 0x4F1F9F, 0x7F00AF,
			0xAF7FCF, 0xAF7FCF, 0xFFFFFF, 0xFFFFFF,
	}};

	for (int k=0; k<PATTERN; k++) {
		RGB32* p = &palettes[k*COLORS];
		for (int i=0,j=0,s=0,e=0; i<COLORS; i++) {
			if (i >= e) {
				j++;
				s = e;
				e = j * COLORS / (KEY_NUM-1);
			}
			unsigned char r = (
					RED(  KEY_RGB[k][j-1]) * (e - i) +
					RED(  KEY_RGB[k][j  ]) * (i - s)) / (e - s);
			unsigned char g = (
					GREEN(KEY_RGB[k][j-1]) * (e - i) +
					GREEN(KEY_RGB[k][j  ]) * (i - s)) / (e - s);
			unsigned char b = (
					BLUE( KEY_RGB[k][j-1]) * (e - i) +
					BLUE( KEY_RGB[k][j  ]) * (i - s)) / (e - s);
			p[i] = RGB(r, g, b);
		}

		for (int i=0; i<COLORS; i++) {
			p[i] &= 0xfefeff;
		}
	}
}

void EnergyTV::setPalette(int val)
{
	palette = &palettes[COLORS * val];
}

void EnergyTV::blob(YUV* src_yuv)
{
	// Binarization
	{
		int t2 = threshold * threshold;
		YUV* vu = src_yuv + video_area + (1 + vw2) * 2;
		int* bb = blob_buffer;
		memset(bb, 0xFF, (1+vw2)*sizeof(int)); // "-1"
		bb += 1 + vw2;
		for (int y=vh2-2; y>0; y--) {
			for (int x=vw2-2; x>0; x--) {
				const int dv = (int)(*vu++) - (int)marker_vu[0];
				const int du = (int)(*vu++) - (int)marker_vu[1];
				*bb++ = (dv*dv<t2 && du*du<t2 ? 0 : -1);
			}
			vu += 2 * 2;
			*bb++ = -1;
			*bb++ = -1;
		}
		memset(bb, 0xFF, (vw2-1)*sizeof(int)); // "-1"
	}

	// Labeling
	int label = 0;
	{
		// Step.1 Connect
		int* bb = blob_buffer + 1 + vw2;
		for (int y=vh2-2; y>0; y--) {
			for (int x=vw2-2; x>0; x--) {
				if (*bb >= 0) {
					const int* bX = bb - 1;
					const int* bY = bb - vw2;
					if (*bX >= 0) {
						int xi=*bX, xl=label_table[xi];
						for ( ; xl>=0; xi=xl,xl=label_table[xi]);
						*bb = xi;
						if (*bY >= 0) {
							for (int yi=*bY,yl=label_table[yi]; yi!=xi; yi=yl,yl=label_table[yi]) {
								label_table[yi] = xi;
								if (yl < 0) {
									break;
								}
							}
						}
					} else if (*bY >= 0) {
						int yi=*bY, yl=label_table[yi];
						for ( ; yl>=0; yi=yl,yl=label_table[yi]);
						*bb = yi;
					} else {
						label_table[label] = -1;
						*bb = label++;
					}
				}
				bb++;
			}
			bb += 2;
		}

		// Step.2 Strip label_table
		for (int k=0; k<label; k++) {
			if (label_table[k] >= 0) {
				int i, j;
				for (i=k,j=label_table[i]; j>=0; i=j,j=label_table[i]);
				int l = i;
				for (i=k,j=label_table[i]; j>=0; i=j,j=label_table[i]) {
					label_table[i] = l;
				}
			}
		}

		// Step.3 Update label
		bb = blob_buffer;
		for (int i=va2; i>0; i--) {
			if (*bb >= 0) {
				if (label_table[*bb] >= 0) {
					*bb = label_table[*bb];
				}
			}
			bb++;
		}

		// Step.4 Count
		memset(label_table, 0, label * sizeof(int));
		bb = blob_buffer;
		for (int i=va2; i>0; i--) {
			if (*bb >= 0) {
				label_table[*bb]++;
			}
			bb++;
		}
	}

	// Boxing
	{
		// Step.1 Sort
		marker_box_num = 0;
		for (int i=0; i<label; i++) {
			int entry_i = i;
			int entry_v = label_table[i];
			if (entry_v >= BLOB_THRESHOLD) {
				for (int j=0; j<marker_box_num; j++) {
					if (entry_v > marker_box[j].sort.val) {
						int tmp_i = marker_box[j].sort.idx;
						int tmp_v = marker_box[j].sort.val;
						marker_box[j].sort.idx = entry_i;
						marker_box[j].sort.val = entry_v;
						entry_i = tmp_i;
						entry_v = tmp_v;
					}
				}
				if (marker_box_num < MAX_MARKER_BOX_NUM) {
					marker_box[marker_box_num].sort.idx = entry_i;
					marker_box[marker_box_num].sort.val = entry_v;
					marker_box_num++;
				}
			}
		}
		if (marker_box_num < 1) {
			if (mode != 0) {
				// Debug mode
				memset(blob_buffer, 0, va2 * sizeof(int));
			}
			return;
		}

		// Step.2 Calculate box size(1/2scale)
		float w = 0.0f;
		for (int i=0; i<marker_box_num; i++) {
			w += (float)marker_box[i].sort.val;
		}
		memset(label_table, 0, label * sizeof(int));
		for (int i=0; i<marker_box_num; i++) {
			marker_box[i].draw.weight = (float)marker_box[i].sort.val / w;

			label_table[marker_box[i].sort.idx] = i + 1;
			marker_box[i].draw.l = marker_box[i].draw.t = INT_MAX;
			marker_box[i].draw.r = marker_box[i].draw.b = -1;
		}
		int* bb = blob_buffer;
		for (int y=0; y<vh2; y++) {
			for (int x=0; x<vw2; x++) {
				const int i = *bb;
				if (i >= 0) {
					const int j = label_table[i] - 1;
					if (j >= 0) {
						if (marker_box[j].draw.l > x) {
							marker_box[j].draw.l = x;
						}
						if (marker_box[j].draw.t > y) {
							marker_box[j].draw.t = y;
						}
						if (marker_box[j].draw.r < x) {
							marker_box[j].draw.r = x;
						}
						if (marker_box[j].draw.b < y) {
							marker_box[j].draw.b = y;
						}
					}
					*bb = j + 1;
				} else {
					*bb = 0;
				}
				bb++;
			}
		}

		// Step.3 Calculate box size(1/1scale) & center coordinate
		marker_center[0] = 0;
		marker_center[1] = 0;
		for (int i=0; i<marker_box_num; i++) {
			marker_box[i].draw.l <<= 1;
			marker_box[i].draw.t <<= 1;
			marker_box[i].draw.r <<= 1;
			marker_box[i].draw.b <<= 1;
			marker_center[0] += (int)((marker_box[i].draw.l + marker_box[i].draw.r) / 2
					* marker_box[i].draw.weight);
			marker_center[1] += (int)((marker_box[i].draw.t + marker_box[i].draw.b) / 2
					* marker_box[i].draw.weight);
		}
	}
}

void EnergyTV::screen(YUV* src_yuv, RGB32* dst_rgb)
{
	switch (mode) {
	case 0: {
		// Normal mode
		mUtils->yuv_YUVtoRGB(src_yuv, dst_rgb);
	} break;
	case 1: {
		// Debug mode
		memcpy(img_buffer, src_yuv, video_area);
		memset(img_buffer+video_area, 0x7F, video_area/2);
		YUV* vu = img_buffer + video_area;
		int* bb = blob_buffer;
		for (int i=va2; i>0; i--) {
			if (*bb > 0) {
				vu[0] = 0xFF;
				vu[1] = 0xFF;
			}
			vu += 2;
			bb++;
		}
		mUtils->yuv_YUVtoRGB(img_buffer, dst_rgb);
	} break;
	}
}

void EnergyTV::lightning()
{
	const double r60 = 60 * M_PI / 180.0;
	const float  c60 = (float)cos(r60);
	const float  s60 = (float)sin(r60);

	for (int i=0; i<marker_box_num; i++) {
		// Skip?
		if (mUtils->fastrandf() >= marker_box[i].draw.weight * marker_box_num) {
			continue;
		}

		float r;
		r = normalrandf();
		const int px = marker_box[i].draw.l * r + marker_box[i].draw.r * (1.0f - r);
		r = normalrandf();
		const int py = marker_box[i].draw.t * r + marker_box[i].draw.b * (1.0f - r);

		const int thick = 1 + mUtils->fastrand() % (int)(1 + MAX_LINE_THICK * marker_box[i].draw.weight);

		double rad = 2.0 * M_PI * mUtils->fastrandf();
		float vx = (float)cos(rad);
		float vy = (float)sin(rad);

		for (float x=marker_center[0],y=marker_center[1],dg=1; dg>=1; x+=vx,y+=vy) {
			// Resize -> Current vector
			float dd = mUtils->fastrand() % 10 + 1;
			float dv = sqrtf(vx * vx + vy * vy);
			vx = (dv>0 ? dd*vx/dv : 0);
			vy = (dv>0 ? dd*vy/dv : dd);

			// Goal vector
			float gx = px - x;
			float gy = py - y;
			dg = sqrtf(gx * gx + gy * gy);

			if (dg < 1.0) {
				// Goal
				vx = gx;
				vy = gy;
			} else {
				// Resize -> Goal vector
				gx = dd * gx / dg;
				gy = dd * gy / dg;

				// Current vector -> Turn right
				float vx_r =   vx * c60 - vy * s60;
				float vy_r =   vx * s60 + vy * c60;
				// Current vector -> Turn left
				float vx_l =   vx * c60 + vy * s60;
				float vy_l = - vx * s60 + vy * c60;

				if (vx * gx + vy * gy <= vx * vx_r + vy * vy_r) {
					vx = gx;
					vy = gy;
				} else if (gx * vx_r + gy * vy_r <= gx * vx_l + gy * vy_l) {
					vx = vx_r;
					vy = vy_r;
				} else {
					vx = vx_l;
					vy = vy_l;
				}
			}

			// Draw line
			line(x, y, x+vx, y+vy, thick);
		}
	}
}

float EnergyTV::normalrandf()
{
	// http://ja.wikipedia.org/wiki/%E4%B9%B1%E6%95%B0%E5%88%97
#if 0
	return (mUtils->fastrandf() + mUtils->fastrandf() + mUtils->fastrandf() +
			mUtils->fastrandf() + mUtils->fastrandf() + mUtils->fastrandf() +
			mUtils->fastrandf() + mUtils->fastrandf() + mUtils->fastrandf() +
			mUtils->fastrandf() + mUtils->fastrandf() + mUtils->fastrandf()) / 12.0f;
#else
	return (mUtils->fastrandf() + mUtils->fastrandf() + mUtils->fastrandf() +
			mUtils->fastrandf() + mUtils->fastrandf() + mUtils->fastrandf()) / 6.0f;
#endif
}

void EnergyTV::line(const float x0, const float y0, const float x1, const float y1, const int thick)
{
	const float d = thick;
	const float d2 = d * d;

	int sx, ex;
	if (x0 <= x1) {
		sx = x0 - d - 1;
		ex = x1 + d + 1;
	} else {
		sx = x1 - d - 1;
		ex = x0 + d + 1;
	}
	if (sx <  1          ) sx = 1;
	if (ex >= video_width) ex = video_width - 1;

	int sy, ey;
	if (y0 <= y1) {
		sy = y0 - d - 1;
		ey = y1 + d + 1;
	} else {
		sy = y1 - d - 1;
		ey = y0 + d + 1;
	}
	if (sy <  1           ) sy = 1;
	if (ey >= video_height) ey = video_height - 1;

	unsigned char* q = blur_buffer_a + sy * video_width;
	for (int y=sy; y<ey; y++) {
		for (int x=sx; x<ex; x++) {
			const float v2 = calLineDist(x, y, x0, y0, x1, y1);
			if (v2 <= d2) {
				const int i = (int)((COLORS - 1) * (d2 - v2) / d2);
				if (q[x] < i) {
					q[x] = i;
				}
			}
		}
		q += video_width;
	}
}

float EnergyTV::calLineDist(const float x, const float y, const float x0, const float y0, const float x1, const float y1) {
	const float vx  = x1 - x0;
	const float vy  = y1 - y0;
	const float m0x = x  - x0;
	const float m0y = y  - y0;
	const float m1x = x  - x1;
	const float m1y = y  - y1;

	if (vx * m0x + vy * m0y <= 0) {
		return m0x * m0x + m0y * m0y;
	} else if (vx * m1x + vy * m1y >= 0) {
		return m1x * m1x + m1y * m1y;
	} else {
		const float d = vx * m0y - vy * m0x;
		return d * d / (vx * vx + vy * vy);
	}
}

void EnergyTV::blur(void)
{
	unsigned char* p = blur_buffer_a + 1 + video_width;
	unsigned char* q = blur_buffer_b + 1 + video_width;

	for (int y=video_height-2; y>0; y--) {
		for (int x=video_width-2; x>0; x--) {
			unsigned char v = *(p-1) + *(p+1) + *(p-video_width) + *(p+video_width);
			if (v > 3) v -= 3;
			*q++ = v >> 2;
			p++;
		}
		p += 2;
		q += 2;
	}

	// swap
	p             = blur_buffer_a;
	blur_buffer_a = blur_buffer_b;
	blur_buffer_b = p;
}

void EnergyTV::blend(RGB32* dst_rgb)
{
	unsigned char* p = blur_buffer_a + 1 + video_width;
	RGB32*         q = dst_rgb       + 1 + video_width;

	for (int y=video_height-2; y>0; y--) {
		for (int x=video_width-2; x>0; x--) {
			RGB32 a = *q & 0xfefeff;
			RGB32 b = palette[*p++];
			a += b;
			b = a & 0x1010100;
			*q++ = a | (b - (b >> 8));
		}
		p += 2;
		q += 2;
	}
}
