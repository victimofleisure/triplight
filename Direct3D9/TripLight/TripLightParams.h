// Copyleft 2013 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
		chris korda
 
		revision history:
		rev		date	comments
        00      25dec15	initial version
 		01		15mar23	add MIDI mapping

		view parameters

*/

#ifdef TLPARAMDEF

//			type	name				init
TLPARAMDEF(	int,	Cols,				4)	// number of columns
TLPARAMDEF(	int,	Rows,				3)	// number of rows
TLPARAMDEF(	int,	Pattern,			CTripLightView::ALT_ROWS)	// pattern bitmask; see enum
TLPARAMDEF(	int,	ColorSpeed,			90)	// color speed, in color delta per tick
TLPARAMDEF(	int,	ColorSpeedStep,		5)	// color speed step size
TLPARAMDEF(	int,	ColorSpeedVar,		6)	// color speed variation, as percentage of color speed
TLPARAMDEF(	int,	ColorSpeedVarStep,	1)	// color speed variation step size

#undef TLPARAMDEF

#endif

#ifdef MAPPINGDEF

MAPPINGDEF(	int,	Target,		0)		// index of target property
MAPPINGDEF(	int,	Event,		0)		// event type; see enum in CMapping
MAPPINGDEF(	int,	Channel,	0)		// channel index, range 0 to 15
MAPPINGDEF(	int,	Control,	0)		// control or note index, range 0 to 127
MAPPINGDEF(	int,	Start,		0)		// start value of target range
MAPPINGDEF(	int,	End,		127)	// end value of target range

#undef MAPPINGDEF

#endif

#ifdef OPTIONSDEF

OPTIONSDEF(	BOOL,	StartFullScreen,	0)		// if true, start in full screen mode
OPTIONSDEF(	int,	MidiInDev,			-1)		// MIDI input device index, or -1 if none
OPTIONSDEF(	int,	MidiOutDev,			-1)		// MIDI output device index, or -1 if none
OPTIONSDEF(	UINT,	RandomSeed,			0)		// random seed, or 0 to use system time

#undef OPTIONSDEF

#endif
