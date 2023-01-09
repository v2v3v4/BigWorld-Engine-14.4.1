
#ifndef GUITABS_TAB_HPP
#define GUITABS_TAB_HPP

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *  This class encapsulates the functionality of a tab, which includes
 *  managing the layout of the content assigned to it.
 */
class Tab : public ReferenceCount
{
public:
	Tab( CWnd* parentWnd, BW::wstring contentID );
	Tab( CWnd* parentWnd, ContentPtr content );
	virtual ~Tab();

	virtual bool load( DataSectionPtr section );
	virtual bool save( DataSectionPtr section );

	virtual BW::wstring getDisplayString();
	virtual BW::wstring getTabDisplayString();
	virtual HICON getIcon();
	virtual CWnd* getCWnd();
	virtual bool isClonable();
	virtual void getPreferredSize( int& width, int& height );
	
	virtual bool isVisible();
	virtual void setVisible( bool visible );
	virtual void show( bool show );

	virtual ContentPtr getContent();

	virtual void handleRightClick( int x, int y );

protected:
	ContentPtr content_;
	bool isVisible_;

	virtual void construct( CWnd* parentWnd );
};


} // namespace
BW_END_NAMESPACE

#endif // GUITABS_TAB_HPP
