#ifndef PYTHON_OUTPUT_WRITER_HPP
#define PYTHON_OUTPUT_WRITER_HPP

#include "Python.h"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class PyOutputStream;
class ScriptOutputHook;

/**
 *	This class is used to manage hooking of Python's stdio output.
 *
 *	Instances should only be created when initialising the Python interpreter.
 *	A new instance will automatically redirect sys.stdout and sys.stderr to
 *	itself.
 *
 *	Note: ReferenceCount is only for internal use, no code except
 *	PyOutputStream should be taking references to this class.
 */
class ScriptOutputWriter : public ReferenceCount
{
public:
	ScriptOutputWriter();

	// These are only for use by ScriptOutputHook
	void addHook( ScriptOutputHook * pHook );
	void delHook( ScriptOutputHook * pHook );

	static ScriptOutputWriter * getCurrentStdio();

private:
	// Prevent subclassing
	~ScriptOutputWriter();

	friend class PyOutputStream;
	void handleWrite( const BW::string & msg, bool isStderr );

	BW::string			stdoutBuffer_;
	BW::string			stderrBuffer_;

	typedef BW::list< ScriptOutputHook * > Hooks;
	// The following are protected by the Python GIL
	Hooks				hooks_;
	ScriptOutputHook *	pCurrentHook_;
	bool				shouldDeleteCurrentHook_;
};

BW_END_NAMESPACE

#endif // PYTHON_OUTPUT_WRITER_HPP
