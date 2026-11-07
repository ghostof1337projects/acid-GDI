#pragma once
#define NOMINMAX
#include <windows.h>
class ScreenBuffer;

// ---------------------------------------------------------------------------
// Primary effects: 54 distinct modes rotated by the Sequencer.
// Balanced progression from subtle GDI glitches -> iconic GDI malware payloads ->
// advanced GDI manipulations -> pure fractal universe -> dynamic trips -> peak meltdown.
// ---------------------------------------------------------------------------
enum class TripMode {
    // Tier 0 - 0% (Subtle GDI Glitches & Inversions)
    ColorInvert = 0,  // Sweeping DSTINVERT waves
    ScreenGlitch,     // Random DSTINVERT rects + jitter
    ScreenMelt,       // Classic GDI melting / dripping columns
    ChannelGhost,     // RGB channel ghost copies with SRCPAINT/SRCAND
    HorizTear,        // Horizontal scanline tears & displacement
    WaveDistort,      // Pure sine-wave horizontal row wobble
    ScreenEcho,       // Desktop echo / smear with SRCPAINT/SRCAND
    Checkerboard,     // Monoxide/Clinique pixel sieve checkerboard inverts

    // Tier 1 - 15% (Iconic GDI Malware Payloads & Geometry)
    ScreenTunnel,     // Infinite zoom inception into screen center
    ScreenShake,      // Violent screen vibration / earthquake with RGB fringe
    IconSpam,         // Swarm of classic bouncing / spiraling Windows system icons
    CursorStorm,      // MEMZ/Hydrus animated cursor swarm & Lissajous trails
    ErrorDialogCascade,// Cascading retro Windows error message boxes
    GridMirror,       // Multi-tile screen mirror & flip
    Pixelate,         // StretchBlt pixelation crunch pump
    StrobeMirror,     // Left-right mirror + strobe DSTINVERT flash
    RGBSplit,         // Chromatic aberration / RGB separation
    RadialRipples,    // Expanding concentric circular ripple inverts

    // Tier 2 - 30% (Advanced GDI Payloads, Slicing & Radar)
    SliceTear,        // Interlaced vertical & horizontal strip slicing
    DiagonalShear,    // Bismuth/Skynet diagonal parallelogram shears
    PieSliceInvert,   // Ursa/Hydrus rotating 360-degree radar pie slice inverts
    LogoBounce,       // Bouncing retro badges with color trails
    HatchStorm,       // Geometric PATINVERT hatch pattern sweeps
    PlgRotate,        // Wobbly PlgBlt parallelogram screen skew & rotation
    Plasma,           // Per-pixel smooth rainbow plasma shader
    Interference,     // Moiré wave interference patterns
    MatrixSpam,       // MEMZ-style cascading matrix alphanumeric rain

    // Tier 3 - 45% (Pure Fractal Universe)
    Mandelbrot,       // Mandelbrot deep zoom fractal
    JuliaFractal,     // Morphing Julia set with HSL color cycling
    NewtonFractal,    // Newton-Raphson cubic root basin boundaries
    Lyapunov,         // Chaotic population orbit density mapping
    BarnsleyFern,     // Barnsley Fern iterated function system (IFS)
    Apollonian,       // Apollonian Gasket recursive circle packing
    DragonCurve,      // Heighway Dragon recursive folding curve
    KochSnowflake,    // Recursive Koch snowflake geometric pulsation
    Sierpinski,       // Sierpinski recursive carpet fractal
    BurningShip,      // Burning Ship fractal trip

    // Tier 4 - 60% (Geometric & Dynamic Trips)
    Mandala,          // Multi-layered sacred geometric mandala
    WarpTunnel,       // Hypnotic multi-ring warping wormhole
    ZoomFeedback,     // Rotating zoom feedback with color inversion
    Helix,            // Glowing 3D double helix strand
    Mushrooms,        // Pulsing psychedelic breathing & shearing
    VortexSpin,       // PlgBlt vortex + glowing concentric rings
    Cube3D,           // Rotating 3D vector cube with XOR background
    Geosphere3D,      // Rotating 3D wireframe geosphere / icosahedron
    Acid,             // Heavy channel shifting, PATINVERT bands & ripple

    // Tier 5 - 75% (Peak Chaos & Total Meltdown)
    DrugRain,         // Digital melting rain with color trails
    RgbShader,        // HSL sweep and XOR pixel crunching
    XorStorm,         // Bytebeat XOR storm on pixel buffer + ROP blits
    ChromaBlast,      // Multi-pass high speed chromatic blits
    MeltChaos,        // Hyper-fast multi-directional dripping and tear blits
    Salvia,           // Tiled warping, polygon shearing, inverted color flips
    Datura,           // Vignette tunnel, ghost trails, deep color distortion
    TunnelDoom,       // Deep recursive 4-way tunnel with rotating PlgBlt

    Count             // Total effect count (54)
};

const wchar_t* TripModeName(TripMode mode);
void SetEffectIntensity(float t);

// Tier 0
void RunColorInvertEffect     (ScreenBuffer& b, float t);
void RunScreenGlitchEffect    (ScreenBuffer& b, float t);
void RunScreenMeltEffect      (ScreenBuffer& b, float t);
void RunChannelGhostEffect    (ScreenBuffer& b, float t);
void RunHorizTearEffect       (ScreenBuffer& b, float t);
void RunWaveDistortEffect     (ScreenBuffer& b, float t);
void RunScreenEchoEffect      (ScreenBuffer& b, float t);
void RunCheckerboardEffect    (ScreenBuffer& b, float t);

// Tier 1
void RunScreenTunnelEffect    (ScreenBuffer& b, float t);
void RunScreenShakeEffect     (ScreenBuffer& b, float t);
void RunIconSpamEffect        (ScreenBuffer& b, float t);
void RunCursorStormEffect     (ScreenBuffer& b, float t);
void RunErrorDialogCascadeEffect(ScreenBuffer& b, float t);
void RunGridMirrorEffect      (ScreenBuffer& b, float t);
void RunPixelateEffect        (ScreenBuffer& b, float t);
void RunStrobeMirrorEffect    (ScreenBuffer& b, float t);
void RunRGBSplitEffect        (ScreenBuffer& b, float t);
void RunRadialRipplesEffect   (ScreenBuffer& b, float t);

// Tier 2
void RunSliceTearEffect       (ScreenBuffer& b, float t);
void RunDiagonalShearEffect   (ScreenBuffer& b, float t);
void RunPieSliceInvertEffect  (ScreenBuffer& b, float t);
void RunLogoBounceEffect      (ScreenBuffer& b, float t);
void RunHatchStormEffect      (ScreenBuffer& b, float t);
void RunPlgRotateEffect       (ScreenBuffer& b, float t);
void RunPlasmaEffect          (ScreenBuffer& b, float t);
void RunInterferenceEffect    (ScreenBuffer& b, float t);
void RunMatrixSpamEffect      (ScreenBuffer& b, float t);

// Tier 3 - Fractals
void RunMandelbrotEffect      (ScreenBuffer& b, float t);
void RunJuliaFractalEffect    (ScreenBuffer& b, float t);
void RunNewtonFractalEffect   (ScreenBuffer& b, float t);
void RunLyapunovEffect        (ScreenBuffer& b, float t);
void RunBarnsleyFernEffect    (ScreenBuffer& b, float t);
void RunApollonianEffect      (ScreenBuffer& b, float t);
void RunDragonCurveEffect     (ScreenBuffer& b, float t);
void RunKochSnowflakeEffect   (ScreenBuffer& b, float t);
void RunSierpinskiEffect      (ScreenBuffer& b, float t);
void RunBurningShipEffect     (ScreenBuffer& b, float t);

// Tier 4
void RunMandalaEffect         (ScreenBuffer& b, float t);
void RunWarpTunnelEffect      (ScreenBuffer& b, float t);
void RunZoomFeedbackEffect    (ScreenBuffer& b, float t);
void RunHelixEffect           (ScreenBuffer& b, float t);
void RunMushroomEffect        (ScreenBuffer& b, float t);
void RunVortexSpinEffect      (ScreenBuffer& b, float t);
void RunCube3DEffect          (ScreenBuffer& b, float t);
void RunGeosphere3DEffect     (ScreenBuffer& b, float t);
void RunAcidEffect            (ScreenBuffer& b, float t);

// Tier 5
void RunDrugRainEffect        (ScreenBuffer& b, float t);
void RunRgbShaderEffect       (ScreenBuffer& b, float t);
void RunXorStormEffect        (ScreenBuffer& b, float t);
void RunChromaBlastEffect     (ScreenBuffer& b, float t);
void RunMeltChaosEffect       (ScreenBuffer& b, float t);
void RunSalviaEffect          (ScreenBuffer& b, float t);
void RunDaturaEffect          (ScreenBuffer& b, float t);
void RunTunnelDoomEffect      (ScreenBuffer& b, float t);

void RunEffect(TripMode mode, ScreenBuffer& buffer, float time);
void CleanupEffectResources();
