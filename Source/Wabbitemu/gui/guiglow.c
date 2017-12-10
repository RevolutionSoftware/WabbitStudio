#include "stdafx.h"

#include "calc.h"

extern HDC hdcSkin;

void DrawGlow(HDC hdcSkin, HDC hdc, RECT *r, COLORREF GIFGRADCOLOR, int GIFGRADWIDTH, BOOL bSkinEnabled, double skinScale) {
	
	// Create the buffer bitmap
	HDC hdcBuf = CreateCompatibleDC(hdc);
	LONG lcdWidth = r->right - r->left;
	LONG lcdHeight = r->bottom - r->top;

	HBITMAP hbmBuf = CreateCompatibleBitmap(hdc, lcdWidth + (2*GIFGRADWIDTH), lcdHeight + (2*GIFGRADWIDTH));
	SelectObject(hdcBuf, hbmBuf);
	
	if (bSkinEnabled) {
		SetStretchBltMode(hdcBuf, HALFTONE);
		StretchBlt(hdcBuf, 0, 0, lcdWidth + (2*GIFGRADWIDTH), lcdHeight + (2*GIFGRADWIDTH),
			hdcSkin, (int)((r->left - GIFGRADWIDTH) * skinScale), (int)((r->top - GIFGRADWIDTH) * skinScale),
			(int)((lcdWidth + (2 * GIFGRADWIDTH))* skinScale), (int)((lcdHeight + (2 * GIFGRADWIDTH)) * skinScale), SRCCOPY);
	} else {
		RECT rc = {0, 0, lcdWidth + (2*GIFGRADWIDTH), lcdHeight + (2*GIFGRADWIDTH)};
		FillRect(hdcBuf, &rc, GetStockBrush(GRAY_BRUSH));
	}
	
	// Set up the alpha function for the bitmap with alpha values
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;
	bf.AlphaFormat = AC_SRC_ALPHA;		
	
	// Create the header for the bitmap with alpha values
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	
	BITMAPINFOHEADER *bi = &bmi.bmiHeader;
	bi->biSize = sizeof(BITMAPINFOHEADER);
	bi->biWidth = GIFGRADWIDTH;
	bi->biHeight = r->bottom - r->top;
	bi->biPlanes = 1;
	bi->biBitCount = 32;
	bi->biCompression = BI_RGB;
	
	int width = bi->biWidth;
	int height = bi->biHeight;
	
	HDC hdcGrad = CreateCompatibleDC(hdc);
	// Create a solid brush of the gradient color
	HBRUSH hbrGrad = CreateSolidBrush(GIFGRADCOLOR);
	SelectObject(hdcGrad, hbrGrad);
	
	SelectObject(hdcGrad, GetStockObject(DC_PEN));
	SetDCPenColor(hdcGrad, GIFGRADCOLOR);
	
	BYTE *pBits;
	HBITMAP hbmGrad = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**) &pBits, NULL, 0);
	
	SelectObject(hdcGrad, hbmGrad);

	// Draw a red rectangle
	Rectangle(hdcGrad, 0, 0, width, height);
	
	int x, y;
	
	BYTE * pPixel = pBits;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			pPixel[3] = (BYTE)(255 * (x + 1) / width);
			
			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
			
		}
	}
	
	AlphaBlend(	hdcBuf, 0, GIFGRADWIDTH, width, height,
				hdcGrad, 0, 0, width, height,
				bf);
	
	// Redraw the red rectangle (since values are premultiplied)
	Rectangle(hdcGrad, 0, 0, width, height);
	
	pPixel = pBits;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			pPixel[3] = (BYTE)(255 * (width - x) / width);
			
			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
			
		}
	}
	
	AlphaBlend(	hdcBuf, r->right - r->left + GIFGRADWIDTH, GIFGRADWIDTH, width, height,
								hdcGrad, 0, 0, width, height,
								bf);
	
	DeleteObject(hbmGrad);

	bmi.bmiHeader.biWidth = width = r->right - r->left;
	bmi.bmiHeader.biHeight = height = GIFGRADWIDTH;
	
	hbmGrad = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	SelectObject(hdcGrad, hbmGrad);
	
	Rectangle(hdcGrad, 0, 0, width, height);

	for (pPixel = pBits, y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			pPixel[3] = (BYTE)(255 * (height - y) / height);
			
			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
		}
	}
	
	AlphaBlend(	hdcBuf, GIFGRADWIDTH, 0, width, height,
								hdcGrad, 0, 0, width, height,
								bf);
	
	Rectangle(hdcGrad, 0, 0, width, height);
	
	for (pPixel = pBits, y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			pPixel[3] = (BYTE)(255 * (y + 1) / height);

			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
			
		}
	}
	
	AlphaBlend(	hdcBuf, GIFGRADWIDTH, GIFGRADWIDTH + r->bottom - r->top, width, height,
								hdcGrad, 0, 0, width, height,
								bf);
	
	DeleteObject(hbmGrad);
	
	bmi.bmiHeader.biWidth = width = GIFGRADWIDTH;
	bmi.bmiHeader.biHeight = height = GIFGRADWIDTH;
	
	hbmGrad = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	SelectObject(hdcGrad, hbmGrad);
	
	Rectangle(hdcGrad, 0, 0, width, height);
	
	for (pPixel = pBits, y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			int res = (int) (255*sqrt((double) ((x*x) + (y*y))) / height);
			if (res > 255) {
				res = 255;
			}

			pPixel[3] = (BYTE)(255 - res);
			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
		}

	}

	AlphaBlend(	hdcBuf, GIFGRADWIDTH+ r->right - r->left, 0, width, height,
								hdcGrad, 0, 0, width, height,
								bf);
	
	Rectangle(hdcGrad, 0, 0, width, height);
	
	for (pPixel = pBits, y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			int res = (int) (256*sqrt((double) ((width - x - 1)*(width - x - 1)) + (y*y)) / height);
			if (res > 255) {
				res = 255;
			}

			pPixel[3] = (BYTE)(255 - res);
			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
		}

	}

	AlphaBlend(	hdcBuf, 0, 0, width, height,
								hdcGrad, 0, 0, width, height,
								bf);
	
	Rectangle(hdcGrad, 0, 0, width, height);
	
	for (pPixel = pBits, y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			int res = (int) (255*sqrt((double) (x*x) + ((height-y - 1)*(height-y - 1))) / height);
			if (res > 255) {
				res = 255;
			}

			pPixel[3] = (BYTE)(255 - res);
			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
		}

	}

	AlphaBlend(	hdcBuf, GIFGRADWIDTH+ r->right - r->left, GIFGRADWIDTH+ r->bottom - r->top, width, height,
								hdcGrad, 0, 0, width, height,
								bf);
	
	Rectangle(hdcGrad, 0, 0, width, height);
	
	for (pPixel = pBits, y = 0; y < height; y++) {
		for (x = 0; x < width; x++, pPixel+=4) {
			int res = (int) (255*sqrt((double) ((width - x - 1)*(width - x - 1)) + ((height-y -1)*(height-y-1))) / height);
			if (res > 255) res = 255;
			pPixel[3] = (BYTE)(255 - res);
			pPixel[0] = pPixel[0] * pPixel[3] / 0xFF;
			pPixel[1] = pPixel[1] * pPixel[3] / 0xFF;
			pPixel[2] = pPixel[2] * pPixel[3] / 0xFF;
		}

	}

	AlphaBlend(	hdcBuf, 0, GIFGRADWIDTH+ r->bottom - r->top, width, height,
								hdcGrad, 0, 0, width, height,
								bf);
	
	DeleteObject(hbmGrad);
	DeleteDC(hdcGrad);
	DeleteObject(hbrGrad);
	
	BitBlt(hdc, r->left - GIFGRADWIDTH, r->top - GIFGRADWIDTH, r->right - r->left + (2*GIFGRADWIDTH), r->bottom - r->top +(2*GIFGRADWIDTH), hdcBuf, 0, 0, SRCCOPY);
	DeleteObject(hbmBuf);
	DeleteDC(hdcBuf);
}