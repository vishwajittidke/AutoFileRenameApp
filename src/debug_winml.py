import sys
import asyncio
import re
import os

async def test():
    from winrt.windows.ai.machinelearning import LearningModel, LearningModelSession, LearningModelDevice, LearningModelDeviceKind, LearningModelBinding, TensorFloat
    from winrt.windows.storage import StorageFile
    import numpy as np
    from PIL import Image
    
    image_path = os.path.abspath(sys.argv[1])
    labels_path = os.path.join(os.path.dirname(__file__), 'imagenet_classes.txt')
    model_path = os.path.join(os.path.dirname(__file__), 'mobilenetv2-7.onnx')
    
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
    
    print(f"Top class: {categories[class_idx]}, Score: {scores[class_idx]}")

if __name__ == '__main__':
    asyncio.run(test())
