import os
import subprocess

files = [
    r"C:\Users\Vishwajit\Downloads\File_20260819_175738.jpg",
    r"C:\Users\Vishwajit\Downloads\VT - Aadhar.jpeg",
    r"C:\Users\Vishwajit\Downloads\logo.png",
    r"C:\Users\Vishwajit\Downloads\Photo.jpeg",
    r"C:\Users\Vishwajit\Downloads\Vishwajit Tidke.png",
    r"C:\Users\Vishwajit\Downloads\appexchange.salesforce.com_consulting_page=14.png",
    r"C:\Users\Vishwajit\Downloads\IMG_20260429_045940_494.jpg",
    r"C:\Users\Vishwajit\Downloads\IMG_20260429_045944_768.jpg",
    r"C:\Users\Vishwajit\Downloads\Photo of Analog Clock.png",
    r"C:\Users\Vishwajit\Downloads\Flight.gif",
    r"C:\Users\Vishwajit\Downloads\image (1).png",
    r"C:\Users\Vishwajit\Downloads\Flight Management 2.png",
    r"C:\Users\Vishwajit\Downloads\Flight Management Data Model.png",
    r"C:\Users\Vishwajit\Downloads\20260309_110742.jpg",
    r"C:\Users\Vishwajit\Downloads\bank.jpg",
    r"C:\Users\Vishwajit\Downloads\institute.png",
    r"C:\Users\Vishwajit\Downloads\Screenshot_26-2-2026_95330_app.zoom.us.jpeg",
    r"C:\Users\Vishwajit\Downloads\Screenshot_28-1-2026_24944_localhost.jpeg",
    r"C:\Users\Vishwajit\Downloads\20251206_190802.jpg",
    r"C:\Users\Vishwajit\Downloads\Screenshot_28-11-2025_20858_affiliate-program.amazon.com.jpeg",
    r"C:\Users\Vishwajit\Downloads\Pan Card ID_1.jpg",
    r"C:\Users\Vishwajit\Downloads\ID Card.png"
]

print("| Original Name | Renamed Name | Status |")
print("|---|---|---|")

test_app = r"build\Release\TestApp.exe"

for file in files:
    if not os.path.exists(file):
        print(f"| {os.path.basename(file)} | N/A | File Missing |")
        continue
        
    try:
        proc = subprocess.run([test_app, file], capture_output=True, text=True, check=True)
        out = proc.stdout
        
        # Look for "Result: [new path]"
        new_path = ""
        for line in out.splitlines():
            if line.startswith("Result: "):
                new_path = line.replace("Result: ", "").strip()
                break
                
        if new_path and new_path != file and os.path.exists(new_path):
            new_name = os.path.basename(new_path)
            # Revert back
            os.rename(new_path, file)
            print(f"| {os.path.basename(file)} | {new_name} | Success & Reverted |")
        else:
            print(f"| {os.path.basename(file)} | N/A | Failed to Rename |")
    except Exception as e:
        print(f"| {os.path.basename(file)} | ERROR | {e} |")
