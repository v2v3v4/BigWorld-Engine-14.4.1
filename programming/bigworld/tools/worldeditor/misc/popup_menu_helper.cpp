
#include "pch.hpp"
#include "popup_menu_helper.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This static method inserts the menu items relevant to the ChunkItem into the
 *	given menu. The menu item ids start from "baseIdx", so this value must be
 *	substracted from the popup menu result when calling edExecuteCommand.
 *
 *	@param pItem	Chunk item that is interested in adding its menu items.
 *	@param baseIdx	Used for creating valid command IDs. Mus tbe greater than 0.
 *	@param destMenu	Menu to receive the ChunkItem's menu items.
 *	@return		Number of menu items in the ChunkItem's commands.
 */
/*static*/ int PopupMenuHelper::buildCommandMenu(
						ChunkItemPtr pItem, int baseIdx, PopupMenu & destMenu )
{
	BW_GUARD;

	if (baseIdx <= 0)
	{
		ERROR_MSG( "PopupMenuHelper::buildCommandMenu: "
					"Base index must be greater than 0\n" );
		return 0;
	}
	
	BW::vector< BW::string > commands = pItem->edCommand( "" );

	UINT pos = baseIdx;
	for (BW::vector<BW::string>::iterator iter = commands.begin();
		iter != commands.end(); ++iter)
	{
		if ((*iter).empty())
		{
			destMenu.addSeparator();
		}
		else if ((*iter) == "##")
		{
			destMenu.endSubmenu();
		}
		else if ((*iter).substr( 0, 1 ) == "#")
		{
			if ((*iter).substr( 1, 2 ) == "`")
			{
				BW::wstring substr;
				bw_utf8tow( LocaliseUTF8( (*iter).substr( 2 ).c_str() ), substr );
				destMenu.startSubmenu( substr );
			}
			else
			{
				BW::wstring substr;
				bw_utf8tow( (*iter).substr( 1 ), substr );
				destMenu.startSubmenu( substr );
			}
		}
		else
		{
			if ((*iter).substr( 0, 1 ) == "`")
			{
				BW::wstring witer;
				bw_utf8tow( LocaliseUTF8( (*iter).substr( 1 ).c_str() ), witer );
				destMenu.addItem( witer, pos++ );
			}
			else
			{
				BW::wstring witer;
				bw_utf8tow( (*iter), witer );
				destMenu.addItem( witer, pos++ );
			}
		}
	}

	return static_cast<int>(commands.size());
}
BW_END_NAMESPACE

