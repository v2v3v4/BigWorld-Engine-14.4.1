def sumVector(vec):
	return sum(vec)

def buildRangeVector(x0, x1):
	return range(x0, x1 + 1)

def clampVec(vec, clampMin, clampMax):
	for i in range(len(vec)):
		if vec[i] < clampMin:
			vec[i] = clampMin
		elif vec[i] > clampMax:
			vec[i] = clampMax;
	vec = vec + vec
	return vec
