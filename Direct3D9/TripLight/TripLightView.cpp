// Copyleft 2015 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      25dec15	initial version
 		01		15mar23	add MIDI support
		02		15apr23	move modulation period to header
		03		16apr23	seed with system time, not tick count
		04		02aug23	restore snapshot support
		05		02aug23	export PNG format by default
		06		01apr24	reduce graphics frame rate by half
		07		03apr24	add Direct3D support

		TripLight view
 
*/

// TripLightView.cpp : implementation of the CTripLightView class
//

#include "stdafx.h"
#include "TripLight.h"
#include "MainFrm.h"
#include "atlimage.h"
#include <math.h>
#include "TripLightDoc.h"
#include "TripLightView.h"
#include "Dib.h"
#include "PathStr.h"
#include "FolderDialog.h"
#include "ProgressDlg.h"
#include "SaveObj.h"
#include "Benchmark.h"
#include "dxerr9.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTripLightView

IMPLEMENT_DYNCREATE(CTripLightView, CView)

/////////////////////////////////////////////////////////////////////////////
// CTripLightView construction/destruction

#define BMP_FILE_EXT _T(".bmp")
#define PNG_FILE_EXT _T(".png")
#define EXPORT_FILTER _T("PNG Files (*.png)|*.png|Bitmap Files (*.bmp)|*.bmp|All Files (*.*)|*.*||")
#define SNAPSHOT_FILTER _T("TripLight Snapshots (*.tripsnap)|*.tripsnap|All Files (*.*)|*.*||")

const LPCTSTR CTripLightView::m_arrNoteName[OCTAVE] = {
	_T("C"), _T("Db"), _T("D"), _T("Eb"), _T("E"), _T("F"), 
	_T("Gb"), _T("G"), _T("Ab"), _T("A"), _T("Bb"), _T("B")
};

#define CHECK_HARMONY 1
#define SHOW_PERF_STATS 1
#define EXPORT_MIDI_ONLY 0
#define DUMP_MIDI_TEXT 0
#define USE_DIRECT3D 1
#define REPORT_DEVICE_ERRORS 0

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)
#define CHECK(x) if (FAILED(x)) { OnDeviceError(x); return false; }

CTripLightView::CTripLightView()
{
	m_bPause = false;
	m_bPrePrintPause = false;
	m_bShowMesh = false;
	m_bBackward = false;
	m_nColorSpeedVarDelta = 0;
	ResetMidiData();
	m_bExportMidi = false;
	m_bShowingErrorMsg = false;
}

CTripLightView::~CTripLightView()
{
}

void CTripLightView::ResetMidiData()
{
	ZeroMemory(m_arrActiveNote, sizeof(m_arrActiveNote));
	ZeroMemory(m_arrVolume, sizeof(m_arrVolume));
	ZeroMemory(m_arrTone, sizeof(m_arrTone));
	m_nAlteredMask = USHORT_MAX;
	m_nUnmuteMask = 0;
	m_nHalfStepMask = 0;
	m_bFadeOut = false;
	m_bModulating = false;
	m_iFrame = 0;
	m_iLastModFrame = 0;
	m_nKey = 0;
	m_nModKey = 0;
	m_nModPrunes = 0;
	ZeroMemory(m_arrModTone, sizeof(m_arrModTone));
	m_nModHalfStepMask = 0;
#ifdef _DEBUG
	ZeroMemory(m_arrTestNote, sizeof(m_arrTestNote));
	ZeroMemory(m_arrTestVolume, sizeof(m_arrTestVolume));
	ZeroMemory(m_testScale, sizeof(m_testScale));
	m_nTestKey = -1;
#endif
}

BOOL CTripLightView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

int CTripLightView::Rand(int nVals)
{
	if (nVals <= 0)
		return -1;
	int	i = Trunc(rand() / double(RAND_MAX) * nVals);
	return min(i, nVals - 1);
}

inline double CTripLightView::frand()
{
	return double(rand()) / RAND_MAX;
}

inline COLOR16 CTripLightView::RandColor()
{
	return static_cast<COLOR16>(Rand(COLOR_MAX));
}

inline void CTripLightView::UpdateColor(COLOR16& nChan, int& nDelta)
{
	int	nVal = nChan + nDelta;
	if (nVal < 0 || nVal > COLOR_MAX) {
		nDelta = -nDelta;
		nVal = nChan + nDelta;
	}
	nChan = static_cast<COLOR16>(nVal);
}

inline void CTripLightView::UpdateColorDelta(int& nDelta) const
{
	int	nNewDelta = m_ColorSpeed + Rand(m_nColorSpeedVarDelta + 1);
	if (nDelta < 0)
		nDelta = -nNewDelta;
	else
		nDelta = nNewDelta;
}

void CTripLightView::UpdateColorDeltas()
{
	UpdateColorSpeedVariance();
	int	nVerts = m_arrVert.GetSize();
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		VERTEX_INFO&	info = m_arrVertInfo[iVert];
		UpdateColorDelta(info.nDeltaRed);
		UpdateColorDelta(info.nDeltaGreen);
		UpdateColorDelta(info.nDeltaBlue);
	}
}

inline void CTripLightView::UpdateColorSpeedVariance()
{
	// convert color speed variance from percentage of color speed to color delta
	m_nColorSpeedVarDelta = Round(double(m_ColorSpeed) * m_ColorSpeedVar / 100);
}

void CTripLightView::CreateMesh() 
{
	m_arrVert.RemoveAll();
	UpdateMesh();
}

void CTripLightView::SetGridSize(CSize szGrid)
{
	m_Cols = szGrid.cx;
	m_Rows = szGrid.cy;
	UpdateMesh();
}

void CTripLightView::UpdateMesh()
{
	int	nOldVerts = m_arrVert.GetSize();
	int	nVerts = (m_Cols + 1) * (m_Rows + 1);
	m_arrVert.SetSize(nVerts);
	m_arrVertInfo.SetSize(nVerts);
	int	nTris = m_Cols * m_Rows * 2;
	m_arrTri.SetSize(nTris);
	for (int iVert = nOldVerts; iVert < nVerts; iVert++) {	// for each vertex
		TRIVERTEX&	vert = m_arrVert[iVert];
		VERTEX_INFO&	info = m_arrVertInfo[iVert];
		vert.Red = RandColor();
		vert.Green = RandColor();
		vert.Blue = RandColor();
		vert.Alpha = 0;
		info.nDeltaRed = Rand(2) - 1;
		info.nDeltaGreen = Rand(2) - 1;
		info.nDeltaBlue = Rand(2) - 1;
		UpdateColorDelta(info.nDeltaRed);
		UpdateColorDelta(info.nDeltaGreen);
		UpdateColorDelta(info.nDeltaBlue);
	}
	ResizeMesh();
}

void CTripLightView::ResizeMesh()
{
	CRect	rc;
	GetClientRect(rc);
	ResizeMesh(rc.Size());
}

void CTripLightView::ResizeMesh(CSize szWnd)
{
	if (!m_arrVert.GetSize())
		return;
	int	iVert = 0;
	int	iRow;
	int	nRows = m_Rows;
	int	nCols = m_Cols;
	double	dx = double(szWnd.cx) / nCols;
	double	dy = double(szWnd.cy) / nRows;
	for (iRow = 0; iRow <= nRows; iRow++) {	// for each row
		for (int iCol = 0; iCol <= nCols; iCol++) {	// for each column
			double	x = dx * iCol;
			double	y = dy * iRow;
			m_arrVert[iVert].x = Round(x);
			m_arrVert[iVert].y = Round(y);
			iVert++;
		}
	}
	bool	bAltRows = (m_Pattern & ALT_ROWS) != 0;
	bool	bAltCols = (m_Pattern & ALT_COLS) != 0;
	bool	bRowDiagRL = (m_Pattern & DIAG_RL) != 0;
	int	iTri = 0;
	for (iRow = 0; iRow < nRows; iRow++) {	// for each row
		bool	bColDiagRL = bRowDiagRL;
		for (int iCol = 0; iCol < nCols; iCol++) {	// for each column
			int	tl = iRow * (nCols + 1) + iCol;
			int	tr = tl + 1;
			int	bl = (iRow + 1) * (nCols + 1) + iCol;
			int	br = bl + 1;
			if (bColDiagRL) {	// if diagonal right to left
				m_arrTri[iTri].Vertex1 = tl;
				m_arrTri[iTri].Vertex2 = tr;
				m_arrTri[iTri].Vertex3 = bl;
				iTri++;
				m_arrTri[iTri].Vertex1 = tr;
				m_arrTri[iTri].Vertex2 = br;
				m_arrTri[iTri].Vertex3 = bl;
				iTri++;
			} else {	// diagonal left to right
				m_arrTri[iTri].Vertex1 = tl;
				m_arrTri[iTri].Vertex2 = tr;
				m_arrTri[iTri].Vertex3 = br;
				iTri++;
				m_arrTri[iTri].Vertex1 = tl;
				m_arrTri[iTri].Vertex2 = br;
				m_arrTri[iTri].Vertex3 = bl;
				iTri++;
			}
			if (bAltCols)
				bColDiagRL ^= 1;
		}
		if (bAltRows)
			bRowDiagRL ^= 1;
	}
#if USE_DIRECT3D	// if using Direct3D
	if (m_pDevice != NULL) {	// if device exists
		CreateBuffers();	// create vertex and index buffers
	}
#endif
	Invalidate();
}

bool CTripLightView::CreateTimer(bool bEnable)
{
	if (bEnable == m_FrameTimer.IsCreated())
		return true;
	if (bEnable) {
		if (!m_FrameTimer.Create(TIMER_PERIOD, TIMER_RESOLUTION, TimerCallback, W64ULONG(this), TIME_PERIODIC))
			return false;
	} else {
		if (MIDI_FAILED(m_FrameTimer.Destroy()))
			return false;
	}
	return true;
}

void CTripLightView::MidiFileWrite(DWORD nShortMsg)
{
	CMidiFile::MIDI_EVENT	evt = {m_iFrame, nShortMsg};
	int	iTone = ChannelToTone(MIDI_CHAN(nShortMsg));
	m_arrMidiTrack[iTone].Add(evt);
}

#ifdef _DEBUG
void CTripLightView::CheckHarmony(int nEvent, int nChannel, int nP1, int nP2)
{
	if (m_nKey != m_nTestKey) {	// if key changed
		GetScale(m_nKey, m_testScale);	// update scale
		m_nTestKey = m_nKey;
	}
	switch (nEvent) {
	case CONTROL:	// control event
		if (nP1 == VOLUME) {	// if volume control
			if (!m_arrTestVolume[nChannel] && nP2) {	// if volume transitioning from zero
				BYTE	nTargetNote = m_arrTestNote[nChannel] % OCTAVE;
				if (m_testScale.Find(nTargetNote) < 0) {	// if note isn't in current scale
					_tprintf(_T("ERROR channel %d: %s not in scale\n"), nChannel, GetKeyName(nTargetNote));
					ASSERT(0);
				}
			}
			m_arrTestVolume[nChannel] = static_cast<BYTE>(nP2);	// update our volume shadow
		}
		break;
	case NOTE_ON:	// note event
		if (nP2) {	// if turning note on
			if (m_arrTestNote[nChannel]) {	// if channel already has active note
				_tprintf(_T("ERROR channel %d: %s multiple notes active\n"), nChannel, GetNoteName(nP1));
				ASSERT(0);
			}
			if (m_arrTestVolume[nChannel]) {	// if channel has non-zero volume
				_tprintf(_T("ERROR channel %d: %s non-silent note change\n"), nChannel, GetNoteName(nP1));
				ASSERT(0);
			}
			m_arrTestNote[nChannel] = static_cast<BYTE>(nP1);
		} else {	// turning note off
			if (nP1 != m_arrTestNote[nChannel]) {	// if not channel's active note
				_tprintf(_T("ERROR channel %d: %s unexpected note off\n"), nChannel, GetNoteName(nP1));
				ASSERT(0);
			}
			m_arrTestNote[nChannel] = 0;	// reset channel's active note
		}
		break;
	}
}
#endif

inline void CTripLightView::MidiOut(int nEvent, int nChannel, int nP1, int nP2)
{
#ifdef _DEBUG
#if CHECK_HARMONY
	CheckHarmony(nEvent, nChannel, nP1, nP2);
#endif
#endif
	DWORD	nShortMsg = MakeMidiMsg(nEvent, nChannel, nP1, nP2);
	if (m_bExportMidi)
		MidiFileWrite(nShortMsg);
	else
		theApp.MidiOut(nShortMsg);
}

inline void CTripLightView::MidiNoteOut(int nChannel, int nNote)
{
	BYTE	nOldNote = m_arrActiveNote[nChannel];
	if (nOldNote)	// if old note is active
		MidiOut(NOTE_ON, nChannel, nOldNote, 0);	// turn old note off
	MidiOut(NOTE_ON, nChannel, nNote, NOTE_VELOCITY);	// turn new note on
	m_arrActiveNote[nChannel] = static_cast<BYTE>(nNote);
}

inline int CTripLightView::ToneToChannel(int iTone)
{
	int	iChan = iTone;	// tones map to MIDI channels, but with one exception
	if (iTone >= DRUM_CHANNEL)	// if drum channel or higher
		iChan++;	// skip drum channel
	return iChan;
}

inline int CTripLightView::ChannelToTone(int iChan)
{
	int	iTone = iChan;	// tones map to MIDI channels, but with one exception
	if (iTone > DRUM_CHANNEL)	// if above drum channel
		iTone--;	// account for skipping drum channel in ToneToChannel
	return iTone;
}

int CTripLightView::ByteSortCompareAscending(const void *elem1, const void *elem2)
{
	const BYTE	*a = (const BYTE *)elem1;
	const BYTE	*b = (const BYTE *)elem2;
	return(*a < *b ? -1 : (*a > *b ? 1 : 0));
}

void CTripLightView::DumpTones(const CChord& arrTone)
{
	int	nTones = arrTone.GetSize();
	for (int iTone = 0; iTone < nTones; iTone++) {	// for each tone
		_tprintf(_T("%s "), GetNoteName(arrTone[iTone]));
	}
	printf("\n");
}

inline void CTripLightView::GetScale(int nKey, CScale& scale)
{
	const BYTE	arrNaturalScale[] = {0, 2, 4, 5, 7, 9, 11};
	for (int iScaleTone = 0; iScaleTone < _countof(arrNaturalScale); iScaleTone++) {
		scale[iScaleTone] = (arrNaturalScale[iScaleTone] + nKey) % OCTAVE;
	}
}

USHORT CTripLightView::GetChord(int nKey, int nMode, CChord& arrTone)
{
	CScale	scale;
	GetScale(nKey, scale);
	qsort(scale, SCALE_TONES, sizeof(BYTE), ByteSortCompareAscending);
	const int	nChordInterval = 2;	// in scale tones
	const UINT	nFlatMask = 0x52b;	// bitmask for flat keys: C, F, Bb, Eb, Ab, Db
	if (!(nFlatMask & (1 << nKey)))	// if sharp key
		nMode--;	// smooth transition from flat to sharp keys
	USHORT	nHalfStepMask = 0;
	for (int iTone = 0; iTone < CHORD_TONES; iTone++) {	// for each chord tone
		int	iScaleTone = iTone * nChordInterval + nMode;
		int	nScaleNote = scale[iScaleTone % SCALE_TONES];
		int	nNextScaleNote = scale[(iScaleTone + 1) % SCALE_TONES];
		int	nInterval = AbsLeastInterval(nScaleNote, nNextScaleNote);	// in semitones
		if (nInterval == 1)	// if scale's next note is a semi-tone away
			nHalfStepMask |= MakeToneBit(iTone);	// set tone's bit in half-step mask
		int	nNote = nScaleNote + OCTAVE * (iScaleTone / SCALE_TONES) + CHORD_START;
		arrTone[iTone] = static_cast<BYTE>(nNote);
	}
	return nHalfStepMask;
}

BYTE CTripLightView::GetToneNote(const CChord& arrTone, USHORT nHalfStepMask, USHORT nAlteredMask, int iTone)
{
	BYTE	nNote = arrTone[iTone];
	UINT	nToneBit = MakeToneBit(iTone);	// make bitmask for this tone
	if (nAlteredMask & nToneBit) {	// if tone is altered
		if (nHalfStepMask & nToneBit) {	// if altering by a semitone
			return nNote + 1;
		} else {	// altering by whole tone
			return nNote + 2;
		}
	}
	return nNote;	// unaltered note
}

inline BYTE CTripLightView::GetTargetToneNote(int iTone) const
{
	return GetToneNote(m_arrModTone, m_nModHalfStepMask, m_nAlteredMask, iTone);
}

void CTripLightView::MidiInit()
{
	ResetMidiData();
	CTripLightDoc	*pDoc = GetDocument();
	theApp.OpenMidiInputDevice(pDoc->m_options.m_MidiInDev);
	if (theApp.OpenMidiOutputDevice(pDoc->m_options.m_MidiOutDev)) {
		m_nHalfStepMask = GetChord(m_nKey, CHORD_MODE, m_arrTone);
		for (int iTone = 0; iTone < CHORD_TONES; iTone++) {	// for each chord tone
			int	iChan = ToneToChannel(iTone);
			MidiOut(PATCH, iChan, PATCH_NUMBER);	// set patch
			MidiOut(CONTROL, iChan, VOLUME);	// zero volume
			int	nPan = iTone / 2 * PAN_STEP_SIZE;	// higher tones are spread wider
			if (iTone % 2)	// if odd tone
				nPan = -nPan;	// pan to opposite side
			MidiOut(CONTROL, iChan, PAN, nPan + 0x40);	// set panning
			int	nNote = GetToneNote(iTone);
			MidiNoteOut(iChan, nNote);
		}
	}
}

LRESULT CTripLightView::OnFrameTimer(WPARAM wParam, LPARAM lParam) 
{
#if SHOW_PERF_STATS
	static double g_fSum;
	static double g_fMin = 1e10;
	static double g_fMax;
	static int	g_nSamps;
	CBenchmark b;
#endif
	if (m_bPause)
		return 0;	// nothing to do
	int	nVerts = m_arrVert.GetSize();
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		TRIVERTEX&	vert = m_arrVert[iVert];
		VERTEX_INFO&	info = m_arrVertInfo[iVert];
		UpdateColor(vert.Red, info.nDeltaRed);
		UpdateColor(vert.Green, info.nDeltaGreen);
		UpdateColor(vert.Blue, info.nDeltaBlue);
	}
//	DumpColorDeltas();
	if (!(m_iFrame & 1)) {	// reduce graphics frame by half
#if USE_DIRECT3D	// if using Direct3D
		Draw3D();	// draw before MIDI output, to space frames as evenly as possible
#else	// using GDI
		Invalidate();
#endif
	}
	if (theApp.IsMidiOutputDeviceOpen()) {	// if MIDI output device is open
		if (!m_bModulating) {	// if not modulating
			if (m_iFrame - m_iLastModFrame >= MODULATION_PERIOD) {	// if modulation due
				m_bModulating = true;
				m_nModKey = (m_nKey + 5) % OCTAVE;
				m_nModHalfStepMask = GetChord(m_nModKey, CHORD_MODE, m_arrModTone);
				m_nModPrunes = Rand(MAX_MOD_PRUNES + 1);
			}
		}
		bool	bSpreadVerts = nVerts >= 9;	// need at least nine vertices to spread them
		for (int iTone = 0; iTone < CHORD_TONES; iTone++) {	// for each chord tone
			int	iVert = iTone / 3;	// convert tone index to vertex index
			if (bSpreadVerts && iVert < 4)	// if spreading vertices and in range
				iVert = (3 - iVert) * 2 + 1;	// use first four odd vertices
			int	iColorChannel = iTone % 3;	// remainder is color channel index
			const TRIVERTEX&	vert = m_arrVert[iVert];	// vertex contains current color channel values
			const BYTE	*pVertByte = reinterpret_cast<const BYTE *>(&vert);	// access color values as array
			const COLOR16	*pColor = reinterpret_cast<const COLOR16 *>(pVertByte + offsetof(TRIVERTEX, Red));
			BYTE	nVolume = pColor[iColorChannel] >> 9;	// convert 16-bit color channel value to 7-bit volume
			if (nVolume != m_arrVolume[iTone]) {	// if tone's volume changed
				m_arrVolume[iTone] = nVolume;	// update volume shadow
				int	iChan = ToneToChannel(iTone);
				UINT	nToneBit = MakeToneBit(iTone);	// make bitmask for this tone
				if (m_nUnmuteMask & nToneBit) {	// if tone is unmuted
					MidiOut(CONTROL, iChan, VOLUME, nVolume);	// set tone's volume
				}
				if (!nVolume) {	// if tone is silent
					if (m_bModulating) {	// if modulating
						TryModulation(iTone);
					} else {	// not modulating
						if (m_bFadeOut)	// if fading out
							m_nUnmuteMask &= ~nToneBit;	// clear muted bit
						else	// fading in
							m_nUnmuteMask |= nToneBit;	// set muted bit
						m_nAlteredMask ^= nToneBit;	// toggle tone's altered state
						BYTE	nNewNote = GetToneNote(iTone);
						MidiNoteOut(iChan, nNewNote);
					}
				}
			}
		}
	}
#if SHOW_PERF_STATS
	if (!(m_iFrame & 1)) {	// reduce graphics frame by half
		double t = b.Elapsed();
		g_fSum += t;
		if (t > g_fMax) g_fMax = t;
		if (t < g_fMin) g_fMin = t;
		g_nSamps++;
		if (g_nSamps >= 1000 / TIMER_PERIOD) {	// update per second
			printf("%f %f %f\n", g_fMin, g_fMax, g_fSum / g_nSamps);
			g_fSum = 0;
			g_fMax = 0;
			g_fMin = 1e10;
			g_nSamps = 0;
		}
	}
#endif
	m_iFrame++;
	return 0;
}

bool CTripLightView::TryModulation(int iSilentTone)
{
	int	nConflicts = 0;
	{
		UINT	nToneBit = MakeToneBit(iSilentTone);	// make bitmask for this tone
		m_nAlteredMask ^= nToneBit;	// toggle tone's altered state
		int	iChan = ToneToChannel(iSilentTone);
		BYTE	nNewNote = GetTargetToneNote(iSilentTone);
		MidiNoteOut(iChan, nNewNote);
		BYTE	nOldNote = GetToneNote(iSilentTone);
		if (nNewNote != nOldNote) {	// if target note differs from current note
			if (m_nUnmuteMask & nToneBit) {	// if tone is unmuted
				m_nUnmuteMask &= ~nToneBit;	// mute tone
				m_nModPrunes--;	// one less prune
				nConflicts++;	// add a conflict
			}
		} else {	// target note matches current note
			if (m_nModPrunes > 0) {	// if prunes remain
				m_nUnmuteMask &= ~nToneBit;	// mute tone
				m_nModPrunes--;	// one less prune
			}
		}
	}
	for (int iTone = 0; iTone < CHORD_TONES; iTone++) {	// for each chord tone
		if (iTone != iSilentTone) {	// if tone other than caller's argument
			int	nOldNote = GetToneNote(iTone);
			int	nNewNote = GetTargetToneNote(iTone);
			if (nNewNote != nOldNote) {	// target note differs from current note
				if (m_nUnmuteMask & MakeToneBit(iTone)) {	// if tone is unmuted
					nConflicts++;	// add a conflict
				}
			}
		}
	}
	if (nConflicts)	// if one or more conflicts
		return false;	// modulation incomplete
	m_nKey = m_nModKey;
	m_arrTone = m_arrModTone;
	m_nHalfStepMask = m_nModHalfStepMask;
	m_bModulating = false;
	m_bFadeOut = false;
	m_iLastModFrame = m_iFrame;
	UpdateColorDeltas();
	return true;
}

bool CTripLightView::SetPause(bool bEnable)
{
	if (!CreateTimer(!bEnable))
		return false;
	m_bPause = bEnable;
	return true;
}

void CTripLightView::DumpColorDeltas()
{
	int	nVerts = m_arrVert.GetSize();
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		VERTEX_INFO&	info = m_arrVertInfo[iVert];
		printf("(%d %d %d)", info.nDeltaRed, info.nDeltaGreen, info.nDeltaBlue);
	}
	printf("\n");
}

void CTripLightView::SetColorSpeed(int nSpeed)
{
	nSpeed = CLAMP(nSpeed, SPEED_MIN, SPEED_MAX);
	if (nSpeed == m_ColorSpeed)
		return;	// nothing to do
	m_ColorSpeed = nSpeed;
	UpdateColorDeltas();
}

void CTripLightView::SetColorSpeedVariance(int nVariance)
{
	nVariance = CLAMP(nVariance, SPEED_MIN, SPEED_MAX);
	if (nVariance == m_ColorSpeedVar)
		return;	// nothing to do
	m_ColorSpeedVar = nVariance;
	UpdateColorDeltas();
}

void CTripLightView::ReverseColorDeltas()
{
	int	nVerts = m_arrVert.GetSize();
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		VERTEX_INFO&	info = m_arrVertInfo[iVert];
		info.nDeltaRed = -info.nDeltaRed;
		info.nDeltaGreen = -info.nDeltaGreen;
		info.nDeltaBlue = -info.nDeltaBlue;
	}
}

void CTripLightView::SetBackward(bool bEnable)
{
	if (bEnable == m_bBackward)
		return;
	ReverseColorDeltas();
	m_bBackward = bEnable;
}

bool CTripLightView::ExportBitmap(LPCTSTR pszPath, CSize szImg)
{
	CDib	dib;
	if (!GetBitmap(szImg, dib))
		return false;
	if (!dib.Write(pszPath))
		return false;
	return true;
}

bool CTripLightView::ExportPNG(LPCTSTR pszPath, CSize szImg)
{
	CDib	dib;
	if (!GetBitmap(szImg, dib))
		return false;
	CImage	image;
	image.Attach(dib);
	HRESULT	hr = image.Save(pszPath, Gdiplus::ImageFormatPNG);
	image.Detach();
	return SUCCEEDED(hr);
}

bool CTripLightView::GetBitmap(CSize szImg, CDib& dibOut)
{
	if (!dibOut.Create(szImg.cx, szImg.cy, 24))
		return false;
	CDC	dc;
	if (!dc.CreateCompatibleDC(NULL))
		return false;
	HGDIOBJ	hPrevBmp = dc.SelectObject(dibOut);
	ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
	MoveWindow(CRect(CPoint(0, 0), szImg));
	Draw(&dc);
	ModifyStyleEx(0, WS_EX_CLIENTEDGE);
	dc.SelectObject(hPrevBmp);
	GetParentFrame()->RecalcLayout();
	return true;
}

bool CTripLightView::CopyDIBToClipboard(const CDib& dibIn, HWND hWnd)
{
	DIBSECTION	ds;
	if (!GetObject(dibIn, sizeof(DIBSECTION), &ds))
		return false;
	if (!(ds.dsBmih.biCompression == BI_RGB && ds.dsBmih.biSizeImage))
		return false;	// compressed format, or image size is zero
	bool	retc = false;
	if (::OpenClipboard(hWnd)) {
		if (EmptyClipboard()) {
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 
				sizeof(BITMAPINFOHEADER) + ds.dsBmih.biSizeImage);
			if (hMem != NULL) {
				BYTE	*pMem = (BYTE *)GlobalLock(hMem);
				if (pMem != NULL) {
					CopyMemory(pMem, &ds.dsBmih, sizeof(BITMAPINFOHEADER));
					CopyMemory(pMem + sizeof(BITMAPINFOHEADER), dibIn.GetBits(), 
						ds.dsBmih.biSizeImage);
					GlobalUnlock(pMem);
					if (SetClipboardData(CF_DIB, hMem) != NULL)
						retc = true;	// success
				} else
					GlobalFree(hMem);
			}
		}
		CloseClipboard();
	}
	return retc;
}

CString CTripLightView::GetDefaultFileName()
{
	CPathStr sName(GetDocument()->GetTitle());
	if (sName == LDS(AFX_IDS_UNTITLED))
		sName.Empty();
	else
		sName.RemoveExtension();
	return sName;
}

CSize CTripLightView::GetExportSize() const
{
	if (theApp.m_settings.m_ImageSizeType == CSettings::IMAGE_SAME_SIZE_AS_WINDOW) {
		CRect	rc;
		GetClientRect(rc);
		return rc.Size();
	} else {
		return CSize(theApp.m_settings.m_ImageWidth, theApp.m_settings.m_ImageHeight);
	}
}

CString CTripLightView::GetNoteName(int nNote)
{
	CString	sOctave;
	sOctave.Format(_T("%d"), nNote / OCTAVE - 1);
	return m_arrNoteName[nNote % OCTAVE] + sOctave;
}

LPCTSTR CTripLightView::GetKeyName(int nNote)
{
	return m_arrNoteName[nNote % OCTAVE];
}

int CTripLightView::AbsLeastInterval(int nNote1, int nNote2)
{
	ASSERT(nNote1 >= 0 && nNote1 < OCTAVE);	// input notes must be normalized
	ASSERT(nNote2 >= 0 && nNote2 < OCTAVE);
	int	nInterval = abs(nNote1 - nNote2);
	if (nInterval > OCTAVE / 2)	// if interval exceeds a tritone
		nInterval = OCTAVE - nInterval;	// invert interval
	return(nInterval);
}

int CTripLightView::WildcardDeleteFile(CString sPath)
{
	// Note that the destination path is double-null terminated. CString's
	// get buffer method allocates the specified number of characters plus
	// one for the null terminator, but we need space for two terminators,
	// hence we must increment nPathLen.
	int	nPathLen = sPath.GetLength();
	LPTSTR	pszPath = sPath.GetBufferSetLength(nPathLen + 1);
	pszPath[nPathLen + 1] = '\0';	// double-null terminated string
	SHFILEOPSTRUCT	SHFileOp;
	ZeroMemory(&SHFileOp, sizeof(SHFileOp));
	SHFileOp.wFunc = FO_DELETE;
	SHFileOp.pFrom = pszPath;
	SHFileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_FILESONLY | FOF_NORECURSION;
	return SHFileOperation(&SHFileOp);
}

CTripLightView::CPause::CPause(CTripLightView& View, bool bEnable) : m_View(View)
{
	m_bPrevPause = View.GetPause();	// save state
	View.SetPause(bEnable);	// enter new state
	// if showing modal dialog, frame's UI handlers aren't called
	theApp.GetMain()->UpdateCmdUI();	//  explicitly update UI
}

CTripLightView::CPause::~CPause()
{
	m_View.SetPause(m_bPrevPause);	// restore previous state
}

void CTripLightView::SaveSnapshot(LPCTSTR Path)
{
	CFile	fp(Path, CFile::modeCreate | CFile::modeWrite);
	CArchive	ar(&fp, CArchive::store);
	SerializeSnapshot(ar);
	int	nVerts = m_arrVert.GetSize();
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		const TRIVERTEX&	vert = m_arrVert[iVert];
		ar << vert.Red;
		ar << vert.Green;
		ar << vert.Blue;
		const VERTEX_INFO&	info = m_arrVertInfo[iVert];
		ar << info.nDeltaRed;
		ar << info.nDeltaGreen;
		ar << info.nDeltaBlue;
	}
}

void CTripLightView::LoadSnapshot(LPCTSTR Path)
{
	theApp.GetMainWnd()->SendMessage(WM_COMMAND, ID_FILE_NEW);
	CFile	fp(Path, CFile::modeRead);
	CArchive	ar(&fp, CArchive::load);
	GetDocument()->SerializeSnapshot(ar);
	OnInitialUpdate();	// order matters; resizes arrays
	int	nVerts = m_arrVert.GetSize();
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		TRIVERTEX&	vert = m_arrVert[iVert];
		ar >> vert.Red;
		ar >> vert.Green;
		ar >> vert.Blue;
		VERTEX_INFO&	info = m_arrVertInfo[iVert];
		ar >> info.nDeltaRed;
		ar >> info.nDeltaGreen;
		ar >> info.nDeltaBlue;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTripLightView message map

BEGIN_MESSAGE_MAP(CTripLightView, CView)
	ON_MESSAGE(UWM_FRAME_TIMER, OnFrameTimer)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_IMAGE_COLS_LESS, OnImageColsLess)
	ON_COMMAND(ID_IMAGE_COLS_MORE, OnImageColsMore)
	ON_COMMAND(ID_IMAGE_ROWS_LESS, OnImageRowsLess)
	ON_COMMAND(ID_IMAGE_ROWS_MORE, OnImageRowsMore)
	ON_COMMAND(ID_IMAGE_SPEED_FASTER, OnImageColorSpeedFaster)
	ON_COMMAND(ID_IMAGE_SPEED_SLOWER, OnImageColorSpeedSlower)
	ON_COMMAND(ID_IMAGE_VARIANCE_MORE, OnImageColorSpeedVarianceMore)
	ON_COMMAND(ID_IMAGE_VARIANCE_LESS, OnImageColorSpeedVarianceLess)
	ON_COMMAND(ID_IMAGE_COLOR_JUMP, OnImageColorJump)
	ON_COMMAND(ID_VIEW_PAUSE, OnViewPause)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PAUSE, OnUpdateViewPause)
	ON_COMMAND(ID_VIEW_SINGLE_STEP, OnViewSingleStep)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SINGLE_STEP, OnUpdateViewSingleStep)
	ON_COMMAND(ID_VIEW_BACKWARD, OnViewBackward)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BACKWARD, OnUpdateViewBackward)
	ON_COMMAND(ID_VIEW_SHOW_MESH, OnViewShowMesh)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_MESH, OnUpdateViewShowMesh)
	ON_COMMAND_RANGE(ID_IMAGE_PATTERN1, ID_IMAGE_PATTERN8, OnImagePattern) 
	ON_UPDATE_COMMAND_UI_RANGE(ID_IMAGE_PATTERN1, ID_IMAGE_PATTERN8, OnUpdateImagePattern) 
	ON_COMMAND(ID_FILE_RECORD, OnFileRecord)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_MESSAGE(UWM_MAPPING_CHANGE, OnMappingChange)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_COMMAND(ID_TOOLS_FADE, OnToolsFade)
	ON_COMMAND(ID_FILE_SAVE_SNAPSHOT, OnFileSaveSnapshot)
	ON_COMMAND(ID_FILE_LOAD_SNAPSHOT, OnFileLoadSnapshot)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTripLightView drawing

void CTripLightView::Draw(CDC* pDC)
{
	CTripLightDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (m_bShowMesh) {	// if showing mesh
		int	nTris = m_arrTri.GetSize();
		for (int iTri = 0; iTri < nTris; iTri++) {	// for each triangle
			const GRADIENT_TRIANGLE& tri = m_arrTri[iTri];
			CPoint	pa[3] = {
				CPoint(m_arrVert[tri.Vertex1].x, m_arrVert[tri.Vertex1].y),
				CPoint(m_arrVert[tri.Vertex2].x, m_arrVert[tri.Vertex2].y),
				CPoint(m_arrVert[tri.Vertex3].x, m_arrVert[tri.Vertex3].y),
			};
			pDC->Polygon(pa, _countof(pa));
		}
	} else {	// not showing mesh; fill triangles
		GradientFill(*pDC, m_arrVert.GetData(), m_arrVert.GetSize(), 
			m_arrTri.GetData(), m_arrTri.GetSize(), GRADIENT_FILL_TRIANGLE);
	}
}

void CTripLightView::OnDraw(CDC* pDC)
{
#if USE_DIRECT3D	// if using Direct3D
	if (m_bPause) {	// only draw if paused
		Draw3D();
	}
#else	// using GDI
	Draw(pDC);
#endif
}

void CTripLightView::GetPresentationParameters(int cx, int cy, D3DPRESENT_PARAMETERS& pp)
{
	ZeroMemory(&pp, sizeof(pp));
	pp.Windowed = true;	
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	pp.BackBufferWidth = cx;
	pp.BackBufferHeight = cy;
}

bool CTripLightView::CreateDevice(int cx, int cy)
{
//	printf("CreateDevice %d %d\n", cx, cy);
	m_pDevice = NULL;	// destroy device
	D3DPRESENT_PARAMETERS	pp;
	GetPresentationParameters(cx, cy, pp);
	DWORD	nFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	CHECK(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, nFlags, &pp, &m_pDevice));
	return InitDevice();
}

void CTripLightView::DestroyDevice()
{
	m_pVertBuf = NULL;
	m_pIdxBuf = NULL;
	m_pDevice = NULL;
}

bool CTripLightView::InitDevice()
{
	CHECK(m_pDevice->SetFVF(D3DFVF_CUSTOMVERTEX));
	return true;
}

bool CTripLightView::CreateBuffers()
{
	m_pVertBuf = NULL;	// destroy vertex buffer
	m_pIdxBuf = NULL;	// destroy index buffer
	int	nVerts = m_arrVert.GetSize();
	int	nTris = m_arrTri.GetSize();
	CHECK(m_pDevice->CreateVertexBuffer(nVerts * sizeof(VERTEX_3D), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &m_pVertBuf, NULL));
	CHECK(m_pDevice->CreateIndexBuffer(nTris * sizeof(GRADIENT_TRIANGLE), 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &m_pIdxBuf, NULL));
	VOID	*pBufData;
	CHECK(m_pVertBuf->Lock(0, 0, &pBufData, 0));	// lock vertex buffer
	VERTEX_3D	*pVert = (VERTEX_3D *)pBufData;
	for (int iVert = 0; iVert < nVerts; iVert++, pVert++) {	// for each vertex
		const TRIVERTEX&	vert = m_arrVert[iVert];
		pVert->x = float(vert.x);
		pVert->y = float(vert.y);
		pVert->z = 0;
		pVert->w = 1;
		pVert->color = D3DCOLOR_XRGB(HIBYTE(vert.Red), HIBYTE(vert.Green), HIBYTE(vert.Blue));
	}
	CHECK(m_pVertBuf->Unlock());	// unlock vertex buffer
	CHECK(m_pIdxBuf->Lock(0, 0, &pBufData, 0));	// lock index buffer
	memcpy(pBufData, m_arrTri.GetData(), nTris * sizeof(GRADIENT_TRIANGLE));	// copy indices to buffer
	CHECK(m_pIdxBuf->Unlock());	// unlock index buffer
	CHECK(m_pDevice->SetStreamSource(0, m_pVertBuf, 0, sizeof(VERTEX_3D)));	// pass vertex buffer to device
	CHECK(m_pDevice->SetIndices(m_pIdxBuf));	// pass index buffer to device
	return true;
}

bool CTripLightView::Draw3D()
{
	if (m_pDevice != NULL) {	// if device exists
		int	nVerts = m_arrVert.GetSize();
		int	nTris = m_arrTri.GetSize();
		VOID	*pBufData;
		CHECK(m_pVertBuf->Lock(0, 0, &pBufData, 0));
		VERTEX_3D	*pVert = (VERTEX_3D *)pBufData;
		for (int iVert = 0; iVert < nVerts; iVert++, pVert++) {	// for each vertex
			const TRIVERTEX&	vert = m_arrVert[iVert];
			pVert->color = D3DCOLOR_XRGB(HIBYTE(vert.Red), HIBYTE(vert.Green), HIBYTE(vert.Blue));
		}
		CHECK(m_pVertBuf->Unlock());
		CHECK(m_pDevice->BeginScene());
		CHECK(m_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, nVerts, 0, nTris));
		CHECK(m_pDevice->EndScene());
		CHECK(m_pDevice->Present(NULL, NULL, NULL, NULL));
	} else {	// device error
		if (!(m_iFrame & 0x3f)) {	// retry around once per second
			return ResizeDevice();
		}
	}
	return true;
}

bool CTripLightView::ResizeDevice(int cx, int cy)
{
//	printf("ResizeDevice %d %d\n", cx, cy);
	if (m_pDevice != NULL) {	// if device exists
		D3DPRESENT_PARAMETERS	pp;
		GetPresentationParameters(cx, cy, pp);
		CHECK(m_pDevice->Reset(&pp));
		if (!InitDevice())
			return false;
	} else {	// device doesn't exist
		if (!CreateDevice(cx, cy))
			return false;
	}
	return CreateBuffers();	// create vertex and index buffers
}

bool CTripLightView::ResizeDevice()
{
	CRect	rc;
	GetClientRect(rc);
	return ResizeDevice(rc.Width(), rc.Height());
}

void CTripLightView::OnDeviceError(HRESULT nError)
{
	m_pDevice = NULL;	// destroy device
	if (REPORT_DEVICE_ERRORS) {
		if (!m_bShowingErrorMsg) {	// if not already showing error message
			m_bShowingErrorMsg = true;	// set reentry guard
			CString	sMsg;
			sMsg.Format(_T("Direct3d error: %s\n%s (0x%x)"), DXGetErrorString9(nError), 
				DXGetErrorString9(nError), nError);
			AfxMessageBox(sMsg);	// display error message
			m_bShowingErrorMsg = false;	// reset reentry guard
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTripLightView printing

BOOL CTripLightView::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

void CTripLightView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	m_bPrePrintPause = m_bPause;
	m_bPause = true;
}

void CTripLightView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	m_bPause = m_bPrePrintPause;
	GetParentFrame()->RecalcLayout();
}

void CTripLightView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	MoveWindow(pInfo->m_rectDraw);
	CView::OnPrint(pDC, pInfo);
}

/////////////////////////////////////////////////////////////////////////////
// CTripLightView diagnostics

#ifdef _DEBUG
void CTripLightView::AssertValid() const
{
	CView::AssertValid();
}

void CTripLightView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTripLightDoc* CTripLightView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTripLightDoc)));
	return (CTripLightDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTripLightView message handlers

int CTripLightView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	STATIC_DOWNCAST(CFrameWnd, AfxGetMainWnd())->SetActiveView(this);
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (MIDI_FAILED(m_TimerPeriod.Create(1))) {
		AfxMessageBox(_T("Can't create timer period."));
		return false;
	}
#if USE_DIRECT3D	// if using Direct3D
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);	// create Direct3D interface
	if (m_pD3D == NULL) {	// if create failed
		AfxMessageBox(_T("Can't create Direct3D."));
		return false;
	}
#endif
	if (!CreateTimer(true)) {
		AfxMessageBox(_T("Can't create frame timer."));
		return false;
	}
	return 0;
}

void CTripLightView::OnDestroy()
{
#if USE_DIRECT3D	// if using Direct3D
	DestroyDevice();
	m_pD3D = NULL;	// destroy Direct3D interface
#endif
	CView::OnDestroy();
}

void CTripLightView::OnInitialUpdate() 
{
	CTripLightDoc	*pDoc = GetDocument();
	CRect	rc;
	UINT	nRandomSeed = pDoc->m_options.m_RandomSeed;
	if (!nRandomSeed) {	// if random number seed not specified
		nRandomSeed = static_cast<UINT>(time(NULL));	// use system time so every run is different
	}
	srand(nRandomSeed);	// set random number seed; do this before calling base class
	CView::OnInitialUpdate();
	MidiInit();
	if (pDoc->m_options.m_StartFullScreen > 0 && !theApp.GetMain()->IsFullScreen()) {
		theApp.GetMain()->PostMessage(WM_COMMAND, ID_WINDOW_FULL_SCREEN);
	}
}

void CTripLightView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CTripLightDoc	*pDoc = GetDocument();
	switch (lHint) {
	case CTripLightDoc::HINT_OPTIONS:
		MidiInit();
		break;
	default:
		CTripLightParams&	params = *this;	// upcast to base struct
		params = *pDoc;	// copy document's parameters to ours
		UpdateColorSpeedVariance();
		CreateMesh();
	}
}

BOOL CTripLightView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CTripLightView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (m_arrVert.GetSize()) {
#if USE_DIRECT3D	// if using Direct3D
		ResizeDevice(cx, cy);
#endif
		ResizeMesh(CSize(cx, cy));
	}
}

void CALLBACK CTripLightView::TimerCallback(UINT uID, UINT uMsg, W64ULONG dwUser, W64ULONG dw1, W64ULONG dw2)
{
	CTripLightView	*pDlg = (CTripLightView *)dwUser;
	pDlg->PostMessage(UWM_FRAME_TIMER);
}

void CTripLightView::OnImageColsLess() 
{
	if (m_Cols > 1) {
		m_Cols--;
		UpdateMesh();
		GetDocument()->SetModifiedFlag();
	}
}

void CTripLightView::OnImageColsMore() 
{
	if (m_Cols < COLS_MAX) {
		m_Cols++;
		UpdateMesh();
		GetDocument()->SetModifiedFlag();
	}
}

void CTripLightView::OnImageRowsLess() 
{
	if (m_Rows > 1) {
		m_Rows--;
		UpdateMesh();
		GetDocument()->SetModifiedFlag();
	}
}

void CTripLightView::OnImageRowsMore() 
{
	if (m_Rows < ROWS_MAX) {
		m_Rows++;
		UpdateMesh();
		GetDocument()->SetModifiedFlag();
	}
}

void CTripLightView::OnImagePattern(UINT nID)
{
	int	iPat = nID - ID_IMAGE_PATTERN1;
	ASSERT(iPat >= 0 && iPat < PATTERNS);
	m_Pattern = iPat;
	ResizeMesh();
	GetDocument()->SetModifiedFlag();
}

void CTripLightView::OnUpdateImagePattern(CCmdUI* pCmdUI)
{
	int	iPat = pCmdUI->m_nID - ID_IMAGE_PATTERN1;
	ASSERT(iPat >= 0 && iPat < PATTERNS);
	pCmdUI->SetRadio(m_Pattern == iPat);
}

void CTripLightView::OnImageColorSpeedFaster() 
{
	SetColorSpeed(m_ColorSpeed + m_ColorSpeedStep);
	GetDocument()->SetModifiedFlag();
}

void CTripLightView::OnImageColorSpeedSlower() 
{
	SetColorSpeed(m_ColorSpeed - m_ColorSpeedStep);
	GetDocument()->SetModifiedFlag();
}

void CTripLightView::OnImageColorSpeedVarianceMore() 
{
	SetColorSpeedVariance(m_ColorSpeedVar + m_ColorSpeedVarStep);
	GetDocument()->SetModifiedFlag();
}

void CTripLightView::OnImageColorSpeedVarianceLess() 
{
	SetColorSpeedVariance(m_ColorSpeedVar - m_ColorSpeedVarStep);
	GetDocument()->SetModifiedFlag();
}

void CTripLightView::OnViewPause() 
{
	SetPause(!m_bPause);
}

void CTripLightView::OnUpdateViewPause(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bPause);
}

void CTripLightView::OnImageColorJump() 
{
	int	nVerts = m_arrVert.GetSize();
	for (int iVert = 0; iVert < nVerts; iVert++) {	// for each vertex
		TRIVERTEX&	vert = m_arrVert[iVert];
		vert.Red = RandColor();
		vert.Green = RandColor();
		vert.Blue = RandColor();
	}
	Invalidate();
}

void CTripLightView::OnViewShowMesh() 
{
	m_bShowMesh ^= 1;
}

void CTripLightView::OnUpdateViewShowMesh(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bShowMesh);
}

void CTripLightView::OnViewBackward() 
{
	SetBackward(!m_bBackward);
}

void CTripLightView::OnUpdateViewBackward(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bBackward);
}

void CTripLightView::OnViewSingleStep() 
{
	m_bPause = false;
	OnFrameTimer(0, 0);
	m_bPause = true;
}

void CTripLightView::OnUpdateViewSingleStep(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bPause);
}

void CTripLightView::OnToolsFade()
{
	m_bFadeOut ^= 1;
}

LRESULT CTripLightView::OnMappingChange(WPARAM wParam, LPARAM lParam)
{
	static bool bPrevJump;
	if (LOBYTE(wParam) < 0xf0) {	// if not system exclusive
		int	nChan = wParam & 0x0f;
		int	nCmd = (wParam >> 4) & 0x0f;
		int	nCtrl = (wParam >> 8) & 0x7f;
		int	nData = (wParam >> 16) & 0x7f;
		CTripLightDoc	*pDoc = GetDocument();
		int	nMappings = pDoc->m_arrMapping.GetSize();
		int	nEvent = nCmd - 8;
		for (int iMap = 0; iMap < nMappings; iMap++) {	// for each mapping
			const CMapping& map = pDoc->m_arrMapping[iMap];
			if (nEvent == map.m_Event && nChan == map.m_Channel && nCtrl == map.m_Control) {
				int	nDelta = map.m_End - map.m_Start;
				int	nVal = Round(nData / 127.0 * nDelta + map.m_Start);
				switch (map.m_Target) {
				case CTripLightDoc::MT_ROWS:
					m_Rows = nVal;
					UpdateMesh();
					break;
				case CTripLightDoc::MT_COLUMNS:
					m_Cols = nVal;
					UpdateMesh();
					break;
				case CTripLightDoc::MT_PATTERN:
					m_Pattern = nVal;
					UpdateMesh();
					break;
				case CTripLightDoc::MT_SPEED:
					SetColorSpeed(nVal);
					break;
				case CTripLightDoc::MT_VARIANCE:
					SetColorSpeedVariance(nVal);
					break;
				case CTripLightDoc::MT_JUMP:
					{
						bool	bJump = nData != 0;
						if (bJump != bPrevJump) {
							bPrevJump = bJump;
							OnImageColorJump();
						}
					}
					break;
				case CTripLightDoc::MT_FADE:
					m_bFadeOut = nVal != 0;
					break;
				}
			}
		}
	}
	return 0;
}

void CTripLightView::OnFileExport() 
{
	CPause	pause(*this);
	CFileDialog	fd(false, PNG_FILE_EXT, GetDefaultFileName(), OFN_OVERWRITEPROMPT, EXPORT_FILTER);
	if (fd.DoModal() == IDOK) {
		CWaitCursor	wc;
		CString	sFileName(fd.GetFileName());
		LPCTSTR	pszExt = PathFindExtension(sFileName);
		if (pszExt != NULL && !_tcsicmp(pszExt, PNG_FILE_EXT)) {	// if file extension is PNG
			if (!ExportPNG(fd.GetPathName(), GetExportSize()))
				AfxThrowResourceException();
		} else {	// assume bitmap
			if (!ExportBitmap(fd.GetPathName(), GetExportSize()))
				AfxThrowResourceException();
		}
	}
}

void CTripLightView::OnFileRecord()
{
	CString	sRecordFolder(theApp.GetProfileString(REG_SETTINGS, _T("RecordFolder")));
	UINT	nFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
	// browse for folder
	CString	sTitle(LDS(IDS_IMAGE_SEQ_FOLDER));
	CPause	pause(*this);	// enter pause
	if (CFolderDialog::BrowseFolder(sTitle, sRecordFolder, NULL, nFlags, sRecordFolder)) {
		theApp.WriteProfileString(REG_SETTINGS, _T("RecordFolder"), sRecordFolder);
		CPathStr	sBasePath(sRecordFolder);
		sBasePath.Append(_T("img"));
		CString	sFrameNum;
		sFrameNum.Format(_T("%05d"), 0);
		CString	sFileExt(".png");
		if (PathFileExists(sBasePath + sFrameNum + sFileExt)) {	// if first frame already exists
			if (AfxMessageBox(IDS_IMAGE_SEQ_OVERWRITE_WARN, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES)
				return;
		}
#if !EXPORT_MIDI_ONLY
		WildcardDeleteFile(sBasePath + '*' + sFileExt);	// delete previously existing frame files if any
#endif
		CProgressDlg	dlg;
		if (!dlg.Create())
			AfxThrowResourceException();
		CSaveObj<bool>	saveMidiExport(m_bExportMidi, true);
		m_arrMidiTrack.SetSize(CHORD_TONES);	// one track per chord tone
		OnInitialUpdate();
		ASSERT(1000 % TIMER_PERIOD == 0);	// otherwise MIDI scheme won't work
		int	nFrameRate = 1000 / TIMER_PERIOD;
		double	fFrameRate = nFrameRate;
		int	nFrames = Round(theApp.m_settings.m_RecordSeconds * fFrameRate);
		dlg.SetRange(0, nFrames);
		CSize	szExport(GetExportSize());
		for (int iFrame = 0; iFrame < nFrames; iFrame++) {	// for each frame
			dlg.SetPos(iFrame);
			if (dlg.Canceled())	// if user canceled
				return;
			OnViewSingleStep();	// update frame
			ValidateRect(NULL);	// don't update our window; saves time and avoids flicker
			sFrameNum.Format(_T("%05d"), iFrame);
#if !EXPORT_MIDI_ONLY
			if (!ExportPNG(sBasePath + sFrameNum + sFileExt, szExport))
				AfxThrowResourceException();
#endif
		}
		CPathStr	sMidiFilePath(sRecordFolder);
		sMidiFilePath.Append(_T("Triplight.mid"));
		CMidiFile	fMidi(sMidiFilePath, CFile::modeCreate | CFile::modeWrite);
		USHORT	nTimeBase = static_cast<USHORT>(nFrameRate);	// one tick per frame
		double	fTempo = 60;	// one quarter note per second
		fMidi.WriteHeader(CHORD_TONES, nTimeBase, fTempo);
#if DUMP_MIDI_TEXT
		CPathStr	sMidiDumpPath(sRecordFolder);
		sMidiDumpPath.Append(_T("TriplightMidi.txt"));
		CStdioFile	fDump(sMidiDumpPath, CFile::modeCreate | CFile::modeWrite);
#endif
		for (int iTone = 0; iTone < CHORD_TONES; iTone++) {	// for each chord tone
			CMidiFile::CMidiEventArray&	arrEvt = m_arrMidiTrack[iTone];
			int	nEvts = arrEvt.GetSize();
			int	nPrevEvtTime = 0;
			for (int iEvt = 0; iEvt < nEvts; iEvt++) {
				CMidiFile::MIDI_EVENT&	evt = arrEvt[iEvt];
				int	nEvtTime = evt.DeltaT;
				evt.DeltaT = nEvtTime - nPrevEvtTime;
				nPrevEvtTime = nEvtTime;
#if DUMP_MIDI_TEXT
				CString	s;
				s.Format(_T("%08d %02x %02x %02x\n"), nEvtTime, MIDI_STAT(evt.Msg), MIDI_P1(evt.Msg), MIDI_P2(evt.Msg));
				fDump.WriteString(s);
#endif
			}
			fMidi.WriteTrack(m_arrMidiTrack[iTone]);
		}
	}
	m_arrMidiTrack.RemoveAll();
}

void CTripLightView::OnFileSaveSnapshot() 
{
	CPause	pause(*this);
	CFileDialog	fd(FALSE, SNAPSHOT_FILE_EXT, GetDefaultFileName(), OFN_OVERWRITEPROMPT, SNAPSHOT_FILTER);
	if (fd.DoModal() == IDOK)
		SaveSnapshot(fd.GetPathName());
}

void CTripLightView::OnFileLoadSnapshot() 
{
	CFileDialog	fd(TRUE, SNAPSHOT_FILE_EXT, NULL, OFN_HIDEREADONLY, SNAPSHOT_FILTER);
	if (fd.DoModal() == IDOK)
		LoadSnapshot(fd.GetPathName());
}
