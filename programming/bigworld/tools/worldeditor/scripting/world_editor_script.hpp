#ifndef WORLD_EDITOR_SCRIPT_HPP
#define WORLD_EDITOR_SCRIPT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include <Python.h>

BW_BEGIN_NAMESPACE

/**
 *	This namespace contains functions relating to scripting WorldEditor.
 */
namespace WorldEditorScript
{
	bool init( DataSectionPtr pDataSection );
	void fini();
	BW::string spaceName();
}

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "world_editor_script.ipp"
#endif


#endif // WORLD_EDITOR_SCRIPT_HPP
