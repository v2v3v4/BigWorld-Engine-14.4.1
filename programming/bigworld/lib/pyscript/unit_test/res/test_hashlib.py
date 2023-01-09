import hashlib

def encryptTest(s):
	m = hashlib.md5()
	m.update(s)
	return m.hexdigest()

