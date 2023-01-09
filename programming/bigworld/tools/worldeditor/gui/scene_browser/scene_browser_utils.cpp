

#include "pch.hpp"
#include "scene_browser_utils.hpp"
#include "scene_browser.hpp"
#include "column_menu_item.hpp"
#include "common/popup_menu.hpp"
#include "gui/pages/panel_manager.hpp"
#include "gui/pages/page_properties.hpp"
#include "gui/dialogs/string_input_dlg.hpp"
#include "misc/popup_menu_helper.hpp"
#include "misc/world_editor_camera.hpp"

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
// Section: ListColumnPopup
///////////////////////////////////////////////////////////////////////////////


/**
 *	This static method handles the column chooser popup menu.
 *
 *	@param pParent			Parent window to the popup menu.
 *	@param pt				Desired menu position.
 *	@param states			List column states.
 *	@param columns			List columns array.
 *	@param groupByColumnIdx	Index of the grouping column, -1 if not grouping.
 *	@return					UPDATE_COLUMNS if columns have been changed, 0 if
 *							closed.
 */
/*static*/ int ListColumnPopup::doModal( CWnd * pParent, const CPoint &pt,
							ListColumnStates & states, ListColumns & columns,
							int groupByColumnIdx )
{
	BW_GUARD;

	const int COLUMNS_OFFSET = 1;
	const int LOAD_PRESET_OFFSET = 2000;
	const int SAVE_PRESET_OFFSET = 4000;
	const int RENAME_PRESET_OFFSET = 6000;
	const int DELETE_PRESET_OFFSET = 8000;
	const int HIDE_ALL = 10000;
	const int SHOW_ALL = 10001;
	const int NEW_PRESET = 10002;
	const int RESET_COLUMNS = 10003;
	const int CLOSE_MENU = 20000;
	const int NO_OP = 30000;

	PopupMenu menu;

	// Build the available columns menu items
	BW::vector< ColumnMenuItem > menuItems;

	for (size_t i = 0; i < columns.size(); ++i)
	{
		BW::string prefix = columns[i].visible() ? "##" : "";

		BW::string typeOwner =
			ItemInfoDB::instance().typeOnwer( columns[i].type() );

		menuItems.push_back( ColumnMenuItem(
					columns[i].name(), prefix, typeOwner, static_cast<int>(i) ) );
	}
	
	std::sort( menuItems.begin(), menuItems.end() );

	bool submenuStarted = false;
	BW::string lastOwner;
	for (size_t i = 0; i < menuItems.size(); ++i)
	{
		BW::string owner = menuItems[i].owner();
		BW::string text = menuItems[i].menuItem();
		int cmdIdx = menuItems[i].colIdx() + COLUMNS_OFFSET;
		if (owner != lastOwner)
		{
			if (submenuStarted)
			{
				menu.endSubmenu();
				submenuStarted = false;
			}
			if (!owner.empty())
			{
				menu.startSubmenu( bw_utf8tow( owner ) );
				submenuStarted = true;
			}
		}

		menu.addItem( bw_utf8tow( text ), cmdIdx );

		lastOwner = owner;
	}
	if (submenuStarted)
	{
		menu.endSubmenu();
	}

	// Hide/show all columns menu items
	menu.addSeparator();

	menu.addItem( Localise( L"SCENEBROWSER/HIDE_COLUMNS" ), HIDE_ALL );
	menu.addItem( Localise( L"SCENEBROWSER/SHOW_COLUMNS" ), SHOW_ALL );

	// Build the column layout presets menu items
	// Load preset
	menu.addSeparator();

	menu.startSubmenu( Localise( L"SCENEBROWSER/LOAD_PRESET" ) );

	DataSectionPtr pPresetsDS =
						BWResource::openSection( SCENE_BROWSER_PRESETS, true );

	BW::vector< BW::string > presets;
	states.listPresets( pPresetsDS, presets );

	if (!presets.empty())
	{
		int cmdIdx = LOAD_PRESET_OFFSET;
		for (BW::vector< BW::string >::iterator it = presets.begin();
			it != presets.end(); ++it)
		{
			menu.addItem( bw_utf8tow( (*it) ), cmdIdx++ );
		}
	}

	menu.endSubmenu();

	// Save preset
	menu.startSubmenu( Localise( L"SCENEBROWSER/SAVE_PRESET" ) );

	menu.addItem( Localise( L"SCENEBROWSER/NEW_PRESET" ), NEW_PRESET );

	if (!presets.empty())
	{
		menu.addSeparator();

		int cmdIdx = SAVE_PRESET_OFFSET;
		for (BW::vector< BW::string >::iterator it = presets.begin();
			it != presets.end(); ++it)
		{
			menu.addItem( bw_utf8tow( (*it) ), cmdIdx++ );
		}
	}

	menu.endSubmenu();

	// Rename preset
	menu.startSubmenu( Localise( L"SCENEBROWSER/RENAME_PRESET" ) );

	if (!presets.empty())
	{
		int cmdIdx = RENAME_PRESET_OFFSET;
		for (BW::vector< BW::string >::iterator it = presets.begin();
			it != presets.end(); ++it)
		{
			menu.addItem( bw_utf8tow( (*it) ), cmdIdx++ );
		}
	}

	menu.endSubmenu();

	// Delete preset
	menu.startSubmenu( Localise( L"SCENEBROWSER/DELETE_PRESET" ) );

	if (!presets.empty())
	{
		int cmdIdx = DELETE_PRESET_OFFSET;
		for (BW::vector< BW::string >::iterator it = presets.begin();
			it != presets.end(); ++it)
		{
			menu.addItem( bw_utf8tow( (*it) ), cmdIdx++ );
		}
	}

	menu.endSubmenu();

	menu.addItem( Localise( L"SCENEBROWSER/RESET_COLUMNS" ), RESET_COLUMNS );

	// Close this menu
	menu.addSeparator();

	menu.addItem( Localise( L"SCENEBROWSER/CLOSE_COLUMNS_MENU" ), CLOSE_MENU );

	// Run it
	int result = menu.doModal( *pParent, pt );
	int colIdx = result - COLUMNS_OFFSET;
	int loadPresetIdx = result - LOAD_PRESET_OFFSET;
	int savePresetIdx = result - SAVE_PRESET_OFFSET;
	int renamePresetIdx = result - RENAME_PRESET_OFFSET;
	int deletePresetIdx = result - DELETE_PRESET_OFFSET;

	ItemInfoDB::Type defaultColType =
		ItemInfoDB::Type::builtinType( ItemInfoDB::Type::TYPEID_ASSETNAME );

	int ret = (result != 0) ? DONT_UPDATE_COLUMNS : 0;

	if (colIdx >= 0 && colIdx < (int)columns.size())
	{
		// Toggle visibility for one column
		if (colIdx != groupByColumnIdx)
		{
			columns[ colIdx ].visible( !columns[ colIdx ].visible() );
			ret = UPDATE_COLUMNS;

			bool allColsHidden = true;
			
			for (size_t i = 0; i < columns.size(); ++i)
			{
				if (columns[ i ].visible())
				{
					allColsHidden = false;
					break;
				}
			}

			if (allColsHidden)
			{
				columns[ colIdx ].visible( true );
			}
		}
	}
	else if (result == HIDE_ALL || result == SHOW_ALL)
	{
		// Hide/show all columns
		bool showAll = (result == SHOW_ALL);
		for (size_t i = 0; i < columns.size(); ++i)
		{
			if (i != groupByColumnIdx)
			{
				if (!showAll && columns[ i ].type() == defaultColType)
				{
					columns[ i ].visible( true );
				}
				else
				{
					columns[ i ].visible( showAll );
				}
			}
		}
		ret = UPDATE_COLUMNS;
	}
	else if (loadPresetIdx >= 0 && loadPresetIdx < (int)presets.size())
	{
		// Load a column layout preset
		states.loadPreset( pPresetsDS, presets[ loadPresetIdx ] );
		states.applyColumnStates( columns );
		ret = UPDATE_ALL; // update columns AND sorting
	}
	else if ((savePresetIdx >= 0 && savePresetIdx < (int)presets.size()) ||
			result == NEW_PRESET)
	{
		// Load column layout to a preset
		BW::string presetName;
		if (result == NEW_PRESET)
		{
			getPresetName( pParent,
					Localise( L"SCENEBROWSER/NEW_PRESET_EDIT" ), presetName );
		}
		else
		{
			if (pParent->MessageBox(
					Localise( L"SCENEBROWSER/REPLACE_PRESET_MSG" ),
					Localise( L"SCENEBROWSER/PRESETS_CAPTION" ),
					MB_YESNO ) == IDYES)
			{
				presetName = presets[ savePresetIdx ];
			}
		}

		if (!presetName.empty())
		{
			states.syncColumnStates( columns );
			states.savePreset( pPresetsDS, presetName );
			pPresetsDS->save();
		}
	}
	else if (renamePresetIdx >= 0 && renamePresetIdx < (int)presets.size())
	{
		// Rename a column layout preset
		BW::string presetName = presets[ renamePresetIdx ];
		getPresetName( pParent,
				Localise( L"SCENEBROWSER/RENAME_PRESET_EDIT" ), presetName );
		if (!presetName.empty())
		{
			states.renamePreset( pPresetsDS, presets[ renamePresetIdx ],
																presetName );
			pPresetsDS->save();
		}
	}
	else if (deletePresetIdx >= 0 && deletePresetIdx < (int)presets.size())
	{
		// Delete a column layout preset
		if (pParent->MessageBox(
				Localise( L"SCENEBROWSER/DELETE_PRESET_MSG" ),
				Localise( L"SCENEBROWSER/PRESETS_CAPTION" ),
				MB_YESNO ) == IDYES)
		{
			states.deletePreset( pPresetsDS, presets[ deletePresetIdx ] );
			pPresetsDS->save();
		}
	}
	else if (result == RESET_COLUMNS)
	{
		if (pParent->MessageBox(
				Localise( L"SCENEBROWSER/RESET_COLUMNS_MSG" ),
				Localise( L"SCENEBROWSER/PRESETS_CAPTION" ),
				MB_YESNO ) == IDYES)
		{
			states.resetDefaultStates( columns );
			ret = UPDATE_ALL;
		}
	}
	else if (result == CLOSE_MENU)
	{
		// close the menu
		ret = 0;
	}

	return ret;
}


/**
 *	This private method opens an input dialog to get a preset name.
 */
/*static*/ void ListColumnPopup::getPresetName( CWnd * pParent,
						const BW::wstring & label, BW::string & presetName )
{
	BW_GUARD;

	StringInputDlg strInput( pParent );
	strInput.init( Localise( L"SCENEBROWSER/PRESETS_CAPTION" ),
													label, 80, presetName );
	bool askForName = true;
	while (askForName)
	{
		askForName = false;
		if (strInput.DoModal() == IDOK)
		{
			presetName = strInput.result();
			if (presetName.empty())
			{
				pParent->MessageBox(
					Localise( L"SCENEBROWSER/EMPTY_PRESET_MSG" ),
					Localise( L"SCENEBROWSER/PRESETS_CAPTION" ),
					MB_ICONERROR );
				askForName = true;
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Section: ListItemPopup
///////////////////////////////////////////////////////////////////////////////


/**
 *	This static method handles the column chooser popup menu.
 *
 *	@param pParent		Parent window to the popup menu.
 *	@param isGroup		Whether the item is or isn't a group.
 *	@param pItem		Desired list item.
 *	@param selHelper	Helps if we need to update the selection.
 *	@param items		Helps if we need to update the selection.
 *	@return				COLLAPSE_ALL, EXPAND_ALL or 0.
 */
/*static*/ int ListItemPopup::doModal( const CWnd * pParent, bool isGroup,
						ItemInfoDB::ItemPtr pItem, ListSelection & selHelper,
						const BW::vector< ItemInfoDB::ItemPtr > & items )
{
	BW_GUARD;

	// Right button was clicked on the list.
	PopupMenu menu;

	const int ZOOM_TO = 3;
	const int ITEM_PROPS = 4;
	const int BASE_ITEM_CMD = 1000;

	menu.addItem( Localise( L"SCENEBROWSER/COLLAPSE_ALL" ), COLLAPSE_ALL );
	menu.addItem( Localise( L"SCENEBROWSER/EXPAND_ALL" ), EXPAND_ALL );
	if (!isGroup)
	{
		// Zoom to items inside groups but not to a group itself
		menu.addSeparator();
		if (pItem && pItem->chunkItem())
		{
			if (PopupMenuHelper::buildCommandMenu( pItem->chunkItem(),
													BASE_ITEM_CMD, menu ) > 0)
			{
				menu.addSeparator();
			}
		}
		menu.addItem( Localise( L"SCENEBROWSER/ZOOM_TO" ), ZOOM_TO );

		menu.addSeparator();
		menu.addItem( Localise( L"SCENEBROWSER/ITEM_PROPS" ), ITEM_PROPS );
	}
	else
	{
		menu.addSeparator();
	}

	int result = menu.doModal( *pParent );

	if (result == ZOOM_TO)
	{
		ListItemZoom::zoomTo( pItem );
	}
	else if (result == ITEM_PROPS )
	{
		// Change the selection to the item if it's not in the current
		// selection, and show the Properties panel.

		if (pItem && pItem->chunkItem())
		{
			const ListSelection::ItemList & selection =
												selHelper.selection( false );

			bool found = false;
			
			for (ListSelection::ItemList::const_iterator
				it = selection.begin(); it != selection.end(); ++it)
			{
				if ((*it)->chunkItem() == pItem->chunkItem())
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				BW::vector< ChunkItemPtr > newSelection;
				newSelection.push_back( pItem->chunkItem() );
				selHelper.selection( newSelection, items );
			}

			PanelManager::instance().showPanel(
											PageProperties::contentID, true );
		}
	}
	else if (result >= BASE_ITEM_CMD)
	{
		if (pItem && pItem->chunkItem())
		{
			pItem->chunkItem()->edExecuteCommand( "", result - BASE_ITEM_CMD );
		}
	}

	return result;
}



///////////////////////////////////////////////////////////////////////////////
// Section: ListItemZoom
///////////////////////////////////////////////////////////////////////////////


/**
 *	This static method zooms to an item.
 *
 *	@param pItem	Item to zoom to.
 */
/*static*/ void ListItemZoom::zoomTo( ItemInfoDB::ItemPtr pItem )
{
	BW_GUARD;

	if (pItem && pItem->chunkItem() && pItem->chunkItem()->chunk())
	{
		WorldEditorCamera::instance().zoomExtents( pItem->chunkItem(), 2.0f );
	}
}

BW_END_NAMESPACE

