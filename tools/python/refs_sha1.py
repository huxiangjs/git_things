import hashlib

if __name__ == '__main__':
	byte_data = b'This is a string for testing!'
	# print(byte_data)
	sha1 = hashlib.sha1()
	sha1.update(byte_data)
	dhex = sha1.hexdigest()
	print('STR SHA1:', dhex)
	# print(sha1.digest())

	sha1 = hashlib.sha1()
	for item in range(0, 256):
		byte_data = bytearray(range(item+1))
		# print(byte_data)
		sha1.update(byte_data)
	dhex = sha1.hexdigest()
	print('BIN SHA1:', dhex)
