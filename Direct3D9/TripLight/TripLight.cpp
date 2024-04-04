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

// TripLight.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "TripLight.h"
#include "MainFrm.h"
#include "TripLightDoc.h"
#include "TripLightView.h"

#include "Win32Console.h"
#include "VersionInfo.h"
#include "PathStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CHECK_MIDI(x) { MMRESULT nResult = x; if (MIDI_FAILED(nResult)) { OnMidiError(nResult); return false; } }

// CTripLightApp

BEGIN_MESSAGE_MAP(CTripLightApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CTripLightApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// CTripLightApp construction

CTripLightApp::CTripLightApp()
{
	m_bHiColorIcons = TRUE;

	// replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("AnalSoftware.TripLight.Production.1.0"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_iMidiDevIn = -1;
	m_iMidiDevOut = -1;
}

// The one and only CTripLightApp object

CTripLightApp theApp;


// CTripLightApp initialization

BOOL CTripLightApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

//@@@#ifdef _DEBUG
	Win32Console::Create();
//#endif

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Anal Software"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CTripLightDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CTripLightView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);



	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CTripLightApp::ExitInstance()
{
	AfxOleTerm(FALSE);
	if (m_iMidiDevOut >= 0) {
		m_midiOut.Reset();
	}
	m_settings.Store();
	return CWinAppEx::ExitInstance();
}

// CTripLightApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

	CStatic	m_AboutText;
	CEdit	m_License;
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ABOUT_LICENSE, m_License);
	DDX_Control(pDX, IDC_ABOUT_TEXT, m_AboutText);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CString	s;
#ifdef _WIN64
	GetWindowText(s);
	s += _T(" (x64)");
	SetWindowText(s);
#endif
	VS_FIXEDFILEINFO	AppInfo;
	CVersionInfo::GetFileInfo(AppInfo, NULL);
	s.Format(IDS_APP_ABOUT_TEXT, theApp.m_pszAppName,
		HIWORD(AppInfo.dwFileVersionMS), LOWORD(AppInfo.dwFileVersionMS),
		HIWORD(AppInfo.dwFileVersionLS), LOWORD(AppInfo.dwFileVersionLS));
	m_AboutText.SetWindowText(s);
	m_License.SetWindowText(LDS(IDS_APP_LICENSE));
	return TRUE;
}

// App command to run the dialog
void CTripLightApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// By default, CWinApp::OnIdle is called after WM_TIMER messages.  This isn't
// normally a problem, but if the application uses a short timer, OnIdle will
// be called frequently, seriously degrading performance.  Performance can be
// improved by overriding IsIdleMessage to return FALSE for WM_TIMER messages,
// which prevents them from triggering OnIdle.  This technique can be applied
// to any idle-triggering message that repeats frequently, e.g. WM_MOUSEMOVE.
//
BOOL CTripLightApp::IsIdleMessage(MSG* pMsg)
{
	if (CWinApp::IsIdleMessage(pMsg)) {
		switch (pMsg->message) {	// don't call OnIdle after these messages
		case UWM_FRAME_TIMER:
			return FALSE;
		default:
			return TRUE;
		}
	} else
		return FALSE;
}

// CTripLightApp message handlers

bool CTripLightApp::OpenMidiInputDevice(int iDevIn)
{
	if (iDevIn == m_iMidiDevIn)	// if already in requested state
		return true;	// nothing to do
	if (m_iMidiDevIn >= 0) {	// if device open
		CHECK_MIDI(m_midiIn.Close());
		m_iMidiDevIn = -1;
	}
	if (iDevIn >= 0) {	// if opening device
		CHECK_MIDI(m_midiIn.Open(iDevIn, reinterpret_cast<W64UINT>(MidiInProc), reinterpret_cast<W64UINT>(this), CALLBACK_FUNCTION));
		CHECK_MIDI(m_midiIn.Start());
		m_iMidiDevIn = iDevIn;
	}
	return true;
}

bool CTripLightApp::OpenMidiOutputDevice(int iDevOut)
{
	if (iDevOut == m_iMidiDevOut)	// if already in requested state
		return true;	// nothing to do
	if (m_iMidiDevOut >= 0) {	// if device open
		CHECK_MIDI(m_midiOut.Close());
		m_iMidiDevOut = -1;
	}
	if (iDevOut >= 0) {	// if opening device
		CHECK_MIDI(m_midiOut.Open(iDevOut, NULL, NULL, 0));
		m_iMidiDevOut = iDevOut;
	}
	return true;
}

void CALLBACK CTripLightApp::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, W64UINT dwInstance, W64UINT dwParam1, W64UINT dwParam2)
{
	// this callback function runs in a worker thread context; 
	// data shared with main thread may require serialization
	static CDWordArrayEx	arrMappedEvent;
	UNREFERENCED_PARAMETER(hMidiIn);
	UNREFERENCED_PARAMETER(dwInstance);
//	_tprintf(_T("MidiInProc %d %d\n"), GetCurrentThreadId(), ::GetThreadPriority(GetCurrentThread()));
	switch (wMsg) {
	case MIM_DATA:
		{
			CMainFrame	*pFrame = theApp.GetMain();
			if (pFrame != NULL) {
				CTripLightView	*pView = theApp.GetMain()->GetView();
				if (pView != NULL) {
					pView->PostMessage(UWM_MAPPING_CHANGE, dwParam1);
				}
			}
		}
		break;
	}
}

void CTripLightApp::OnMidiError(MMRESULT nResult)
{
	AfxMessageBox(CMidiOut::GetErrorString(nResult));
}


