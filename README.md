# Acid — GDI Visual Trip Simulator

Acid is a Windows C++20 visual-effects demo built with the Win32 GDI. It captures the primary monitor, applies a rotating sequence of screen distortions and generative visuals, and adds procedural 8-bit audio. Intensity increases over time, gradually unlocking more effects.

> [!WARNING]
> Acid draws directly over the **primary monitor's desktop** and can produce fast flashes, distortion, and disorienting visuals. Save your work and use it only in a safe environment. Press **Esc** to stop it; avoid running it where unexpected screen effects could disrupt other people.

## What it does

- Rotates through 54 visual effects, from color inversions and screen waves to fractals, geometric animations, and image distortions.
- Increases intensity over approximately 2.5 minutes, unlocking effect tiers at 15%, 30%, 45%, 60%, and 75%.
- Generates 8-bit mono Bytebeat audio and changes its formula as effects advance.
- Displays the current effect, a countdown, and intensity progress on the screen.
- Targets the primary monitor; it does not create a conventional application window.

### Effect tiers

| Intensity | Theme | Examples |
| --- | --- | --- |
| 0% | Glitches and inversions | Color invert, screen melt, channel ghosting, wave distortion, checkerboard |
| 15% | Geometry and motion | Screen tunnel, shake, cursor trails, pixelation, RGB split, ripples |
| 30% | Slices and patterns | Slice tear, diagonal shear, plasma, interference, matrix rain |
| 45% | Fractals | Mandelbrot, Julia, Newton, Barnsley fern, Dragon curve, Burning Ship |
| 60% | Geometric animation | Mandala, warp tunnel, zoom feedback, helix, vortex, 3D cube |
| 75% | High-intensity effects | XOR storm, chroma blast, melt chaos, Salvia, Datura, Tunnel Doom |

## Controls

- **Esc** — quit immediately. The program checks this key regardless of which window is focused.
- Effects advance automatically; intensity and the HUD update as the demo runs.

## Build

### Requirements

- Windows 10 or 11
- Visual Studio with the **Desktop development with C++** workload
- The **v145** MSVC toolset and a Windows 10 SDK

### Steps

1. Clone or download this repository.
2. Open [`Acid/acid/acid.slnx`](Acid/acid/acid.slnx) in Visual Studio. If your Visual Studio installation does not support `.slnx`, open [`Acid/acid/acid.vcxproj`](Acid/acid/acid.vcxproj) instead.
3. Select a configuration (such as **Release**) and platform (**x64** or **Win32**), then build the `acid` project.
4. Run the built application from Visual Studio or the output directory.

The project uses the C++20 language standard and links against the Windows multimedia API for audio.

## Safety and privacy

Acid is a visual demo, not a screen overlay window: it captures and redraws the primary monitor's desktop while running. Its source does not include code to modify or delete files, install persistence, spread, encrypt or exfiltrate data, or hook other processes. It does poll the system-wide Escape key state so you can exit even when another window has focus. As with any program that captures and redraws a display, run it only when you understand and accept its behavior.

## Disclaimer

For educational and artistic use. The visual effects may be uncomfortable or disorienting, especially for people sensitive to flashing images or motion. Stop the program with **Esc** if you feel unwell.
