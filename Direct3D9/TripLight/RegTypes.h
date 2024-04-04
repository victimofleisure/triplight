// Copyleft 2015 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      24mar15	initial version

        define types for registry template specializations
 
*/

// These macros generate specializations of the RdReg and WrReg
// registry templates for the following built-in numeric types.
// Each type must fit in the 32 bits of a DWORD registry key.

//				type		prefix
REG_TYPE_DEF(	int,		(int))
REG_TYPE_DEF(	long,		(int))
REG_TYPE_DEF(	UINT,		(int))
REG_TYPE_DEF(	DWORD,		(int))
REG_TYPE_DEF(	short,		(int))
REG_TYPE_DEF(	WORD,		(int))
REG_TYPE_DEF(	char,		(int))
REG_TYPE_DEF(	BYTE,		(int))
REG_TYPE_DEF(	bool,		0!=)	// avoids performance warning

#undef REG_TYPE_DEF
