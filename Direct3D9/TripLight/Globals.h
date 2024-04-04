// Copyleft 2012 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		08mar10	initial version
		01		01jul14	add lead sheet file extension
		02		09sep14	add undo test flags
		03		29sep14	add OpenThread for MFC 6
		04		05apr15	add chord dictionary change message
		05		29apr15	remove hide sizing bar message
		06		11jun15	remove chord dictionary change message
		07		02aug23	add snapshot file extension

		global definitions and inlines

*/

#pragma once

#pragma warning(disable : 4100)	// unreferenced formal parameter

// minimal base for non-CObject classes
#include "WObject.h"

// registry strings
#define REG_SETTINGS		_T("Settings")

// key status bits for GetKeyState and GetAsyncKeyState
#define GKS_TOGGLED			0x0001
#define GKS_DOWN			0x8000

// clamp a value to a range
#define CLAMP(x, lo, hi) (min(max((x), (lo)), (hi)))

// trap bogus default case in switch statement
#define NODEFAULTCASE	ASSERT(0)

// load string from resource via temporary object
#define LDS(x) CString(MAKEINTRESOURCE(x))

#if _MFC_VER < 0x0800
// calculate number of elements in a fixed-size array
#define _countof(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

// atof's generic-text wrapper is missing in MFC 6
#ifndef _tstof
#ifdef UNICODE
#define _tstof(x) _tcstod(x, NULL)
#else
#define _tstof(x) atof(x)
#endif
#endif

#if _MFC_VER < 0x0800
#define genericException generic	// generic was deprecated in .NET 2005
#endif

#if _MFC_VER < 0x0700
#define TBS_DOWNISLEFT          0x0400  // Down=Left and Up=Right (default is Down=Right and Up=Left)
#endif

#if _MFC_VER < 0x0700
// MFC 6 CArchive doesn't support bool
inline CArchive& operator<<(CArchive& ar, bool b)
	{ return ar << ((BYTE)b); }
inline CArchive& operator>>(CArchive& ar, bool& b)
	{ return ar >> ((BYTE&)b); }
#endif

#if _MSC_VER < 1300
#define ACTIVATEAPPTASK HTASK
#else
#define ACTIVATEAPPTASK DWORD
#endif

#ifndef WS_EX_COMPOSITED	// supported from XP on, but omitted in MFC6
#define WS_EX_COMPOSITED        0x02000000L
#endif

// x64 wrappers
#include "Wrapx64.h"

// optimized rounding and truncation
#include "Round.h"

#if _MFC_VER > 0x0600
// suppress spurious level 4 warning on ceil function
#pragma warning (push)
#pragma warning (disable: 4985)	// attributes not present on previous declaration.
#include <math.h>
#pragma warning (pop)
#endif

// replace AfxGetApp with faster method
class CTripLightApp;
extern CTripLightApp theApp;
inline CWinApp *FastGetApp()
{
	return reinterpret_cast<CWinApp*>(&theApp);
}
#define AfxGetApp FastGetApp

// define benchmarking macros
#define BENCH_START CBenchmark b;
#define BENCH_STOP printf("%f\n", b.Elapsed());

// app-specific globals

#define SNAPSHOT_FILE_EXT _T(".tripsnap")

enum {	// user-defined app windows messages
	UWM_FIRST = WM_APP,
	UWM_FRAME_TIMER,		// wParam: unused, lParam: unused
	UWM_MAPPING_CHANGE,		// wParam: MIDI event, lParam: unused
	UWM_MODELESS_DESTROY,	// wParam: CWnd*, lParam: none
};
