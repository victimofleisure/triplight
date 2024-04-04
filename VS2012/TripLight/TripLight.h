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
		02		01apr24	update for VS2012

		TripLight application
 
*/

// TripLight.h : main header file for the TripLight application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "MidiWrap.h"
#include "Settings.h"


// CTripLightApp:
// See TripLight.cpp for the implementation of this class
//

class CMainFrame;

class CTripLightApp : public CWinAppEx
{
public:
	CTripLightApp();

// Attributes
	CMainFrame	*GetMain();
	CString	GetAppPath();
	CString	GetAppFolder();
	bool	IsMidiInputDeviceOpen() const;
	bool	IsMidiOutputDeviceOpen() const;
	int		GetMidiInputDevice() const;
	int		GetMidiOutputDevice() const;

// Operations
	bool	OpenMidiInputDevice(int iDevIn);
	bool	OpenMidiOutputDevice(int iDevOut);
	void	MidiOut(DWORD nMsg);

// Public data
	CSettings	m_settings;		// persistent settings that aren't part of document

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL IsIdleMessage(MSG* pMsg);

// Implementation
	BOOL  m_bHiColorIcons;

protected:
// Data members
	int		m_iMidiDevIn;		// MIDI input device index, or -1 if none
	int		m_iMidiDevOut;		// MIDI output device index, or -1 if none
	CMidiIn	m_midiIn;			// MIDI input device instance
	CMidiOut	m_midiOut;		// MIDI output device instance

// Helpers
	void	OnMidiError(MMRESULT nResult);
	static	void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, W64UINT dwInstance, W64UINT dwParam1, W64UINT dwParam2);

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

inline CMainFrame *CTripLightApp::GetMain()
{
	return((CMainFrame *)m_pMainWnd);
}

inline bool CTripLightApp::IsMidiInputDeviceOpen() const
{
	return m_iMidiDevIn >= 0;
}

inline bool CTripLightApp::IsMidiOutputDeviceOpen() const
{
	return m_iMidiDevOut >= 0;
}

inline int CTripLightApp::GetMidiInputDevice() const
{
	return m_iMidiDevIn;
}

inline int CTripLightApp::GetMidiOutputDevice() const
{
	return m_iMidiDevOut;
}

inline void CTripLightApp::MidiOut(DWORD nMsg)
{
	m_midiOut.OutShortMsg(nMsg);
}

extern CTripLightApp theApp;
