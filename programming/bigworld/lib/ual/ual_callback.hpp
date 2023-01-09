/**
 *	UalCallback: Contains all callback/functor related code
 */


#ifndef UAL_CALLBACK_HPP
#define UAL_CALLBACK_HPP

#include "asset_info.hpp"
#include "common/popup_menu.hpp"
#include "cstdmf/bw_functor.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

// Forward
class UalDialog;
class UalManager;


/**
 *	This class implements an Asset Browser item info container, used when
 *	notifying the application via the functors declared bellow.
 */
class UalItemInfo
{
public:
	// For now, it only accepts initialization when being constructed
	// or using the default assignment operator
	UalItemInfo() :
		dialog_( 0 ),
		x_( 0 ),
		y_( 0 ),
		isFolder_( false ),
		folderExtraData_( 0 ),
		next_( 0 )
	{
	}

	UalItemInfo(
		UalDialog* dialog,
		AssetInfo assetInfo,
		int x,
		int y,
		bool isFolder = false,
		void* folderData = 0 ) :
		dialog_( dialog ),
		assetInfo_( assetInfo ),
		x_( x ),
		y_( y ),
		isFolder_( isFolder ),
		folderExtraData_( folderData ),
		next_( 0 )
	{
	}

	~UalItemInfo() { bw_safe_delete(next_); }

	const UalDialog* dialog() const { return dialog_; }
	const AssetInfo& assetInfo() const { return assetInfo_; }
	int x() const { return x_; }
	int y() const { return y_; }
	bool isFolder() const { return isFolder_; }
	const BW::wstring& type() const { return assetInfo_.type(); }
	const BW::wstring& text() const { return assetInfo_.text(); }
	const BW::wstring& longText() const { return assetInfo_.longText(); }
	const BW::wstring& thumbnail() const { return assetInfo_.thumbnail(); }
	const BW::wstring& description() const { return assetInfo_.description(); }

	UalItemInfo* getNext() const { return next_; }

	UalItemInfo operator=( const UalItemInfo& other )
	{
		BW_GUARD;

		dialog_ = other.dialog_;
		assetInfo_ = other.assetInfo_;
		x_ = other.x_;
		y_ = other.y_;
		isFolder_ = other.isFolder_;
		folderExtraData_ = other.folderExtraData_;
		next_ = 0;
		return *this;
	}

	bool operator==( const UalItemInfo& other )
	{
		BW_GUARD;

		return dialog_ == other.dialog_ &&
			assetInfo_.equalTo( other.assetInfo_ ) &&
			isFolder_ == other.isFolder_ &&
			folderExtraData_ == other.folderExtraData_;
	}

private:
	friend UalDialog;
	friend UalManager;
	UalDialog* dialog_;
	AssetInfo assetInfo_;
	int x_;
	int y_;
	bool isFolder_;
	void* folderExtraData_;
	UalItemInfo* next_;

	void setNext( UalItemInfo* next ) { next_ = next; };
};


///////////////////////////////////////////////////////////////////////////////
//	Asset Browser functor objects for sending notifications to the app.
///////////////////////////////////////////////////////////////////////////////


// Typedefs
// types for the right-click callback ( use static methods in PopupMenu to add
// items to the vector )
typedef PopupMenu::Item UalPopupMenuItem;
typedef PopupMenu::Items UalPopupMenuItems;

//	drag&drop and click on an item in the list ( item info )
typedef BWBaseFunctor1< UalItemInfo* > UalItemCallback;

//	right-click on an item in the list ( item info, map to be filled with data )
typedef BWBaseFunctor2< UalItemInfo*, UalPopupMenuItems& > UalStartPopupMenuCallback;
typedef BWBaseFunctor2< UalItemInfo*, int > UalEndPopupMenuCallback;

// send errors to app ( error message string )
typedef BWBaseFunctor1< const BW::string& > UalErrorCallback;

// onSetFocus/KillFocus ( UalDialog ptr and focus state )
typedef BWBaseFunctor2< UalDialog*, bool > UalFocusCallback;

// Aliases to the BWFunctor classes.
#define UalCallback0	BWBaseFunctor0
#define UalCallback1	BWBaseFunctor1
#define UalCallback2	BWBaseFunctor2
#define UalFunctor0		BWFunctor0
#define UalFunctor1		BWFunctor1
#define UalFunctor2		BWFunctor2


BW_END_NAMESPACE

#endif // UAL_CALLBACK_HPP
