
#ifndef __CONTENT_CONTAINER_HPP__
#define __CONTENT_CONTAINER_HPP__

#include "datatypes.hpp"
#include "content.hpp"
#include "content_factory.hpp"

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *	ContentContainer class.
 *	Implements a panel that contains other Content-inheriting classes, useful
 *  in making switching between different contents modal. It basically behaves
 *  as a single tab that changes its content dynamically.
 */
class ContentContainer : public CDialog, public Content
{
public:
	static const BW::wstring contentID;

	ContentContainer();
	~ContentContainer();

	// ContentContainer methods
	void addContent( ContentPtr content );
	void addContent( BW::wstring subcontentID );
	void currentContent( ContentPtr content );
	void currentContent( BW::wstring subcontentID );
	void currentContent( int index );
	ContentPtr currentContent();

	bool contains( ContentPtr content );
	int contains( const BW::wstring subcontentID );
	ContentPtr getContent( const BW::wstring subcontentID );
	ContentPtr getContent( const BW::wstring subcontentID, int& index );

	void broadcastMessage( UINT msg, WPARAM wParam, LPARAM lParam );

	// Content methods
	BW::wstring getContentID();
	bool load( DataSectionPtr section );
	bool save( DataSectionPtr section );
	BW::wstring getDisplayString();
	BW::wstring getTabDisplayString();
	HICON getIcon();
	CWnd* getCWnd();
	void getPreferredSize( int& width, int& height );
	OnCloseAction onClose( bool isLastContent );
	void handleRightClick( int x, int y );
	// clone not supported
	ContentPtr clone() { return 0; }
	bool isClonable() { return false; }

private:
	typedef BW::vector<ContentPtr> ContentVec;
	typedef ContentVec::iterator ContentVecIt;
	ContentVec contents_;
	ContentPtr currentContent_;

	void createContentWnd( ContentPtr content );

	// MFC needed overrides
	void OnOK() {}

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	DECLARE_MESSAGE_MAP();
};


/**
 *	This class implementes the factor for a ContentContainer object.
 */
class ContentContainerFactory : public ContentFactory
{
public:
	ContentPtr create() { return new ContentContainer(); }
	BW::wstring getContentID() { return ContentContainer::contentID; }
};

typedef SmartPointer<ContentContainer> ContentContainerPtr;


} // namespace
BW_END_NAMESPACE

#endif // __CONTENT_CONTAINER_HPP__
