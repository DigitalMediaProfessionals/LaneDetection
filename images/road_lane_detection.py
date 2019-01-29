import numpy as np
import cv2
from keras.models import load_model

model = load_model("LaneDetectionModel.h5")
image_bgr = cv2.imread("image.jpg")
image = image_bgr[:,:,::-1]
small_image = np.array(image_bgr)[None, :, :, :] / 255
prediction = model.predict(small_image)
print(prediction.shape, prediction.dtype)

result = (np.clip(prediction[0, :, :, 0], 0, 1) * 255).astype(np.uint8)
cv2.imwrite("result.png", result)
