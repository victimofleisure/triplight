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

//				type	name			initval
SETTINGSDEF(	int,	ImageSizeType,	CSettings::IMAGE_SAME_SIZE_AS_WINDOW)
SETTINGSDEF(	int,	ImageWidth,		1024)
SETTINGSDEF(	int,	ImageHeight,	768)
SETTINGSDEF(	int,	RecordSeconds,	300)

#undef SETTINGSDEF
