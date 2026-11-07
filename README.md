Acid — GDI Visual Trip Simulator
A harmless Windows C++ demo that mimics classic GDI screen-distortion techniques (the kind sometimes seen in old malware samples) using only visual effects. It captures your desktop each frame and applies animated filters with BitBlt, PatBlt, StretchBlt, and PlgBlt.
This project does not:
Modify or delete files
Persist across reboots
Spread, encrypt, or exfiltrate data
Hook other processes
Press ESC at any time to exit instantly.
Effects & Progression
Acid features an intensity-driven sequencer that rotates through 54 distinct visual effects, seamlessly blending classic GDI malware payloads (MEMZ, Hydrus, NoEscape, Monoxide, Ursa, Bismuth, Skynet) with pure mathematical fractals and pixel shaders:
Tier 0 (0% - Subtle GDI Glitches & Inversions): Color Invert Waves, Screen Glitch, Screen Melt (Classic Dripping), Channel Ghosting, Horizontal Scanline Tears, Wave Distort (MEMZ Sine Wobble), Screen Echo, Checkerboard (Monoxide/Clinique Sieve Inverts).
Tier 1 (15% - Iconic GDI Malware Payloads & Geometry): Screen Tunnel (Inception Zoom), Screen Shake (Earthquake Vibration), Icon Spam (Bouncing/Orbiting Windows Icons), Cursor Storm (MEMZ Cursor Vortex Trails), Error Dialog Cascade (Retro 3D Error Box Swarm), Grid Mirror, Pixelate (Retro Crunch), Strobe Mirror, RGB Split, Radial Ripples.
Tier 2 (30% - Advanced GDI Payloads, Slicing & Radar): Slice Tear (Interlaced Slicing), Diagonal Shear (Bismuth Parallelogram Slices), Radar Pie Invert (Ursa 360° Fan Inversion), Logo Bounce (Bouncing Retro Badge Trails), Hatch Storm (PATINVERT Sweeps), PlgRotate (Wobbly Skew Rotation), Rainbow Plasma, Moiré Interference, Matrix Spam (MEMZ Falling Alphanumeric Code).
Tier 3 (45% - Pure Fractal Universe): Mandelbrot Zoom, Morphing Julia Set, Newton-Raphson Cubic Root Basins, Lyapunov Population Chaos, Barnsley Fern IFS, Apollonian Gasket Circle Packing, Heighway Dragon Curve, Koch Snowflake Pulsation, Sierpinski Carpet, Burning Ship.
Tier 4 (60% - Geometric & Dynamic Trips): Sacred Mandala, Warp Tunnel, Zoom Feedback, 3D Glowing Helix, Mushrooms Breathing, Vortex Spin, 3D Vector Cube, 3D Wireframe Geosphere (Hydrus Icosahedron), Acid Channel Shift.
Tier 5 (75% - Peak Chaos & Total Meltdown): Drug Rain, RGB HSL Shader, XOR Storm (Bytebeat Crunch), Chroma Blast, Melt Chaos, Salvia Warping, Datura Vignette Delirium, Tunnel Doom.
Controls
ESC — Quit immediately
Intensity ramps automatically over time, unlocking increasingly chaotic tiers of effects with synchronized 8-bit Bytebeat audio synthesis.
A HUD at the bottom displays the current effect name, countdown timer, and intensity progress bar.
Requirements
Windows 10/11
Visual Studio with C++ Desktop workload (toolset v145 / VS 2022)
Disclaimer
For educational and artistic use only. The fullscreen overlay can be disorienting — use in a safe environment and close with ESC when finished.
