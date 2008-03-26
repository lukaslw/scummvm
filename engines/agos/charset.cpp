/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */



#include "common/system.h"

#include "agos/agos.h"
#include "agos/intern.h"

#include "graphics/surface.h"

namespace AGOS {

void AGOSEngine_Feeble::doOutput(const byte *src, uint len) {
	if (_textWindow == NULL)
		return;

	while (len-- != 0) {
		if (getBitFlag(93)) {
			if (_curWindow == 3) {
				if ((_newLines >= _textWindow->scrollY) && (_newLines < (_textWindow->scrollY + 3)))
					sendWindow(*src);
				if (*src == '\n')		// Do two top lines of text only
					_newLines++;
				src++;
			}
		} else {
			if (getBitFlag(94)) {
				if (_curWindow == 3) {
					if (_newLines == (_textWindow->scrollY + 7))
						sendWindow(*src);
					if (*src == '\n')	// Do two top lines of text only
						_newLines++;
					src++;
				}
			} else {
				if (getBitFlag(92))
					delay(50);
				sendWindow(*src++);
			}
		}
	}
}

void AGOSEngine::doOutput(const byte *src, uint len) {
	uint idx;

	if (_textWindow == NULL)
		return;

	while (len-- != 0) {
		if (*src != 12 && _textWindow->iconPtr != NULL &&
				_fcsData1[idx = getWindowNum(_textWindow)] != 2) {

			_fcsData1[idx] = 2;
			_fcsData2[idx] = 1;
		}

		sendWindow(*src++);
	}
}

void AGOSEngine::clsCheck(WindowBlock *window) {
	uint index = getWindowNum(window);
	tidyIconArray(index);
	_fcsData1[index] = 0;
}

void AGOSEngine::tidyIconArray(uint i) {
	WindowBlock *window;

	if (_fcsData2[i]) {
		mouseOff();
		window = _windowArray[i];
		drawIconArray(i, window->iconPtr->itemRef, window->iconPtr->line, window->iconPtr->classMask);
		_fcsData2[i] = 0;
		mouseOn();
	}
}

static const byte simon_agaFont[] = {
	0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x50,0x20,0x10,0x40,0x88,0x30,0x40,0x00,0x88,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x50,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x05,
	0x00,0x00,0x00,0x30,0x00,0x10,0x20,0x48,0x10,0x20,0x00,0x48,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,
	0x00,0x00,0x00,0x28,0x00,0x00,0x28,0x54,0x00,0x28,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x28,0x00,0x00,0x54,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x10,0x00,0x00,0x10,0x28,0x08,0x10,0x20,0x44,0x00,0x40,0x00,0xB8,0x30,0x00,0x00,0x48,0x08,0x00,0x00,0x74,0x30,0x00,0x40,0x88,0x20,0x00,0x00,0x50,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x48,0xB4,0x00,0x48,0x00,0xB4,0x10,0x00,0x00,0x68,0x00,0x20,0x00,0x58,0x00,0x08,0x40,0xB4,0x08,0x40,0x00,0xB4,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x00,0x06,
	0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x50,0x10,0x00,0x40,0xA8,0x18,0x20,0x00,0x44,0x10,0x00,0x00,0x28,0x18,0x20,0x00,0x44,0x10,0x00,0x40,0xAC,0x0C,0x20,0x00,0x52,0x00,0x00,0x00,0x2C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x30,0x00,0x10,0x20,0x48,0x10,0x20,0x00,0x48,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,
	0x00,0x00,0x00,0x28,0x00,0x00,0x28,0x54,0x00,0x28,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x28,0x00,0x00,0x54,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x28,0x00,0x00,0x28,0x54,0x00,0x28,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x28,0x00,0x00,0x54,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x28,0x00,0x00,0x28,0x54,0x00,0x28,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x1C,0x20,0x40,0x82,0x28,0x00,0x00,0x54,0x28,0x00,0x00,0x54,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x10,0x20,0x48,0x10,0x20,0x00,0x48,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x05,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x10,0x20,0x48,0x10,0x20,0x00,0x48,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x05,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x18,0x20,0x40,0x84,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x20,0x40,0x90,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x04,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x20,0x40,0x90,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x04,
	0x00,0x00,0x00,0x38,0x00,0x18,0x20,0x44,0x04,0x00,0x40,0xBA,0x0C,0x00,0x40,0xB2,0x04,0x50,0x00,0xAA,0x44,0x20,0x00,0x9A,0x44,0x00,0x00,0xBA,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x50,0x00,0x20,0x40,0x90,0x00,0x20,0x00,0x50,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x05,
	0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x00,0x40,0xBA,0x04,0x00,0x00,0x5A,0x08,0x10,0x00,0x24,0x00,0x20,0x00,0x5C,0x00,0x04,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x08,0x10,0x20,0x44,0x04,0x00,0x40,0xBA,0x04,0x00,0x00,0x5A,0x00,0x08,0x10,0x24,0x04,0x00,0x00,0x5A,0x04,0x00,0x40,0xBA,0x18,0x20,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x08,0x00,0x08,0x00,0x14,0x08,0x00,0x10,0x24,0x08,0x00,0x20,0x54,0x08,0x40,0x00,0xB4,0x3C,0x40,0x00,0x82,0x08,0x00,0x00,0x74,0x04,0x08,0x10,0x22,0x00,0x00,0x00,0x1C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x18,0x60,0x82,0x00,0x40,0x00,0xBC,0x78,0x00,0x00,0x84,0x04,0x00,0x00,0x7A,0x04,0x00,0x00,0x4A,0x04,0x00,0x40,0xBA,0x18,0x20,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x18,0x08,0x10,0x00,0x24,0x00,0x00,0x20,0x58,0x00,0x40,0x00,0xB8,0x38,0x40,0x00,0x84,0x44,0x00,0x00,0xBA,0x44,0x00,0x00,0xBA,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x00,0x04,0x00,0x4A,0x00,0x00,0x08,0x14,0x00,0x10,0x00,0x28,0x00,0x10,0x00,0x28,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x18,0x20,0x44,0x04,0x00,0x40,0xBA,0x04,0x40,0x00,0xBA,0x18,0x20,0x00,0x44,0x04,0x00,0x40,0xBA,0x04,0x40,0x00,0xBA,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x18,0x20,0x44,0x04,0x00,0x40,0xBA,0x04,0x40,0x00,0xBA,0x1C,0x20,0x00,0x42,0x04,0x00,0x00,0x3A,0x00,0x08,0x00,0x34,0x00,0x10,0x20,0x48,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x20,0x40,0x90,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x20,0x40,0x90,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x00,0x04,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x10,0x20,0x48,0x10,0x20,0x00,0x48,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x30,0x00,0x10,0x20,0x48,0x10,0x20,0x00,0x48,0x20,0x40,0x00,0x90,0x00,0x00,0x00,0x60,0x05,
	0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x40,0x00,0xBA,0x04,0x00,0x00,0x4A,0x08,0x00,0x00,0x14,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x40,0x00,0xBA,0x04,0x00,0x00,0x4A,0x08,0x00,0x00,0x14,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x40,0x00,0xBA,0x04,0x00,0x00,0x4A,0x08,0x00,0x00,0x14,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x40,0x00,0xBA,0x04,0x00,0x00,0x4A,0x08,0x00,0x00,0x14,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x00,0x40,0xBA,0x04,0x40,0x10,0xAA,0x0C,0x50,0x00,0xA2,0x1C,0x40,0x00,0xA2,0x40,0x00,0x00,0xBC,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x08,0x10,0x20,0x44,0x04,0x00,0x40,0xBA,0x04,0x00,0x40,0xBA,0x0C,0x70,0x00,0x82,0x04,0x40,0x00,0xBA,0x44,0x00,0x00,0xAA,0x44,0x00,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x78,0x08,0x10,0x60,0x84,0x04,0x00,0x20,0x5A,0x04,0x20,0x00,0x5A,0x28,0x10,0x00,0x44,0x24,0x00,0x00,0x5A,0x24,0x00,0x00,0x5A,0x18,0x20,0x40,0x84,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x18,0x00,0x18,0x00,0x24,0x04,0x00,0x20,0x5A,0x00,0x00,0x40,0xA4,0x00,0x00,0x40,0xA0,0x00,0x40,0x00,0xA4,0x24,0x00,0x00,0x5A,0x18,0x00,0x00,0x24,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x70,0x10,0x20,0x40,0x88,0x08,0x20,0x00,0x54,0x04,0x20,0x00,0x5A,0x04,0x20,0x00,0x5A,0x24,0x00,0x00,0x5A,0x28,0x00,0x00,0x54,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x18,0x60,0x82,0x04,0x20,0x00,0x5A,0x00,0x20,0x00,0x5C,0x28,0x10,0x00,0x44,0x20,0x00,0x00,0x5C,0x20,0x04,0x00,0x5A,0x0C,0x30,0x40,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x18,0x60,0x82,0x04,0x20,0x00,0x5A,0x00,0x20,0x00,0x5C,0x28,0x10,0x00,0x44,0x20,0x00,0x00,0x58,0x20,0x00,0x00,0x50,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x18,0x00,0x18,0x00,0x24,0x04,0x00,0x20,0x5A,0x00,0x40,0x00,0xAC,0x00,0x44,0x08,0xB2,0x44,0x00,0x00,0xAA,0x24,0x00,0x00,0x5A,0x1C,0x00,0x00,0x22,0x00,0x00,0x00,0x1C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x00,0x44,0x00,0xAA,0x04,0x40,0x00,0xBA,0x4C,0x30,0x00,0x82,0x44,0x00,0x00,0xBA,0x44,0x00,0x00,0xAA,0x44,0x00,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x00,0x10,0x00,0x6C,0x10,0x00,0x00,0x28,0x10,0x00,0x00,0x28,0x10,0x00,0x00,0x28,0x10,0x00,0x00,0x6C,0x1C,0x20,0x40,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x0C,0x00,0x04,0x08,0x12,0x00,0x04,0x00,0x0A,0x04,0x00,0x00,0x0A,0x04,0x00,0x00,0x4A,0x04,0x00,0x40,0xAA,0x04,0x40,0x00,0xBA,0x18,0x20,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x64,0x00,0x24,0x40,0x9A,0x00,0x24,0x00,0x5A,0x20,0x08,0x00,0x54,0x20,0x10,0x00,0x48,0x28,0x00,0x00,0x54,0x24,0x00,0x00,0x5A,0x04,0x20,0x40,0x9A,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x70,0x00,0x30,0x40,0x88,0x00,0x20,0x00,0x50,0x00,0x20,0x00,0x50,0x00,0x20,0x00,0x50,0x20,0x00,0x00,0x54,0x24,0x00,0x00,0x5A,0x1C,0x20,0x40,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x00,0x04,0x40,0xAA,0x24,0x40,0x08,0x92,0x04,0x50,0x00,0xAA,0x14,0x40,0x00,0xAA,0x44,0x00,0x00,0xBA,0x44,0x00,0x00,0xAA,0x44,0x00,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x20,0x44,0x00,0x9A,0x14,0x40,0x00,0xAA,0x0C,0x40,0x00,0xB2,0x44,0x00,0x00,0xAA,0x44,0x00,0x00,0xAA,0x44,0x00,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x18,0x20,0x44,0x04,0x00,0x40,0xBA,0x04,0x00,0x40,0xAA,0x04,0x40,0x00,0xAA,0x04,0x40,0x00,0xAA,0x04,0x40,0x00,0xBA,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x78,0x08,0x30,0x40,0x84,0x04,0x20,0x00,0x5A,0x04,0x20,0x00,0x5A,0x28,0x10,0x00,0x44,0x20,0x00,0x00,0x58,0x20,0x00,0x00,0x50,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x08,0x10,0x20,0x44,0x04,0x00,0x40,0xBA,0x04,0x00,0x40,0xAA,0x04,0x40,0x00,0xAA,0x04,0x40,0x00,0xBA,0x44,0x00,0x10,0xAA,0x30,0x08,0x00,0x44,0x04,0x00,0x00,0x3A,0x00,0x00,0x00,0x04,0x07,
	0x00,0x00,0x00,0x78,0x00,0x18,0x60,0x84,0x04,0x20,0x00,0x5A,0x24,0x00,0x00,0x5A,0x28,0x10,0x00,0x44,0x24,0x00,0x00,0x5A,0x24,0x00,0x00,0x5A,0x04,0x20,0x40,0x9A,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x38,0x00,0x18,0x20,0x44,0x04,0x00,0x40,0xBA,0x00,0x40,0x00,0xBC,0x08,0x30,0x00,0x44,0x04,0x00,0x00,0x7A,0x04,0x00,0x40,0xBA,0x08,0x30,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x18,0x60,0x82,0x14,0x40,0x00,0xAA,0x10,0x00,0x00,0x6C,0x10,0x00,0x00,0x28,0x10,0x00,0x00,0x28,0x10,0x00,0x00,0x28,0x08,0x10,0x20,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x00,0x44,0x00,0xAA,0x04,0x40,0x00,0xAA,0x04,0x40,0x00,0xAA,0x44,0x00,0x00,0xAA,0x44,0x00,0x00,0xBA,0x3C,0x00,0x00,0x42,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x00,0x04,0x40,0xAA,0x00,0x44,0x00,0xAA,0x04,0x40,0x00,0xAA,0x28,0x00,0x00,0x54,0x28,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x00,0x04,0x40,0xAA,0x04,0x40,0x00,0xAA,0x04,0x40,0x00,0xBA,0x54,0x00,0x00,0xAA,0x6C,0x00,0x00,0x92,0x44,0x00,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x04,0x40,0x00,0xAA,0x28,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x08,0x20,0x00,0x54,0x04,0x00,0x40,0xAA,0x04,0x40,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x44,0x04,0x00,0x40,0xAA,0x04,0x40,0x00,0xAA,0x04,0x40,0x00,0xBA,0x38,0x00,0x00,0x44,0x10,0x00,0x00,0x28,0x10,0x00,0x00,0x28,0x08,0x10,0x20,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x08,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x20,0x00,0x54,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x08,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x20,0x00,0x54,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x08,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x20,0x00,0x54,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x08,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x20,0x00,0x54,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x08,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x20,0x00,0x54,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x08,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x20,0x00,0x54,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x04,0x40,0x00,0xBA,0x08,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x20,0x00,0x54,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x00,0x28,0x10,0x44,0x04,0x00,0x00,0x3A,0x04,0x08,0x30,0x42,0x04,0x40,0x00,0xBA,0x3C,0x00,0x00,0x42,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x60,0x00,0x20,0x40,0x90,0x00,0x20,0x00,0x58,0x00,0x28,0x00,0x54,0x04,0x20,0x10,0x4A,0x04,0x20,0x00,0x5A,0x04,0x20,0x00,0x5A,0x18,0x20,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x00,0x40,0xBA,0x00,0x40,0x00,0xA4,0x04,0x40,0x00,0xBA,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x0C,0x00,0x04,0x08,0x12,0x08,0x00,0x00,0x34,0x08,0x20,0x00,0x54,0x18,0x40,0x00,0xA4,0x08,0x40,0x00,0xB4,0x08,0x40,0x00,0xB4,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x00,0x08,0x30,0x44,0x04,0x00,0x40,0xBA,0x3C,0x40,0x00,0x82,0x00,0x40,0x00,0xBC,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x18,0x00,0x08,0x10,0x24,0x04,0x00,0x20,0x5A,0x00,0x20,0x00,0x54,0x10,0x20,0x40,0x88,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x30,0x40,0x00,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x34,0x04,0x10,0x20,0x4A,0x08,0x00,0x40,0xB4,0x08,0x40,0x00,0xB4,0x38,0x00,0x00,0x44,0x04,0x00,0x40,0xBA,0x18,0x20,0x00,0x44,0x00,0x00,0x00,0x38,0x07,
	0x00,0x00,0x00,0x60,0x00,0x20,0x40,0x90,0x00,0x20,0x00,0x58,0x20,0x08,0x00,0x54,0x24,0x00,0x10,0x4A,0x24,0x00,0x00,0x5A,0x24,0x00,0x00,0x5A,0x04,0x20,0x40,0x9A,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x50,0x00,0x00,0x00,0x60,0x00,0x40,0x20,0x90,0x00,0x20,0x00,0x50,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x05,
	0x00,0x00,0x00,0x04,0x00,0x00,0x04,0x0A,0x00,0x00,0x00,0x04,0x00,0x00,0x04,0x0A,0x00,0x04,0x00,0x0A,0x04,0x00,0x00,0x0A,0x04,0x00,0x00,0x4A,0x04,0x00,0x40,0xBA,0x18,0x20,0x00,0x44,0x00,0x00,0x00,0x38,0x07,
	0x00,0x00,0x00,0x60,0x00,0x00,0x60,0x90,0x00,0x20,0x00,0x54,0x20,0x04,0x00,0x5A,0x20,0x00,0x08,0x54,0x20,0x10,0x00,0x48,0x28,0x00,0x00,0x54,0x04,0x20,0x40,0x9A,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x60,0x00,0x20,0x40,0x90,0x00,0x20,0x00,0x50,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x20,0x00,0x00,0x50,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x05,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x48,0xB4,0x24,0x48,0x00,0x92,0x44,0x10,0x00,0xAA,0x44,0x00,0x00,0xBA,0x44,0x00,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x08,0x10,0x60,0x84,0x04,0x00,0x40,0xBA,0x04,0x40,0x00,0xAA,0x44,0x00,0x00,0xAA,0x44,0x00,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x08,0x10,0x20,0x44,0x04,0x00,0x40,0xBA,0x04,0x40,0x00,0xAA,0x44,0x00,0x00,0xBA,0x38,0x00,0x00,0x44,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x58,0x00,0x08,0x50,0xA4,0x04,0x20,0x00,0x5A,0x24,0x00,0x00,0x5A,0x38,0x00,0x00,0x44,0x20,0x00,0x00,0x58,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x34,0x00,0x14,0x20,0x4A,0x08,0x40,0x00,0xB4,0x48,0x00,0x00,0xB4,0x38,0x00,0x00,0x44,0x08,0x00,0x00,0x34,0x04,0x08,0x10,0x22,0x00,0x00,0x00,0x1C,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x58,0x00,0x08,0x50,0xA4,0x04,0x20,0x00,0x5A,0x24,0x00,0x00,0x5A,0x20,0x00,0x00,0x54,0x10,0x20,0x40,0x88,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x04,0x18,0x20,0x42,0x00,0x40,0x00,0xBC,0x38,0x00,0x00,0x44,0x04,0x00,0x00,0x7A,0x38,0x00,0x40,0x84,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x50,0x00,0x20,0x00,0x58,0x08,0x30,0x40,0x84,0x20,0x00,0x00,0x58,0x20,0x00,0x00,0x54,0x04,0x20,0x00,0x5A,0x08,0x10,0x00,0x24,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x48,0xB4,0x00,0x48,0x00,0xB4,0x08,0x40,0x00,0xB4,0x48,0x00,0x00,0xB4,0x34,0x00,0x00,0x4A,0x00,0x00,0x00,0x34,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x00,0x44,0x00,0xAA,0x04,0x40,0x00,0xAA,0x28,0x00,0x00,0x54,0x10,0x00,0x00,0x28,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x00,0x44,0x00,0xBA,0x14,0x40,0x00,0xAA,0x2C,0x40,0x00,0x92,0x48,0x00,0x00,0xB4,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x00,0x28,0x00,0x54,0x10,0x00,0x00,0x28,0x08,0x20,0x00,0x54,0x04,0x40,0x00,0xAA,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x00,0x00,0x44,0xAA,0x00,0x44,0x00,0xAA,0x04,0x40,0x00,0xBA,0x38,0x00,0x00,0x44,0x00,0x10,0x00,0x68,0x00,0x20,0x40,0x90,0x00,0x00,0x00,0x60,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x04,0x38,0x40,0x82,0x08,0x40,0x00,0xB4,0x00,0x10,0x00,0x6C,0x04,0x20,0x00,0x5A,0x3C,0x00,0x40,0x82,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x07
};

void AGOSEngine::renderStringAmiga(uint vga_sprite_id, uint color, uint width, uint height, const char *txt) {
	VgaPointersEntry *vpe = &_vgaBufferPointers[2];
	byte *p, *dst, *dst_org, chr;
	uint count;

	if (vga_sprite_id >= 100) {
		vga_sprite_id -= 100;
		vpe++;
	}

	dst = vpe->vgaFile2;

	count = 2000;
	if (vga_sprite_id == 1)
		count *= 2;

	p = dst + vga_sprite_id * 8;
	WRITE_BE_UINT16(p + 4, height);
	WRITE_BE_UINT16(p + 6, width);
	dst += READ_BE_UINT32(p);

	width /= 8;	// convert width from pixels to bytes

	uint charsize = width * height;
	memset(dst, 0, count);
	dst_org = dst;
	int delta = 0;
	while ((chr = *txt++) != 0) {
		int img_width = 1;
		if (chr == 10) {
			dst += width * 10;
			dst_org = dst;
			delta = 0;
		} else if ((signed char)(chr -= '!') < 0) {
			img_width = 7;
		} else {
			const byte *img = simon_agaFont + chr * 41;
			img_width = img[40];
			byte *cur_dst = dst_org;
			for (int row = 0; row < 10; row++) {
				int col = color;
				for (int plane = 0; plane < 3; plane++) {
					chr = img[plane] >> delta;
					if (chr) {
						if (col & 1) cur_dst[charsize * 0] |= chr;
						if (col & 2) cur_dst[charsize * 1] |= chr;
						if (col & 4) cur_dst[charsize * 2] |= chr;
						if (col & 8) cur_dst[charsize * 3] |= chr;
					}
					chr = img[plane] << (8 - delta);
					if (((8 - delta) < img_width) && (chr)) {
						if (col & 1) cur_dst[charsize * 0 + 1] |= chr;
						if (col & 2) cur_dst[charsize * 1 + 1] |= chr;
						if (col & 4) cur_dst[charsize * 2 + 1] |= chr;
						if (col & 8) cur_dst[charsize * 3 + 1] |= chr;
					}
					col++;
				}
				chr = img[3] >> delta;
				if (chr) {
					cur_dst[charsize * 0] |= chr;
					cur_dst[charsize * 1] |= chr;
					cur_dst[charsize * 2] |= chr;
					cur_dst[charsize * 3] |= chr;
				}
				chr = img[3] << (8 - delta);
				if (((8 - delta) < img_width) && (chr)) {
					cur_dst[charsize * 0 + 1] |= chr;
					cur_dst[charsize * 1 + 1] |= chr;
					cur_dst[charsize * 2 + 1] |= chr;
					cur_dst[charsize * 3 + 1] |= chr;
				}
				cur_dst += width;
				img += 4;
			}
		}
		delta += img_width - 1;
		if (delta >= 8) {
			delta -= 8;
			dst_org++;
		}
	}
}

void AGOSEngine::renderString(uint vga_sprite_id, uint color, uint width, uint height, const char *txt) {
	VgaPointersEntry *vpe = &_vgaBufferPointers[2];
	byte *src, *dst, *p, *dst_org, chr;
	const int textHeight = (getGameType() == GType_FF || getGameType() == GType_PP) ? 15: 10;
	uint count = 0;

	if (vga_sprite_id >= 100) {
		vga_sprite_id -= 100;
		vpe++;
	}

	src = dst = vpe->vgaFile2;

	if (getGameType() == GType_FF || getGameType() == GType_PP) {
		if (vga_sprite_id == 1)
			count = 45000;
	} else {
		count = 4000;
		if (vga_sprite_id == 1)
			count *= 2;
	}

	p = dst + vga_sprite_id * 8;

	if (getGameType() == GType_FF || getGameType() == GType_PP) {
		WRITE_LE_UINT16(p + 4, height);
		WRITE_LE_UINT16(p + 6, width);
		// We need to adjust the offset to the next buffer to be right
		// after this one. By default, each buffer is only 9000 bytes
		// long. A two-line string can very well be more than twice
		// that size!
		//
		// The original seems to make an exception for sprite id 1, but
		// even the first conversation option can be a long line. For
		// some reason, I cannot reproduce the text corruption with the
		// original interpreter, though, so maybe we're missing some
		// detail here. Let's hope it's safe to always adjust the
		// buffer size anyway.
		WRITE_LE_UINT16(p + 8, READ_LE_UINT32(p) + width * height);
	} else {
		WRITE_BE_UINT16(p + 4, height);
		WRITE_BE_UINT16(p + 6, width);
	}
	dst += readUint32Wrapper(p);

	if (count != 0)
		memset(dst, 0, count);

	if (_language == Common::HB_ISR)
		dst += width - 1; // For Hebrew, start at the right edge, not the left.

	dst_org = dst;
	while ((chr = *txt++) != 0) {
		if (chr == 10) {
			dst_org += width * textHeight;
			dst = dst_org;
		} else if ((chr -= ' ') == 0) {
			dst += (_language == Common::HB_ISR ? -6 : 6); // Hebrew moves to the left, all others to the right
		} else {
			byte *img_hdr, *img;
			uint i, img_width, img_height;

			if (getGameType() == GType_FF || getGameType() == GType_PP) {
				img_hdr = src + 96 + chr * 8;
				img_height = READ_LE_UINT16(img_hdr + 4);
				img_width = READ_LE_UINT16(img_hdr + 6);
				img = src + READ_LE_UINT32(img_hdr);
			} else {
				img_hdr = src + 48 + chr * 4;
				img_height = img_hdr[2];
				img_width = img_hdr[3];
				img = src + READ_LE_UINT16(img_hdr);
			}

			if (_language == Common::HB_ISR)
				dst -= img_width - 1; // For Hebrew, move from right edge to left edge of image.
			byte *cur_dst = dst;

			// Occurs in Amiga and Macintosh ports of The Feeble Files, when
			// special characters are used by French/German/Spanish versions.
			// Due to the English image data, been used by all languages.
			if (img_width == 0 || img_height == 0)
				continue;

			assert(img_width < 50 && img_height < 50);

			do {
				for (i = 0; i != img_width; i++) {
					chr = *img++;
					if (chr) {
						if (chr == 0xF)
							chr = 207;
						else
							chr += color;
						cur_dst[i] = chr;
					}
				}
				cur_dst += width;
			} while (--img_height);

			if (_language != Common::HB_ISR) // Hebrew character movement is done higher up
				dst += img_width - 1;
		}
	}
}

void AGOSEngine::showMessageFormat(const char *s, ...) {
	char buf[STRINGBUFLEN];
	char *str;
	va_list va;

	va_start(va, s);
	vsnprintf(buf, STRINGBUFLEN, s, va);
	va_end(va);

	if (!_fcsData1[_curWindow]) {
		if (getGameType() == GType_ELVIRA1 || getGameType() == GType_ELVIRA2 || getGameType() == GType_WW) {
			if (_showMessageFlag) {
				if (_windowArray[_curWindow]->flags & 128) {
					haltAnimation();
				}
			}
		}
		openTextWindow();
		if (!_showMessageFlag) {
			_windowArray[0] = _textWindow;
			justifyStart();
		}
		_showMessageFlag = true;
		_fcsData1[_curWindow] = 1;
	}

	for (str = buf; *str; str++)
		justifyOutPut(*str);
}

void AGOSEngine::justifyStart() {
	if (getGameType() == GType_FF || getGameType() == GType_PP) {
		_printCharCurPos = _textWindow->textColumn;
		_printCharMaxPos = _textWindow->width;
	} else {
		_printCharCurPos = _textWindow->textLength;
		_printCharMaxPos = _textWindow->textMaxLength;
	}
	_printCharPixelCount = 0;
	_numLettersToPrint = 0;
	_newLines = 0;
}

void AGOSEngine::justifyOutPut(byte chr) {
	if (chr == 12) {
		_numLettersToPrint = 0;
		_printCharCurPos = 0;
		_printCharPixelCount = 0;
		doOutput(&chr, 1);
		clsCheck(_textWindow);
	} else if (chr == 0 || chr == ' ' || chr == 10) {
		bool fit;

		if (getGameType() == GType_FF || getGameType() == GType_PP) {
			fit = _printCharMaxPos - _printCharCurPos > _printCharPixelCount;
		} else {
			fit = _printCharMaxPos - _printCharCurPos >= _printCharPixelCount;
		}

		if (fit) {
			_printCharCurPos += _printCharPixelCount;
			doOutput(_lettersToPrintBuf, _numLettersToPrint);

			if (_printCharCurPos == _printCharMaxPos) {
				_printCharCurPos = 0;
			} else {
				if (chr)
					doOutput(&chr, 1);
				if (chr == 10)
					_printCharCurPos = 0;
				else if (chr != 0)
					_printCharCurPos += (getGameType() == GType_FF || getGameType() == GType_PP) ? getFeebleFontSize(chr) : 1;
			}
		} else {
			const byte newline_character = 10;
			_printCharCurPos = _printCharPixelCount;
			doOutput(&newline_character, 1);
			doOutput(_lettersToPrintBuf, _numLettersToPrint);
			if (chr == ' ') {
				doOutput(&chr, 1);
				_printCharCurPos += (getGameType() == GType_FF || getGameType() == GType_PP) ? getFeebleFontSize(chr) : 1;
			} else {
				doOutput(&chr, 1);
				_printCharCurPos = 0;
			}
		}
		_numLettersToPrint = 0;
		_printCharPixelCount = 0;
	} else {
		_lettersToPrintBuf[_numLettersToPrint++] = chr;
		_printCharPixelCount += (getGameType() == GType_FF || getGameType() == GType_PP) ? getFeebleFontSize(chr) : 1;
	}
}

void AGOSEngine::openTextWindow() {
	if (_textWindow) {
		if (getGameType() == GType_ELVIRA1 || getGameType() == GType_ELVIRA2 || getGameType() == GType_WW) {
			if (_textWindow->flags & 0x80)
				clearWindow(_textWindow);
		}
		return;
	}

	if (getGameType() == GType_FF || getGameType() == GType_PP)
		_textWindow = openWindow(64, 96, 384, 172, 1, 0, 15);
	else
		_textWindow = openWindow(8, 144, 24, 6, 1, 0, 15);
}

void AGOSEngine::windowPutChar(WindowBlock *window, byte c, byte b) {
	byte width = 6;

	if (c == 12) {
		clearWindow(window);
	} else if (c == 13 || c == 10) {
		windowNewLine(window);
	} else if ((c == 1 && _language != Common::HB_ISR) || (c == 8)) {
		if (_language == Common::HB_ISR) {
			if (b >= 64 && b < 91)
				width = _hebrewCharWidths [b - 64];

			if (window->textLength != 0) {
				window->textLength--;
				window->textColumnOffset += width;
				if (window->textColumnOffset >= 8) {
					window->textColumnOffset -= 8;
					window->textColumn--;
				}
			}
		} else {
			int8 val = (c == 8) ? 6 : 4;

			if (window->textLength != 0) {
				window->textLength--;
				window->textColumnOffset -= val;
				if ((int8)window->textColumnOffset < val) {
					window->textColumnOffset += 8;
					window->textColumn--;
				}
			}
		}
	} else if (c >= 32) {
		if (getGameType() == GType_FF || getGameType() == GType_PP) {
			// Ignore invalid characters
			if (c - 32 > 195)
				return;

			windowDrawChar(window, window->textColumn + window->x, window->textRow + window->y, c);
			window->textColumn += getFeebleFontSize(c);
			return;
		}

		// Ignore invalid characters
		if (c - 32 > 98)
			return;

		if (window->textLength == window->textMaxLength) {
			windowNewLine(window);
		} else if (window->textRow == window->height) {
			windowNewLine(window);
			window->textRow--;
		}

		if (_language == Common::HB_ISR) {
			if (c >= 64 && c < 91)
				width = _hebrewCharWidths [c - 64];
			window->textColumnOffset -= width;
			if (window->textColumnOffset >= width) {
				window->textColumnOffset += 8;
				window->textColumn++;
			}
			windowDrawChar(window, (window->width + window->x - window->textColumn) * 8, window->textRow * 8 + window->y, c);
			window->textLength++;
		} else {
			windowDrawChar(window, (window->textColumn + window->x) * 8, window->textRow * 8 + window->y, c);

			window->textLength++;
			window->textColumnOffset += 6;
			if (getGameType() == GType_SIMON1 || getGameType() == GType_SIMON2) {
				if (c == 'i' || c == 'l')
					window->textColumnOffset -= 2;
			}
			if (window->textColumnOffset >= 8) {
				window->textColumnOffset -= 8;
				window->textColumn++;
			}
		}
	}
}

void AGOSEngine_Feeble::windowNewLine(WindowBlock *window) {
	if (_noOracleScroll == 0) {
		if (window->height < window->textRow + 30) {
			if (!getBitFlag(94)) {
				_noOracleScroll = 1;
				if (getBitFlag(92)) {
					_noOracleScroll = 0;
					checkLinkBox();
					scrollOracle();
					linksUp();
					window->scrollY++;
					_oracleMaxScrollY++;
				} else {
					_oracleMaxScrollY++;
					checkLinkBox();
				}
			}
		} else {
			window->textRow += 15;
			checkLinkBox();
		}
	} else {
		_oracleMaxScrollY++;
		checkLinkBox();
	}

	window->textColumn = 0;
	window->textColumnOffset = 0;
	window->textLength = 0;
}

void AGOSEngine::windowNewLine(WindowBlock *window) {
	window->textColumn = 0;
	window->textColumnOffset = (getGameType() == GType_ELVIRA2) ? 4 : 0;
	window->textLength = 0;

	if (window->textRow == window->height) {
		if (getGameType() == GType_ELVIRA1 || getGameType() == GType_ELVIRA2 ||
			getGameType() == GType_WW) {
			windowScroll(window);
		}
	} else {
		window->textRow++;
	}
}

void AGOSEngine::windowScroll(WindowBlock *window) {
	_lockWord |= 0x8000;

	if (window->height != 1) {
		Graphics::Surface *screen = _system->lockScreen();

		byte *src, *dst;
		uint16 w, h;

		w = window->width * 8;
		h = (window->height -1) * 8;

		dst = (byte *)screen->pixels + window->y * _screenWidth + window->x * 8;
		src = dst + 8 * _screenWidth;

		do {
			memcpy(dst, src, w);
			src += _screenWidth;
			dst += _screenWidth;
		} while (--h);

		_system->unlockScreen();
	}

	colorBlock(window, window->x * 8, (window->height - 1) * 8 + window->y, window->width * 8, 8);

	_lockWord &= ~0x8000;
}
} // End of namespace AGOS

