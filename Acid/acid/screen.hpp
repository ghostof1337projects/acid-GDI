#pragma once

#define NOMINMAX
#include <windows.h>

// Double-buffered desktop capture for GDI effect passes.
// Supports multi-monitor virtual desktop configurations.
class ScreenBuffer {
public:
    int x = 0;      // Virtual screen left offset (SM_XVIRTUALSCREEN)
    int y = 0;      // Virtual screen top offset (SM_YVIRTUALSCREEN)
    int width = 0;  // Virtual screen total width (SM_CXVIRTUALSCREEN)
    int height = 0; // Virtual screen total height (SM_CYVIRTUALSCREEN)

    bool init();
    void captureDesktop();
    void present(HDC targetDC) const;
    void cleanup();

    HDC frontDC() const { return frontDC_; }
    HDC backDC() const { return backDC_; }

    void swap();

private:
    HDC screenDC_ = nullptr;
    HDC frontDC_ = nullptr;
    HDC backDC_ = nullptr;
    HBITMAP frontBitmap_ = nullptr;
    HBITMAP backBitmap_ = nullptr;
    HBITMAP oldFront_ = nullptr;
    HBITMAP oldBack_ = nullptr;
};
