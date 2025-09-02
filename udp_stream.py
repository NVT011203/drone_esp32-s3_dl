import io
import socket

import cv2
import numpy as np
from PIL import Image

UDP_IP = "0.0.0.0"
UDP_PORT = 1234
BUFFER_SIZE = 65536  # Kích thước bộ đệm (tối đa cho UDP)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print("Waiting for UDP packets...")

# Kích thước phóng to (ví dụ: gấp đôi 160x120 thành 320x240)
output_file = "stream.mp4"
fourcc = cv2.VideoWriter_fourcc(*"mp4v")
fps = 20
frame_size = (320, 240)
video_writer = cv2.VideoWriter(output_file, fourcc, fps, frame_size)

SCALE_FACTOR = 3
DISPLAY_WIDTH = 320 * SCALE_FACTOR
DISPLAY_HEIGHT = 240 * SCALE_FACTOR

buffer = bytearray()

print(f"UDP server running on {UDP_IP}: {UDP_PORT}")

try:
    while True:
        data, addr = sock.recvfrom(BUFFER_SIZE)
        buffer.extend(data)

        try:
            img = Image.open(io.BytesIO(buffer))
            frame = np.array(img)
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

            # Write to output_file
            video_writer.write(frame)

            # Resize hình ảnh
            frame = cv2.resize(
                frame, (DISPLAY_WIDTH, DISPLAY_HEIGHT), interpolation=cv2.INTER_LINEAR
            )

            cv2.imshow("Video Stream", frame)
            buffer = bytearray()
        except:
            continue

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
except KeyboardInterrupt:
    print("Stopping server...")
finally:
    video_writer.release()
    cv2.destroyAllWindows()
    sock.close()
    print(f"Video saved as {output_file}")
