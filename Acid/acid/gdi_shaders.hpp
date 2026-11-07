#pragma once

#include "pixel_buffer.hpp"
#include <cstdint>

struct ColorRgb { BYTE r, g, b; };

ColorRgb HueCycle(float phase);
ColorRgb HslToRgb(float h, float s, float l);

void ApplyXorFractal  (PixelBuffer& buffer, float time);
void ApplyHslShader   (PixelBuffer& buffer, float time);
void ApplyChannelSplit(PixelBuffer& buffer, int shiftR, int shiftB);

void RenderJuliaFractal      (PixelBuffer& buffer, float time, float opacity);
void RenderMandelbrotOverlay (PixelBuffer& buffer, float time, float zoom);
void RenderNewtonFractal     (PixelBuffer& buffer, float time, float zoom);
void RenderLyapunovFractal   (PixelBuffer& buffer, float time);
void RenderBarnsleyLayer     (PixelBuffer& buffer, float time);
void RenderApollonianLayer   (PixelBuffer& buffer, float time);
void RenderSierpinskiLayer   (PixelBuffer& buffer, float time);
void RenderPlasmaLayer       (PixelBuffer& buffer, float time);
void RenderBurningShipLayer  (PixelBuffer& buffer, float time);
void RenderInterferenceLayer (PixelBuffer& buffer, float time);
void ApplyKaleidoscope       (PixelBuffer& buffer, int segments, float time);

