#include "pch.hpp"

#include "editor_views.hpp"
#include "base_property_table.hpp"
#include "gizmo/general_editor.hpp"


DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	*** REQUIRES THAT THE VIEW'S NAME HAS BEEN CACHED ***
	 */
	bool viewIsGreater( BaseView* item1, BaseView* item2 )
	{
		return item1->cachedName() < item2->cachedName();
	}


	/**
	 *	*** REQUIRES THAT THE VIEW'S NAME HAS BEEN CACHED ***
	 */
	bool isSameView( BaseView* view1, BaseView* view2 )
	{
		BW_GUARD;

		if (view1->cachedName() == view2->cachedName())
		{
			if (view1->propertyItems().size() != view2->propertyItems().size())
				return false;
			for (size_t i = 0; i < view1->propertyItems().size(); ++i)
			{
				if (view1->propertyItems()[i]->getType() != view2->propertyItems()[i]->getType())
					return false;
			}
			return true;
		}
		return false;
	}


	class BasePropertyTableImpl: public SafeReferenceCount
	{
	public:
		BasePropertyTableImpl( BasePropertyTable* propertyTable )
			: propertyList( propertyTable )
		{
		}

		PropertyList propertyList;
		
		BW::list< BaseView* > viewList;
	};

} // anonymous namespace


BasePropertyTable::BasePropertyTable() :
	guiInterleave_( 0 )
{
	BW_GUARD;

	pImpl_ = new BasePropertyTableImpl( this );
}


BasePropertyTable::~BasePropertyTable()
{
}


void BasePropertyTable::update( int interleaveStep /*= 0*/, int maxTimeMS /*= 0*/ )
{
	BW_GUARD;

	PropertyList::update();

	if (!pImpl_->viewList.empty())
	{
		if (interleaveStep > 1)
		{
			uint64 startTime = timestamp();
			int iterations = interleaveStep; // max iterations
			while (--iterations)
			{
				int idx = 0;
				for (BW::list< BaseView* >::iterator it = pImpl_->viewList.begin();
					it != pImpl_->viewList.end(); ++it)
				{
					if ((idx % interleaveStep) == (guiInterleave_ % interleaveStep))
					{
						(*it)->updateGUI();
					}
					++idx;
				}
				++guiInterleave_;
				if (((timestamp() - startTime) * 1000 / stampsPerSecond()) > maxTimeMS)
				{
					break;
				}
			}
		}
		else
		{
			for (BW::list< BaseView* >::iterator it = pImpl_->viewList.begin();
				it != pImpl_->viewList.end(); ++it)
			{
				(*it)->updateGUI();
			}
		}
	}
}


void BasePropertyTable::clear()
{
	BW_GUARD;

	pImpl_->propertyList.clear();

	for (BW::list< BaseView* >::iterator i = pImpl_->viewList.begin();
		 i != pImpl_->viewList.end(); ++i)
	{
		if ((*i)->isCommonView())
		{
			bw_safe_delete(*i);
		}
	}
	pImpl_->viewList.clear();
}


PropertyList* BasePropertyTable::propertyList()
{
	return &(pImpl_->propertyList);
}


BW::list< BaseView* > & BasePropertyTable::viewList()
{
	return pImpl_->viewList;
}


int BasePropertyTable::addView( BaseView* view )
{
	BW_GUARD;

	if (view)
	{
		pImpl_->viewList.push_back( view );
		view->propertyCount( view->propertyItems().size() );

		for (size_t i = 0; i < view->propertyCount(); ++i)
		{
			pImpl_->propertyList.setArrayProperty( view->propertyItems()[i] );
		}

		return GeneralEditor::settingMultipleEditors() ? -1 : addItemsForView( view );
	}
	else 
	{
		return GeneralEditor::settingMultipleEditors() && GeneralProperty::View::pLastElected() ? addItemsForViews() : -1;
	}
}


int BasePropertyTable::addItemsForView( BaseView* view )
{
	BW_GUARD;

	int firstIndex = -1;
	for (size_t i = 0; i < view->propertyCount(); ++i)
	{
		int newIndex = pImpl_->propertyList.AddPropItem(
									view->propertyItems()[i], 
											static_cast<int>(firstIndex + i) );
		if (firstIndex == -1)
			firstIndex = newIndex;
	}
	return firstIndex;
}


int BasePropertyTable::addItemsForViews()
{
	BW_GUARD;

	BW::vector< BaseView* > list;
	for (BW::list< BaseView* >::iterator i = pImpl_->viewList.begin(); i != pImpl_->viewList.end(); ++i)
	{
		BaseView* item = *i;
		if (item->isCommonView())
		{
			CommonView* commItem = (CommonView*)item;
			list.insert( list.end(), commItem->views().begin(), commItem->views().end() );
			bw_safe_delete(commItem);
		}
		else
		{
			list.push_back( item );
		}

	}
	pImpl_->viewList.clear();
	pImpl_->propertyList.clear();

	// Cache view names for speed (BaseView::name is slow).
	for (BW::vector< BaseView* >::iterator i = list.begin(); i != list.end(); ++i)
	{
		(*i)->cacheName();
	}
	
	std::sort( list.begin(), list.end(), viewIsGreater );

	BW::vector< std::pair< size_t, size_t > > info;

	size_t i = 0;
	while (i < list.size())
	{
		if (list[i]->cachedName().empty())
		{
			i++;
			continue;
		}
		size_t b = i++;
		
		while (i < list.size() && isSameView( list[i], list[b] ))
		{
			i++;
		}

		size_t sz = i-b;
		if (sz > 1)
		{
			if (!info.size())
			{
				info.push_back( std::make_pair( b, sz ) );
			}
			else if (sz > info.back().second)
			{
				info.erase( info.begin(), info.end()-1 );
				info.back().first = b;
				info.back().second = sz;
			}
			else if (sz == info.back().second)
			{
				info.push_back( std::make_pair( b, sz ) );
			}
		}
	}

	int cnt = 0;
	for (i = 0; i < info.size(); ++i)
	{
		CommonView* commItem = new CommonView( list[ info[ i ].first ] );
		for (size_t j = 1; j < info[i].second; ++j)
		{
			BaseView* view = list[ info[ i ].first + j ];
			commItem->addView( view );
			for (size_t k = 0; k < view->propertyItems().size(); ++k)
			{
				view->propertyItems()[k]->create( &(pImpl_->propertyList) );
			}
		}
		commItem->propertyCount( commItem->views()[0]->propertyCount() );
		commItem->initGroups();
		pImpl_->viewList.push_back( commItem );
		cnt += addItemsForView( commItem );
		commItem->updateDisplayItems();
	}

	return cnt;
}
BW_END_NAMESPACE

