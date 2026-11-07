#pragma once

#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

#include <atomic>
#include <cstdint>

#pragma comment(lib, "winmm.lib")

// Bytebeat audio engine - generates 8-bit PCM via waveOut at 8 kHz.
// Formulas are classic algorithmic / bytebeat patterns (Viznut lineage).
class BytebeatAudio {
public:
    static constexpr int kSampleRate    = 8000;
    static constexpr int kBufferSamples = 4096;
    static constexpr int kNumFormulas   = 8;

    bool init();
    void cleanup();
    void setFormula(int index);          // 0..kNumFormulas-1, wraps
    int  formulaCount() const { return kNumFormulas; }

private:
    static void CALLBACK WaveCallback(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
    void     fillBuffer(int bufIdx);
    uint8_t  generateSample(uint32_t t) const;

    HWAVEOUT              waveOut_  = nullptr;
    WAVEHDR               headers_[2] {};
    uint8_t               buffers_[2][kBufferSamples] {};
    std::atomic<uint32_t> tick_    { 0 };
    std::atomic<int>      formula_ { 0 };
};
