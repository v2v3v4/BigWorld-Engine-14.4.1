import zlib

def testZlib():
	testVal = "test data to compress. 1234567890 test test123456test "
	compressedData = zlib.compress(testVal, 8)
	uncompressedData = zlib.decompress(compressedData)
	return testVal == uncompressedData