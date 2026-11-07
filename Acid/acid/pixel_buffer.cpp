#include "pixel_buffer.hpp"

bool PixelBuffer::init(HDC reference, int w, int h) {
    width = w;
    height = h;

    dc_ = CreateCompatibleDC(reference);
    if (!dc_) {
        return false;
    }

    BITMAPINFO bmi {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    bitmap_ = CreateDIBSection(dc_, &bmi, DIB_RGB_COLORS,
                               reinterpret_cast<void**>(&bits_), nullptr, 0);
    if (!bitmap_ || !bits_) {
        cleanup();
        return false;
    }

    oldBitmap_ = static_cast<HBITMAP>(SelectObject(dc_, bitmap_));
    return true;
}

void PixelBuffer::copyFrom(HDC source, int srcX, int srcY) {
    BitBlt(dc_, 0, 0, width, height, source, srcX, srcY, SRCCOPY);
}

void PixelBuffer::copyTo(HDC dest, int dstX, int dstY) const {
    BitBlt(dest, dstX, dstY, width, height, dc_, 0, 0, SRCCOPY);
}

void PixelBuffer::cleanup() {
    if (dc_ && oldBitmap_) {
        SelectObject(dc_, oldBitmap_);
        oldBitmap_ = nullptr;
    }
    if (bitmap_) {
        DeleteObject(bitmap_);
        bitmap_ = nullptr;
    }
    if (dc_) {
        DeleteDC(dc_);
        dc_ = nullptr;
    }
    bits_ = nullptr;
}
