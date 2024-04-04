// Copyleft 2023 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      14mar23	initial version

*/

// PropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TripLight.h"
#include "PropertiesDlg.h"
#include "TripLightView.h"

// CPropertiesDlg dialog

IMPLEMENT_DYNAMIC(CPropertiesDlg, CDialog)

CPropertiesDlg::CPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPropertiesDlg::IDD, pParent)
{
}

CPropertiesDlg::~CPropertiesDlg()
{
}

void CPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PROPS_ROWS, m_Rows);
	DDV_MinMaxInt(pDX, m_Rows, 1, SHRT_MAX);
	DDX_Text(pDX, IDC_PROPS_COLS, m_Cols);
	DDV_MinMaxInt(pDX, m_Cols, 1, SHRT_MAX);
	DDX_Text(pDX, IDC_PROPS_PATTERN, m_Pattern);
	DDV_MinMaxInt(pDX, m_Pattern, 0, CTripLightView::PATTERNS - 1);
	DDX_Text(pDX, IDC_PROPS_COLOR_SPEED, m_ColorSpeed);
	DDX_Text(pDX, IDC_PROPS_COLOR_SPEED_STEP, m_ColorSpeedStep);
	DDX_Text(pDX, IDC_PROPS_COLOR_VARIANCE, m_ColorSpeedVar);
	DDX_Text(pDX, IDC_PROPS_COLOR_VARIANCE_STEP, m_ColorSpeedVarStep);
}

BEGIN_MESSAGE_MAP(CPropertiesDlg, CDialog)
END_MESSAGE_MAP()

// CPropertiesDlg message handlers
