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

// PianoWnd.cpp : implementation file
//

#include "stdafx.h"
#include "TripLight.h"
#include "MainFrm.h"
#include "PianoWnd.h"

// CPianoWnd dialog

IMPLEMENT_DYNAMIC(CPianoWnd, CWnd)

const double CPianoWnd::m_fBlackHeightRatio = 2.0 / 3.0;

const CPianoWnd::KEY_INFO CPianoWnd::m_arrKeyInfo[NOTES] = {
	{0,	0},	// C
	{1,	C_SHARP_OFFSET},	// C#
	{1, 0},	// D
	{2,	C_SHARP_OFFSET + BLACK_WIDTH * 2},	// D#
	{2, 0},	// E
	{3, 0},	// F
	{4,	C_SHARP_OFFSET * 2 + F_SHARP_OFFSET + BLACK_WIDTH * 3},	// F#
	{4, 0},	// G
	{5,	C_SHARP_OFFSET * 2 + F_SHARP_OFFSET + BLACK_WIDTH * 5},	// G#
	{5, 0},	// A
	{6,	C_SHARP_OFFSET * 2 + F_SHARP_OFFSET + BLACK_WIDTH * 7},	// A#
	{6,	0},	// B
};

const D2D1::ColorF	CPianoWnd::m_arrKeyTypeColor[KEY_TYPES] = {
	D2D1::ColorF::White,
	D2D1::ColorF::Black, 
};

CPianoWnd::CPianoWnd()
{
	m_bUsingD2D = false;
	m_nStartNote = 0;
	m_nPrevStartNote = 0;
	m_nKeys = 0;
	m_szRender = CD2DSizeF(0, 0);
	m_szDPI = CD2DSizeF(0, 0);
}

CPianoWnd::~CPianoWnd()
{
}

void CPianoWnd::UpdateKeyArray()
{
	int	nKeys = m_arrKey.GetSize();	// get existing key count
	int	nStartDelta = m_nStartNote - m_nPrevStartNote;
	if (nStartDelta) {	// if start note changed
		if (nStartDelta > 0)	// if start note increased
			m_arrKey.RemoveAt(0, min(nStartDelta, nKeys));
		else {	// start note decreased
			CKey	key;
			m_arrKey.InsertAt(0, key, min(-nStartDelta, nKeys));
		}
		m_nPrevStartNote = m_nStartNote;
	}
	m_arrKey.SetSize(m_nKeys);	// resize key array, possibly reallocating it
}

void CPianoWnd::UpdateKeyRects()
{
	int	nKeys = m_nKeys;	// get key count
	int	nWhites = 0;
	for (int iKey = 0; iKey < nKeys; iKey++) {	// for each key
		int	iKeyInfo = (m_nStartNote + iKey) % NOTES;	// account for start note
		if (!m_arrKeyInfo[iKeyInfo].nBlackOffset)	// if key is black
			nWhites++;
	}
	double	fWhiteWidth = double(m_szRender.width) / nWhites;
	double	fHScale = fWhiteWidth / WHITE_WIDTH;
	double	fBlackWidth = BLACK_WIDTH * fHScale;
	double	fBlackHeight = m_fBlackHeightRatio * m_szRender.height;
	int	iStartWhite = m_arrKeyInfo[m_nStartNote % NOTES].nWhiteIndex;
	double	fStartOffset = iStartWhite * fWhiteWidth;
	int	iWhite = iStartWhite;
	double	fPrevWhiteX = 0;
	iWhite = iStartWhite;
	for (int iKey = 0; iKey < nKeys; iKey++) {	// for each key
		CKey&	key = m_arrKey[iKey];
		int	iKeyInfo = (m_nStartNote + iKey) % NOTES;	// account for start note
		int	nBlackOffset = m_arrKeyInfo[iKeyInfo].nBlackOffset;
		key.m_bIsBlack = nBlackOffset != 0;
		if (nBlackOffset) {	// if black key
			double	fOctaveOffset = (iWhite / WHITES) * fWhiteWidth * WHITES;
			double	x = nBlackOffset * fHScale - fStartOffset + fOctaveOffset;
			key.m_rKey = CD2DRectF(static_cast<float>(x), 0, static_cast<float>(x + fBlackWidth), static_cast<float>(fBlackHeight));
		} else {	// white key
			double	x = (iWhite + 1) * fWhiteWidth - fStartOffset;
			key.m_rKey = CD2DRectF(static_cast<float>(fPrevWhiteX), 0, static_cast<float>(x), m_szRender.height);
			fPrevWhiteX = x;
			iWhite++;
		}
	}
}

int	CPianoWnd::HitTest(CPoint pt) const
{
	// convert from GDI physical pixels to Direct2D device-independent pixels
	double	fDPIScaleX = m_szDPI.width ? 96.0 / m_szDPI.width : 0;
	double	fDPIScaleY = m_szDPI.height ? 96.0 / m_szDPI.height : 0;
	double	fCursorX = pt.x * fDPIScaleX;
	double	fCursorY = pt.y * fDPIScaleY;
	int	nKeys = m_nKeys;
	for (int iKeyType = 0; iKeyType < KEY_TYPES; iKeyType++) {	// for each key type
		bool	bIsBlack = (iKeyType == 0);	// test black keys first
		for (int iKey = 0; iKey < nKeys; iKey++) {	// for each key
			const CKey&	key = m_arrKey[iKey];
			if (key.m_bIsBlack == bIsBlack) {	// if key type matches
				if (fCursorX >= key.m_rKey.left && fCursorY >= key.m_rKey.top
				&& fCursorX < key.m_rKey.right && fCursorY < key.m_rKey.bottom) {
					return iKey;
				}
			}
		}
	}
	return -1;
}

inline CPianoWnd::CKey::CKey() :
	m_color(0, 0, 0, 0)
{
	m_bIsBlack = false;
}

BEGIN_MESSAGE_MAP(CPianoWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_REGISTERED_MESSAGE(AFX_WM_DRAW2D, OnDrawD2D)
	ON_WM_CREATE()
END_MESSAGE_MAP()

// CPianoWnd message handlers

int CPianoWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	EnableD2DSupport();	// enable Direct2D for this window; takes around 60ms
	m_bUsingD2D = IsD2DSupportEnabled() != 0;	// cache to avoid performance hit

	return 0;
}

void CPianoWnd::OnDestroy()
{
	theApp.GetMain()->SendMessage(UWM_MODELESS_DESTROY, reinterpret_cast<WPARAM>(this));
	CWnd::OnDestroy();
}

void CPianoWnd::PostNcDestroy()
{
	CWnd::PostNcDestroy();
	delete this;	// destroy this instance
}

BOOL CPianoWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CPianoWnd::OnPaint()
{
	if (m_bUsingD2D) {	// if using Direct2D
		Default();	// don't paint window, it belongs to OnDrawD2D
		return;
	}
	CPaintDC dc(this); // device context for painting
	CRect	rClip;
	dc.GetClipBox(rClip);
	dc.FillSolidRect(rClip, 0);
}

LRESULT CPianoWnd::OnDrawD2D(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	CRenderTarget* pRenderTarget = reinterpret_cast<CRenderTarget*>(lParam);
	ASSERT_VALID(pRenderTarget);
	if (!pRenderTarget->IsValid())
		return 0;
	D2D1_SIZE_F szRender = pRenderTarget->GetSize();	// get target size in DIPs
	m_szDPI = pRenderTarget->GetDpi();	// store DPI for mouse coordinate scaling
	if (szRender.width != m_szRender.width || szRender.height != m_szRender.height) {	// if render size changed
		m_szRender = szRender;
		UpdateKeyRects();
	}
	const D2D1::ColorF	clrBkgnd(D2D1::ColorF::White);
	pRenderTarget->Clear(clrBkgnd);	// erase background
	CD2DSolidColorBrush	brOutline(pRenderTarget, D2D1::ColorF(D2D1::ColorF::Black));
	int	nKeys = m_nKeys;
	for (int iKeyType = 0; iKeyType < KEY_TYPES; iKeyType++) {	// for each key type
		D2D1::ColorF	clrDefault(m_arrKeyTypeColor[iKeyType]);
		bool	bIsBlack = (iKeyType != 0);	// draw white keys first
		for (int iKey = 0; iKey < nKeys; iKey++) {	// for each key
			const CKey&	key = m_arrKey[iKey];
			if (key.m_bIsBlack == bIsBlack) {	// if key type matches
				D2D1::ColorF	clrKey = key.m_color.a ? key.m_color : clrDefault;
				CD2DSolidColorBrush	brFill(pRenderTarget, clrKey);
				pRenderTarget->FillRectangle(key.m_rKey, &brFill);
				pRenderTarget->DrawRectangle(key.m_rKey, &brOutline);
			}
		}
	}
	return 0;
}
