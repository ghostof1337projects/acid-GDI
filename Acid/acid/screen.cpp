#include "screen.hpp"

bool ScreenBuffer::init() {
    // Specifically target the first (primary) monitor
    HMONITOR hMon = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{ sizeof(MONITORINFO) };
    if (hMon && GetMonitorInfoW(hMon, &mi)) {
        x = mi.rcMonitor.left;
        y = mi.rcMonitor.top;
        width = mi.rcMonitor.right - mi.rcMonitor.left;
        height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    } else {
        x = 0;
        y = 0;
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    screenDC_ = GetDC(nullptr);
    if (!screenDC_) {
        return false;
    }

    frontDC_ = CreateCompatibleDC(screenDC_);
    backDC_ = CreateCompatibleDC(screenDC_);
    if (!frontDC_ || !backDC_) {
        cleanup();
        return false;
    }

    frontBitmap_ = CreateCompatibleBitmap(screenDC_, width, height);
    backBitmap_ = CreateCompatibleBitmap(screenDC_, width, height);
    if (!frontBitmap_ || !backBitmap_) {
        cleanup();
        return false;
    }

    oldFront_ = static_cast<HBITMAP>(SelectObject(frontDC_, frontBitmap_));
    oldBack_ = static_cast<HBITMAP>(SelectObject(backDC_, backBitmap_));
    return true;
}

void ScreenBuffer::captureDesktop() {
    HDC sdc = GetDC(nullptr);
    if (sdc) {
        BitBlt(frontDC_, 0, 0, width, height, sdc, x, y, SRCCOPY);
        ReleaseDC(nullptr, sdc);
    }
}

void ScreenBuffer::present(HDC targetDC) const {
    BitBlt(targetDC, x, y, width, height, frontDC_, 0, 0, SRCCOPY);
}

void ScreenBuffer::swap() {
    HDC tempDC = frontDC_;
    frontDC_ = backDC_;
    backDC_ = tempDC;

    HBITMAP tempBitmap = frontBitmap_;
    frontBitmap_ = backBitmap_;
    backBitmap_ = tempBitmap;

    HBITMAP tempOld = oldFront_;
    oldFront_ = oldBack_;
    oldBack_ = tempOld;
}

void ScreenBuffer::cleanup() {
    if (frontDC_ && oldFront_) {
        SelectObject(frontDC_, oldFront_);
    }
    if (backDC_ && oldBack_) {
        SelectObject(backDC_, oldBack_);
    }

    if (frontBitmap_) {
        DeleteObject(frontBitmap_);
        frontBitmap_ = nullptr;
    }
    if (backBitmap_) {
        DeleteObject(backBitmap_);
        backBitmap_ = nullptr;
    }
    if (frontDC_) {
        DeleteDC(frontDC_);
        frontDC_ = nullptr;
    }
    if (backDC_) {
        DeleteDC(backDC_);
        backDC_ = nullptr;
    }
    if (screenDC_) {
        ReleaseDC(nullptr, screenDC_);
        screenDC_ = nullptr;
    }
}
