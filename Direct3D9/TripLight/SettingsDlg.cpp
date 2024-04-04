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

// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TripLight.h"
#include "SettingsDlg.h"
#include <afxpriv.h>	// for WM_KICKIDLE

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg dialog


CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent)
{
	m_ImageHeight = 0;
	m_ImageWidth = 0;
	m_ImageSizeType = -1;
}

void CSettingsDlg::GetInfo(CSettings& info) const
{
	#define SETTINGSDEF(type, name, initval) info.m_##name = m_##name;
	#include "SettingsDef.h"	// generate code to copy members
}

void CSettingsDlg::SetInfo(const CSettings& info)
{
	#define SETTINGSDEF(type, name, initval) m_##name = info.m_##name;
	#include "SettingsDef.h"	// generate code to copy members
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_SETTINGS_IMAGE_SIZE_TYPE, m_ImageSizeType);
	DDX_Text(pDX, IDC_SETTINGS_IMAGE_HEIGHT, m_ImageHeight);
	DDV_MinMaxInt(pDX, m_ImageHeight, 1, 32767);
	DDX_Text(pDX, IDC_SETTINGS_IMAGE_WIDTH, m_ImageWidth);
	DDV_MinMaxInt(pDX, m_ImageWidth, 1, 32767);
	DDX_Text(pDX, IDC_SETTINGS_RECORD_SECONDS, m_RecordSeconds);
	DDV_MinMaxInt(pDX, m_RecordSeconds, 0, INT_MAX);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_UPDATE_COMMAND_UI(IDC_SETTINGS_IMAGE_WIDTH, OnUpdateBmpSizeType)
	ON_UPDATE_COMMAND_UI(IDC_SETTINGS_IMAGE_HEIGHT, OnUpdateBmpSizeType)
	ON_UPDATE_COMMAND_UI(IDC_SETTINGS_IMAGE_WIDTH_CAP, OnUpdateBmpSizeType)
	ON_UPDATE_COMMAND_UI(IDC_SETTINGS_IMAGE_HEIGHT_CAP, OnUpdateBmpSizeType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg message handlers

BOOL CSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
}

LRESULT CSettingsDlg::OnKickIdle(WPARAM, LPARAM)
{
	UpdateDialogControls(this, FALSE);
	return 0;
}

void CSettingsDlg::OnUpdateBmpSizeType(CCmdUI *pCmdUI)
{
	int nChecked = GetCheckedRadioButton(IDC_SETTINGS_IMAGE_SIZE_TYPE, IDC_SETTINGS_IMAGE_SIZE_TYPE2);
	pCmdUI->Enable(nChecked == IDC_SETTINGS_IMAGE_SIZE_TYPE2);
}
