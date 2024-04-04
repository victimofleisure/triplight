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

class CSettings {
public:
// Construction
	CSettings();
	void	Load();
	void	Store();

// Constants
	enum {	// image size types
		IMAGE_SAME_SIZE_AS_WINDOW,
		IMAGE_CUSTOM_SIZE,
		IMAGE_SIZE_TYPES
	};

// Data members
	#define SETTINGSDEF(type, name, initval) type m_##name;
	#include "SettingsDef.h"	// generate code to define members
};
