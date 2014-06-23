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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This code is based on original Sfinx source code
 * Copyright (c) 1994-1997 Janus B. Wisniewski and L.K. Avalon
 */

#include "sound.h"
#include "cge2/cge2_main.h"
#include "cge2/cge2.h"
#include "cge2/vga13h.h"
#include "cge2/text.h"
#include "cge2/snail.h"
#include "cge2/hero.h"
#include "cge2/spare.h"
#include "cge2/events.h"
#include "cge2/map.h"
#include "cge2/vmenu.h"

namespace CGE2 {

System::System(CGE2Engine *vm) : Sprite(vm), _vm(vm) {
	_blinkCounter = 0;
	_blinkSprite = nullptr;
	tick();
}

void System::touch(uint16 mask, V2D pos, Common::KeyCode keyCode) {
	if (mask & kEventKeyb) {
		if (keyCode == Common::KEYCODE_ESCAPE) {
			// The original was calling keyClick()
			// The sound is uselessly annoying and noisy, so it has been removed
			_vm->killText();
			if (_vm->_startupMode == 1) {
				_vm->_commandHandler->addCommand(kCmdClear, -1, 0, NULL);
				return;
			}
		}
	} else {
		if (_vm->_startupMode)
			return;
		_vm->_infoLine->setText(nullptr);
		
		if (mask & kMouseLeftUp) {
			if (pos.y >= 0)	{ // world
				if (!_vm->_talk && pos.y < _vm->_mouseTop)
					_vm->_heroTab[_vm->_sex]->_ptr->walkTo(pos);
			} else { // panel
				if (_vm->_commandHandler->idle()) {
					int sex = pos.x < kPocketsWidth;
					if (sex || pos.x >= kScrWidth - kPocketsWidth) {
						_vm->switchHero(sex);
						if (_vm->_sex == sex) {
							int dx = kPocketsWidth >> 1,
								dy = 1 - (kPanHeight >> 1);
							Sprite *s;
							if (!sex)
								pos.x -= kScrWidth - kPocketsWidth;
							dx -= pos.x;
							dy -= pos.y;
							if (dx * dx + dy * dy > 10 * 10) {
								int n = 0;
								if (1 - pos.y >= (kPanHeight >> 1))
									n += 2;
								if (pos.x >= (kPocketsWidth >> 1))
									++n;
								s = _vm->_heroTab[_vm->_sex]->_pocket[n];
								if (_vm->_sys->_blinkSprite)
									_vm->_sys->_blinkSprite->_flags._hide = false;
								if (_vm->_sys->_blinkSprite == s)
									_vm->_sys->_blinkSprite = nullptr;
								else
									_vm->_sys->_blinkSprite = s;
							}
						}
					}
				}
			}
		}
	}
}

void System::tick() {
	_time = kSysTimeRate;

	if (_blinkCounter)
		--_blinkCounter;
	else {
		if (_blinkSprite)
			_blinkSprite->_flags._hide ^= 1;
		_blinkCounter = kBlinkRate;
	}
}

int CGE2Engine::number(char *s) { // TODO: Rework it later to include the preceding token() call!
	int r = atoi(s);
	char *pp = strchr(s, ':');
	if (pp)
		r = (r << 8) + atoi(pp + 1);
	return r;
}

char *CGE2Engine::token(char *s) {
	return strtok(s, " =\t,;/()");
}

char *CGE2Engine::tail(char *s) {
	if (s && (*s == '='))
		s++;
	return s;
}

int CGE2Engine::takeEnum(const char **tab, const char *text) {
	if (text) {
		for (const char **e = tab; *e; e++) {
			if (scumm_stricmp(text, *e) == 0) {
				return e - tab;
			}
		}
	}
	return -1;
}

ID CGE2Engine::ident(const char *s) {
	return ID(takeEnum(EncryptedStream::kIdTab, s));
}

bool CGE2Engine::testBool(char *s) {
	return number(s) != 0;
}

void CGE2Engine::badLab(const char *fn) {
	error("Misplaced label in %s!", fn);
}

void CGE2Engine::loadSprite(const char *fname, int ref, int scene, V3D &pos) {
	int shpcnt = 0;
	int seqcnt = 0;
	int cnt[kActions];
	for (int i = 0; i < kActions; i++)
		cnt[i] = 0;
	ID section = kIdPhase;
	bool frnt = true;
	bool east = false;
	bool port = false;
	bool tran = false;
	Hero *h;
	ID id;

	char tmpStr[kLineMax + 1];
	mergeExt(tmpStr, fname, kSprExt);

	if (_resman->exist(tmpStr)) { // sprite description file exist
		EncryptedStream sprf(this, tmpStr);
		if (sprf.err())
			error("Bad SPR [%s]", tmpStr);

		int label = kNoByte;
		Common::String line;

		for (line = sprf.readLine(); !sprf.eos(); line = sprf.readLine()){
			if (line.empty())
				continue;
			Common::strlcpy(tmpStr, line.c_str(), sizeof(tmpStr));
				
			char *p;
			p = token(tmpStr);
			if (*p == '@') {
				if (label != kNoByte)
					badLab(fname);
				label = atoi(p + 1);
				continue;
			}

			id = ident(p);
			switch (id) {
			case kIdName: // will be taken in Expand routine
				if (label != kNoByte)
					badLab(fname);
				break;
			case kIdType:
				if (label != kNoByte)
					badLab(fname);
				break;
			case kIdNear:
			case kIdMTake:
			case kIdFTake:
			case kIdPhase:
			case kIdSeq:
				if (label != kNoByte)
					badLab(fname);
				section = id;
				break;
			case kIdFront:
				if (label != kNoByte)
					badLab(fname);
				p = token(nullptr);
				frnt = testBool(p);
				break;
			case kIdEast:
				if (label != kNoByte)
					badLab(fname);
				p = token(nullptr);
				east = testBool(p);
				break;
			case kIdPortable:
				if (label != kNoByte)
					badLab(fname);
				p = token(nullptr);
				port = testBool(p);
				break;
			case kIdTransparent:
				if (label != kNoByte)
					badLab(fname);
				p = token(nullptr);
				tran = testBool(p);
				break;
			default:
				if (id >= kIdNear)
					break;
				switch (section) {
				case kIdNear:
				case kIdMTake:
				case kIdFTake:
					if (_commandHandler->getComId(p) >= 0)
						++cnt[section];
					else
						error("Bad line %d [%s]", sprf.getLineCount(), tmpStr);
					break;
				case kIdPhase:
					if (label != kNoByte)
						badLab(fname);
					++shpcnt;
					break;
				case kIdSeq:
					if (label != kNoByte)
						badLab(fname);
					++seqcnt;
					break;
				default:
					break;
				}
				break;
			}
			label = kNoByte;
		}

		if (!shpcnt) {
			error("No shapes - %s", fname);
		}
	} else // No sprite description: mono-shaped sprite with only .BMP file.
		++shpcnt;

	// Make sprite of choosen type:
	char c = *fname | 0x20;
	if (c >= 'a' && c <= 'z' && fname[1] == '0' && fname[2] == '\0') {
		h = new Hero(this);
		if (h) {
			h->gotoxyz(pos);
			_sprite = h;
		}
	} else {
		if (_sprite)
			delete _sprite;
		_sprite = new Sprite(this);
		if (_sprite)
			_sprite->gotoxyz(pos);
	}

	if (_sprite) {
		_sprite->_ref = ref;
		_sprite->_scene = scene;

		_sprite->_flags._frnt = frnt;
		_sprite->_flags._east = east;
		_sprite->_flags._port = port;
		_sprite->_flags._tran = tran;
		_sprite->_flags._kill = true;

		// Extract the filename, without the extension
		Common::strlcpy(_sprite->_file, fname, sizeof(_sprite->_file));
		char *p = strchr(_sprite->_file, '.');
		if (p)
			*p = '\0';

		_sprite->_shpCnt = shpcnt;
		_sprite->_seqCnt = seqcnt;

		for (int i = 0; i < kActions; i++)
			_sprite->_actionCtrl[i]._cnt = cnt[i];
	}
}

void CGE2Engine::loadScript(const char *fname) {
	EncryptedStream scrf(this, fname);

	if (scrf.err())
		return;

	bool ok = true;
	int lcnt = 0;

	char tmpStr[kLineMax + 1];
	Common::String line;

	for (line = scrf.readLine(); !scrf.eos(); line = scrf.readLine()) {
		if (line.empty())
			continue;

		char *p;

		lcnt++;
		Common::strlcpy(tmpStr, line.c_str(), sizeof(tmpStr));
		
		ok = false; // not OK if break

		V3D P;

		// sprite ident number
		if ((p = token(tmpStr)) == NULL)
			break;
		int SpI = number(p);

		// sprite file name
		char *SpN;
		if ((SpN = token(nullptr)) == NULL)
			break;

		// sprite scene
		if ((p = token(nullptr)) == NULL)
			break;
		int SpA = number(p);

		// sprite column
		if ((p = token(nullptr)) == NULL)
			break;
		P._x = number(p);

		// sprite row
		if ((p = token(nullptr)) == NULL)
			break;
		P._y = number(p);

		// sprite Z pos
		if ((p = token(nullptr)) == NULL)
			break;
		P._z = number(p);

		// sprite life
		if ((p = token(nullptr)) == NULL)
			break;
		bool BkG = number(p) == 0;

		ok = true; // no break: OK

		_sprite = NULL;
		loadSprite(SpN, SpI, SpA, P);
		if (_sprite) {
			if (BkG)
				_sprite->_flags._back = true;

			int n = _spare->count();
			if (_spare->locate(_sprite->_ref) == nullptr)
				_spare->dispose(_sprite);
			_sprite = nullptr;
			if (_spare->count() == n)
				error("Durplicated reference! %s", SpN);
		}
	}

	if (!ok)
		error("Bad INI line %d [%s]", scrf.getLineCount(), fname);
}

void CGE2Engine::movie(const char *ext) {
	assert(ext);

	if (_quitFlag)
		return;

	char fn[12];
	sprintf(fn, "CGE.%s", (*ext == '.') ? ext + 1 : ext);

	if (_resman->exist(fn)) {
		int now = _now;
		_now = atoi(ext + 2);
		loadScript(fn);
		sceneUp(_now);

		while (!_commandHandler->idle() && !_quitFlag)
			mainLoop();

		warning("STUB: CGE2Engine::movie()");

		_commandHandler->addCommand(kCmdClear, -1, 0, nullptr);
		_commandHandlerTurbo->addCommand(kCmdClear, -1, 0, nullptr);
		_vga->_showQ->clear();
		_spare->clear();
		_now = now;
	}
}

void CGE2Engine::sceneUp(int cav) {
	_now = cav;
	int bakRef = _now << 8;
	if (_music)
		_midiPlayer->loadMidi(bakRef);
	showBak(bakRef);
	*_eye = *(_eyeTab[_now]);
	_mouseTop = V2D(this, V3D(0, 1, kScrDepth)).y;
	_map->load(_now);
	_spare->takeScene(_now);
	openPocket();
	
	for (int i = 0; i < 2; i++) {
		Hero *h = _heroTab[i]->_ptr;
		if (h && h->_scene == _now) {
			V2D p = *_heroTab[i]->_posTab[_now];
			h->gotoxyz(V3D(p.x, 0, p.y));
			h->clrHide();
			_vga->_showQ->insert(h);
			h->park();
			h->setCurrent();
			h->setContact();
		}
	}
	
	_sound->stop();
	_fx->clear();

	selectPocket(-1);
	_infoLine->setText(nullptr);
	busy(false);

	if (!_dark)
		_vga->sunset();
	_vga->show();
	_vga->copyPage(1, 0);
	_vga->show();

	_sprite = _vga->_showQ->first();
	_vga->sunrise(_vga->_sysPal);

	_dark = false;

	if (!_startupMode)
		_mouse->on();

	feedSnail(_vga->_showQ->locate(bakRef + 255), kNear, _heroTab[_sex]->_ptr);
	//setDrawColors(); - It's only for debugging purposes. Can be left out for now.
}

void CGE2Engine::sceneDown() {
	busy(true);
	_commandStat._wait = nullptr; // unlock snail
	Sprite *spr = _vga->_showQ->locate((_now << 8) | 254);
	if (spr)
		feedSnail(spr, kNear, _heroTab[_sex]->_ptr);
	while (!(_commandHandler->idle() && _commandHandlerTurbo->idle())) {
		_commandHandlerTurbo->runCommand();
		_commandHandler->runCommand();
	}
	closePocket();
	for (int i = 0; i < 2; i++)
		_spare->update(_vga->_showQ->remove(_heroTab[i]->_ptr));
	_spare->dispose();
}

void CGE2Engine::switchScene(int scene) {
	if (scene == _now)
		return;
	
	/* TODO:: Recheck this flag-thing! As far as it seems it's only for checking if the game
	   is being run from the CD-ROM. In this context, it's useless for us.

	if (scene >= 0) {
		if (!_flag[2]) // PROT
			_flag[2] = true;
		else {
			int t = _text->getText(kCrackedText) ? kCrackedText : kExitOkText;
			_commandHandler->addCommand(kCmdInf, -1, t, nullptr);
			return;
		}
	}
	*/

	_req = scene;
	for (int i = 0; i < 2; i++) {
		Hero *h = _heroTab[i]->_ptr;
		if (h->_scene == _now) {
			delete _heroTab[i]->_posTab[_now];
			V2D *temp = new V2D(this, h->_pos3D._x.trunc(), h->_pos3D._z.trunc());
			_heroTab[i]->_posTab[_now] = temp;
		}
	}
	*(_eyeTab[_now]) = *_eye;
	if (scene < 0)
		_commandHandler->addCallback(kCmdExec, -1, 0, kQGame); // quit game
	else {
		if (_heroTab[_sex]->_ptr->_scene == _now) {
			_heroTab[_sex]->_ptr->setScene(scene);
			if (_heroTab[!_sex]->_ptr->_scene == _now)
				_heroTab[!_sex]->_ptr->setScene(scene);
		}
		_mouse->off();
		if (_heroTab[_sex]->_ptr)
			_heroTab[_sex]->_ptr->park();
		killText();
		_commandHandler->addCallback(kCmdExec, -1, 0, kXScene); // switch scene
	}
}

void CGE2Engine::showBak(int ref) {
	Sprite *spr = _spare->take(ref);
	if (spr != nullptr) {
		_bitmapPalette = _vga->_sysPal;
		spr->expand();
		_bitmapPalette = NULL;
		spr->show(2);
		_vga->copyPage(1, 2);
		_spare->dispose(spr);
	}
}

void CGE2Engine::mainLoop() {
	_vga->show();
	_commandHandlerTurbo->runCommand();
	_commandHandler->runCommand();

	// Handle a delay between game frames
	handleFrame();

	// Handle any pending events
	_eventManager->poll();

	// Check shouldQuit()
	_quitFlag = shouldQuit();
}

void CGE2Engine::handleFrame() {
	// Game frame delay
	uint32 millis = g_system->getMillis();
	while (!_quitFlag && (millis < (_lastFrame + kGameFrameDelay))) {
		// Handle any pending events
		_eventManager->poll();

		if (millis >= (_lastTick + kGameTickDelay)) {
			// Dispatch the tick to any active objects
			tick();
			_lastTick = millis;
		}

		// Slight delay
		g_system->delayMillis(5);
		millis = g_system->getMillis();
	}
	_lastFrame = millis;

	if (millis >= (_lastTick + kGameTickDelay)) {
		// Dispatch the tick to any active objects
		tick();
		_lastTick = millis;
	}
}

Sprite *CGE2Engine::locate(int ref) {
	_taken = false;
	Sprite *spr = _vga->_showQ->locate(ref);
	if (!spr) {
		spr = _spare->locate(ref);
		if (spr)
			_taken = true;
	}
	return spr;
}

bool CGE2Engine::isHero(Sprite *spr) {
	return spr && spr->_ref / 10 == 14;
}

void CGE2Engine::tick() {
	// system pseudo-sprite
	if (_sys && _sys->_time && (--_sys->_time == 0))
		_sys->tick();

	for (Sprite *spr = _vga->_showQ->first(); spr; spr = spr->_next) {
		if (spr->_time && (--spr->_time == 0))
				spr->tick();
		
		if (_waitRef) {
			if (_waitRef == spr->_ref && spr->seqTest(_waitSeq))
					_waitRef = 0;
		}
	}

	_mouse->tick();
}

void CGE2Engine::openPocket() {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < kPocketMax + 1; j++) {
			int ref = (int)_heroTab[i]->_pocket[j];
			_heroTab[i]->_pocket[j] = (ref == -1) ? nullptr : _vga->_showQ->locate(ref);
		}
	}
}

void CGE2Engine::closePocket() {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < kPocketMax + 1; j++) {
			Sprite *spr = _heroTab[i]->_pocket[j];
			_heroTab[i]->_pocket[j] = (Sprite*)((spr) ? spr->_ref : -1);
		}
	}
}


void CGE2Engine::selectPocket(int n) {
	Sprite **p = _heroTab[_sex]->_pocket;
	int &pp = _heroTab[_sex]->_pocPtr;
	if (n < 0 || pp == n) {
		n = findActivePocket(-1);
		if (n >= 0)
			pp = n;
	} else if (p[n]) {
		pp = n;
	}
}

void CGE2Engine::busy(bool on) {
	if (on) {
		_spriteNotify = _midiNotify = &CGE2::CGE2Engine::busyStep;
		busyStep();
	} else {
		if (_busyPtr)
			_busyPtr->step(0);
		_spriteNotify = _midiNotify = nullptr;
	}
}

void CGE2Engine::busyStep() {
	if (_busyPtr) {
		_busyPtr->step((_busyPtr->_seqPtr) ? -1 : 1);
		_busyPtr->show(0);
	}
}

void CGE2Engine::runGame() {
	if (_quitFlag)
		return;

	selectPocket(-1);

	loadUser();

	_commandHandlerTurbo->addCommand(kCmdSeq, kMusicRef, _music, nullptr);
	if (!_music)
		_midiPlayer->killMidi();

	checkSaySwitch();
		
	_infoLine->gotoxyz(V3D(kInfoX, kInfoY, 0));
	_infoLine->setText(nullptr);
	_vga->_showQ->insert(_infoLine);

	sceneUp(_now);
	_startupMode = 0;
	_mouse->center();
	_mouse->off();
	_mouse->on();

	_keyboard->setClient(_sys);
	_commandHandler->addCommand(kCmdSeq, kPowerRef, 1, nullptr);

	_busyPtr = _vga->_showQ->locate(kBusyRef);

	_vol[0] = _vga->_showQ->locate(kDvolRef);
	_vol[1] = _vga->_showQ->locate(kMvolRef);

	// these sprites are loaded with SeqPtr==0 (why?!)
	if (_vol[0])
		_vol[0]->step((/*(int)SNDDrvInfo.VOL4.DL * */ _vol[0]->_seqCnt + _vol[0]->_seqCnt / 2) >> 4);
	if (_vol[1])
		_vol[1]->step((/*(int)SNDDrvInfo.VOL4.ML * */ _vol[1]->_seqCnt + _vol[1]->_seqCnt / 2) >> 4);
	// TODO: Recheck these! ^

	// main loop
	while (!_endGame && !_quitFlag) {
		if (_flag[3]) // Flag FINIS
			_commandHandler->addCallback(kCmdExec, -1, 0, kQGame);
		mainLoop();
	}

	// If finishing game due to closing ScummVM window, explicitly save the game
	if (!_endGame && canSaveGameStateCurrently())
		qGame();

	_keyboard->setClient(nullptr);
	_commandHandler->addCommand(kCmdClear, -1, 0, nullptr);
	_commandHandlerTurbo->addCommand(kCmdClear, -1, 0, nullptr);
	_mouse->off();
	_vga->_showQ->clear();
}

void CGE2Engine::loadUser() {
	warning("STUB: CGE2Engine::loadUser()");
	// Missing loading from file. TODO: Implement it with the saving/loading!
	loadScript("CGE.INI");
	loadGame();
	loadPos();
}

void CGE2Engine::loadGame() {
	warning("STUB: CGE2Engine::loadGame()");

	// load sprites & pocket
	
	Sprite *s;
	Hero *h = nullptr;
	
	// initialize Andzia/Anna
	s = _spare->take(142);
	if (s) {
		h = new Hero(this);
		*(Sprite*)h = *s;
		delete s;
		s = nullptr;
		h->expand();
		_spare->update(h);
	}
	_heroTab[0]->_ptr = h;
	s = _spare->take(152);
	_vga->_showQ->insert(s);
	_heroTab[0]->_face = s;

	// initialize Wacek/Vincent
	s = _spare->take(141);
	if (s) {
		h = new Hero(this);
		*(Sprite*)h = *s;
		delete s;
		s = nullptr;
		h->expand();
		_spare->update(h);
	}
	_heroTab[1]->_ptr = h;
	s = _spare->take(151);
	_vga->_showQ->insert(s);
	_heroTab[1]->_face = s;

	//--- start!
	switchHero(_sex);
}

void CGE2Engine::loadPos() {
	if (_resman->exist("CGE.HXY")) {
		for (int cav = 0; cav < kSceneMax; cav++)
			_heroTab[1]->_posTab[cav] = new V2D(this, 180, 10);
		
		EncryptedStream file(this, "CGE.HXY");

		for (int cav = 0; cav < kSceneMax; cav++) {
			_heroTab[0]->_posTab[cav] = new V2D(this);
			_heroTab[0]->_posTab[cav]->x = file.readSint16LE();
			_heroTab[0]->_posTab[cav]->y = file.readSint16LE();
		}

		for (int cav = 0; cav < 41; cav++) { // (564 - 400) / 4 = 41
			_heroTab[1]->_posTab[cav]->x = file.readSint16LE();
			_heroTab[1]->_posTab[cav]->y = file.readSint16LE();
		}
	} else
		error("Missing file: CGE.HXY");
}

void CGE2Engine::releasePocket(Sprite *spr) {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < kPocketMax; j++) {
			Sprite *poc = _heroTab[i]->_pocket[j];
			if (poc == spr) {
				spr->_flags._kept = false;
				poc = nullptr;
				return;
			}
		}
	}
}

void CGE2Engine::checkSaySwitch() {
	warning("STUB: CGE2Engine::checkSaySwitch()");
}

void CGE2Engine::loadTab() {
	setEye(_text->getText(240));
	for (int i = 0; i < kSceneMax; i++)
		*(_eyeTab[i]) = *_eye;

	if  (_resman->exist(kTabName)) {
		EncryptedStream f(this, kTabName);
		uint32 v;

		for (int i = 0; i < kSceneMax; i++) {
			v = f.readUint32LE();
			_eyeTab[i]->_x = FXP(v >> 8, static_cast<int>((int8)(v & 0xff)));

			v = f.readUint32LE();
			_eyeTab[i]->_y = FXP(v >> 8, static_cast<int>((int8)(v & 0xff)));

			v = f.readUint32LE();
			_eyeTab[i]->_z = FXP(v >> 8, static_cast<int>((int8)(v & 0xff)));
		}
	}
}

void CGE2Engine::cge2_main() {
	warning("STUB: CGE2Engine::cge2_main()");

	loadTab();

	_mode++;

	if (showTitle("WELCOME")) {
#if 0
		if (_mode == 1)
			movie(kIntroExt);
#endif
		if (_text->getText(255) != NULL) {
			runGame();
			_startupMode = 2;
		} else
			_vga->sunset();
	} else
		_vga->sunset();
}

char *CGE2Engine::mergeExt(char *buf, const char *name, const char *ext) {
	strcpy(buf, name);
	char *dot = strrchr(buf, '.');
	if (!dot)
		strcat(buf, ext);

	return buf;
}

void CGE2Engine::setEye(const V3D &e) { 
	*_eye = e;
}

void CGE2Engine::setEye(const V2D& e2, int z) {
	_eye->_x = e2.x;
	_eye->_y = e2.y;
	_eye->_z = z;
}

void CGE2Engine::setEye(const char *s) {
	char tempStr[kLineMax];
	strcpy(tempStr, s);
	_eye->_x = atoi(token(tempStr));
	_eye->_y = atoi(token(NULL));
	_eye->_z = atoi(token(NULL));
}

int CGE2Engine::newRandom(int range) {
	if (!range)
		return 0;

	return _randomSource.getRandomNumber(range - 1);
}

bool CGE2Engine::showTitle(const char *name) {
	if (_quitFlag)
		return false;

	_bitmapPalette = _vga->_sysPal;
	BitmapPtr LB = new Bitmap[1];
	LB[0] = Bitmap(this, name);
	_bitmapPalette = nullptr;

	Sprite D(this, LB, 1);
	D._flags._kill = true;
	strcpy(D._file, "hatter");
	warning("STUB: Sprite::showTitle() - Flags changed compared to CGE1's Sprite type.");
	D.gotoxyz(kScrWidth >> 1, -(kPanHeight >> 1));
	_vga->sunset();

	D.show(2);

	_vga->copyPage(1, 2);
	_vga->copyPage(0, 1);

	_vga->sunrise(_vga->_sysPal);

	_vga->update();

	warning("STUB: CGE2Engine::showTitle()");
	
	return true;
}

int CGE2Engine::freePockets(int sx) {
	int n = 0;
	for (int i = 0; i < kPocketMax; i++){
		if (_heroTab[sx]->_pocket[i] == nullptr)
			++n;
	}
	return n;
}

int CGE2Engine::findActivePocket(int ref) {
	for (int i = 0; i < kPocketMax; i++) {
		Sprite *spr = _heroTab[_sex]->_pocket[i];
		if (ref >= 0) {
			if (spr && spr->_ref == ref)
				return i;
		} else if (!spr)
			return i;
	}
	return -1;
}

void CGE2Engine::pocFul() {
	Hero *h = _heroTab[_sex]->_ptr;
	h->park();
	_commandHandler->addCommand(kCmdWait, -1, -1, h);
	_commandHandler->addCommand(kCmdSound, -1, 2, h);
	_commandHandler->addCommand(kCmdSay, -1, kPocketFull + _sex, h);
}

void CGE2Engine::killText() {
	if (!_talk)
		return;

	_commandHandlerTurbo->addCommand(kCmdKill, -1, 0, _talk);
	_talk = NULL;
}

void CGE2Engine::switchHero(int sex) {
	if (sex != _sex) {
		int scene = _heroTab[sex]->_ptr->_scene;
		if (_sys->_blinkSprite) {
			_sys->_blinkSprite->_flags._hide = false;
			_sys->_blinkSprite = nullptr;
		}
		if (scene >= 0) {
			_commandHandler->addCommand(kCmdSeq, -1, 2, _heroTab[_sex]->_face);
			_sex ^= 1;
			switchScene(scene);
		}
	}
	Sprite *face = _heroTab[_sex]->_face;
	if (face->_seqPtr == 0)
		_commandHandler->addCommand(kCmdSeq, -1, 1, face);
}

#pragma argsused
void Sprite::touch(uint16 mask, V2D pos, Common::KeyCode keyCode) {
	if ((mask & kEventAttn) != 0)
		return;

	if (!_vm->_startupMode)
		_vm->_infoLine->setText(name());

	if (_ref < 0)
		return; // cannot access system sprites

	if (_ref / 10 == 12) {
		_vm->optionTouch(_ref % 10, mask);
		return;
	}

	if ((mask & kMouseLeftUp) && _vm->_commandHandler->idle()) {
		if (_vm->isHero(this) && !_vm->_sys->_blinkSprite) {
			_vm->switchHero((this == _vm->_heroTab[1]->_ptr) ? 1 : 0);
		} else if (_flags._kept) { // sprite in pocket
			for (int sex = 0; sex < 2; ++sex) {
				for (int p = 0; p < kPocketMax; ++p) {
					if (_vm->_heroTab[sex]->_pocket[p] == this) {
						_vm->switchHero(sex);
						if (_vm->_sex == sex) {
							if (_vm->_sys->_blinkSprite)
								_vm->_sys->_blinkSprite->_flags._hide = false;
							if (_vm->_sys->_blinkSprite == this)
								_vm->_sys->_blinkSprite = nullptr;
							else
								_vm->_sys->_blinkSprite = this;
						}
					}
				}
			}
		} else { // sprite NOT in pocket
			Hero *h = _vm->_heroTab[_vm->_sex]->_ptr;
			if (!_vm->_talk) {
				if ((_ref & 0xFF) < 200 && h->distance(this) > (h->_maxDist << 1))
					h->walkTo(this);
				else if (_vm->_sys->_blinkSprite) {
					if (works(_vm->_sys->_blinkSprite)) {
						_vm->feedSnail(_vm->_sys->_blinkSprite, (_vm->_sex) ? kMTake : kFTake, _vm->_heroTab[_vm->_sex]->_ptr);
						_vm->_sys->_blinkSprite->_flags._hide = false;
						_vm->_sys->_blinkSprite = nullptr;
					} else
						_vm->offUse();

					_vm->selectPocket(-1);
					// else, no pocket sprite selected
				} else if (_flags._port) { // portable
					if (_vm->findActivePocket(-1) < 0)
						_vm->pocFul();
					else {
						_vm->_commandHandler->addCommand(kCmdReach, -2, _ref, nullptr);
						_vm->_commandHandler->addCommand(kCmdKeep, -1, -1, this);
						_flags._port = false;
					}
				} else { // non-portable
					Action a = h->action();
					if (_actionCtrl[a]._cnt) {
						CommandHandler::Command *cmdList = snList(a);
						if (cmdList[_actionCtrl[a]._ptr]._commandType == kCmdNext)
							_vm->offUse();
						else
							_vm->feedSnail(this, a, h);
					} else
						_vm->offUse();
				}
			}
		}
	}
}

void CGE2Engine::optionTouch(int opt, uint16 mask) {
	switch (opt) {
	case 1:
		if (mask & kMouseLeftUp)
			switchColorMode();
		break;
	case 2:
		if (mask & kMouseLeftUp)
			switchMusic();
		break;
	case 3:
		if (mask & kMouseLeftUp)
			quit();
		break;
	case 4:
		if (mask & (kMouseLeftUp | kMouseRightUp))
			setVolume(opt - 4, (mask & kMouseLeftUp) ? 1 : -1);
		break;
	case 5:
		if (mask & (kMouseLeftUp | kMouseRightUp))
			setVolume(opt - 4, (mask & kMouseLeftUp) ? 1 : -1);
		break;
	case 8:
		if (mask & kMouseLeftUp)
			switchCap();
		break;
	case 9:
		if (mask & kMouseLeftUp)
			switchVox();
		break;
	default:
		break;
	}
}

void CGE2Engine::switchColorMode() {
	warning("STUB: CGE2Engine::switchColorMode()");
}

void CGE2Engine::switchMusic() {
	warning("STUB: CGE2Engine::switchMusic()");
}

void CGE2Engine::quit() {
	Common::Array<Choice *> quitMenu; // Deleted in VMenu's destructor.
	quitMenu.push_back(new StartCountDownChoice(this));
	quitMenu.push_back(new ResetQSwitchChoice(this));

	if (_commandHandler->idle()) {
		if (VMenu::_addr) {
			_commandHandlerTurbo->addCommand(kCmdKill, -1, 0, VMenu::_addr);
			ResetQSwitchChoice rqsChoice(this);
			rqsChoice.proc();
		} else {
			quitMenu[0]->_text = _text->getText(kQuitText);
			quitMenu[1]->_text = _text->getText(kNoQuitText);
			(new VMenu(this, quitMenu, V2D(this, -1, -1), kCBMnu))->setName(_text->getText(kQuitTitle));
			_commandHandlerTurbo->addCommand(kCmdSeq, kPowerRef, 0, nullptr);
			keyClick();
		}
	}
}

void CGE2Engine::keyClick() {
	_commandHandlerTurbo->addCommand(kCmdSound, -1, 5, nullptr);
}

void CGE2Engine::setVolume(int idx, int cnt) {
	warning("STUB: CGE2Engine::setVolume()");
}

void CGE2Engine::switchCap() {
	warning("STUB: CGE2Engine::switchCap()");
}

void CGE2Engine::switchVox() {
	warning("STUB: CGE2Engine::switchVox()");
}

void CGE2Engine::offUse() {
	int seq = 0;
	int offUseCount = atoi(_text->getText(kOffUseCount));

	// This fixes the issue of empty speech bubbles in the original.
	// Now we only let this cycle pass if it randoms a valid value for getText().
	int txt = 0;
	do { 
		txt = kOffUseText + _sex * offUseCount + newRandom(offUseCount);
	} while (_text->getText(txt) == nullptr);

	Hero *h = _heroTab[_sex]->_ptr;
	h->park();
	_commandHandler->addCommand(kCmdWait, -1, -1, h);
	_commandHandler->addCommand(kCmdSeq, -1, seq, h);
	if (!_sayVox)
		_commandHandler->addCommand(kCmdSound, -1, 6 + _sex, h);
	_commandHandler->addCommand(kCmdWait, -1, -1, h);
	_commandHandler->addCommand(kCmdSay, -1, txt, h);
}

Sprite *CGE2Engine::spriteAt(V2D pos) {
	Sprite *spr;

	for (spr = _vga->_showQ->last(); spr; spr = spr->_prev) {
		if (!spr->_flags._hide && !spr->_flags._tran) {
			if (spr->getShp()->solidAt(pos - spr->_pos2D))
				break;
		}
	}

	return spr;
}

} // End of namespace CGE2
