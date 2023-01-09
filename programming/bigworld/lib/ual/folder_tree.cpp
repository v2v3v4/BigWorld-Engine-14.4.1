/**
 *	FolderTree: Inherits from CTreeCtrl to make a folder tree control with
 *	drag & drop support.
 */

#include "pch.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "ual_resource.h"
#include "folder_tree.hpp"
#include "list_cache.hpp"
#include "smart_list_ctrl.hpp"
#include "controls/drag_image.hpp"
#include "common/string_utils.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/string_utils.hpp"

// for flicker free redrawing
#include "controls/memdc.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param thumbnailManager Provider of thumbnails for the tree view.
 */
FolderTree::FolderTree( ThumbnailManagerPtr thumbnailManager ) :
	initialised_( false ),
	sortVFolders_( true ),
	sortSubFolders_( true ),
	thumbnailManager_( thumbnailManager ),
	eventHandler_( 0 ),
	dragImgList_( 0 ),
	dragImage_( NULL ),
	dragging_( false ),
	vfolderIcon_( -1 ),
	vfolderIconSel_( -1 ),
	itemIcon_( -1 ),
	itemIconSel_( -1 ),
	firstImageIndex_( 0 )
{
	BW_GUARD;

	MF_ASSERT( thumbnailManager_ != NULL );
}


/**
 *	Destructor.
 */
FolderTree::~FolderTree()
{
	BW_GUARD;

	thumbnailManager_->resetPendingRequests( this );

	bw_safe_delete(dragImgList_);
	bw_safe_delete(dragImage_);

	imgList_.DeleteImageList();
}


/**
 *	This method allows for another object to handle FolderTree events.
 *
 *	@param eventHandler	Object that will handle events from this FolderTree.
 */
void FolderTree::setEventHandler( FolderTreeEventHandler* eventHandler )
{
	BW_GUARD;

	eventHandler_ = eventHandler;
}


/**
 *	This method adds an icon to the image list for later use.
 *
 *	@param image	Icon to add.
 *	@return		Index of the icon in the internal list.
 */
int FolderTree::addIcon( HICON image )
{
	BW_GUARD;

	if ( !unusedImages_.empty() )
	{
		int i = *unusedImages_.begin();
		unusedImages_.erase( unusedImages_.begin() );
		imgList_.Replace( i, image );
		return i;
	}
	else
		return imgList_.Add( image );
}


/**
 *	This method adds a bitmap to the image list for later use.
 *
 *	@param image	Bitmap to add.
 *	@return		Index of the bitmap in the internal list.
 */
int FolderTree::addBitmap( HBITMAP image )
{
	BW_GUARD;

	if ( !unusedImages_.empty() )
	{
		int i = *unusedImages_.begin();
		unusedImages_.erase( unusedImages_.begin() );
		CImage mask;
		mask.Create( IMAGE_SIZE, IMAGE_SIZE, 24 );
		CDC* dc = CDC::FromHandle( mask.GetDC() );
		dc->FillSolidRect( 0, 0, IMAGE_SIZE, IMAGE_SIZE, RGB( 0, 0, 0 ) );
		dc->Detach();
		mask.ReleaseDC();
		imgList_.Replace( i, CBitmap::FromHandle( image ), CBitmap::FromHandle( mask ) );
		return i;
	}
	else
		return imgList_.Add( CBitmap::FromHandle( image ), (CBitmap*)0 );
}


/**
 *	This method removes the image at position index.  It simply tags the index
 *	as "unused" for speed and also so indices to other images are still valid.
 *
 *	@param index	Index to the image to remove from the list.
 */
void FolderTree::removeImage( int index )
{
	BW_GUARD;

	if ( index >= 0 && index >= firstImageIndex_ && index < imgList_.GetImageCount() )
		unusedImages_.push_back( index );
}


/**
 *	This method initialises the control.
 */
void FolderTree::init()
{
	BW_GUARD;

	if ( initialised_ )
		return;

	imgList_.Create( IMAGE_SIZE, IMAGE_SIZE, ILC_COLOR24|ILC_MASK, 2, 32 );
	imgList_.SetBkColor( ( GetBkColor() == -1 ) ? GetSysColor( COLOR_WINDOW ) : GetBkColor() );
	addIcon( AfxGetApp()->LoadIcon( IDI_UALFOLDER ) );
	addIcon( AfxGetApp()->LoadIcon( IDI_UALFOLDERSEL ) );
	addIcon( AfxGetApp()->LoadIcon( IDI_UALFILE ) );
	addIcon( AfxGetApp()->LoadIcon( IDI_UALFILESEL ) );

	SetImageList( &imgList_, TVSIL_NORMAL );

	int idx = 0;
	vfolderIcon_ = idx++;
	vfolderIconSel_ = idx++;
	itemIcon_ = idx++;
	itemIconSel_ = idx++;
	firstImageIndex_ = idx;
}


/**
 *	This method sets the icon indices for a group. The group tells whether the
 *	icon represents an asset or a folder.
 *
 *	@param group	Index of the group.
 *	@param icon		Index of the icon for the unselected image.
 *	@param iconSel	Index of the icon for when the image is selected.
 */
void FolderTree::setGroupIcons( int group, HICON icon, HICON iconSel )
{
	BW_GUARD;

	BW::vector<GroupIcons>::iterator i;
	for( i = groupIcons_.begin(); i != groupIcons_.end(); ++i )
	{
		if ( (*i).group == group )
			break;
	}

	if ( i == groupIcons_.end() )
	{
		GroupIcons gi;
		gi.group = group;
		gi.icon = vfolderIcon_;
		gi.iconSel = vfolderIconSel_;
		groupIcons_.push_back( gi );
		i = groupIcons_.end();
		--i;
	}
	if ( icon )
		(*i).icon = addIcon( icon );

	if ( iconSel )
		(*i).iconSel = addIcon( iconSel );
}


/**
 *	This method tells whether an icon is one of the built-in icons or not.
 *
 *	@param icon		Index of the image to check.
 *	@return	True if the image at the index is a built-in one, false if not.
 */
bool FolderTree::isStockIcon( int icon )
{
	BW_GUARD;

	if ( icon < 2 )
		return true;
	for( BW::vector<GroupIcons>::iterator i = groupIcons_.begin();
		i != groupIcons_.end(); ++i )
		if ( (*i).icon == icon || (*i).iconSel == icon )
			return true;
	for( BW::vector<ExtensionsIcons>::iterator i = extensionsIcons_.begin();
		i != extensionsIcons_.end(); ++i )
		if ( (*i).icon == icon || (*i).iconSel == icon )
			return true;
	return false;
}


/**
 *	This method returns the icon indices for a group, and whether or not it is
 *	expandable in the tree view. The group tells whether the icon represents an
 *	asset or a folder.
 *
 *	@param group	Index of the group.
 *	@param icon		Returns the index of the icon for the unselected image.
 *	@param iconSel	Returns the index of the icon for when it is selected.
 *	@param expandable	True if the item should expand on click, false if not.
 */
void FolderTree::getGroupIcons( int group, int& icon, int& iconSel, bool expandable )
{
	BW_GUARD;

	for( BW::vector<GroupIcons>::iterator i = groupIcons_.begin(); i != groupIcons_.end(); ++i )
	{
		if ( (*i).group == group )
		{
			icon = (*i).icon;
			iconSel = (*i).iconSel;
			return;
		}
	}
	if ( expandable )
	{
		icon = vfolderIcon_;
		iconSel = vfolderIconSel_;
	}
	else
	{
		icon = itemIcon_;
		iconSel = itemIconSel_;
	}
}


/**
 *	This method associates an icon pair to a set of filename extensions.
 *
 *	@param extensions	Comma or semicolon separated list of extensions.
 *	@param icon		The index of the icon for the unselected image.
 *	@param iconSel	The index of the icon for when it is selected.
 */
void FolderTree::setExtensionsIcons( BW::wstring extensions, HICON icon, HICON iconSel )
{
	BW_GUARD;

	ExtensionsIcons extIcons;

	bw_tokenise( extensions, L",;", extIcons.extensions );
	
	extIcons.icon = vfolderIcon_;
	extIcons.iconSel = vfolderIconSel_;

	if ( icon )
		extIcons.icon = addIcon( icon );

	if ( iconSel )
		extIcons.iconSel = addIcon( iconSel );

	extensionsIcons_.push_back( extIcons );
}


/**
 *	This method returns the asset's associated icon pair by checking its
 *	filename extensions.
 *
 *	@param name		Asset filename to test for its extension.
 *	@param icon		Returns the index of the icon for the unselected image.
 *	@param iconSel	Returns the index of the icon for when it is selected.
 */
void FolderTree::getExtensionIcons( const BW::wstring& name, int& icon, int& iconSel )
{
	BW_GUARD;

	icon = vfolderIcon_;
	iconSel = vfolderIconSel_;

	BW::string nname;
	bw_wtoutf8( name, nname );
	BW::string ext = BWResource::getExtension( nname ).to_string();

	if ( !ext.length() )
		return;

	for( BW::vector<ExtensionsIcons>::iterator i = extensionsIcons_.begin(); i != extensionsIcons_.end(); ++i )
	{
		for( BW::vector<BW::wstring>::iterator j = (*i).extensions.begin(); j != (*i).extensions.end(); ++j )
		{
			if ( bw_MW_stricmp( (*j).c_str(), ext.c_str() ) == 0 )
			{
				icon = (*i).icon;
				iconSel = (*i).iconSel;
				return;
			}
		}
	}
}


/**
 *	This method returns the virtual folder an item belongs to.
 *
 *	@param data	Item data containing the item's info.
 *	@return	Parent virtual folder if found, NULL if the item is an orphan.
 */
VFolderPtr FolderTree::getVFolder( VFolderItemData* data )
{
	BW_GUARD;

	while ( data && !data->isVFolder() )
	{
		HTREEITEM item = GetParentItem( data->getTreeItem() );
		if ( item )
			data = (VFolderItemData*)GetItemData( item );
		else
			data = 0;
	}

	if ( data )
		return data->getVFolder();	

	return 0;
}


/**
 *	This method finds the tree view item corresponding to the item represented
 *	by the VFolderItemData pointer "data", starting at the passed in "item".
 *	TODO:  See if this method is still needed, VFolderItemData objects now
 *	store their corresponding HTREEITEM internally.
 *
 *	@param data	Item data containing the item's info.
 *	@param item	Tree view item to start the search (usually a parent VFolder).
 *	@return	Tree view item corresponding to the VFolderItemData pointer.
 */
HTREEITEM FolderTree::getItem( VFolderItemData* data, HTREEITEM item )
{
	BW_GUARD;

	if ( item != TVI_ROOT )
	{
		if ( (VFolderItemData*)GetItemData( item ) == data )
			return item;
	}

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		HTREEITEM next = GetNextItem( child, TVGN_NEXT );
		item = getItem( data, child );
		if ( item )
			return item;
		child = next;
	}

	return 0;
}


/**
 *	This method returns the order the virtual folders show up in the tree view,
 *	which is user-configurable via xml and/or drag and drop.
 *
 *	@param orderStr	Recursion param, contains the current accumulated order.
 *	@param item		Recursion param, tree view item to start the search.
 *	@return	List of semicolon separated virtual folder names, representing the
 *			order of the virtual folders in the tree view.
 */
BW::wstring FolderTree::getVFolderOrder( const BW::wstring orderStr, HTREEITEM item )
{
	BW_GUARD;

	BW::wstring accum = orderStr;

	if ( !item )
		return accum;

	if ( item != TVI_ROOT )
	{
		VFolderItemData* data = (VFolderItemData*)GetItemData( item );
		if ( data && data->isVFolder() )
		{
			if ( !accum.empty() )
				accum += L";";
			accum += data->assetInfo().text();
		}
		else
			return accum;
	}

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		accum = getVFolderOrder( accum, child );
		child = GetNextItem( child, TVGN_NEXT );
	}

	return accum;
}


/**
 *	This static function is used as a SortChildrenCB callback so virtual 
 *	folders get sorted properly, specially because virtual folders can be moved
 *	around by the user.
 *
 *	@param lParam1	First item to compare.
 *	@param lParam2	The other item to compare.
 *	@param lParamSort	Sort order string.
 *	@return	less than zero if lParam1 < lParam2, greater than zero if
 *			lParam1 > lParam2, zero if they are equal.
 */
static int CALLBACK FolderTreeOrderStrFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	BW_GUARD;

	if ( lParam1 == lParam2 )
		return 0;

	VFolderItemData* data1 = (VFolderItemData*)lParam1;
	VFolderItemData* data2 = (VFolderItemData*)lParam2;

	if ( !data1 || !data1->isVFolder() )
		return -1;
	if ( !data2 || !data2->isVFolder() )
		return 1;

	BW::vector<BW::wstring>* oi = (BW::vector<BW::wstring>*)lParamSort;

	int cnt1 = 0;
	int cnt2 = 0;
	for( int i = 0; i < (int)oi->size(); ++i )
		if ( oi->at(i) == data1->assetInfo().text() )
		{
			cnt1 = i;
			break;
		}
	for( int i = 0; i < (int)oi->size(); ++i )
		if ( oi->at(i) == data2->assetInfo().text() )
		{
			cnt2 = i;
			break;
		}

	return cnt1-cnt2;
}


/**
 *	This method sorts virtual folders to the way the user has arranged them.
 *
 *	@param orderStr	List of comma or semicolon separated virtual folder names
 *					representing the order of virtual folders in the tree view.
 */
void FolderTree::setVFolderOrder( const BW::wstring orderStr )
{
	BW_GUARD;

	if ( orderStr.empty() )
		return;
	BW::vector<BW::wstring> oi;
	bw_tokenise( orderStr, L",;", oi );
	TVSORTCB sortCB;
	sortCB.hParent = TVI_ROOT;
	sortCB.lpfnCompare = FolderTreeOrderStrFunc;
	sortCB.lParam = (LPARAM)&oi;
	SortChildrenCB( &sortCB );
}


/**
 *	This method moves a virtual folder to before another virtual folder, or to
 *	the bottom of the tree if no other virtual folder is passed in.
 *
 *	@param vf1	Virtual folder to move.
 *	@param vf2	Destination virtual folder, to place vf1 before it, or NULL if
 *				vf1 should be placed at the botton of the tree view.
 */
void FolderTree::moveVFolder( VFolderPtr vf1, VFolderPtr vf2 )
{
	BW_GUARD;

	if ( !vf1 || vf1 == vf2 )
		return;

	BW::vector<BW::wstring> oi;
	bw_tokenise( getVFolderOrder(), L",;", oi );

	for( BW::vector<BW::wstring>::iterator i = oi.begin(); i != oi.end(); ++i )
		if ( (*i) == vf1->getName() )
		{
			oi.erase( i );
			break;
		}

	if ( !vf2 )
		oi.push_back( vf1->getName() );
	else
	{
		bool added = false;
		for( BW::vector<BW::wstring>::iterator i = oi.begin(); i != oi.end(); ++i )
			if ( (*i) == vf2->getName() )
			{
				oi.insert( i, vf1->getName() );
				added = true;
				break;
			}
		if ( !added )
			oi.push_back( vf1->getName() );
	}

	BW::wstring str;
	bw_stringify( oi, L";", str );
	setVFolderOrder( str );
}


/**
 *	This method calculates how deep the tree view is.
 *
 *	@param item		Tree view item to start at, usually TVI_ROOT.
 *	@returns Number of levels of the tree.
 */
int FolderTree::getLevelCount( HTREEITEM item )
{
	BW_GUARD;

	int cnt = 0;
	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		cnt++;
		child = GetNextItem( child, TVGN_NEXT );
	}
	return cnt;
}


/**
 *	This method recursively releases resources for a tree view branch, starting
 *	at "item".
 *
 *	@param item		Tree view item to start at.
 */
void FolderTree::freeSubtreeData( HTREEITEM item )
{
	BW_GUARD;

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		freeSubtreeData( child );
		VFolderItemData* data = (VFolderItemData*)GetItemData( child );
		int icon = 0;
		int iconSel = 0;
		GetItemImage( child, icon, iconSel );
		if ( !isStockIcon( icon ) )
			removeImage( icon );
		if ( iconSel != icon && !isStockIcon( iconSel ))
			removeImage( iconSel );
		for( BW::vector<VFolderItemDataPtr>::iterator i = itemDataHeap_.begin();
			i != itemDataHeap_.end();
			++i )
		{
			if ( (*i).getObject() == data )
			{
				itemDataHeap_.erase( i );
				SetItemData( child, 0 );
				break;
			}
		}
		child = GetNextItem( child, TVGN_NEXT );
	}
}


/**
 *	This method reloads the contents of a virtual folder.
 *
 *	@param vfolder	Virtual folder to reload/refresh its data.
 */
void FolderTree::refreshVFolder( VFolderPtr vfolder )
{
	BW_GUARD;

	if ( !vfolder || !vfolder->isExpandable() || !vfolder->getItem() )
		return;
	
	SetRedraw( FALSE );

	int scrollX = GetScrollPos( SB_HORZ );
	int scrollY = GetScrollPos( SB_VERT );

	HTREEITEM item = vfolder->getItem();

	// save selected item's parents
	BW::vector<BW::wstring> lastSelected;
	HTREEITEM sel = GetSelectedItem();
	while ( sel )
	{
		if ( sel == item )
			break; // found the vfolder, so the vfolder's subtree includes the selected item
		lastSelected.push_back( (LPCTSTR)GetItemText( sel ) );
		sel = GetParentItem( sel );
	}
	if ( !sel )
		lastSelected.clear(); // selected item not in the vfolder's subtree, so don't care

	UINT state = GetItemState( item, TVIS_EXPANDED );
//	if ( state & TVIS_EXPANDED )
//		Expand( item, TVE_COLLAPSE );
//	SetItemState( item, 0, TVIS_EXPANDEDONCE );

	// Erase subtree item's their mem gets released
	freeSubtreeData( item );

	// Erase the actual items from the subtree
	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		HTREEITEM next = GetNextItem( child, TVGN_NEXT );
		DeleteItem( child );
		child = next;
	}

	// Rebuild tree
	InsertItem( L"***dummychild***", 0, 0, item );
//	if ( state & TVIS_EXPANDED )
//		Expand( item, TVE_EXPAND );
	if ( state & TVIS_EXPANDED )
		expandItem( item );

	if ( !lastSelected.empty() )
	{
		// the selected item was inside the vfolder's tree before reconstruction, so reselect
		child = item;
		HTREEITEM deepestNull = item;
		for( BW::vector<BW::wstring>::reverse_iterator i = lastSelected.rbegin();
			i != lastSelected.rend();
			++i )
		{
			// find the child by name
			state = GetItemState( child, TVIS_EXPANDED );
			if ( !(state & TVIS_EXPANDED) )
				Expand( child, TVE_EXPAND );
			child = GetChildItem( child );
			while ( child )
			{
				if ( (*i) == (LPCTSTR)GetItemText( child ) )
					break;
				child = GetNextItem( child, TVGN_NEXT );
			}
			if ( !child )
				break;
			deepestNull = child;
		}
		if ( deepestNull )
			SelectItem( deepestNull ); // select the deepest level of the tree found ( hopefully, the old selected item )
	}

	SetScrollPos( SB_HORZ, scrollX );
	SetScrollPos( SB_VERT, scrollY );

	SetRedraw( TRUE );
	Invalidate();
	UpdateWindow();
}


/**
 *	This method recursively looks for virtual folders in the tree and refreshes
 *	their contents.
 *
 *	@param provider	Recursion param, current virtual folder provider.
 *	@param item		Recursion param, tree view item to start at.
 */
void FolderTree::refreshVFolders( VFolderProviderPtr provider, HTREEITEM item )
{
	BW_GUARD;

	if ( !item )
		return;

	if ( item != TVI_ROOT )
	{
		VFolderItemData* data = (VFolderItemData*)GetItemData( item );
		if ( !data || !data->isVFolder() )
			return;
		if ( !provider || data->getProvider() == provider )
		{
			refreshVFolder( data->getVFolder() );
			if ( data->getProvider() == provider )
				return; // no need to keep looking
		}
	}

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		HTREEITEM next = GetNextItem( child, TVGN_NEXT );
		refreshVFolders( provider, child );
		child = next;
	}
}


/**
 *	This method recursively looks for a virtual folder named "name". If the
 *	"strict" flag is set to false, it will also look for child items named
 *	"name" and if such an item is found, return its parent virtual folder.
 *
 *	@param name		Name of the virtual folder we are looking for.
 *	@param strict	True to look only for VFolders, false to also look for
 *					items and return their parent virtual volder.
 *	@param item		Recursion param, tree view item to start at.
 *	@return			Desired virtual folder, or NULL of none was found.
 */
VFolderPtr FolderTree::getVFolder( const BW::wstring& name, bool strict, HTREEITEM item )
{
	BW_GUARD;

	if ( !item )
		return 0;

	if ( name.empty() )
		return 0;

	if ( item != TVI_ROOT )
	{
		VFolderItemData* data = (VFolderItemData*)GetItemData( item );
		if ( data && name == (LPCTSTR)GetItemText( item ) )
		{
			if ( data->isVFolder() )
				return data->getVFolder();
			else if ( !strict )
				return getVFolder( data );
		}
	}

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		VFolderPtr vfolder = getVFolder( name, strict, child );
		if ( !!vfolder )
			return vfolder;
		child = GetNextItem( child, TVGN_NEXT );
	}
	return 0;
}


/**
 *	This method recursively looks for a virtual folder using a custom test
 *	callback function. If the "strict" flag is set to false, it will also look
 *	for child items and if such an item is found, return its parent virtual
 *	folder.
 *
 *	@param test		Custom test callback function.
 *	@param testData	Data needed by the custom test callback.
 *	@param strict	True to look only for VFolders, false to also look for
 *					items and return their parent virtual volder.
 *	@param item		Recursion param, tree view item to start at.
 *	@return			Desired virtual folder, or NULL of none was found.
 */
VFolderPtr FolderTree::getVFolderCustom( ItemTestCB test, void* testData, bool strict, HTREEITEM item )
{
	BW_GUARD;

	if ( !item )
		return 0;

	if ( !test )
		return 0;

	if ( item != TVI_ROOT )
	{
		VFolderItemData* data = (VFolderItemData*)GetItemData( item );
		if ( data && test( item, testData ) )
		{
			if ( data->isVFolder() )
				return data->getVFolder();
			else if ( !strict )
				return getVFolder( data );
		}
	}

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		VFolderPtr vfolder = getVFolderCustom( test, testData, strict, child );
		if ( !!vfolder )
			return vfolder;
		child = GetNextItem( child, TVGN_NEXT );
	}
	return 0;
}


/**
 *	This method gathers and returns all the virtual folders in the tree view
 *	(when called with the default "item" parameter).
 *
 *	@param items	Return vector containing all found virtual folders.
 *	@param item		Recursion param, tree view item to start at.
 */
void FolderTree::getVFolders( BW::vector<HTREEITEM>& items, HTREEITEM item )
{
	BW_GUARD;

	if ( !item )
		return;

	if ( item != TVI_ROOT )
	{
		VFolderItemData* data = (VFolderItemData*)GetItemData( item );
		if ( data && data->isVFolder() )
			items.push_back( item );
	}

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		getVFolders( items, child );
		child = GetNextItem( child, TVGN_NEXT );
	}
	return;
}


/**
 *	This method selects a virtual folder by name in the tree view.
 *
 *	@param name		Name of the virtual folder to select.
 */
void FolderTree::selectVFolder( const BW::wstring& name )
{
	BW_GUARD;

	if ( name.empty() )
		return;

	VFolderPtr vfolder = getVFolder( name );
	if ( vfolder && vfolder->getItem() )
		SelectItem( vfolder->getItem() );
}


/**
 *	This method selects a virtual folder in the tree view using a custom
 *	callback test function.
 *
 *	@param test		Custom test callback function.
 *	@param testData	Data needed by the custom test callback.
 */
void FolderTree::selectVFolderCustom( ItemTestCB test, void* testData )
{
	BW_GUARD;

	if ( !test )
		return;

	VFolderPtr vfolder = getVFolderCustom( test, testData );
	if ( vfolder && vfolder->getItem() )
		SelectItem( vfolder->getItem() );
}


/**
 *	This static function is used as a SortChildrenCB callback so tree view
 *	items that are not virtual folders get sorted properly.
 *
 *	@param lParam1	First item to compare.
 *	@param lParam2	The other item to compare.
 *	@param lParamSort	Sort order string.
 *	@return	less than zero if lParam1 < lParam2, greater than zero if
 *			lParam1 > lParam2, zero if they are equal.
 */
static int CALLBACK FolderTreeCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	BW_GUARD;

	VFolderItemData* data1 = (VFolderItemData*)lParam1;
	VFolderItemData* data2 = (VFolderItemData*)lParam2;

	if ( !data1 ) 
		return -1;

	if ( !data2 ) 
		return 1;

	// Check groups, folders go first before files/items.
	if ( data1->getGroup() > data2->getGroup() )
		return 1;
	else if ( data1->getGroup() < data2->getGroup() )
		return -1;

	return wcsicmp( data1->assetInfo().text().c_str(), data2->assetInfo().text().c_str() );
}


/**
 *	This method sorts the subtree of items of a folder.
 *
 *	@param item Tree view item corresponding to the folder to sort.
 */
void FolderTree::sortSubTree( HTREEITEM item )
{
	BW_GUARD;

	TVSORTCB sortCB;
	sortCB.hParent = item;
	sortCB.lpfnCompare = FolderTreeCompareFunc;
	sortCB.lParam = 0;
	SortChildrenCB( &sortCB );
}


/**
 *	This method sets a tree view item's data to it's corresponding
 *	VFolderItemData info object.
 *
 *	@param item		Tree view item to add the data to.
 *	@param data		Item's data, in the form of a VFolderItemData object.
 */
void FolderTree::setItemData( HTREEITEM item, VFolderItemDataPtr data )
{
	BW_GUARD;

	SetItemData( item, (DWORD_PTR)data.getObject() );

	if ( data )
	{
		data->setTreeItem( item );
		itemDataHeap_.push_back( data );
	}
}


/**
 *	This method gathers items for a folder by iterating with its provider.
 *
 *	@param parentItem	Tree view item corresponding to the folder.
 *	@param provider		VFolderProvider object providing the child items.
 */
void FolderTree::buildTree( HTREEITEM parentItem, VFolderProviderPtr provider )
{
	BW_GUARD;

	if ( !provider )
		return;

	VFolderItemDataPtr parentData = (VFolderItemData*)GetItemData( parentItem );
	VFolderItemDataPtr data;

	if ( !provider->startEnumChildren( parentData ) )
		return;

	CWaitCursor wait;

	CImage img;
	// get children
	while( data = provider->getNextChild( *thumbnailManager_, img ) )
	{
		int icon;
		int iconSel;
		if ( !img.IsNull() )
		{
			icon = addBitmap( (HBITMAP)img );
			iconSel = icon;
			img.Destroy();
		}
		else
			getGroupIcons( data->getGroup(), icon, iconSel, data->getExpandable() );

		BW::wstring name = data->assetInfo().text().c_str();
		HTREEITEM child = GetChildItem( parentItem );
		while ( child )
		{
			if ( wcsicmp( name.c_str(), GetItemText( child ) ) == 0 )
				break;
			child = GetNextItem( child, TVGN_NEXT );
		}

		bool dontAdd = false;
		if ( child )
		{
			VFolderItemDataPtr oldData = (VFolderItemData*)GetItemData( child );
			dontAdd = oldData->handleDuplicate( data );
		}
		if ( !dontAdd )
		{
			HTREEITEM item = InsertItem( data->assetInfo().text().c_str(), icon, iconSel, parentItem );
			setItemData( item, data );
			if ( getVFolder( data.getObject() )->getSortSubFolders() )
				sortSubTree( parentItem );
			if ( data->getExpandable() )
				InsertItem( L"***dummychild***", 0, 0, item ); // later removed when expanded.
		}
	}
	if ( parentData && parentData->isVFolder() )
	{
		// custom items
		XmlItemVec* items = parentData->getVFolder()->getCustomItems();
		if ( items )
		{
			HTREEITEM topItem = TVI_FIRST;
			for( XmlItemVec::iterator i = items->begin();
				i != items->end(); ++i )
			{
				int icon = itemIcon_;
				int iconSel = itemIconSel_;
				thumbnailManager_->create( (*i).assetInfo().thumbnail(),
									img, IMAGE_SIZE, IMAGE_SIZE, this, true );
				data = new VFolderItemData( provider, (*i).assetInfo(), 0, false );
				data->isCustomItem( true );
				if ( !img.IsNull() )
				{
					icon = addBitmap( (HBITMAP)img );
					iconSel = icon;
					img.Destroy();
				}

				HTREEITEM item = InsertItem( data->assetInfo().text().c_str(),
					icon, iconSel, parentItem,
					(*i).position() == XmlItem::TOP ? topItem : TVI_LAST );
				if ( (*i).position() == XmlItem::TOP )
					topItem = item;
				setItemData( item, data );
			}
		}
	}
}


/**
 *	This method creates a new virtual folder and adds it to the appropriate
 *	place in the tree view.
 *
 *	@param displayName	Text label for the folder in the GUI.
 *	@param provider		Item list provider.
 *	@param parent		Parent tree view item, if any.
 *	@param icon			Image index for when the folder is not selected.
 *	@param iconSel		Image index for when the folder is selected.
 *	@param show			True if the folder should be visible straight away.
 *	@param expandable	True if it's got subitems to show, false otherwise.
 *	@param customItems	These are items that are not in the provider, but that
 *						still need to be part of this folder. For example,
 *						there is a "(Use game lighting)" custom item in Model
 *						Editor's asset browser "Lights" folder.
 *	@param data			Application-specific data pointer.
 *	@param subVFolders	True if it has nested child virtual folders.
 *	@return		Newly created virtual folder.
 */
VFolderPtr FolderTree::addVFolder(
	const BW::wstring& displayName,
	VFolderProviderPtr provider,
	VFolderPtr parent, HICON icon, HICON iconSel,
	bool show,
	bool expandable,
	XmlItemVec* customItems,
	void* data,
	bool subVFolders )
{
	BW_GUARD;

	if ( initialised_ )
		return 0;

	HTREEITEM item = 0;
	HTREEITEM parentItem = 0;
	if ( !!parent )
		parentItem = parent->getItem();

	if ( show )
	{
		int i;
		int is;

		getGroupIcons( 0, i, is, expandable );

		if ( icon )
		{
			i = addIcon( icon );
			if ( !iconSel )
				is = i;
		}
		if ( iconSel )
		{
			is = addIcon( iconSel );
		}

		item = InsertItem( displayName.c_str(), i, is, parentItem );
		setItemData( item, new VFolderItemData(
			provider, AssetInfo( L"", displayName.c_str(), L"" ), 0, true ) );
		if ( sortVFolders_ )
			sortSubTree( parentItem ); // sort siblings
	
		if ( expandable )
			InsertItem( L"***dummychild***", 0, 0, item ); // later removed when expanded.
	}

	VFolderPtr newVFolder = new VFolder( parent, displayName, item, provider, expandable, sortSubFolders_, customItems, data, subVFolders );
	if ( item )
	{
		VFolderItemData* data = (VFolderItemData*)GetItemData( item );
		data->setVFolder( newVFolder );
	}

	return newVFolder;
}


/**
 *	This method removes a virtual folder by name.
 *
 *	@param displayName	Virtual folder's name.
 *	@param curItem		Recursion param, tree view item to start searching at.
 */
void FolderTree::removeVFolder( const BW::wstring& displayName, HTREEITEM curItem )
{
	BW_GUARD;

	HTREEITEM child = GetChildItem( curItem );
	while ( child )
	{
		HTREEITEM next = GetNextItem( child, TVGN_NEXT );
		VFolderItemData* data = (VFolderItemData*)GetItemData( child );
		if ( data && data->isVFolder() )
		{
			if ( displayName == (LPCTSTR)GetItemText( child ) )
			{
				freeSubtreeData( child );
				DeleteItem( child );
				return;
			}
			else
				removeVFolder( displayName, child );
		}
		child = next;
	}
}


/**
 *	This method removes a virtual folder by tree view item handle.
 *
 *	@param item			Virtual folder tree view item handle.
 *	@param curItem		Recursion param, tree view item to start searching at.
 */
void FolderTree::removeVFolder( HTREEITEM item, HTREEITEM curItem )
{
	BW_GUARD;

	HTREEITEM child = GetChildItem( curItem );
	while ( child )
	{
		HTREEITEM next = GetNextItem( child, TVGN_NEXT );
		if ( child == item )
		{
			freeSubtreeData( child );
			DeleteItem( child );
			return;
		}
		else
			removeVFolder( item, child );
		child = next;
	}
}


/**
 *	This method clears the contents of the whole tree view control.
 */
void FolderTree::clear()
{
	BW_GUARD;

	freeSubtreeData( TVI_ROOT );
	DeleteAllItems();
}


/**
 *	This method removes a virtual folder by tree view item handle.
 *
 *	@param item			Virtual folder tree view item handle.
 *	@param curItem		Recursion param, tree view item to start searching at.
 */
void FolderTree::setSortVFolders( bool sort )
{
	BW_GUARD;

	sortVFolders_ = sort;
}


/**
 *	This method allows enabling or disabling the sorting of subfolders (which
 *	is a setting in the xml config file).
 *
 *	@param sort	True to sort subitems.
 */
void FolderTree::setSortSubFolders( bool sort )
{
	BW_GUARD;

	sortSubFolders_ = sort;
}


/**
 *	This method expands a folder and, if it's being expanded for the first
 *	time, it will call the relevant provider to fill it in with its items (on
 *	demand to avoid having to fill the whole tree on startup = very slow).
 *
 *	@param item	Tree view item corresponding to the folder.
 *	@return	True if the folder is empty to avoid further attempts of exanding.
 */
bool FolderTree::expandItem( HTREEITEM item )
{
	BW_GUARD;

	HTREEITEM child = GetChildItem( item );
	if ( child && GetItemText( child ).Compare(L"***dummychild***") == 0 ) {
		// it's the first time it's expanded, remove dummy item and build
		DeleteItem( child );
		VFolderItemDataPtr data = (VFolderItemData*)GetItemData( item );
		
		if ( !!data->getProvider() )
		{
			buildTree( item, data->getProvider() );

			HTREEITEM child = GetChildItem( item );
			if ( !child )
				return true; // avoid expanding
		}
	}
	return false;
}


// MFC messages
BEGIN_MESSAGE_MAP(FolderTree,CTreeCtrl)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT( TVN_SELCHANGED, OnSelChanged )
	ON_NOTIFY_REFLECT( NM_RCLICK, OnRightClick )
	ON_NOTIFY_REFLECT( TVN_ITEMEXPANDING, OnItemExpanding )
	ON_NOTIFY_REFLECT( TVN_BEGINDRAG, OnBeginDrag )
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


/**
 *	This MFC method is overriden to avoid flickering that occures with the
 *	default implementation.
 *
 *	@param pDC	Device context.
 *	@return	Returning FALSE.
 */
BOOL FolderTree::OnEraseBkgnd(CDC* pDC) 
{
	BW_GUARD;

	return FALSE;
}


/**
 *	This MFC method is overriden to avoid flickering by implementing double
 *	buffered drawing.
 */
void FolderTree::OnPaint() 
{
	BW_GUARD;

	CPaintDC dc(this);
	CRect rect;
	GetClientRect( &rect );
    controls::MemDC memDC;
	controls::MemDCScope memDCScope( memDC, dc, &rect );
	CWnd::DefWindowProc( WM_PAINT, (WPARAM)memDC.m_hDC, 0 );
}


/**
 *	This MFC method is overriden to handle special keys, such as "Delete" to 
 *	delete an item or a folder in the tree view (aapplication-dependent).
 *
 *	@param nChar	MFC char code.
 *	@param nRepCnt	MFC key repetition count.
 *	@param nFlags	MFC flags.
 */
void FolderTree::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	BW_GUARD;

	if ( nChar == VK_DELETE && eventHandler_ )
	{
		HTREEITEM item = GetSelectedItem();
		if ( item )
			eventHandler_->folderTreeItemDelete( (VFolderItemData*)GetItemData( item ) );
		return;
	}
	CTreeCtrl::OnKeyDown( nChar, nRepCnt, nFlags );
}


/**
 *	This MFC method is overriden to allow for additional application-specific
 *	handling of left-clicks on the tree view.
 *
 *	@param nFlags	MFC flags.
 *	@param point	MFC mouse position.
 */
void FolderTree::OnLButtonDown(UINT nFlags, CPoint point) 
{
	BW_GUARD;

	//select item
	HTREEITEM item = GetSelectedItem();
	CTreeCtrl::OnLButtonDown(nFlags, point);
	// send a synthetic select message when the user clicks an item and the
	// selection doesn't change
	UINT hitflags;
	if ( eventHandler_ && item && item == GetSelectedItem() && item == HitTest( point, &hitflags ) )
		eventHandler_->folderTreeSelect( (VFolderItemData*)GetItemData( item ) ); 
}


/**
 *	This MFC method is overriden to ensure that the item below the mouse when
 *	a right-click occurs is selected.
 *
 *	@param nFlags	MFC flags.
 *	@param point	MFC mouse position.
 */
void FolderTree::OnRButtonDown(UINT nFlags, CPoint point) 
{
	BW_GUARD;

	//select item
	UINT hitflags;
	HTREEITEM hitem = HitTest( point, &hitflags ) ;
	if ( hitflags & (TVHT_ONITEM | TVHT_ONITEMBUTTON  )) 
		SelectItem( hitem );
	CTreeCtrl::OnRButtonDown(nFlags, point);
}


/**
 *	This MFC method is overriden to allow for additional application-specific
 *	handling of selection change eevents on the tree view.
 *
 *	@param pNotifyStruct	MFC notify struct.
 *	@param result	MFC result.
 */
void FolderTree::OnSelChanged( NMHDR * pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

	if ( eventHandler_ )
	{
		HTREEITEM item = GetSelectedItem();
		if ( item )
			eventHandler_->folderTreeSelect( (VFolderItemData*)GetItemData( item ) );
	}
}


/**
 *	This MFC method handles right-clicks and relays the event to the
 *	application-defined callback.
 *
 *	@param pNotifyStruct	MFC notify struct.
 *	@param result	MFC result.
 */
void FolderTree::OnRightClick( NMHDR * pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

	*result = 1;
	if ( eventHandler_ )
	{
		CPoint pt;
		GetCursorPos( &pt );
		ScreenToClient( &pt );
		UINT hitflags;
		HTREEITEM item = HitTest( pt, &hitflags ) ;
		VFolderItemData* data = 0;
		if ( item )
			data = (VFolderItemData*)GetItemData( item );
		eventHandler_->folderTreeRightClick( data );
	}
}


/**
 *	This MFC method makes sure a subtree gets populated when the user attempts
 *	to expand a folder.
 *
 *	@param pNotifyStruct	MFC notify struct.
 *	@param result	MFC result.
 */
void FolderTree::OnItemExpanding( NMHDR * pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

	LPNMTREEVIEW ns = (LPNMTREEVIEW) pNotifyStruct;

	*result = FALSE;

	if ( ns->action == TVE_EXPAND || ns->action == TVE_EXPANDPARTIAL )
		*result = expandItem( ns->itemNew.hItem ) ? TRUE : FALSE;
}


/**
 *	This MFC method handles the begining of a drag&drop operation by creating
 *	an image of the element to drag and relaying the event to the application.
 *
 *	@param pNMHDR	MFC notify message header.
 *	@param pResult	MFC result.
 */
void FolderTree::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	BW_GUARD;

	*pResult = 0;

	if ( !eventHandler_ )
		return;

	LPNMTREEVIEW info = (LPNMTREEVIEW) pNMHDR;
	TVITEM item = info->itemNew;

	SelectItem( item.hItem );

	POINT pt;
	GetCursorPos( &pt );
	bw_safe_delete(dragImgList_);
	bw_safe_delete(dragImage_);
	dragImgList_ = CreateDragImage( item.hItem );  // get the image list for dragging
	dragImgList_->SetBkColor( imgList_.GetBkColor() );
	CPoint offset( IMAGE_SIZE, IMAGE_SIZE );
	dragImage_ = new controls::DragImage( *dragImgList_, pt, offset );

	dragging_ = true;
	eventHandler_->folderTreeStartDrag( (VFolderItemData*)GetItemData( item.hItem ) );
}


/**
 *	This MFC method relays double-click events to the application.
 *
 *	@param nFlags	MFC flags.
 *	@param point	MFC mouse position.
 */
void FolderTree::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	BW_GUARD;

	if ( !eventHandler_ )
		return;

	UINT hitflags;
	HTREEITEM item = HitTest( point , &hitflags );
	if ( item && ( hitflags & TVHT_ONITEM ))
		eventHandler_->folderTreeDoubleClick( (VFolderItemData*)GetItemData( item ) );
}


/**
 *	This method tells whether or not a drag&drop operation is under way.
 *
 *	@return	True if the user is dragging an item, false otherwise.
 */
bool FolderTree::isDragging()
{
	BW_GUARD;

	return dragging_;
}


/**
 *	This method is called each frame when performing a drag & drop operation.
 *
 *	@param x	Mouse X coordinate.
 *	@param Y	Mouse Y coordinate.
 */
void FolderTree::updateDrag( int x, int y )
{
	BW_GUARD;

	POINT pt = { x, y };
	dragImage_->update( pt );
}


/**
 *	This method is be called at the end of a drag & drop operation.
 */
void FolderTree::endDrag()
{
	BW_GUARD;

	bw_safe_delete(dragImage_);
	bw_safe_delete(dragImgList_);
	dragging_ = false;
}


/**
 *	This method can be called to refresh a single item, for example, when
 *	externally updating its thumbnail image.
 *
 *	@param assetInfo	Information about the item.
 *	@param item			RecurseTree view item handle.
 *	@return	True if the item was update successfully.
 */
bool FolderTree::updateItem( const AssetInfo& assetInfo, HTREEITEM item )
{
	BW_GUARD;

	// Static variable that will contain the thumbnail for the asset, so
	// we can get the thumbnail once and use it for all tree items that
	// point to this same asset.
	static CImage img;

	if ( item != TVI_ROOT )
	{
		VFolderItemData* data = (VFolderItemData*)GetItemData( item );
		if ( data )
		{
			BW::wstring infLongText = data->assetInfo().longText();
			BW::wstring assetInfoLongText = assetInfo.longText();

			if ( data &&
				( assetInfo.text() == L"" || data->assetInfo().text() == assetInfo.text() ) &&
				wcsicmp( infLongText.c_str(), assetInfoLongText.c_str() ) == 0 )
			{
				// Found. Just refresh the it's thumbnail
				if ( img.IsNull() )
					data->getProvider()->getThumbnail( *thumbnailManager_, data, img );
				int icon;
				int iconSel;
				if ( !img.IsNull() )
				{
					icon = addBitmap( (HBITMAP)img );
					iconSel = icon;
				}
				else
					getGroupIcons( data->getGroup(), icon, iconSel, data->getExpandable() );
				SetItemImage( item, icon, iconSel );

				return false; // return false so it keeps looking
			}
		}
	}
	else
	{
		// init the static if we are at the root
		if ( !img.IsNull() )
			img.Destroy();
	}

	HTREEITEM child = GetChildItem( item );
	while ( child )
	{
		if ( updateItem( assetInfo, child ) )
			return true;
		child = GetNextItem( child, TVGN_NEXT );
	}

	if ( item == TVI_ROOT && !img.IsNull() )
	{
		// destroy the static image if we are at the root item 
		img.Destroy();
	}

	return false;
}


/**
 *	This method can be called to refresh a single item's thumbnail image.
 *
 *	@param longText	Description of the item, usually a file path.
 */
void FolderTree::thumbManagerUpdate( const BW::wstring& longText )
{
	BW_GUARD;

	if ( !GetSafeHwnd() || longText.empty() )
		return;

	BW::wstring longTextTmp = longText;
	std::replace( longTextTmp.begin(), longTextTmp.end(), L'/', L'\\' );

	updateItem( AssetInfo( L"", L"", longTextTmp ) );
}

BW_END_NAMESPACE

