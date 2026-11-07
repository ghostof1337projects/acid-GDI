#include "audio.hpp"
#include "effects.hpp"
#include "gdi_shaders.hpp"
#include "screen.hpp"

#include <algorithm>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Intensity-aware pool sequencer
// Intensity 0..1 unlocks more effects over time.
// Duration shrinks from 10s down to 3s as intensity rises.
// ---------------------------------------------------------------------------
struct Sequencer {
    // Threshold (0-100) at which each TripMode unlocks.
    // Array index matches TripMode enum value (54 total modes).
    static constexpr int kTotal = static_cast<int>(TripMode::Count); // 54
    static constexpr int kThresh[kTotal] = {
        // Tier 0 - 0% (Subtle GDI Glitches & Inversions) [8]
        0, 0, 0, 0, 0, 0, 0, 0,
        // Tier 1 - 15% (Iconic GDI Malware Payloads & Geometry) [10]
        15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
        // Tier 2 - 30% (Advanced GDI Payloads, Slicing & Radar) [9]
        30, 30, 30, 30, 30, 30, 30, 30, 30,
        // Tier 3 - 45% (Pure Fractal Universe) [10]
        45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
        // Tier 4 - 60% (Geometric & Dynamic Trips) [9]
        60, 60, 60, 60, 60, 60, 60, 60, 60,
        // Tier 5 - 75% (Peak Chaos & Total Meltdown) [8]
        75, 75, 75, 75, 75, 75, 75, 75
    };

    int   pool[kTotal] {};
    int   poolSize  = 0;
    int   pos       = 0;
    float timer     = 0.0f;
    float duration  = 9.0f;
    int   formulaIdx = 0;
    int   lastLevel  = -1; // last unlock level

    void buildPool(int pct) {
        poolSize = 0;
        for (int i = 0; i < kTotal; ++i)
            if (kThresh[i] <= pct) pool[poolSize++] = i;
    }

    void shufflePool(unsigned seed) {
        for (int i = poolSize - 1; i > 0; --i) {
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            const int j = static_cast<int>(seed % static_cast<unsigned>(i + 1));
            const int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
        }
        pos = 0;
    }

    TripMode current() const {
        return static_cast<TripMode>(pool[pos]);
    }

    // Call once per frame. Returns true when effect changes.
    bool tick(float dt, float intensity, BytebeatAudio& audio) {
        timer += dt;

        // Unlock check: rebuild + reshuffle when a new tier opens
        const int pct   = static_cast<int>(intensity * 100.0f);
        const int level = pct / 15; // unlock new tier roughly every 15%
        if (level > lastLevel) {
            buildPool(pct);
            shufflePool(static_cast<unsigned>(GetTickCount()));
            lastLevel = level;
        }

        if (timer < duration) return false;

        // Advance to next effect
        timer = 0.0f;
        ++pos;
        if (pos >= poolSize) {
            buildPool(pct);
            shufflePool(static_cast<unsigned>(GetTickCount() ^ 0xDEAD));
        }

        // Duration shrinks linearly: 5s at start -> 1s at peak
        const float maxDur = 5.0f - intensity * 4.0f;   // 5..1
        const float minDur = maxDur * 0.65f;             // 3.25..0.65
        unsigned seed = GetTickCount();
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        const float rng = static_cast<float>(seed % 2001) / 2000.0f;
        duration = std::max(0.8f, minDur + rng * (maxDur - minDur));

        // Rotate bytebeat formula
        formulaIdx = (formulaIdx + 1) % audio.formulaCount();
        audio.setFormula(formulaIdx);
        return true;
    }
};

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------
struct AppState {
    ScreenBuffer  buffer   {};
    BytebeatAudio audio    {};
    Sequencer     seq      {};
    float         globalTime = 0.0f;
    float         intensity  = 0.0f;
};

AppState* GetState(HWND hwnd) {
    return reinterpret_cast<AppState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

// ---------------------------------------------------------------------------
// HUD: effect name, countdown, intensity bar
// ---------------------------------------------------------------------------
void DrawHud(HDC hdc, const AppState& st, int w, int h) {
    SetBkMode(hdc, TRANSPARENT);

    HFONT font = CreateFontW(18,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Consolas");
    HFONT of = static_cast<HFONT>(SelectObject(hdc, font));

    const float rem = st.seq.duration - st.seq.timer;
    wchar_t line[256];
    swprintf_s(line, L"%-18s  %.1fs    ESC to exit",
               TripModeName(st.seq.current()), rem);

    // Shadow
    SetTextColor(hdc, RGB(0,0,0));
    RECT rs{13,h-37,w-11,h-7}; DrawTextW(hdc,line,-1,&rs,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    // Text
    SetTextColor(hdc, RGB(210,255,170));
    RECT rt{12,h-38,w-12,h-8}; DrawTextW(hdc,line,-1,&rt,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

    // Effect duration progress bar (bottom)
    const float efrac = std::clamp(st.seq.timer / st.seq.duration, 0.0f, 1.0f);
    HBRUSH eb = CreateSolidBrush(RGB(80,220,120));
    RECT erc{12,h-5,12+static_cast<int>((w-24)*efrac),h-2};
    FillRect(hdc,&erc,eb); DeleteObject(eb);

    // Intensity bar (top) - green -> yellow -> red
    const float ifrac = std::clamp(st.intensity, 0.0f, 1.0f);
    const int ir = static_cast<int>(ifrac * 255);
    const int ig = static_cast<int>((1.0f - ifrac) * 220);
    HBRUSH ib = CreateSolidBrush(RGB(ir, ig, 20));
    RECT irc{0,0,static_cast<int>(w*ifrac),4};
    FillRect(hdc,&irc,ib); DeleteObject(ib);

    // Intensity label (tiny, top-right)
    wchar_t ilabel[32];
    swprintf_s(ilabel, L"INTENSITY %.0f%%", ifrac*100.0f);
    SetTextColor(hdc, RGB(ir, ig, 20));
    RECT il{w-180,6,w-8,22}; DrawTextW(hdc,ilabel,-1,&il,DT_RIGHT|DT_SINGLELINE);

    SelectObject(hdc,of); DeleteObject(font);
}

// In-place chaos overlay applied at high intensity on top of the rendered frame.
// Uses SRCINVERT/DSTINVERT on the already-rendered frontDC — no extra swap needed.
static void ApplyIntensityOverlay(HDC dc, int w, int h, float time, float intensity) {
    if (intensity < 0.58f) return;
    const float t = (intensity - 0.58f) / 0.42f; // 0..1
    const int ops  = static_cast<int>(t * 12.0f);
    unsigned seed  = static_cast<unsigned>(time * 80.0f) ^ 0xBEEF;
    for (int i = 0; i < ops; ++i) {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        const int x = (seed >> 1) % w;
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        const int y = (seed >> 1) % h;
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        const int bw = std::max(4, static_cast<int>((seed & 0xFF) * w / 512));
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        const int bh = std::max(2, static_cast<int>((seed & 0xFF) * h / 512));
        const int ox = static_cast<int>((int)(seed >> 8 & 0xFF) - 128) / 4;
        const int sx = std::clamp(x + ox, 0, w - 1);
        const int uw = std::min(bw, w - std::max(x, sx));
        const int uh = std::min(bh, h - y);
        if (uw > 0 && uh > 0)
            BitBlt(dc, x<w-uw?x:0, y<h-uh?y:0, uw, uh, dc, sx, y, SRCINVERT);
    }
    // Flash full invert briefly near peak
    if (intensity > 0.88f && static_cast<int>(time * 7.0f) % 6 == 0)
        PatBlt(dc, 0, 0, w, h, DSTINVERT);
}

// ---------------------------------------------------------------------------
// Frame rendering: captures desktop, applies effect, overlays HUD, presents directly to screen DC
// ---------------------------------------------------------------------------
void RenderFrame(AppState& st) {
    st.buffer.captureDesktop();
    SetEffectIntensity(st.intensity);
    RunEffect(st.seq.current(), st.buffer, st.seq.timer);
    
    // At high intensity: apply in-place chaos pass
    ApplyIntensityOverlay(st.buffer.frontDC(), st.buffer.width, st.buffer.height,
                          st.globalTime, st.intensity);
                          
    // Draw HUD on the front buffer before presenting to screen
    DrawHud(st.buffer.frontDC(), st, st.buffer.width, st.buffer.height);
    
    // Present to real desktop display DC
    HDC screenDC = GetDC(nullptr);
    if (screenDC) {
        st.buffer.present(screenDC);
        ReleaseDC(nullptr, screenDC);
    }
}

// ---------------------------------------------------------------------------
// Entry point (Classic GDI Malware Architecture: Live desktop interaction)
// ---------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    // Enable Per-Monitor V2 DPI Awareness for exact multi-monitor virtual screen metrics
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    std::srand(static_cast<unsigned>(GetTickCount64()));

    AppState st{};
    if (!st.buffer.init()) {
        MessageBoxW(nullptr, L"Screen buffer init failed.", L"Acid", MB_ICONERROR);
        return 1;
    }

    // Boot into gentle pool (intensity=0)
    st.seq.buildPool(0);
    st.seq.shufflePool(static_cast<unsigned>(GetTickCount64()));
    st.seq.lastLevel = 0;
    st.seq.duration  = 5.0f;   // start slow, decreases over time

    st.audio.init();
    st.audio.setFormula(0);

    ULONGLONG lastTick = GetTickCount64();

    // Main animation loop
    while (true) {
        // Process any pending Windows messages
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto cleanup;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Instant escape key listener (works globally regardless of focused window)
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;
        }

        ULONGLONG now = GetTickCount64();
        float dt = static_cast<float>(now - lastTick) / 1000.0f;
        if (dt <= 0.0f) dt = 0.001f;
        if (dt > 0.1f)  dt = 0.1f;
        lastTick = now;

        st.globalTime += dt;
        // 2.5-minute ramp to full intensity
        st.intensity = std::min(1.0f, st.globalTime / 150.0f);

        // When effect finishes: clean up resources and force full desktop redraw so no artifacts persist
        if (st.seq.tick(dt, st.intensity, st.audio)) {
            CleanupEffectResources();
            InvalidateRect(nullptr, nullptr, TRUE);
            RedrawWindow(nullptr, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }

        RenderFrame(st);

        // Frame limiter ~60fps
        Sleep(16);
    }

cleanup:
    // Fully clear and restore the desktop screen
    InvalidateRect(nullptr, nullptr, TRUE);
    RedrawWindow(nullptr, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);

    st.audio.cleanup();
    st.buffer.cleanup();
    CleanupEffectResources();
    return 0;
}
