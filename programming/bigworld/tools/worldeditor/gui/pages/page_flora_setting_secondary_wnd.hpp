#ifndef PAGE_FLORA_SETTING_SACONDARY_WND_HPP
#define PAGE_FLORA_SETTING_SACONDARY_WND_HPP


#include "afxwin.h"
#include "afxcmn.h"
#include "list_cache.hpp"
#include "smart_list_ctrl.hpp"
#include "controls/image_button.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/image_control.hpp"
#include "filters_ctrl.hpp"

BW_BEGIN_NAMESPACE

class FloraSettingTree;
class FloatSection;
class PageFloraSetting;
class FloraSettingTreeSection;


/**
 *  This window handle float property's editing
 *	it's included in FloraSettingPropertiesWnd.
 */
class FloraSettingFloatBarWnd : 
	public CDialog
{
	DECLARE_DYNAMIC(FloraSettingFloatBarWnd)

public:
	enum { IDD = IDD_FLORA_SETTING_FLOAT_WND };

	FloraSettingFloatBarWnd( CWnd* pParent = NULL);
	void init( PageFloraSetting* pPageWnd );
	virtual void onActive( HTREEITEM hItem );
	HTREEITEM item()	{ return hItem_; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	DECLARE_MESSAGE_MAP()
private:
	CSliderCtrl sliderBar_;
	controls::EditNumeric valueEdit_;
	float min_;
	float max_;	
	float value_;

	FloraSettingTreeSection* pSection_;
	PageFloraSetting* pPageWnd_;
	HTREEITEM hItem_;
	bool changeFromScrollBar_;
	bool changeFromScrollBarConfirmed_;

public:
	afx_msg void OnFloatSliderValueChanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void OnEnChangeValueEdit();

};


/**
 *  This window display the available assets
 *	and handle drag interraction.
 */
class FloraSettingAssetWnd : 
	public CDialog,
	public FiltersCtrlEventHandler
{
	DECLARE_DYNAMIC(FloraSettingAssetWnd)

public:
	// Dialog Data
	enum { IDD = IDD_FLORA_SETTING_ASSET_WND };

	FloraSettingAssetWnd( PageFloraSetting* pPageWnd, CWnd* pParent = NULL);
	SmartListCtrl&	assetList()	{ return assetList_; }
	FilterHolder& filterHolder()	{ return filterHolder_; }
	void onActive( HTREEITEM hItem );
	void onDeactive( HTREEITEM hItem );

	int minimumWidth();
	int minimumHeight();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void filterClicked( const wchar_t* name, bool pushed, void* data );

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg LRESULT OnSearchFieldChanged( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()

private:
	SmartListCtrl				assetList_;
	SearchField					search_;
	FilterHolder				filterHolder_;
	FiltersCtrl					filtersCtrl_;
	ListProviderPtr				assetListprovider_;

	PageFloraSetting*			pPageWnd_;
	FloraSettingTreeSection*	pSection_;
	HTREEITEM					hItem_;

	void layOut();
	void buildAssetListFilters();

};


/**
 *  This window display already exist properties.
 */
class FloraSettingPropertiesWnd: public CDialog
{
	DECLARE_DYNAMIC(FloraSettingPropertiesWnd)

public:
	// Dialog Data
	enum { IDD = IDD_FLORA_SETTING_PROPERTIES_WND };

	FloraSettingPropertiesWnd( PageFloraSetting* pPageWnd, CWnd* pParent = NULL );

	SmartListCtrl&	existingResourceList()	{ return existingResourceList_; }
	void onActive( HTREEITEM hItem );
	void onDeactive( HTREEITEM hItem );
	void updateCurDisplayImage();

	int minimumWidth();
	int minimumHeight();
	
private:
	SmartListCtrl				existingResourceList_;
	ListProviderPtr				existingResourceListprovider_;

	controls::ImageControl32	curDisplayImage_;
	bool						showCurDisplayResource_;

	bool						showError_;

	#define MAX_FLOAT_BAR_WND 5
	FloraSettingFloatBarWnd		floatBarWnds[ MAX_FLOAT_BAR_WND ];
	int							usedFloatBarWnds_;

	PageFloraSetting*			pPageWnd_;
	FloraSettingTreeSection*	pSection_;
	HTREEITEM					hItem_;


	void layOut();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

};


/**
 *  This window is the right window who includes 
 *  FloraSettingAssetWnd and FloraSettingPropertiesWnd.
 */
class FloraSettingSecondaryWnd: public CWnd
{

public:
	FloraSettingSecondaryWnd( PageFloraSetting* pPageWnd );
	SmartListCtrl&	assetList(){ return assetListWnd_.assetList(); }
	FilterHolder& filterHolder()	{ return assetListWnd_.filterHolder(); }

	SmartListCtrl&	existResourceList()
					{ return propertiesWnd_.existingResourceList(); }

	FloraSettingPropertiesWnd& propertiesWnd() { return propertiesWnd_;	}

	void onActive( HTREEITEM hItem );
	void onDeactive( HTREEITEM hItem );
	void listItemToolTip( int index, BW::wstring& info );
	HTREEITEM curItem()	{ return hItem_; }

	int minimumWidth();
	int minimumHeight();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );

	DECLARE_MESSAGE_MAP()

private:

	FloraSettingAssetWnd		assetListWnd_;
	NiceSplitterWnd				splitterBar_;
	FloraSettingPropertiesWnd	propertiesWnd_;

	PageFloraSetting*			pPageWnd_;
	FloraSettingTreeSection*	pSection_;
	HTREEITEM					hItem_;
	bool						initialized_;

	void layOut();
	void init();
};

BW_END_NAMESPACE

#endif // PAGE_FLORA_SETTING_SACONDARY_WND_HPP
