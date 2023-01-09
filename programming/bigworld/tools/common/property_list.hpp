#pragma once

#include "resmgr/string_provider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/slider.hpp"
#include "cstdmf/bw_functor.hpp"
#include "gizmo/general_properties.hpp"
#include <stack>

BW_BEGIN_NAMESPACE

class PropertyList;

// PropertyItem
class PropertyItem
{
public:
	PropertyItem(const Name& name);
	virtual ~PropertyItem() {}

	virtual void create(PropertyList* parent) = 0;
	virtual void select(CRect rect, bool showDropDown = true) = 0;
	virtual void deselect() = 0;
	virtual bool hasSameValue( PropertyItem* item ) { return value() == item->value(); }

	virtual Name name() const;
	Name fullName() const;
	void multipleValue( bool flag ) { multipleValue_ = flag; }
	bool multipleValue() const { return multipleValue_; }
	CString value() const { return stringValue_; }
	CString displayValue() const;

	void setSelectable(bool option) { selectable_ = option; }
	bool getSelectable() const { return selectable_; }

	void setChangeBuddy(void* buddy) { changeBuddy_ = buddy; }
	void * getChangeBuddy() { return changeBuddy_; }

	virtual controls::EditNumeric* ownEdit() { return NULL; }
		
	virtual void comboChange() {}
	virtual void onBrowse() {}
	virtual void sliderChange( int value, bool transient ) {}
	virtual void editChange() {}
	virtual void onDefault() {}
	virtual void onKeyDown( UINT key ) {}
	virtual void onCustom( UINT nID ) {}
	virtual BW::wstring menuOptions() { return L""; }
	virtual BW::wstring textureFeed() { return L""; }

	enum ItemType
	{
		Type_Unknown,
		Type_Group,
		Type_Colour,
		Type_ColourScalar,
		Type_Vector,
		Type_Label,
		Type_Label_Highlight,
		Type_String,
		Type_String_ReadOnly,
		Type_ID
	};
	virtual ItemType getType() const { return Type_Unknown; }

	bool isGroupType() const;
	
	virtual const Name& descName() const { return descName_; }
	virtual void descName( const Name& desc) { descName_ = desc; }

	virtual BW::string UIDescL() const { return LocaliseUTF8( uiDesc_.c_str() ); }
	virtual const char* UIDesc() const { return uiDesc_.c_str(); }
	virtual void UIDesc( const Name& desc) { uiDesc_ = desc; }

	virtual const Name& exposedToScriptName() const { return exposedToScriptName_; }
	virtual void exposedToScriptName( const Name& name) { exposedToScriptName_ = name; }

	virtual void canExposeToScript( bool canExposeToScript ) { canExposeToScript_ = canExposeToScript; }
	virtual bool canExposeToScript() const { return canExposeToScript_; }

	virtual BW::wstring UIDescExtra() const;

	void setGroup( const Name& group );
	const Name& getGroup() const { return group_; }
	void setGroupDepth( int depth ) { groupDepth_ = depth; }
	int getGroupDepth() const { return groupDepth_; }

	void arrayData( int arrayIndex, BWBaseFunctor1<int>* arrayCallback, bool arrayReadOnly )
	{
		arrayIndex_ = arrayIndex;
		arrayCallback_ = arrayCallback;
		arrayReadOnly_ = arrayReadOnly;
	}
	int arrayIndex() const {	return arrayIndex_;	}
	BWBaseFunctor1<int>* arrayCallback() {	return arrayCallback_.getObject();	}
	bool arrayReadOnly() const	{	return arrayReadOnly_;	}

protected:
	Name name_;
	CString stringValue_;
	bool multipleValue_;

	Name descName_;
	Name uiDesc_;
	Name exposedToScriptName_;

	bool canExposeToScript_;

	bool selectable_;

	PropertyList* parent_;
	void* changeBuddy_;

	Name group_;
	int groupDepth_;

	int arrayIndex_;
	SmartPointer< BWBaseFunctor1<int> > arrayCallback_;
	bool arrayReadOnly_;
};

typedef BW::vector< PropertyItem * > PropertyItemVector;


/**
 *	Small class that detects when a CEdit has been touched (a key was
 *	pressed on it).
 */
class ChangeDetectEdit : public CEdit
{
public:
	ChangeDetectEdit() : touched_( false )
	{
	}

	void touched( bool val )	{ touched_ = val; }
	bool touched() const		{ return touched_; }

protected:
	afx_msg void OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
	{
		touched_ = true;
		CEdit::OnChar( nChar, nRepCnt, nFlags );
	}
	DECLARE_MESSAGE_MAP()

private:
	bool touched_;
};


class GroupPropertyItem : public PropertyItem
{
public:
	GroupPropertyItem(const Name& name, int depth);
	virtual ~GroupPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	virtual ItemType getType() const { return Type_Group; }

	void addChild( PropertyItem * item );

	PropertyItemVector & getChildren() { return children_; }

	void setExpanded( bool option ) { expanded_ = option; }
	bool getExpanded() const { return expanded_; }

	void setGroupDepth( int depth ) { groupDepth_ = depth; }

protected:
	PropertyItemVector children_;
	bool expanded_;
};

class ColourPropertyItem : public GroupPropertyItem
{
public:
	ColourPropertyItem(const Name& name, const CString& init, int depth, bool colour = true);
	virtual ~ColourPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void set(const BW::wstring& value);
	BW::wstring get() const;

	virtual ItemType getType() const { return colour_ ? Type_Colour : Type_Vector; }

	virtual void onBrowse();
	virtual BW::wstring menuOptions();

private:
	static BW::map<CWnd*, ChangeDetectEdit*> edit_;
	static BW::map<CWnd*, CButton*> button_;
	bool colour_;
};


class ColourScalarPropertyItem : public GroupPropertyItem
{
public:
	ColourScalarPropertyItem(const Name& name, const CString& init, int depth);
	virtual ~ColourScalarPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void set(const BW::string& value);
	BW::string get() const;

	virtual ItemType getType() const { return Type_ColourScalar; }

	virtual void onBrowse();
	virtual BW::wstring menuOptions();

private:
	static BW::map<CWnd*, ChangeDetectEdit*> edit_;
	static BW::map<CWnd*, CButton*> button_;	
};


class LabelPropertyItem : public PropertyItem
{
public:
	LabelPropertyItem(const Name& name, bool highlight = false);
	virtual ~LabelPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	virtual ItemType getType() const;

private:
	bool highlight_;
};


class StringPropertyItem : public PropertyItem
{
public:
	StringPropertyItem(const Name& name, const CString& currentValue, bool readOnly = false);
	virtual ~StringPropertyItem();

	virtual Name name();
	
	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void set(const BW::wstring& value);
	BW::wstring get() const;
	
	void fileFilter( const Name& filter ) { fileFilter_ = filter; }
	const Name& fileFilter() const { return fileFilter_; }

	void defaultDir( const BW::wstring& dir ) { defaultDir_ = dir; }
	BW::wstring defaultDir() const { return defaultDir_; }

	void canTextureFeed( bool val ) { canTextureFeed_ = val; }
	bool canTextureFeed() const { return canTextureFeed_; }
	
	void textureFeed( const BW::wstring& textureFeed )	{ textureFeed_ = textureFeed; }
	BW::wstring textureFeed() const	{ return textureFeed_; }

	BW::wstring UIDescExtra() const
	{
		if ( !canTextureFeed_ ) return L"";

		return Localise(L"COMMON/PROPERTY_LIST/ASSIGN_FEED");
	}

	virtual void onBrowse();
	virtual BW::wstring menuOptions();

	virtual ItemType getType() const;
	bool isHexColor() const	{	return stringValue_.GetLength() == 7 && stringValue_[0] == '#';	}
	bool isVectColor() const { BW::wstring val = stringValue_; return std::count( val.begin( ), val.end( ), ',' ) == 2; }
private:
	static BW::map<CWnd*, ChangeDetectEdit*> edit_;
	static BW::map<CWnd*, CButton*> button_;
	Name fileFilter_;
	BW::wstring defaultDir_;
	bool canTextureFeed_;
	BW::wstring textureFeed_;
	bool readOnly_;
};


class IDPropertyItem : public PropertyItem
{
public:
	IDPropertyItem(const Name& name, const CString& currentValue);
	virtual ~IDPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void set(const BW::wstring& value);
	BW::wstring get() const;

	virtual ItemType getType() const;

private:
	static BW::map<CWnd*, ChangeDetectEdit*> edit_;
};


class ComboPropertyItem : public PropertyItem
{
public:
	ComboPropertyItem(const Name& name, CString currentValue,
				const BW::vector<BW::wstring>& possibleValues);
	ComboPropertyItem(const Name& name, int currentValueIndex,
				const BW::vector<BW::wstring>& possibleValues);
	virtual ~ComboPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void set(const BW::wstring& value);
	void set(int index);
	BW::wstring get() const;

	virtual void comboChange();

private:
	static BW::map<CWnd*, CComboBox*> comboBox_;
	BW::vector<BW::wstring> possibleValues_;
};


class BoolPropertyItem : public PropertyItem
{
public:
	BoolPropertyItem(const Name& name, int currentValue);
	virtual ~BoolPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void set(bool value);
	bool get() const;

	virtual void comboChange();

	virtual BW::wstring menuOptions();

private:
	static BW::map<CWnd*, CComboBox*> comboBox_;
	int value_;
};


class FloatPropertyItem : public PropertyItem
{
public:
	FloatPropertyItem(const Name& name, float currentValue);
	virtual ~FloatPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void setDigits( int digits );
	void setRange( float min, float max, int digits );
	void setDefault( float def );

	void set(float value);
	float get() const;

	virtual void sliderChange( int value, bool transient );
	virtual void editChange();
	virtual void onDefault();

	virtual BW::wstring menuOptions();

	virtual controls::EditNumeric* ownEdit();

private:
	static BW::map<CWnd*, controls::EditNumeric*> editNumeric_;
	static BW::map<CWnd*, controls::EditNumeric*> editNumericFormatting_;
	static BW::map<CWnd*, controls::Slider*> slider_;
	static BW::map<CWnd*, CButton*> button_;

	float value_;
	float min_;
	float max_;
	int digits_;
	bool ranged_;
	bool changing_;
	float def_;
	bool hasDef_;
};


class IntPropertyItem : public PropertyItem
{
public:
	IntPropertyItem(const Name& name, int currentValue);
	virtual ~IntPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void setRange( int min, int max );
	void set(int value);
	int get() const;

	virtual void sliderChange( int value, bool transient );
	virtual void editChange();

	virtual BW::wstring menuOptions();

	virtual controls::EditNumeric* ownEdit();

private:
	static BW::map<CWnd*, controls::EditNumeric*> editNumeric_;
	static BW::map<CWnd*, controls::EditNumeric*> editNumericFormatting_;
	static BW::map<CWnd*, controls::Slider*> slider_;
	int value_;
	int min_;
	int max_;
	bool ranged_;
	bool changing_;
};


class UIntPropertyItem : public PropertyItem
{
public:
	UIntPropertyItem(const Name& name, uint32 currentValue);
	virtual ~UIntPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	void setRange( uint32 min, uint32 max );
	void set(uint32 value);
	uint32 get() const;

	virtual void sliderChange( int value, bool transient );
	virtual void editChange();

	virtual BW::wstring menuOptions();

	virtual controls::EditNumeric* ownEdit();

private:
	static BW::map<CWnd*, controls::EditNumeric*> editNumeric_;
	static BW::map<CWnd*, controls::EditNumeric*> editNumericFormatting_;
	static BW::map<CWnd*, controls::Slider*> slider_;
	uint32 value_;
	uint32 min_;
	uint32 max_;
	bool ranged_;
	bool changing_;
};


class ArrayPropertyItem : public GroupPropertyItem
{
public:
	ArrayPropertyItem(const Name& name, const Name& group, const CString& str, ArrayProxyPtr proxy);
	virtual ~ArrayPropertyItem();

	virtual void create(PropertyList* parent);
	virtual void select(CRect rect, bool showDropDown = true);
	virtual void deselect();

	virtual void onCustom( UINT nID );

	virtual void clear();

private:
	ArrayProxyPtr proxy_;
	static BW::map<CWnd*, CButton*>				addButton_;
	static BW::map<CWnd*, CButton*>				delButton_;
};

class BasePropertyTable;

// PropertyList window

class PropertyList : public CListBox
{
public:
	static void mainFrame( CFrameWnd* mainFrame ) { mainFrame_ = mainFrame; }

	PropertyList( BasePropertyTable* propertyTable );
	virtual ~PropertyList();

	void enable( bool enable );

	void isSorted( bool sorted );

	void setArrayProperty( PropertyItem* item );

	int AddPropItem( PropertyItem* item, int forceIndex = -1 );

	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	void clear();

	bool changeSelectItem( int delta );
	bool selectItem( int itemIndex, bool changeFocus = true );
	void deselectCurrentItem();
	void needsDeselectCurrentItem() { needsDeselect_ = true; }
	void needsSelectPrevItem() { needsSelectPrevItem_ = true; }
	void needsSelectNextItem() { needsSelectNextItem_ = true; }
	static void deselectAllItems();
	static void update();
	void selectPrevItem();
	void selectNextItem();

	void setDividerPos( int x );

	PropertyItem * getHighlightedItem();

	void collapseGroup(GroupPropertyItem* gItem, int index, bool reselect = true);
	void expandGroup(GroupPropertyItem* gItem, int index);

	void startArray( BWBaseFunctor1<int>* callback, bool readOnly );
	void endArray();

	CRect dropTest( CPoint point, const BW::wstring& fileName );
	bool doDrop( CPoint point, const BW::wstring& fileName );

	static WCHAR s_tooltipBuffer_[512];
	
	INT_PTR OnToolHitTest(CPoint point, TOOLINFO * pTI) const;
	BOOL OnToolTipText( UINT id, NMHDR* pTTTStruct, LRESULT* pResult );

	BasePropertyTable* propertyTable() const	{	return propertyTable_;	}

	PropertyItem * selectedItem() const { return selectedItem_; }
	void selectedItem( PropertyItem * newSelectedItem ) { selectedItem_ = newSelectedItem; }

protected:
	virtual void PreSubclassWindow();

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnPaint();
	afx_msg void OnSelchange();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnChangePropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg void OnComboChange();
	afx_msg void OnBrowse();
	afx_msg void OnDefault();
	afx_msg void OnCustom(UINT nID);
	afx_msg void OnArrayDelete();
	afx_msg void OnEditChange( ); 
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point );
	afx_msg void OnRButtonUp( UINT, CPoint );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );

	DECLARE_MESSAGE_MAP()

private:
	static CFrameWnd* mainFrame_;

	typedef BW::set< PropertyList * > PropertyLists;
	static PropertyLists s_propertyLists_;

	void DrawDivider(int xpos);
	void selChange( bool showDropDown, bool changeFocus = true );
	void Select(int selected);

	CToolTipCtrl toolTip_;
	CString toolTipMsg_;

	bool enabled_;

	bool isSorted_;
	
	int selected_;
	PropertyItem * selectedItem_;
	bool needsDeselect_;
	bool needsSelectPrevItem_;
	bool needsSelectNextItem_;

	int dividerPos_;
	int dividerTop_;
	int dividerBottom_;
	int dividerLastX_;
	bool dividerMove_;
	HCURSOR cursorArrow_;
	HCURSOR cursorSize_;

	bool tooltipsEnabled_;

	BW::vector< GroupPropertyItem * > parentGroupStack_;

	bool delayRedraw_;

	std::stack<int> arrayIndex_;
	std::stack<int> arrayPropertItemIndex_;
	std::stack<SmartPointer< BWBaseFunctor1<int> > > arrayCallback_;
	std::stack<bool> arrayReadOnly_;
	static CButton s_arrayDeleteButton_;

	void establishGroup( PropertyItem* item );
	void makeSubGroup( const BW::string & subGroup, PropertyItem* item );
	void addGroupToStack( const Name& label, PropertyItem* item = NULL );

	int findSortedIndex( PropertyItem * item ) const;

	BasePropertyTable* propertyTable_;
};
BW_END_NAMESPACE

