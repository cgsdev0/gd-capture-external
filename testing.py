import os
os.environ["CUDA_VISIBLE_DEVICES"] = "0"  # Ensure CUDA GPU is used
os.environ["OPENCV_OPENGL"] = "1"  # Enable OpenGL acceleration

import cv2
import socket
import time
import struct
import mediapipe as mp

mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils

# Enable GPU with OpenGL
hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=2,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)

cap = cv2.VideoCapture(3)  # Use webcam

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while cap.isOpened():
    success, frame = cap.read()
    if not success:
        break

    frame = cv2.resize(frame, (640, 360), interpolation=cv2.INTER_AREA)
    # Convert to RGB
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    # Process with MediaPipe
    results = hands.process(frame_rgb)
    now = time.time()
    packet = bytearray()
    packet.extend(bytearray(struct.pack("d", now)))
    num_hands = len(results.multi_hand_landmarks) if results.multi_hand_landmarks else 0
    packet.extend(struct.pack("B", num_hands))  # uint8 for num_hands
    
    if results.multi_hand_landmarks:
        for i, hand_landmarks in enumerate(results.multi_hand_landmarks):
            hand_label = results.multi_handedness[i].classification[0].label  # "Left" or "Right"
            hand_id = 0 if hand_label == "Left" else 1
            packet.extend(struct.pack("B", hand_id)) 
            packet.extend(struct.pack("B", len(hand_landmarks.landmark)))  # uint8 for num_landmarks_per_hand
            for landmark in hand_landmarks.landmark:
                packet.extend(struct.pack("fff", landmark.x, landmark.y, landmark.z))

    sock.sendto(packet, ("127.0.0.1", 7033))
    #if results.multi_hand_landmarks:
    #    for hand_landmarks in results.multi_hand_landmarks:
    #        mp_drawing.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

    # cv2.imshow("Hand Tracking", frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()