#ifndef SHOW_CURSOR_HELPER_HPP
#define SHOW_CURSOR_HELPER_HPP


BW_BEGIN_NAMESPACE

/**
 *	This class changes the visibility of the cursor on constryction and
 *	restores it on destruction.
 */
class ShowCursorHelper
{
public:
	ShowCursorHelper( bool show );
	~ShowCursorHelper();

	// calling this method will prevent this object from restoring the
	// previous cursor state
	void dontRestore();

	// restore's previous cursor state early
	void restoreNow();

	// static helper method to now if the cursor is visible or not
	static bool visible();

private:
	bool show_;
	int showCursorCount_;
};

BW_END_NAMESPACE

#endif // SHOW_CURSOR_HELPER_HPP
