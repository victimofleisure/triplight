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

// TripLightDoc.h : interface of the CTripLightDoc class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ArrayEx.h"

class CTripLightParams {
public:
	CTripLightParams();
	#define TLPARAMDEF(type, name, init) type m_##name;
	#include "TripLightParams.h"	// generate code to define members
	void	SerializeSnapshot(CArchive& ar);
	enum {
		SNAPSHOT_FILE_ID = 0x70697254,	// Trip
		SNAPSHOT_FILE_VERSION = 0,
	};
};

class CMapping {
public:
	CMapping();
	#define MAPPINGDEF(type, name, init) type m_##name;
	#include "TripLightParams.h"	// generate code to define members
	enum {	// MIDI event types
		#define MIDICHANSTATDEF(name) EVT_##name,
		#include "MidiCtrlrDef.h"
		EVENT_TYPES
	};
	bool	IsDefault() const;
	void	Read(LPCTSTR pszSection);
	void	Write(LPCTSTR pszSection);
};

class CMappingArray : public CArrayEx<CMapping, CMapping&> {
public:
	void	Read();
	void	Write();
};

class COptions {
public:
	COptions();
	#define OPTIONSDEF(type, name, init) type m_##name;
	#include "TripLightParams.h"	// generate code to define members
	void	Read();
	void	Write();
};

class CTripLightDoc : public CDocument, public CTripLightParams
{
protected: // create from serialization only
	CTripLightDoc();
	DECLARE_DYNCREATE(CTripLightDoc)

// Constants
	enum {	// update hints
		HINT_NONE,
		HINT_OPTIONS,
	};
	enum {	// mapping targets
		MT_COLUMNS,
		MT_ROWS,
		MT_PATTERN,
		MT_SPEED,
		MT_VARIANCE,
		MT_JUMP,
		MT_FADE,
		MAPPING_TARGETS
	};

// Attributes
public:
	CMappingArray	m_arrMapping;	// array of MIDI mappings
	COptions	m_options;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);

// Implementation
public:
	virtual ~CTripLightDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	int		m_nFileVersion;	// file version number

// Helpers
	void	ResetData();

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnToolsMapping();
	afx_msg void OnToolsProperties();
	afx_msg void OnToolsOptions();
};

