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

#pragma once

#include "TripLightDoc.h"

// CMappingDlg dialog

class CMappingDlg : public CDialog
{
	DECLARE_DYNAMIC(CMappingDlg)

// Construction
public:
	CMappingDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMappingDlg();

// Dialog Data
	enum { IDD = IDD_MAPPING };
	CMappingArray	m_arrMapping;

protected:
// Constants
	static const int m_arrEventTypeNameID[CMapping::EVENT_TYPES];

// Overrides
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

// Message map functions
	DECLARE_MESSAGE_MAP()
};
