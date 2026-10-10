#include "SfxMixer.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr unsigned RampFrames=32,Taps=16,Phases=256;
struct Effect {const int16_t* pcm=nullptr;unsigned frames=0,channels=0;int gain=0;};
struct Voice {unsigned position=0,tail=0,ramp=0;bool playing=false;};
Effect effects[CtrSfxMixer::EffectCount];
Voice voices[CtrSfxMixer::EffectCount];
constexpr unsigned Chunk=256;
int32_t mixed[Chunk*2]; // Single callback owner; keep off NDSP's 4 KiB stack.

int Sample(const Effect& effect,unsigned frame,unsigned channel) {
 return frame<effect.frames?effect.pcm[frame*effect.channels+channel]:0;
}
}

void CtrSfxMixer::PreparePcm(std::vector<int16_t>& pcm,unsigned rate,unsigned channels) {
 if(rate==SampleRate)return;
 // A windowed-sinc polyphase resampler avoids doing sample-rate conversion
 // per voice in the callback. Coefficients are shared across this whole sample.
 std::array<std::array<float,Taps>,Phases> filters{};
 constexpr double pi=3.14159265358979323846;
 const double cutoff=std::min(1.,double(SampleRate)/rate);
 for(unsigned phase=0;phase<Phases;phase++) {
  double sum=0;
  for(unsigned tap=0;tap<Taps;tap++) {
   const double x=int(tap)-int(Taps/2-1)-double(phase)/Phases;
   const double sinc=std::abs(x)<1.e-9?cutoff:std::sin(pi*x*cutoff)/(pi*x);
   const double window=.5+.5*std::cos(pi*x/(Taps/2));
   filters[phase][tap]=float(sinc*window);sum+=filters[phase][tap];
  }
  for(auto& coefficient:filters[phase])coefficient=float(coefficient/sum);
 }
 const unsigned sourceFrames=pcm.size()/channels;
 const unsigned frames=(uint64_t(sourceFrames)*SampleRate+rate-1)/rate;
 std::vector<int16_t> output(size_t(frames)*channels);
 for(unsigned frame=0;frame<frames;frame++) {
  const uint64_t position=uint64_t(frame)*rate;
  const int center=position/SampleRate;
  const auto& filter=filters[(position%SampleRate)*Phases/SampleRate];
  for(unsigned channel=0;channel<channels;channel++) {
   float value=0;
   for(unsigned tap=0;tap<Taps;tap++) {
    const int index=center+int(tap)-int(Taps/2-1);
    if(index>=0 && index<int(sourceFrames))value+=pcm[size_t(index)*channels+channel]*filter[tap];
   }
   output[size_t(frame)*channels+channel]=int16_t(std::clamp(std::lround(value),-32768l,32767l));
  }
 }
 pcm.swap(output);
}

void CtrSfxMixer::SetEffect(unsigned id,const int16_t* pcm,unsigned frames,unsigned channels,float gain) {
 effects[id]={pcm,frames,channels,int(std::lround(gain*32768.f))};
}
void CtrSfxMixer::ResetVoices() {for(auto& voice:voices)voice={};}
void CtrSfxMixer::Clear() {ResetVoices();for(auto& effect:effects)effect={};}
void CtrSfxMixer::Trigger(uint32_t mask) {
 for(unsigned id=0;id<EffectCount;id++)if((mask&(1u<<id)) && effects[id].pcm) {
  auto& voice=voices[id];
  // Preserve the original one voice per effect/restart semantics, with a
  // sub-millisecond crossfade instead of a discontinuous cursor jump.
  voice.tail=voice.position;voice.ramp=voice.playing?RampFrames:0;
  voice.position=0;voice.playing=true;
 }
}
void CtrSfxMixer::Render(int16_t* stereo,unsigned frames,int volume) {
 // Accumulate in 32 bits and clip once, preserving individual gains without
 // reducing volume as more voices join. Storage is fixed and never allocates.
 for(unsigned offset=0;offset<frames;offset+=Chunk) {
  const unsigned count=std::min(Chunk,frames-offset);
  std::fill(std::begin(mixed),std::end(mixed),0);
  for(unsigned id=0;id<EffectCount;id++) {
   auto& voice=voices[id];if(!voice.playing)continue;
   const auto& effect=effects[id];
   const unsigned length=std::min(count,effect.frames-voice.position);
   for(unsigned frame=0;frame<length;frame++) {
    int left=Sample(effect,voice.position,0);
    int right=effect.channels==1?left:Sample(effect,voice.position,1);
    if(voice.ramp) {
     const int oldLeft=Sample(effect,voice.tail,0);
     const int oldRight=effect.channels==1?oldLeft:Sample(effect,voice.tail,1);
     left=(left*int(RampFrames-voice.ramp)+oldLeft*int(voice.ramp))/int(RampFrames);
     right=(right*int(RampFrames-voice.ramp)+oldRight*int(voice.ramp))/int(RampFrames);
     ++voice.tail;--voice.ramp;
    }
    mixed[frame*2]+=(left*effect.gain)>>15;
    mixed[frame*2+1]+=(right*effect.gain)>>15;
    ++voice.position;
   }
   if(voice.position==effect.frames)voice.playing=false;
  }
  for(unsigned sample=0;sample<count*2;sample++) {
   const int value=mixed[sample]*volume/100;
   stereo[offset*2+sample]=int16_t(std::clamp(value,-32768,32767));
  }
 }
}
