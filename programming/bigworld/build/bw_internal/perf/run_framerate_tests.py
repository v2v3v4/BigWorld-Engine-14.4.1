import sys

from performance_test.main import main
from performance_test.constants import TEST_FRAMERATE

if __name__ == "__main__":
	sys.exit( main( TEST_FRAMERATE ) )
