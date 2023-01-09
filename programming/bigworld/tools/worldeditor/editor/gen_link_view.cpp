#include "pch.hpp"
#include "gen_link_view.hpp"
#include "common/property_list.hpp"

BW_BEGIN_NAMESPACE

GenLinkView::GenLinkView( LinkProperty & property )
: property_( property )
{
}

StringPropertyItem* GenLinkView::item()
{
	return (StringPropertyItem*)&*(propertyItems_[0]);
}

void GenLinkView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}

	CString linkValue = property_.link()->linkValue().c_str();
	oldValue_ = linkValue;
	StringPropertyItem* newItem = new StringPropertyItem(
		property_.name(), linkValue, true);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);
	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

void GenLinkView::onSelect()
{
	BW_GUARD;
	property_.select();
}

bool GenLinkView::updateGUI()
{
	BW_GUARD;

	const CString newValue = property_.link()->linkValue().c_str();
	if (newValue != oldValue_)
	{
		oldValue_ = newValue;
		item()->set(newValue.GetString());
	}		
	return false;
}

/*static*/ LinkProperty::View * GenLinkView::create( LinkProperty & property )
{
	BW_GUARD;

	return new GenLinkView( property );
}

PropertyManagerPtr GenLinkView::getPropertyManager() const
{
	BW_GUARD;
	return property_.getPropertyManager();
}

// View factory initialiser.
GenLinkView::Enroller GenLinkView::Enroller::s_instance;

BW_END_NAMESPACE

