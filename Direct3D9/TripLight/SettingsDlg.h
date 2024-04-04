// Copyleft 2016 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
		00		27feb16	initial version
		01		15mar23	refactor
 
*/

#pragma once

// SettingsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg dialog

#include "Settings.h"

class CSettingsDlg : public CDialog, public CSettings
{
// Construction
public:
	CSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	void	GetInfo(CSettings& info) const;
	void	SetInfo(const CSettings& info);

// Overrides
	// ClassWizard generated virtual function overrides
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
// Dialog Data
	enum { IDD = IDD_SETTINGS };

// Overrides
	virtual BOOL OnInitDialog();

// Generated message map functions
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateBmpSizeType(CCmdUI *pCmdUI);
	DECLARE_MESSAGE_MAP()
};

