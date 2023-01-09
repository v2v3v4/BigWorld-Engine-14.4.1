#pragma once

#include "common/property_list.hpp"

BW_BEGIN_NAMESPACE

class BaseView;

namespace
{
	class BasePropertyTableImpl;
}


class BasePropertyTable
{
public:
	
	BasePropertyTable();
	virtual ~BasePropertyTable();
	
	virtual int addView( BaseView* view );

	virtual int addItemsForView( BaseView* view );

	virtual int addItemsForViews();

	virtual void update( int interleaveStep = 0, int maxTimeMS = 0 );
	
	virtual void clear();

	PropertyList* propertyList();

protected:
	BW::list< BaseView* > & viewList();

private:
	SmartPointer< BasePropertyTableImpl > pImpl_;
	int guiInterleave_;
};
BW_END_NAMESPACE

