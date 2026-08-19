import cv2
import numpy as np
import mss
import time
import threading
import pyautogui
import os
import subprocess
import glob

def record_screen():
    with mss.mss() as sct:
        monitor = sct.monitors[1]
        width = monitor["width"]
        height = monitor["height"]
        
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        out = cv2.VideoWriter(r'D:\Test\AutoRename_Real_Demo.mp4', fourcc, 10.0, (width, height))
        
        start_time = time.time()
        while time.time() - start_time < 30:
            img = np.array(sct.grab(monitor))
            frame = cv2.cvtColor(img, cv2.COLOR_BGRA2BGR)
            out.write(frame)
            time.sleep(0.1)
            
        out.release()

# Hide everything before recording
pyautogui.hotkey('win', 'd')
time.sleep(1)

print("Starting screen recording...")
t = threading.Thread(target=record_screen)
t.start()

# Automate UI
subprocess.Popen(['explorer', r'D:\Test'])
time.sleep(4) # Let explorer open completely

pyautogui.hotkey('ctrl', 'a')
time.sleep(1)

files = glob.glob(r'D:\Test\*')
files = [f for f in files if not f.endswith('.mp4')]
subprocess.Popen([r'C:\Users\Vishwajit\Desktop\Auto File Rename App\build\Release\TestApp.exe'] + files)

time.sleep(25)
t.join()
print("Recording saved to D:\Test\AutoRename_Real_Demo.mp4")
