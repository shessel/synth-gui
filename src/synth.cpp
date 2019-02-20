#include "synth.h"

#include <cstdint>
#include <cmath>
#include <thread>
#include <deque>

#include <Windows.h>

static constexpr float PI = 3.14159265f;

static constexpr uint32_t SAMPLE_RATE = 44100;

static constexpr size_t BPM = 120;
static constexpr size_t MEASURES = 1;
static constexpr size_t METER_IN_4TH = 1;
static constexpr float BEAT_DURATION_SEC = 60.0 / BPM;
static constexpr size_t BEATS_TOTAL = MEASURES * METER_IN_4TH;
static constexpr float DURATION_SEC = BEATS_TOTAL * BEAT_DURATION_SEC;
static constexpr size_t NUM_CHANNELS = 2;
static constexpr size_t BYTE_PER_SAMPLE = 2;
static constexpr size_t BITS_PER_SAMPLE = BYTE_PER_SAMPLE * 8;
static constexpr size_t SAMPLE_COUNT = static_cast<size_t>(SAMPLE_RATE * NUM_CHANNELS * DURATION_SEC);
static constexpr size_t SAMPLE_OFFSET = 0;// 44 / BYTE_PER_SAMPLE; // WAV_HEADER is 44 bytes, so 22 uint16 elements
static constexpr size_t BUFFER_COUNT = SAMPLE_OFFSET + SAMPLE_COUNT;

static float noise_buffer[SAMPLE_RATE];

// platform is little-endian, so least significant byte comes first in memory
// => bytes are reversed when specifying multiple bytes as one int
static constexpr uint32_t WAV_HEADER[] = {
    // Main chunk
    0x46464952, // ChunkId = "RIFF"
    BUFFER_COUNT * BYTE_PER_SAMPLE - 8,// ChunkSize, size of the main chunk, excluding the first two 4-byte fields
    0x45564157, // Format = "WAVE"
    // WAVE format has 2 sub chunks, fmt and data

    // Sub chunk fmt
    0x20746d66, // SubChunkId = "fmt "
    16, // SubChunkSize, size of sub chunk, excluding the first two 4-byte fields
    NUM_CHANNELS << 16 | 1, // 2 bytes AudioFormat = 1 for PCM, 2 bytes NumChannels,
    SAMPLE_RATE, // SampleRate
    SAMPLE_RATE * NUM_CHANNELS * BYTE_PER_SAMPLE, // ByteRate = SampleRate * NumChannels * BytesPerSample
    BITS_PER_SAMPLE << 16 | NUM_CHANNELS * BYTE_PER_SAMPLE, // 2 bytes BlockAlign = NumChannels * BytesPerSample, 2 bytes BitsPerSample

    // Sub chunk data
    0x61746164, // SubChunkId = "data"
    SAMPLE_COUNT * BYTE_PER_SAMPLE,// SubChunkSize, size of sub chunk, excluding the first two 4-byte fields
};

float square(float t, float frequency, float flip = 0.5f)
{
    float dummy;
    return std::modff(t * frequency, &dummy) < flip ? -1.0f : 1.0f;
}

float sin(float t, float frequency)
{
    return std::sinf(t * frequency * PI * 2.0f);
}

float rand(float x)
{
    float dummy;
    return std::modff(std::sin(x*2357911.13f)*1113171.9f, &dummy);
}

void initNoiseBuffer()
{
    static bool initialized = false;
    if (!initialized) {
        std::srand(42);
    }

    for (size_t i = 0; i < SAMPLE_RATE; ++i)
    {
        noise_buffer[i] = -1.0f + 2.0f * (static_cast<float>(std::rand()) / RAND_MAX);
    }

    initialized = true;
}

float noise(float t, float freq)
{
    //static bool initialized = false;
    //if (!initialized) {
    //    std::srand(42);
    //    initialized = true;
    //}
    //return -1.0f + 2.0f * (static_cast<float>(std::rand()) / RAND_MAX);
//    float dummy;
//    float frac = std::modff(t * freq, &dummy);
    return noise_buffer[static_cast<size_t>(t * freq)];
}

float envSqrt(float period)
{
    period = period < 0.5 ? period : 1.0f - period;
    return sqrtf(2.0f * period);
}

float envSq(float period)
{
    period = period < 0.5 ? period : 1.0f - period;
    return period * period * 4.0f;
}

float envAdsr(float period, float attack, float decay, float sustainLevel, float release)
{
    if (period <= attack)
    {
        return period / attack;
    }
    else if (period <= decay)
    {
        return 1.0f - (1.0f - sustainLevel) * ((period - attack) / (decay - attack));
    }
    else if (period <= release)
    {
        return sustainLevel;
    }
    else
    {
        return sustainLevel - sustainLevel * (period - release) / (1.0f - release);
    }
}

template <typename T>
void clamp(T& x, const T& min, const T& max)
{
    x = x > max ? max : x;
    x = x < min ? min : x;
}

float compress(float x, float threshold, float reduction)
{
    static float peak = 0.0f;
    peak = std::fmaxf(std::fabsf(x), peak);
    x = peak > threshold ? x * (1.0f / reduction) : x;
    peak -= 1.0f / (2.0f * 44100.0f);
    return x;
}

float interpolateLin(float v0, float v1, float x, float x0 = 0.0, float x1 = 1.0)
{
    return v0 + (v1 - v0) * (x - x0) / (x1 - x0);
}

float kick(float t, float /*startFreq*/)
{
    // add noise?
    float env = std::expf(-1.5f*t);
    float freqFalloff = std::expf(-0.45f*t);
    // float lowBoom = env * square(t, 40.0f * freqFalloff);
    float lowBoom = env * std::sinf(2.0f * PI * 60.0f * freqFalloff);

    float punchT = t * 400.0f;
    float punchFalloff = std::expf(-0.95f*punchT);
    clamp(punchT, 0.0f, 1.0f);
    float punch = punchFalloff * noise(t, 240.0) * 0.7f;

    float slapT = t * 400.0f;
    float slapFalloff = std::expf(-0.25f*slapT);
    clamp(slapT, 0.0f, 1.0f);
    float slap = slapFalloff * noise(t, 5000.0f) * 0.3f;
    return 1.0f * (lowBoom + punch + slap);
}

std::deque<WAVEHDR> hdrs;

LPSTR current_buffer = nullptr;
HWAVEOUT hwo = 0;
void CALLBACK waveOutProc(
    HWAVEOUT  hwoo,
    UINT      uMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
    ) {
    switch (uMsg)
    {
    case WOM_CLOSE:
        uMsg *= 1;
        break;
    case WOM_OPEN:
        uMsg *= 1;
        break;
    case WOM_DONE:
        if (!hdrs.empty()) {
            WAVEHDR& hdr = hdrs.front();
            waveOutUnprepareHeader(hwo, &hdr, sizeof(WAVEHDR));
            hdrs.pop_front();
        }
        uMsg *= 1;
        break;
    default:
        uMsg *= 1;
        uMsg;
    }
    hwoo;
    uMsg;
    dwInstance;
    dwParam1;
    dwParam2;
};

void synth_init()
{
    initNoiseBuffer();
    WAVEFORMATEX fmt{};
    fmt.wBitsPerSample = 16;
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = NUM_CHANNELS;
    fmt.nSamplesPerSec = SAMPLE_RATE;
    fmt.nAvgBytesPerSec = SAMPLE_RATE * NUM_CHANNELS * BYTE_PER_SAMPLE;
    fmt.nBlockAlign = NUM_CHANNELS * BYTE_PER_SAMPLE;

    synth_generate({ 0.01f, 0.2f, 1.0f, 0.0f });
    waveOutOpen(&hwo, WAVE_MAPPER, &fmt, (DWORD_PTR) &waveOutProc, NULL, CALLBACK_FUNCTION);
    //waveOutClose(hwo);
}

int16_t* buffer;

void synth_generate(ADSR adsr)
{
    if (!buffer) {
        buffer = new int16_t[BUFFER_COUNT];
    }
    //memcpy(buffer, WAV_HEADER, 44);

    float frequency = 440.0;
    float dummy;
    for (size_t i = SAMPLE_OFFSET; i < BUFFER_COUNT; ++i) {
        //int channel = i & 1;
        float t = 0.5f * static_cast<float>(i - SAMPLE_OFFSET) / SAMPLE_RATE;
        float period = std::modff(t / BEAT_DURATION_SEC, &dummy);
        frequency = 220.0f * pow(1.0594631f, dummy);
        //float env = envAdsr(period, .1f, .3f, .2f, .7f);
        float env = envAdsr(period, adsr.a, adsr.d, adsr.s, adsr.r);
        float level = (1.0f / 3.0f);// *(0.6f + 0.4f * std::sinf(0.5f * t * PI + channel * PI));
        float sample = level * env * kick(period, 60.0f);
        //sample += level * env * kick(std::modf(period + 0.25f, &dummy), 60.0f);
        //sample += level * env * kick(std::modf(period + 0.5f, &dummy), 60.0f);
        //sample += level * env * kick(std::modf(period + 0.75f, &dummy), 60.0f);
        //sample += level * env * sin(t, frequency*pow(1.0594631f, 4));
        // sample += level * 0.25 * noise(period, 20.0f);
        //sample += level * env * sin(t, frequency*pow(1.0594631f, 4));
        //sample += level * env * sin(t, frequency*pow(1.0594631f, 7));
        //sample += level * env * sin(t, frequency*pow(1.0594631f, 10));
        //sample = compress(sample, 0.8f, 4.0f);
        buffer[i] = static_cast<int16_t>(sample * 32767);
        clamp(buffer[i], int16_t(-32767), int16_t(32767));
    }

    current_buffer = reinterpret_cast<LPSTR>(buffer);
}

void synth_play()
{
    hdrs.emplace_back();
    WAVEHDR& hdr = hdrs.back();// { current_buffer, BUFFER_COUNT / 8, 0, 0, 0, 0, 0, 0 };
    hdr.dwLoops = static_cast<DWORD>(-1);
    hdr.lpData = current_buffer;
    hdr.dwBufferLength = BUFFER_COUNT*sizeof(int16_t);
    hdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
    waveOutPrepareHeader(hwo, &hdr, sizeof(WAVEHDR));
    //res *= 1;
    //
    //char* test = new char[hdr.dwBufferLength];
    //memcpy(test, current_buffer, hdr.dwBufferLength);
    //delete[] test;
    //
    //while ((hdr.dwFlags & WHDR_PREPARED) == 0)
    //    Sleep(10);
    //
    waveOutWrite(hwo, &hdr, sizeof(WAVEHDR));
    //waveOutUnprepareHeader(hwo, &hdr, sizeof(WAVEHDR));

    //WAVEHDR header = { current_buffer, BUFFER_COUNT * 2, 0, 0, 0, 0, 0, 0 };
    //waveOutPrepareHeader(hwo, &header, sizeof(WAVEHDR));
    //waveOutWrite(hwo, &header, sizeof(WAVEHDR));
    //static bool play = true;
    //if (play)
    //{
    //    PlaySound(current_buffer, nullptr, SND_MEMORY | SND_ASYNC | SND_LOOP);
    //    play = false;
    //}
    //else
    //{
    //    PlaySound(nullptr, nullptr, SND_MEMORY | SND_ASYNC | SND_LOOP);
    //    play = true;
    //}
}