#pragma once
#include <cstdint>
#include <vector>

namespace CtrSfxMixer {
constexpr unsigned SampleRate=44100,EffectCount=32;
// Convert once on load; the audio callback only mixes prepared PCM.
void PreparePcm(std::vector<int16_t>& pcm,unsigned rate,unsigned channels);
void SetEffect(unsigned id,const int16_t* pcm,unsigned frames,unsigned channels,float gain);
void ResetVoices();
void Clear();
void Trigger(uint32_t mask);
void Render(int16_t* stereo,unsigned frames,int volume);
}
