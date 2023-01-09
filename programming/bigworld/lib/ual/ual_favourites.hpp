
#ifndef UAL_FAVOURITES_HPP
#define UAL_FAVOURITES_HPP

#include "resmgr/datasection.hpp"
#include "xml_item_list.hpp"
#include "ual_callback.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a favourites manager for the Asset Browser dialog. It
 *	derives from XmlItemList which implements the actual adding and removing of
 *	xml items to an xml list.
 */
class UalFavourites : public XmlItemList
{
public:
	UalFavourites();
	virtual ~UalFavourites();

	void setChangedCallback( UalCallback0* callback ) { changedCallback_ = callback; };

	// NOTE: You should take advantage of the fact that an AssetInfo object
	// can get automatically converted to an XmlItem object.
	DataSectionPtr add( const XmlItem& item );
	DataSectionPtr addAt(
		const XmlItem& item,
		const XmlItem& atItem );
	void remove( const XmlItem& item, bool callCallback = true );
	void clear();
private:
	// favourites notifications
	SmartPointer<UalCallback0> changedCallback_;
};

BW_END_NAMESPACE

#endif // UAL_FAVOURITES_HPP
