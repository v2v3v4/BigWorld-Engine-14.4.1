#include "pch.hpp"

#include "test_py_output_writer.hpp"

#include "pyscript/script.hpp"

/**
 * Test various aspects of the python ScriptOutputWriter and its hooks
 */

namespace BW
{

// Test stdout and stderr passing through to DebugMessage system
TEST_FP( ScriptDebugMatchFixture,
	ScriptDebugMatchFixture( "Test stdio string: 25\n", false ),
	PythonScriptOutputWriter_testStdioDebugMessage )
{
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"print 'Test stdio string: %s' % ( 25, )",
		/* printResult */ true ) );

#if ENABLE_MSG_LOGGING
	CHECK( this->hasMatched() );
#else // ENABLE_MSG_LOGGING
	CHECK( !this->hasMatched() );
#endif // ENABLE_MSG_LOGGING
}


TEST_FP( ScriptDebugMatchFixture,
	ScriptDebugMatchFixture( "Test stderr string: 45\n", true ),
	PythonScriptOutputWriter_testStderrDebugMessage )
{
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; print >> sys.stderr, 'Test stderr string: %s' % ( 45, )",
		/* printResult */ true ) );

#if ENABLE_MSG_LOGGING
	CHECK( this->hasMatched() );
#else // ENABLE_MSG_LOGGING
	CHECK( !this->hasMatched() );
#endif // ENABLE_MSG_LOGGING
}


// Test stdout and stderr passing through to ScriptOutputHook system
TEST_FP( ScriptOutputHookLatchFixture,
	ScriptOutputHookLatchFixture( "Test stdio string: 25", false ),
	PythonScriptOutputWriter_testStdioHookMessage )
{
	CHECK( !this->isHooked() );

	this->hook();

	CHECK( this->isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"print 'Test stdio string: %s' % ( 25, )",
		/* printResult */ true ) );

	CHECK( this->hasMatched() );
}


TEST_FP( ScriptOutputHookLatchFixture,
	ScriptOutputHookLatchFixture( "Test stderr string: 45", true ),
	PythonScriptOutputWriter_testStderrHookMessage )
{
	CHECK( !this->isHooked() );

	this->hook();

	CHECK( this->isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; print >> sys.stderr, 'Test stderr string: %s' % ( 45, )",
		/* printResult */ true ) );

	CHECK( this->hasMatched() );
}


// Test that we can hook when only stdout or stderr is redirected, and that we
// cannot hook if neither is redirected.
// These tests need to be careful, we must leave sys as we found it.
// TODO: Once we can Script::init for each test, we can be less careful.
TEST_FP( ScriptOutputHookLatchFixture,
	ScriptOutputHookLatchFixture( "Test stdio string: 25", false ),
	PythonScriptOutputWriter_testStdioOnlyHookMessage )
{
	CHECK( !this->isHooked() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; sys.savestderr = sys.stderr; sys.stderr = sys.__stderr__",
		/* printResult */ true ) );

	this->hook();

	CHECK( this->isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"print 'Test stdio string: %s' % ( 25, )",
		/* printResult */ true ) );

	CHECK( this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; sys.stderr = sys.savestderr; del sys.savestderr",
		/* printResult */ true ) );
}


TEST_FP( ScriptOutputHookLatchFixture,
		ScriptOutputHookLatchFixture( "Test stderr string: 45", true ),
		PythonScriptOutputWriter_testStderrOnlyHookMessage )
{
	CHECK( !this->isHooked() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; sys.savestdout = sys.stdout; sys.stdout = sys.__stdout__",
		/* printResult */ true ) );

	this->hook();

	CHECK( this->isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; print >> sys.stderr, 'Test stderr string: %s' % ( 45, )",
		/* printResult */ true ) );

	CHECK( this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; sys.stdout = sys.savestdout; del sys.savestdout",
		/* printResult */ true ) );
}


TEST_FP( ScriptOutputHookLatchFixture,
		ScriptOutputHookLatchFixture( "Test stdout string: 25", true ),
		PythonScriptOutputWriter_testCannotHookMessage )
{
	CHECK( !this->isHooked() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; sys.savestdout = sys.stdout; sys.stdout = sys.__stdout__; "
			"sys.savestderr = sys.stderr; sys.stderr = sys.__stderr__",
		/* printResult */ true ) );

	this->hook();

	CHECK( !this->isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"print 'Test stdio string: %s' % ( 25, )",
		/* printResult */ true ) );

	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None, Script::runString(
		"import sys; sys.stdout = sys.savestdout; del sys.savestdout; "
		"sys.stderr = sys.savestderr; del sys.savestderr",
		/* printResult */ true ) );
}


// Test that it's safe to mutate ScriptOutputHook list from the callbacks
// Deliberately testing with different matches, ScriptOutputHook makes no
// promises that new hooks are added at the end...
TEST_FP( ScriptOutputHookLatchFixture,
	ScriptOutputHookLatchFixture( "Latch", false ),
	PythonScriptOutputWriter_testHookAddingHook )
{
	ScriptOutputAddOtherHook addHook( this, "Hook", false );

	CHECK( !this->isHooked() );
	CHECK( !addHook.isHooked() );

	addHook.hook();

	CHECK( addHook.isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None,
		Script::runString( "print 'Hook'", /* printResult */ true ) );

	CHECK( this->isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None,
		Script::runString( "print 'Latch'", /* printResult */ true ) );

	CHECK( this->isHooked() );
	CHECK( this->hasMatched() );
}

// Again, order of hooks added is not specified. But if they are ordered in a
// way other than by insertion (e.g. sorted container) these two tests need
// to be changed.
TEST_FP( ScriptOutputHookLatchFixture,
	ScriptOutputHookLatchFixture( "Latch", false ),
	PythonScriptOutputWriter_testHookDeletingPreviousHook )
{
	ScriptOutputDelOtherHook delHook( this, "Hook", false );

	CHECK( !this->isHooked() );
	CHECK( !delHook.isHooked() );

	this->hook();

	CHECK( this->isHooked() );

	delHook.hook();

	CHECK( delHook.isHooked() );
	CHECK( !this->hasMatched() );

	CHECK_EQUAL( Py_None,
		Script::runString( "print 'Hook'", /* printResult */ true ) );

	CHECK( !this->hasMatched() );
	CHECK( !this->isHooked() );
	CHECK( !this->wasUsedAfterFree() );

	CHECK_EQUAL( Py_None,
		Script::runString( "print 'Latch'", /* printResult */ true ) );

	CHECK( !this->hasMatched() );
	CHECK( !this->isHooked() );
	CHECK( !this->wasUsedAfterFree() );
}

TEST_FP( ScriptOutputHookLatchFixture,
	ScriptOutputHookLatchFixture( "Latch", false ),
	PythonScriptOutputWriter_testHookDeletingNextHook )
{
	ScriptOutputDelOtherHook delHook( this, "Hook", false );

	CHECK( !this->isHooked() );
	CHECK( !delHook.isHooked() );

	delHook.hook();

	CHECK( delHook.isHooked() );

	this->hook();

	CHECK( !this->hasMatched() );
	CHECK( this->isHooked() );

	CHECK_EQUAL( Py_None,
		Script::runString( "print 'Hook'", /* printResult */ true ) );

	CHECK( !this->hasMatched() );
	CHECK( !this->isHooked() );
	CHECK( !this->wasUsedAfterFree() );

	CHECK_EQUAL( Py_None,
		Script::runString( "print 'Latch'", /* printResult */ true ) );

	CHECK( !this->hasMatched() );
	CHECK( !this->wasUsedAfterFree() );
}

TEST_FP( ScriptOutputDelSelfHook,
	ScriptOutputDelSelfHook( "Hook", false ),
	PythonScriptOutputWriter_testHookDeletingSelf )
{
	CHECK( !this->isHooked() );

	this->hook();

	CHECK( this->isHooked() );

	CHECK_EQUAL( Py_None,
		Script::runString( "print 'Hook'", /* printResult */ true ) );

	CHECK( !this->isHooked() );
	CHECK( !this->wasUsedAfterFree() );
}

} // namespace BW

