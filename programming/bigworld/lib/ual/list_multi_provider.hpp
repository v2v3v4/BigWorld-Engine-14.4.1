
#ifndef LIST_MULTI_PROVIDER_HPP
#define LIST_MULTI_PROVIDER_HPP

#include "list_cache.hpp"
#include "filter_spec.hpp"
#include "smart_list_ctrl.hpp"
#include "atlimage.h"

BW_BEGIN_NAMESPACE

/**
 *	This class derives from ListProvider to implement a list provider that
 *	manages one or more sub-providers, allowing multiple asset sources to be
 *	shown under one Asset Browser folder.
 */
class ListMultiProvider : public ListProvider
{
public:
	ListMultiProvider();
	virtual ~ListMultiProvider();

	virtual void refresh();

	virtual bool finished();

	virtual int getNumItems();

	virtual	const AssetInfo getAssetInfo( int index );
	virtual void getThumbnail( ThumbnailManager& manager,
								int index, CImage& img, int w, int h,
								ThumbnailUpdater* updater );

	virtual void filterItems();

	// additional interface
	void addProvider( ListProviderPtr provider );

private:
	typedef BW::vector<ListProviderPtr> ProvVec;
	ProvVec providers_;


	/**
	 *	This internal class caches a list item, also storing information about
	 *	it's source provider.
	 */
	class ListItem
	{
	public:
		ListItem( ListProviderPtr provider, int index );

		ListProviderPtr provider() const { return provider_; }
		int index() const { return index_; }

		const wchar_t* text() const;

	private:
		ListProviderPtr provider_;
		int index_;
		mutable BW::wstring text_;
		mutable bool inited_;
	};


	BW::vector<ListItem> items_;
	int lastNumItems_;

	static bool s_comparator( const ListItem& a, const ListItem& b );

	void updateItems();
	void fillItems();
};

BW_END_NAMESPACE

#endif // LIST_MULTI_PROVIDER_HPP
