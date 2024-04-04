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

// MappingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TripLight.h"
#include "MappingDlg.h"
#include "Midi.h"

// CMappingDlg dialog

IMPLEMENT_DYNAMIC(CMappingDlg, CDialog)

const int CMappingDlg::m_arrEventTypeNameID[CMapping::EVENT_TYPES] = {
	#define MIDICHANSTATDEF(name) IDS_MIDI_EVENT_TYPE_##name,
	#include "MidiCtrlrDef.h"
};

CMappingDlg::CMappingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMappingDlg::IDD, pParent)
{
	m_arrMapping.SetSize(CTripLightDoc::MAPPING_TARGETS);
}

CMappingDlg::~CMappingDlg()
{
}

void CMappingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	for (int iTarg = 0; iTarg < CTripLightDoc::MAPPING_TARGETS; iTarg++) {
		CMapping&	map = m_arrMapping[iTarg];
		DDX_CBIndex(pDX, IDC_MAPPING_CHANNEL1 + iTarg, map.m_Channel);
		DDX_Text(pDX, IDC_MAPPING_CONTROL1 + iTarg, map.m_Control);
		DDV_MinMaxInt(pDX, map.m_Control, 0, 127);
		DDX_Text(pDX, IDC_MAPPING_END1 + iTarg, map.m_End);
		DDX_Text(pDX, IDC_MAPPING_START1 + iTarg, map.m_Start);
		DDX_CBIndex(pDX, IDC_MAPPING_EVENT1 + iTarg, map.m_Event);
	}
}

BEGIN_MESSAGE_MAP(CMappingDlg, CDialog)
END_MESSAGE_MAP()

// CMappingDlg message handlers

BOOL CMappingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	for (int iTarg = 0; iTarg < CTripLightDoc::MAPPING_TARGETS; iTarg++) {
		CMapping&	map = m_arrMapping[iTarg];
		map.m_Target = 0;	// so IsDefault doesn't give false positives
		CWnd	*pCtrl = GetDlgItem(IDC_MAPPING_EVENT1 + iTarg);
		CComboBox	*pCombo = static_cast<CComboBox *>(pCtrl);
		pCombo->AddString(LDS(IDS_NONE));	// replaces note off event which is unused
		for (int iType = CMapping::EVT_NOTE_ON; iType < CMapping::EVENT_TYPES; iType++) {
			pCombo->AddString(LDS(m_arrEventTypeNameID[iType]));
		}
		pCombo->SetCurSel(map.m_Event);
		pCtrl = GetDlgItem(IDC_MAPPING_CHANNEL1 + iTarg);
		pCombo = static_cast<CComboBox *>(pCtrl);
		for (int iChan = 0; iChan < MIDI_CHANNELS; iChan++) {
			CString	sChan;
			sChan.Format(_T("%d"), iChan + 1);
			pCombo->AddString(sChan);
		}
		pCombo->SetCurSel(map.m_Channel);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
}
