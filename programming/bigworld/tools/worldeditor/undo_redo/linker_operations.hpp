#ifndef LINKER_OPERATIONS_HPP
#define LINKER_OPERATIONS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "common/base_properties_helper.hpp"
#include "gizmo/undoredo.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/unique_id.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class saves all three affected linkers of the link change so
 *	that the operation can be reverted if needed.
 */
class LinkerUndoChangeLinkOperation : public UndoRedo::Operation
{
public:
    explicit LinkerUndoChangeLinkOperation(
		const EditorChunkItemLinkable* startEcil,
		const EditorChunkItemLinkable* oldEcil,
		const EditorChunkItemLinkable* newEcil,
		PropertyIndex propIdx);

	/*virtual*/ void undo();

    /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

private:
    LinkerUndoChangeLinkOperation(LinkerUndoChangeLinkOperation const &);
    LinkerUndoChangeLinkOperation &operator=(LinkerUndoChangeLinkOperation const &);

	// Member variables
	UniqueID startUID_;
	BW::string startCID_;
	UniqueID oldUID_;
	BW::string oldCID_;
	UniqueID newUID_;
	BW::string newCID_;
	PropertyIndex propIdx_;
};


/**
 *  This class saves the new link added so that it can be deleted later if
 *	needed.
 */
class LinkerUndoAddLinkOperation : public UndoRedo::Operation
{
public:
    explicit LinkerUndoAddLinkOperation(
		const EditorChunkItemLinkable* startEcil,
		const EditorChunkItemLinkable* endEcil,
		PropertyIndex propIdx);

	/*virtual*/ void undo();

    /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

private:
    LinkerUndoAddLinkOperation(LinkerUndoAddLinkOperation const &);
    LinkerUndoAddLinkOperation &operator=(LinkerUndoAddLinkOperation const &);

	// Member variables
	UniqueID startUID_;
	BW::string startCID_;
	UniqueID endUID_;
	BW::string endCID_;
	PropertyIndex propIdx_;
};


/**
 *  This class saves all information regarding the deletion of a link so that it
 *	can be recreated if needed.
 */
class LinkerUndoDeleteLinkOperation : public UndoRedo::Operation
{
public:
    explicit LinkerUndoDeleteLinkOperation(
		const EditorChunkItemLinkable* startEcil,
		const EditorChunkItemLinkable* endEcil,
		DataSectionPtr data,
		PropertyIndex propIdx);

	/*virtual*/ void undo();

    /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

private:
    LinkerUndoDeleteLinkOperation(LinkerUndoDeleteLinkOperation const &);
    LinkerUndoDeleteLinkOperation &operator=(LinkerUndoDeleteLinkOperation const &);

	// Member variables
	UniqueID startUID_;
	BW::string startCID_;
	UniqueID endUID_;
	BW::string endCID_;
	DataSectionPtr data_;
	PropertyIndex propIdx_;
};


/**
 *  This class saves all information regarding two linker objects so that their
 *	relationship can be updated.
 */
class LinkerUpdateLinkOperation : public UndoRedo::Operation
{
public:
    explicit LinkerUpdateLinkOperation(
		const EditorChunkItemLinkable* startEcil,
		BW::string targetUID,
		BW::string targetChunkID);

	/*virtual*/ void undo();

    /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

private:
    LinkerUpdateLinkOperation(LinkerUpdateLinkOperation const &);
    LinkerUpdateLinkOperation &operator=(LinkerUpdateLinkOperation const &);

	// Member variables
	UniqueID startUID_;
	BW::string startCID_;
	UniqueID targetUID_;
	BW::string targetCID_;
};

BW_END_NAMESPACE

#endif // LINKER_OPERATIONS_HPP
