#ifndef SINGLE_PROPERTY_EDITOR_HPP
#define SINGLE_PROPERTY_EDITOR_HPP


#include "world/item_info_db.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to change a property in a chunk item.
 */
class SinglePropertyEditor : public GeneralEditor
{
public:

	SinglePropertyEditor( ChunkItemPtr pItem,
				const ItemInfoDB::Type & type, const BW::string & newVal );

	virtual ~SinglePropertyEditor();

	virtual void addProperty( GeneralProperty * pProp );

private:
	ChunkItemPtr pItem_;
	ItemInfoDB::Type type_;
	BW::string newVal_;
	bool editHidden_;
	bool editFrozen_;
	GeneralProperty * pProperty_;
};

BW_END_NAMESPACE

#endif // SINGLE_PROPERTY_EDITOR_HPP
