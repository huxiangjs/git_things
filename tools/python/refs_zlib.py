import zlib

if __name__ == '__main__':
	s = b'This is a string for testing!'

	print('zlib version:', zlib.ZLIB_VERSION)

	c = zlib.compress(s)
	print(c.hex(' '), '(%dbyte)' % len(c))

	s = zlib.decompress(c)
	print(s.decode(encoding='utf-8'), '(%dbyte)' % len(s))
