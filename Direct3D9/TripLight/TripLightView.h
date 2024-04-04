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
		02		15apr23	add modulation period constant
		03		02aug23	restore snapshot support
		04		03apr24	add Direct3D support

		TripLight view
 
*/

// TripLightView.h : interface of the CTripLightView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ArrayEx.h"
#include "FixedArray.h"
#include "MidiWrap.h"
#include "MidiFile.h"
#include "Midi.h"
#include "d3d9.h"

class CDib;

class CTripLightView : public CView, protected CTripLightParams
{
protected: // create from serialization only
	CTripLightView();
	DECLARE_DYNCREATE(CTripLightView)

// Constants
	enum {	// pattern bitmasks
		DIAG_RL		= 0x1,		// diagonal top right to bottom left
		ALT_ROWS	= 0x2,		// reverse diagonal for each row
		ALT_COLS	= 0x4,		// reverse diagonal for each column
		PATTERNS	= 8,		// total number of patterns
	};

// Attributes
public:
	CTripLightDoc* GetDocument();
	void	GetParams(CTripLightParams& params) const;
	CSize	GetGridSize() const;
	void	SetGridSize(CSize szGrid);
	int		GetColorSpeed() const;
	void	SetColorSpeed(int nSpeed);
	int		GetColorSpeedVariance() const;
	void	SetColorSpeedVariance(int nVariance);
	bool	GetPause() const;
	bool	SetPause(bool bEnable);
	bool	GetBackward() const;
	void	SetBackward(bool bEnable);
	bool	GetBitmap(CSize szImg, CDib& dibOut);
	int		GetToneCount() const;
	BYTE	GetToneNote(int iTone) const;
	BYTE	GetToneVolume(int iTone) const;
	bool	IsToneUnmuted(int iTone) const;

// Operations
public:
	void	CreateMesh();
	void	UpdateMesh();
	void	ResizeMesh(CSize szWnd);
	void	ResizeMesh();
	bool	ExportBitmap(LPCTSTR pszPath, CSize szImg);
	bool	ExportPNG(LPCTSTR pszPath, CSize szImg);
	void	SaveSnapshot(LPCTSTR Path);
	void	LoadSnapshot(LPCTSTR Path);
	static	CString	GetNoteName(int nNote);
	static	LPCTSTR	GetKeyName(int nNote);

// Overrides
	// ClassWizard generated virtual function overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CTripLightView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
// Types
	struct VERTEX_INFO {
		int		nDeltaRed;
		int		nDeltaGreen;
		int		nDeltaBlue;
	};
	typedef CArrayEx<TRIVERTEX, TRIVERTEX&> CTriVertexArray;
	typedef CArrayEx<GRADIENT_TRIANGLE, GRADIENT_TRIANGLE&> CGradientTriangleArray;
	typedef CArrayEx<VERTEX_INFO, VERTEX_INFO&> CVertexInfoArray;
	class CPause {
	public:
		CPause(CTripLightView& View, bool bEnable = TRUE);
		~CPause();
		CTripLightView&	m_View;
		bool	m_bPrevPause;
	};
	struct VERTEX_3D {
		FLOAT	x, y, z, w;	// position
	    DWORD	color;		// color
	};
	typedef CArrayEx<VERTEX_3D, VERTEX_3D&> CVertex3DArray;

// Constants
	enum {
		TIMER_PERIOD = 10,		// timer period, in milliseconds
		TIMER_RESOLUTION = 1,	// timer resolution, in milliseconds
		ROWS_MAX = SHRT_MAX,	// maximum number of rows
		COLS_MAX = SHRT_MAX,	// maximum number of columns
		SPEED_MIN = 0,			// minimum color speed
		SPEED_MAX = 0x4000,		// maximum color speed
		COLOR_MAX = USHRT_MAX,	// maximum color value
	};

// Data members
	bool	m_bPause;			// true if paused
	bool	m_bPrePrintPause;	// true if paused before printing started
	bool	m_bShowMesh;		// true if showing mesh
	bool	m_bBackward;		// true if going backward
	int		m_nColorSpeedVarDelta;	// color speed variation, in color delta per tick
	CMMTimerPeriod	m_TimerPeriod;	// multimedia timer period
	CMMTimer	m_FrameTimer;	// multimedia timer	
	CTriVertexArray	m_arrVert;	// array of vertices
	CGradientTriangleArray	m_arrTri;	// array of triangles
	CVertexInfoArray	m_arrVertInfo;	// array of vertex info
	bool	m_bShowingErrorMsg;	// true if showing error message

// Direct3D members
	CComPtr<IDirect3D9>	m_pD3D;	// Direct3D interface
	CComPtr<IDirect3DDevice9>	m_pDevice;	// Direct3D device interface
	CComPtr<IDirect3DVertexBuffer9>	m_pVertBuf;	// Direct3D vertex buffer
	CComPtr<IDirect3DIndexBuffer9>	m_pIdxBuf;	// Direct3D index buffer

// Music constants
	enum {
		OCTAVE = 12,			// size of octave, in semitones
		SCALE_TONES = 7,		// number of tones in scale
		CHORD_TONES = 12,		// number of tones in chord
		CHORD_MODE = 3,			// chord's mode in C (Lydian)
		DRUM_CHANNEL = 9,		// MIDI channel reserved for drums
		PAN_STEP_SIZE = 4,		// in MIDI pan units
		NOTE_VELOCITY = 100,	// in MIDI velocity units
		CHORD_START = 36,		// in MIDI notes
		PATCH_NUMBER = 48,		// strings patch
		MODULATION_PERIOD = 3000,	// modulation period in timer ticks
		MAX_MOD_PRUNES = 3,		// maximum number of voices to prune during modulation
	};
	static const LPCTSTR m_arrNoteName[OCTAVE];

// Music types
	typedef CFixedArray<BYTE, SCALE_TONES> CScale;
	typedef CFixedArray<BYTE, CHORD_TONES> CChord;

// Music data members
	BYTE	m_arrActiveNote[MIDI_CHANNELS];	// active note on MIDI each channel
	CChord	m_arrVolume;		// array of MIDI volumes
	CChord	m_arrTone;			// array of unaltered chord tones, as MIDI notes
	WORD	m_nAlteredMask;		// bit for each tone, set if tone is altered
	WORD	m_nUnmuteMask;		// bit for each tone, set if tone is unmuted
	WORD	m_nHalfStepMask;	// bit for each tone, set if tone's alteration is a semitone
	bool	m_bFadeOut;			// true if fading out, otherwise fading in
	bool	m_bModulating;		// true if modulating to next key
	int		m_iFrame;			// index of current frame
	int		m_iLastModFrame;	// frame index of most recent modulation
	int		m_nKey;				// current key, in normalized semitones
	int		m_nModKey;			// during modulation, target key
	int		m_nModPrunes;		// during modulation, number of voice prunes remaining
	CChord	m_arrModTone;		// during modulation, target chord tones 
	WORD	m_nModHalfStepMask;	// during modulation, target half step mask

// Data for CheckHarmony, debug only
#ifdef _DEBUG
	BYTE	m_arrTestNote[MIDI_CHANNELS];
	BYTE	m_arrTestVolume[MIDI_CHANNELS];
	CScale	m_testScale;
	int		m_nTestKey;
#endif

// Midi export members
	bool	m_bExportMidi;		// true if exporting MIDI file
	CMidiFile::CMidiTrackArray	m_arrMidiTrack;	// MIDI file tracks for export

// Helpers
	bool	CreateTimer(bool bEnable);
	static	void	UpdateColor(COLOR16& nColor, int& nDelta);
	void	UpdateColorDelta(int& nDelta) const;
	void	UpdateColorDeltas();
	void	ReverseColorDeltas();
	void	UpdateColorSpeedVariance();
	void	DumpColorDeltas();
	void	ResetMidiData();
	void	MidiInit();
	void	MidiOut(int nEvent, int nChannel, int nP1, int nP2 = 0);
	void	MidiNoteOut(int nChannel, int nNote);
	void	MidiFileWrite(DWORD nShortMsg);
	CString	GetDefaultFileName();
	CSize	GetExportSize() const;
	bool	TryModulation(int iSilentTone);
	BYTE	GetTargetToneNote(int iTone) const;
	void	CheckHarmony(int nEvent, int nChannel, int nP1, int nP2);
	static	bool	CopyDIBToClipboard(const CDib& dibIn, HWND hWnd);
	static	void	CALLBACK TimerCallback(UINT uID, UINT uMsg, W64ULONG dwUser, W64ULONG dw1, W64ULONG dw2);
	static	int		Rand(int nVals);
	static	double	frand();
	static	COLOR16	RandColor();
	static	int		WildcardDeleteFile(CString sPath);
	static	int		ToneToChannel(int iTone);
	static	int		ChannelToTone(int iChan);
	static	int		ByteSortCompareAscending(const void *elem1, const void *elem2);
	static	int		AbsLeastInterval(int nNote1, int nNote2);
	static	void	GetScale(int nKey, CScale& scale);
	static	USHORT	GetChord(int nKey, int nMode, CChord& arrTone);
	static	BYTE	GetToneNote(const CChord& arrTone, USHORT nHalfStepMask, USHORT nAlteredMask, int iTone);
	static	void	DumpTones(const CChord& arrTone);
	static	UINT	MakeToneBit(int iTone);
	void	GetPresentationParameters(int cx, int cy, D3DPRESENT_PARAMETERS& pp);
	bool	CreateDevice(int cx, int cy);
	void	DestroyDevice();
	bool	InitDevice();
	bool	CreateBuffers();
	bool	ResizeDevice(int cx, int cy);
	bool	ResizeDevice();
	void	OnDeviceError(HRESULT nError);	
	void	Draw(CDC* pDC);
	bool	Draw3D();

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnImageColsLess();
	afx_msg void OnImageColsMore();
	afx_msg void OnImageRowsLess();
	afx_msg void OnImageRowsMore();
	afx_msg void OnImageColorSpeedFaster();
	afx_msg void OnImageColorSpeedSlower();
	afx_msg void OnViewPause();
	afx_msg void OnUpdateViewPause(CCmdUI* pCmdUI);
	afx_msg void OnImageColorSpeedVarianceMore();
	afx_msg void OnImageColorSpeedVarianceLess();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnImageColorJump();
	afx_msg void OnViewShowMesh();
	afx_msg void OnUpdateViewShowMesh(CCmdUI* pCmdUI);
	afx_msg void OnViewBackward();
	afx_msg void OnUpdateViewBackward(CCmdUI* pCmdUI);
	afx_msg void OnViewSingleStep();
	afx_msg void OnUpdateViewSingleStep(CCmdUI* pCmdUI);
	afx_msg void OnImagePattern(UINT nID);
	afx_msg void OnUpdateImagePattern(CCmdUI* pCmdUI);
	afx_msg LRESULT OnFrameTimer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMappingChange(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFileExport();
	afx_msg void OnToolsFade();
	afx_msg void OnFileRecord();
	afx_msg void OnFileSaveSnapshot();
	afx_msg void OnFileLoadSnapshot();
	afx_msg void OnDestroy();
};

#ifndef _DEBUG  // debug version in TripLightView.cpp
inline CTripLightDoc* CTripLightView::GetDocument()
   { return (CTripLightDoc*)m_pDocument; }
#endif

inline void CTripLightView::GetParams(CTripLightParams& params) const
{
	params = *this;
}

inline CSize CTripLightView::GetGridSize() const
{
	return CSize(m_Cols, m_Rows);
}

inline int CTripLightView::GetColorSpeed() const
{
	return m_ColorSpeed;
}

inline int CTripLightView::GetColorSpeedVariance() const
{
	return m_ColorSpeedVar;
}

inline bool CTripLightView::GetPause() const
{
	return m_bPause;
}

inline bool CTripLightView::GetBackward() const
{
	return m_bBackward;
}

inline int CTripLightView::GetToneCount() const
{
	return CHORD_TONES;
}

inline BYTE CTripLightView::GetToneVolume(int iTone) const
{
	return m_arrVolume[iTone];
}

inline BYTE CTripLightView::GetToneNote(int iTone) const
{
	return GetToneNote(m_arrTone, m_nHalfStepMask, m_nAlteredMask, iTone);
}

inline UINT CTripLightView::MakeToneBit(int iTone)
{
	return 1 << iTone;
}

inline bool CTripLightView::IsToneUnmuted(int iTone) const
{
	return (m_nUnmuteMask & MakeToneBit(iTone)) != 0;
}
