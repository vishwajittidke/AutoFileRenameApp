import sys
import asyncio
import re
import os

# Suppress ONNX runtime logs
os.environ["ORT_MIN_LOG_LEVEL"] = "3"
os.environ["ORT_LOGGING_LEVEL"] = "3"

DOC_TYPES = {
    r'\baadhaar\b': "Aadhaar Card",
    r'\baadhar\b': "Aadhaar Card",
    r'\buidai\b': "Aadhaar Card",
    r'\b\d{4}\s\d{4}\s\d{4}\b': "Aadhaar Card",
    r'permanent account number': "PAN Card",
    r'income tax department': "PAN Card",
    r'\b[A-Z]{5}[0-9]{4}[A-Z]{1}\b': "PAN Card",
    r'driver license': "Driver License",
    r'driving license': "Driver License",
    r'passport': "Passport",
    r'boarding pass': "Boarding Pass",
    r'payment successful': "Payment Receipt",
    r'order id': "Order Receipt",
    r'invoice': "Invoice",
    r'tax invoice': "Tax Invoice",
    r'resume': "Resume",
    r'curriculum vitae': "Resume",
    r'bank statement': "Bank Statement",
    r'electricity bill': "Electricity Bill",
    r'certificate': "Certificate"
}

async def detect_face(image_path):
    try:
        from winrt.windows.media.faceanalysis import FaceDetector
        from winrt.windows.graphics.imaging import BitmapDecoder, SoftwareBitmap, BitmapPixelFormat
        from winrt.windows.storage import StorageFile
        
        file = await StorageFile.get_file_from_path_async(image_path)
        stream = await file.open_async(0)
        decoder = await BitmapDecoder.create_async(stream)
        bitmap = await decoder.get_software_bitmap_async()
        
        if not FaceDetector.is_bitmap_pixel_format_supported(bitmap.bitmap_pixel_format):
            bitmap = SoftwareBitmap.convert(bitmap, BitmapPixelFormat.NV12)
            
        detector = await FaceDetector.create_async()
        faces = await detector.detect_faces_async(bitmap)
        
        if len(faces) > 1:
            return "Group Photo"
        elif len(faces) == 1:
            return "Portrait"
    except Exception as e:
        pass
    return None

async def get_object_label(image_path):
    try:
        from winrt.windows.ai.machinelearning import LearningModel, LearningModelSession, LearningModelDevice, LearningModelDeviceKind, LearningModelBinding, TensorFloat
        from winrt.windows.storage import StorageFile
        import numpy as np
        from PIL import Image
        
        labels_path = os.path.abspath(os.path.join(os.path.dirname(__file__), 'imagenet_classes.txt'))
        model_path = os.path.abspath(os.path.join(os.path.dirname(__file__), 'mobilenetv2-7.onnx'))
        
        if not os.path.exists(model_path): return ""
        
        with open(labels_path, "r") as f:
            categories = [s.strip() for s in f.readlines()]
            
        file = await StorageFile.get_file_from_path_async(model_path)
        model = await LearningModel.load_from_storage_file_async(file)
        device = LearningModelDevice(LearningModelDeviceKind.CPU)
        session = LearningModelSession(model, device)
        
        img = Image.open(image_path).convert('RGB')
        img = img.resize((224, 224), Image.Resampling.BILINEAR)
        img_data = np.array(img).astype('float32')
        img_data = np.transpose(img_data, [2, 0, 1])
        img_data = np.expand_dims(img_data, axis=0)
        
        mean_vec = np.array([0.485, 0.456, 0.406]).reshape(1, 3, 1, 1).astype('float32')
        std_vec = np.array([0.229, 0.224, 0.225]).reshape(1, 3, 1, 1).astype('float32')
        img_data = (img_data / 255.0 - mean_vec) / std_vec
        
        flat_data = img_data.flatten().tolist()
        tensor = TensorFloat.create_from_iterable([1, 3, 224, 224], flat_data)
        
        binding = LearningModelBinding(session)
        binding.bind("data", tensor)
        
        result = await session.evaluate_async(binding, "Run")
        output_tensor = TensorFloat._from(result.outputs["mobilenetv20_output_flatten0_reshape0"])
        
        scores = np.array(list(output_tensor.get_as_vector_view()))
        class_idx = np.argmax(scores)
        
        if scores[class_idx] > 3.0:
            return categories[class_idx].replace('_', ' ').title()
    except Exception as e:
        pass
    return ""

async def extract_text(image_path):
    try:
        from winrt.windows.media.ocr import OcrEngine
        from winrt.windows.graphics.imaging import BitmapDecoder
        from winrt.windows.storage import StorageFile
    except ImportError:
        return

    text_found = False
    try:
        file = await StorageFile.get_file_from_path_async(image_path)
        stream = await file.open_async(0) 
        decoder = await BitmapDecoder.create_async(stream)
        bitmap = await decoder.get_software_bitmap_async()
        
        engine = OcrEngine.try_create_from_user_profile_languages()
        if engine:
            result = await engine.recognize_async(bitmap)
            full_text = result.text.replace('\n', ' ').replace('\r', ' ')
            
            doc_type = None
            for pattern, name in DOC_TYPES.items():
                if re.search(pattern, full_text, re.IGNORECASE):
                    doc_type = name
                    break
                    
            person_name = ""
            for line_obj in result.lines:
                line = line_obj.text.strip()
                if re.search(r'GOVT OF INDIA|INCOME TAX|GOVERNMENT|DEPARTMENT|REPUBLIC|Permanent Account|UIDAI|Aadhar|Aadhaar|Card|Name|Father|Certificate', line, re.IGNORECASE):
                    continue
                if len(line) < 3 or not re.search(r'[a-zA-Z]{3,}', line):
                    continue
                words = line.split()
                clean_words = []
                for w in words:
                    if re.search(r'\d{2,}', w) or sum(c.isdigit() for c in w) > 2:
                        break
                    clean_words.append(w)
                final_str = " ".join(clean_words).strip()
                if len(final_str) >= 3:
                    person_name = final_str[:50].strip()
                    break
                    
            if doc_type and person_name:
                if person_name.lower() in doc_type.lower() or doc_type.lower() in person_name.lower():
                    print("$$RESULT$$:" + doc_type)
                else:
                    print("$$RESULT$$:" + f"{doc_type} - {person_name}")
                text_found = True
            elif doc_type:
                print("$$RESULT$$:" + doc_type)
                text_found = True
            elif person_name:
                print("$$RESULT$$:" + person_name)
                text_found = True
    except Exception as e:
        pass
        
    if not text_found:
        face_type = await detect_face(image_path)
        if face_type:
            print("$$RESULT$$:" + face_type)
        else:
            label = await get_object_label(image_path)
            if label:
                print("$$RESULT$$:" + f"Photo of {label}")

if __name__ == '__main__':
    if len(sys.argv) < 2: sys.exit(1)
    abs_path = os.path.abspath(sys.argv[1])
    asyncio.run(extract_text(abs_path))
