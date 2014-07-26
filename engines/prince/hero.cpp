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
 */
#include "common/debug.h"
#include "common/random.h"

#include "prince/hero.h"
#include "prince/hero_set.h"
#include "prince/animation.h"
#include "prince/resource.h"
#include "prince/prince.h"
#include "prince/graphics.h"
#include "prince/flags.h"
#include "prince/script.h"

namespace Prince {

Hero::Hero(PrinceEngine *vm, GraphicsMan *graph) : _vm(vm), _graph(graph)
	, _number(0), _visible(false), _state(kHeroStateStay), _middleX(0), _middleY(0)
	, _boreNum(1), _currHeight(0), _moveDelay(0), _shadMinus(0), _moveSetType(0)
	, _lastDirection(kHeroDirDown), _destDirection(kHeroDirDown), _talkTime(0), _boredomTime(0), _phase(0)
	, _specAnim(nullptr), _drawX(0), _drawY(0), _drawZ(0), _zoomFactor(0), _scaleValue(0)
	, _shadZoomFactor(0), _shadScaleValue(0), _shadLineLen(0), _shadDrawX(0), _shadDrawY(0)
	, _frameXSize(0), _frameYSize(0), _scaledFrameXSize(0), _scaledFrameYSize(0), _color(0)
	, _coords(nullptr), _dirTab(nullptr), _currCoords(nullptr), _currDirTab(nullptr), _step(0)
	, _maxBoredom(200), _turnAnim(0), _leftRightMainDir(0), _upDownMainDir(0)
{
	_zoomBitmap = (byte *)malloc(kZoomBitmapLen);
	_shadowBitmap = (byte *)malloc(2 * kShadowBitmapSize);
	_shadowLine = new byte[kShadowLineArraySize];
}

Hero::~Hero() {
	free(_zoomBitmap);
	free(_shadowBitmap);
	delete[] _shadowLine;
	freeHeroAnim();
	freeOldMove();
}

bool Hero::loadAnimSet(uint32 animSetNr) {
	if (animSetNr > sizeof(heroSetTable)) {
		return false;
	}

	_shadMinus = heroSetBack[animSetNr];

	for (uint32 i = 0; i < _moveSet.size(); i++) {
		delete _moveSet[i];
	}

	const HeroSetAnimNames &animSet = *heroSetTable[animSetNr];

	_moveSet.resize(kMoveSetSize);
	for (uint32 i = 0; i < kMoveSetSize; ++i) {
		debug("Anim set item %d %s", i, animSet[i]);
		Animation *anim = NULL;
		if (animSet[i] != NULL) {
			anim = new Animation();
			Resource::loadResource(anim, animSet[i], true);
		}
		_moveSet[i] = anim;
	}

	return true;
}

Graphics::Surface *Hero::getSurface() {
	Animation *heroAnim = nullptr;
	if (_specAnim != nullptr) {
		heroAnim = _specAnim;
	} else {
		heroAnim = _moveSet[_moveSetType];
	}

	if (heroAnim != nullptr) {
		int16 phaseFrameIndex = heroAnim->getPhaseFrameIndex(_phase);
		Graphics::Surface *heroFrame = heroAnim->getFrame(phaseFrameIndex);
		return heroFrame;
	}
	return NULL;
}

uint16 Hero::getData(AttrId dataId) {
	switch (dataId) {
	case kHeroLastDir:
		return _lastDirection;
	case kHeroAnimSet:
		return _moveSetType;
	default:
		assert(false);
		return 0;
	}
}

int Hero::getScaledValue(int size) {
	int newSize = 0;
	int16 initScaleValue = _scaleValue;
	if (_scaleValue != 10000) {
		for(int i = 0; i < size; i++) {
			initScaleValue -= 100;
			if(initScaleValue >= 0) {
				newSize++;
			} else {
				initScaleValue += _scaleValue;
			}
		}
		return newSize;
	} else {
		return size;
	}
}

Graphics::Surface *Hero::zoomSprite(Graphics::Surface *heroFrame) {
	Graphics::Surface *zoomedFrame = new Graphics::Surface();
	zoomedFrame->create(_scaledFrameXSize, _scaledFrameYSize, Graphics::PixelFormat::createFormatCLUT8());
	
	int sprZoomX;
	int sprZoomY = _scaleValue;
	uint xSource = 0;
	uint ySource = 0;
	uint xDest = 0;
	uint yDest = 0;

	for (int i = 0; i < _scaledFrameYSize; i++) {
		// linear_loop:
		while(1) {
			sprZoomY -= 100;
			if (sprZoomY >= 0 || _scaleValue == 10000) {
				// all_r_y
				sprZoomX = _scaleValue;
				break; // to loop_lin
			} else {
				sprZoomY += _scaleValue;
				xSource = 0;
				ySource++;
			}
		}
		// loop_lin:
		for (int j = 0; j < _scaledFrameXSize; j++) {
			sprZoomX -= 100;
			if (sprZoomX >= 0) {
				// its_all_r
				memcpy(zoomedFrame->getBasePtr(xDest, yDest), heroFrame->getBasePtr(xSource, ySource), 1);
				xDest++;
			} else {
				sprZoomX += _scaleValue;
				j--;
			}
			xSource++;
		}
		xDest = 0;
		yDest++;
		xSource = 0;
		ySource++;
	}
	return zoomedFrame;
}

void Hero::countDrawPosition() {
	Animation *heroAnim = nullptr;
	if (_specAnim != nullptr) {
		heroAnim = _specAnim;
	} else {
		heroAnim = _moveSet[_moveSetType];
	}
	if (heroAnim != nullptr) {
		int16 tempMiddleY;
		int16 baseX = heroAnim->getBaseX();
		int16 baseY = heroAnim->getBaseY();
		// any chance?
		if (baseX == 320) {
			tempMiddleY = _middleY - (baseY - 240);
		} else {
			tempMiddleY = _middleY;
		}
		int phaseFrameIndex = heroAnim->getPhaseFrameIndex(_phase);
		_frameXSize = heroAnim->getFrameWidth(phaseFrameIndex);
		_frameYSize = heroAnim->getFrameHeight(phaseFrameIndex);
		_scaledFrameXSize = getScaledValue(_frameXSize);
		_scaledFrameYSize = getScaledValue(_frameYSize);

		// any use of this?
		if (!heroAnim->testId()) {
			error("countDrawPosition - !heroAnim->testId()");
			//int diffX = heroAnim->getIdXDiff();
			//int diffY = heroAnim->getIdYDiff();
		}

		if (_zoomFactor != 0) {
			//notfullSize
			_drawX = _middleX - _scaledFrameXSize / 2;
			_drawY = tempMiddleY + 1 - _scaledFrameYSize;
			_vm->checkMasks(_drawX, _drawY - 1, _scaledFrameXSize, _scaledFrameYSize, _middleY);
		} else {
			//fullSize
			_drawX = _middleX - _frameXSize / 2;
			_drawY = tempMiddleY + 1 - _frameYSize;
			_vm->checkMasks(_drawX, _drawY - 1, _frameXSize, _frameYSize, _middleY);
		}

		_drawZ = tempMiddleY;
	}
}

void Hero::plotPoint(int x, int y) {
	WRITE_UINT16(&_shadowLine[_shadLineLen * 4], x);
	WRITE_UINT16(&_shadowLine[_shadLineLen * 4 + 2], y);
}

static void plot(int x, int y, int color, void *data) {
	Hero *shadowLine = (Hero *)data;
	shadowLine->plotPoint(x, y);
	shadowLine->_shadLineLen++;
}

void Hero::showHeroShadow(Graphics::Surface *heroFrame) {
	Graphics::Surface *makeShadow = new Graphics::Surface();
	makeShadow->create(_frameXSize, _frameYSize, Graphics::PixelFormat::createFormatCLUT8());

	for (int y = 0; y < _frameYSize; y++) {
		byte *src = (byte *)heroFrame->getBasePtr(0, y);
		byte *dst = (byte *)makeShadow->getBasePtr(0, y);

		for (int x = 0; x < _frameXSize; x++, dst++, src++) {
			if (*src != 0xFF) {
				*dst = _graph->kShadowColor;
			} else {
				*dst = *src;
			}
		}
	}

	int destX = _middleX - _scaledFrameXSize / 2;
	int destY = _middleY - _shadMinus - 1;

	if (destY > 1 && destY < kMaxPicHeight) {
		int shadDirection;
		if (_lightY > destY) {
			shadDirection = 1;
		} else {
			shadDirection = 0;
		}

		_shadLineLen = 0;
		Graphics::drawLine(_lightX, _lightY, destX, destY, 0, &plot, this);

		byte *sprShadow = (byte *)_graph->_shadowTable70;

		_shadDrawX = destX - _vm->_picWindowX;
		_shadDrawY = destY - _vm->_picWindowY;

		int shadPosX = _shadDrawX;
		int shadPosY = _shadDrawY;
		int shadBitAddr = destY * kMaxPicWidth / 8 + destX / 8;
		int shadBitMask = 128 >> (destX % 8);

		int shadZoomY2 = _shadScaleValue;
		int shadZoomY = _scaleValue;

		int diffX = 0;
		int diffY = 0;

		int shadowHeroX = 0;
		int shadowHeroY = _frameYSize - 1;

		int shadLastY = 0;

		byte *shadowHero = (byte *)makeShadow->getBasePtr(shadowHeroX, shadowHeroY); // first pixel from last row of shadow hero
		byte *background = (byte *)_graph->_frontScreen->getBasePtr(_shadDrawX, _shadDrawY); // pixel of background where shadow sprite starts

		// banked2
		byte *shadowLineStart = _shadowLine + 8;

		int shadWallDown = 0;
		int shadWallBitAddr = 0;
		int shadWallBitMask = 0;
		byte *shadWallDestAddr = 0;
		int shadWallPosY = 0;
		int shadWallSkipX = 0;
		int shadWallModulo = 0;

		// linear_loop
		for (int i = 0; i < _frameYSize; i++) {
			int shadSkipX = 0;
			int ct_loop = 0;
			int sprModulo = 0;

			int j;
			//retry_line:
			for (j = _frameYSize - i; j > 0; j--) {
				shadZoomY -= 100;
				if (shadZoomY < 0 && _scaleValue != 10000) {
					shadZoomY += _scaleValue;
					shadowHeroY--;
					if (shadowHeroY < 0) {
						break;
					}
					shadowHero = (byte *)makeShadow->getBasePtr(shadowHeroX, shadowHeroY);
				} else {
					break;
				}
			}
			if (j == 0) {
				break;
			}
			if (shadowHeroY < 0) {
				break;
			}
			//line_y_ok
			if (shadLastY != shadPosY && shadPosY >= 0 && shadPosY < 480 && shadPosX < 640) {
				shadLastY = shadPosY;
				if (shadPosX < 0) {
					shadSkipX = -1 * shadPosX;
					background += shadSkipX;
					if (_frameXSize > shadSkipX) {
						shadowHero += shadSkipX;
						shadBitAddr += shadSkipX / 8;
						if ((shadSkipX % 8) != 0) {
							//loop_rotate:
							for (int a = 0; a < (shadSkipX % 8); a++) {
								if (shadBitMask == 1) {
									shadBitMask = 128;
									shadBitAddr++;
								} else {
									shadBitMask >>= 1;
								}
							}
						}
					} else {
						//skip_line
						//test it
					}
				} else {
					//x1_ok
					if (shadPosX + _frameXSize > 640) {
						ct_loop = 640 - shadPosX; // test it
						sprModulo = shadPosX + _frameXSize - 640;
					} else {
						//draw_line
						ct_loop = _frameXSize;
					}
				}
				//draw_line1
				//retry_line2
				int k;
				for (k = j; k > 0; k--) {
					shadZoomY2 -= 100;
					if (shadZoomY2 < 0 && _shadScaleValue != 10000) {
						shadZoomY2 += _shadScaleValue;
						shadowHeroY--;
						if (shadowHeroY < 0) {
							break;
						}
						shadowHero = (byte *)makeShadow->getBasePtr(shadowHeroX, shadowHeroY);
					} else {
						break;
					}
				}
				if (shadowHeroY < 0) {
					break;
				}
				if (k == 0) {
					break;
				}
				//line_y_ok_2:
				//copy_trans
				bool shadWDFlag = false;
				int shadZoomX = _scaleValue;
				int backgroundDiff = 0;
				int shadBitMaskCopyTrans = shadBitMask;
				int shadBitAddrCopyTrans = shadBitAddr;
				//ct_loop:
				for (int l = 0; l < ct_loop; l++) {
					shadZoomX -= 100;
					if (shadZoomX < 0 && _scaleValue != 10000) {
						shadZoomX += _scaleValue;
					} else {
						if (*shadowHero == _graph->kShadowColor) {
							if ((shadBitMaskCopyTrans & _shadowBitmap[shadBitAddrCopyTrans]) != 0) {
								if (shadWallDown == 0) {
									if ((shadBitMaskCopyTrans & _shadowBitmap[shadBitAddrCopyTrans + kShadowBitmapSize]) != 0) {
										shadWDFlag = true;
										//shadow
										*background = *(sprShadow + *background);
									}
								}
							} else {
								//shadow
								*background = *(sprShadow + *background);
							}
						}
						//ct_next
						if (shadBitMaskCopyTrans == 1) {
							shadBitMaskCopyTrans = 128;
							shadBitAddrCopyTrans++;
						} else {
							shadBitMaskCopyTrans >>= 1;
						}
						//okok
						backgroundDiff++;
						background = (byte *)_graph->_frontScreen->getBasePtr(_shadDrawX + diffX + backgroundDiff, _shadDrawY + diffY);
					}
					shadowHeroX++;
					shadowHero = (byte *)makeShadow->getBasePtr(shadowHeroX, shadowHeroY);
				}
				//byebyebye
				if (!shadWallDown && shadWDFlag) {
					shadWallDown = shadPosX;
					shadWallBitAddr = shadBitAddr;
					shadWallDestAddr = (byte *)_graph->_frontScreen->getBasePtr(_shadDrawX + diffX, _shadDrawY + diffY);
					shadWallBitMask = shadBitMask;
					shadWallPosY = shadPosY;
					shadWallSkipX = shadSkipX;
					shadWallModulo = sprModulo;
				}
				//byebye
				if (shadDirection != 0 && shadWallDown != 0) {
					int shadBitMaskWallCopyTrans = shadWallBitMask;
					int shadBitAddrWallCopyTrans = shadWallBitAddr;
					background = shadWallDestAddr;
					shadowHero = (byte *)makeShadow->getBasePtr(shadWallSkipX, shadowHeroY);

					if (ct_loop > shadWallSkipX && ct_loop - shadWallSkipX > shadWallModulo) {
						//WALL_copy_trans
						shadWDFlag = false;
						int shadZoomXWall = _scaleValue;
						int backgroundDiffWall = 0;
						int shadowHeroXWall = 0;
						//ct_loop:
						for (int m = 0; m < ct_loop; m++) {
							shadZoomXWall -= 100;
							if (shadZoomXWall < 0 && _scaleValue != 10000) {
								shadZoomXWall += _scaleValue;
							} else {
								//point_ok:
								if (*shadowHero == _graph->kShadowColor) {
									if ((shadBitMaskWallCopyTrans & _shadowBitmap[shadBitAddrWallCopyTrans + kShadowBitmapSize]) != 0) {
										*background = *(sprShadow + *background);
									}
								}
								//ct_next
								if (shadBitMaskWallCopyTrans == 1) {
									shadBitMaskWallCopyTrans = 128;
									shadBitAddrWallCopyTrans++;
								} else {
									shadBitMaskWallCopyTrans >>= 1;
								}
								//okok
								backgroundDiffWall++;
								background = shadWallDestAddr + backgroundDiffWall;
							}
							shadowHeroXWall++;
							shadowHero = (byte *)makeShadow->getBasePtr(shadWallSkipX + shadowHeroXWall, shadowHeroY);
						}
					}
					//krap2
					shadWallDestAddr -= kScreenWidth;
					shadWallBitAddr -= kMaxPicWidth / 8;
					shadWallPosY--;
				}
			}
			//skip_line
			//next_line
			if (*(shadowLineStart + 2) < *(shadowLineStart - 2)) {
				//minus_y
				shadBitAddr -= kMaxPicWidth / 8;
				shadPosY--;
				diffY--;
			} else if (*(shadowLineStart + 2) > *(shadowLineStart - 2)) {
				shadBitAddr += kMaxPicWidth / 8;
				shadPosY++;
				diffY++;
			}
			//no_change_y
			if (*shadowLineStart < *(shadowLineStart - 4)) {
				//minus_x
				shadPosX--;
				//rol
				if (shadBitMask == 128) {
					shadBitMask = 1;
					shadBitAddr--;
				} else {
					shadBitMask <<= 1;
				}
				diffX--;
			} else if (*shadowLineStart > *(shadowLineStart - 4)) {
				shadPosX++;
				//ror
				if (shadBitMask == 1) {
					shadBitMask = 128;
					shadBitAddr++;
				} else {
					shadBitMask >>= 1;
				}
				diffX++;
			}
			//no_change_x
			shadowLineStart += 4;
			shadowHeroY--;
			if (shadowHeroY < 0) {
				break;
			}
			shadowHeroX = 0;
			background = (byte *)_graph->_frontScreen->getBasePtr(_shadDrawX + diffX, _shadDrawY + diffY);
			shadowHero = (byte *)makeShadow->getBasePtr(shadowHeroX, shadowHeroY);
		}
		//koniec_bajki - end_of_a_story
	}
	makeShadow->free();
	delete makeShadow;
}

void Hero::setScale(int8 zoomBitmapValue) {
	if (zoomBitmapValue == 0) {
		_zoomFactor = 0;
		_scaleValue = 10000;
	} else {
		_zoomFactor = zoomBitmapValue;
		_scaleValue = 10000 / _zoomFactor;
	}
}

void Hero::selectZoom() {
	int8 zoomBitmapValue = *(_zoomBitmap + _middleY / 4 * kZoomBitmapWidth + _middleX / 4);
	setScale(zoomBitmapValue);
}

void Hero::setShadowScale(int32 shadowScale) {
	shadowScale = 100 - shadowScale;
	if (shadowScale == 0) {
		_shadZoomFactor = 0;
		_shadScaleValue = 10000;
	} else {
		_shadZoomFactor = shadowScale;
		_shadScaleValue = 10000 / _shadZoomFactor;
	}
}

int Hero::rotateHero(int oldDirection, int newDirection) {
	switch (oldDirection) {
	case kHeroDirLeft:
		switch (newDirection) {
		case kHeroDirRight:
			return kMove_MLR;
		case kHeroDirUp:
			return kMove_MLU;
		case kHeroDirDown:
			return kMove_MLD;
		}
		break;
	case kHeroDirRight:
		switch (newDirection) {
		case kHeroDirLeft:
			return kMove_MRL;
		case kHeroDirUp:
			return kMove_MRU;
		case kHeroDirDown:
			return kMove_MRD;
		}
		break;
	case kHeroDirUp:
		switch (newDirection) {
		case kHeroDirLeft:
			return kMove_MUL;
		case kHeroDirRight:
			return kMove_MUR;
		case kHeroDirDown:
			return kMove_MUD;
		}
		break;
	case kHeroDirDown:
		switch (newDirection) {
		case kHeroDirLeft:
			return kMove_MDL;
		case kHeroDirRight:
			return kMove_MDR;
		case kHeroDirUp:
			return kMove_MDU;
		}
		break;
	}
	error("rotateHero - wrong directions - old %d, new %d", oldDirection, newDirection);
}

void Hero::heroStanding() {
	_phase = 0;
	switch (_lastDirection) {
	case kHeroDirLeft:
		_moveSetType = kMove_SL;
		break;
	case kHeroDirRight:
		_moveSetType = kMove_SR;
		break;
	case kHeroDirUp:
		_moveSetType = kMove_SU;
		break;
	case kHeroDirDown:
		_moveSetType = kMove_SD;
		break;
	}
}

void Hero::showHero() {
	if (_visible && !_vm->_flags->getFlagValue(Flags::NOHEROATALL)) {

		if (_talkTime != 0) {
			_talkTime--;
		}

		// Scale of hero
		selectZoom();

		if (_state != kHeroStateStay) {
			_boredomTime = 0;
		}

		if (_state == kHeroStateSpec) {
			if (_specAnim != nullptr) {
				if (_phase < _specAnim->getPhaseCount() - 1) {
					_phase++;
				} else {
					_phase = 0;
					freeHeroAnim();
					if (!_talkTime) {
						_state = kHeroStateStay;
					} else {
						_state = kHeroStateTalk;
					}
				}
			} else {
				_state = kHeroStateStay;
			}
		}

		if (_state == kHeroStateTalk) {
			if (_talkTime) {
				switch (_lastDirection) {
				case kHeroDirLeft:
					_moveSetType = kMove_TL;
					break;
				case kHeroDirRight:
					_moveSetType = kMove_TR;
					break;
				case kHeroDirUp:
					_moveSetType = kMove_TU;
					break;
				case kHeroDirDown:
					_moveSetType = kMove_TD;
					break;
				}
				if (_phase < _moveSet[_moveSetType]->getPhaseCount() - 1) {
					_phase++;
				} else {
					_phase = _moveSet[_moveSetType]->getLoopCount();
				}
			} else {
				_state = kHeroStateStay;
			}
		}

		if (_state == kHeroStateBore) {
			switch (_boreNum) {
			case 0:
				_moveSetType = kMove_BORED1;
				break;
			case 1:
				_moveSetType = kMove_BORED2;
				break;
			}
			if (_moveSet[_moveSetType] != nullptr) {
				if (_phase < _moveSet[_moveSetType]->getPhaseCount() - 1) {
					_phase++;
				} else {
					_phase = 0;
					_lastDirection = kHeroDirDown;
					_state = kHeroStateStay;
				}
			} else {
				_state = kHeroStateStay;
			}
		}

		if (_state == kHeroStateStay) {
			if (!_vm->_optionsFlag) {
				if (!_vm->_interpreter->getLastOPCode() || !_vm->_interpreter->getFgOpcodePC()) {
					_boredomTime++;
					if (_boredomTime == _maxBoredom) {
						_boreNum =_vm->_randomSource.getRandomNumber(1); // rand one of two 'bored' animation
						_phase = 0;
						_state = kHeroStateBore;
						if (_lastDirection == kHeroDirUp) {
							_lastDirection = kHeroDirLeft;
						} else {
							_lastDirection = kHeroDirDown;
						}
					}
				} else {
					_boredomTime = 0;
				}
			} else {
				_boredomTime = 0;
			}
			heroStanding();
		}

		if (_state == kHeroStateTurn) {
			if (_destDirection && (_lastDirection != _destDirection)) {
				_phase = 0;
				int rotateDir = rotateHero(_lastDirection, _destDirection);
				_lastDirection = _destDirection;
				if (rotateDir) {
					_turnAnim = rotateDir;
					_state = kHeroStateTran;
				} else {
					_state = kHeroStateStay;
					heroStanding();
				}
			} else {
				_state = kHeroStateStay;
				heroStanding();
			}
		}

		if (_state == kHeroStateTran) {
			if (_moveSet[_turnAnim] != nullptr) {
				// only in bear form
				_moveSetType = _turnAnim;
				if (_phase < _moveSet[_moveSetType]->getPhaseCount() - 2) {
					_phase += 2;
				} else {
					_state = kHeroStateStay;
					heroStanding();
				}
			} else {
				_state = kHeroStateStay;
				heroStanding();
			}
		}

		if (_state == kHeroStateMvan) {
			if (_moveSet[_turnAnim] != nullptr) {
				// only in bear form
				_moveSetType = _turnAnim;
				if (_phase < _moveSet[_moveSetType]->getPhaseCount() - 2) {
					_phase += 2;
				} else {
					_state = kHeroStateMove;
				}
			} else {
				_state = kHeroStateMove;
			}
		}

		if (_state == kHeroStateDelayMove) {
			_moveDelay--;
			if (!_moveDelay) {
				_state = kHeroStateMove;
			}
		}

		int x, y, dir;

		if (_state == kHeroStateMove) {
			//go_for_it:
			while (1) {
				if (_currCoords != nullptr) {
					if (READ_UINT32(_currCoords) != 0xFFFFFFFF) {
						x = READ_UINT16(_currCoords);
						y = READ_UINT16(_currCoords + 2);
						_currCoords += 4;
						dir = *_currDirTab;
						_currDirTab++;
						if (_lastDirection != dir) {
							_phase = 0;
							int rotateDir = rotateHero(_lastDirection, dir);
							_lastDirection = dir;
							if (!rotateDir) {
								continue;
							} else {
								_turnAnim = rotateDir;
								_state = kHeroStateMvan;
								if (_moveSet[_turnAnim] != nullptr) {
									// only in bear form
									_moveSetType = _turnAnim;
									if (_phase < _moveSet[_moveSetType]->getPhaseCount() - 2) {
										_phase += 2;
										break;
									} else {
										_turnAnim = 0;
										_state = kHeroStateMove;
										continue;
									}
								} else {
									_state = kHeroStateMove;
									continue;
								}
							}
						}
						//no_need_direction_change
						if (dir == kHeroDirLeft) {
							if (_middleX - x >= _step) {
								heroMoveGotIt(x, y, dir);
								break;
							}
						} else if (dir == kHeroDirRight) {
							if (x - _middleX >= _step) {
								heroMoveGotIt(x, y, dir);
								break;
							}
						} else if (dir == kHeroDirUp) {
							if (_middleY - y >= _step) {
								heroMoveGotIt(x, y, dir);
								break;
							}
						} else if (dir == kHeroDirDown) {
							if (y - _middleY >= _step) {
								heroMoveGotIt(x, y, dir);
								break;
							}
						}
					} else {
						//finito
						_middleX = READ_UINT16(_currCoords - 4);
						_middleY = READ_UINT16(_currCoords - 2);
						selectZoom();

						if (_coords != nullptr) {
							free(_coords);
							_coords = nullptr;
							_currCoords = nullptr;
						}

						if (_dirTab != nullptr) {
							free(_dirTab);
							_dirTab = nullptr;
							_currDirTab = nullptr;
						}

						_boredomTime = 0;
						_phase = 0;
						_state = kHeroStateTurn;

						if (!_destDirection) {
							_destDirection = _lastDirection;
						}

						heroStanding();

						break;
					}
				} else {
					heroStanding();
					break;
				}
			}
		}

		countDrawPosition();

	}
}

void Hero::heroMoveGotIt(int x, int y, int dir) {
	_middleX = x;
	_middleY = y;
	selectZoom();

	switch (dir) {
	case kHeroDirLeft:
		_moveSetType = kMove_ML;
		break;
	case kHeroDirRight:
		_moveSetType = kMove_MR;
		break;
	case kHeroDirUp:
		_moveSetType = kMove_MU;
		break;
	case kHeroDirDown:
		_moveSetType = kMove_MD;
		break;
	}

	if (_vm->_flags->getFlagValue(Flags::HEROFAST) || _state == kHeroStateRun) {
		if (_phase < _moveSet[_moveSetType]->getPhaseCount() - 2) {
			_phase += 2;
		} else {
			_phase = 0;
		}
	} else {
		if (_phase < _moveSet[_moveSetType]->getPhaseCount() - 1) {
			_phase++;
		} else {
			_phase = 0;
		}
	}

	_step = kStepLeftRight;
	if (_moveSetType == kMove_MU || _moveSetType == kMove_MD) {
		_step = kStepUpDown;
	}
	if (_vm->_flags->getFlagValue(Flags::HEROFAST)) {
		_step *= 2.5;
	} else if (_state == kHeroStateRun) {
		_step *= 2;
	}
}

//TODO - test this
void Hero::scrollHero() {
	int scrollType = _vm->_flags->getFlagValue(Flags::SCROLLTYPE);
	int position = _middleX;
	int scrollValue, scrollValue2;

	switch (scrollType) {
	case 0:
		position = _middleX;
		break;
	case 1:
		scrollValue = _vm->_flags->getFlagValue(Flags::SCROLLVALUE);
		position = _vm->_normAnimList[scrollValue]._currX + _vm->_normAnimList[scrollValue]._currW / 2;
		break;
	case 2:
		scrollValue = _vm->_flags->getFlagValue(Flags::SCROLLVALUE);
		scrollValue2 = _vm->_flags->getFlagValue(Flags::SCROLLVALUE2);
		position = scrollValue;
		if (scrollValue < scrollValue2) {
			_vm->_flags->setFlagValue(Flags::SCROLLVALUE, 0);
		} else {
			_vm->_flags->setFlagValue(Flags::SCROLLVALUE, scrollValue - scrollValue2);
		}
		break;
	}

	int locationWidth = _vm->_sceneWidth;
	int difference = locationWidth - _vm->kNormalWidth / 2;

	int destValue = 0;
	if (position > _vm->kNormalWidth / 2) {
		destValue = difference - _vm->kNormalWidth / 2;
	}
	if (position < difference) {
		destValue = position - _vm->kNormalWidth / 2;
	}
	if(destValue < 0) {
		destValue = 0;
	}
	_vm->_picWindowX = destValue;
	_drawX -= destValue;
}

void Hero::freeOldMove() {
	if (_coords != nullptr) {
		free(_coords);
		_coords = nullptr;
	}
	if (_dirTab != nullptr) {
		free(_dirTab);
		_dirTab = nullptr;
	}
	_step = 0;
	_phase = 0;
	_moveDelay = 0;
	_state = Hero::kHeroStateStay;
}

void Hero::freeHeroAnim() {
	if (_specAnim != nullptr) {
		delete _specAnim;
		_specAnim = nullptr;
	}
}

}

/* vim: set tabstop=4 noexpandtab: */
