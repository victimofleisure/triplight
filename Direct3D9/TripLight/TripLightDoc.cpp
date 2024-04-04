// Copyleft 2015 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      25dec15	initial version
 		01		15mar23	add MIDI support
		02		02aug23	restore snapshot support

		TripLight document
 
*/

// TripLightDoc.cpp : implementation of the CTripLightDoc class
//

#include "stdafx.h"
#include "TripLight.h"

#include "TripLightDoc.h"
#include "TripLightView.h"
#include "MainFrm.h"
#include "IniFile.h"
#include "RegWrap.h"
#include "MappingDlg.h"
#include "PropertiesDlg.h"
#include "OptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RK_MAPPING_COUNT _T("Mappings")
#define RK_MAPPING_SECTION _T("Mapping\\")

CMapping::CMapping()
{
	#define MAPPINGDEF(type, name, init) m_##name = init;
	#include "TripLightParams.h"	// generate code to initialize members
}

bool CMapping::IsDefault() const
{
	#define MAPPINGDEF(type, name, init) if (m_##name != init) return false;
	#include "TripLightParams.h"	// generate code to test members
	return TRUE;
}

void CMapping::Read(LPCTSTR pszSection)
{
	#define MAPPINGDEF(type, name, init) RdReg(pszSection, _T(#name), m_##name);
	#include "TripLightParams.h"	// generate code to read members
}

void CMapping::Write(LPCTSTR pszSection)
{
	#define MAPPINGDEF(type, name, init) WrReg(pszSection, _T(#name), m_##name);
	#include "TripLightParams.h"	// generate code to write members
}

void CMappingArray::Read()
{
	int	nMappings = 0;
	RdReg(theApp.m_pszAppName, RK_MAPPING_COUNT, nMappings);
	SetSize(nMappings);
	CString	sSection(RK_MAPPING_SECTION);
	CString	sNum;
	for (int iMap = 0; iMap < nMappings; iMap++) {
		sNum.Format(_T("%d"), iMap);
		GetAt(iMap).Read(sSection + sNum);
	}
}

void CMappingArray::Write()
{
	int	nMappings = GetSize();
	if (nMappings) {
		WrReg(theApp.m_pszAppName, RK_MAPPING_COUNT, nMappings);
		CString	sSection(RK_MAPPING_SECTION);
		CString	sNum;
		for (int iMap = 0; iMap < nMappings; iMap++) {
			sNum.Format(_T("%d"), iMap);
			GetAt(iMap).Write(sSection + sNum);
		}
	}
}

#define RK_OPTS_MIDI_IN_DEVICE _T("MidiInDev")
#define RK_OPTS_MIDI_OUT_DEVICE _T("MidiOutDev")
#define RK_OPTS_FULL_SCREEN _T("FullScreen")

COptions::COptions()
{
	#define OPTIONSDEF(type, name, init) m_##name = init;
	#include "TripLightParams.h"	// generate code to initialize members
}

void COptions::Read()
{
	#define OPTIONSDEF(type, name, init) RdReg(theApp.m_pszAppName, _T(#name), m_##name);
	#include "TripLightParams.h"	// generate code to define members
}

void COptions::Write()
{
	#define OPTIONSDEF(type, name, init) if (m_##name != init) WrReg(theApp.m_pszAppName, _T(#name), m_##name);
	#include "TripLightParams.h"	// generate code to define members
}

/////////////////////////////////////////////////////////////////////////////
// CTripLightDoc

IMPLEMENT_DYNCREATE(CTripLightDoc, CDocument)

BEGIN_MESSAGE_MAP(CTripLightDoc, CDocument)
	ON_COMMAND(ID_TOOLS_MAPPING, OnToolsMapping)
	ON_COMMAND(ID_TOOLS_PROPERTIES, OnToolsProperties)
	ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTripLightDoc construction/destruction

#define RK_FILE_VERSION _T("FileVersion")
#define FILE_VERSION 1

CTripLightDoc::CTripLightDoc()
{
	m_nFileVersion = FILE_VERSION;
	ResetData();
}

CTripLightDoc::~CTripLightDoc()
{
}

void CTripLightDoc::ResetData()
{
	CTripLightParams	paramsDefault;
	CTripLightParams&	params = *this;
	params = paramsDefault;
	m_arrMapping.RemoveAll();
	COptions	optsDefault;
	m_options = optsDefault;
}

BOOL CTripLightDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	ResetData();

	return TRUE;
}

CTripLightParams::CTripLightParams()
{
	#define TLPARAMDEF(type, name, init) m_##name = init;
	#include "TripLightParams.h"	// generate code to initialize members
}

void CTripLightParams::SerializeSnapshot(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << SNAPSHOT_FILE_ID;
		ar << SNAPSHOT_FILE_VERSION;
		#define TLPARAMDEF(type, name, init) ar << m_##name;
		#include "TripLightParams.h"	// generate code to store members
	}
	else
	{
		int	FileID;
		ar >> FileID;
		if (FileID != SNAPSHOT_FILE_ID)
			AfxThrowArchiveException(CArchiveException::badIndex, ar.GetFile()->GetFilePath());
		int	FileVersion;
		ar >> FileVersion;
		#define TLPARAMDEF(type, name, init) ar >> m_##name;
		#include "TripLightParams.h"	// generate code to load members
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTripLightDoc serialization

void CTripLightDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTripLightDoc diagnostics

#ifdef _DEBUG
void CTripLightDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CTripLightDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTripLightDoc commands

BOOL CTripLightDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	LPCTSTR	pszExt = PathFindExtension(lpszPathName);
	if (pszExt != NULL && !_tcsicmp(pszExt, SNAPSHOT_FILE_EXT)) {	// if file extension is snapshot
		CTripLightView	*pView = STATIC_DOWNCAST(CTripLightView, theApp.GetMain()->GetActiveView());
		pView->LoadSnapshot(lpszPathName);
		return FALSE;
	}
	CIniFile	file;
	file.Open(lpszPathName, CFile::modeRead);
	int	nFileVersion = theApp.GetProfileInt(theApp.m_pszAppName, RK_FILE_VERSION, -1);
	if (nFileVersion < 0)	// if invalid file version
		AfxThrowArchiveException(CArchiveException::badIndex, lpszPathName);	// throw invalid format
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	m_nFileVersion = nFileVersion;
	ResetData();
	#define TLPARAMDEF(type, name, init) RdReg(theApp.m_pszAppName, _T(#name), m_##name);
	#include "TripLightParams.h"	// generate code to read members from INI file
	m_arrMapping.Read();
	m_options.Read();
	return TRUE;
}

BOOL CTripLightDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{	
	if (!CDocument::OnSaveDocument(lpszPathName))
		return FALSE;
	CIniFile	file;
	file.Open(lpszPathName, CFile::modeCreate | CFile::modeWrite);
	CTripLightParams&	params = *this;
	theApp.GetMain()->GetView()->GetParams(params);
	WrReg(theApp.m_pszAppName, RK_FILE_VERSION, FILE_VERSION);
	#define TLPARAMDEF(type, name, init) WrReg(theApp.m_pszAppName, _T(#name), m_##name);
	#include "TripLightParams.h"	// generate code to write members to INI file
	m_arrMapping.Write();
	m_options.Write();
	return TRUE;
}

void CTripLightDoc::OnToolsMapping()
{
	CMappingDlg	dlg;
	int	nMaps = m_arrMapping.GetSize();
	for (int iMap = 0; iMap < nMaps; iMap++) {
		const CMapping&	map = m_arrMapping[iMap];
		dlg.m_arrMapping[map.m_Target] = map;
	}
	if (dlg.DoModal() == IDOK) {
		m_arrMapping.RemoveAll();
		for (int iTarg = 0; iTarg < MAPPING_TARGETS; iTarg++) {
			CMapping&	map = dlg.m_arrMapping[iTarg];
			if (!map.IsDefault()) {
				map.m_Target = iTarg;
				m_arrMapping.Add(map);
			}
		}
		SetModifiedFlag();
	}
}

void CTripLightDoc::OnToolsProperties()
{
	CPropertiesDlg	dlg;
	theApp.GetMain()->GetView()->GetParams(dlg);
	if (dlg.DoModal() == IDOK) {
		CTripLightParams&	paramsDoc = *this;
		paramsDoc = dlg;
		SetModifiedFlag();
		UpdateAllViews(NULL, HINT_NONE);
	}
}

void CTripLightDoc::OnToolsOptions()
{
	COptionsDlg	dlg;
	COptions&	optsDlg = dlg;
	optsDlg = m_options;
	if (dlg.DoModal() == IDOK) {
		m_options = dlg;
		SetModifiedFlag();
		UpdateAllViews(NULL, HINT_OPTIONS);
	}
}
