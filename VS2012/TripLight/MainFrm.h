// Copyleft 2015 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      25dec15	initial version
 		01		15mar23	add settings
		02		01apr24	update full screen method

		TripLight main window
 
*/

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ArrayEx.h"

class CTripLightView;
class CPianoWnd;

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	CTripLightView	*GetView();
	bool	IsFullScreen() const;
	CPianoWnd	*GetPianoWnd();
	bool	IsPianoShown() const;

// Operations
public:
	void	FullScreen(bool Enable);
	void	UpdateCmdUI();
	bool	ShowPiano(bool bShow);

// Overrides
	// ClassWizard generated virtual function overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_StatusBar;
	CToolBar    m_ToolBar;

// Generated message map functions
protected:
// Constants
	enum {	// status bar panes
		SBP_HINT,
		SBP_ROWS,
		SBP_COLS,
		SBP_COLOR_SPEED,
		SBP_COLOR_SPEED_VAR,
		SBP_PAUSE,
		SBP_BACKWARD,
		SBP_FRAME_NUMBER,
	};
	enum {
		PIANO_TIMER_ID = 1861,
		PIANO_TIMER_PERIOD = 20,
	};

// Data members
	CTripLightView	*m_pView;	// one and only view
	bool	m_WasShown;			// true if window was shown
	bool	m_FullScreen;		// true if full screen
	bool	m_ShowToolBar;		// true if tool bar was showing prior to going full screen
	bool	m_ShowStatusBar;	// true if status bar was showing prior to going full screen
	CPoint	m_PrevCursorPos;	// cursor position prior to going full screen
	CPianoWnd	*m_pPianoWnd;	// pointer to piano window instance if any
	UINT_PTR	m_nPianoTimer;	// piano timer instance
	CByteArrayEx	m_arrPianoNote;	// array of piano's active MIDI notes
	WINDOWPLACEMENT	m_wpFullScreenPrev;	// window placement before going full screen

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowFullscreen();
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnUpdateIndicatorCols(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorRows(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorColorSpeed(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorColorSpeedVariance(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorPause(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorBackward(CCmdUI *pCmdUI);
	afx_msg void OnWindowEscape();
	afx_msg void OnClose();
	afx_msg void OnToolsSettings();
	afx_msg void OnViewPiano();
	afx_msg void OnUpdateViewPiano(CCmdUI *pCmdUI);
	afx_msg LRESULT OnModelessDestroy(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

inline CTripLightView *CMainFrame::GetView()
{
	return(m_pView);
}

inline bool CMainFrame::IsFullScreen() const
{
	return m_FullScreen;
}

inline CPianoWnd *CMainFrame::GetPianoWnd()
{
	return m_pPianoWnd;
}

inline bool CMainFrame::IsPianoShown() const
{
	return m_pPianoWnd != NULL;
}
