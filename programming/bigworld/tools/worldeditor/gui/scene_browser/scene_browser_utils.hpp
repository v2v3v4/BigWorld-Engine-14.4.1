#ifndef SCENE_BROWSER_UTILS_HPP
#define SCENE_BROWSER_UTILS_HPP


#include "list_column.hpp"
#include "list_column_states.hpp"
#include "list_selection.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Helper class that handles the column chooser popup menu.
 */
class ListColumnPopup
{
public:
	static const int DONT_UPDATE_COLUMNS = 1;
	static const int UPDATE_COLUMNS = 2;
	static const int UPDATE_ALL = 3;

	static int doModal( CWnd * pParent, const CPoint &pt,
							ListColumnStates & states, ListColumns & columns,
							int groupByColumnIdx );

private:
	static void getPresetName( CWnd * pParent, const BW::wstring & label,
													BW::string & presetName );
};


/**
 *	Helper class that handles the displaying of an item's context menu.
 */
class ListItemPopup
{
public:
	static const int COLLAPSE_ALL = 1;
	static const int EXPAND_ALL = 2;

	static int doModal( const CWnd * pParent, bool isGroup,
						ItemInfoDB::ItemPtr pItem, ListSelection & selHelper,
						const BW::vector< ItemInfoDB::ItemPtr > & items );
};


/**
 *	Helper class that allows zooming to an object.
 */
class ListItemZoom
{
public:
	static void zoomTo( ItemInfoDB::ItemPtr pItem );
};

BW_END_NAMESPACE

#endif // SCENE_BROWSER_UTILS_HPP
