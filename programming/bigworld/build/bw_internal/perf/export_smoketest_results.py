import sys
import performance_test.export

from performance_test.database import SmokeTestResult

performance_test.export.EXPORT_TYPE = SmokeTestResult
if __name__ == "__main__":
	sys.exit( performance_test.export.main() )
