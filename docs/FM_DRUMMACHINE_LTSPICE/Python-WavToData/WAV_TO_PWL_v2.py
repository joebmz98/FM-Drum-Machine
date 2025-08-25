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

# Normalize amplitude to [-1, 1]
amplitude = data / np.max(np.abs(data))

# Create time vector
duration = len(data) / rate
time = np.linspace(0, duration, len(data))

# Save as PWL format
with open('wav_input.txt', 'w') as f:
    for t, a in zip(time, amplitude):
        f.write(f"{t:.6f} {float(a):.6f}\n")

print("Conversion complete. Output saved to wav_input.txt.")
