#include "audio.hpp"

uint8_t BytebeatAudio::generateSample(uint32_t t) const {
    const int f = formula_.load(std::memory_order_relaxed);
    uint32_t v = 0;
    switch (f) {
    case 0: v = (t * (t >> 5 | t >> 8)) >> (t >> 16);            break; // Viznut "Symphony"
    case 1: v = t & (t >> 8);                                     break; // classic bitcrush
    case 2: v = t * (((t >> 12) | (t >> 8)) & (63 & (t >> 4)));   break; // harmonic series
    case 3: v = ((t >> 1) ^ (t >> 4)) | (t >> 9);                 break; // xor shift
    case 4: v = t * (t >> 11 & t >> 9 & t >> 3 & t >> 6);         break; // dense mult
    case 5: v = (t >> 6) * (t ^ (t >> 11));                       break; // AM modulation
    case 6: v = (t * 5 & t >> 7) | (t * 3 & t >> 10);             break; // dual multiplier
    case 7: v = ((t >> 3) & 127) + ((t >> 5) & 127);              break; // two sawtooths
    default: v = 0; break;
    }
    return static_cast<uint8_t>(v & 0xFF);
}

void BytebeatAudio::fillBuffer(int bufIdx) {
    const uint32_t base = tick_.load(std::memory_order_relaxed);
    for (int i = 0; i < kBufferSamples; ++i)
        buffers_[bufIdx][i] = generateSample(base + static_cast<uint32_t>(i));
    tick_.fetch_add(static_cast<uint32_t>(kBufferSamples), std::memory_order_relaxed);

    headers_[bufIdx].dwFlags &= ~WHDR_DONE;
    waveOutWrite(waveOut_, &headers_[bufIdx], sizeof(WAVEHDR));
}

void CALLBACK BytebeatAudio::WaveCallback(HWAVEOUT, UINT msg,
    DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR) {
    if (msg != WOM_DONE) return;
    auto* self = reinterpret_cast<BytebeatAudio*>(instance);
    auto* hdr  = reinterpret_cast<WAVEHDR*>(param1);
    for (int i = 0; i < 2; ++i) {
        if (hdr == &self->headers_[i]) {
            self->fillBuffer(i);
            return;
        }
    }
}

bool BytebeatAudio::init() {
    WAVEFORMATEX wfx {};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 1;
    wfx.nSamplesPerSec  = kSampleRate;
    wfx.wBitsPerSample  = 8;
    wfx.nBlockAlign     = 1;
    wfx.nAvgBytesPerSec = kSampleRate;

    if (waveOutOpen(&waveOut_, WAVE_MAPPER, &wfx,
                    reinterpret_cast<DWORD_PTR>(&WaveCallback),
                    reinterpret_cast<DWORD_PTR>(this),
                    CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
        return false;

    for (int i = 0; i < 2; ++i) {
        headers_[i].lpData         = reinterpret_cast<LPSTR>(buffers_[i]);
        headers_[i].dwBufferLength = kBufferSamples;
        waveOutPrepareHeader(waveOut_, &headers_[i], sizeof(WAVEHDR));
    }
    fillBuffer(0);
    fillBuffer(1);
    return true;
}

void BytebeatAudio::cleanup() {
    if (!waveOut_) return;
    waveOutReset(waveOut_);
    for (int i = 0; i < 2; ++i)
        waveOutUnprepareHeader(waveOut_, &headers_[i], sizeof(WAVEHDR));
    waveOutClose(waveOut_);
    waveOut_ = nullptr;
}

void BytebeatAudio::setFormula(int index) {
    const int n = kNumFormulas;
    formula_.store(((index % n) + n) % n, std::memory_order_relaxed);
}
