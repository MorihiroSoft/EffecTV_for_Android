/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * Utils.cpp: utilities
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
 * utils.c: utilities
 * image.c: utilities for image processing.
 * yuv.c: YUV(YCbCr) color system utilities
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <android/log.h>

#include "Utils.h"

#define  LOG_TAG "Utils"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

// R = .... ...R RRRR RRR.
// G = .... ..GG GGGG GG..
// B = .... .... BBBB BBBB
#define RGBtoBG(R,G,B,P) \
		R = (*P & 0x00FF0000) >> (16-1); \
		G = (*P & 0x0000FF00) >> ( 8-2); \
		B = (*P & 0x000000FF);

//---------------------------------------------------------------------
// UTILS: PUBLIC METHODS
//---------------------------------------------------------------------
// Constructor
Utils::Utils(int width, int height)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	int ret;

	ret = image_init(width, height);
	if (ret != 0) {
		LOGE("%s(L=%d): image_init()=%d", __func__, __LINE__, ret);
		return;
	}

	ret = yuv_init();
	if (ret != 0) {
		LOGE("%s(L=%d): yuv_init()=%d", __func__, __LINE__, ret);
		return;
	}
}

// Destructor
Utils::~Utils()
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	int ret;

	ret = image_end();
	if (ret != 0) {
		LOGE("%s(L=%d): image_end()=%d", __func__, __LINE__, ret);
		return;
	}

	ret = yuv_end();
	if (ret != 0) {
		LOGE("%s(L=%d): yuv_end()=%d", __func__, __LINE__, ret);
		return;
	}
}

//
void Utils::HSItoRGB(double H, double S, double I, int *r, int *g, int *b)
{
	double T,Rv,Gv,Bv;

	Rv=1+S*sin(H-2*M_PI/3);
	Gv=1+S*sin(H);
	Bv=1+S*sin(H+2*M_PI/3);
	T=255.999*I/2;
	*r=trunc(Rv*T);
	*g=trunc(Gv*T);
	*b=trunc(Bv*T);
}

//
void Utils::fastsrand(unsigned int seed)
{
	fastrand_val = seed;
}

//
float Utils::fastrandf() {
	return (float)(fastrand()&0xFFFFFF) / (float)0xFFFFFF;
}

//---------------------------------------------------------------------
// IMAGE: PUBLIC METHODS
//---------------------------------------------------------------------
/* Initializer is called from utils_init(). */
int Utils::image_init(int width, int height)
{
	stretching_buffer = NULL;
	background = NULL;
	diff = NULL;
	diff2 = NULL;

	video_width  = width;
	video_height = height;
	video_area   = width * height;

	stretching_buffer = new RGB32[video_area];
	background = new RGB32[video_area];
	diff = new unsigned char[video_area];
	diff2 = new unsigned char[video_area];
	if (stretching_buffer == NULL || background == NULL || diff == NULL || diff2 == NULL) {
		return -1;
	}
	memset(diff2, 0, video_area * sizeof(unsigned char));
	return 0;
}

//
int Utils::image_end(void)
{
	if (stretching_buffer != NULL) {
		delete[] stretching_buffer;
		stretching_buffer = NULL;
	}
	if (background != NULL) {
		delete[] background;
		background = NULL;
	}
	if (diff != NULL) {
		delete[] diff;
		diff = NULL;
	}
	if (diff2 != NULL) {
		delete[] diff2;
		diff2 = NULL;
	}
	return 0;
}

//
void Utils::image_stretching_buffer_clear(RGB32 color)
{
	int i;
	RGB32* p;

	p = stretching_buffer;
	for(i=0; i<video_area; i++) {
		*p++ = color;
	}
}

//
void Utils::image_stretch(RGB32* src, int src_width, int src_height,
		RGB32* dst, int dst_width, int dst_height)
{
	int x, y;
	int sx, sy;
	int tx, ty;
	RGB32* p;

	tx = src_width * 65536 / dst_width;
	ty = src_height * 65536 / dst_height;
	sy = 0;
	for (y=0; y<dst_height; y++) {
		p = src + (sy>>16) * src_width;
		sx = 0;
		for (x=0; x<dst_width; x++) {
			*dst++ = p[(sx>>16)];
			sx += tx;
		}
		sy += ty;
	}
}

#if 0
//
void Utils::image_stretch_to_screen(void)
{
	if (screen_scale == 2) {
		image_stretch_to_screen_double();
	} else {
		image_stretch(stretching_buffer, video_width, video_height,
				(RGB32 *)screen_getaddress(), screen_width, screen_height);
	}
}
#endif

/*
 * Collection of background subtraction functions
 */

/* checks only fake-Y value */
/* In these function Y value is treated as R*2+G*4+B. */

void Utils::image_set_threshold_y(int threshold)
{
	y_threshold = threshold * 7; /* fake-Y value is timed by 7 */
}

//
void Utils::image_bgset_y(RGB32* src)
{
	int i;
	int R, G, B;
	RGB32* p;
	short* q;

	p = src;
	q = (short*)background;
	for (i=0; i<video_area; i++) {
		RGBtoBG(R,G,B,p);
		*q = (short)(R + G + B);
		p++;
		q++;
	}
}

//
void Utils::image_bgget_y(RGB32* dst)
{
	int i;
	int R, G, B;
	short* p;
	RGB32* q;

	p = (short*)background;
	q = dst;
	for (i=0; i<video_area; i++) {
		R = ((*p & 0xFC00) >> 10) << 2;
		G = ((*p & 0x03E0) >>  5) << 3;
		B = ((*p & 0x001F)      ) << 3;
		*q = (R<<16) | (G<<8) | B;
		p++;
		q++;
	}
}

//
unsigned char* Utils::image_bgsubtract_y(RGB32* src)
{
	int i;
	int R, G, B;
	RGB32* p;
	short* q;
	unsigned char* r;
	int v;

	p = src;
	q = (short*)background;
	r = diff;
	for (i=0; i<video_area; i++) {
		RGBtoBG(R,G,B,p);
		v = (R + G + B) - (int)(*q);
		*r = ((y_threshold + v)>>24) | ((y_threshold - v)>>24);

		p++;
		q++;
		r++;
	}

	return diff;
	/* The origin of subtraction function is;
	 * diff(src, dst) = (abs(src - dst) > threshold) ? 0xff : 0;
	 *
	 * This functions is transformed to;
	 * (threshold > (src - dst) > -threshold) ? 0 : 0xff;
	 *
	 * (v + threshold)>>24 is 0xff when v is less than -threshold.
	 * (v - threshold)>>24 is 0xff when v is less than threshold.
	 * So, ((v + threshold)>>24) | ((threshold - v)>>24) will become 0xff when
	 * abs(src - dst) > threshold.
	 */
}

/* Background image is refreshed every frame */
unsigned char* Utils::image_bgsubtract_update_y(RGB32* src)
{
	int i;
	int R, G, B;
	RGB32* p;
	short* q;
	unsigned char* r;
	int v;

	p = src;
	q = (short*)background;
	r = diff;
	for (i=0; i<video_area; i++) {
		RGBtoBG(R,G,B,p);
		v = (R + G + B) - (int)(*q);
		*q = (short)(R + G + B);
		*r = ((y_threshold + v)>>24) | ((y_threshold - v)>>24);

		p++;
		q++;
		r++;
	}

	return diff;
}

/* Y value filters */
unsigned char* Utils::image_y_over(RGB32* src)
{
	int i;
	int R, G, B, v;
	unsigned char* p = diff;

	for (i = video_area; i>0; i--) {
		RGBtoBG(R,G,B,src);
		v = y_threshold - (R + G + B);
		*p = (unsigned char)(v>>24);
		src++;
		p++;
	}

	return diff;
}

//
unsigned char* Utils::image_y_under(RGB32* src)
{
	int i;
	int R, G, B, v;
	unsigned char* p = diff;

	for (i = video_area; i>0; i--) {
		RGBtoBG(R,G,B,src);
		v = (R + G + B) - y_threshold;
		*p = (unsigned char)(v>>24);
		src++;
		p++;
	}

	return diff;
}

//
void Utils::image_set_threshold_yuv_y(int threshold)
{
	y_threshold = threshold;
}

//
void Utils::image_bgset_yuv_y(YUV* src)
{
	memcpy(background, src, video_area * sizeof(YUV));
}

//
unsigned char* Utils::image_bgsubtract_yuv_y(YUV* src)
{
	int i;
	YUV*           p = src;
	YUV*           q = (YUV*)background;
	unsigned char* r = diff;
	int v;

	for (i=video_area; i>0; i--) {
		v = (int)(*p) - (int)(*q);
		*r = ((y_threshold + v)>>24) | ((y_threshold - v)>>24);
		p++;
		q++;
		r++;
	}

	return diff;
}

//
unsigned char* Utils::image_bgsubtract_update_yuv_y(YUV* src)
{
	int i;
	YUV*           p = src;
	YUV*           q = (YUV*)background;
	unsigned char* r = diff;
	int v;

	for (i=video_area; i>0; i--) {
		v = (int)(*p) - (int)(*q);
		*r = ((y_threshold + v)>>24) | ((y_threshold - v)>>24);
		*q = *p;
		p++;
		q++;
		r++;
	}

	return diff;
}

//
unsigned char* Utils::image_yuv_y_over(YUV* src)
{
	int i;
	YUV*           p = src;
	unsigned char* r = diff;
	int v;

	for (i=video_area; i>0; i--) {
		v = y_threshold - *p;
		*r = (unsigned char)(v>>24);
		p++;
		r++;
	}

	return diff;
}

//
unsigned char* Utils::image_yuv_y_under(YUV* src)
{
	int i;
	YUV*           p = src;
	unsigned char* r = diff;
	int v;

	for (i=video_area; i>0; i--) {
		v = *p - y_threshold;
		*r = (unsigned char)(v>>24);
		p++;
		r++;
	}

	return diff;
}

/* The range of r, g, b are [0..7] */
void Utils::image_set_threshold_RGB(int r, int g, int b)
{
	unsigned char R, G, B;

	R = G = B = 0xff;
	R = R<<r;
	G = G<<g;
	B = B<<b;
	rgb_threshold = (RGB32)(R<<16 | G<<8 | B);
}

//
void Utils::image_bgset_RGB(RGB32* src)
{
	int i;
	RGB32* p;

	p = background;
	for (i=0; i<video_area; i++) {
		*p++ = (*src++) & 0xfefefe;
	}
}

//
unsigned char* Utils::image_bgsubtract_RGB(RGB32* src)
{
	int i;
	RGB32* p;
	RGB32* q;
	unsigned a, b;
	unsigned char* r;

	p = src;
	q = background;
	r = diff;
	for (i=0; i<video_area; i++) {
		a = (*p++)|0x1010100;
		b = *q++;
		a = a - b;
		b = a & 0x1010100;
		b = b - (b>>8);
		b = b ^ 0xffffff;
		a = a ^ b;
		a = a & rgb_threshold;
		*r++ = (0 - a)>>24;
	}
	return diff;
}

//
unsigned char* Utils::image_bgsubtract_update_RGB(RGB32* src)
{
	int i;
	RGB32* p;
	RGB32* q;
	unsigned a, b;
	unsigned char* r;

	p = src;
	q = background;
	r = diff;
	for (i=0; i<video_area; i++) {
		a = *p|0x1010100;
		b = *q&0xfefefe;
		*q++ = *p++;
		a = a - b;
		b = a & 0x1010100;
		b = b - (b>>8);
		b = b ^ 0xffffff;
		a = a ^ b;
		a = a & rgb_threshold;
		*r++ = (0 - a)>>24;
	}
	return diff;
}

/* noise filter for subtracted image. */
unsigned char* Utils::image_diff_filter(unsigned char* diff)
{
	int x, y;
	unsigned char* src;
	unsigned char* dst;
	unsigned int count;
	unsigned int sum1, sum2, sum3;
	const int width = video_width;

	src = diff;
	dst = diff2 + width +1;
	for (y=1; y<video_height-1; y++) {
		sum1 = src[0] + src[width] + src[width*2];
		sum2 = src[1] + src[width+1] + src[width*2+1];
		src += 2;
		for (x=1; x<width-1; x++) {
			sum3 = src[0] + src[width] + src[width*2];
			count = sum1 + sum2 + sum3;
			sum1 = sum2;
			sum2 = sum3;
			*dst++ = (0xff*3 - count)>>24;
			src++;
		}
		dst += 2;
	}

	return diff2;
}

/* tiny edge detection */
unsigned char* Utils::image_edge(RGB32* src)
{
	int x, y;
	unsigned char* p;
	unsigned char* q;
	int r, g, b;
	int ar, ag, ab;
	int w;

	p = (unsigned char*)src;
	q = diff2;
	w = video_width * sizeof(RGB32);

	for (y=0; y<video_height - 1; y++) {
		for (x=0; x<video_width - 1; x++) {
			b = p[0];
			g = p[1];
			r = p[2];
			ab = abs(b - p[4]);
			ag = abs(g - p[5]);
			ar = abs(r - p[6]);
			ab += abs(b - p[w]);
			ag += abs(g - p[w+1]);
			ar += abs(r - p[w+2]);
			b = ab+ag+ar;
			if(b > y_threshold) {
				*q = 255;
			} else {
				*q = 0;
			}
			q++;
			p += 4;
		}
		p += 4;
		*q++ = 0;
	}
	memset(q, 0, video_width);

	return diff2;
}

/* horizontal flipping */
void Utils::image_hflip(RGB32* src, RGB32* dst, int width, int height)
{
	int x, y;

	src += width - 1;
	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {
			*dst++ = *src--;
		}
		src += width * 2;
	}
}

//---------------------------------------------------------------------
// IMAGE: PRIVATE METHODS
//---------------------------------------------------------------------
#if 0
//
void Utils::image_stretch_to_screen_double(void)
{
	int x, y;
	RGB32* src;
	RGB32* dst1;
	RGB32* dst2;
	int width = video_width;
	int height = video_height;
	int swidth = screen_width;

	src = stretching_buffer;
	dst1 = (RGB32*)screen_getaddress();
	dst2 = dst1 + swidth;

	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {
			dst1[x*2] = *src;
			dst1[x*2+1] = *src;
			dst2[x*2] = *src;
			dst2[x*2+1] = *src;
			src++;
		}
		dst1 += swidth*2;
		dst2 += swidth*2;
	}
}
#endif

//---------------------------------------------------------------------
// YUV: PUBLIC METHODS
//---------------------------------------------------------------------
//
int Utils::yuv_init(void)
{
	int i;

	for (i=0; i<256; i++) {
		YtoRGB[i] =  1.164*(i-16);
		VtoR[i] =  1.596*(i-128);
		VtoG[i] = -0.813*(i-128);
		UtoG[i] = -0.391*(i-128);
		UtoB[i] =  2.018*(i-128);
		RtoY[i] =  0.257*i;
		RtoU[i] = -0.148*i;
		RtoV[i] =  0.439*i;
		GtoY[i] =  0.504*i;
		GtoU[i] = -0.291*i;
		GtoV[i] = -0.368*i;
		BtoY[i] =  0.098*i;
		BtoV[i] = -0.071*i;
	}

	tmp_rgb32 = new RGB32[video_area];
	if (tmp_rgb32 == NULL) {
		return -1;
	}

	tmp_yuv = new YUV[video_area * 3 / 2];
	if (tmp_yuv == NULL) {
		return -1;
	}

	return 0;
}

//
int Utils::yuv_end(void)
{
	if (tmp_rgb32 != NULL) {
		delete[] tmp_rgb32;
		tmp_rgb32 = NULL;
	}
	if (tmp_yuv != NULL) {
		delete[] tmp_yuv;
		tmp_yuv = NULL;
	}
	return 0;
}

//
unsigned char Utils::yuv_RGBtoY(int rgb)
{
	int i =
			RtoY[(rgb>>16)&0xff] +
			GtoY[(rgb>> 8)&0xff] +
			BtoY[(rgb    )&0xff] +
			16;
	return i;
}

//
unsigned char Utils::yuv_RGBtoU(int rgb)
{
	int i =
			RtoU[(rgb>>16)&0xff] +
			GtoU[(rgb>> 8)&0xff] +
			RtoV[(rgb    )&0xff] + /* BtoU == RtoV */
			128;
	return i;
}

//
unsigned char Utils::yuv_RGBtoV(int rgb)
{
	int i =
			RtoV[(rgb>>16)&0xff] +
			GtoV[(rgb>> 8)&0xff] +
			BtoV[(rgb    )&0xff] +
			128;
	return i;
}

// YUV(NV21)→ARGB(8888)
RGB32* Utils::yuv_YUVtoRGB(YUV* yuv, RGB32* rgb32)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	if (rgb32 == NULL) {
		rgb32 = tmp_rgb32;
	}
	const unsigned int vw  = video_width;
	const unsigned int vh  = video_height;
	const unsigned int vw2 = vw >> 1;
	const unsigned int vh2 = vh >> 1;
	const unsigned int va  = video_area;
	YUV*   yP1     = &yuv[0];
	YUV*   yP2     = &yuv[vw];
	YUV*   vuP     = &yuv[va];
	RGB32* rgb32P1 = &rgb32[0];
	RGB32* rgb32P2 = &rgb32[vw];

	for (unsigned int i=0; i<vh2; i++) {
		for (unsigned int j=0; j<vw2; j++) {
			int y1 = ((*(yP1++))&0xFF) - 16;
			if (y1 < 0) y1 = 0;
			int y2 = ((*(yP1++))&0xFF) - 16;
			if (y2 < 0) y2 = 0;
			int y3 = ((*(yP2++))&0xFF) - 16;
			if (y3 < 0) y3 = 0;
			int y4 = ((*(yP2++))&0xFF) - 16;
			if (y4 < 0) y4 = 0;

			int v = ((*(vuP++))&0xFF) - 128;
			int u = ((*(vuP++))&0xFF) - 128;

			*(rgb32P1++) = yuv_YUVtoRGB_1(y1, u, v);
			*(rgb32P1++) = yuv_YUVtoRGB_1(y2, u, v);
			*(rgb32P2++) = yuv_YUVtoRGB_1(y3, u, v);
			*(rgb32P2++) = yuv_YUVtoRGB_1(y4, u, v);
		}
		yP1 = yP2;
		yP2 += vw;
		rgb32P1 = rgb32P2;
		rgb32P2 += vw;
	}

	return rgb32;
}

// ARGB(8888)→YUV(*)
YUV* Utils::yuv_RGBtoYUV(RGB32* rgb32, int yuv_fmt, YUV* yuv)
{
	LOGI("%s(L=%d): yuv_fmt=%d", __func__, __LINE__, yuv_fmt);
	if (yuv == NULL) {
		yuv = tmp_yuv;
	}
	switch(yuv_fmt) {
	case 1: // Encoder.ColorFormat_NV12 (java)
		return yuv_RGBtoNV12(rgb32, yuv);
	case 2: // Encoder.ColorFormat_NV21 (java)
		return yuv_RGBtoNV21(rgb32, yuv);
	case 3: // Encoder.ColorFormat_I420 (java)
		return yuv_RGBtoI420(rgb32, yuv);
	}
	return NULL;
}

//---------------------------------------------------------------------
// YUV: PRIVATE METHODS
//---------------------------------------------------------------------
// YUV(NV21)→ARGB(8888)
RGB32 Utils::yuv_YUVtoRGB_1(int y, int u, int v)
{
	y *= 1192;

	int r = (y + 1634 * v);
	int g = (y - 833 * v - 400 * u);
	int b = (y + 2066 * u);

	r = (r<0 ? 0 : (r>262143 ? 262143 : r));
	g = (g<0 ? 0 : (g>262143 ? 262143 : g));
	b = (b<0 ? 0 : (b>262143 ? 262143 : b));

	return (RGB32)(
			(0x00FF0000 & (r<< 6)) |
			(0x0000FF00 & (g>> 2)) |
			(0x000000FF & (b>>10)));
}

// ARGB(8888)→YUV(NV12)
YUV* Utils::yuv_RGBtoNV12(RGB32* rgb32, YUV* yuv)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	const unsigned int vw = video_width;
	const unsigned int vh = video_height;
	const unsigned int va = video_area;

	int yIndex = 0;
	int uvIndex = va;

	for (unsigned int i=0,k=0; i<vh; i++) {
		for (unsigned int j=0; j<vw; j++,k++) {
			const int R = (rgb32[k] & 0x00FF0000) >> 16;
			const int G = (rgb32[k] & 0x0000FF00) >> 8;
			const int B = (rgb32[k] & 0x000000FF);

			const int Y = (( 66*R + 129*G +  25*B + 128) >> 8) +  16;
			yuv[yIndex++] = (YUV) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));

			if (i % 2 == 0 && j % 2 == 0) {
				const int U = ((-38*R -  74*G + 112*B + 128) >> 8) + 128;
				const int V = ((112*R -  94*G -  18*B + 128) >> 8) + 128;
				yuv[uvIndex++] = (YUV)((U<0) ? 0 : ((U > 255) ? 255 : U));
				yuv[uvIndex++] = (YUV)((V<0) ? 0 : ((V > 255) ? 255 : V));
			}
		}
	}

	return yuv;
}

// ARGB(8888)→YUV(NV21)
YUV* Utils::yuv_RGBtoNV21(RGB32* rgb32, YUV* yuv)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	const unsigned int vw = video_width;
	const unsigned int vh = video_height;
	const unsigned int va = video_area;

	int yIndex = 0;
	int uvIndex = va;

	for (unsigned int i=0,k=0; i<vh; i++) {
		for (unsigned int j=0; j<vw; j++,k++) {
			const int R = (rgb32[k] & 0x00FF0000) >> 16;
			const int G = (rgb32[k] & 0x0000FF00) >> 8;
			const int B = (rgb32[k] & 0x000000FF);

			const int Y = (( 66*R + 129*G +  25*B + 128) >> 8) +  16;
			yuv[yIndex++] = (YUV) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));

			if (i % 2 == 0 && j % 2 == 0) {
				const int V = ((112*R -  94*G -  18*B + 128) >> 8) + 128;
				const int U = ((-38*R -  74*G + 112*B + 128) >> 8) + 128;
				yuv[uvIndex++] = (YUV)((V<0) ? 0 : ((V > 255) ? 255 : V));
				yuv[uvIndex++] = (YUV)((U<0) ? 0 : ((U > 255) ? 255 : U));
			}
		}
	}

	return yuv;
}

// ARGB(8888)→YUV(I420)
YUV* Utils::yuv_RGBtoI420(RGB32* rgb32, YUV* yuv)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	const unsigned int vw = video_width;
	const unsigned int vh = video_height;
	const unsigned int va = video_area;

	int yIndex = 0;
	int uIndex = va;
	int vIndex = va + va / 4;

	for (unsigned int i=0,k=0; i<vh; i++) {
		for (unsigned int j=0; j<vw; j++,k++) {
			const int R = (rgb32[k] & 0x00FF0000) >> 16;
			const int G = (rgb32[k] & 0x0000FF00) >> 8;
			const int B = (rgb32[k] & 0x000000FF);

			const int Y = (( 66*R + 129*G +  25*B + 128) >> 8) +  16;
			yuv[yIndex++] = (YUV) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));

			if (i % 2 == 0 && j % 2 == 0) {
				const int U = ((-38*R -  74*G + 112*B + 128) >> 8) + 128;
				yuv[uIndex++] = (YUV)((U<0) ? 0 : ((U > 255) ? 255 : U));

				const int V = ((112*R -  94*G -  18*B + 128) >> 8) + 128;
				yuv[vIndex++] = (YUV)((V<0) ? 0 : ((V > 255) ? 255 : V));
			}
		}
	}

	return yuv;
}
