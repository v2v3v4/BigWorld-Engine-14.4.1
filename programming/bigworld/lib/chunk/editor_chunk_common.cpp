#include "pch.hpp"
#include "editor_chunk_common.hpp"

BW_BEGIN_NAMESPACE

bool EditorChunkCommonLoadSave::edCommonSave( DataSectionPtr pSection )
{
	BW_GUARD;

	DataSectionPtr editorSection = pSection->openSection( "editorOnly", true );
	if (editorSection)
	{
		editorSection->writeBool( "hidden", hidden_ );
		editorSection->writeBool( "frozen", frozen_ );
	}
	return true;
}

bool EditorChunkCommonLoadSave::edCommonLoad( DataSectionPtr pSection )
{
	BW_GUARD;

	DataSectionPtr editorSection = pSection->openSection( "editorOnly", false );
	if (editorSection)
	{
		hidden_ = editorSection->readBool( "hidden", false );
		frozen_ = editorSection->readBool( "frozen", false );
	}
	return true;
}

/**
 *	Hide/Show the the chunk item.
 *
 */
/*virtual*/
void EditorChunkCommonLoadSave::edHidden( bool value )
{
	hidden_ = value;
}


/**
 *	Retreive the hidden status for this item.
 *
 * @return The current hidden status.
 */
/*virtual*/
bool EditorChunkCommonLoadSave::edHidden( ) const
{
	return hidden_;
}


/**
 *	Freeze/Unfreeze the the chunk item.
 *
 */
/*virtual*/
void EditorChunkCommonLoadSave::edFrozen( bool value )
{
	frozen_ = value;
}


/**
 *	Retreive the frozen status for this item.
 *
 * @return The current frozen status.
 */
/*virtual*/
bool EditorChunkCommonLoadSave::edFrozen( ) const
{
	return frozen_;
}

BW_END_NAMESPACE

// editor_chunk_common.cpp
