#pragma once

#define NOMINMAX
#include <windows.h>

#pragma pack(push, 1)
struct BgrPixel {
    BYTE b;
    BYTE g;
    BYTE r;
    BYTE a;
};
#pragma pack(pop)

// 32-bpp DIB section for per-pixel GDI shader passes (RGBQUAD-style access).
class PixelBuffer {
public:
    int width = 0;
    int height = 0;

    bool init(HDC reference, int w, int h);
    void cleanup();

    HDC dc() const { return dc_; }
    BgrPixel* bits() const { return bits_; }
    size_t byteSize() const { return static_cast<size_t>(width) * height * sizeof(BgrPixel); }

    void copyFrom(HDC source, int srcX, int srcY);
    void copyTo(HDC dest, int dstX, int dstY) const;

private:
    HDC dc_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HBITMAP oldBitmap_ = nullptr;
    BgrPixel* bits_ = nullptr;
};
