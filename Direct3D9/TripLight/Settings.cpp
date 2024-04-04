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

#pragma once

#include "stdafx.h"
#include "TripLight.h"
#include "Settings.h"
#include "RegWrap.h"

#define RK_SETTINGS_SECTION _T("Settings")

CSettings::CSettings()
{
	#define SETTINGSDEF(type, name, initval) m_##name = initval;
	#include "SettingsDef.h"	// generate code to initialize read members
}

void CSettings::Load()
{
	#define SETTINGSDEF(type, name, initval) RdReg(RK_SETTINGS_SECTION, _T(#name), m_##name);
	#include "SettingsDef.h"	// generate code to read members
}

void CSettings::Store()
{
	#define SETTINGSDEF(type, name, initval) WrReg(RK_SETTINGS_SECTION, _T(#name), m_##name);
	#include "SettingsDef.h"	// generate code to write members
}
