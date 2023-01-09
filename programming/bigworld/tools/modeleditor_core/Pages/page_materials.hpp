#pragma once
#include "resource.h"

#include "common/property_table.hpp"
#include "controls/auto_tooltip.hpp"
#include "editor_shared/pages/gui_tab_content.hpp"
#include "resmgr/string_provider.hpp"
#include "moo/forward_declarations.hpp"
#include "common/material_properties.hpp"
#include "math/rectt.hpp"

BW_BEGIN_NAMESPACE

class UalItemInfo;

typedef std::pair < BW::string , BW::string > StringPair;

namespace Moo
{
	class Visual;
	typedef SmartPointer< class Visual > VisualPtr;
	class EffectMaterial;
}

// PageMaterials

class PageMaterials
	: public PropertyTable
	, public MaterialPropertiesUser
	, public GuiTabContent
{
	IMPLEMENT_BASIC_CONTENT( 
		Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/SHORT_NAME"), 
		Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/LONG_NAME"), 
		285, 800, NULL )

	DECLARE_AUTO_TOOLTIP( PageMaterials, PropertyTable )

public:
	PageMaterials();
	virtual ~PageMaterials();

	static PageMaterials* currPage();
	
	//These are exposed to python as:
	void tintNew();			// newTint()
	void mfmLoad();			// loadMFM()
	void mfmSave();			// saveMFM()
	void tintDelete();		// deleteTint()

	bool canTintDelete();	// canDeleteTint()


// Dialog Data
	enum { IDD = IDD_MATERIALS }; 

protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
	virtual BOOL OnInitDialog();

	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	void OnUpdateMaterialPreview();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

private:
	
	SmartPointer< struct PageMaterialsImpl > pImpl_;
	
	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

	void OnSize( UINT nType, int cx, int cy );
	
	afx_msg LRESULT OnChangePropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRightClkPropertyItem(WPARAM wParam, LPARAM lParam);

	bool clearCurrMaterial();
		
	void drawMaterialsList();

	void redrawList( CComboBox& list, const BW::string& name, bool sel );

public:

	afx_msg void OnTvnItemexpandingMaterialsTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnSelchangedMaterialsTree(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnEnChangeMaterialsMaterial();
	afx_msg void OnEnKillfocusMaterialsMaterial();

	afx_msg void OnEnChangeMaterialsMatter();
	afx_msg void OnEnKillfocusMaterialsMatter();

	afx_msg void OnEnChangeMaterialsTint();
	afx_msg void OnEnKillfocusMaterialsTint();

	afx_msg void OnCbnSelchangeMaterialsFxList();
	afx_msg void OnBnClickedMaterialsFxSel();

	bool changeShader( const BW::wstring& fxFile );
	bool changeShaderDrop( UalItemInfo* ii );

	bool changeMFM( const BW::wstring& mfmFile );
	bool changeMFMDrop( UalItemInfo* ii );

	RectInt dropTest( UalItemInfo* ii );
	bool doDrop( UalItemInfo* ii );
	
	afx_msg void OnCbnSelchangeMaterialsPreviewList();
	afx_msg void OnBnClickedMaterialsPreview();

	bool previewMode();
	Moo::VisualPtr previewObject();

	void restoreView();

	Moo::ComplexEffectMaterialPtr currMaterial();

	const BW::wstring & materialName() const;
	const BW::wstring & matterName() const;
	const BW::wstring & tintName() const;

	// MaterialPropertiesUser interface
	void proxySetCallback();

	BW::string textureFeed( const BW::string& descName ) const;

	BW::string defaultTextureDir() const;

	BW::string exposedToScriptName( const BW::string& descName ) const;	

	StringProxyPtr textureProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnTex getFn, MaterialPropertiesUser::SetFnTex setFn, const BW::string& uiName, const BW::string& descName ) const;

	BoolProxyPtr boolProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnBool getFn, MaterialPropertiesUser::SetFnBool setFn, const BW::string& uiName, const BW::string& descName ) const;
	
	IntProxyPtr enumProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnEnum getFn, MaterialPropertiesUser::SetFnEnum setFn, const BW::string& uiName, const BW::string& descName ) const;

	IntProxyPtr intProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnInt getFn, MaterialPropertiesUser::SetFnInt setFn, MaterialPropertiesUser::RangeFnInt rangeFn, const BW::string& uiName,
		const BW::string& descName ) const;

	FloatProxyPtr floatProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnFloat getFn, MaterialPropertiesUser::SetFnFloat setFn, MaterialPropertiesUser::RangeFnFloat rangeFn, const BW::string& uiName,
		const BW::string& descName ) const;

	Vector4ProxyPtr vector4Proxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnVec4 getFn, MaterialPropertiesUser::SetFnVec4 setFn, const BW::string& uiName, const BW::string& descName ) const;

};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageMaterials )
BW_END_NAMESPACE

