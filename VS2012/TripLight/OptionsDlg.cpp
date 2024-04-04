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

// OptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TripLight.h"
#include "OptionsDlg.h"
#include "MidiWrap.h"
#include "MainFrm.h"
#include "TripLightDoc.h"
#include "TripLightView.h"

// COptionsDlg dialog

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)

COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_OPTIONS_MIDI_IN, m_MidiInDev);
	DDX_CBIndex(pDX, IDC_OPTIONS_MIDI_OUT, m_MidiOutDev);
	DDX_Check(pDX, IDC_OPTIONS_START_FULL_SCREEN, m_StartFullScreen);
	DDX_Text(pDX, IDC_OPTIONS_RANDOM_SEED, m_RandomSeed);
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	ON_BN_CLICKED(IDC_OPTIONS_SEED_FROM_TIME, &OnClickedSeedFromTime)
END_MESSAGE_MAP()


// COptionsDlg message handlers

BOOL COptionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CStringArray	arrDevInName;
	CMidiIn::GetDeviceNames(arrDevInName);
	CComboBox	*pCombo = static_cast<CComboBox *>(GetDlgItem(IDC_OPTIONS_MIDI_IN));
	pCombo->AddString(LDS(IDS_NONE));
	for (int iDev = 0; iDev < arrDevInName.GetSize(); iDev++) {
		pCombo->AddString(arrDevInName[iDev]);
	}
	m_MidiInDev = CLAMP(m_MidiInDev + 1, 0, int(arrDevInName.GetSize()));	// account for none
	pCombo->SetCurSel(m_MidiInDev);
	CStringArray	arrDevOutName;
	CMidiOut::GetDeviceNames(arrDevOutName);
	pCombo = static_cast<CComboBox *>(GetDlgItem(IDC_OPTIONS_MIDI_OUT));
	pCombo->AddString(LDS(IDS_NONE));
	for (int iDev = 0; iDev < arrDevOutName.GetSize(); iDev++) {
		pCombo->AddString(arrDevOutName[iDev]);
	}
	m_MidiOutDev = CLAMP(m_MidiOutDev + 1, 0, int(arrDevOutName.GetSize()));	// account for none
	pCombo->SetCurSel(m_MidiOutDev);
	CheckDlgButton(IDC_OPTIONS_SEED_FROM_TIME, !m_RandomSeed);
	OnClickedSeedFromTime();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void COptionsDlg::OnOK()
{
	CDialog::OnOK();
	m_MidiInDev--;	// account for none
	m_MidiOutDev--;
}

void COptionsDlg::OnClickedSeedFromTime()
{
	BOOL	bSeedFromTime = IsDlgButtonChecked(IDC_OPTIONS_SEED_FROM_TIME);
	CWnd	*pEdit = GetDlgItem(IDC_OPTIONS_RANDOM_SEED);
	pEdit->EnableWindow(!bSeedFromTime);
	if (bSeedFromTime)
		pEdit->SetWindowText(_T("0"));
}
