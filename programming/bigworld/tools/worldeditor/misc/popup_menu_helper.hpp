#ifndef POPUP_MENU_HELPER_HPP
#define POPUP_MENU_HELPER_HPP


#include "chunk/chunk_item.hpp"
#include "common/popup_menu.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class has utility methods for creating some popup menus in WE.
 */
class PopupMenuHelper
{
public:
	static int buildCommandMenu(
					ChunkItemPtr pItem, int baseIdx, PopupMenu & destMenu );
};

BW_END_NAMESPACE

#endif // POPUP_MENU_HELPER_HPP
