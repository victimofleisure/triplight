// Copyleft 2023 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      22mar23	initial version

*/

#pragma once

// CPianoWnd dialog

#include "ArrayEx.h"

class CPianoWnd : public CWnd
{
	DECLARE_DYNAMIC(CPianoWnd)

// Construction
public:
	CPianoWnd();
	virtual ~CPianoWnd();

// Attributes
	int		GetKeyCount() const;
	void	SetKeyCount(int nKeys);
	int		GetStartNote() const;
	void	SetStartNote(int nStartNote);
	void	SetKeyColor(int iKey, D2D1::ColorF color);

// Operations
	void	UpdateKeyArray();
	int		HitTest(CPoint pt) const;

protected:
// Types
	struct KEY_INFO {	// information about each key within the octave
		int		nWhiteIndex;	// index of nearest white key; range is [0..6]
		int		nBlackOffset;	// if black key, its offset from C in Savard units
	};
	class CKey {
	public:
		CKey();
		CD2DRectF	m_rKey;		// key rectangle in DIPs
		bool	m_bIsBlack;		// true if key is black
		D2D1::ColorF	m_color;	// per-key color
	};
	typedef CArrayEx<CKey, CKey&> CKeyArray;

// Constants
	enum {	// dimensions from "The Size of the Piano Keyboard" by John J. G. Savard
		WHITE_WIDTH = 24,		// width of white key, in Savard units
		BLACK_WIDTH = 14,		// width of black key, in Savard units
		C_SHARP_OFFSET = 15,	// offset of C# key from C key, in Savard units
		F_SHARP_OFFSET = 13,	// offset of F# key from F key, in Savard units
	};
	enum {
		NOTES = 12,				// semitones per octave
		WHITES = 7,				// number of white keys per octave
	};
	enum {	// key types
		KT_WHITE,
		KT_BLACK,
		KEY_TYPES
	};
	static const double m_fBlackHeightRatio;	// ratio of black height to white height
	static const KEY_INFO	m_arrKeyInfo[NOTES];	// static key information
	static const D2D1::ColorF	m_arrKeyTypeColor[KEY_TYPES];	// key type colors

// Data members
	bool	m_bUsingD2D;
	int		m_nKeys;			// total number of keys on keyboard
	int		m_nStartNote;		// MIDI note number of keyboard's first note
	int		m_nPrevStartNote;	// previous start note, for detecting changes
	CKeyArray	m_arrKey;		// dynamic information about each key
	CD2DSizeF	m_szRender;		// cached render size
	CD2DSizeF	m_szDPI;		// resolution in DPI

// Overrides

// Helpers
	void	UpdateKeyRects();

// Message map functions
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	virtual void PostNcDestroy();
	LRESULT OnDrawD2D(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

inline int CPianoWnd::GetKeyCount() const
{
	return(m_nKeys);
}

inline int CPianoWnd::GetStartNote() const
{
	return(m_nStartNote);
}

inline void CPianoWnd::SetKeyCount(int nKeys)
{
	m_nKeys = nKeys;
}

inline void CPianoWnd::SetStartNote(int nStartNote)
{
	m_nStartNote = nStartNote;
}

inline void CPianoWnd::SetKeyColor(int iKey, D2D1::ColorF color)
{
	m_arrKey[iKey].m_color = color;
}
