#include "effects.hpp"
#include "gdi_shaders.hpp"
#include "pixel_buffer.hpp"
#include "screen.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

namespace {
constexpr float kPi = std::numbers::pi_v<float>;

PixelBuffer g_pixels;
PixelBuffer g_fractalLayer;
bool g_pixelsReady = false;
float g_intensity  = 0.0f;

PixelBuffer& EnsurePixels(HDC ref, int w, int h) {
    if (!g_pixelsReady || g_pixels.width != w || g_pixels.height != h) {
        g_pixels.cleanup(); g_fractalLayer.cleanup();
        g_pixels.init(ref, w, h);
        g_fractalLayer.init(ref, w / 2, h / 2);
        g_pixelsReady = true;
    }
    return g_pixels;
}

void CopyBaseFrame(const ScreenBuffer& b) {
    BitBlt(b.backDC(), 0, 0, b.width, b.height, b.frontDC(), 0, 0, SRCCOPY);
}

int WaveOff(float t, float spd, float amp, int idx) {
    return static_cast<int>(std::sin(t * spd + idx * 0.37f) * amp);
}

ColorRgb HueCycleTime(float t) { return HueCycle(t * 40.0f); }

void ApplySineRipple(HDC dc, int w, int h, float t) {
    double a = t * 4.0;
    for (int y = 0; y < h; y += 2) {
        BitBlt(dc, 0, y, w, 2, dc, static_cast<int>(std::sin(a) * 24.0), y, SRCCOPY);
        a += kPi / 28.0;
    }
}

void StretchToFractalLayer(HDC src, int srcW, int srcH) {
    SetStretchBltMode(g_fractalLayer.dc(), COLORONCOLOR);
    StretchBlt(g_fractalLayer.dc(), 0, 0, g_fractalLayer.width, g_fractalLayer.height,
               src, 0, 0, srcW, srcH, SRCCOPY);
}

unsigned Xorshift(unsigned s) { s^=s<<13; s^=s>>17; s^=s<<5; return s; }

} // namespace

void SetEffectIntensity(float t) { g_intensity = std::clamp(t, 0.0f, 1.0f); }

const wchar_t* TripModeName(TripMode mode) {
    switch (mode) {
    case TripMode::ColorInvert:       return L"COLOR INVERT";
    case TripMode::ScreenGlitch:      return L"SCREEN GLITCH";
    case TripMode::ScreenMelt:        return L"SCREEN MELT";
    case TripMode::ChannelGhost:      return L"CHANNEL GHOST";
    case TripMode::HorizTear:         return L"HORIZ TEAR";
    case TripMode::WaveDistort:       return L"WAVE DISTORT";
    case TripMode::ScreenEcho:        return L"SCREEN ECHO";
    case TripMode::Checkerboard:      return L"CHECKERBOARD";

    case TripMode::ScreenTunnel:      return L"SCREEN TUNNEL";
    case TripMode::ScreenShake:       return L"SCREEN SHAKE";
    case TripMode::IconSpam:          return L"ICON SPAM";
    case TripMode::CursorStorm:       return L"CURSOR STORM";
    case TripMode::ErrorDialogCascade:return L"ERROR DIALOGS";
    case TripMode::GridMirror:        return L"GRID MIRROR";
    case TripMode::Pixelate:          return L"PIXELATE";
    case TripMode::StrobeMirror:      return L"STROBE MIRROR";
    case TripMode::RGBSplit:          return L"RGB SPLIT";
    case TripMode::RadialRipples:     return L"RADIAL RIPPLES";

    case TripMode::SliceTear:         return L"SLICE TEAR";
    case TripMode::DiagonalShear:     return L"DIAGONAL SHEAR";
    case TripMode::PieSliceInvert:    return L"RADAR PIE INVERT";
    case TripMode::LogoBounce:        return L"LOGO BOUNCE";
    case TripMode::HatchStorm:        return L"HATCH STORM";
    case TripMode::PlgRotate:         return L"PLG ROTATE";
    case TripMode::Plasma:            return L"PLASMA";
    case TripMode::Interference:      return L"INTERFERENCE";
    case TripMode::MatrixSpam:        return L"MATRIX SPAM";

    case TripMode::Mandelbrot:        return L"MANDELBROT";
    case TripMode::JuliaFractal:      return L"JULIA FRACTAL";
    case TripMode::NewtonFractal:     return L"NEWTON FRACTAL";
    case TripMode::Lyapunov:          return L"LYAPUNOV FRACTAL";
    case TripMode::BarnsleyFern:      return L"BARNSLEY FERN";
    case TripMode::Apollonian:        return L"APOLLONIAN GASKET";
    case TripMode::DragonCurve:       return L"DRAGON CURVE";
    case TripMode::KochSnowflake:     return L"KOCH SNOWFLAKE";
    case TripMode::Sierpinski:        return L"SIERPINSKI";
    case TripMode::BurningShip:       return L"BURNING SHIP";

    case TripMode::Mandala:           return L"MANDALA";
    case TripMode::WarpTunnel:        return L"WARP TUNNEL";
    case TripMode::ZoomFeedback:      return L"ZOOM FEEDBACK";
    case TripMode::Helix:             return L"HELIX";
    case TripMode::Mushrooms:         return L"MUSHROOMS";
    case TripMode::VortexSpin:        return L"VORTEX SPIN";
    case TripMode::Cube3D:            return L"3D CUBE";
    case TripMode::Geosphere3D:       return L"3D GEOSPHERE";
    case TripMode::Acid:              return L"ACID";

    case TripMode::DrugRain:          return L"DRUG RAIN";
    case TripMode::RgbShader:         return L"RGB SHADER";
    case TripMode::XorStorm:          return L"XOR STORM";
    case TripMode::ChromaBlast:       return L"CHROMA BLAST";
    case TripMode::MeltChaos:         return L"MELT CHAOS";
    case TripMode::Salvia:            return L"SALVIA";
    case TripMode::Datura:            return L"DATURA";
    case TripMode::TunnelDoom:        return L"TUNNEL DOOM";
    default: return L"???";
    }
}

// ===========================================================================
// TIER 0 - Subtle GDI Glitches & Inversions (0%)
// ===========================================================================

void RunColorInvertEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    CopyBaseFrame(b);
    const float spd = 0.6f + g_intensity*2.8f;
    const int bandH = std::max(2, static_cast<int>(h * (0.04f - g_intensity*0.025f)));
    const int numBands = 1 + static_cast<int>(g_intensity * 14.0f);
    for (int i = 0; i < numBands; ++i) {
        const float ph = t*spd + i * (1.0f/numBands) * 2.0f*kPi;
        const int y = static_cast<int>((std::sin(ph)*0.5f+0.5f) * (h-bandH));
        PatBlt(dst, 0, y, w, bandH, DSTINVERT);
    }
    if (g_intensity > 0.25f) {
        const int shift = static_cast<int>((g_intensity-0.25f)/(1.0f-0.25f) * 20.0f);
        BitBlt(dst, shift, 0, w-shift, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunScreenGlitchEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    CopyBaseFrame(b);
    const int epoch = static_cast<int>(t * 18.0f);
    unsigned seed = static_cast<unsigned>(epoch) ^ 0xF0CA;
    const int numOps = 2 + static_cast<int>(g_intensity * 32.0f);
    for (int i = 0; i < numOps; ++i) {
        seed = Xorshift(seed);
        const int rx = (seed>>1)%w, ry = (seed>>9)%h;
        seed = Xorshift(seed);
        const int rw = std::max(4,(int)((seed&0xFF)*w*(0.04f+g_intensity*0.30f)/255));
        const int rh = std::max(2,(int)((seed>>8&0xFF)*h*(0.01f+g_intensity*0.14f)/255));
        const int cw = std::min(rw,w-rx), ch = std::min(rh,h-ry);
        if (cw<=0||ch<=0) continue;
        switch (i%4) {
        case 0: PatBlt(dst,rx,ry,cw,ch,DSTINVERT); break;
        case 1: { int ox=((int)(seed>>16&0xFF)-128)>>3, oy=((int)(seed>>24&0xFF)-128)>>3;
                  BitBlt(dst,rx,ry,cw,ch,src,std::clamp(rx+ox,0,w-1),std::clamp(ry+oy,0,h-1),SRCINVERT); break; }
        case 2: BitBlt(dst,0,ry,w,1,src,((int)(seed>>10&0xFF)-128)>>3,ry,SRCCOPY); break;
        case 3: BitBlt(dst,rx,ry,cw,ch,src,rx,ry,SRCPAINT); break;
        }
    }
    const int jx = static_cast<int>((seed&3)-1) * std::max(1,(int)(g_intensity*6));
    if (jx) BitBlt(dst,jx,0,w-std::abs(jx),h,dst,0,0,SRCCOPY);
    b.swap();
}

void RunScreenMeltEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    const int colW = std::max(2, 6 - static_cast<int>(g_intensity * 3.0f));
    const float speed = 140.0f + g_intensity * 320.0f;
    for (int x = 0; x < w; x += colW) {
        float colSeed = static_cast<float>((x * 9301 + 49297) % 233280) / 233280.0f;
        int drop = static_cast<int>(std::fmod(t * speed * (0.6f + colSeed * 0.8f), static_cast<float>(h)));
        int cw = std::min(colW, w - x);
        if (drop > 0 && drop < h && cw > 0) {
            BitBlt(dst, x, drop, cw, h - drop, src, x, 0, SRCCOPY);
            if (g_intensity > 0.15f) {
                PatBlt(dst, x, 0, cw, std::min(drop, 16), DSTINVERT);
            }
        }
    }
    if (g_intensity > 0.35f) {
        int ry = (static_cast<int>(t * 45.0f) % (h / 10)) * 10;
        int roff = static_cast<int>(std::sin(t * 12.0f) * (18.0f + g_intensity * 40.0f));
        BitBlt(dst, roff, ry, w, 10, src, 0, ry, SRCINVERT);
    }
    b.swap();
}

void RunChannelGhostEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    CopyBaseFrame(b);
    const float spd = 1.0f + g_intensity*1.8f;
    const float amp = 4.0f + g_intensity*28.0f;
    const int rx = static_cast<int>(std::sin(t*spd)*amp);
    const int ry = static_cast<int>(std::cos(t*spd*0.7f)*amp*0.4f);
    if (rx>0) BitBlt(dst,rx,0,w-rx,h,src,0,0,SRCPAINT);
    else      BitBlt(dst,0,0,w+rx,h,src,-rx,0,SRCPAINT);

    const int gx = static_cast<int>(std::sin(t*spd*0.8f+kPi*0.66f)*amp*0.7f);
    if (gx>0) BitBlt(dst,0,0,w-gx,h,src,gx,0,SRCAND);
    else      BitBlt(dst,-gx,0,w+gx,h,src,0,0,SRCAND);

    const int bx = static_cast<int>(std::sin(t*spd*1.1f+kPi*1.33f)*amp*0.5f);
    BitBlt(dst,std::abs(bx),ry,w-std::abs(bx),h-std::abs(ry),src,0,0,SRCINVERT);
    b.swap();
}

void RunHorizTearEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    CopyBaseFrame(b);
    const float spd = 2.0f + g_intensity*7.0f;
    const int tears = 3 + static_cast<int>(g_intensity*30.0f);
    const float maxOff = 8.0f + g_intensity*90.0f;
    for (int i = 0; i < tears; ++i) {
        const float ph = t*spd + i*1.37f;
        const int y = static_cast<int>(std::fmod(ph*h*0.14f,(float)h));
        const int lh = 1 + static_cast<int>(g_intensity*5.0f);
        const int off = static_cast<int>(std::sin(ph*2.9f)*maxOff);
        if (y<0||y+lh>=h) continue;
        const int sx=std::clamp(off,-(w-1),w-1);
        BitBlt(dst,std::max(0,sx),y,w-std::abs(sx),lh,src,std::max(0,-sx),y,SRCCOPY);
        if (g_intensity>0.2f) {
            const ColorRgb tc=HslToRgb(t*60.0f+i*35.0f,0.9f,0.55f);
            HBRUSH tb=CreateSolidBrush(RGB(tc.r,tc.g,tc.b));
            HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,tb));
            PatBlt(dst,0,y,w,1,PATINVERT);
            SelectObject(dst,ob); DeleteObject(tb);
        }
    }
    if (g_intensity>0.4f) {
        const int sh=static_cast<int>(g_intensity*22.0f*std::sin(t*3.1f));
        BitBlt(dst,sh,0,w,h,src,0,0,SRCINVERT);
    }
    b.swap();
}

void RunWaveDistortEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    const float spd=1.5f+g_intensity*4.0f;
    const float amp=8.0f+g_intensity*70.0f;
    const float freq=kPi/(16.0f-g_intensity*12.0f);
    for (int y=0;y<h;++y) {
        const int off=static_cast<int>(std::sin(y*freq+t*spd)*amp);
        const int sx=std::clamp(off,-(w-1),w-1);
        BitBlt(dst,std::max(0,sx),y,w-std::abs(sx),1,src,std::max(0,-sx),y,SRCCOPY);
    }
    if (g_intensity>0.3f) {
        const int sh=static_cast<int>(g_intensity*12.0f*std::sin(t*2.5f));
        BitBlt(dst,sh,0,w,h,src,0,0,SRCINVERT);
    }
    b.swap();
}

void RunScreenEchoEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    CopyBaseFrame(b);
    const int echoes = 2 + static_cast<int>(g_intensity*8.0f);
    const float spd=1.0f+g_intensity*2.0f;
    for (int i=1;i<=echoes;++i) {
        const float ph = t*spd + i*0.9f;
        const int ox = static_cast<int>(std::sin(ph)*( 6.0f+g_intensity*30.0f));
        const int oy = static_cast<int>(std::cos(ph*0.7f)*(4.0f+g_intensity*20.0f));
        const int bx=std::max(0,ox),by=std::max(0,oy);
        const int bw=w-std::abs(ox),bh=h-std::abs(oy);
        if (bw>0&&bh>0)
            BitBlt(dst,bx,by,bw,bh,src,std::max(0,-ox),std::max(0,-oy),
                   i%2==0?SRCPAINT:SRCAND);
    }
    b.swap();
}

void RunCheckerboardEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    const int sz = std::max(4, 16 - static_cast<int>(g_intensity * 10.0f));
    const float speed = 1.8f + g_intensity * 3.5f;
    const int cx = w / 2, cy = h / 2;
    for (int y = 0; y < h; y += sz) {
        for (int x = 0; x < w; x += sz) {
            float dist = std::sqrt(static_cast<float>((x - cx) * (x - cx) + (y - cy) * (y - cy)));
            bool invert = static_cast<int>(dist / (sz * 2.0f) - t * speed) % 2 == 0;
            if (((x / sz) + (y / sz)) % 2 == 0 && invert) {
                PatBlt(dst, x, y, sz, sz, DSTINVERT);
            }
        }
    }
    if (g_intensity > 0.3f) {
        int off = static_cast<int>(std::sin(t * 3.5f) * 12.0f);
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCPAINT);
    }
    b.swap();
}

// ===========================================================================
// TIER 1 - Iconic GDI Malware Payloads & Geometry (15%)
// ===========================================================================

void RunScreenTunnelEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    float shrink = 0.88f - 0.05f * std::sin(t * 2.5f) - g_intensity * 0.04f;
    int dw = static_cast<int>(w * shrink);
    int dh = static_cast<int>(h * shrink);
    int dx = (w - dw) / 2;
    int dy = (h - dh) / 2;
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, dx, dy, dw, dh, src, 0, 0, w, h, SRCCOPY);
    if (static_cast<int>(t * 4.0f) % 3 == 0) {
        PatBlt(dst, dx, dy, dw, 2, DSTINVERT);
        PatBlt(dst, dx, dy + dh - 2, dw, 2, DSTINVERT);
        PatBlt(dst, dx, dy, 2, dh, DSTINVERT);
        PatBlt(dst, dx + dw - 2, dy, 2, dh, DSTINVERT);
    }
    if (g_intensity > 0.3f) {
        int off = static_cast<int>(std::sin(t * 4.0f) * 12.0f);
        BitBlt(dst, off, 0, w, h, dst, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunScreenShakeEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    const float amp = 6.0f + g_intensity * 35.0f;
    int jx = static_cast<int>(std::sin(t * 48.0f) * amp + (std::rand() % 9 - 4));
    int jy = static_cast<int>(std::cos(t * 36.0f) * (amp * 0.7f) + (std::rand() % 9 - 4));
    
    BitBlt(dst, jx, jy, w - std::abs(jx), h - std::abs(jy),
           src, std::max(0, -jx), std::max(0, -jy), SRCCOPY);
           
    if (g_intensity > 0.15f) {
        int rx = -jx * 2;
        int ry = -jy;
        BitBlt(dst, std::max(0, rx), std::max(0, ry), w - std::abs(rx), h - std::abs(ry),
               src, std::max(0, -rx), std::max(0, -ry), SRCPAINT);
        BitBlt(dst, std::max(0, -rx), std::max(0, -ry), w - std::abs(rx), h - std::abs(ry),
               src, std::max(0, rx), std::max(0, ry), SRCAND);
    }
    b.swap();
}

void RunIconSpamEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    
    if (g_intensity > 0.25f) {
        BitBlt(dst, 3, 3, w - 6, h - 6, src, 0, 0, SRCPAINT);
    }
    
    static const LPCWSTR kIcons[] = {
        IDI_WARNING, IDI_ERROR, IDI_QUESTION, IDI_INFORMATION, IDI_SHIELD, IDI_APPLICATION
    };
    const int numIcons = 8 + static_cast<int>(g_intensity * 24.0f);
    const int iconBaseSize = 32;
    
    for (int i = 0; i < numIcons; ++i) {
        LPCWSTR iconId = kIcons[i % 6];
        HICON hIcon = LoadIconW(nullptr, iconId);
        if (!hIcon) continue;
        
        float speed = 1.2f + (i % 5) * 0.4f;
        float phase = t * speed + i * (2.0f * kPi / numIcons);
        
        int cx = w / 2 + static_cast<int>(std::sin(phase * 1.3f + i) * (w * 0.42f));
        int cy = h / 2 + static_cast<int>(std::cos(phase * 1.1f + i * 0.7f) * (h * 0.42f));
        
        int isz = iconBaseSize + static_cast<int>((1.0f + std::sin(t * 3.0f + i)) * 12.0f);
        DrawIconEx(dst, cx - isz / 2, cy - isz / 2, hIcon, isz, isz, 0, nullptr, DI_NORMAL);
        
        if (g_intensity > 0.4f && (i % 2 == 0)) {
            PatBlt(dst, cx - isz / 2, cy - isz / 2, isz, isz, DSTINVERT);
        }
    }
    b.swap();
}

void RunCursorStormEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    if (g_intensity > 0.25f) {
        BitBlt(dst, 2, 2, w - 4, h - 4, src, 0, 0, SRCPAINT);
    }
    static const LPCWSTR kCursors[] = {
        IDC_ARROW, IDC_WAIT, IDC_HAND, IDC_CROSS, IDC_HELP, IDC_NO, IDC_UPARROW
    };
    const int numCursors = 14 + static_cast<int>(g_intensity * 28.0f);
    for (int i = 0; i < numCursors; ++i) {
        HCURSOR hCur = LoadCursorW(nullptr, kCursors[i % 7]);
        if (!hCur) continue;
        float speed = 1.0f + (i % 6) * 0.35f;
        float ph = t * speed + i * (2.0f * kPi / numCursors);
        int cx = w / 2 + static_cast<int>(std::sin(ph * 1.5f + i * 0.7f) * (w * 0.44f));
        int cy = h / 2 + static_cast<int>(std::cos(ph * 1.2f + i * 0.5f) * (h * 0.44f));
        DrawIconEx(dst, cx - 16, cy - 16, hCur, 32, 32, 0, nullptr, DI_NORMAL);
        if (g_intensity > 0.4f && (i % 2 == 0)) {
            PatBlt(dst, cx - 16, cy - 16, 32, 32, DSTINVERT);
        }
    }
    b.swap();
}

void RunErrorDialogCascadeEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    
    SetBkMode(dst, TRANSPARENT);
    HFONT font = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
    HFONT oldFont = static_cast<HFONT>(SelectObject(dst, font));

    const int numDialogs = 3 + static_cast<int>(g_intensity * 9.0f);
    const int dw = std::min(320, w - 20);
    const int dh = 130;

    for (int i = 0; i < numDialogs; ++i) {
        float spd = 0.8f + i * 0.25f;
        float ph = t * spd + i * 1.2f;
        int dx = static_cast<int>(std::fmod(ph * 140.0f + i * 60.0f, static_cast<float>(w - dw + 40))) - 20;
        int dy = static_cast<int>(std::fmod(ph * 90.0f + i * 80.0f, static_cast<float>(h - dh + 40))) - 20;

        RECT rc{ dx, dy, dx + dw, dy + dh };
        HBRUSH bg = CreateSolidBrush(RGB(236, 233, 216));
        FillRect(dst, &rc, bg);
        DeleteObject(bg);

        DrawEdge(dst, &rc, EDGE_RAISED, BF_RECT);

        RECT tbRc{ dx + 3, dy + 3, dx + dw - 3, dy + 25 };
        HBRUSH tbBrush = CreateSolidBrush(RGB(10, 36, 106));
        FillRect(dst, &tbRc, tbBrush);
        DeleteObject(tbBrush);

        SetTextColor(dst, RGB(255, 255, 255));
        RECT titleTextRc{ dx + 8, dy + 4, dx + dw - 8, dy + 24 };
        DrawTextW(dst, L"System Alert", -1, &titleTextRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        HICON errIcon = LoadIconW(nullptr, IDI_ERROR);
        if (errIcon) {
            DrawIconEx(dst, dx + 16, dy + 40, errIcon, 32, 32, 0, nullptr, DI_NORMAL);
        }

        SetTextColor(dst, RGB(0, 0, 0));
        RECT msgRc{ dx + 58, dy + 38, dx + dw - 12, dy + 85 };
        DrawTextW(dst, L"Acid.exe visual overload.\nSystem graphics distorted.", -1, &msgRc, DT_LEFT | DT_WORDBREAK);

        RECT btnRc{ dx + dw / 2 - 35, dy + dh - 32, dx + dw / 2 + 35, dy + dh - 10 };
        DrawFrameControl(dst, &btnRc, DFC_BUTTON, DFCS_BUTTONPUSH);
        DrawTextW(dst, L"OK", -1, &btnRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(dst, oldFont);
    DeleteObject(font);

    if (g_intensity > 0.35f) {
        int off = static_cast<int>(std::sin(t * 4.0f) * 14.0f);
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunGridMirrorEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    const int grid = 2 + static_cast<int>(g_intensity*4.0f);
    const int tw=w/grid, th=h/grid;
    const float spd=0.8f+g_intensity*1.4f;
    const int mirrorToggle = static_cast<int>(t*spd) % 4;
    for (int gy=0;gy<grid;++gy) {
        for (int gx=0;gx<grid;++gx) {
            const int dx=gx*tw, dy=gy*th;
            const bool flipX=(gx+gy+mirrorToggle)%2==0;
            const bool flipY=(gx-gy+mirrorToggle)%3==0;
            const int sx=flipX?w-tw-dx:dx, sy=flipY?h-th-dy:dy;
            const int asx=std::clamp(sx,0,w-tw), asy=std::clamp(sy,0,h-th);
            BitBlt(dst,dx,dy,tw,th,src,asx,asy,SRCCOPY);
        }
    }
    if (g_intensity>0.2f) {
        const ColorRgb hc=HueCycleTime(t);
        HBRUSH hb=CreateHatchBrush(HS_CROSS,RGB(hc.r,hc.g,hc.b));
        HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,hb));
        PatBlt(dst,0,0,w,h,PATINVERT);
        SelectObject(dst,ob); DeleteObject(hb);
    }
    b.swap();
}

void RunPixelateEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    EnsurePixels(src,w,h);
    const float phase=std::sin(t*(0.8f+g_intensity))*0.5f+0.5f;
    const int blockW=std::max(2,static_cast<int>((4+g_intensity*60)*phase+2));
    const int blockH=std::max(2,static_cast<int>((3+g_intensity*40)*phase+2));
    const int pw=std::max(1,w/blockW), ph2=std::max(1,h/blockH);
    SetStretchBltMode(dst,COLORONCOLOR);
    StretchBlt(g_fractalLayer.dc(),0,0,pw,ph2,src,0,0,w,h,SRCCOPY);
    StretchBlt(dst,0,0,w,h,g_fractalLayer.dc(),0,0,pw,ph2,SRCCOPY);
    if (g_intensity>0.3f) {
        const int sh=static_cast<int>(g_intensity*10*std::sin(t*2.2f));
        BitBlt(dst,sh,0,w,h,src,0,0,SRCINVERT);
    }
    b.swap();
}

void RunStrobeMirrorEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    CopyBaseFrame(b);
    const float strobeRate=1.5f+g_intensity*8.0f;
    const bool strobe=static_cast<int>(t*strobeRate*2)%2==0;
    SetStretchBltMode(dst,COLORONCOLOR);
    StretchBlt(dst,w/2,0,w/2,h,src,w/2,0,-w/2,h,SRCCOPY);
    if (g_intensity>0.3f)
        StretchBlt(dst,0,h/2,w,h/2,dst,0,h/2,w,-h/2,SRCCOPY);
    if (strobe&&g_intensity>0.1f)
        PatBlt(dst,0,0,w,h,DSTINVERT);
    const int sh=static_cast<int>(g_intensity*16*std::sin(t*3.5f));
    if (sh) BitBlt(dst,sh,0,w,h,src,0,0,SRCINVERT);
    b.swap();
}

void RunRGBSplitEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(), dst=b.backDC();
    const int w=b.width, h=b.height;
    EnsurePixels(src,w,h);
    g_pixels.copyFrom(src,0,0);
    const float spd=2.0f+g_intensity*2.0f;
    const int split=static_cast<int>(8.0f+g_intensity*44.0f);
    ApplyChannelSplit(g_pixels,
                      static_cast<int>(split*std::sin(t*spd)),
                      static_cast<int>(-split*std::cos(t*spd*0.8f)));
    g_pixels.copyTo(dst,0,0);
    const int sh=static_cast<int>(g_intensity*20*std::sin(t*1.7f));
    BitBlt(dst,sh,0,w-std::abs(sh),h,src,0,0,SRCINVERT);
    ApplySineRipple(dst,w,h,t*(1.0f+g_intensity));
    b.swap();
}

void RunRadialRipplesEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    const int cx = w / 2, cy = h / 2;
    const float maxR = std::sqrt(static_cast<float>(w * w + h * h)) * 0.55f;
    const int numRings = 5 + static_cast<int>(g_intensity * 12.0f);
    const float speed = 120.0f + g_intensity * 250.0f;
    
    HBRUSH nullBrush = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dst, nullBrush));
    
    for (int i = 0; i < numRings; ++i) {
        float r = std::fmod(t * speed + i * (maxR / numRings), maxR);
        int ir = static_cast<int>(r);
        int penW = 3 + static_cast<int>(g_intensity * 8.0f);
        ColorRgb col = HslToRgb(t * 70.0f + i * 40.0f, 0.95f, 0.55f);
        HPEN pen = CreatePen(PS_SOLID, penW, RGB(col.r, col.g, col.b));
        HPEN oldPen = static_cast<HPEN>(SelectObject(dst, pen));
        
        Ellipse(dst, cx - ir, cy - ir, cx + ir, cy + ir);
        
        SelectObject(dst, oldPen);
        DeleteObject(pen);
        
        if (i % 2 == 0) {
            PatBlt(dst, cx - ir, cy - 2, ir * 2, 4, DSTINVERT);
        }
    }
    SelectObject(dst, oldBrush);
    
    if (g_intensity > 0.35f) {
        int off = static_cast<int>(std::sin(t * 3.0f) * 16.0f);
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

// ===========================================================================
// TIER 2 - Advanced GDI Payloads, Slicing & Radar (30%)
// ===========================================================================

void RunSliceTearEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    const int sliceH = std::max(4, 20 - static_cast<int>(g_intensity * 12.0f));
    const float spd = 2.0f + g_intensity * 4.0f;
    const float amp = 15.0f + g_intensity * 75.0f;
    
    for (int y = 0; y < h; y += sliceH) {
        int idx = y / sliceH;
        int dir = (idx % 2 == 0) ? 1 : -1;
        int off = static_cast<int>(std::sin(t * spd + idx * 0.4f) * amp * dir);
        int sh = std::min(sliceH, h - y);
        int sx = std::clamp(off, -(w - 1), w - 1);
        BitBlt(dst, std::max(0, sx), y, w - std::abs(sx), sh,
               src, std::max(0, -sx), y, SRCCOPY);
    }
    
    if (g_intensity > 0.35f) {
        const int sliceW = std::max(8, 32 - static_cast<int>(g_intensity * 16.0f));
        for (int x = 0; x < w; x += sliceW) {
            int idx = x / sliceW;
            int dir = (idx % 2 == 0) ? 1 : -1;
            int off = static_cast<int>(std::cos(t * spd * 0.8f + idx * 0.5f) * (amp * 0.5f) * dir);
            int sw = std::min(sliceW, w - x);
            int sy = std::clamp(off, -(h - 1), h - 1);
            BitBlt(dst, x, std::max(0, sy), sw, h - std::abs(sy),
                   dst, x, std::max(0, -sy), SRCCOPY);
        }
    }
    b.swap();
}

void RunDiagonalShearEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    const int bandH = std::max(8, 36 - static_cast<int>(g_intensity * 20.0f));
    const float spd = 1.5f + g_intensity * 3.5f;
    const float amp = 20.0f + g_intensity * 80.0f;
    for (int y = 0; y < h; y += bandH) {
        int idx = y / bandH;
        float ph = t * spd + idx * 0.45f;
        int off = static_cast<int>(std::sin(ph) * amp);
        int curH = std::min(bandH, h - y);
        POINT pts[3];
        pts[0] = { off, y };
        pts[1] = { off + w, y };
        pts[2] = { -off, y + curH };
        PlgBlt(dst, pts, src, 0, y, w, curH, nullptr, 0, 0);
    }
    if (g_intensity > 0.3f) {
        BitBlt(dst, 0, 0, w, h, src, 0, 0, SRCPAINT);
    }
    b.swap();
}

void RunPieSliceInvertEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    const int cx = w / 2, cy = h / 2;
    const int r = std::max(w, h);
    const int slices = 4 + static_cast<int>(g_intensity * 8.0f);
    const float angleOffset = t * (1.2f + g_intensity * 2.4f);
    const float sliceAngle = (2.0f * kPi) / slices;

    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dst, brush));
    HPEN oldPen = static_cast<HPEN>(SelectObject(dst, GetStockObject(NULL_PEN)));

    for (int i = 0; i < slices; i += 2) {
        float a1 = angleOffset + i * sliceAngle;
        float a2 = a1 + sliceAngle * 0.75f;
        int x1 = cx + static_cast<int>(std::cos(a1) * r);
        int y1 = cy + static_cast<int>(std::sin(a1) * r);
        int x2 = cx + static_cast<int>(std::cos(a2) * r);
        int y2 = cy + static_cast<int>(std::sin(a2) * r);

        Pie(dst, cx - r, cy - r, cx + r, cy + r, x1, y1, x2, y2);
        PatBlt(dst, 0, 0, w, h, 0x005A0049);
    }

    SelectObject(dst, oldPen);
    SelectObject(dst, oldBrush);
    DeleteObject(brush);

    if (g_intensity > 0.35f) {
        int off = static_cast<int>(std::sin(t * 3.0f) * 16.0f);
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

static float g_lx = 100.0f, g_ly = 100.0f, g_vx = 320.0f, g_vy = 240.0f;
static bool g_initLogo = false;

void RunLogoBounceEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);

    if (t < 0.05f || !g_initLogo) {
        g_lx = w * 0.3f; g_ly = h * 0.3f;
        g_vx = 280.0f + g_intensity * 150.0f;
        g_vy = 210.0f + g_intensity * 120.0f;
        g_initLogo = true;
    }

    const int bw = 160, bh = 80;
    constexpr float dt = 0.016f;
    g_lx += g_vx * dt; g_ly += g_vy * dt;

    if (g_lx < 0) { g_lx = 0; g_vx = std::abs(g_vx); PatBlt(dst, 0, 0, w, h, DSTINVERT); }
    if (g_lx + bw > w) { g_lx = static_cast<float>(w - bw); g_vx = -std::abs(g_vx); PatBlt(dst, 0, 0, w, h, DSTINVERT); }
    if (g_ly < 0) { g_ly = 0; g_vy = std::abs(g_vy); PatBlt(dst, 0, 0, w, h, DSTINVERT); }
    if (g_ly + bh > h) { g_ly = static_cast<float>(h - bh); g_vy = -std::abs(g_vy); PatBlt(dst, 0, 0, w, h, DSTINVERT); }

    int ilx = static_cast<int>(g_lx), ily = static_cast<int>(g_ly);
    ColorRgb col = HslToRgb(t * 90.0f, 0.95f, 0.55f);
    HBRUSH lgb = CreateSolidBrush(RGB(col.r, col.g, col.b));
    RECT lrc{ ilx, ily, ilx + bw, ily + bh };
    FillRect(dst, &lrc, lgb);
    DeleteObject(lgb);
    DrawEdge(dst, &lrc, EDGE_RAISED, BF_RECT);

    SetBkMode(dst, TRANSPARENT);
    SetTextColor(dst, RGB(0, 0, 0));
    HFONT font = CreateFontW(26, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial Black");
    HFONT oldFont = static_cast<HFONT>(SelectObject(dst, font));
    DrawTextW(dst, L"ACID", -1, &lrc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dst, oldFont);
    DeleteObject(font);

    if (g_intensity > 0.3f) {
        BitBlt(dst, ilx - 10, ily - 10, bw + 20, bh + 20, src, ilx - 10, ily - 10, SRCINVERT);
    }
    b.swap();
}

void RunHatchStormEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    
    static const int kHatchStyles[] = {
        HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS, HS_FDIAGONAL, HS_HORIZONTAL, HS_VERTICAL
    };
    
    const int numBoxes = 4 + static_cast<int>(g_intensity * 12.0f);
    for (int i = 0; i < numBoxes; ++i) {
        int hs = kHatchStyles[(i + static_cast<int>(t)) % 6];
        ColorRgb col = HslToRgb(t * 80.0f + i * 45.0f, 0.95f, 0.55f);
        HBRUSH hb = CreateHatchBrush(hs, RGB(col.r, col.g, col.b));
        HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dst, hb));
        
        float ph = t * 1.5f + i * 1.1f;
        int bw = w / 3 + static_cast<int>(std::sin(ph) * (w / 6));
        int bh = h / 3 + static_cast<int>(std::cos(ph * 0.9f) * (h / 6));
        int bx = static_cast<int>((std::sin(ph * 0.7f) * 0.5f + 0.5f) * (w - bw));
        int by = static_cast<int>((std::cos(ph * 0.6f) * 0.5f + 0.5f) * (h - bh));
        
        PatBlt(dst, bx, by, bw, bh, PATINVERT);
        
        SelectObject(dst, oldBrush);
        DeleteObject(hb);
    }
    
    if (g_intensity > 0.3f) {
        int off = static_cast<int>(std::sin(t * 5.0f) * 14.0f);
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCPAINT);
    }
    b.swap();
}

void RunPlgRotateEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);
    
    float angle = std::sin(t * 1.4f) * (0.15f + g_intensity * 0.35f);
    float fcx = w * 0.5f, fcy = h * 0.5f;
    float cs = std::cos(angle), sn = std::sin(angle);
    
    float zoom = 1.0f + std::abs(std::sin(angle)) * 0.6f;
    float hx = (w * 0.5f) / zoom;
    float hy = (h * 0.5f) / zoom;
    
    POINT pts[3];
    pts[0].x = static_cast<LONG>(fcx + (-hx) * cs - (-hy) * sn);
    pts[0].y = static_cast<LONG>(fcy + (-hx) * sn + (-hy) * cs);
    pts[1].x = static_cast<LONG>(fcx + (hx) * cs - (-hy) * sn);
    pts[1].y = static_cast<LONG>(fcy + (hx) * sn + (-hy) * cs);
    pts[2].x = static_cast<LONG>(fcx + (-hx) * cs - (hy) * sn);
    pts[2].y = static_cast<LONG>(fcy + (-hx) * sn + (hy) * cs);
    
    PlgBlt(dst, pts, src, 0, 0, w, h, nullptr, 0, 0);
    
    if (g_intensity > 0.3f) {
        BitBlt(dst, 0, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunPlasmaEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    EnsurePixels(src,w,h);
    RenderPlasmaLayer(g_fractalLayer,t*(1.0f+g_intensity*2.2f));
    SetStretchBltMode(dst,HALFTONE);
    StretchBlt(dst,0,0,w,h,g_fractalLayer.dc(),0,0,g_fractalLayer.width,g_fractalLayer.height,SRCCOPY);
    const int sh=static_cast<int>((3.0f+g_intensity*8)*std::sin(t*2.2f));
    BitBlt(dst,sh,0,w,h,src,0,0,SRCINVERT); b.swap();
}

void RunInterferenceEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    EnsurePixels(src,w,h);
    RenderInterferenceLayer(g_fractalLayer,t*(1.0f+g_intensity*2.0f));
    SetStretchBltMode(dst,HALFTONE);
    StretchBlt(dst,0,0,w,h,g_fractalLayer.dc(),0,0,g_fractalLayer.width,g_fractalLayer.height,SRCCOPY);
    if (g_intensity>0.3f) { int sh=static_cast<int>((g_intensity-0.3f)*20*std::sin(t*1.5f)); BitBlt(dst,sh,0,w,h,src,0,0,SRCINVERT); }
    b.swap();
}

void RunMatrixSpamEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);

    SetBkMode(dst, TRANSPARENT);
    HFONT font = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
    HFONT oldFont = static_cast<HFONT>(SelectObject(dst, font));

    const int colSpacing = 20;
    const int numCols = w / colSpacing;
    const float speed = 180.0f + g_intensity * 300.0f;

    static const wchar_t kChars[] = L"0123456789ABCDEF!@#$%&*<>{}[]=~+";
    constexpr int kNumChars = sizeof(kChars) / sizeof(kChars[0]) - 1;

    for (int col = 0; col < numCols; ++col) {
        float colSeed = static_cast<float>((col * 9301 + 49297) % 233280) / 233280.0f;
        float yHead = std::fmod(t * speed * (0.7f + colSeed * 0.6f), static_cast<float>(h + 300)) - 100;
        int iyHead = static_cast<int>(yHead);
        int x = col * colSpacing + 2;

        int len = 12 + static_cast<int>(colSeed * 10.0f);
        for (int row = 0; row < len; ++row) {
            int y = iyHead - row * 16;
            if (y >= 0 && y < h) {
                wchar_t ch = kChars[(col * 7 + row * 13 + static_cast<int>(t * 15.0f)) % kNumChars];
                if (row == 0) {
                    SetTextColor(dst, RGB(255, 255, 255));
                } else {
                    float fade = 1.0f - (static_cast<float>(row) / len);
                    ColorRgb c = HslToRgb(t * 50.0f + col * 12.0f, 0.95f, fade * 0.55f);
                    SetTextColor(dst, RGB(c.r, c.g, c.b));
                }
                TextOutW(dst, x, y, &ch, 1);
            }
        }
    }

    SelectObject(dst, oldFont);
    DeleteObject(font);

    if (g_intensity > 0.35f) {
        int off = static_cast<int>(std::sin(t * 4.0f) * 12.0f);
        BitBlt(dst, off, 0, w, h, dst, 0, 0, SRCINVERT);
    }
    b.swap();
}

// ===========================================================================
// TIER 3 - Pure Fractal Universe (45%)
// ===========================================================================

void RunMandelbrotEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    EnsurePixels(src, w, h);
    float zoom = 1.2f + 0.8f * std::sin(t * 0.25f);
    RenderMandelbrotOverlay(g_fractalLayer, t, zoom);
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, 0, 0, w, h, g_fractalLayer.dc(), 0, 0, g_fractalLayer.width, g_fractalLayer.height, SRCCOPY);
    ApplySineRipple(dst, w, h, t * (1.0f + g_intensity * 0.8f));
    if (g_intensity > 0.35f) {
        int sh = static_cast<int>(g_intensity * 14.0f * std::sin(t * 2.0f));
        BitBlt(dst, sh, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunJuliaFractalEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    EnsurePixels(src, w, h);
    RenderJuliaFractal(g_fractalLayer, t, 0.85f);
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, 0, 0, w, h, g_fractalLayer.dc(), 0, 0, g_fractalLayer.width, g_fractalLayer.height, SRCCOPY);
    if (g_intensity > 0.3f) {
        int off = static_cast<int>((4.0f + g_intensity * 12.0f) * std::sin(t * 2.2f));
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunNewtonFractalEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    EnsurePixels(src, w, h);
    float zoom = 1.0f + 0.5f * std::sin(t * 0.3f);
    RenderNewtonFractal(g_fractalLayer, t, zoom);
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, 0, 0, w, h, g_fractalLayer.dc(), 0, 0, g_fractalLayer.width, g_fractalLayer.height, SRCCOPY);
    if (g_intensity > 0.3f) {
        int off = static_cast<int>((4.0f + g_intensity * 10.0f) * std::sin(t * 2.5f));
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunLyapunovEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    EnsurePixels(src, w, h);
    RenderLyapunovFractal(g_fractalLayer, t);
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, 0, 0, w, h, g_fractalLayer.dc(), 0, 0, g_fractalLayer.width, g_fractalLayer.height, SRCCOPY);
    ApplySineRipple(dst, w, h, t * (1.2f + g_intensity));
    if (g_intensity > 0.3f) {
        int sh = static_cast<int>(g_intensity * 12.0f * std::sin(t * 2.8f));
        BitBlt(dst, sh, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunBarnsleyFernEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    EnsurePixels(src, w, h);
    RenderBarnsleyLayer(g_fractalLayer, t);
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, 0, 0, w, h, g_fractalLayer.dc(), 0, 0, g_fractalLayer.width, g_fractalLayer.height, SRCCOPY);
    if (g_intensity > 0.35f) {
        BitBlt(dst, 0, 0, w, h, src, 0, 0, SRCPAINT);
    }
    b.swap();
}

void RunApollonianEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    EnsurePixels(src, w, h);
    RenderApollonianLayer(g_fractalLayer, t);
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, 0, 0, w, h, g_fractalLayer.dc(), 0, 0, g_fractalLayer.width, g_fractalLayer.height, SRCCOPY);
    if (g_intensity > 0.3f) {
        int off = static_cast<int>((5.0f + g_intensity * 12.0f) * std::sin(t * 3.0f));
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunDragonCurveEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);

    HBRUSH bg = CreateSolidBrush(RGB(5, 5, 20));
    RECT rc{ 0, 0, w, h };
    FillRect(dst, &rc, bg);
    DeleteObject(bg);

    constexpr int kMaxSteps = 4096;
    static int turns[kMaxSteps];
    static bool turnsInit = false;
    if (!turnsInit) {
        turns[0] = 1;
        int len = 1;
        for (int iter = 0; iter < 11; ++iter) {
            turns[len] = 1;
            for (int k = 0; k < len; ++k) {
                turns[len + 1 + k] = -turns[len - 1 - k];
            }
            len = len * 2 + 1;
        }
        turnsInit = true;
    }

    float rot = t * 0.6f;
    float stepLen = (std::min(w, h) * 0.5f) / 45.0f;
    float curX = w * 0.42f, curY = h * 0.58f;
    float dirAngle = rot;

    int numSteps = std::min(kMaxSteps, 800 + static_cast<int>(g_intensity * 3000.0f));

    for (int i = 0; i < numSteps; ++i) {
        float nextX = curX + std::cos(dirAngle) * stepLen;
        float nextY = curY + std::sin(dirAngle) * stepLen;

        ColorRgb c = HslToRgb(t * 60.0f + i * 0.4f, 0.95f, 0.6f);
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(c.r, c.g, c.b));
        HPEN oldPen = static_cast<HPEN>(SelectObject(dst, pen));

        MoveToEx(dst, static_cast<int>(curX), static_cast<int>(curY), nullptr);
        LineTo(dst, static_cast<int>(nextX), static_cast<int>(nextY));

        SelectObject(dst, oldPen);
        DeleteObject(pen);

        curX = nextX;
        curY = nextY;
        dirAngle += turns[i] * (kPi * 0.5f);
    }

    if (g_intensity > 0.35f) {
        int off = static_cast<int>(std::sin(t * 3.5f) * 12.0f);
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunKochSnowflakeEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);

    const int cx = w / 2, cy = h / 2;
    const float maxR = std::min(w, h) * (0.35f + 0.08f * std::sin(t * 1.5f));
    const float rot = t * 0.5f;

    struct KochHelper {
        static void DrawSegment(HDC dc, float x1, float y1, float x2, float y2, int depth, float t) {
            if (depth == 0) {
                MoveToEx(dc, static_cast<int>(x1), static_cast<int>(y1), nullptr);
                LineTo(dc, static_cast<int>(x2), static_cast<int>(y2));
                return;
            }
            float dx = x2 - x1, dy = y2 - y1;
            float p1x = x1 + dx / 3.0f, p1y = y1 + dy / 3.0f;
            float p3x = x1 + dx * 2.0f / 3.0f, p3y = y1 + dy * 2.0f / 3.0f;
            float cos60 = 0.5f, sin60 = 0.8660254f;
            float p2x = p1x + (dx / 3.0f) * cos60 - (dy / 3.0f) * sin60;
            float p2y = p1y + (dx / 3.0f) * sin60 + (dy / 3.0f) * cos60;

            DrawSegment(dc, x1, y1, p1x, p1y, depth - 1, t);
            DrawSegment(dc, p1x, p1y, p2x, p2y, depth - 1, t);
            DrawSegment(dc, p2x, p2y, p3x, p3y, depth - 1, t);
            DrawSegment(dc, p3x, p3y, x2, y2, depth - 1, t);
        }
    };

    int depth = 3 + static_cast<int>(g_intensity);
    ColorRgb c = HslToRgb(t * 70.0f, 0.95f, 0.6f);
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(c.r, c.g, c.b));
    HPEN oldPen = static_cast<HPEN>(SelectObject(dst, pen));

    POINT p[3];
    for (int i = 0; i < 3; ++i) {
        float a = rot + i * (2.0f * kPi / 3.0f);
        p[i].x = cx + static_cast<int>(std::cos(a) * maxR);
        p[i].y = cy + static_cast<int>(std::sin(a) * maxR);
    }

    KochHelper::DrawSegment(dst, static_cast<float>(p[0].x), static_cast<float>(p[0].y), static_cast<float>(p[1].x), static_cast<float>(p[1].y), depth, t);
    KochHelper::DrawSegment(dst, static_cast<float>(p[1].x), static_cast<float>(p[1].y), static_cast<float>(p[2].x), static_cast<float>(p[2].y), depth, t);
    KochHelper::DrawSegment(dst, static_cast<float>(p[2].x), static_cast<float>(p[2].y), static_cast<float>(p[0].x), static_cast<float>(p[0].y), depth, t);

    SelectObject(dst, oldPen);
    DeleteObject(pen);

    PatBlt(dst, cx - static_cast<int>(maxR * 0.4f), cy - static_cast<int>(maxR * 0.4f),
           static_cast<int>(maxR * 0.8f), static_cast<int>(maxR * 0.8f), DSTINVERT);

    b.swap();
}

void RunSierpinskiEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    EnsurePixels(src,w,h); StretchToFractalLayer(src,w,h);
    RenderSierpinskiLayer(g_fractalLayer,t*(1.0f+g_intensity));
    SetStretchBltMode(dst,HALFTONE); StretchBlt(dst,0,0,w,h,g_fractalLayer.dc(),0,0,g_fractalLayer.width,g_fractalLayer.height,SRCCOPY);
    ApplySineRipple(dst,w,h,t*(1.0f+g_intensity*0.8f));
    if (g_intensity>0.25f){HBRUSH h1=CreateHatchBrush(HS_CROSS,RGB(0,220,255));HBRUSH o1=static_cast<HBRUSH>(SelectObject(dst,h1));PatBlt(dst,0,0,w,h,PATINVERT);SelectObject(dst,o1);DeleteObject(h1);}
    b.swap();
}

void RunBurningShipEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    EnsurePixels(src,w,h);
    RenderBurningShipLayer(g_fractalLayer,t*(1.0f+g_intensity*1.4f));
    SetStretchBltMode(dst,HALFTONE); StretchBlt(dst,0,0,w,h,g_fractalLayer.dc(),0,0,g_fractalLayer.width,g_fractalLayer.height,SRCCOPY);
    ApplySineRipple(dst,w,h,t*(1.0f+g_intensity));
    ColorRgb hc=HueCycleTime(t); HBRUSH ht=CreateHatchBrush(HS_BDIAGONAL,RGB(hc.r,hc.g,hc.b)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,ht));
    if (g_intensity>0.3f) PatBlt(dst,0,0,w,h,PATINVERT);
    SelectObject(dst,ob); DeleteObject(ht); b.swap();
}

// ===========================================================================
// TIER 4 - Geometric & Dynamic Trips (60%)
// ===========================================================================

void RunMandalaEffect(ScreenBuffer& b, float t) {
    const HDC dst=b.backDC(); const int w=b.width,h=b.height;
    const float spd=1.0f+g_intensity*1.8f, T=t*spd;
    HBRUSH bg=CreateSolidBrush(RGB(2,0,8)); RECT rc{0,0,w,h}; FillRect(dst,&rc,bg); DeleteObject(bg);
    const int cx=w/2,cy=h/2; const float maxR=std::min(w,h)*0.44f;
    const int layers=4+static_cast<int>(g_intensity*5);
    HBRUSH nb=static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,nb));
    for (int L=0;L<layers;++L) {
        float r=maxR*(L+1.0f)/layers;
        float rot=T*(L%2==0?0.22f:-0.16f)*(1+L*0.12f)+L*kPi/6;
        ColorRgb c=HslToRgb(T*28.0f+L*360.0f/layers,0.95f,std::clamp(0.4f+0.15f*std::sin(T*0.5f+L),0.2f,0.7f));
        int pw=std::max(1,(int)(1+L*0.5f));
        HPEN pn=CreatePen(PS_SOLID,pw,RGB(c.r,c.g,c.b));
        HPEN op=static_cast<HPEN>(SelectObject(dst,pn));
        POINT pts[6]; for (int i=0;i<6;++i){float a=rot+i*2.0f*kPi/6;pts[i].x=cx+(int)(r*std::cos(a));pts[i].y=cy+(int)(r*std::sin(a));}
        Polygon(dst,pts,6); int ir=(int)(r*0.62f); Ellipse(dst,cx-ir,cy-ir,cx+ir,cy+ir);
        SelectObject(dst,op); DeleteObject(pn);
    }
    float petalR=maxR*0.30f,petalSz=maxR*0.34f;
    for (int i=0;i<6;++i){float a=T*0.18f+i*2.0f*kPi/6;int px2=cx+(int)(petalR*std::cos(a)),py2=cy+(int)(petalR*std::sin(a));ColorRgb c=HslToRgb(T*38+i*60,0.9f,0.5f);HPEN pn=CreatePen(PS_SOLID,1,RGB(c.r,c.g,c.b));HPEN op=static_cast<HPEN>(SelectObject(dst,pn));int ps=(int)petalSz;Ellipse(dst,px2-ps,py2-ps,px2+ps,py2+ps);SelectObject(dst,op);DeleteObject(pn);}
    ColorRgb cc=HueCycle(T*75); HPEN cp=CreatePen(PS_SOLID,2,RGB(cc.r,cc.g,cc.b)); HPEN op=static_cast<HPEN>(SelectObject(dst,cp)); int hr=(int)(maxR*0.11f); Ellipse(dst,cx-hr,cy-hr,cx+hr,cy+hr); SelectObject(dst,op); DeleteObject(cp);
    SelectObject(dst,ob); b.swap();
}

void RunWarpTunnelEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    const float T=t*(1.5f+g_intensity*4.0f);
    if (g_intensity<0.45f){HBRUSH bg=CreateSolidBrush(RGB(0,0,5));RECT rc{0,0,w,h};FillRect(dst,&rc,bg);DeleteObject(bg);}
    else CopyBaseFrame(b);
    const int cx=w/2,cy=h/2,rings=28+static_cast<int>(g_intensity*32);
    HBRUSH nb=static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,nb));
    float phase=std::fmod(T,1.0f);
    for (int i=0;i<rings;++i) {
        float r=std::fmod((float)i/rings+phase,1.0f);
        float rx=r*w*0.54f+2,ry2=r*h*0.54f+2;
        float wb=std::sin(T*0.28f+r*kPi*2)*12;
        ColorRgb c=HslToRgb((1-r)*280+T*38,0.96f,0.28f+r*0.3f);
        int pw=std::max(1,(int)((1-r)*3.5f));
        HPEN pn=CreatePen(PS_SOLID,pw,RGB(c.r,c.g,c.b)); HPEN op=static_cast<HPEN>(SelectObject(dst,pn));
        int ox=(int)wb,oy=(int)(wb*0.55f);
        Ellipse(dst,cx+ox-(int)rx,cy+oy-(int)ry2,cx+ox+(int)rx,cy+oy+(int)ry2);
        SelectObject(dst,op); DeleteObject(pn);
    }
    SelectObject(dst,ob); b.swap();
}

void RunZoomFeedbackEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b);
    float zf=1.025f+0.015f*g_intensity+0.010f*std::sin(t*0.7f); int zw=(int)(w*zf),zh=(int)(h*zf);
    SetStretchBltMode(dst,HALFTONE); StretchBlt(dst,(w-zw)/2,(h-zh)/2,zw,zh,src,0,0,w,h,SRCINVERT);
    int sh=(int)((3.0f+g_intensity*6)*std::sin(t*3.2f)); BitBlt(dst,sh,0,w,h,dst,0,0,SRCINVERT); BitBlt(dst,0,sh,w,h,dst,0,0,SRCPAINT);
    int rings=4+static_cast<int>(g_intensity*5);
    HBRUSH nb=static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,nb));
    for (int i=1;i<=rings;++i){float ph=t*(1.0f+g_intensity*0.6f)+i*0.55f;int r=(int)((0.07f+0.13f*i)*std::min(w,h)*(1.0f+0.08f*std::sin(ph)));ColorRgb rc=HslToRgb(t*60.0f+i*55,0.95f,0.55f);HPEN pn=CreatePen(PS_SOLID,2,RGB(rc.r,rc.g,rc.b));HPEN op=static_cast<HPEN>(SelectObject(dst,pn));Ellipse(dst,w/2-r,h/2-r,w/2+r,h/2+r);SelectObject(dst,op);DeleteObject(pn);}
    SelectObject(dst,ob); b.swap();
}

void RunHelixEffect(ScreenBuffer& b, float t) {
    const HDC dst=b.backDC(); const int w=b.width,h=b.height;
    const float T=t*(1.4f+g_intensity*2.2f);
    HBRUSH bg=CreateSolidBrush(RGB(0,0,12));RECT rc{0,0,w,h};FillRect(dst,&rc,bg);DeleteObject(bg);
    float helixW=w*0.38f,helixH=h*0.44f; int cx=w/2; float baseY=h*0.5f;
    int N=55+static_cast<int>(g_intensity*45);
    HPEN np=static_cast<HPEN>(GetStockObject(NULL_PEN));
    for (int helix=0;helix<2;++helix) {
        float phOff=helix*kPi;
        for (int i=0;i<N-1;++i){float f1=(float)i/N,f2=(float)(i+1)/N;float a1=f1*4*kPi+T+phOff,a2=f2*4*kPi+T+phOff;int x1=cx+(int)(helixW*std::cos(a1)),y1=(int)(baseY+helixH*(f1-0.5f)),x2=cx+(int)(helixW*std::cos(a2)),y2=(int)(baseY+helixH*(f2-0.5f));ColorRgb lc=HslToRgb(f1*270+T*32+helix*180,0.8f,0.45f);HPEN lp=CreatePen(PS_SOLID,1,RGB(lc.r,lc.g,lc.b));HPEN op=static_cast<HPEN>(SelectObject(dst,lp));MoveToEx(dst,x1,y1,nullptr);LineTo(dst,x2,y2);SelectObject(dst,op);DeleteObject(lp);}
        for (int i=N-1;i>=0;--i){float frac=(float)i/N;float a=frac*4*kPi+T+phOff;float cosA=std::cos(a);int x=cx+(int)(helixW*cosA),y=(int)(baseY+helixH*(frac-0.5f));float z=(cosA+1)*0.5f;int r=std::max(2,(int)(z*11+3));ColorRgb c=HslToRgb(frac*270+T*32+helix*180,0.96f,0.32f+z*0.36f);HBRUSH fb=CreateSolidBrush(RGB(c.r,c.g,c.b));HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,fb));HPEN op=static_cast<HPEN>(SelectObject(dst,np));Ellipse(dst,x-r,y-(int)(r*0.52f+1),x+r,y+(int)(r*0.52f+1));SelectObject(dst,ob);SelectObject(dst,op);DeleteObject(fb);}
    }
    int rungs=18+static_cast<int>(g_intensity*14);
    for (int i=0;i<rungs;++i){float frac=(float)i/rungs;float a=frac*4*kPi+T;int x1=cx+(int)(helixW*std::cos(a)),x2=cx+(int)(helixW*std::cos(a+kPi)),y=(int)(baseY+helixH*(frac-0.5f));ColorRgb rc=HslToRgb(frac*270+T*32,0.7f,0.62f);HPEN rp=CreatePen(PS_SOLID,1,RGB(rc.r,rc.g,rc.b));HPEN op=static_cast<HPEN>(SelectObject(dst,rp));MoveToEx(dst,x1,y,nullptr);LineTo(dst,x2,y);SelectObject(dst,op);DeleteObject(rp);}
    b.swap();
}

void RunMushroomEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    float pulse=0.90f+0.10f*std::sin(t*1.4f); int nw=(int)(w*pulse),nh=(int)(h*pulse);
    SetStretchBltMode(dst,HALFTONE); StretchBlt(dst,(w-nw)/2,(h-nh)/2,nw,nh,src,0,0,w,h,SRCCOPY);
    int gs=(int)((5+g_intensity*8)*std::sin(t*0.9f));
    BitBlt(dst,gs,gs,w-gs,h-gs,src,0,0,SRCPAINT); BitBlt(dst,-gs,gs/2,w,h,src,gs,0,SRCAND);
    HBRUSH rb=CreateSolidBrush(RGB(255,180,80)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,rb));
    int ringCount=5+static_cast<int>(g_intensity*5);
    for (int i=0;i<ringCount;++i){int r=(int)((80+i*75)*(1.0f+0.18f*std::sin(t+i)));PatBlt(dst,w/2-r,h/2-r,r*2,r*2,PATINVERT);}
    SelectObject(dst,ob); DeleteObject(rb);
    POINT pts[3]={{(LONG)(80.0*std::sin(t)),0},{w+(int)(80.0*std::cos(t*0.7f)),0},{0,h}};
    PlgBlt(dst,pts,src,0,0,w,h,nullptr,0,0);
    int wx=WaveOff(t,2.5f,22.0f,2),wy=WaveOff(t,1.8f,18.0f,3);
    BitBlt(dst,wx,wy,w-std::abs(wx),h-std::abs(wy),src,0,0,MERGEPAINT);
    b.swap();
}

void RunVortexSpinEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    float angle=t*(1.4f+g_intensity*2.0f);
    float fcx=w*0.5f,fcy=h*0.5f,cs=std::cos(angle),sn=std::sin(angle);
    POINT pts[3]={{(LONG)(fcx+(-fcx)*cs-(-fcy)*sn),(LONG)(fcy+(-fcx)*sn+(-fcy)*cs)},{(LONG)(fcx+(fcx)*cs-(-fcy)*sn),(LONG)(fcy+(fcx)*sn+(-fcy)*cs)},{(LONG)(fcx+(-fcx)*cs-(fcy)*sn),(LONG)(fcy+(-fcx)*sn+(fcy)*cs)}};
    PlgBlt(dst,pts,src,0,0,w,h,nullptr,0,0); BitBlt(dst,0,0,w,h,src,0,0,SRCINVERT);
    int ringCount=6+static_cast<int>(g_intensity*7),md=std::min(w,h);
    HBRUSH nb=static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,nb));
    for (int ring=1;ring<=ringCount;++ring){ColorRgb rc=HslToRgb(t*55+ring*45,0.95f,0.55f);float ph=t*(1.2f+g_intensity*0.8f+ring*0.28f);int r=(int)(ring*md/16.0f),ox=(int)(std::cos(ph)*r*0.45f),oy=(int)(std::sin(ph)*r*0.45f);HPEN pn=CreatePen(PS_SOLID,2,RGB(rc.r,rc.g,rc.b));HPEN op=static_cast<HPEN>(SelectObject(dst,pn));Ellipse(dst,w/2+ox-r,h/2+oy-r,w/2+ox+r,h/2+oy+r);SelectObject(dst,op);DeleteObject(pn);}
    SelectObject(dst,ob); ApplySineRipple(dst,w,h,t*(1.5f+g_intensity)); b.swap();
}

static float g_cx2 = 0, g_cy2 = 0, g_vx2 = 240, g_vy2 = 175;
static bool g_initCube = false;

void RunCube3DEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b); EnsurePixels(src,w,h);
    g_pixels.copyFrom(src,0,0); ApplyXorFractal(g_pixels,t*(0.6f+g_intensity*0.8f)); g_pixels.copyTo(dst,0,0);
    ApplySineRipple(dst,w,h,t*(0.9f+g_intensity*0.6f));
    if (t<0.05f || !g_initCube){g_cx2=w*0.5f;g_cy2=h*0.5f;g_vx2=240.0f+g_intensity*120;g_vy2=175.0f+g_intensity*80;g_initCube=true;}
    float sz=std::min(w,h)*0.26f; constexpr float dt=0.016f; float spd2=1.0f+g_intensity*1.5f;
    g_cx2+=g_vx2*dt*spd2; g_cy2+=g_vy2*dt*spd2;
    if(g_cx2-sz<0){g_cx2=sz;g_vx2=std::abs(g_vx2);} if(g_cx2+sz>w){g_cx2=w-sz;g_vx2=-std::abs(g_vx2);}
    if(g_cy2-sz<0){g_cy2=sz;g_vy2=std::abs(g_vy2);} if(g_cy2+sz>h){g_cy2=h-sz;g_vy2=-std::abs(g_vy2);}
    float rotSpd=1.1f+g_intensity*1.4f,ay=t*rotSpd,ax=t*rotSpd*0.66f;
    float cosY=std::cos(ay),sinY=std::sin(ay),cosX=std::cos(ax),sinX=std::sin(ax);
    static constexpr float kV[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    static constexpr int kE[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    static constexpr int kF[6][4]={{0,1,2,3},{4,7,6,5},{0,3,7,4},{1,5,6,2},{0,4,5,1},{3,2,6,7}};
    float pr[8][2]; float dp[8];
    for (int i=0;i<8;++i){float x=kV[i][0],y=kV[i][1],z=kV[i][2];float rx=x*cosY-z*sinY,rz=x*sinY+z*cosY;x=rx;z=rz;float ry=y*cosX-z*sinX;rz=y*sinX+z*cosX;y=ry;z=rz;dp[i]=z;float s=sz/(4.0f-z);pr[i][0]=g_cx2+x*s;pr[i][1]=g_cy2+y*s;}
    int fo[6]={0,1,2,3,4,5}; float fd[6];
    for (int f=0;f<6;++f) fd[f]=(dp[kF[f][0]]+dp[kF[f][1]]+dp[kF[f][2]]+dp[kF[f][3]])*0.25f;
    for (int i=1;i<6;++i){int k=fo[i];float kd=fd[k];int j=i-1;while(j>=0&&fd[fo[j]]>kd){fo[j+1]=fo[j];--j;}fo[j+1]=k;}
    HPEN np2=static_cast<HPEN>(GetStockObject(NULL_PEN)); HPEN op=static_cast<HPEN>(SelectObject(dst,np2));
    for (int fi=0;fi<6;++fi){int f=fo[fi];float lum=std::clamp(0.22f+(fd[f]+1.4f)*0.13f,0.12f,0.52f);ColorRgb fc=HslToRgb(t*42+f*60,0.95f,lum);POINT pts[4];for(int v=0;v<4;++v){pts[v].x=(LONG)pr[kF[f][v]][0];pts[v].y=(LONG)pr[kF[f][v]][1];}HBRUSH fb=CreateSolidBrush(RGB(fc.r,fc.g,fc.b));HBRUSH ob2=static_cast<HBRUSH>(SelectObject(dst,fb));Polygon(dst,pts,4);SelectObject(dst,ob2);DeleteObject(fb);}
    SelectObject(dst,op);
    ColorRgb ec=HueCycle(t*85+180); int epw=1+static_cast<int>(g_intensity*2); HPEN ep=CreatePen(PS_SOLID,epw,RGB(ec.r,ec.g,ec.b)); op=static_cast<HPEN>(SelectObject(dst,ep));
    for (int e=0;e<12;++e){MoveToEx(dst,(int)pr[kE[e][0]][0],(int)pr[kE[e][0]][1],nullptr);LineTo(dst,(int)pr[kE[e][1]][0],(int)pr[kE[e][1]][1]);}
    SelectObject(dst,op); DeleteObject(ep); b.swap();
}

void RunGeosphere3DEffect(ScreenBuffer& b, float t) {
    const HDC src = b.frontDC(), dst = b.backDC();
    const int w = b.width, h = b.height;
    CopyBaseFrame(b);

    const int cx = w / 2, cy = h / 2;
    const float radius = std::min(w, h) * (0.28f + 0.05f * std::sin(t * 1.8f));
    const float rotY = t * (1.2f + g_intensity * 1.5f);
    const float rotX = t * (0.8f + g_intensity * 1.0f);
    const float cosY = std::cos(rotY), sinY = std::sin(rotY);
    const float cosX = std::cos(rotX), sinX = std::sin(rotX);

    const float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;
    static const float kVerts[12][3] = {
        {-1, phi, 0}, {1, phi, 0}, {-1, -phi, 0}, {1, -phi, 0},
        {0, -1, phi}, {0, 1, phi}, {0, -1, -phi}, {0, 1, -phi},
        {phi, 0, -1}, {phi, 0, 1}, {-phi, 0, -1}, {-phi, 0, 1}
    };
    static const int kEdges[30][2] = {
        {0,11},{0,5},{0,1},{0,7},{0,10},{1,5},{1,7},{1,8},{1,9},{2,11},
        {2,10},{2,4},{2,6},{2,3},{3,9},{3,8},{3,4},{3,6},{4,9},{4,5},
        {4,11},{5,9},{5,11},{6,10},{6,7},{6,8},{7,10},{7,8},{8,9},{10,11}
    };

    POINT proj[12];
    for (int i = 0; i < 12; ++i) {
        float x = kVerts[i][0] / phi;
        float y = kVerts[i][1] / phi;
        float z = kVerts[i][2] / phi;

        float rx = x * cosY - z * sinY;
        float rz = x * sinY + z * cosY;
        float ry = y * cosX - rz * sinX;
        rz = y * sinX + rz * cosX;

        float dist = 3.5f - rz;
        proj[i].x = cx + static_cast<int>((rx * radius * 2.2f) / dist);
        proj[i].y = cy + static_cast<int>((ry * radius * 2.2f) / dist);
    }

    for (int e = 0; e < 30; ++e) {
        ColorRgb c = HslToRgb(t * 80.0f + e * 12.0f, 0.95f, 0.6f);
        HPEN pen = CreatePen(PS_SOLID, 2 + static_cast<int>(g_intensity * 2.0f), RGB(c.r, c.g, c.b));
        HPEN oldPen = static_cast<HPEN>(SelectObject(dst, pen));

        MoveToEx(dst, proj[kEdges[e][0]].x, proj[kEdges[e][0]].y, nullptr);
        LineTo(dst, proj[kEdges[e][1]].x, proj[kEdges[e][1]].y);

        SelectObject(dst, oldPen);
        DeleteObject(pen);
    }

    if (g_intensity > 0.3f) {
        int off = static_cast<int>(std::sin(t * 3.0f) * 14.0f);
        BitBlt(dst, off, 0, w, h, src, 0, 0, SRCINVERT);
    }
    b.swap();
}

void RunAcidEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b); EnsurePixels(src,w,h);
    g_pixels.copyFrom(src,0,0); ApplyXorFractal(g_pixels,t*(1.0f+g_intensity*2)); ApplyChannelSplit(g_pixels,(int)((10.0f+g_intensity*20)*std::sin(t*2.3f)),(int)(-(10.0f+g_intensity*20)*std::cos(t*1.9f))); g_pixels.copyTo(dst,0,0);
    int shift=(int)((8.0f+g_intensity*14)+6*std::sin(t*2.3f));
    BitBlt(dst,shift,0,w-shift,h,src,0,0,SRCINVERT); BitBlt(dst,-shift/2,0,w,h,src,shift/2,0,SRCPAINT);
    ColorRgb hc=HueCycleTime(t); HBRUSH hatch=CreateHatchBrush(HS_DIAGCROSS,RGB(hc.r,hc.g,hc.b)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,hatch)); PatBlt(dst,0,0,w,h,PATINVERT);
    int band=std::max(8,(int)((40.0f-g_intensity*22)+20*std::sin(t*3)));
    for (int y=-h;y<h*2;y+=band) PatBlt(dst,0,y+(int)(t*(60.0f+g_intensity*80))%band,w,band/3,PATINVERT);
    SelectObject(dst,ob); DeleteObject(hatch); ApplySineRipple(dst,w,h,t*(1+g_intensity)); b.swap();
}

// ===========================================================================
// TIER 5 - Peak Chaos & Total Meltdown (75%)
// ===========================================================================

void RunDrugRainEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b);
    int cW=std::max(6,14-(int)(g_intensity*6)); float spd=2.8f+g_intensity*3.5f;
    for (int x=0;x<w;x+=cW){float ph=x*0.065f+t*spd;int drop=std::clamp((int)(h*0.5f*(1+std::sin(ph))),0,h-1);if(drop>0&&drop<h){BitBlt(dst,x,0,cW,h-drop,src,x,drop,SRCCOPY);ColorRgb gc=HslToRgb(x*0.28f+t*75,0.9f,0.42f);HBRUSH gb=CreateSolidBrush(RGB(gc.r,gc.g,gc.b));RECT rc2{x,h-drop,x+cW,h};FillRect(dst,&rc2,gb);DeleteObject(gb);}}
    HBRUSH sc=CreateHatchBrush(HS_FDIAGONAL,RGB(0,255,180)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,sc));
    PatBlt(dst,0,(int)std::fmod(t*(320.0f+g_intensity*280),h),w,4,PATINVERT);
    SelectObject(dst,ob); DeleteObject(sc); BitBlt(dst,0,0,w,h,src,0,0,SRCPAINT); b.swap();
}

void RunRgbShaderEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    EnsurePixels(src,w,h); g_pixels.copyFrom(src,0,0);
    if ((int)t%4<2) ApplyHslShader(g_pixels,t*(1.0f+g_intensity)); else ApplyXorFractal(g_pixels,t*(2.0f+g_intensity*1.5f));
    g_pixels.copyTo(dst,0,0);
    int off=(int)((12.0f+g_intensity*14)+8*std::sin(t));
    BitBlt(dst,off,0,w-off,h,dst,0,0,SRCAND); BitBlt(dst,-off,0,w,h,dst,0,0,SRCPAINT); BitBlt(dst,0,off,w,h-off,dst,0,0,SRCINVERT);
    ApplySineRipple(dst,w,h,t*(1.3f+g_intensity)); b.swap();
}

void RunXorStormEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    EnsurePixels(src,w,h); g_pixels.copyFrom(src,0,0);
    float spd=1.0f+g_intensity*2.2f;
    int t1=(int)(t*130*spd),t2=(int)(t*88*spd),t3=(int)(t*210*spd);
    BgrPixel* bits=g_pixels.bits();
    for (int y=0;y<g_pixels.height;++y) for (int x=0;x<g_pixels.width;++x){int i=y*g_pixels.width+x;bits[i].r^=(BYTE)((x^y^t1)&0xFF);bits[i].g^=(BYTE)(((x*3)^(y*5)^t2)&0xFF);bits[i].b^=(BYTE)(((x^(y<<1))^t3)&0xFF);}
    g_pixels.copyTo(dst,0,0);
    int passes=2+static_cast<int>(g_intensity*2);
    static constexpr DWORD kR[3]={SRCINVERT,SRCPAINT,MERGEPAINT};
    for (int p=0;p<passes;++p){int ox=(int)(std::sin(t*(2.0f+p*0.65f))*(18.0f+g_intensity*14));int oy=(int)(std::cos(t*(1.4f+p*0.85f))*(12.0f+g_intensity*10));BitBlt(dst,ox,oy,w,h,src,0,0,kR[p%3]);}
    ApplySineRipple(dst,w,h,t*(3.0f+g_intensity*2)); b.swap();
}

void RunChromaBlastEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b);
    struct Pass{float asp,rfx,rfy;DWORD rop;};
    static constexpr Pass kP[]={{2.3f,0.22f,0.18f,SRCINVERT},{1.7f,0.32f,0.25f,SRCPAINT},{3.1f,0.14f,0.12f,MERGEPAINT},{1.1f,0.40f,0.35f,SRCINVERT},{2.7f,0.28f,0.20f,SRCPAINT}};
    float spd=1.0f+g_intensity*1.8f;
    for (const auto& p:kP){float a=t*p.asp*spd;int ox=(int)(w*(p.rfx+g_intensity*0.08f)*std::cos(a)),oy=(int)(h*(p.rfy+g_intensity*0.06f)*std::sin(a));int bx=std::max(0,ox),by=std::max(0,oy),bw=w-std::abs(ox),bh=h-std::abs(oy);if(bw>0&&bh>0)BitBlt(dst,bx,by,bw,bh,src,std::max(0,-ox),std::max(0,-oy),p.rop);}
    ColorRgb hc=HueCycleTime(t); HBRUSH ht=CreateHatchBrush(HS_DIAGCROSS,RGB(hc.r,hc.g,hc.b)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,ht)); PatBlt(dst,0,0,w,h,PATINVERT);
    int bd=std::max(20,80-(int)(g_intensity*50)),bdY=(int)std::fmod(t*(420+g_intensity*360),h+bd)-bd; PatBlt(dst,0,bdY,w,bd,PATCOPY);
    SelectObject(dst,ob); DeleteObject(ht); ApplySineRipple(dst,w,h,t*(2.2f+g_intensity)); b.swap();
}

void RunMeltChaosEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b); std::srand((unsigned)(t*1000));
    float maxDrip=30.0f+g_intensity*50;
    for (int y=0;y<h;y+=3){int drip=(int)((std::rand()%(int)(maxDrip*2+1))-maxDrip)+WaveOff(t,0.8f,25.0f,y);BitBlt(dst,drip,y,w-std::abs(drip),3,src,0,y,SRCCOPY);}
    for (int x=0;x<w;x+=3){int drip=WaveOff(t,1.1f,20.0f,x);if(x+3<w)BitBlt(dst,x,drip,3,h-std::abs(drip),src,x,0,SRCCOPY);}
    HBRUSH db=CreateSolidBrush(RGB(120,255,180)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,db));
    for (int y=0;y<h;y+=std::max(12,48-(int)(g_intensity*30))) PatBlt(dst,WaveOff(t,1.5f,static_cast<float>(w/3),y),y,80,6,PATINVERT);
    SelectObject(dst,ob); DeleteObject(db); BitBlt(dst,0,0,w,h,src,0,0,MERGEPAINT); b.swap();
}

void RunSalviaEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    HBRUSH bg=CreateSolidBrush(RGB(10,5,20));RECT full{0,0,w,h};FillRect(dst,&full,bg);DeleteObject(bg);
    float spd=1.0f+g_intensity; int tW=std::max(60,140-(int)(g_intensity*60)),tH=std::max(40,100-(int)(g_intensity*40));
    for (int y=0;y<h;y+=tH) for (int x=0;x<w;x+=tW){int ti=(x/tW)+(y/tH);float amp=90.0f+g_intensity*60;int sx=x+WaveOff(t*spd,3.0f,amp,ti),sy=y+WaveOff(t*spd,2.4f,70.0f,ti+11);BitBlt(dst,x,y,tW,tH,src,sx,sy,SRCCOPY);int sk=(int)((40.0f+g_intensity*30)*std::sin(t*0.8f*spd+ti*0.5f));POINT c[3]={{x,y},{x+tW+sk,y-sk/2},{x-sk/2,y+tH}};PlgBlt(dst,c,src,sx,sy,tW,tH,nullptr,0,0);HBRUSH ln=CreateSolidBrush(RGB(0,255,120));HBRUSH ol=static_cast<HBRUSH>(SelectObject(dst,ln));PatBlt(dst,x,y,tW,2,PATCOPY);PatBlt(dst,x,y,2,tH,PATCOPY);SelectObject(dst,ol);DeleteObject(ln);}
    BitBlt(dst,0,0,w/2,h,dst,w/2,0,NOTSRCCOPY); BitBlt(dst,w/2,0,w/2,h,dst,0,0,SRCINVERT);
    HBRUSH sc=CreateHatchBrush(HS_BDIAGONAL,RGB(180,0,255));HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,sc));
    PatBlt(dst,0,(int)std::fmod(t*(220.0f+g_intensity*200),(float)h),w,8,PATINVERT);
    SelectObject(dst,ob);DeleteObject(sc);
    POINT wpts[3]={{(LONG)(80*std::sin(t*spd)),0},{w+(int)(80*std::cos(t*0.7f*spd)),0},{0,h}};
    PlgBlt(dst,wpts,src,0,0,w,h,nullptr,0,0); b.swap();
}

void RunDaturaEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b); float spd=1.0f+g_intensity*0.8f;
    int gA=(int)((16.0f+g_intensity*20)+12*std::sin(t*0.7f*spd)),gB=(int)((-12.0f-g_intensity*12)+10*std::cos(t*1.1f*spd));
    BitBlt(dst,gA,gA/2,w-gA,h,src,0,0,SRCAND); BitBlt(dst,gB,-gB,w,h,src,-gB,0,MERGEPAINT); BitBlt(dst,0,0,w,h,src,gA/3,gB/3,SRCPAINT);
    HBRUSH dl=CreateSolidBrush(RGB(210,190,60)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,dl)); PatBlt(dst,0,0,w,h,0x00AF0229); SelectObject(dst,ob); DeleteObject(dl);
    HBRUSH vg=CreateSolidBrush(RGB(0,0,0)); ob=static_cast<HBRUSH>(SelectObject(dst,vg));
    int mg=(int)((90.0f-g_intensity*40)+50*std::sin(t*0.5f));
    if (mg>0){PatBlt(dst,0,0,w,mg,PATCOPY);PatBlt(dst,0,h-mg,w,mg,PATCOPY);PatBlt(dst,0,0,mg,h,PATCOPY);PatBlt(dst,w-mg,0,mg,h,PATCOPY);}
    SelectObject(dst,ob);DeleteObject(vg);
    for (int y=0;y<h;y+=40){int off=WaveOff(t*spd,1.2f,45.0f+g_intensity*30,y);BitBlt(dst,off,y,w-std::abs(off),20,src,0,y,SRCCOPY);}
    if ((int)(t*8*(1+g_intensity))%2==0) PatBlt(dst,0,0,w,h,DSTINVERT); b.swap();
}

void RunTunnelDoomEffect(ScreenBuffer& b, float t) {
    const HDC src=b.frontDC(),dst=b.backDC(); const int w=b.width,h=b.height;
    CopyBaseFrame(b); float spd=1.0f+g_intensity*1.5f;
    int step=(int)((18.0f+g_intensity*14)+14*std::sin(t*1.8f));
    BitBlt(dst,0,0,w,h,dst,-step,0,SRCCOPY); BitBlt(dst,0,0,w,h,dst,w-step,0,SRCCOPY);
    BitBlt(dst,0,0,w,h,dst,0,-step,SRCCOPY); BitBlt(dst,0,0,w,h,dst,0,h-step,SRCCOPY);
    float p=0.04f+0.02f*std::sin(t*2.5f*spd); int tw=std::max(4,(int)(w*p)),th=std::max(4,(int)(h*p));
    SetStretchBltMode(dst,COLORONCOLOR); StretchBlt(dst,(w-tw)/2,(h-th)/2,tw,th,src,0,0,w,h,SRCCOPY); StretchBlt(dst,0,0,w,h,dst,(w-tw)/2,(h-th)/2,tw,th,SRCCOPY);
    float rs=std::sin(t*0.9f*spd)*(120.0f+g_intensity*60);
    POINT pts[3]={{(LONG)rs,0},{w,(LONG)(-rs)},{0,h}};
    PlgBlt(dst,pts,dst,0,0,w,h,nullptr,0,0); BitBlt(dst,0,0,w,h,src,0,0,SRCINVERT);
    BitBlt(dst,0,0,w,h,dst,WaveOff(t*spd,3.0f,20.0f,0),0,SRCPAINT);
    ColorRgb rg=HueCycleTime(t); HBRUSH rgb=CreateSolidBrush(RGB(rg.r,rg.g,rg.b)); HBRUSH ob=static_cast<HBRUSH>(SelectObject(dst,rgb)); PatBlt(dst,w/4,h/4,w/2,h/2,PATINVERT); SelectObject(dst,ob);DeleteObject(rgb); b.swap();
}

// ---------------------------------------------------------------------------
// Primary effect dispatcher (54 modes)
// ---------------------------------------------------------------------------
void RunEffect(TripMode mode, ScreenBuffer& buffer, float time) {
    switch (mode) {
    // Tier 0
    case TripMode::ColorInvert:        RunColorInvertEffect(buffer, time); break;
    case TripMode::ScreenGlitch:       RunScreenGlitchEffect(buffer, time); break;
    case TripMode::ScreenMelt:         RunScreenMeltEffect(buffer, time); break;
    case TripMode::ChannelGhost:       RunChannelGhostEffect(buffer, time); break;
    case TripMode::HorizTear:          RunHorizTearEffect(buffer, time); break;
    case TripMode::WaveDistort:        RunWaveDistortEffect(buffer, time); break;
    case TripMode::ScreenEcho:         RunScreenEchoEffect(buffer, time); break;
    case TripMode::Checkerboard:       RunCheckerboardEffect(buffer, time); break;

    // Tier 1
    case TripMode::ScreenTunnel:       RunScreenTunnelEffect(buffer, time); break;
    case TripMode::ScreenShake:        RunScreenShakeEffect(buffer, time); break;
    case TripMode::IconSpam:           RunIconSpamEffect(buffer, time); break;
    case TripMode::CursorStorm:        RunCursorStormEffect(buffer, time); break;
    case TripMode::ErrorDialogCascade: RunErrorDialogCascadeEffect(buffer, time); break;
    case TripMode::GridMirror:         RunGridMirrorEffect(buffer, time); break;
    case TripMode::Pixelate:           RunPixelateEffect(buffer, time); break;
    case TripMode::StrobeMirror:       RunStrobeMirrorEffect(buffer, time); break;
    case TripMode::RGBSplit:           RunRGBSplitEffect(buffer, time); break;
    case TripMode::RadialRipples:      RunRadialRipplesEffect(buffer, time); break;

    // Tier 2
    case TripMode::SliceTear:          RunSliceTearEffect(buffer, time); break;
    case TripMode::DiagonalShear:      RunDiagonalShearEffect(buffer, time); break;
    case TripMode::PieSliceInvert:     RunPieSliceInvertEffect(buffer, time); break;
    case TripMode::LogoBounce:         RunLogoBounceEffect(buffer, time); break;
    case TripMode::HatchStorm:         RunHatchStormEffect(buffer, time); break;
    case TripMode::PlgRotate:          RunPlgRotateEffect(buffer, time); break;
    case TripMode::Plasma:             RunPlasmaEffect(buffer, time); break;
    case TripMode::Interference:       RunInterferenceEffect(buffer, time); break;
    case TripMode::MatrixSpam:         RunMatrixSpamEffect(buffer, time); break;

    // Tier 3 - Fractals
    case TripMode::Mandelbrot:         RunMandelbrotEffect(buffer, time); break;
    case TripMode::JuliaFractal:       RunJuliaFractalEffect(buffer, time); break;
    case TripMode::NewtonFractal:      RunNewtonFractalEffect(buffer, time); break;
    case TripMode::Lyapunov:           RunLyapunovEffect(buffer, time); break;
    case TripMode::BarnsleyFern:       RunBarnsleyFernEffect(buffer, time); break;
    case TripMode::Apollonian:         RunApollonianEffect(buffer, time); break;
    case TripMode::DragonCurve:        RunDragonCurveEffect(buffer, time); break;
    case TripMode::KochSnowflake:      RunKochSnowflakeEffect(buffer, time); break;
    case TripMode::Sierpinski:         RunSierpinskiEffect(buffer, time); break;
    case TripMode::BurningShip:        RunBurningShipEffect(buffer, time); break;

    // Tier 4
    case TripMode::Mandala:            RunMandalaEffect(buffer, time); break;
    case TripMode::WarpTunnel:         RunWarpTunnelEffect(buffer, time); break;
    case TripMode::ZoomFeedback:       RunZoomFeedbackEffect(buffer, time); break;
    case TripMode::Helix:              RunHelixEffect(buffer, time); break;
    case TripMode::Mushrooms:          RunMushroomEffect(buffer, time); break;
    case TripMode::VortexSpin:         RunVortexSpinEffect(buffer, time); break;
    case TripMode::Cube3D:             RunCube3DEffect(buffer, time); break;
    case TripMode::Geosphere3D:        RunGeosphere3DEffect(buffer, time); break;
    case TripMode::Acid:               RunAcidEffect(buffer, time); break;

    // Tier 5
    case TripMode::DrugRain:           RunDrugRainEffect(buffer, time); break;
    case TripMode::RgbShader:          RunRgbShaderEffect(buffer, time); break;
    case TripMode::XorStorm:           RunXorStormEffect(buffer, time); break;
    case TripMode::ChromaBlast:        RunChromaBlastEffect(buffer, time); break;
    case TripMode::MeltChaos:          RunMeltChaosEffect(buffer, time); break;
    case TripMode::Salvia:             RunSalviaEffect(buffer, time); break;
    case TripMode::Datura:             RunDaturaEffect(buffer, time); break;
    case TripMode::TunnelDoom:         RunTunnelDoomEffect(buffer, time); break;
    default:                           RunColorInvertEffect(buffer, time); break;
    }
}

void CleanupEffectResources() {
    g_pixels.cleanup();
    g_fractalLayer.cleanup();
    g_pixelsReady = false;
    g_initLogo = false;
    g_initCube = false;
}
