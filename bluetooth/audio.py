import pyaudio
import sys

# Audio Configuration
SAMPLE_RATE = 16000
CHANNELS = 1
FORMAT = pyaudio.paInt16
CHUNK = 1024  # Adjust for latency

def find_bluetooth_device(p):
    print("\nAvailable audio input devices:")
    for i in range(p.get_device_count()):
        dev = p.get_device_info_by_index(i)
        if dev['maxInputChannels'] > 0:
            print(f"{i}: {dev['name']} (inputs: {dev['maxInputChannels']})")
    
    while True:
        try:
            index = int(input("Enter Bluetooth audio input device index: "))
            dev = p.get_device_info_by_index(index)
            if dev['maxInputChannels'] > 0:
                return index
            print("Device has no input channels. Try again.")
        except ValueError:
            print("Please enter a valid number.")

def main():
    p = pyaudio.PyAudio()
    device_index = find_bluetooth_device(p)
    
    stream = p.open(
        format=FORMAT,
        channels=CHANNELS,
        rate=SAMPLE_RATE,
        input=True,
        input_device_index=device_index,
        frames_per_buffer=CHUNK
    )
    
    print("\nLive audio streaming started - Press Ctrl+C to stop")
    
    try:
        while True:
            data = stream.read(CHUNK, exception_on_overflow=False)            
    except KeyboardInterrupt:
        print("\nStopping stream...")
        stream.stop_stream()
        stream.close()
        p.terminate()
        print("Done.")

if __name__ == "__main__":
    main()