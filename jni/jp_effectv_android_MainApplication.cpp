/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * jp_effectv_android_MainApplication.cpp : JNI
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

#include "stdio.h"

#include "jp_effectv_android_MainApplication.h"
#include "Utils.h"
#include "effects/BaseEffecTV.h"
#include "effects/NoOperation.h"
#include "effects/AgingTV.h"
#include "effects/BaltanTV.h"
#include "effects/BlueScreenTV.h"
#include "effects/BrokenTV.h"
#include "effects/BurningTV.h"
#include "effects/ChameleonTV.h"
#include "effects/ColorfulStreak.h"
#include "effects/CycleTV.h"
#include "effects/DiceTV.h"
#include "effects/DiffTV.h"
#include "effects/DisplayWall.h"
#include "effects/DotTV.h"
#include "effects/EdgeTV.h"
#include "effects/EdgeBlurTV.h"
#include "effects/FireTV.h"
#include "effects/HolographicTV.h"
#include "effects/LensTV.h"
#include "effects/LifeTV.h"
#include "effects/MatrixTV.h"
#include "effects/MosaicTV.h"
#include "effects/NervousHalf.h"
#include "effects/NervousTV.h"
#include "effects/NoiseTV.h"
#include "effects/OneDTV.h"
#include "effects/OpTV.h"
#include "effects/PredatorTV.h"
#include "effects/PUPTV.h"
#include "effects/PuzzleTV.h"
#include "effects/QuarkTV.h"
#include "effects/RadioacTV.h"
#include "effects/RandomDotStereoTV.h"
#include "effects/RevTV.h"
#include "effects/RippleTV.h"
#include "effects/RndmTV.h"
#include "effects/ShagadelicTV.h"
#include "effects/SimuraTV.h"
#include "effects/SloFastTV.h"
#include "effects/SparkTV.h"
#include "effects/SpiralTV.h"
#include "effects/StreakTV.h"
#include "effects/TimeDistortion.h"
#include "effects/TransformTV.h"
#include "effects/VertigoTV.h"
#include "effects/WarholTV.h"
#include "effects/WarpTV.h"

#define  LOG_TAG "JNI"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

#if 1
#ifdef __cplusplus
extern "C" {
#endif
extern int cnvavc(const char* src_path, const char* dst_path,
		int video_w, int video_h, int fps);
#ifdef __cplusplus
}
#endif
#endif

//---------------------------------------------------------------------
// Static data
//---------------------------------------------------------------------
static BaseEffecTV* sEffects[] = {
		new NoOperation(),

		new AgingTV(),
		new BaltanTV(),
		new BlueScreenTV(),
		new BrokenTV(),
		new BurningTV(),
		new ChameleonTV(),
		new ColorfulStreak(),
		new CycleTV(),
		new DiceTV(),
		new DiffTV(),
		new DisplayWall(),
		new DotTV(),
		new EdgeBlurTV(),
		new EdgeTV(),
		new FireTV(),
		new HolographicTV(),
		new LensTV(),
		new LifeTV(),
		new MatrixTV(),
		new MosaicTV(),
		new NervousHalf(),
		new NervousTV(),
		new NoiseTV(),
		new OneDTV(),
		new OpTV(),
		new PredatorTV(),
		new PUPTV(),
		new PuzzleTV(),
		new QuarkTV(),
		new RadioacTV(),
		new RandomDotStereoTV(),
		new RevTV(),
		new RippleTV(),
		new RndmTV(),
		new ShagadelicTV(),
		new SimuraTV(),
		new SloFastTV(),
		new SparkTV(),
		new SpiralTV(),
		new StreakTV(),
		new TimeDistortion(),
		new TransformTV(),
		new VertigoTV(),
		new WarholTV(),
		new WarpTV(),
};
static const int sEffectsNum = sizeof(sEffects) / sizeof(BaseEffecTV*);
static int sEffectType = -1;

static Utils* sUtils = NULL;

static bool sFacingFront = false;
static int sCameraW = 0;
static int sCameraH = 0;
static int sFps = 0;
static unsigned int sDataSize = 0;
static unsigned char* sDataP = NULL;
static char sTmpDstMsg[jp_effectv_android_MainApplication_DST_MSG_LEN];

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM* vm, void* reserved)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
/** . */
JNIEXPORT void JNICALL Java_jp_effectv_android_MainApplication_native_1init
(JNIEnv* env, jclass clazz, jstring jLocalPath)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	const char* local_path = env->GetStringUTFChars(jLocalPath, NULL);

	for (int i=0; i<sEffectsNum; i++) {
		sEffects[i]->setConfigPath(local_path, sEffects[i]->name());
	}

	if (local_path != NULL) {
		env->ReleaseStringUTFChars(jLocalPath, local_path);
	}
}

/** . */
JNIEXPORT jobjectArray JNICALL Java_jp_effectv_android_MainApplication_native_1names
(JNIEnv* env, jclass clazz)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	jclass c = env->FindClass("java/lang/String");
	jobjectArray a = env->NewObjectArray(sEffectsNum, c, jstring());

	for (int i=0; i<sEffectsNum; i++) {
		env->SetObjectArrayElement(a, i, env->NewStringUTF(sEffects[i]->name()));
	}

	return a;
}

/** . */
JNIEXPORT jobjectArray JNICALL Java_jp_effectv_android_MainApplication_native_1titles
(JNIEnv* env, jclass clazz)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	jclass c = env->FindClass("java/lang/String");
	jobjectArray a = env->NewObjectArray(sEffectsNum, c, jstring());

	for (int i=0; i<sEffectsNum; i++) {
		env->SetObjectArrayElement(a, i, env->NewStringUTF(sEffects[i]->title()));
	}

	return a;
}

/** . */
JNIEXPORT jobjectArray JNICALL Java_jp_effectv_android_MainApplication_native_1funcs
(JNIEnv* env, jclass clazz, jint jEffectType)
{
	LOGI("%s(L=%d): type=%d", __func__, __LINE__, jEffectType);

	if (jEffectType < 0 || sEffectsNum <= jEffectType) {
		return NULL;
	}

	const char** funcs = sEffects[jEffectType]->funcs();
	int funcs_num = 0;
	for (; funcs[funcs_num]!=NULL; funcs_num++);

	jclass c = env->FindClass("java/lang/String");
	jobjectArray a = env->NewObjectArray(funcs_num, c, jstring());

	for (int i=0; i<funcs_num; i++) {
		env->SetObjectArrayElement(a, i, env->NewStringUTF(funcs[i]));
	}

	return a;
}

/** . */
JNIEXPORT jint JNICALL Java_jp_effectv_android_MainApplication_native_1start
(JNIEnv* env, jclass clazz, jboolean jFacingFront, jint jCameraW, jint jCameraH, jint jFps, jint jEffectType)
{
	LOGI("%s(L=%d): f=%s, w=%d, h=%d, type=%d", __func__, __LINE__, (jFacingFront?"front":"back"), jCameraW, jCameraH, jEffectType);

	if (jCameraW <= 0 || jCameraH <= 0 || jFps <= 0 || jEffectType < 0 || sEffectsNum <= jEffectType) {
		return -1;
	}

	sFacingFront = jFacingFront;
	sCameraW = jCameraW;
	sCameraH = jCameraH;
	sFps = jFps;
	if (sDataP != NULL) {
		delete[] sDataP;
		sDataP = NULL;
	}
	sDataSize = sCameraW * sCameraH * 3 / 2;
	sDataP = new unsigned char[sDataSize];

	sEffectType = jEffectType;

	sUtils = new Utils(jCameraW, jCameraH);

	return sEffects[sEffectType]->start(sUtils, jCameraW, jCameraH);
}

/** . */
JNIEXPORT jint JNICALL Java_jp_effectv_android_MainApplication_native_1stop
(JNIEnv* env, jclass clazz)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	jint rcode = 0;

	if (0 <= sEffectType && sEffectType < sEffectsNum) {
		rcode = sEffects[sEffectType]->stop();
	}

	if (sUtils != NULL) {
		delete sUtils;
		sUtils = NULL;
	}

	sCameraW = 0;
	sCameraH = 0;
	sDataSize = 0;
	if (sDataP != NULL) {
		delete[] sDataP;
		sDataP = NULL;
	}

	return rcode;
}

/** . */
JNIEXPORT jint JNICALL Java_jp_effectv_android_MainApplication_native_1draw
(JNIEnv* env, jclass clazz, jbyteArray jSrcYuv, jintArray jDstRgb, jcharArray jDstMsg, jint jDstYuvFmt, jbyteArray jDstYuv)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	if (sCameraW <= 0 || sCameraH <= 0 || sDataP == NULL) {
		return -1;
	}

	unsigned char* srcYuvP = (unsigned char*)(env->GetByteArrayElements(jSrcYuv, NULL));
	unsigned int*  dstRgbP = (unsigned int *)(env->GetIntArrayElements(jDstRgb, NULL));
	jchar*         dstMsgP = (jchar        *)(env->GetCharArrayElements(jDstMsg, NULL));
	unsigned char* dstYuvP = NULL;
	if (jDstYuv != NULL) {
		dstYuvP = (unsigned char*)(env->GetByteArrayElements(jDstYuv, NULL));
	}

	// H-FLIP?
	if (sFacingFront) {
		// only for NV21
		const int w1 = sCameraW;
		const int h1 = sCameraH;
		const int wh = w1 / 2;
		const int hh = h1 / 2;
		const int w2 = w1 * 2;
		unsigned char* sy = srcYuvP;
		unsigned char* dy = sDataP + w1 - 1;
		unsigned short* suv = ((unsigned short*)srcYuvP) + wh * h1;
		unsigned short* duv = ((unsigned short*)sDataP)  + wh * h1 + wh - 1;
		// Y
		for (int y=h1; y>0; y--) {
			for (int x=w1; x>0; x--) {
				*dy-- = *sy++;
			}
			dy += w2;
		}
		// U/V
		for (int y=hh; y>0; y--) {
			for (int x=wh; x>0; x--) {
				*duv-- = *suv++;
			}
			duv += w1;
		}
	} else {
		memcpy(sDataP, srcYuvP, sizeof(unsigned char) * sDataSize);
	}

	sTmpDstMsg[0] = 0;
	jint rcode = sEffects[sEffectType]->draw(sDataP, dstRgbP, sTmpDstMsg);
	if (rcode == 0) {
		// for Video recording
		if (dstYuvP != NULL) {
			sUtils->yuv_RGBtoYUV(dstRgbP, jDstYuvFmt, dstYuvP);
		}

		// ASCII -> UTF-16
		int i = 0;
		for (i=0; sTmpDstMsg[i]!=0; i++) {
			dstMsgP[i] = sTmpDstMsg[i];
		}
		dstMsgP[i] = 0;
	}

	env->ReleaseByteArrayElements(jSrcYuv, (jbyte*)srcYuvP, JNI_ABORT);
	env->ReleaseIntArrayElements(jDstRgb, (jint*)dstRgbP, 0);
	env->ReleaseCharArrayElements(jDstMsg, (jchar*)dstMsgP, 0);
	if (jDstYuv != NULL) {
		env->ReleaseByteArrayElements(jDstYuv, (jbyte*)dstYuvP, 0);
	}

	return rcode;
}

/** . */
JNIEXPORT jstring JNICALL Java_jp_effectv_android_MainApplication_native_1event
(JNIEnv* env, jclass clazz, jint jKeyCode)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, jKeyCode);
	const char* msg = sEffects[sEffectType]->event(jKeyCode);
	return (msg!=NULL ? env->NewStringUTF(msg) : NULL);
}

/** . */
JNIEXPORT jstring JNICALL Java_jp_effectv_android_MainApplication_native_1touch
(JNIEnv* env, jclass clazz, jint jAction, jint jX, jint jY)
{
	LOGI("%s(L=%d): a=%d, x=%d, y=%d", __func__, __LINE__, jAction, jX, jY);
	const char* msg = sEffects[sEffectType]->touch(jAction, jX, jY);
	return (msg!=NULL ? env->NewStringUTF(msg) : NULL);
}

/** . */
JNIEXPORT jint JNICALL Java_jp_effectv_android_MainApplication_native_1cnvavc
(JNIEnv* env, jclass clazz, jstring jSrcPath, jstring jDstPath)
{
	LOGI("%s(L=%d)", __func__, __LINE__);

	const char* src_path = env->GetStringUTFChars(jSrcPath, NULL);
	const char* dst_path = env->GetStringUTFChars(jDstPath, NULL);

	jint rcode = cnvavc(src_path, dst_path, sCameraW, sCameraH, sFps);

	if (src_path != NULL) {
		env->ReleaseStringUTFChars(jSrcPath, src_path);
	}
	if (dst_path != NULL) {
		env->ReleaseStringUTFChars(jDstPath, dst_path);
	}

	return rcode;
}
