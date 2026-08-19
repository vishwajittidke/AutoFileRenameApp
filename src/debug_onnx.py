import sys
import os
import onnxruntime as ort
import numpy as np
from PIL import Image

image_path = os.path.abspath(sys.argv[1])
model_path = os.path.join(os.path.dirname(__file__), 'mobilenetv2-7.onnx')
labels_path = os.path.join(os.path.dirname(__file__), 'imagenet_classes.txt')

with open(labels_path, 'r') as f:
    categories = [s.strip() for s in f.readlines()]

session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
input_name = session.get_inputs()[0].name

img = Image.open(image_path).convert('RGB')
img = img.resize((224, 224), Image.Resampling.BILINEAR)
img_data = np.array(img).astype('float32')
img_data = np.transpose(img_data, [2, 0, 1])
img_data = np.expand_dims(img_data, axis=0)

mean_vec = np.array([0.485, 0.456, 0.406]).reshape(1, 3, 1, 1).astype('float32')
std_vec = np.array([0.229, 0.224, 0.225]).reshape(1, 3, 1, 1).astype('float32')
img_data = (img_data / 255.0 - mean_vec) / std_vec

result = session.run(None, {input_name: img_data})
scores = result[0].squeeze()
class_idx = np.argmax(scores)

print(f"Top prediction: {categories[class_idx]}, Score: {scores[class_idx]}")
