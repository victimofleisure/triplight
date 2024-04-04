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
		02		01apr24	add previous window placement var

		TripLight main window
 
*/

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "TripLight.h"
#include "MainFrm.h"

#include "TripLightDoc.h"
#include "TripLightView.h"
#include "Persist.h"
#include "SettingsDlg.h"
#include "PianoWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_COLS,
	ID_INDICATOR_ROWS,
	ID_INDICATOR_COLOR_SPEED,
	ID_INDICATOR_COLOR_SPEED_VAR,
	ID_INDICATOR_PAUSE,
	ID_INDICATOR_BACKWARD,
};

#define RK_MAIN_FRAME		_T("MainFrame")

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_WasShown = FALSE;
	m_FullScreen = FALSE;
	m_ShowToolBar = FALSE;
	m_ShowStatusBar = FALSE;
	m_PrevCursorPos = CPoint(0, 0);
	m_pPianoWnd = NULL;
	m_nPianoTimer = 0;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	theApp.m_pMainWnd = this;	// so view's OnCreate can call SetActiveView
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_pView = STATIC_DOWNCAST(CTripLightView, m_pViewActive);
	ASSERT(m_pView != NULL);
	if (!m_ToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_ToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	if (!m_StatusBar.Create(this) ||
		!m_StatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_ToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_ToolBar);
	DragAcceptFiles();
	m_arrPianoNote.SetSize(m_pView->GetToneCount());

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

void CMainFrame::FullScreen(bool Enable)
{
	if (Enable == m_FullScreen)
		return;
	if (Enable) {	// if entering full screen
		m_ShowToolBar = m_ToolBar.IsWindowVisible() != 0;	// save control bar visibility
		m_ShowStatusBar = m_StatusBar.IsWindowVisible() != 0;
		SetMenu(NULL);	// remove menus
		ShowControlBar(&m_StatusBar, FALSE, TRUE);	// hide control bars
		ShowControlBar(&m_ToolBar, FALSE, TRUE);
		m_pView->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);	// remove view's client edge
		ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_TABSTOP, 0);	// remove frame
		UpdateWindow();	// reduce glitch by applying above changes before resizing
		MONITORINFO	mi = { sizeof(mi) };	// full screen method: Raymond Chen 20100412-00
		GetWindowPlacement(&m_wpFullScreenPrev);
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
		SetWindowPos(&CWnd::wndTop, mi.rcMonitor.left, mi.rcMonitor.top, 
			mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		GetCursorPos(&m_PrevCursorPos);	// save cursor position
		SetCursorPos(mi.rcMonitor.right, mi.rcMonitor.bottom);	// move cursor off-screen
		while (ShowCursor(FALSE) >= 0);	// try to hide cursor completely; may take multiple tries
	} else {	// exiting full screen
		SetWindowPlacement(&m_wpFullScreenPrev);
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE 
			| SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		ModifyStyle(0, WS_CAPTION | WS_THICKFRAME | WS_TABSTOP);	// restore frame
		m_pView->ModifyStyleEx(0, WS_EX_CLIENTEDGE);	// restore view's client edge
		::SetMenu(m_hWnd, m_hMenuDefault);	// restore menus
		ShowControlBar(&m_StatusBar, m_ShowStatusBar, TRUE);	// restore control bars
		ShowControlBar(&m_ToolBar, m_ShowToolBar, TRUE);
		CPoint	CursorPos;
		GetCursorPos(&CursorPos);
		if (CursorPos.x == GetSystemMetrics(SM_CXSCREEN) - 1	// if cursor still off-screen
		&& CursorPos.y == GetSystemMetrics(SM_CYSCREEN) - 1)
			SetCursorPos(m_PrevCursorPos.x, m_PrevCursorPos.y);	// restore cursor position
		ShowCursor(true);	// make cursor visible again
	}
	m_FullScreen = Enable;
}

void CMainFrame::UpdateCmdUI()
{
	m_ToolBar.OnUpdateCmdUI(this, FALSE);
	m_StatusBar.OnUpdateCmdUI(this, FALSE);
}

bool CMainFrame::ShowPiano(bool bShow)
{
	bool	bIsShown = m_pPianoWnd != NULL;
	if (bShow == bIsShown)	// if already in requested state
		return true;	// nothing to do
	if (bShow) {	// if showing piano
		CPianoWnd	*pPianoWnd = new CPianoWnd;
		DWORD	nStyle = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
		LPCTSTR	pszClass = AfxRegisterWndClass(0, ::LoadCursor(NULL, IDC_ARROW));
		CRect	rPiano(CPoint(200, 100), CSize(800, 150));
		BOOL	bRetVal = pPianoWnd->CreateEx(0, pszClass, _T("Piano"), nStyle, rPiano, this, 0, 0);
		ASSERT(bRetVal);
		if (!bRetVal) {	// if error creating window
			delete pPianoWnd;	// delete instance
			return false;
		}
		m_pPianoWnd = pPianoWnd;
		const int nPianoSize = 12 * 4;
		const int nStartNote = 12 * 3;
		pPianoWnd->SetKeyCount(nPianoSize);
		pPianoWnd->SetStartNote(nStartNote);
		pPianoWnd->UpdateKeyArray();
		m_nPianoTimer = SetTimer(PIANO_TIMER_ID, PIANO_TIMER_PERIOD, NULL);	// start timer
		SetFocus();
	} else {	// hiding piano
		KillTimer(m_nPianoTimer);	// stop timer
		m_nPianoTimer = 0;
		m_pPianoWnd->DestroyWindow();	// modeless destroy message resets our pointer
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message map

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_COMMAND(ID_WINDOW_FULL_SCREEN, OnWindowFullscreen)
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_COLS, OnUpdateIndicatorCols)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_ROWS, OnUpdateIndicatorRows)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_COLOR_SPEED, OnUpdateIndicatorColorSpeed)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_COLOR_SPEED_VAR, OnUpdateIndicatorColorSpeedVariance)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_PAUSE, OnUpdateIndicatorPause)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_BACKWARD, OnUpdateIndicatorBackward)
	ON_COMMAND(ID_WINDOW_ESCAPE, OnWindowEscape)
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_COMMAND(ID_TOOLS_SETTINGS, OnToolsSettings)
	ON_COMMAND(ID_VIEW_PIANO, OnViewPiano)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PIANO, OnUpdateViewPiano)
	ON_MESSAGE(UWM_MODELESS_DESTROY, OnModelessDestroy)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CFrameWnd::OnShowWindow(bShow, nStatus);
	if (!m_WasShown && !IsWindowVisible()) {
		m_WasShown = TRUE;
		CPersist::LoadWnd(REG_SETTINGS, this, RK_MAIN_FRAME, CPersist::NO_MINIMIZE);
	}
}

void CMainFrame::OnClose() 
{
	FullScreen(FALSE);
	CFrameWnd::OnClose();
}

void CMainFrame::OnDestroy() 
{
	ShowPiano(false);
	CPersist::SaveWnd(REG_SETTINGS, this, RK_MAIN_FRAME);
	CFrameWnd::OnDestroy();
}

void CMainFrame::OnUpdateIndicatorCols(CCmdUI *pCmdUI)
{
	CString	s;
	s.Format(_T("%d"), m_pView->GetGridSize().cx);
	pCmdUI->SetText(s);
}

void CMainFrame::OnUpdateIndicatorRows(CCmdUI *pCmdUI)
{
	CString	s;
	s.Format(_T("%d"), m_pView->GetGridSize().cy);
	pCmdUI->SetText(s);
}

void CMainFrame::OnUpdateIndicatorColorSpeed(CCmdUI *pCmdUI)
{
	CString	s;
	s.Format(_T("%d"), m_pView->GetColorSpeed());
	pCmdUI->SetText(s);
}

void CMainFrame::OnUpdateIndicatorColorSpeedVariance(CCmdUI *pCmdUI)
{
	CString	s;
	s.Format(_T("%d"), m_pView->GetColorSpeedVariance());
	pCmdUI->SetText(s);
}

void CMainFrame::OnUpdateIndicatorPause(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pView->GetPause());
}

void CMainFrame::OnUpdateIndicatorBackward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pView->GetBackward());
}

void CMainFrame::OnWindowFullscreen() 
{
	FullScreen(!m_FullScreen);
}

void CMainFrame::OnWindowEscape() 
{
	FullScreen(FALSE);
}

void CMainFrame::OnToolsSettings() 
{
	CSettingsDlg	dlg;
	dlg.SetInfo(theApp.m_settings);
	if (dlg.DoModal() == IDOK) {
		dlg.GetInfo(theApp.m_settings);
	}
}

void CMainFrame::OnViewPiano()
{
	ShowPiano(!IsPianoShown());
}

void CMainFrame::OnUpdateViewPiano(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(IsPianoShown());
}

LRESULT CMainFrame::OnModelessDestroy(WPARAM wParam, LPARAM lParam)
{
	CWnd	*pWnd = reinterpret_cast<CWnd *>(wParam);
	if (pWnd == m_pPianoWnd) {
		m_pPianoWnd = NULL;	// instance destroyed itself
		KillTimer(m_nPianoTimer);
		m_nPianoTimer = 0;
	}
	return 0;
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == PIANO_TIMER_ID && m_pPianoWnd != NULL) {
		int	nKeys = m_pPianoWnd->GetKeyCount();
		int	nStartNote = m_pPianoWnd->GetStartNote();
		int	nTones = m_pView->GetToneCount();
		for (int iTone = 0; iTone < nTones; iTone++) {	// for each chord tone
			BYTE	nNote = m_pView->GetToneNote(iTone);	// get tone's note
			BYTE	nVolume = m_pView->GetToneVolume(iTone);	// get tone's volume
			int	iKey = nNote - nStartNote;	// index of key that's turned on
			if (iKey >= 0 && iKey < nKeys) {	// if key within piano's range
				if (m_pView->IsToneUnmuted(iTone)) {	// if tone is unmuted
					double	fVolumeNorm = nVolume / 127.0;	// normalize volume
					D2D1::ColorF	clrKey(static_cast<float>(fVolumeNorm),	// red = max volume
						static_cast<float>(1 - fVolumeNorm), 0);	// green = min volume
					m_pPianoWnd->SetKeyColor(iKey, clrKey);	// color key according to volume
				} else {	// tone is muted
					m_pPianoWnd->SetKeyColor(iKey, D2D1::ColorF(0, 0, 0, 0));	// reset key color
				}
				if (nNote != m_arrPianoNote[iTone]) {	// if tone changed to different note
					iKey = m_arrPianoNote[iTone] - nStartNote;	// index of key to turn off
					if (iKey >= 0 && iKey < nKeys) {	// if key within piano's range
						m_pPianoWnd->SetKeyColor(iKey, D2D1::ColorF(0, 0, 0, 0));	// reset key color
					}
					m_arrPianoNote[iTone] = nNote;	// update shadow
				}
			}
		}
		m_pPianoWnd->Invalidate();
	}
	CFrameWnd::OnTimer(nIDEvent);
}
