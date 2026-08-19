import sys
import asyncio
import re

async def extract_text(image_path):
    from winrt.windows.media.ocr import OcrEngine
    from winrt.windows.graphics.imaging import BitmapDecoder
    from winrt.windows.storage import StorageFile
    
    file = await StorageFile.get_file_from_path_async(image_path)
    stream = await file.open_async(0) 
    decoder = await BitmapDecoder.create_async(stream)
    bitmap = await decoder.get_software_bitmap_async()
    engine = OcrEngine.try_create_from_user_profile_languages()
    result = await engine.recognize_async(bitmap)
    
    for line_obj in result.lines:
        print(f"RAW: {line_obj.text.strip()}")

if __name__ == '__main__':
    import os
    asyncio.run(extract_text(os.path.abspath(sys.argv[1])))
