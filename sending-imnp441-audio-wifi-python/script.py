import ttkbootstrap as tb
from ttkbootstrap.window import Window
from tkinter import filedialog, messagebox, scrolledtext
import threading
import socket
import numpy as np
from scipy.io import wavfile

# ESP32 Configuration
SERVER_IP = "192.168.137.231"  # Change to your ESP32 IP
SERVER_PORT = 12345
BUFFER_LEN = 512
SAMPLE_RATE = 16000
RECORD_DURATION = 5  # 5 seconds

class AudioStreamerGUI:
    def __init__(self, master):
        self.master = master
        self.master.title("ESP32 Audio Recorder")
        self.master.geometry("600x400")
        self.master.resizable(True, True)

        self.style = tb.Style("darkly")  
        self.sock = None
        self.audio_data = []

        # GUI Layout
        self.connect_button = tb.Button(master, text="Connect to ESP32", bootstyle="primary", command=self.connect_to_esp32)
        self.connect_button.pack(pady=10, fill="x")

        self.record_button = tb.Button(master, text="Record 5s", bootstyle="success", command=self.start_recording, state="disabled")
        self.record_button.pack(pady=10, fill="x")

        self.save_button = tb.Button(master, text="Save Audio", bootstyle="warning", command=self.save_audio, state="disabled")
        self.save_button.pack(pady=10, fill="x")

        self.repeat_button = tb.Button(master, text="Repeat Recording", bootstyle="danger", command=self.repeat_recording, state="disabled")
        self.repeat_button.pack(pady=10, fill="x")

        self.status_label = tb.Label(master, text="Status: Not Connected", bootstyle="inverse")
        self.status_label.pack(pady=10)

        self.raw_data_label = tb.Label(master, text="Raw Audio Data:", bootstyle="info")
        self.raw_data_label.pack(pady=5)

        self.raw_data_text = scrolledtext.ScrolledText(master, width=80, height=10, wrap="none")
        self.raw_data_text.pack(pady=10, padx=10, fill="both", expand=True)

    def connect_to_esp32(self):
        """Establishes connection with ESP32."""
        self.status_label.config(text="Status: Connecting...")
        threading.Thread(target=self.establish_connection, daemon=True).start()

    def establish_connection(self):
        """Handles connection setup in a separate thread."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((SERVER_IP, SERVER_PORT))
            self.status_label.config(text="Status: Connected to ESP32")

            self.record_button.config(state="normal")  # Enable recording button

        except Exception as e:
            self.status_label.config(text="Status: Connection Failed")
            messagebox.showerror("Connection Error", str(e))

    def start_recording(self):
        """Records 5 seconds of audio from ESP32."""
        if not self.sock:
            messagebox.showerror("Error", "Connect to ESP32 first.")
            return

        self.status_label.config(text="Status: Recording 5s...")
        self.record_button.config(state="disabled")
        self.save_button.config(state="disabled")
        self.repeat_button.config(state="disabled")

        self.audio_data = []
        threading.Thread(target=self.receive_audio_data, daemon=True).start()

    def receive_audio_data(self):
        """Receives audio data from ESP32 for a fixed duration (5s)."""
        try:
            self.sock.sendall(b"record")  # Send start signal

            num_samples = SAMPLE_RATE * RECORD_DURATION  # Total samples for 5s
            received_samples = 0

            while received_samples < num_samples:
                data = self.sock.recv(BUFFER_LEN * 2)
                if not data:
                    break

                samples = np.frombuffer(data, dtype=np.int16)
                self.audio_data.extend(samples.tolist())
                received_samples += len(samples)

                # Update GUI
                self.raw_data_text.insert("end", f"{samples[:10]}\n")
                self.raw_data_text.see("end")

            self.status_label.config(text="Status: Recording Complete")
            self.save_button.config(state="normal")
            self.repeat_button.config(state="normal")

        except Exception as e:
            self.status_label.config(text="Status: Error during recording")
            messagebox.showerror("Recording Error", str(e))

    def save_audio(self):
        """Saves recorded audio as a WAV file."""
        if not self.audio_data:
            messagebox.showwarning("Warning", "No audio data to save.")
            return

        file_path = filedialog.asksaveasfilename(defaultextension=".wav", filetypes=[("Wave files", "*.wav")])
        if file_path:
            wavfile.write(file_path, SAMPLE_RATE, np.array(self.audio_data, dtype=np.int16))
            messagebox.showinfo("Success", f"Audio saved to {file_path}")

    def repeat_recording(self):
        """Clears previous recording and enables new recording."""
        self.audio_data = []
        self.raw_data_text.delete("1.0", "end")
        self.status_label.config(text="Status: Ready to Record")
        self.record_button.config(state="normal")
        self.save_button.config(state="disabled")
        self.repeat_button.config(state="disabled")

if __name__ == "__main__":
    root = Window(themename="darkly")
    app = AudioStreamerGUI(root)
    root.mainloop()
