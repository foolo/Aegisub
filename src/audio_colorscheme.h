// Copyright (c) 2009-2010, Niels Martin Hansen
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

/// @file audio_colorscheme.h
/// @see audio_colorscheme.cpp
/// @ingroup audio_ui
///
/// Manage colour schemes for the audio display

#pragma once

#include <vector>

#include <wx/colour.h>

#include "utils.h"


/// @class AudioSpectrumColorMap
/// @brief Provides colour maps for audio display rendering
///
/// Maps values from floats in range 0..1 into RGB colour values.
///
/// First create an instance of this class, then call an initialisation function
/// in it to fill the palette with a colour map.
class AudioColorScheme {

public:
	/// @brief Map a floating point value to RGB
	/// @param val   [in] The value to map from
	/// @param pixel [out] First byte of the pixel to write
	///
	/// Writes into the XRGB pixel (assumed 32 bit without alpha) passed.
	/// The pixel format is assumed to be the same as that in the palette.
	static void map(float val, unsigned char *pixel)
	{
		val = std::min(val, 1.0f);
		int t = (1.0 - val) * 255;
		// Copy to the destination.
		// Has to be done one byte at a time since we're writing RGB and not RGBX or RGBA
		// data, and we otherwise write past the end of the pixel we're writing, possibly
		// hitting adjacent memory blocks or just overwriting the start of the following
		// scanline in the image.
		// As the image is 24 bpp, 3 of every 4 uint32_t writes would  be unaligned anyway.
		pixel[0] = t;
		pixel[1] = t;
		pixel[2] = t;
	}
};
