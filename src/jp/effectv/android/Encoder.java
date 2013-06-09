/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * Encoder.java : Video encoder
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

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Calendar;
import java.util.Locale;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaScannerConnection;
import android.text.format.DateFormat;
import android.util.Log;

public class Encoder {
	private static final boolean DEBUG = true;
	private static final String TAG = "Encoder";

	//---------------------------------------------------------------------
	// CONSTANTS
	//---------------------------------------------------------------------
	private static final int ColorFormat_NV12 = 1;
	private static final int ColorFormat_NV21 = 2;
	private static final int ColorFormat_I420 = 3;

	private static final int[][] ColorFormatList = {
		{ColorFormat_NV12, MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar},
		{ColorFormat_NV21, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar},
		{ColorFormat_I420, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar},
	};

	//---------------------------------------------------------------------
	// MEMBERS
	//---------------------------------------------------------------------
	private Context mContext = null;
	private String mTmpEtvPath = null;
	private String mTmpMp4Path = null;
	private File mTmpEtvFile = null;
	private BufferedOutputStream mOutput = null;
	private MediaCodec mCodec = null;
	private int mColorFormat = 0;

	//---------------------------------------------------------------------
	// PUBLIC METHODS
	//---------------------------------------------------------------------
	public Encoder(Context context) {
		if (DEBUG) Log.d(TAG, "Encoder()");
		mContext = context;

		File f = context.getExternalFilesDir(null);
		if (f==null || (!f.exists() && !f.mkdirs())) {
			throw new RuntimeException(f.getAbsolutePath());
		}

		mTmpEtvPath = f.getPath()+"/tmp.etv";
		File f2 = new File(mTmpEtvPath);
		if (!f2.exists()) {
			try {
				f2.createNewFile();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}

		mTmpMp4Path = f.getPath()+"/tmp.mp4";
		File f3 = new File(mTmpMp4Path);
		if (!f3.exists()) {
			try {
				f3.createNewFile();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	public int start(int w, int h, int fps, String mime, int bps, int ifi) {
		if (DEBUG) Log.d(TAG, "start(): w="+w+", h="+h+", fps="+fps+", mime="+mime+", bps="+bps+", ifi="+ifi);

		// File
		mTmpEtvFile = new File(mTmpEtvPath);
		try {
			mOutput = new BufferedOutputStream(new FileOutputStream(mTmpEtvFile));
		} catch (Exception e){
			e.printStackTrace();
			return -1;
		}

		//
		MediaFormat mediaFormat;
		mCodec = MediaCodec.createEncoderByType(mime);
		mediaFormat = MediaFormat.createVideoFormat(mime, w, h);
		mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, bps);
		mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, fps);
		mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, ifi);
		for (int[] colorFormat : ColorFormatList) {
			try {
				mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat[1]);
				mCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
				mCodec.start();
				// Success
				mColorFormat = colorFormat[0];
				return 0;
			} catch (Exception e) {
				continue;
			}
		}

		Log.e(TAG, "start(): Failure");

		// Failure
		if (mCodec != null) {
			mCodec.release();
			mCodec = null;
		}
		if (mOutput != null) {
			try {
				mOutput.close();
			} catch (Exception e){
				e.printStackTrace();
			}
			mOutput = null;
		}
		if (mTmpEtvFile != null) {
			if (mTmpEtvFile.exists()) {
				mTmpEtvFile.delete();
			}
			mTmpEtvFile = null;
		}
		return -1;
	}

	public void stop(final File dir) {
		if (DEBUG) Log.d(TAG, "stop()");
		try {
			mCodec.stop();
			mCodec.release();
			mOutput.flush();
			mOutput.close();
		} catch (Exception e){
			e.printStackTrace();
		}
		mCodec = null;
		mOutput = null;
		mTmpEtvFile = null;

		// Convert
		int rcode = MainApplication.native_cnvavc(mTmpEtvPath, mTmpMp4Path);
		if (rcode != 0) {
			throw new RuntimeException("native_cnvavc()="+rcode);
		}

		// Find unique file name
		final String now = (String)DateFormat.format("yyyyMMdd_kkmmss", Calendar.getInstance());
		File path = null;
		for (int i=1; ; i++) {
			final String name = String.format(Locale.US,
					"ETV_%s_%1d.mp4", now, i);
			path = new File(dir, name);
			if (!path.exists()) {
				break;
			}
		}

		// Move file
		(new File(mTmpMp4Path)).renameTo(path);

		// Registry MediaScanner
		MediaScannerConnection.scanFile(mContext,
				new String[]{ path.getAbsolutePath() },
				new String[]{ "video/mp4" },
				null);
	}

	public int getColorFormat() {
		return mColorFormat;
	}

	public void offerEncoder(byte[] in) {
		if (DEBUG) Log.d(TAG, "offerEncoder(): in.length="+in.length);
		try {
			ByteBuffer[] iBufs = mCodec.getInputBuffers();
			ByteBuffer[] oBufs = mCodec.getOutputBuffers();

			int iIdx = mCodec.dequeueInputBuffer(-1);
			if (iIdx >= 0) {
				ByteBuffer iBuf = iBufs[iIdx];
				iBuf.clear();
				iBuf.put(in);
				mCodec.queueInputBuffer(iIdx, 0, in.length, 0, 0);
			}

			MediaCodec.BufferInfo bufInfo = new MediaCodec.BufferInfo();
			int oIdx = mCodec.dequeueOutputBuffer(bufInfo,0);
			while (oIdx >= 0) {
				ByteBuffer oBuf = oBufs[oIdx];
				byte[] out = new byte[bufInfo.size];
				oBuf.get(out);
				mOutput.write(out, 0, out.length);
				mCodec.releaseOutputBuffer(oIdx, false);
				oIdx = mCodec.dequeueOutputBuffer(bufInfo, 0);
			}
		} catch (Throwable t) {
			t.printStackTrace();
		}
	}
}
