#ifndef AUTOMATION_HPP
#define AUTOMATION_HPP

#include "pyobject_plus.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This namespace manages the automation script.
 */
namespace Automation
{
	void parseCommandLine( LPCTSTR lpCmdLine );
	void executeScript( const char * scriptPath );
} // namespace Automation

BW_END_NAMESPACE

#endif // AUTOMATION_HPP