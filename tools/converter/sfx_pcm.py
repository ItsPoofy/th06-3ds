"""Prepare SFX at the software mixer's rate before copying them to the SD card."""
import struct
import numpy as np

RATE = 44100


def encode(pcm, rate, channels):
    samples = np.frombuffer(pcm, dtype='<i2').reshape(-1, channels)
    if rate != RATE:
        taps, phases = 16, 256
        x = np.arange(taps)[None, :] - (taps // 2 - 1) - np.arange(phases)[:, None] / phases
        cutoff = min(1., RATE / rate)
        filters = (cutoff * np.sinc(x * cutoff) * (.5 + .5 * np.cos(np.pi * x / (taps / 2))))
        filters = (filters / filters.sum(axis=1, keepdims=True)).astype(np.float32)
        frames = (len(samples) * RATE + rate - 1) // rate
        positions = np.arange(frames, dtype=np.int64) * rate
        indices = positions[:, None] // RATE + np.arange(taps) - (taps // 2 - 1)
        valid = (indices >= 0) & (indices < len(samples))
        values = samples[np.clip(indices, 0, len(samples) - 1)].astype(np.float32)
        values *= valid[:, :, None]
        weights = filters[(positions % RATE) * phases // RATE]
        output = (values * weights[:, :, None]).sum(axis=1)
        rounded = np.sign(output) * np.floor(np.abs(output) + .5)
        samples = np.clip(rounded, -32768, 32767).astype('<i2')
    return struct.pack('<4sIII', b'PCM3', RATE, channels, len(samples)) + samples.tobytes()
