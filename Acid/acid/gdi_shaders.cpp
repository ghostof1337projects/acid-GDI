#include "gdi_shaders.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

constexpr float kPi = std::numbers::pi_v<float>;

inline BYTE ClampByte(int v) {
    return static_cast<BYTE>(std::clamp(v, 0, 255));
}

} // namespace

ColorRgb HueCycle(float phase) {
    const float rad = phase * kPi / 180.0f;
    return {
        ClampByte(static_cast<int>(127.0f + 127.0f * std::sin(rad))),
        ClampByte(static_cast<int>(127.0f + 127.0f * std::sin(rad + 2.094f))),
        ClampByte(static_cast<int>(127.0f + 127.0f * std::sin(rad + 4.188f))),
    };
}

ColorRgb HslToRgb(float h, float s, float l) {
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) {
        h += 360.0f;
    }

    const float c = (1.0f - std::fabs(2.0f * l - 1.0f)) * s;
    const float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    const float m = l - c / 2.0f;

    float rp = 0.0f;
    float gp = 0.0f;
    float bp = 0.0f;

    if (h < 60.0f) {
        rp = c; gp = x;
    } else if (h < 120.0f) {
        rp = x; gp = c;
    } else if (h < 180.0f) {
        gp = c; bp = x;
    } else if (h < 240.0f) {
        gp = x; bp = c;
    } else if (h < 300.0f) {
        rp = x; bp = c;
    } else {
        rp = c; bp = x;
    }

    return {
        ClampByte(static_cast<int>((rp + m) * 255.0f)),
        ClampByte(static_cast<int>((gp + m) * 255.0f)),
        ClampByte(static_cast<int>((bp + m) * 255.0f)),
    };
}

// Inspired by pankoza2-pl/gdithings "betterrgbquad" — XOR/bit fractal on pixels.
void ApplyXorFractal(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width;
    const int h = buffer.height;
    const int t = static_cast<int>(time * 60.0f);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int i = y * w + x;
            const int pattern = (x ^ y ^ t) & (x + y + t);
            const BgrPixel& src = px[i];

            px[i].r = static_cast<BYTE>((src.r ^ pattern) & 0xFF);
            px[i].g = static_cast<BYTE>((src.g ^ (pattern >> 3)) & 0xFF);
            px[i].b = static_cast<BYTE>((src.b ^ (pattern >> 5)) & 0xFF);
        }
    }
}

// HSL sweep shader — technique family from gdithings hslshader / Lixo trojan writeups.
void ApplyHslShader(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int count = buffer.width * buffer.height;
    const float hueBase = time * 55.0f;

    for (int i = 0; i < count; ++i) {
        const BgrPixel& src = px[i];
        const float lum = (src.r * 0.299f + src.g * 0.587f + src.b * 0.114f) / 255.0f;
        const float hue = hueBase + i * 0.015f;
        const ColorRgb out = HslToRgb(hue, 0.85f, lum * 0.55f + 0.2f);
        px[i].r = out.r;
        px[i].g = out.g;
        px[i].b = out.b;
    }
}

void ApplyChannelSplit(PixelBuffer& buffer, int shiftR, int shiftB) {
    PixelBuffer temp {};
    HDC ref = CreateCompatibleDC(nullptr);
    if (!ref || !temp.init(ref, buffer.width, buffer.height)) {
        DeleteDC(ref);
        return;
    }

    temp.copyFrom(buffer.dc(), 0, 0);
    BgrPixel* dst = buffer.bits();
    const BgrPixel* src = temp.bits();
    const int w = buffer.width;
    const int h = buffer.height;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int i = y * w + x;
            const int xr = std::clamp(x + shiftR, 0, w - 1);
            const int xb = std::clamp(x + shiftB, 0, w - 1);
            const int ir = y * w + xr;
            const int ib = y * w + xb;

            dst[i].r = src[ir].r;
            dst[i].g = src[i].g;
            dst[i].b = src[ib].b;
        }
    }

    temp.cleanup();
    DeleteDC(ref);
}

void RenderJuliaFractal(PixelBuffer& buffer, float time, float opacity) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width;
    const int h = buffer.height;

    const float cx = -0.7f + 0.3f * std::sin(time * 0.4f);
    const float cy = 0.27015f + 0.2f * std::cos(time * 0.31f);
    const float scale = 3.2f / static_cast<float>(std::min(w, h));

    for (int py = 0; py < h; ++py) {
        for (int pxX = 0; pxX < w; ++pxX) {
            float zx = (pxX - w * 0.5f) * scale;
            float zy = (py - h * 0.5f) * scale;

            int iter = 0;
            constexpr int maxIter = 64;
            while (zx * zx + zy * zy < 4.0f && iter < maxIter) {
                const float tx = zx * zx - zy * zy + cx;
                zy = 2.0f * zx * zy + cy;
                zx = tx;
                ++iter;
            }

            const float t = static_cast<float>(iter) / maxIter;
            const ColorRgb fractal = HslToRgb(t * 300.0f + time * 40.0f, 0.9f, t * 0.45f + 0.05f);
            const int i = py * w + pxX;
            BgrPixel& out = px[i];

            const float a = std::clamp(opacity, 0.0f, 1.0f);
            out.r = ClampByte(static_cast<int>(out.r * (1.0f - a) + fractal.r * a));
            out.g = ClampByte(static_cast<int>(out.g * (1.0f - a) + fractal.g * a));
            out.b = ClampByte(static_cast<int>(out.b * (1.0f - a) + fractal.b * a));
        }
    }
}

void RenderMandelbrotOverlay(PixelBuffer& buffer, float time, float zoom) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width;
    const int h = buffer.height;

    const float centerX = -0.745f + 0.05f * std::sin(time * 0.2f);
    const float centerY = 0.113f + 0.05f * std::cos(time * 0.17f);
    const float span = 1.6f / zoom;

    for (int py = 0; py < h; ++py) {
        for (int pxX = 0; pxX < w; ++pxX) {
            const float cr = centerX + (pxX - w * 0.5f) / w * span;
            const float ci = centerY + (py - h * 0.5f) / h * span;

            float zr = 0.0f;
            float zi = 0.0f;
            int iter = 0;
            constexpr int maxIter = 80;
            while (zr * zr + zi * zi < 4.0f && iter < maxIter) {
                const float tr = zr * zr - zi * zi + cr;
                zi = 2.0f * zr * zi + ci;
                zr = tr;
                ++iter;
            }

            const float t = static_cast<float>(iter) / maxIter;
            const ColorRgb fractal = HslToRgb(240.0f - t * 240.0f + time * 25.0f, 0.95f, t * 0.5f);
            const int i = py * w + pxX;
            BgrPixel& out = px[i];

            out.r = ClampByte(static_cast<int>(out.r * 0.35f + fractal.r * 0.65f));
            out.g = ClampByte(static_cast<int>(out.g * 0.35f + fractal.g * 0.65f));
            out.b = ClampByte(static_cast<int>(out.b * 0.35f + fractal.b * 0.65f));
        }
    }
}

// ---------------------------------------------------------------------------
// Sierpinski carpet XOR field - animated zoom + HSL color blend
// ---------------------------------------------------------------------------
void RenderSierpinskiLayer(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w  = buffer.width;
    const int h  = buffer.height;

    const int scale = std::max(2, static_cast<int>(28.0f + 20.0f * std::sin(time * 0.35f)));
    const int offX  = static_cast<int>(time * 28.0f);
    const int offY  = static_cast<int>(time * 19.0f);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int sx = (x + offX) / scale;
            const int sy = (y + offY) / scale;
            const bool inFull  = (sx & sy) == 0;
            const bool inInner = ((sx / 3) & (sy / 3)) == 0;
            const int i = y * w + x;

            if (inFull && inInner) {
                // Hot pixel: vivid HSL colour
                const float hue = time * 55.0f + (x ^ y) * 0.04f;
                const ColorRgb c = HslToRgb(hue, 1.0f, 0.65f);
                const float a = 0.82f;
                px[i].r = ClampByte(static_cast<int>(px[i].r * (1.0f - a) + c.r * a));
                px[i].g = ClampByte(static_cast<int>(px[i].g * (1.0f - a) + c.g * a));
                px[i].b = ClampByte(static_cast<int>(px[i].b * (1.0f - a) + c.b * a));
            } else if (inFull) {
                // Mid pixel: dimmer tint
                const float hue = time * 55.0f + 140.0f + x * 0.018f;
                const ColorRgb c = HslToRgb(hue, 0.85f, 0.38f);
                const float a = 0.45f;
                px[i].r = ClampByte(static_cast<int>(px[i].r * (1.0f - a) + c.r * a));
                px[i].g = ClampByte(static_cast<int>(px[i].g * (1.0f - a) + c.g * a));
                px[i].b = ClampByte(static_cast<int>(px[i].b * (1.0f - a) + c.b * a));
            } else {
                // Shadow pixel: darken
                px[i].r = static_cast<BYTE>(px[i].r >> 1);
                px[i].g = static_cast<BYTE>(px[i].g >> 1);
                px[i].b = static_cast<BYTE>(px[i].b >> 1);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 4-sine Lissajous plasma
// ---------------------------------------------------------------------------
void RenderPlasmaLayer(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w  = buffer.width;
    const int h  = buffer.height;
    const float cx = w * 0.5f, cy = h * 0.5f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float fx   = x * 0.042f;
            const float fy   = y * 0.042f;
            const float dist = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy)) * 0.038f;

            float v = std::sin(fx + time * 1.0f);
            v      += std::sin(fy + time * 1.3f);
            v      += std::sin((fx + fy) * 0.8f + time * 0.9f);
            v      += std::sin(dist + time * 1.6f);
            v       = (v + 4.0f) * 0.125f; // normalise [0,1]

            const ColorRgb c = HslToRgb(v * 360.0f + time * 28.0f, 0.97f, 0.52f);
            const int i = y * w + x;
            px[i].r = c.r;
            px[i].g = c.g;
            px[i].b = c.b;
        }
    }
}

// ---------------------------------------------------------------------------
// Burning Ship fractal (abs values before squaring)
// ---------------------------------------------------------------------------
void RenderBurningShipLayer(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w  = buffer.width;
    const int h  = buffer.height;

    const float centerX = -1.76f + 0.06f * std::sin(time * 0.18f);
    const float centerY = -0.04f + 0.05f * std::cos(time * 0.14f);
    const float zoom    = 1.6f + 0.9f * std::sin(time * 0.22f);
    const float span    = 2.6f / zoom;

    for (int py = 0; py < h; ++py) {
        for (int pxX = 0; pxX < w; ++pxX) {
            const float cr = centerX + (pxX - w * 0.5f) / w * span;
            const float ci = centerY + (py  - h * 0.5f) / h * span;

            float zr = 0.0f, zi = 0.0f;
            int   iter = 0;
            constexpr int maxIter = 80;
            while (zr * zr + zi * zi < 4.0f && iter < maxIter) {
                const float tr = zr * zr - zi * zi + cr;
                zi = 2.0f * std::abs(zr) * std::abs(zi) + ci;
                zr = tr;
                ++iter;
            }

            const float t = static_cast<float>(iter) / maxIter;
            const ColorRgb c = HslToRgb(t * 290.0f + time * 32.0f, 0.92f, t * 0.48f + 0.02f);
            const int i = py * w + pxX;
            px[i].r = c.r;
            px[i].g = c.g;
            px[i].b = c.b;
        }
    }
}

// ---------------------------------------------------------------------------
// N-fold polar kaleidoscope - mirror-fold each pixel into one segment
// ---------------------------------------------------------------------------
void ApplyKaleidoscope(PixelBuffer& buffer, int segments, float time) {
    PixelBuffer temp {};
    HDC ref = CreateCompatibleDC(nullptr);
    if (!ref || !temp.init(ref, buffer.width, buffer.height)) {
        DeleteDC(ref);
        return;
    }

    temp.copyFrom(buffer.dc(), 0, 0);
    BgrPixel*       dst = buffer.bits();
    const BgrPixel* src = temp.bits();
    const int   w = buffer.width;
    const int   h = buffer.height;
    const float cx = w * 0.5f, cy = h * 0.5f;
    const float segAngle = 2.0f * kPi / static_cast<float>(segments);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float dx = x - cx, dy = y - cy;
            const float r  = std::sqrt(dx * dx + dy * dy);
            float angle    = std::atan2(dy, dx) + time * 0.38f;

            // Fold angle into [0, segAngle)
            angle = std::fmod(std::fabs(angle), segAngle);
            if (angle > segAngle * 0.5f) angle = segAngle - angle;

            const int sx = std::clamp(static_cast<int>(cx + r * std::cos(angle)), 0, w - 1);
            const int sy = std::clamp(static_cast<int>(cy + r * std::sin(angle)), 0, h - 1);
            dst[y * w + x] = src[sy * w + sx];
        }
    }

    temp.cleanup();
    DeleteDC(ref);
}

// ---------------------------------------------------------------------------
// Two-source wave interference pattern - beautiful moiré-like colour field
// ---------------------------------------------------------------------------
void RenderInterferenceLayer(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width, h = buffer.height;
    const float s1x = (0.32f + 0.18f * std::cos(time * 0.50f)) * w;
    const float s1y = (0.50f + 0.18f * std::sin(time * 0.32f)) * h;
    const float s2x = (0.68f - 0.18f * std::cos(time * 0.41f)) * w;
    const float s2y = (0.50f - 0.18f * std::sin(time * 0.55f)) * h;
    const float freq = 0.088f;
    const float ts   = time * 2.8f;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float d1 = std::sqrt((x-s1x)*(x-s1x) + (y-s1y)*(y-s1y));
            const float d2 = std::sqrt((x-s2x)*(x-s2x) + (y-s2y)*(y-s2y));
            const float v  = std::sin(d1*freq - ts) + std::sin(d2*freq - ts);
            const float n  = (v + 2.0f) * 0.25f; // [0,1]
            const ColorRgb c = HslToRgb(n*360.0f + time*22.0f, 0.97f, 0.50f);
            const int i = y*w + x;
            px[i].r = c.r; px[i].g = c.g; px[i].b = c.b;
        }
    }
}

// ---------------------------------------------------------------------------
// Newton fractal for z^3 - 1 = 0 with rotating perspective
// ---------------------------------------------------------------------------
void RenderNewtonFractal(PixelBuffer& buffer, float time, float zoom) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width;
    const int h = buffer.height;
    const float scale = (2.8f / zoom) / static_cast<float>(std::min(w, h));
    const float rot = time * 0.22f;
    const float cosR = std::cos(rot), sinR = std::sin(rot);

    for (int py = 0; py < h; ++py) {
        for (int pxX = 0; pxX < w; ++pxX) {
            float x0 = (pxX - w * 0.5f) * scale;
            float y0 = (py - h * 0.5f) * scale;
            float x = x0 * cosR - y0 * sinR;
            float y = x0 * sinR + y0 * cosR;

            int root = 0;
            int iter = 0;
            constexpr int maxIter = 30;

            while (iter < maxIter) {
                float x2 = x * x, y2 = y * y;
                float r2 = x2 + y2;
                if (r2 < 1e-6f) break;

                float d1 = (x - 1.0f) * (x - 1.0f) + y2;
                float d2 = (x + 0.5f) * (x + 0.5f) + (y - 0.8660254f) * (y - 0.8660254f);
                float d3 = (x + 0.5f) * (x + 0.5f) + (y + 0.8660254f) * (y + 0.8660254f);

                if (d1 < 0.001f) { root = 1; break; }
                if (d2 < 0.001f) { root = 2; break; }
                if (d3 < 0.001f) { root = 3; break; }

                float denom = 3.0f * (r2 * r2);
                float invX = (x2 - y2) / denom;
                float invY = (-2.0f * x * y) / denom;

                x = (2.0f / 3.0f) * x + invX;
                y = (2.0f / 3.0f) * y + invY;
                ++iter;
            }

            float hue = (root == 1 ? 0.0f : (root == 2 ? 120.0f : 240.0f)) + time * 35.0f + iter * 6.0f;
            float lum = std::clamp(0.12f + (static_cast<float>(iter) / maxIter) * 0.75f, 0.1f, 0.85f);
            ColorRgb c = HslToRgb(hue, 0.96f, lum);
            int i = py * w + pxX;
            px[i].r = c.r; px[i].g = c.g; px[i].b = c.b;
        }
    }
}

// ---------------------------------------------------------------------------
// Lyapunov fractal with chaotic population orbit mapping
// ---------------------------------------------------------------------------
void RenderLyapunovFractal(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width;
    const int h = buffer.height;
    const float minA = 2.4f + 0.35f * std::sin(time * 0.15f);
    const float maxA = minA + 1.4f;
    const float minB = 2.8f + 0.30f * std::cos(time * 0.12f);
    const float maxB = minB + 1.2f;
    static const int kSeq[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
    constexpr int kSeqLen = 8;
    constexpr int kWarmup = 18;
    constexpr int kSteps  = 40;

    for (int py = 0; py < h; ++py) {
        float b = minB + (static_cast<float>(py) / h) * (maxB - minB);
        for (int pxX = 0; pxX < w; ++pxX) {
            float a = minA + (static_cast<float>(pxX) / w) * (maxA - minA);
            float x = 0.5f;

            for (int k = 0; k < kWarmup; ++k) {
                float r = kSeq[k % kSeqLen] == 0 ? a : b;
                x = r * x * (1.0f - x);
            }

            float lambda = 0.0f;
            for (int k = 0; k < kSteps; ++k) {
                float r = kSeq[(k + kWarmup) % kSeqLen] == 0 ? a : b;
                x = r * x * (1.0f - x);
                float deriv = std::abs(r * (1.0f - 2.0f * x));
                if (deriv > 1e-6f) lambda += std::log(deriv);
            }
            lambda /= kSteps;

            ColorRgb c;
            if (lambda < 0.0f) {
                float val = std::clamp(-lambda * 0.85f, 0.0f, 1.0f);
                c = HslToRgb(200.0f + val * 65.0f + time * 25.0f, 0.96f, val * 0.55f + 0.1f);
            } else {
                float val = std::clamp(lambda * 0.55f, 0.0f, 1.0f);
                c = HslToRgb(15.0f + val * 45.0f + time * 38.0f, 0.96f, val * 0.6f + 0.2f);
            }
            int i = py * w + pxX;
            px[i].r = c.r; px[i].g = c.g; px[i].b = c.b;
        }
    }
}

// ---------------------------------------------------------------------------
// Barnsley Fern IFS Fractal with morphing affine matrices
// ---------------------------------------------------------------------------
void RenderBarnsleyLayer(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width;
    const int h = buffer.height;
    
    for (int i = 0; i < w * h; ++i) {
        px[i].r = static_cast<BYTE>(px[i].r >> 2);
        px[i].g = static_cast<BYTE>(px[i].g >> 2);
        px[i].b = static_cast<BYTE>(px[i].b >> 2);
    }

    float x = 0.0f, y = 0.0f;
    unsigned seed = static_cast<unsigned>(time * 1000.0f) ^ 0xACE1;
    const int numPoints = 26000;
    const float morph = std::sin(time * 0.5f) * 0.06f;

    for (int i = 0; i < numPoints; ++i) {
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        float r = static_cast<float>(seed % 10000) / 10000.0f;
        float nextX, nextY;

        if (r < 0.01f) {
            nextX = 0.0f;
            nextY = 0.16f * y;
        } else if (r < 0.86f) {
            nextX = 0.85f * x + (0.04f + morph) * y;
            nextY = (-0.04f - morph) * x + 0.85f * y + 1.6f;
        } else if (r < 0.93f) {
            nextX = 0.20f * x - 0.26f * y;
            nextY = 0.23f * x + 0.22f * y + 1.6f;
        } else {
            nextX = -0.15f * x + 0.28f * y;
            nextY = 0.26f * x + 0.24f * y + 0.44f;
        }
        x = nextX;
        y = nextY;

        int screenX = static_cast<int>(w * 0.5f + x * (w * 0.088f));
        int screenY = static_cast<int>(h * 0.95f - y * (h * 0.092f));

        if (screenX >= 0 && screenX < w && screenY >= 0 && screenY < h) {
            int idx = screenY * w + screenX;
            ColorRgb c = HslToRgb(time * 65.0f + y * 28.0f, 0.95f, 0.65f);
            px[idx].r = ClampByte(px[idx].r + c.r);
            px[idx].g = ClampByte(px[idx].g + c.g);
            px[idx].b = ClampByte(px[idx].b + c.b);
        }
    }
}

// ---------------------------------------------------------------------------
// Apollonian Gasket circle-inversion fractal
// ---------------------------------------------------------------------------
void RenderApollonianLayer(PixelBuffer& buffer, float time) {
    BgrPixel* px = buffer.bits();
    const int w = buffer.width;
    const int h = buffer.height;
    const float scale = 2.4f / static_cast<float>(std::min(w, h));
    const float rot = time * 0.2f;
    const float cosR = std::cos(rot), sinR = std::sin(rot);

    for (int py = 0; py < h; ++py) {
        for (int pxX = 0; pxX < w; ++pxX) {
            float x = (pxX - w * 0.5f) * scale;
            float y = (py - h * 0.5f) * scale;
            float rx = x * cosR - y * sinR;
            float ry = x * sinR + y * cosR;

            float dmin = 1e5f;
            float trap = 0.0f;
            constexpr int kIter = 9;

            for (int k = 0; k < kIter; ++k) {
                rx = -1.0f + 2.0f * std::fmod(std::abs(rx + 1.0f), 2.0f);
                ry = -1.0f + 2.0f * std::fmod(std::abs(ry + 1.0f), 2.0f);

                float r2 = rx * rx + ry * ry;
                if (r2 < 1e-4f) break;
                float kscale = 1.4f / r2;
                rx *= kscale;
                ry *= kscale;

                float dist = std::abs(std::sqrt(r2) - 0.7f);
                if (dist < dmin) {
                    dmin = dist;
                    trap = static_cast<float>(k);
                }
            }

            float hue = trap * 45.0f + time * 50.0f + dmin * 200.0f;
            float lum = std::clamp(1.0f - dmin * 3.5f, 0.05f, 0.95f);
            ColorRgb c = HslToRgb(hue, 0.95f, lum);
            int i = py * w + pxX;
            px[i].r = c.r; px[i].g = c.g; px[i].b = c.b;
        }
    }
}
