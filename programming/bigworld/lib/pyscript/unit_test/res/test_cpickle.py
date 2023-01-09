import cPickle

def testPickleAndUnpickle():
	testObj = ["boo", "foo", ["em1", "em2"]]
	pickledObj = cPickle.dumps( testObj )
	unPickledObj = cPickle.loads( pickledObj )
	return testObj == unPickledObj