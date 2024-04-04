// Copyleft 2008 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00      14feb08	initial version
		01		16feb09	in Create, don't set members until after Destroy
		02		06jan10	W64: in Serialize, use 32-bit size of BITMAP
		03		23oct15	add color table support
		04		23sep22	rename rounding methods

        device-independent bitmap
 
*/

#include "stdafx.h"
#include "Dib.h"

CDib::CDib()
{
	m_hDib = NULL;
	m_pBits = NULL;
}

CDib::~CDib()
{
	Destroy();
}

void CDib::Copy(const CDib& Dib)
{
	BITMAP	bmp;
	if (GetObject(Dib, sizeof(BITMAP), &bmp)) {
		Destroy();
		if (Create(bmp.bmWidth, bmp.bmHeight, bmp.bmBitsPixel))
			memcpy(m_pBits, Dib.m_pBits, bmp.bmHeight * bmp.bmWidthBytes);
	}
}

void CDib::Destroy()
{
	if (m_hDib != NULL) {
		DeleteObject(m_hDib);
		m_hDib = NULL;
		m_pBits = NULL;
	}
}

bool CDib::Create(int Width, int Height, WORD BitCount)
{
	BITMAPINFO	bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = Width;
	bi.bmiHeader.biHeight = Height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = BitCount;
	PVOID	pBits;
	HBITMAP	hDib = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
	if (hDib == NULL)
		return(FALSE);
	Destroy();
	m_hDib = hDib;	// copy to members
	m_pBits = pBits;
	return(TRUE);
}

bool CDib::Create(const CBitmap& Bitmap)
{
	BITMAP	bmp;
	if (GetObject(Bitmap, sizeof(bmp), &bmp)) {
		CWindowDC	dc(NULL);
		Destroy();
		if (Create(bmp.bmWidth, bmp.bmHeight, bmp.bmBitsPixel)) {
			BITMAPINFO	bi;
			ZeroMemory(&bi, sizeof(bi));
			bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bi.bmiHeader.biWidth = bmp.bmWidth;
			bi.bmiHeader.biHeight = bmp.bmHeight;
			bi.bmiHeader.biPlanes = bmp.bmPlanes;
			bi.bmiHeader.biBitCount = bmp.bmBitsPixel;
			return(GetDIBits(dc, Bitmap, 0, bmp.bmHeight, m_pBits, 
				&bi, DIB_RGB_COLORS) != 0);
		}
	}
	return(FALSE);
}

bool CDib::CreateBitmap(CBitmap& Bitmap) const
{
	BITMAP	bmp;
	if (GetBitmap(&bmp)) {
		CWindowDC	dc(NULL);
		HBITMAP	hBmp = CreateCompatibleBitmap(dc, bmp.bmWidth, bmp.bmHeight);
		if (hBmp != NULL) {
			BITMAPINFO	bi;
			ZeroMemory(&bi, sizeof(bi));
			bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bi.bmiHeader.biWidth = bmp.bmWidth;
			bi.bmiHeader.biHeight = bmp.bmHeight;
			bi.bmiHeader.biPlanes = bmp.bmPlanes;
			bi.bmiHeader.biBitCount = bmp.bmBitsPixel;
			if (SetDIBits(dc, hBmp, 0, bmp.bmHeight, m_pBits,
				&bi, DIB_RGB_COLORS)) {
				return(Bitmap.Attach(hBmp) != 0);
			}
		}
	}
	return(FALSE);
} 

bool CDib::Read(LPCTSTR Path)
{
	HBITMAP	hDib = (HBITMAP)LoadImage(NULL, Path, IMAGE_BITMAP, 
			0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (hDib == NULL)
		return(FALSE);
	Destroy();
	m_hDib = hDib;
	BITMAP	bmp;
	if (GetBitmap(&bmp)) {
		m_pBits = bmp.bmBits;
		return(TRUE);
	}
	return(FALSE);
}

bool CDib::Write(CFile& File, int Resolution)
{
	DIBSECTION	ds;
	if (!GetObject(m_hDib, sizeof(DIBSECTION), &ds))
		return(FALSE);
	UINT	nColors;
	if (ds.dsBm.bmBitsPixel <= 8)
		nColors = 1 << ds.dsBm.bmBitsPixel;
	else
		nColors = 0;	// no color table
	BITMAPFILEHEADER	bfh;
	ZeroMemory(&bfh, sizeof(BITMAPFILEHEADER));
	bfh.bfType = 0x4d42;	// BM
	UINT	ColorTblSize = nColors * sizeof(RGBQUAD);
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + ColorTblSize;
	bfh.bfSize = bfh.bfOffBits + ds.dsBmih.biSizeImage;
	static const double INCHES_PER_METER = .0254;
	int	MetricRes = Round(Resolution / INCHES_PER_METER);
	ds.dsBmih.biXPelsPerMeter = MetricRes;
	ds.dsBmih.biYPelsPerMeter = MetricRes;
	File.Write(&bfh, sizeof(BITMAPFILEHEADER));
	File.Write(&ds.dsBmih, sizeof(BITMAPINFOHEADER));
	if (nColors) {
		CDWordArray	ColorTbl;
		ColorTbl.SetSize(nColors);
		if (GetColorTable(ColorTbl.GetData(), nColors) != nColors)
			return(FALSE);
		File.Write(ColorTbl.GetData(), ColorTblSize);
	}
	File.Write(ds.dsBm.bmBits, ds.dsBmih.biSizeImage);
	return(TRUE);
}

bool CDib::Write(LPCTSTR Path, int Resolution, CFileException *pError)
{
	TRY {
		CFile	File(Path, CFile::modeCreate | CFile::modeWrite);
		if (!Write(File, Resolution))
			return(FALSE);
	}
	CATCH(CFileException, e)
	{
		if (pError != NULL) {
			pError->m_lOsError = e->m_lOsError;
			pError->m_cause = e->m_cause;
			pError->m_strFileName = e->m_strFileName;
		}
		return(FALSE);
	}
	END_CATCH
	return(TRUE);
}

void CDib::Serialize(CArchive& ar)
{
	// The BITMAP struct got bigger in 64-bit Windows, due to the bmBits member
	// being a pointer. To keep our archives compatible with 32-bit Windows, we
	// must use the original size of BITMAP. The 64-bit load case leaves bmBits
	// uninitialized, but since we don't use bmBits here it doesn't matter.
#ifdef _WIN64
	static const int BITMAP_SIZE = 24;	// size of BITMAP in 32-bit Windows
#else
	static const int BITMAP_SIZE = sizeof(BITMAP);
#endif
	if (ar.IsStoring()) {
		BITMAP	bmp;
		if (m_pBits == NULL || !GetBitmap(&bmp))
			AfxThrowArchiveException(CArchiveException::genericException, ar.m_strFileName);
		ar.Write(&bmp, BITMAP_SIZE);
		ar.Write(m_pBits, bmp.bmWidthBytes * bmp.bmHeight);
	} else {
		BITMAP	bmp;
		ar.Read(&bmp, BITMAP_SIZE);
		if (!Create(bmp.bmWidth, bmp.bmHeight, bmp.bmBitsPixel))
			AfxThrowArchiveException(CArchiveException::genericException, ar.m_strFileName);
		ar.Read(m_pBits, bmp.bmWidthBytes * bmp.bmHeight);
	}
}

void CDib::Swap(CDib& Dib)
{
	HBITMAP	hDib = m_hDib;
	m_hDib = Dib.m_hDib;
	Dib.m_hDib = hDib;
	PVOID	pBits = m_pBits;
	m_pBits = Dib.m_pBits;
	Dib.m_pBits = pBits;
}

void CDib::Attach(HBITMAP Bitmap, PVOID Bits)
{
	Destroy();
	m_hDib = Bitmap;
	m_pBits = Bits;
}

HBITMAP CDib::Detach(PVOID& Bits)
{
	HBITMAP	hDib = m_hDib;
	Bits = m_pBits;
	m_hDib = NULL;
	m_pBits = NULL;
	return(hDib);
}

UINT CDib::SetColorTable(const COLORREF *pColor, UINT nEntries)
{
	CWindowDC	dc(NULL);
	CDC	DibDC;
	if (!DibDC.CreateCompatibleDC(&dc))
		return(0);
	HGDIOBJ	hPrevBmp = DibDC.SelectObject(*this);
	UINT	retc = SetDIBColorTable(DibDC, 0, nEntries, (RGBQUAD *)pColor);
	DibDC.SelectObject(hPrevBmp);
	return(retc);
}

UINT CDib::GetColorTable(COLORREF *pColor, UINT nEntries)
{
	CWindowDC	dc(NULL);
	CDC	DibDC;
	if (!DibDC.CreateCompatibleDC(&dc))
		return(0);
	HGDIOBJ	hPrevBmp = DibDC.SelectObject(*this);
	UINT	retc = GetDIBColorTable(DibDC, 0, nEntries, (RGBQUAD *)pColor);
	DibDC.SelectObject(hPrevBmp);
	return(retc);
}

// pDibBits	bitmap data; 24 bits per pixel ONLY
// Width	bitmap width, in pixels; must be evenly divisible by Factor
// Height	bitmap height, in pixels; must be evenly divisible by Factor
// Stride	memory required for a line, in bytes
// Factor	downsampling factor; operation is done in place
//
void CDib::Downsample(PVOID pDibBits, int Width, int Height, int Stride, int Factor)
{
	Width /= Factor;
	Height /= Factor;
	int	DstStride = ((Width + 1) * 3) & 0xfffc;
	int	FactorSquared = Factor * Factor;
	int	FactorTimes3 = Factor * 3;
	int	StrideTimesFactor = Stride * Factor;
	int	Delta1 = Stride - FactorTimes3;
	int	Delta2 = StrideTimesFactor - FactorTimes3;
	PBYTE	pDst = (PBYTE)pDibBits;
	PBYTE	pSrc = (PBYTE)pDibBits;
	for (int y = 0; y < Height; y++) {
		BYTE	*pSaveDst = pDst;
		BYTE	*pSaveSrc = pSrc;
		for (int x = 0; x < Width; x++) {
			int	r = 0;
			int	g = 0;
			int	b = 0;
			for (int yi = 0; yi < Factor; yi++) {
				for (int xi = 0; xi < Factor; xi++) {
					r += *pSrc++;
					g += *pSrc++;
					b += *pSrc++;
				}
				pSrc += Delta1;
			}
			pSrc -= Delta2;
			*pDst++ = BYTE(r / FactorSquared);
			*pDst++ = BYTE(g / FactorSquared);
			*pDst++ = BYTE(b / FactorSquared);
		}
		pSrc = pSaveSrc + StrideTimesFactor;
		pDst = pSaveDst + DstStride;
	}
}

void CDib::Letterbox(const CRect& Rect1, CSize Rect2, CRect& Result)
{
	CSize	sz1 = Rect1.Size();
	CRect	r(CPoint(0, 0), sz1);
	if (sz1.cx && sz1.cy) {	// avoid divide by zero
		CSize	sz2 = Rect2;
		float	asp1 = float(sz1.cy) / sz1.cx;
		float	asp2 = float(sz2.cy) / sz2.cx;
		if (asp2 > asp1)
			r.right = Round(float(sz2.cx) * sz1.cy / sz2.cy);
		else
			r.bottom = Round(float(sz2.cy) * sz1.cx / sz2.cx);
		if (r.right < sz1.cx)
			r.OffsetRect((sz1.cx - r.right) / 2, 0);
		if (r.bottom < sz1.cy)
			r.OffsetRect(0, (sz1.cy - r.bottom) / 2);
	}
	r += Rect1.TopLeft();
	Result = r;
}
