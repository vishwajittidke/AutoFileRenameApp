import asyncio
import os
import sys

async def test_face_detection():
    try:
        from winrt.windows.media.faceanalysis import FaceDetector
        from winrt.windows.graphics.imaging import BitmapDecoder, BitmapPixelFormat
        from winrt.windows.storage import StorageFile
    except ImportError as e:
        print("Import error:", e)
        return

    try:
        image_path = r"C:\Users\Vishwajit\Downloads\Photo.jpeg"
        file = await StorageFile.get_file_from_path_async(image_path)
        stream = await file.open_async(0)
        decoder = await BitmapDecoder.create_async(stream)
        
        # FaceDetector supports Nv12 or Gray8, or we can use SoftwareBitmap directly if the format is supported.
        # Let's get the standard software bitmap.
        bitmap = await decoder.get_software_bitmap_async()
        
        # We need to convert it to a supported format for FaceDetector (usually Nv12 or Gray8)
        from winrt.windows.graphics.imaging import SoftwareBitmap
        if not FaceDetector.is_bitmap_pixel_format_supported(bitmap.bitmap_pixel_format):
            # Try to convert to Nv12
            bitmap = SoftwareBitmap.convert(bitmap, BitmapPixelFormat.NV12)
            
        detector = await FaceDetector.create_async()
        faces = await detector.detect_faces_async(bitmap)
        
        if len(faces) > 0:
            print(f"Detected {len(faces)} face(s).")
        else:
            print("No faces detected.")
            
    except Exception as e:
        print("Error:", e)

if __name__ == '__main__':
    asyncio.run(test_face_detection())
