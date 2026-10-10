#pragma once
namespace CtrAudio {
// Counts are stereo frames, except MixSamples (individual 16-bit samples).
inline constexpr unsigned DeviceFrames=512;
inline constexpr unsigned MixSamples=DeviceFrames*2;
inline constexpr unsigned QueueBytes=DeviceFrames*2*2*2;
}
