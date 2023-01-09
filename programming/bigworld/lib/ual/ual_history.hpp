
#ifndef UAL_HISTORY_HPP
#define UAL_HISTORY_HPP

#include "resmgr/datasection.hpp"
#include "xml_item_list.hpp"
#include "asset_info.hpp"
#include "ual_callback.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a history manager for the Asset Browser dialog. It
 *	derives from XmlItemList which implements the actual adding and removing of
 *	xml items to an xml list.
 */
class UalHistory : public XmlItemList
{
public:
	UalHistory();
	virtual ~UalHistory();

	void setChangedCallback( UalCallback0* callback ) { changedCallback_ = callback; };

	// to add an item to history, prepareItem on EndDrag and addPreparedItem
	// after your app thinks the operation was successful.
	// NOTE: You should take advantage of the fact that an AssetInfo object
	// can get automatically converted to an XmlItem object.
	void prepareItem( const XmlItem& item );
	bool addPreparedItem();
	void discardPreparedItem();
	const XmlItem getPreparedItem();

	// shouldn't use this one. The preferred method is prepareItem + addPreparedItem
	DataSectionPtr add( const XmlItem& item );

	void setMaxItems( int maxItems ) { maxItems_ = maxItems; };
	int getMaxItems() { return maxItems_; };
	void remove( const XmlItem& item, bool callCallback = true );
	void clear();

private:
	// history notifications
	SmartPointer<UalCallback0> changedCallback_;

	int maxItems_;
	bool preparedItemValid_;
	XmlItem preparedItem_;

	void saveTimestamp( DataSectionPtr ds );
	time_t loadTimestamp( DataSectionPtr ds );
};


BW_END_NAMESPACE

#endif // UAL_HISTORY_HPP
