import struct
import select

import tensorflow as tf
import tensorflow_hub as hub
import os

thresh = 0.5

module_handle = os.getcwd()
detector = hub.load(module_handle).signatures['default']

while 1:
	try:
		fin.close()
		fout.close()
	except Exception:
		print("OK")

	try:
		fin=open("./in", 'rb')
		fout=open("./out", 'wb')
	except Exception:
		fin.close()
		fout.close()
		exit(0)

	try:
		volume=struct.unpack('Q', fin.read(8))[0]
		data=fin.read(volume)
		img = tf.image.decode_jpeg(data, channels=3)
		converted_img  = tf.image.convert_image_dtype(img, tf.float32)[tf.newaxis, ...]
	except Exception:
		continue
	result = detector(converted_img)
	result = {key:value.numpy() for key,value in result.items()}
	scores = result["detection_scores"]
	hasTag=0
	for i in range(len(scores)):
		if scores[i]>0.5:
			hasTag=1
			final=bytearray(result["detection_boxes"][i])+bytearray(result["detection_class_entities"][i])+bytearray(1)+bytearray(result["detection_scores"][i])
			final_len = len(final)
			fout.write((final_len).to_bytes(8, byteorder='little'))
			fout.write(final)
	final_len=0;
	fout.write((final_len).to_bytes(8, byteorder='little'))
	fout.flush()
