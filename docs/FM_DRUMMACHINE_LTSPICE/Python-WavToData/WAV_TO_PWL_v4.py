import numpy as np
import scipy.io.wavfile as wav
import warnings

# Suppress warnings from scipy wav reader
warnings.simplefilter("ignore", wav.WavFileWarning)

# Load WAV file
rate, data = wav.read('drumloop.wav')

# Convert stereo to mono if needed
if len(data.shape) == 2:
    data = data.mean(axis=1)

# Trim to first 2 seconds
max_samples = int(rate * 2)
data = data[:max_samples]

# Downsample by keeping every 100th sample
data = data[::100]

# Normalize amplitude to [-1, 1]
amplitude = data / np.max(np.abs(data))

# Scale to simulate line-level audio (~1 Vrms ≈ 1.4 V peak)
amplitude *= 1.4

# Create time vector for downsampled data
time = np.linspace(0, 2, len(amplitude))

# Save as PWL format
with open('wav_input_line_level_2s_downsampled.txt', 'w') as f:
    for t, a in zip(time, amplitude):
        f.write(f"{t:.6f} {float(a):.6f}\n")

print("Conversion complete. Output saved to wav_input_line_level_2s_downsampled.txt.")
