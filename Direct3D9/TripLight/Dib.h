// Copyleft 2008 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      14feb08	initial version
		01		23oct15	add color table support
		02		04nov15	in Write, add default resolution

        device-independent bitmap
 
*/

#ifndef CDIB_INCLUDED
#define CDIB_INCLUDED

class CDib : public WObject {
public:
// Construction
	CDib();
	~CDib();
	CDib(const CDib& Dib);
	CDib& operator=(const CDib& Dib);
	bool	Create(int Width, int Height, WORD BitCount);
	bool	Create(const CBitmap& Bitmap);
	bool	CreateBitmap(CBitmap& Bitmap) const;
	void	Destroy();

// Attributes
	operator HBITMAP() const;
	bool	IsEmpty() const;
	PVOID	GetBits() const;
	bool	GetBitmap(BITMAP *pBmp) const;
	static	int	GetStride(const BITMAP& Bmp);
	UINT	SetColorTable(const COLORREF *pColor, UINT nEntries);
	UINT	GetColorTable(COLORREF *pColor, UINT nEntries);

// Operations
	bool	Write(CFile& File, int Resolution = 72);
	bool	Write(LPCTSTR Path, int Resolution = 72, CFileException *pError = NULL);
	bool	Read(LPCTSTR Path);
	void	Serialize(CArchive& ar);
	void	Swap(CDib& Dib);
	void	Attach(HBITMAP Bitmap, PVOID Bits);
	HBITMAP Detach(PVOID& Bits);
	static	void	Downsample(PVOID pDibBits, int Width, int Height, int Stride, int Factor);
	static	void	Letterbox(const CRect& Rect1, CSize Rect2, CRect& Result);

// Helpers
	void	Copy(const CDib& Dib);

protected:
// Data members
	HBITMAP	m_hDib;
	PVOID	m_pBits;
};

inline CDib::CDib(const CDib& Dib)
{
	Copy(Dib);
}

inline CDib& CDib::operator=(const CDib& Dib)
{
	Copy(Dib);
	return(*this);
}

inline CDib::operator HBITMAP() const
{
	return(m_hDib);
}

inline bool CDib::IsEmpty() const
{
	return(m_hDib == NULL);
}

inline PVOID CDib::GetBits() const
{
	return(m_pBits);
}

inline bool CDib::GetBitmap(BITMAP *pBmp) const
{
	return(GetObject(m_hDib, sizeof(BITMAP), pBmp) != 0); 
}

inline int CDib::GetStride(const BITMAP& Bmp)
{
	return(((Bmp.bmWidthBytes - 1) & ~3) + 4);	// round up to 4-byte boundary
}

#endif
