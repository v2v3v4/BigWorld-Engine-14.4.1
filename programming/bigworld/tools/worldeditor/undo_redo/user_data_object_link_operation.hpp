#ifndef USER_DATA_OBJECT_LINK_OPERATION_HPP
#define USER_DATA_OBJECT_LINK_OPERATION_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "gizmo/undoredo.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class saves the link information for a User Data Object with this
 *  property and undoes changes to this link information.
 */
class UserDataObjectLinkOperation : public UndoRedo::Operation
{
public:
    explicit UserDataObjectLinkOperation
    (
        EditorChunkUserDataObjectPtr        udo,
		const BW::string&					linkName
    );

	/*virtual*/ void undo();

    /*virtual*/ bool iseq( UndoRedo::Operation const & other ) const;

private:
    UserDataObjectLinkOperation( UserDataObjectLinkOperation const & );
    UserDataObjectLinkOperation & operator=(
		UserDataObjectLinkOperation const & );

protected:
    BW::string						linkName_;
    BW::string						udoLink_;
    EditorChunkUserDataObjectPtr	udo_;
};

BW_END_NAMESPACE

#endif // USER_DATA_OBJECT_LINK_OPERATION_HPP
