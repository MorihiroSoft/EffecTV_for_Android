#
# EffecTV for Android
# Copyright (C) 2013 Morihiro Soft
#
# Android.mk : make file
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS=false

# Library name
LOCAL_MODULE    := libEffecTV

# Source files
LOCAL_SRC_FILES := \
    effects/BaseEffecTV.cpp \
    effects/NoOperation.cpp \
    effects/AgingTV.cpp \
    effects/BaltanTV.cpp \
    effects/BlueScreenTV.cpp \
    effects/BrokenTV.cpp \
    effects/BurningTV.cpp \
    effects/ChameleonTV.cpp \
    effects/ColorfulStreak.cpp \
    effects/CycleTV.cpp \
    effects/DiceTV.cpp \
    effects/DiffTV.cpp \
    effects/DisplayWall.cpp \
    effects/DotTV.cpp \
    effects/EdgeTV.cpp \
    effects/EdgeBlurTV.cpp \
    effects/FireTV.cpp \
    effects/HolographicTV.cpp \
    effects/LensTV.cpp \
    effects/LifeTV.cpp \
    effects/MatrixTV.cpp \
    effects/MosaicTV.cpp \
    effects/NervousHalf.cpp \
    effects/NervousTV.cpp \
    effects/NoiseTV.cpp \
    effects/OneDTV.cpp \
    effects/OpTV.cpp \
    effects/PredatorTV.cpp \
    effects/PUPTV.cpp \
    effects/PuzzleTV.cpp \
    effects/QuarkTV.cpp \
    effects/RadioacTV.cpp \
    effects/RandomDotStereoTV.cpp \
    effects/RevTV.cpp \
    effects/RippleTV.cpp \
    effects/RndmTV.cpp \
    effects/ShagadelicTV.cpp \
    effects/SimuraTV.cpp \
    effects/SloFastTV.cpp \
    effects/SparkTV.cpp \
    effects/SpiralTV.cpp \
    effects/StreakTV.cpp \
    effects/TimeDistortion.cpp \
    effects/TransformTV.cpp \
    effects/VertigoTV.cpp \
    effects/WarholTV.cpp \
    effects/WarpTV.cpp \
    Utils.cpp \
    cnvavc.c \
    jp_effectv_android_MainApplication.cpp

#
LOCAL_CFLAGS    := -Wall -Werror -Wno-deprecated
LOCAL_LDLIBS    := -llog -lm

include $(BUILD_SHARED_LIBRARY)
