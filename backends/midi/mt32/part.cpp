/* Copyright (c) 2003-2004 Various contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <string.h>
#include <math.h>

#include "mt32emu.h"

namespace MT32Emu {

static const Bit8u PartialStruct[13] = {
	0, 0, 2, 2, 1, 3,
	3, 0, 3, 0, 2, 1, 3 };

static const Bit8u PartialMixStruct[13] = {
	0, 1, 0, 1, 1, 0,
	1, 3, 3, 2, 2, 2, 2 };

// This caches the timbres/settings in use by the rhythm part
static PatchCache drumCache[94][4];
static StereoVolume drumPan[64];

//FIXME:KG: Put this dpoly stuff somewhere better
bool dpoly::isActive() {
	return partials[0] != NULL || partials[1] != NULL || partials[2] != NULL || partials[3] != NULL;
}

Bit64s dpoly::getAge() {
	for (int i = 0; i < 4; i++) {
		if (partials[i] != NULL) {
			return partials[i]->age;
		}
	}
	return 0;
}

Part::Part(Synth *useSynth, int usePartNum) {
	this->synth = useSynth;
	this->partNum = usePartNum;
	isRhythm = (usePartNum == 8);
	holdpedal = false;
	if (isRhythm) {
		strcpy(name, "Rhythm");
		patchTemp = NULL;
		timbreTemp = NULL;
		rhythmTemp = &synth->mt32ram.params.rhythmSettings[0];
	} else {
		sprintf(name, "Part %d", partNum + 1);
		patchTemp = &synth->mt32ram.params.patchSettings[partNum];
		timbreTemp = &synth->mt32ram.params.timbreSettings[partNum];
		rhythmTemp = NULL;
	}
	currentInstr[0] = 0;
	currentInstr[10] = 0;
	volume = 102;
	volumesetting.leftvol = 32767;
	volumesetting.rightvol = 32767;
	bend = 0.0f;
	memset(polyTable,0,sizeof(polyTable));
	memset(patchCache, 0, sizeof(patchCache));

	if (isRhythm) {
		init = true;
		refreshDrumCache();
	}
	init = false;
}

void Part::setHoldPedal(bool pedalval) {
	if (holdpedal && !pedalval)
		stopPedalHold();
	holdpedal = pedalval;
}

void Part::setBend(int midiBend) {
	if (isRhythm) {
		synth->printDebug("%s: Setting bend (%d) not supported on rhythm", name, midiBend);
		return;
	}
	// FIXME:KG: Slightly uneven increments, but I wanted min -1.0, centre 0.0 and max 1.0
	if (midiBend <= 0x2000) {
		bend = (midiBend - 0x2000) / (float)0x2000;
	} else {
		bend = (midiBend - 0x2000) / (float)0x1FFF;
	}
	// Loop through all partials to update their bend
	for (int i = 0; i < MT32EMU_MAX_POLY; i++) {
		for (int j = 0; j < 4; j++) {
			if (polyTable[i].partials[j] != NULL) {
				polyTable[i].partials[j]->setBend(bend);
			}
		}
	}
}

void Part::setModulation(int vol) {
	if (isRhythm) {
		synth->printDebug("%s: Setting modulation (%d) not supported on rhythm", name, vol);
		return;
	}
	// Just a bloody guess, as always, before I get things figured out
	for (int t = 0; t < 4; t++) {
		if (patchCache[t].playPartial) {
			int newrate = (patchCache[t].modsense * vol) >> 7;
			//patchCache[t].lfoperiod = lfotable[newrate];
			patchCache[t].lfodepth = newrate;
			//FIXME:KG: timbreTemp->partial[t].lfo.depth =
		}
	}
}

void Part::refreshDrumCache() {
	if (!isRhythm) {
		synth->printDebug("ERROR: RefreshDrumCache() called on non-rhythm part");
	}
	// Cache drum patches
	for (int m = 0; m < 64; m++) {
		int drumTimbre = rhythmTemp[m].timbre;
		if (drumTimbre >= 94)
			continue;
		setPatch(drumTimbre + 128); // This is to cache all the mapped drum timbres ahead of time
		Bit16s pan = rhythmTemp[m].panpot; // They use R-L 0-14...
		// FIXME:KG: If I don't have left/right mixed up here, it's pure luck
		if (pan < 7) {
			drumPan[m].leftvol = 32767;
			drumPan[m].rightvol = pan * 4681;
		} else {
			drumPan[m].rightvol = 32767;
			drumPan[m].leftvol = (14 - pan) * 4681;
		}
	}
}

int Part::fixBiaslevel(int srcpnt, int *dir) {
	int noteat = srcpnt & 63;
	int outnote;
	*dir = 1;
	if (srcpnt < 64)
		*dir = 0;
	outnote = 33 + noteat;
	//synth->printDebug("Bias note %d, dir %d", outnote, *dir);

	return outnote;
}

int Part::fixKeyfollow(int srckey, int *dir) {
	if (srckey>=0 && srckey<=16) {
		//int keyfix[17] = { 256, 128, 64, 0, 32, 64, 96, 128, 128+32, 192, 192+32, 256, 256+64, 256+128, 512, 259, 269 };
		int keyfix[17] = { 256*16, 128*16, 64*16, 0, 32*16, 64*16, 96*16, 128*16, (128+32)*16, 192*16, (192+32)*16, 256*16, (256+64)*16, (256+128)*16, (512)*16, 4100, 4116};

		if (srckey<3)
			*dir = -1;
		else if (srckey==3)
			*dir = 0;
		else
			*dir = 1;

		return keyfix[srckey];
	} else {
		//LOG(LOG_ERROR|LOG_MISC,"Missed key: %d", srckey);
		return 256;
	}
}

void Part::refreshPatch() {
	setPatch(-1);
}

void Part::abortPoly(dpoly *poly) {
	if (!poly->isPlaying) {
		return;
	}
	for (int i = 0; i < 4; i++) {
		Partial *partial = poly->partials[i];
		if (partial != NULL) {
			partial->deactivate();
		}
	}
	poly->isPlaying = false;
}

void Part::setPatch(PatchParam *patch) {
	patchTemp->patch = *patch;
}

void Part::setTimbre(TimbreParam *timbre) {
	*timbreTemp = *timbre;
}

unsigned int Part::getAbsTimbreNum() {
	if (isRhythm) {
		synth->printDebug("%s: Attempted to call getAbsTimbreNum() - doesn't make sense for rhythm");
		return 0;
	}
	return (patchTemp->patch.timbreGroup * 64) + patchTemp->patch.timbreNum;
}

void Part::setPatch(int patchNum) {
	int absTimbreNum = -1; // Initialised to please compiler
	const TimbreParam *timbre;
	if (isRhythm) {
		// "patchNum" is treated as "timbreNum" for rhythm part
		if (patchNum < 128) {
			synth->printDebug("%s: Patch #%d is not valid for rhythm (must be >= 128)", name, patchNum);
			return;
		}
		absTimbreNum = patchNum;
		timbre = &synth->mt32ram.params.timbres[absTimbreNum].timbre;
	} else {
		if (patchNum >= 0) {
			setPatch(&synth->mt32ram.params.patches[patchNum]);
		}
		if (patchNum >= 0) {
			setTimbre(&synth->mt32ram.params.timbres[getAbsTimbreNum()].timbre);
		}
		timbre = timbreTemp;
#if 0
		// Immediately stop all partials on this part (this is apparently *not* the correct behaviour)
		for (int m = 0; m < MT32EMU_MAX_POLY; m++) {
			AbortPoly(poly);
		}
#else
		// check if any partials are still playing with the old patch cache
		// if so then duplicate the cached data from the part to the partial so that
		// we can change the part's cache without affecting the partial.
		// We delay this until now to avoid a copy operation with every note played
		for (int m = 0; m < MT32EMU_MAX_POLY; m++) {
			for (int i = 0; i < 4; i++) {
				Partial *partial = polyTable[m].partials[i];
				if (partial != NULL && partial->patchCache == &patchCache[i]) {
					// copy cache data
					partial->cachebackup = patchCache[i];
					// update pointers
					partial->patchCache = &partial->cachebackup;
				}
			}
		}
#endif
	}

	memcpy(currentInstr, timbre->common.name, 10);

	int partialCount = 0;
	for (int t = 0; t < 4; t++) {
		if (((timbre->common.pmute >> t) & 0x1) == 1) {
			patchCache[t].playPartial = true;
			partialCount++;
		} else {
			patchCache[t].playPartial = false;
			continue;
		}

		// Calculate and cache common parameters

		patchCache[t].pcm = timbre->partial[t].wg.pcmwave;
		patchCache[t].useBender = (timbre->partial[t].wg.bender == 1);

		switch (t) {
		case 0:
			patchCache[t].PCMPartial = (PartialStruct[(int)timbre->common.pstruct12] & 0x2) ? true : false;
			patchCache[t].structureMix = PartialMixStruct[(int)timbre->common.pstruct12];
			patchCache[t].structurePosition = 0;
			patchCache[t].structurePair = 1;
			break;
		case 1:
			patchCache[t].PCMPartial = (PartialStruct[(int)timbre->common.pstruct12] & 0x1) ? true : false;
			patchCache[t].structureMix = PartialMixStruct[(int)timbre->common.pstruct12];
			patchCache[t].structurePosition = 1;
			patchCache[t].structurePair = 0;
			break;
		case 2:
			patchCache[t].PCMPartial = (PartialStruct[(int)timbre->common.pstruct34] & 0x2) ? true : false;
			patchCache[t].structureMix = PartialMixStruct[(int)timbre->common.pstruct34];
			patchCache[t].structurePosition = 0;
			patchCache[t].structurePair = 3;
			break;
		case 3:
			patchCache[t].PCMPartial = (PartialStruct[(int)timbre->common.pstruct34] & 0x1) ? true : false;
			patchCache[t].structureMix = PartialMixStruct[(int)timbre->common.pstruct34];
			patchCache[t].structurePosition = 1;
			patchCache[t].structurePair = 2;
			break;
		default:
			break;
		}

		patchCache[t].waveform = timbre->partial[t].wg.waveform;
		patchCache[t].pulsewidth = timbre->partial[t].wg.pulsewid;
		patchCache[t].pwsens = timbre->partial[t].wg.pwvelo;
		patchCache[t].pitchkeyfollow = fixKeyfollow(timbre->partial[t].wg.keyfollow, &patchCache[t].pitchkeydir);

		// Calculate and cache pitch stuff
		patchCache[t].pitchshift = timbre->partial[t].wg.coarse;
		Bit32s pFine, fShift;
		pFine = (Bit32s)timbre->partial[t].wg.fine;
		if (isRhythm) {
			patchCache[t].pitchshift += 24;
			fShift = pFine + 50;
		} else {
			patchCache[t].pitchshift += patchTemp->patch.keyShift;
			fShift = pFine + (Bit32s)patchTemp->patch.fineTune;
		}
		patchCache[t].fineshift = finetable[fShift];

		patchCache[t].pitchEnv = timbre->partial[t].env;
		patchCache[t].pitchEnv.sensitivity = (char)((float)patchCache[t].pitchEnv.sensitivity*1.27);
		patchCache[t].pitchsustain = patchCache[t].pitchEnv.level[3];

		// Calculate and cache TVA envelope stuff
		patchCache[t].ampEnv = timbre->partial[t].tva;
		for (int i = 0; i < 4; i++)
			patchCache[t].ampEnv.envlevel[i] = (char)((float)patchCache[t].ampEnv.envlevel[i] * 1.27f);
		patchCache[t].ampEnv.level = (char)((float)patchCache[t].ampEnv.level * 1.27f);
		float tvelo = ((float)timbre->partial[t].tva.velosens / 100.0f);
		float velo = fabs(tvelo-0.5f) * 2.0f;
		velo *= 63.0f;
		patchCache[t].ampEnv.velosens = (char)velo;
		if (tvelo<0.5f)
			patchCache[t].ampenvdir = 1;
		else
			patchCache[t].ampenvdir = 0;

		patchCache[t].ampbias[0] = fixBiaslevel(patchCache[t].ampEnv.biaspoint1, &patchCache[t].ampdir[0]);
		patchCache[t].ampblevel[0] = 12 - patchCache[t].ampEnv.biaslevel1;
		patchCache[t].ampbias[1] = fixBiaslevel(patchCache[t].ampEnv.biaspoint2, &patchCache[t].ampdir[1]);
		patchCache[t].ampblevel[1] = 12 - patchCache[t].ampEnv.biaslevel2;
		patchCache[t].ampdepth = patchCache[t].ampEnv.envvkf * patchCache[t].ampEnv.envvkf;
		patchCache[t].ampsustain = patchCache[t].ampEnv.envlevel[3];
		patchCache[t].amplevel = patchCache[t].ampEnv.level;

		// Calculate and cache filter stuff
		patchCache[t].filtEnv = timbre->partial[t].tvf;
		patchCache[t].tvfdepth = patchCache[t].filtEnv.envdkf;
		patchCache[t].filtkeyfollow  = fixKeyfollow(patchCache[t].filtEnv.keyfollow, &patchCache[t].keydir);
		patchCache[t].filtEnv.envdepth = (char)((float)patchCache[t].filtEnv.envdepth * 1.27);
		patchCache[t].tvfbias = fixBiaslevel(patchCache[t].filtEnv.biaspoint, &patchCache[t].tvfdir);
		patchCache[t].tvfblevel = patchCache[t].filtEnv.biaslevel;
		patchCache[t].filtsustain  = patchCache[t].filtEnv.envlevel[3];

		// Calculate and cache LFO stuff
		patchCache[t].lfodepth = timbre->partial[t].lfo.depth;
		patchCache[t].lfoperiod = lfotable[(int)timbre->partial[t].lfo.rate];
		patchCache[t].lforate = timbre->partial[t].lfo.rate;
		patchCache[t].modsense = timbre->partial[t].lfo.modsense;
	}
	for (int t = 0; t < 4; t++) {
		// Common parameters, stored redundantly
		patchCache[t].partialCount = partialCount;
		patchCache[t].sustain = (timbre->common.nosustain == 0);
		if (isRhythm) {
			patchCache[t].benderRange = 0;
		} else {
			patchCache[t].benderRange = patchTemp->patch.benderRange;
		}
	}
	//synth->printDebug("Res 1: %d 2: %d 3: %d 4: %d", patchCache[0].waveform, patchCache[1].waveform, patchCache[2].waveform, patchCache[3].waveform);

	if (isRhythm)
		memcpy(drumCache[absTimbreNum - 128], patchCache, sizeof(patchCache));
	else
		allStop();
#if MT32EMU_MONITOR_INSTRUMENTS == 1
	synth->printDebug("%s: Recache, param %d (timbre: %s)", name, patchNum, currentInstr);
	for (int i = 0; i < 4; i++) {
		synth->printDebug(" %d: play=%s, pcm=%s (%d), wave=%d", i, patchCache[i].playPartial ? "YES" : "NO", patchCache[i].PCMPartial ? "YES" : "NO", timbre->partial[i].wg.pcmwave, timbre->partial[i].wg.waveform);
	}
#endif
}

char *Part::getName() {
	return name;
}

void Part::setVolume(int vol) {
	volume = voltable[vol];
}

void Part::setPan(int pan) {
	// FIXME:KG: This is unchangeable for drums (they always use drumPan), is that correct?
	// FIXME:KG: Tweaked this a bit so that we have a left 100%, centre and right 100%
	// (But this makes the range somewhat skewed)
	if (pan < 64) {
		volumesetting.leftvol = 32767;
		volumesetting.rightvol = (Bit16s)(pan * 512);
	} else if (pan == 64) {
		volumesetting.leftvol = 32767;
		volumesetting.rightvol = 32767;
	} else {
		volumesetting.rightvol = 32767;
		volumesetting.leftvol = (Bit16s)((127 - pan) * 520);
	}
	//synth->printDebug("%s (%s): Set pan to %d", name, currentInstr, panpot);
}

void Part::playNote(PartialManager *partialManager, unsigned int key, int vel) {
	int drumNum = -1; // Initialised to please compiler
	int drumTimbre = -1; // As above
	int freqNum;

	if (isRhythm) {
		if (key < 24 || key > 87) {
			synth->printDebug("%s: Attempted to play invalid key %d", name, key);
			return;
		}
		drumNum = key - 24;
		drumTimbre = rhythmTemp[drumNum].timbre;
		if (drumTimbre >= 94) {
			synth->printDebug("%s: Attempted to play unmapped key %d", name, key);
			return;
		}
		memcpy(patchCache, drumCache[drumTimbre], sizeof(patchCache));
		memcpy(&currentInstr, synth->mt32ram.params.timbres[128 + drumTimbre].timbre.common.name, 10);
		freqNum = MIDDLEC;
	} else {
		if (key < 12) {
			synth->printDebug("%s (%s): Attempted to play invalid key %d < 12; moving up by octave", name, currentInstr, key);
			key += 12;
		} else if (key > 108) {
			synth->printDebug("%s (%s): Attempted to play invalid key %d > 108; moving down by octave", name, currentInstr, key);
			while (key > 108) {
				key -= 12;
			}
		}
		freqNum = key;
	}
	// POLY1 mode, Single Assign
	// Haven't found any software that uses any of the other poly modes
	// FIXME:KG: Should this also apply to rhythm?
	if (!isRhythm) {
		for (unsigned int i = 0; i < MT32EMU_MAX_POLY; i++) {
			if (polyTable[i].isActive() && (polyTable[i].key == key)) {
				//AbortPoly(&polyTable[i]);
				stopNote(key);
				break;
			}
		}
	}

	unsigned int needPartials = patchCache[0].partialCount;

	if (needPartials > partialManager->GetFreePartialCount()) {
		if (!partialManager->FreePartials(needPartials, partNum)) {
			synth->printDebug("%s (%s): Insufficient free partials to play key %d (vel=%d)", name, currentInstr, key, vel);
			return;
		}
	}
	// Find free poly
	int m;
	for (m = 0; m < MT32EMU_MAX_POLY; m++) {
		if (!polyTable[m].isActive()) {
			break;
		}
	}
	if (m == MT32EMU_MAX_POLY) {
		synth->printDebug("%s (%s): No free poly to play key %d (vel %d)", name, currentInstr, key, vel);
		return;
	}

	dpoly *tpoly = &polyTable[m];

	tpoly->isPlaying = true;
	tpoly->key = key;
	tpoly->isDecay = false;
	tpoly->freqnum = freqNum;
	tpoly->vel = vel;
	tpoly->pedalhold = false;

	bool allnull = true;
	for (int x = 0; x < 4; x++) {
		if (patchCache[x].playPartial) {
			tpoly->partials[x] = partialManager->AllocPartial(partNum);
			allnull = false;
		} else {
			tpoly->partials[x] = NULL;
		}
	}

	if (allnull)
		synth->printDebug("%s (%s): No partials to play for this instrument", name, this->currentInstr);

	if (isRhythm) {
		tpoly->pansetptr = &drumPan[drumNum];
		tpoly->reverb = rhythmTemp[drumNum].reverbSwitch > 0;
	} else {
		tpoly->pansetptr = &volumesetting;
		tpoly->reverb = patchTemp->patch.reverbSwitch > 0;
	}
	tpoly->sustain = patchCache[0].sustain;
	tpoly->volumeptr = &volume;

#if MT32EMU_MONITOR_INSTRUMENTS == 1
	if (isRhythm) {
		synth->printDebug("%s (%s): starting poly %d (drum %d, timbre %d) - Vel %d Key %d Vol %d", name, currentInstr, m, drumNum, drumTimbre, vel, key, volume);
	} else {
		synth->printDebug("%s (%s): starting poly %d - Vel %d Key %d Vol %d", name, currentInstr, m, vel, key, volume);
	}
#endif

	for (int x = 0; x < 4; x++) {
		if (tpoly->partials[x] != NULL) {
			tpoly->partials[x]->startPartial(tpoly, &patchCache[x], tpoly->partials[patchCache[x].structurePair]);
			tpoly->partials[x]->setBend(bend);
		}
	}
}

static void startDecayPoly(dpoly *tpoly) {
	if (tpoly->isDecay) {
		return;
	}
	tpoly->isDecay = true;

	for (int t = 0; t < 4; t++) {
		Partial *partial = tpoly->partials[t];
		if (partial == NULL)
			continue;
		partial->startDecayAll();
	}
	tpoly->isPlaying = false;
}

void Part::allStop() {
	for (int q = 0; q < MT32EMU_MAX_POLY; q++) {
		dpoly *tpoly = &polyTable[q];
		if (tpoly->isPlaying) {
			startDecayPoly(tpoly);
		}
	}
}

void Part::stopPedalHold() {
	for (int q = 0; q < MT32EMU_MAX_POLY; q++) {
		dpoly *tpoly;
		tpoly = &polyTable[q];
		if (tpoly->isActive() && tpoly->pedalhold)
			stopNote(tpoly->key);
	}
}

void Part::stopNote(unsigned int key) {
	// Non-sustaining instruments ignore stop commands.
	// They die away eventually anyway

#if MT32EMU_MONITOR_INSTRUMENTS == 1
	synth->printDebug("%s (%s): stopping key %d", name, currentInstr, key);
#endif

	if (key != 255) {
		for (int q = 0; q < MT32EMU_MAX_POLY; q++) {
			dpoly *tpoly = &polyTable[q];
			if (tpoly->isPlaying && tpoly->key == key) {
				if (holdpedal)
					tpoly->pedalhold = true;
				else if (tpoly->sustain)
					startDecayPoly(tpoly);
			}
		}
		return;
	}

	// Find oldest poly... yes, the MT-32 can be reconfigured to kill different poly first
	// This is simplest
	int oldest = -1;
	Bit64s oldage = -1;

	for (int q = 0; q < MT32EMU_MAX_POLY; q++) {
		dpoly *tpoly = &polyTable[q];

		if (tpoly->isPlaying && !tpoly->isDecay) {
			if (tpoly->getAge() >= oldage) {
				oldage = tpoly->getAge();
				oldest = q;
			}
		}
	}

	if (oldest!=-1) {
		startDecayPoly(&polyTable[oldest]);
	}
}

}
