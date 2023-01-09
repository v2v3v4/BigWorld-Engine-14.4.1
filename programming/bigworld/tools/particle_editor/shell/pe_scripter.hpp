#ifndef SCRIPTER_HPP
#define SCRIPTER_HPP

#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

class Scripter
{
public:
	static bool init( DataSectionPtr pDataSection );
	static void fini();
	static bool update();

private:
	static PyObject* s_stdout;
	static PyObject* s_stderr;
};

BW_END_NAMESPACE

#endif // SCRIPTER_HPP
