/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DumpMediaCodec.java : Dump MediaCodec information.
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

package jp.effectv.android;

import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecCapabilities;
import android.media.MediaCodecInfo.CodecProfileLevel;
import android.media.MediaCodecList;
import android.util.Log;

public class DumpMediaCodec {
	private static final String TAG = "MediaCodec";

	//---------------------------------------------------------------------
	// PUBLIC METHODS
	//---------------------------------------------------------------------
	public static void dump(boolean encoder) {
		// Codec
		final int cnt = MediaCodecList.getCodecCount();
		for (int i=0; i<cnt; i++) {
			final MediaCodecInfo mediaCodecInfo = MediaCodecList.getCodecInfoAt(i);
			if (encoder != mediaCodecInfo.isEncoder()) {
				continue;
			}
			Log.i(TAG, "------------------------------------------------");
			Log.i(TAG, "NAME: "+mediaCodecInfo.getName());
			// Type
			for (final String type : mediaCodecInfo.getSupportedTypes()) {
				CodecCapabilities codecCapabilities;
				try {
					codecCapabilities = mediaCodecInfo.getCapabilitiesForType(type);
				} catch (Exception e) {
					continue;
				}
				Log.i(TAG, "TYPE: "+type);
				// Color format
				for (final int colorFormat : codecCapabilities.colorFormats) {
					Log.i(TAG, "Color Format: "+colorFormatToString(colorFormat));
				}
				// Profile level
				for (final CodecProfileLevel codecProfileLevel : codecCapabilities.profileLevels) {
					if (type.contains("3gpp")) {
						Log.i(TAG,
								"Profile: "+h263ProfileToString(codecProfileLevel.profile)+
								", Level: "+h263LevelToString(codecProfileLevel.level));
					} else if (type.contains("avc")) {
						Log.i(TAG,
								"Profile: "+avcProfileToString(codecProfileLevel.profile)+
								", Level: "+avcLevelToString(codecProfileLevel.level));
					} else if (type.contains("mp4")) {
						Log.i(TAG,
								"Profile: "+mpeg4ProfileToString(codecProfileLevel.profile)+
								", Level: "+mpeg4LevelToString(codecProfileLevel.level));
					} else if (type.contains("aac")) {
						Log.i(TAG,
								"Profile: "+aacProfileToString(codecProfileLevel.profile));
					} else {
						Log.i(TAG, "unknown type");
					}
				}
			}
		}
	}

	//---------------------------------------------------------------------
	// PRIVATE METHODS
	//---------------------------------------------------------------------
	private static String colorFormatToString(int colorFormat) {
		switch(colorFormat) {
		case CodecCapabilities.COLOR_Format12bitRGB444:
			return "COLOR_Format12bitRGB444";
		case CodecCapabilities.COLOR_Format16bitARGB1555:
			return "COLOR_Format16bitARGB1555";
		case CodecCapabilities.COLOR_Format16bitARGB4444:
			return "COLOR_Format16bitARGB4444";
		case CodecCapabilities.COLOR_Format16bitBGR565:
			return "COLOR_Format16bitBGR565";
		case CodecCapabilities.COLOR_Format16bitRGB565:
			return "COLOR_Format16bitRGB565";
		case CodecCapabilities.COLOR_Format18BitBGR666:
			return "COLOR_Format18BitBGR666";
		case CodecCapabilities.COLOR_Format18bitARGB1665:
			return "COLOR_Format18bitARGB1665";
		case CodecCapabilities.COLOR_Format18bitRGB666:
			return "COLOR_Format18bitRGB666";
		case CodecCapabilities.COLOR_Format19bitARGB1666:
			return "COLOR_Format19bitARGB1666";
		case CodecCapabilities.COLOR_Format24BitABGR6666:
			return "COLOR_Format24BitABGR6666";
		case CodecCapabilities.COLOR_Format24BitARGB6666:
			return "COLOR_Format24BitARGB6666";
		case CodecCapabilities.COLOR_Format24bitARGB1887:
			return "COLOR_Format24bitARGB1887";
		case CodecCapabilities.COLOR_Format24bitBGR888:
			return "COLOR_Format24bitBGR888";
		case CodecCapabilities.COLOR_Format24bitRGB888:
			return "COLOR_Format24bitRGB888";
		case CodecCapabilities.COLOR_Format25bitARGB1888:
			return "COLOR_Format25bitARGB1888";
		case CodecCapabilities.COLOR_Format32bitARGB8888:
			return "COLOR_Format32bitARGB8888";
		case CodecCapabilities.COLOR_Format32bitBGRA8888:
			return "COLOR_Format32bitBGRA8888";
		case CodecCapabilities.COLOR_Format8bitRGB332:
			return "COLOR_Format8bitRGB332";
		case CodecCapabilities.COLOR_FormatCbYCrY:
			return "COLOR_FormatCbYCrY";
		case CodecCapabilities.COLOR_FormatCrYCbY:
			return "COLOR_FormatCrYCbY";
		case CodecCapabilities.COLOR_FormatL16:
			return "COLOR_FormatL16";
		case CodecCapabilities.COLOR_FormatL2:
			return "COLOR_FormatL2";
		case CodecCapabilities.COLOR_FormatL24:
			return "COLOR_FormatL24";
		case CodecCapabilities.COLOR_FormatL32:
			return "COLOR_FormatL32";
		case CodecCapabilities.COLOR_FormatL4:
			return "COLOR_FormatL4";
		case CodecCapabilities.COLOR_FormatL8:
			return "COLOR_FormatL8";
		case CodecCapabilities.COLOR_FormatMonochrome:
			return "COLOR_FormatMonochrome";
		case CodecCapabilities.COLOR_FormatRawBayer10bit:
			return "COLOR_FormatRawBayer10bit";
		case CodecCapabilities.COLOR_FormatRawBayer8bit:
			return "COLOR_FormatRawBayer8bit";
		case CodecCapabilities.COLOR_FormatRawBayer8bitcompressed:
			return "COLOR_FormatRawBayer8bitcompressed";
		case CodecCapabilities.COLOR_FormatYCbYCr:
			return "COLOR_FormatYCbYCr";
		case CodecCapabilities.COLOR_FormatYCrYCb:
			return "COLOR_FormatYCrYCb";
		case CodecCapabilities.COLOR_FormatYUV411PackedPlanar:
			return "COLOR_FormatYUV411PackedPlanar";
		case CodecCapabilities.COLOR_FormatYUV411Planar:
			return "COLOR_FormatYUV411Planar";
		case CodecCapabilities.COLOR_FormatYUV420PackedPlanar:
			return "COLOR_FormatYUV420PackedPlanar";
		case CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar:
			return "COLOR_FormatYUV420PackedSemiPlanar";
		case CodecCapabilities.COLOR_FormatYUV420Planar:
			return "COLOR_FormatYUV420Planar";
		case CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
			return "COLOR_FormatYUV420SemiPlanar";
		case CodecCapabilities.COLOR_FormatYUV422PackedPlanar:
			return "COLOR_FormatYUV422PackedPlanar";
		case CodecCapabilities.COLOR_FormatYUV422PackedSemiPlanar:
			return "COLOR_FormatYUV422PackedSemiPlanar";
		case CodecCapabilities.COLOR_FormatYUV422Planar:
			return "COLOR_FormatYUV422Planar";
		case CodecCapabilities.COLOR_FormatYUV422SemiPlanar:
			return "COLOR_FormatYUV422SemiPlanar";
		case CodecCapabilities.COLOR_FormatYUV444Interleaved:
			return "COLOR_FormatYUV444Interleaved";
		case CodecCapabilities.COLOR_QCOM_FormatYUV420SemiPlanar:
			return "COLOR_QCOM_FormatYUV420SemiPlanar";
		case CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar:
			return "COLOR_TI_FormatYUV420PackedSemiPlanar";
		}
		return "unknown("+colorFormat+")";
	}

	private static String mpeg4ProfileToString(int profile) {
		switch (profile) {
		case CodecProfileLevel.MPEG4ProfileAdvancedCoding:
			return "MPEG4ProfileAdvancedCoding";
		case CodecProfileLevel.MPEG4ProfileAdvancedCore:
			return "MPEG4ProfileAdvancedCore";
		case CodecProfileLevel.MPEG4ProfileAdvancedRealTime:
			return "MPEG4ProfileAdvancedRealTime";
		case CodecProfileLevel.MPEG4ProfileAdvancedScalable:
			return "MPEG4ProfileAdvancedScalable";
		case CodecProfileLevel.MPEG4ProfileAdvancedSimple:
			return "MPEG4ProfileAdvancedSimple";
		case CodecProfileLevel.MPEG4ProfileBasicAnimated:
			return "MPEG4ProfileBasicAnimated";
		case CodecProfileLevel.MPEG4ProfileCore:
			return "MPEG4ProfileCore";
		case CodecProfileLevel.MPEG4ProfileCoreScalable:
			return "MPEG4ProfileCoreScalable";
		case CodecProfileLevel.MPEG4ProfileHybrid:
			return "MPEG4ProfileHybrid";
		case CodecProfileLevel.MPEG4ProfileMain:
			return "MPEG4ProfileMain";
		case CodecProfileLevel.MPEG4ProfileNbit:
			return "MPEG4ProfileNbit";
		case CodecProfileLevel.MPEG4ProfileScalableTexture:
			return "MPEG4ProfileScalableTexture";
		case CodecProfileLevel.MPEG4ProfileSimple:
			return "MPEG4ProfileSimple";
		case CodecProfileLevel.MPEG4ProfileSimpleFBA:
			return "MPEG4ProfileSimpleFBA";
		case CodecProfileLevel.MPEG4ProfileSimpleFace:
			return "MPEG4ProfileSimpleFace";
		case CodecProfileLevel.MPEG4ProfileSimpleScalable:
			return "MPEG4ProfileSimpleScalable";
		}
		return "unknown(mpeg4:"+profile+")";
	}

	private static String h263ProfileToString(int profile) {
		switch (profile) {
		case CodecProfileLevel.H263ProfileBackwardCompatible:
			return "H263ProfileBackwardCompatible";
		case CodecProfileLevel.H263ProfileBaseline:
			return "H263ProfileBaseline";
		case CodecProfileLevel.H263ProfileH320Coding:
			return "H263ProfileH320Coding";
		case CodecProfileLevel.H263ProfileHighCompression:
			return "H263ProfileHighCompression";
		case CodecProfileLevel.H263ProfileHighLatency:
			return "H263ProfileHighLatency";
		case CodecProfileLevel.H263ProfileISWV2:
			return "H263ProfileISWV2";
		case CodecProfileLevel.H263ProfileISWV3:
			return "H263ProfileISWV3";
		case CodecProfileLevel.H263ProfileInterlace:
			return "H263ProfileInterlace";
		case CodecProfileLevel.H263ProfileInternet:
			return "H263ProfileInternet";
		}
		return "unknown(h263:"+profile+")";
	}

	private static String avcProfileToString(int profile) {
		switch (profile) {
		case CodecProfileLevel.AVCProfileBaseline:
			return "AVCProfileBaseline";
		case CodecProfileLevel.AVCProfileExtended:
			return "AVCProfileExtended";
		case CodecProfileLevel.AVCProfileHigh:
			return "AVCProfileHigh";
		case CodecProfileLevel.AVCProfileHigh10:
			return "AVCProfileHigh10";
		case CodecProfileLevel.AVCProfileHigh422:
			return "AVCProfileHigh422";
		case CodecProfileLevel.AVCProfileHigh444:
			return "AVCProfileHigh444";
		case CodecProfileLevel.AVCProfileMain:
			return "AVCProfileMain";
		}
		return "unknown(avc:"+profile+")";
	}

	private static String aacProfileToString(int profile) {
		switch (profile) {
		case CodecProfileLevel.AACObjectELD:
			return "AACObjectELD";
		case CodecProfileLevel.AACObjectERLC:
			return "AACObjectERLC";
		case CodecProfileLevel.AACObjectHE:
			return "AACObjectHE";
		case CodecProfileLevel.AACObjectHE_PS:
			return "AACObjectHE_PS";
		case CodecProfileLevel.AACObjectLC:
			return "AACObjectLC";
		case CodecProfileLevel.AACObjectLD:
			return "AACObjectLD";
		case CodecProfileLevel.AACObjectLTP:
			return "AACObjectLTP";
		case CodecProfileLevel.AACObjectMain:
			return "AACObjectMain";
		case CodecProfileLevel.AACObjectSSR:
			return "AACObjectSSR";
		case CodecProfileLevel.AACObjectScalable:
			return "AACObjectScalable";
		}
		return "unknown(aac:"+profile+")";
	}

	private static String mpeg4LevelToString(int level) {
		switch (level) {
		case CodecProfileLevel.MPEG4Level0:
			return "MPEG4Level0";
		case CodecProfileLevel.MPEG4Level0b:
			return "MPEG4Level0b";
		case CodecProfileLevel.MPEG4Level1:
			return "MPEG4Level1";
		case CodecProfileLevel.MPEG4Level2:
			return "MPEG4Level2";
		case CodecProfileLevel.MPEG4Level3:
			return "MPEG4Level3";
		case CodecProfileLevel.MPEG4Level4:
			return "MPEG4Level4";
		case CodecProfileLevel.MPEG4Level4a:
			return "MPEG4Level4a";
		case CodecProfileLevel.MPEG4Level5:
			return "MPEG4Level5";
		}
		return "unknown(mpeg4:"+level+")";
	}

	private static String h263LevelToString(int level) {
		switch (level) {
		case CodecProfileLevel.H263Level10:
			return "H263Level10";
		case CodecProfileLevel.H263Level20:
			return "H263Level20";
		case CodecProfileLevel.H263Level30:
			return "H263Level30";
		case CodecProfileLevel.H263Level40:
			return "H263Level40";
		case CodecProfileLevel.H263Level45:
			return "H263Level45";
		case CodecProfileLevel.H263Level50:
			return "H263Level50";
		case CodecProfileLevel.H263Level60:
			return "H263Level60";
		case CodecProfileLevel.H263Level70:
			return "H263Level70";
		}
		return "unknown(h263:"+level+")";
	}

	private static String avcLevelToString(int level) {
		switch (level) {
		case CodecProfileLevel.AVCLevel1:
			return "AVCLevel1";
		case CodecProfileLevel.AVCLevel11:
			return "AVCLevel11";
		case CodecProfileLevel.AVCLevel12:
			return "AVCLevel12";
		case CodecProfileLevel.AVCLevel13:
			return "AVCLevel13";
		case CodecProfileLevel.AVCLevel1b:
			return "AVCLevel1b";
		case CodecProfileLevel.AVCLevel2:
			return "AVCLevel2";
		case CodecProfileLevel.AVCLevel21:
			return "AVCLevel21";
		case CodecProfileLevel.AVCLevel22:
			return "AVCLevel22";
		case CodecProfileLevel.AVCLevel3:
			return "AVCLevel3";
		case CodecProfileLevel.AVCLevel31:
			return "AVCLevel31";
		case CodecProfileLevel.AVCLevel32:
			return "AVCLevel32";
		case CodecProfileLevel.AVCLevel4:
			return "AVCLevel4";
		case CodecProfileLevel.AVCLevel41:
			return "AVCLevel41";
		case CodecProfileLevel.AVCLevel42:
			return "AVCLevel42";
		case CodecProfileLevel.AVCLevel5:
			return "AVCLevel5";
		case CodecProfileLevel.AVCLevel51:
			return "AVCLevel51";
		}
		return "unknown(avc:"+level+")";
	}
}
