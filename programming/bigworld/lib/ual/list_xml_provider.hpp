
#ifndef LIST_XML_PROVIDER_HPP
#define LIST_XML_PROVIDER_HPP

#include "list_cache.hpp"
#include "filter_spec.hpp"
#include "smart_list_ctrl.hpp"
#include "atlimage.h"

BW_BEGIN_NAMESPACE

/**
 *	This class derives from ListProvider to implement an XML virtual list
 *	provider.
 */
class ListXmlProvider : public ListProvider
{
public:
	ListXmlProvider();
	virtual ~ListXmlProvider();

	virtual void init( const BW::wstring& fname );

	virtual void refresh();

	virtual bool finished();

	virtual int getNumItems();

	virtual	const AssetInfo getAssetInfo( int index );
	virtual void getThumbnail( ThumbnailManager& manager,
								int index, CImage& img, int w, int h,
								ThumbnailUpdater* updater );

	virtual void filterItems();

	// additional interface
	bool errorLoading() { return errorLoading_; };

protected:
	static const int VECTOR_BLOCK_SIZE = 1000;
	typedef SmartPointer<AssetInfo> ListItemPtr;
	BW::vector<ListItemPtr> items_;
	BW::vector<ListItemPtr> searchResults_;
	typedef BW::vector<ListItemPtr>::iterator ItemsItr;
	BW::wstring path_;
	bool errorLoading_;

	static bool s_comparator( const ListItemPtr& a, const ListItemPtr& b );

	void clearItems();

	void refreshPurge( bool purge );
};

typedef SmartPointer<ListXmlProvider> ListXmlProviderPtr;

/**
 *	This class derives from ListXmlProvider, but the path specified
 *	doesn't have to be a file name, can be a section under an xml file
 *	ie.: "a.xml/section_b"
 */
class ListXmlSectionProvider: public ListXmlProvider
{
public:
	void init( const BW::wstring& path );
};
typedef SmartPointer<ListXmlSectionProvider> ListXmlSectionProviderPtr;

BW_END_NAMESPACE

#endif // LIST_XML_PROVIDER_HPP
