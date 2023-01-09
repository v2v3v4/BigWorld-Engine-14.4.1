
#include "pch.hpp"
#include "scene_browser.hpp"
#include "scene_browser_dlg.hpp"
#include "common/user_messages.hpp"
#include "gui/pages/panel_manager.hpp"

BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( SceneBrowser );


/**
 *	Constructor
 */
SceneBrowser::SceneBrowser()
{
	BW_GUARD;
}


/**
 *	This method tells the dialog to scroll the list until this item is visible.
 */
void SceneBrowser::scrollTo( ChunkItemPtr pItem ) const
{
	BW_GUARD;

	PanelManager::instance().showPanel( SceneBrowserDlg::contentID, TRUE );

	PanelManager::instance().panels().sendMessage(
								SceneBrowserDlg::contentID,
								WM_LIST_SCROLL_TO, 0, (LPARAM)pItem.get() );
}


/**
 *	This method returns true if the SceneBrowser dialog or any of its controls
 *	are focused.
 */
bool SceneBrowser::isFocused() const
{
	BW_GUARD;

	GUITABS::Content * pSB = PanelManager::instance().panels().getContent(
												SceneBrowserDlg::contentID );
	return isFocusedInternal( pSB );
}


/**
 *	This method returns the currently visible items
 *
 *	@param chunkItems	Container where the items are returned.
 *	@return				True if the items were set.
 */
bool SceneBrowser::currentItems(
							BW::vector< ChunkItemPtr > & chunkItems ) const
{
	BW_GUARD;

	GUITABS::Content * pSBwnd = PanelManager::instance().panels().getContent(
												  SceneBrowserDlg::contentID );
	bool itemsWereCopied = false;
	if (pSBwnd->getCWnd() && isFocusedInternal( pSBwnd ))
	{
		SceneBrowserDlg * pSB = static_cast< SceneBrowserDlg *>(
														pSBwnd->getCWnd() );
		pSB->currentItems( chunkItems );
		itemsWereCopied = true;
	}
	return itemsWereCopied;
}


/**
 *	This method returns true if the CWnd or any of its controls
 *	are focused.
 */
bool SceneBrowser::isFocusedInternal( GUITABS::Content * cnt ) const
{
	BW_GUARD;

	bool focused = false;

	if (cnt && cnt->getCWnd())
	{
		CWnd * pCurFocus = CWnd::FromHandle( GetFocus() );

		focused = (cnt->getCWnd() == pCurFocus) ||
					cnt->getCWnd()->IsChild( pCurFocus );
	}

	return focused;
}

BW_END_NAMESPACE

