#pragma once

#include "pyscript/script_math.hpp"
#include "property_list.hpp"
#include "base_property_table.hpp"

#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "cstdmf/string_utils.hpp"
#include "math/vector4.hpp"

BW_BEGIN_NAMESPACE

void extractUnicode( PyObject* pValue, BW::wstring &s );


#define MILLIS_SINCE( T ) (float(((int64)(timestamp() - (T))) / stampsPerSecondD()) * 1000.f)


class BasePropertyTable;

class PropTable
{
public:
	static void table( BasePropertyTable* table ) { s_propTable_ = table; }
	static BasePropertyTable* table() { return s_propTable_; }
private:
	static BasePropertyTable* s_propTable_;
};


///////////////////////////////////////////////////////////////////////////////
// Section: PropTableSetter helper local class
///////////////////////////////////////////////////////////////////////////////
/**
 *	Changes the current property page the the one specified on construction and
 *  resets the previous property page on destruction.
 */
class PropTableSetter
{
public:
	PropTableSetter( BasePropertyTable* table ) :
		oldTable_( PropTable::table() )
	{
		BW_GUARD;

		PropTable::table( table );
	}

	~PropTableSetter()
	{
		BW_GUARD;

		PropTable::table( oldTable_ );
	}
private:
	BasePropertyTable* oldTable_;
};


class BaseView : public GeneralProperty::View
{
public:
	BaseView();

	typedef BW::vector<PropertyItem*> PropertyItems;
	PropertyItems& propertyItems() { return propertyItems_; }

	virtual void onChange( bool transient, bool addBarrier = true ) = 0;
	virtual bool updateGUI() = 0;
	virtual void cloneValue( BaseView* another ) = 0;
	virtual void addItem() {}
	virtual void delItems()	{}
	virtual void delItem( int index )
	{
		BW_GUARD;

		PropertyModifyGuard guard;

		for (size_t i = 0; i < propertyCount(); ++i)
		{
			PropertyItem* item = propertyItems()[i];

			if (item->arrayIndex() != -1)
			{
				(*item->arrayCallback())( item->arrayIndex() );
			}
		}
	}
	virtual void deleteSelf()
	{
		BW_GUARD;

		for (PropertyItems::iterator it = propertyItems_.begin();
			it != propertyItems_.end();
			it++)
		{
			bw_safe_delete(*it);
		}
		propertyItems_.clear();

		GeneralProperty::View::deleteSelf();
	}

	virtual void expel();

	virtual bool isEditorView() const { return true; }

	virtual void lastElected();

	virtual void select() {};

	virtual void onSelect() {};

	virtual bool isCommonView() { return false; }

	virtual PropertyManagerPtr getPropertyManager() const { return NULL; }
	virtual void setToDefault(){}
	virtual bool isDefault(){	return false;	}
	const BW::string name();
	size_t propertyCount() const { return propertyCount_; }
	void propertyCount( size_t val ) { propertyCount_ = val; }

	// Methods for speeding up sorting and the like.
	const BW::string & cachedName() const { return cachedName_; }
	void cacheName() { cachedName_ = name(); }
protected:

	size_t propertyCount_;
	PropertyItems propertyItems_;
	BW::vector< BW::string > itemGroups_;
	BasePropertyTable* propTable_;
	BW::string cachedName_;
};

/**
 * common view
 */

class CommonView : public BaseView
{
public:

	CommonView( BaseView* view );
	~CommonView();
	virtual void onChange( bool transient, bool addBarrier = true );
	virtual bool updateGUI();
	virtual void cloneValue( BaseView* another );

	virtual void addItem();
	virtual void delItems();
	virtual void delItem( int index );

	virtual void deleteSelf();

	virtual void expel();

	virtual void elect() {}

	virtual void select();

	virtual void onSelect();

	virtual bool isCommonView() { return true; }

	virtual PropertyManagerPtr getPropertyManager();
	virtual void setToDefault();
	virtual bool isDefault();

	void addView( BaseView* view );

	void updateDisplayItems();

	BW::vector< BaseView * > & views() { return views_; }

	bool isSameValue( const wchar_t * v1, const wchar_t * v2 ) const;
	void initGroups();

private:
	void onChangeInternal( bool skipFirst, bool transient );

	BW::vector< BaseView * > views_;
	bool firstChange_;

};


// text
class TextView : public BaseView
{
public:
	TextView( TextProperty & property ):
		property_( property )
	{
	}

	StringPropertyItem* item() { return (StringPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

        BW::wstring s( item()->get() );

		if (s.compare( oldValue_ ) != 0)
        {
            setCurrentValue( s, transient, addBarrier );
            oldValue_ = s;
        }
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		TextView* view = (TextView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );

		if (s.compare( oldValue_ ) != 0)
        {
            oldValue_ = s;
			item()->set(s);
			return true;
        }
		return false;
	}

	static TextProperty::View * create( TextProperty & property )
	{
		BW_GUARD;

		return new TextView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
    BW::wstring getCurrentValue()
    {
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );
        return s;
    }

    void setCurrentValue( BW::wstring s, bool transient, bool addBarrier )
    {
		BW_GUARD;

		BW::string narrowStr;
		bw_wtoutf8( s, narrowStr );

        property_.pySet( PyString_FromString( narrowStr.c_str() ), transient, addBarrier );
    }


	TextProperty & property_;
	BW::wstring oldValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			TextProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};



// static text
class StaticTextView : public BaseView
{
public:
	StaticTextView( StaticTextProperty & property )
		: property_( property )
	{
	}

	StringPropertyItem* item() { return (StringPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

        BW::wstring s( item()->get() );

		if (s.compare( oldValue_ ) != 0)
        {
            oldValue_ = s;
        }
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );

		if (s.compare( oldValue_ ) != 0)
        {
            oldValue_ = s;
			item()->set(s);
			return true;
        }
		return false;
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		StaticTextView* view = (StaticTextView*)another;
		item()->set( view->item()->get() );
	}

	static StaticTextProperty::View * create( StaticTextProperty & property )
	{
		BW_GUARD;

		return new StaticTextView( property );
	}

    void setCurrentValue(BW::wstring s)
    {
		// Nothing to do, we are read-only
    }

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
    BW::wstring getCurrentValue()
    {
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );
        return s;
    }

	StaticTextProperty & property_;
	BW::wstring oldValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			StaticTextProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};



// static text
class TextLabelView : public BaseView
{
public:
	TextLabelView( TextLabelProperty & property )
		: property_( property )
	{
	}

	LabelPropertyItem* item() { return (LabelPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
	}

	virtual bool updateGUI()
	{
		return false;
	}

	virtual void cloneValue( BaseView* another )
	{
	}

	static TextLabelProperty::View * create( TextLabelProperty & property )
	{
		BW_GUARD;

		return new TextLabelView( property );
	}

    void setCurrentValue(BW::string s)
    {
    }

	void * getUserObject()
	{
		BW_GUARD;

		return property_.getUserObject();
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	TextLabelProperty & property_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			TextLabelProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// id view
class IDView : public BaseView
{
public:
	IDView( IDProperty & property )
		: property_( property )
	{
	}

	IDPropertyItem* item() { return (IDPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

        BW::wstring s( item()->get() );

		if (s.compare( oldValue_ ) != 0)
        {
            setCurrentValue( s );
            oldValue_ = s;
        }
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		IDView* view = (IDView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );

		if (s.compare( oldValue_ ) != 0)
        {
            oldValue_ = s;
			item()->set(s);
			return true;
        }
		return false;
	}

	static IDProperty::View * create( IDProperty & property )
	{
		BW_GUARD;

		return new IDView( property );
	}

    void setCurrentValue(BW::wstring s)
    {
		BW_GUARD;

        property_.pySet( Py_BuildValue( "s", s.c_str() ) );
    }

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
    BW::wstring getCurrentValue()
    {
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );
        return s;
    }

	IDProperty & property_;
	BW::wstring oldValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			IDProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};



// group view
class GroupView : public BaseView
{
public:
	GroupView( GroupProperty & property )
		: property_( property )
	{
	}

	GroupPropertyItem* item() { return (GroupPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
	}

	virtual void cloneValue( BaseView* another )
	{
	}

	virtual bool updateGUI()
	{
		return false;
	}

	static GroupProperty::View * create( GroupProperty & property )
	{
		BW_GUARD;

		return new GroupView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	GroupProperty & property_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GroupProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};

// list of text items
class ListTextView : public BaseView
{
public:
	ListTextView( ListTextProperty & property )
		: property_( property )
	{
	}

	ComboPropertyItem* item() { return (ComboPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void expel()
	{
		BW_GUARD;

		item()->setChangeBuddy(NULL);
		BaseView::expel();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

        BW::wstring s( item()->get() );

		if (s.compare( oldValue_ ) != 0)
        {
            setCurrentValue( s, transient, addBarrier );
            oldValue_ = s;
        }
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		ListTextView* view = (ListTextView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );

		if (s.compare( oldValue_ ) != 0)
        {
            oldValue_ = s;
			item()->set(s);
			return true;
        }
		return false;
	}

	static ListTextProperty::View * create( ListTextProperty & property )
	{
		BW_GUARD;

		return new ListTextView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
    BW::wstring getCurrentValue()
    {
		BW_GUARD;

        BW::wstring s;
		bw_utf8tow( property_.pString()->get(), s );
        return s;
    }

    void setCurrentValue( BW::wstring s, bool transient, bool addBarrier )
    {
		BW_GUARD;

		BW::string narrowStr;
		bw_wtoutf8( s, narrowStr );

		property_.pySet( PyString_FromString( narrowStr.c_str() ), transient, addBarrier );
    }


	ListTextProperty & property_;
	BW::wstring oldValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			ListTextProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// list of text items associated to an int field
class ChoiceView : public BaseView
{
public:
	ChoiceView( ChoiceProperty & property )
		: property_( property )
	{
	}

	ComboPropertyItem* item() { return (ComboPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		BW::wstring s( item()->get() );
		int v = choices_.find( s )->second;

		if (v != oldValue_)
		{
			property_.proxySet( v, false, addBarrier );
			oldValue_ = v;
		}
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		ChoiceView* view = (ChoiceView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		const int newValue = property_.proxyGet();

		if (newValue != oldValue_)
		{
			oldValue_ = newValue;

			// set the combobox based on string as the int values may be disparate
			BW::wstring strValue;
			for ( WStringHashMap<int>::iterator it = choices_.begin();
				it != choices_.end();
				it++ )
			{
				if (it->second == newValue)
				{
					strValue = it->first;
					break;
				}
			}

			item()->set(strValue);
			return true;
		}
		return false;
	}

	static ChoiceProperty::View * create( ChoiceProperty & property )
	{
		BW_GUARD;

		return new ChoiceView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	ChoiceProperty & property_;
	int oldValue_;
	WStringHashMap<int> choices_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			ChoiceProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};



// bool
class GenBoolView : public BaseView
{
public:
	GenBoolView( GenBoolProperty & property ) : property_( property )
	{
	}

	BoolPropertyItem* item() { return (BoolPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		bool newValue = item()->get();
		property_.pBool()->set( newValue, false, addBarrier );
		oldValue_ = newValue;
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		GenBoolView* view = (GenBoolView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		const bool newValue = property_.pBool()->get();

		if (newValue != oldValue_)
		{
			oldValue_ = newValue;
			item()->set(newValue);
			return true;
		}
		return false;
	}

	static GenFloatProperty::View * create( GenBoolProperty & property )
	{
		BW_GUARD;

		return new GenBoolView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	GenBoolProperty & property_;
	bool oldValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenBoolProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// float
class GenFloatView : public BaseView
{
public:
	GenFloatView( GenFloatProperty & property )
		: property_( property ),
		transient_( true ),
		addBarrier_( true )
	{
	}

	FloatPropertyItem* item() { return (FloatPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void setToDefault()
	{
		BW_GUARD;

		property_.pFloat()->setToDefault();
	}

	virtual bool isDefault()
	{
		BW_GUARD;

		return property_.pFloat()->isDefault();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		newValue_ = item()->get();
		transient_ = transient;
		addBarrier_ = addBarrier;
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		GenFloatView* view = (GenFloatView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		float v = property_.pFloat()->get();

		bool changed = false;

		if (v != oldValue_)
		{
			newValue_ = v;
			oldValue_ = v;
			item()->set( v );
			changed = true;
		}

		if ((newValue_ != oldValue_) || (!transient_))
		{
			if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
			{
				if (!transient_)
				{
					property_.pFloat()->set( lastValue_, true);
					lastValue_ = newValue_;
				}
				property_.pFloat()->set( newValue_, transient_, addBarrier_ );
				oldValue_ = newValue_;
				lastTimeStamp_ = timestamp();
				transient_ = true;
				changed = true;
			}
		}
		return changed;
	}

	static GenFloatProperty::View * create( GenFloatProperty & property )
	{
		BW_GUARD;

		return new GenFloatView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

protected:

private:
	GenFloatProperty & property_;
	float oldValue_;
	float newValue_;
	float lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;


	class Enroller {
		Enroller() {
			BW_GUARD;

			GenFloatProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// int
class GenIntView : public BaseView
{
public:
	GenIntView( GenIntProperty & property )
		: property_( property ),
		transient_( true ),
		addBarrier_( true )
	{
	}

	IntPropertyItem* item() { return (IntPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		newValue_ = item()->get();
		transient_ = transient;
		addBarrier_ = addBarrier;
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		GenIntView* view = (GenIntView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		int v = property_.pInt()->get();
		bool changed = false;

		if (v != oldValue_)
		{
			newValue_ = v;
			oldValue_ = v;
			item()->set( v );
			changed = true;
		}

		if ((newValue_ != oldValue_) || (!transient_))
		{
			if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
			{
				if (!transient_)
				{
					property_.pInt()->set( lastValue_, true);
					lastValue_ = newValue_;
				}
				property_.pInt()->set( newValue_, transient_, addBarrier_ );
				oldValue_ = newValue_;
				lastTimeStamp_ = timestamp();
				transient_ = true;
				changed = true;
			}
		}
		return changed;
	}

	static GenIntProperty::View * create( GenIntProperty & property )
	{
		BW_GUARD;

		return new GenIntView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

protected:

private:
	GenIntProperty & property_;
	int newValue_;
	int oldValue_;
	int lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenIntProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// int
class GenUIntView : public BaseView
{
public:
	GenUIntView( GenUIntProperty & property )
		: property_( property ),
		transient_( true ),
		addBarrier_( true )
	{
	}

	UIntPropertyItem* item() { return (UIntPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		newValue_ = item()->get();
		transient_ = transient;
		addBarrier_ = addBarrier;
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		GenUIntView* view = (GenUIntView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		int v = property_.pUInt()->get();
		bool changed = false;

		if (v != oldValue_)
		{
			newValue_ = v;
			oldValue_ = v;
			item()->set( v );
			changed = true;
		}

		if ((newValue_ != oldValue_) || (!transient_))
		{
			if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
			{
				if (!transient_)
				{
					property_.pUInt()->set( lastValue_, true);
					lastValue_ = newValue_;
				}
				property_.pUInt()->set( newValue_, transient_, addBarrier_ );
				oldValue_ = newValue_;
				lastTimeStamp_ = timestamp();
				transient_ = true;
				changed = true;
			}
		}
		return changed;
	}

	static GenUIntProperty::View * create( GenUIntProperty & property )
	{
		BW_GUARD;

		return new GenUIntView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

protected:

private:
	GenUIntProperty & property_;
	uint32 newValue_;
	uint32 oldValue_;
	uint32 lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenUIntProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// vector3 position
class GenPositionView : public BaseView
{
public:
	GenPositionView( GenPositionProperty & property )
		: property_( property )
	{
	}

	FloatPropertyItem* itemX() { return (FloatPropertyItem*)&*(propertyItems_[0]); }
	FloatPropertyItem* itemY() { return (FloatPropertyItem*)&*(propertyItems_[1]); }
	FloatPropertyItem* itemZ() { return (FloatPropertyItem*)&*(propertyItems_[2]); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		changedValue_ = oldValue_;
		Vector3 newValue;
		newValue.x = itemX()->get();
		newValue.y = itemY()->get();
		newValue.z = itemZ()->get();

		Matrix matrix, ctxInv;
		property_.pMatrix()->recordState();
		property_.pMatrix()->getMatrix( matrix, false );
		property_.pMatrix()->getMatrixContextInverse( ctxInv );
		matrix.translation( ctxInv.applyPoint( newValue ) );

		if (!property_.pMatrix()->setMatrix( matrix ))
		{
			oldValue_ = newValue;
			updateGUI();
		}

		if (!property_.pMatrix()->commitState( false, addBarrier ))
		{
			oldValue_ = newValue;
			updateGUI();
		}
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		GenPositionView* view = (GenPositionView*)another;
		if (!almostEqual( view->itemX()->get(), view->changedValue_.x ))
		{
			itemX()->set( view->itemX()->get() );
		}
		if (!almostEqual( view->itemY()->get(), view->changedValue_.y ))
		{
			itemY()->set( view->itemY()->get() );
		}
		if (!almostEqual( view->itemZ()->get(), view->changedValue_.z ))
		{
			itemZ()->set( view->itemZ()->get() );
		}
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		Matrix matrix;
		property_.pMatrix()->getMatrix( matrix );
		const Vector3 & newValue = matrix.applyToOrigin();

		if (newValue != oldValue_)
		{
			changedValue_ = oldValue_;
			oldValue_ = newValue;

			itemX()->set(newValue.x);
			itemY()->set(newValue.y);
			itemZ()->set(newValue.z);
		}
		return false;
	}

	static GenPositionProperty::View * create( GenPositionProperty & property )
	{
		BW_GUARD;

		return new GenPositionView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	GenPositionProperty & property_;
	Vector3 oldValue_;
	Vector3 changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenPositionProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// rotation
class GenRotationView : public BaseView
{
public:
	GenRotationView( GenRotationProperty & property )
		: property_( property )
	{
	}

	FloatPropertyItem* itemYaw() { return (FloatPropertyItem*)&*(propertyItems_[0]); }
	FloatPropertyItem* itemPitch() { return (FloatPropertyItem*)&*(propertyItems_[1]); }
	FloatPropertyItem* itemRoll() { return (FloatPropertyItem*)&*(propertyItems_[2]); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		changedValue_ = oldValue_;
		Matrix prevMatrix;
		property_.pMatrix()->recordState();
		property_.pMatrix()->getMatrix( prevMatrix, false );

		Matrix newMatrix, temp;
		newMatrix.setScale(
			prevMatrix.applyToUnitAxisVector( X_AXIS ).length(),
			prevMatrix.applyToUnitAxisVector( Y_AXIS ).length(),
			prevMatrix.applyToUnitAxisVector( Z_AXIS ).length() );

		// If pitch = 90deg, we add a small epsilon to get meaningful yaw and
		// roll values.
		float xPitch = itemPitch()->get();
		if (almostEqual( fabs(xPitch), 90.f ))
		{
			// Add the epsilon in the same direction of the sign of the pitch.
			xPitch += xPitch >= 0.f ? -0.04f : 0.04f;
		}
		float yYaw = itemYaw()->get();
		float zRoll = itemRoll()->get();

		temp.setRotate(
			DEG_TO_RAD( yYaw ),
			DEG_TO_RAD( xPitch ),
			DEG_TO_RAD( zRoll ) );
		newMatrix.postMultiply( temp );

		temp.setTranslate( prevMatrix.applyToOrigin() );

		newMatrix.postMultiply( temp );

		if( !property_.pMatrix()->setMatrix( newMatrix ) )
		{
			oldValue_ = Vector3( itemPitch()->get(), itemYaw()->get(), itemRoll()->get() );
			updateGUI();
		}
		property_.pMatrix()->commitState( false, addBarrier );
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		GenRotationView* view = (GenRotationView*)another;
		if (!almostEqual( view->itemPitch()->get(), view->changedValue_.x ))
		{
			itemPitch()->set( view->itemPitch()->get() );
		}
		if (!almostEqual( view->itemYaw()->get(), view->changedValue_.y ))
		{
			itemYaw()->set( view->itemYaw()->get() );
		}
		if (!almostEqual( view->itemRoll()->get(), view->changedValue_.z ))
		{
			itemRoll()->set( view->itemRoll()->get() );
		}
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		const Vector3 newValue = rotation();

		if (newValue != oldValue_)
		{
			changedValue_ = oldValue_;
			oldValue_ = newValue;
			// We need to round to 1 decimal only. More than that makes it
			// difficult/confusing for the user to edit the angles.
			itemPitch()->set( roundTo( newValue.x, 10, 1 ) );
			itemYaw()->set( roundTo( newValue.y, 10, 1 ) );
			itemRoll()->set( roundTo( newValue.z, 10, 1 ) );
		}
		return false;
	}

	static GenRotationProperty::View * create( GenRotationProperty & property )
	{
		BW_GUARD;

		return new GenRotationView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:

	/**
	 *  This method rounds a float value to the nearest number that could be divided by 
	 *  a epsilon( e.g. 0.05 ) without remainder.
	 *  A few examples:
	 *	- roundTo( 13.9352, 100, 1 ) = 13.94 (it's rounded to 0.01)
	 *	- roundTo( 13.9352, 100, 3 ) = 13.95 (it's rounded to 0.03)
	 *
	 *	@param value	number to be rounded
	 *	@param base		a power of 10 to express how many digits (10 = 1, 100 = 2, etc)
	 *	@param multiple this param and base build the epsilon
	 *	@return			rounded number
	 */
	float roundTo( float value, int base, int multiple )
	{
		float temp = floorf( value * base + 0.5f );
		temp = floorf( temp / multiple + 0.5f );
		temp = floorf( temp * multiple );
		return temp / base;
	}


	/**
	 *  This method format the yaw, pitch and roll, if both yaw and roll 
	 *  are > 90deg, change them back to a value <= 90deg by adjusting 
	 *  pitch at same time
	 */
	Vector3 formatRotation( float xPitch, float yYaw, float zRoll ) const
	{
		BW_GUARD;

		if (almostEqual( xPitch, -180.f, 0.02f ))
		{
			xPitch = -xPitch;
		}

		if (almostEqual( yYaw, -180.f, 0.02f ))
		{
			yYaw = -yYaw;
		}

		if (almostEqual( zRoll, -180.f, 0.02f ))
		{
			zRoll = -zRoll;
		}

		if ((yYaw < -90.f || yYaw > 90.f) && (zRoll < -90.f || zRoll > 90.f))
		{
			float adj = (almostEqual( xPitch, 0.f ) || xPitch > 0.f) ? 180.f : -180.f;
			xPitch = adj - xPitch;
			yYaw = fmodf(yYaw - adj, 360.f);
			
			if (yYaw < -180.f)
			{
				yYaw += 360.f;
			}
			else if (yYaw > 180.f)
			{
				yYaw -= 360.f;
			}

			zRoll = fmodf(zRoll - adj, 360.f);

			if (zRoll < -180.f)
			{
				zRoll += 360.f;
			}
			else if (zRoll > 180.f)
			{
				zRoll -= 360.f;
			}

		}

		return Vector3( xPitch, yYaw, zRoll );

	}

	Vector3 rotation() const
	{
		BW_GUARD;

		Matrix matrix;
		property_.pMatrix()->getMatrix( matrix );

		return formatRotation(
			RAD_TO_DEG( matrix.pitch() ),
			RAD_TO_DEG( matrix.yaw() ) ,
			RAD_TO_DEG( matrix.roll() ) );
	}

	GenRotationProperty & property_;
	Vector3 oldValue_;
	Vector3 changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenRotationProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// scale
class GenScaleView : public BaseView
{
public:
	GenScaleView( GenScaleProperty & property )
		: property_( property )
	{
	}

	FloatPropertyItem* itemX() { return (FloatPropertyItem*)&*(propertyItems_[0]); }
	FloatPropertyItem* itemY() { return (FloatPropertyItem*)&*(propertyItems_[1]); }
	FloatPropertyItem* itemZ() { return (FloatPropertyItem*)&*(propertyItems_[2]); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		changedValue_ = oldValue_;
		Vector3 newValue;
		newValue.x = itemX()->get();
		newValue.y = itemY()->get();
		newValue.z = itemZ()->get();

		if (oldValue_.x != 0.f &&
			oldValue_.y != 0.f &&
			oldValue_.z != 0.f &&
			newValue.x != 0.f &&
			newValue.y != 0.f &&
			newValue.z != 0.f)
		{
			Matrix matrix;
			property_.pMatrix()->recordState();
			property_.pMatrix()->getMatrix( matrix, false );
			Matrix scaleMatrix;
			scaleMatrix.setScale(
				newValue.x/oldValue_.x,
				newValue.y/oldValue_.y,
				newValue.z/oldValue_.z );
			matrix.preMultiply( scaleMatrix );

			if( !property_.pMatrix()->setMatrix( matrix ) )
			{
				oldValue_ = newValue;
				updateGUI();
			}
			property_.pMatrix()->commitState( false, addBarrier );
		}
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		GenScaleView* view = (GenScaleView*)another;
		if (!almostEqual( view->itemX()->get(), view->changedValue_.x ))
		{
			itemX()->set( view->itemX()->get() );
		}
		if (!almostEqual( view->itemY()->get(), view->changedValue_.y ))
		{
			itemY()->set( view->itemY()->get() );
		}
		if (!almostEqual( view->itemZ()->get(), view->changedValue_.z ))
		{
			itemZ()->set( view->itemZ()->get() );
		}
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		const Vector3 newValue = this->scale();

		if (newValue != oldValue_)
		{
			changedValue_ = oldValue_;
			oldValue_ = newValue;

			itemX()->set(newValue.x);
			itemY()->set(newValue.y);
			itemZ()->set(newValue.z);
		}
		return false;
	}

	static GenScaleProperty::View * create( GenScaleProperty & property )
	{
		BW_GUARD;

		return new GenScaleView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	Vector3 scale() const
	{
		BW_GUARD;

		Matrix matrix;
		property_.pMatrix()->getMatrix( matrix );
		return Vector3(
			matrix.applyToUnitAxisVector( X_AXIS ).length(),
			matrix.applyToUnitAxisVector( Y_AXIS ).length(),
			matrix.applyToUnitAxisVector( Z_AXIS ).length() );
	}

	GenScaleProperty & property_;
	Vector3 oldValue_;
	Vector3 changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenScaleProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// colour
class ColourView : public BaseView
{
public:
	ColourView( ColourProperty & property );

	~ColourView();

	ColourPropertyItem* item() { return (ColourPropertyItem*)&*(propertyItems_[0]); }

	IntPropertyItem* item( int index ) { return (IntPropertyItem*)&*(propertyItems_[index+1]); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true );

	virtual void cloneValue( BaseView* another );

	virtual bool updateGUI();

	static ColourProperty::View * create( ColourProperty & property )
	{
		BW_GUARD;

		return new ColourView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	Vector4 toVector( Moo::Colour c )
	{
		return Vector4(c.r, c.g, c.b, c.a);
	}

	Moo::Colour toColour( Vector4& v )
	{
		Moo::Colour c;
		c.r = v.x;
		c.g = v.y;
		c.b = v.z;
		c.a = v.w;
		return c;
	}

	bool fromString( const char* s, Moo::Colour& c )
	{
		BW_GUARD;

		if (*s == '#')
			s++;

		if (strlen(s) != 8)
			return false;

		char* endptr;

		char buf[3];
		buf[2] = '\0';

		buf[0] = s[0];
		buf[1] = s[1];
		c.r = (float)strtol(buf, &endptr, 16);

		buf[0] = s[2];
		buf[1] = s[3];
		c.g = (float)strtol(buf, &endptr, 16);

		buf[0] = s[4];
		buf[1] = s[5];
		c.b = (float)strtol(buf, &endptr, 16);

		buf[0] = s[6];
		buf[1] = s[7];
		c.a = (float)strtol(buf, &endptr, 16);

		return true;
	}

	void setCurrentValue( Moo::Colour& c, bool transient )
	{
		BW_GUARD;

		property_.pySet( Py_BuildValue( "ffff", c.r, c.g, c.b, c.a ),
													transient, addBarrier_ );
	}

	bool getCurrentValue(Moo::Colour& c)
	{
		BW_GUARD;

		PyObject* pValue = property_.pyGet();
		if (!pValue) {
			PyErr_Clear();
			return false;
		}

		// Using !PyVector<Vector4>::Check( pValue ) doesn't work so we do it
		// this dodgy way
		if (BW::string(pValue->ob_type->tp_name) != "Math.Vector4")
		{
			PyErr_SetString( PyExc_TypeError, "ColourView::getCurrentValue() "
				"expects a PyVector<Vector4>" );

    		Py_DECREF( pValue );
			return false;
		}

		PyVector<Vector4>* pv = static_cast<PyVector<Vector4>*>( pValue );
		Vector4 v = pv->getVector();

		c.r = v.x;
		c.g = v.y;
		c.b = v.z;
		c.a = v.w;

		Py_DECREF( pValue );

		return true;
	}

	bool equal( Moo::Colour c1, Moo::Colour c2 )
	{
		return ((int) c1.r == (int) c2.r &&
			(int) c1.g == (int) c2.g &&
			(int) c1.b == (int) c2.b &&
			(int) c1.a == (int) c2.a);
	}

	ColourProperty & property_;
	Moo::Colour oldValue_;
	Moo::Colour newValue_;
	Moo::Colour lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;
	Moo::Colour changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			ColourProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


class MultiplierFloatView : public BaseView
{
public:
	MultiplierFloatView( GenFloatProperty & property );

	~MultiplierFloatView();

	//FloatPropertyItem* item() { return (FloatPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true );

	virtual void cloneValue( BaseView* another ) {}

	virtual bool updateGUI();

	static GenFloatProperty::View * create( GenFloatProperty & property )
	{
		BW_GUARD;

		GenFloatProperty::View* view = new MultiplierFloatView( property );
		return view;
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

protected:

private:
	bool isMultiplier_;
	GenFloatProperty & property_;
	float oldValue_;

	float lastSeenSliderValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenFloatProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// Python
class PythonView : public BaseView
{
public:
	PythonView( PythonProperty & property )
		: property_( property )
	{
	}

	StringPropertyItem* item() { return (StringPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

        BW::wstring s( item()->get() );

        if (s != oldValue_)
        {
            if (this->setCurrentValue( s, addBarrier ))
			{
				oldValue_ = s;
			}
        }
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		PythonView* view = (PythonView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

        BW::wstring s( this->getCurrentValue() );

        if (s != oldValue_)
        {
			if (this->setCurrentValue( s ))
			{
				oldValue_ = s;
				return true;
			}
        }
		return false;
	}

	static PythonProperty::View * create( PythonProperty & property )
	{
		BW_GUARD;

		return new PythonView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
    BW::wstring getCurrentValue()
    {
		BW_GUARD;

		PyObject* pValue = property_.pyGet();
        if (!pValue)
		{
			PyErr_Clear();
            return L"";
		}

        BW::wstring s;

		extractUnicode( pValue, s );

		Py_DECREF( pValue );

        return s;
    }

    bool setCurrentValue( BW::wstring s, bool addBarrier = true )
    {
		BW_GUARD;

		BW::string ns;
		bw_wtoutf8( s, ns );
		PyObject * pNew = Script::runString( ns.c_str(), false );

		if (pNew)
		{
			property_.pySet( pNew, false, addBarrier );
			// This may be slightly differnt to s
			BW::wstring newStr =  this->getCurrentValue();
			this->item()->set( newStr );
			oldValue_ = newStr;
			Py_DECREF( pNew );
		}
		else
		{
			PyErr_Clear();
			return false;
		}

		return true;
    }


	PythonProperty & property_;
	BW::wstring oldValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			PythonProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};

// help method for vector4 and matrix
#include <sstream>

template<typename EntryType>
static BW::wstring NumVecToStr( const EntryType* vec, int size )
{
	BW_GUARD;

	BW::wostringstream oss;
	for( int i = 0; i < size; ++i )
	{
		oss << vec[ i ];
		if( i != size - 1 )
			oss << L',';
	}
	return oss.str();
}

template<typename EntryType>
static bool StrToNumVec( const BW::wstring& str, EntryType* vec, int size )
{
	BW_GUARD;

	bool result = false;
	BW::wistringstream iss( str );

	for( int i = 0; i < size; ++i )
	{
		wchar_t ch;
		iss >> vec[ i ];
		if( i != size - 1 )
		{
			iss >> ch;
			if( ch != L',' )
				break;
		}
		else
			result = iss.rdbuf() != NULL;
	}
	return result;
}

// vector2
class Vector2View : public BaseView
{
public:
	Vector2View( Vector2Property & property )
		: property_( property ),
		transient_( true ),
		addBarrier_( true )
	{}

	StringPropertyItem* item() { return (StringPropertyItem*)&*(propertyItems_.front()); }

	FloatPropertyItem* item( int index ) { return (FloatPropertyItem*)&*(propertyItems_[index+1]); }

	virtual void elect();

	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true );

	virtual void cloneValue( BaseView* another );

	virtual bool updateGUI();

	static Vector2Property::View * create( Vector2Property & property )
	{
		BW_GUARD;

		return new Vector2View( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

protected:

private:
	void setCurrentValue( Vector2& v, bool transient )
	{
		BW_GUARD;

		property_.pySet( Py_BuildValue( "ff", v.x, v.y ), transient, addBarrier_ );
	}

	bool getCurrentValue( Vector2& v )
	{
		BW_GUARD;

		PyObject* pValue = property_.pyGet();
		if (!pValue) {
			PyErr_Clear();
			return false;
		}

		// Using !PyVector<Vector2>::Check( pValue ) doesn't work so we do it
		// this dodgy way
		if (BW::string(pValue->ob_type->tp_name) != "Math.Vector2")
		{
			PyErr_SetString( PyExc_TypeError, "Vector4View::getCurrentValue() "
				"expects a PyVector<Vector2>" );

    		Py_DECREF( pValue );
			return false;
		}

		PyVector<Vector2>* pv = static_cast<PyVector<Vector2>*>( pValue );
		Vector2 rv = pv->getVector();
		v = rv;

		Py_DECREF( pValue );

		return true;
	}

	Vector2Property & property_;

	Vector2 newValue_;
	Vector2 oldValue_;
	Vector2 lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;
	Vector2 changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			Vector2Property_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};

// vector3
class Vector3View : public BaseView
{
public:
	Vector3View( Vector3Property & property )
		: property_( property ),
		transient_( true ),
		addBarrier_( true )
	{}

	StringPropertyItem* item() { return (StringPropertyItem*)&*(propertyItems_.front()); }

	FloatPropertyItem* item( int index ) { return (FloatPropertyItem*)&*(propertyItems_[index+1]); }

	virtual void elect();

	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true );

	virtual void cloneValue( BaseView* another );

	virtual bool updateGUI();

	static Vector3Property::View * create( Vector3Property & property )
	{
		BW_GUARD;

		return new Vector3View( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	void setCurrentValue( Vector3& v, bool transient )
	{
		BW_GUARD;

		property_.pySet( Py_BuildValue( "fff", v.x, v.y, v.z ),
													transient, addBarrier_ );
	}

	bool getCurrentValue( Vector3& v )
	{
		BW_GUARD;

		PyObject* pValue = property_.pyGet();
		if (!pValue) {
			PyErr_Clear();
			return false;
		}

		// Using !PyVector<Vector3>::Check( pValue ) doesn't work so we do it
		// this dodgy way
		if (BW::string(pValue->ob_type->tp_name) != "Math.Vector3")
		{
			PyErr_SetString( PyExc_TypeError, "Vector3View::getCurrentValue() "
				"expects a PyVector<Vector3>" );

    		Py_DECREF( pValue );
			return false;
		}

		PyVector<Vector3>* pv = static_cast<PyVector<Vector3>*>( pValue );
		Vector3 tv = pv->getVector();

		v.x = tv[0];
		v.y = tv[1];
		v.z = tv[2];

		Py_DECREF( pValue );

		return true;
	}

	Vector3Property & property_;

	Vector3 newValue_;
	Vector3 oldValue_;
	Vector3 lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;
	Vector3 changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			Vector3Property_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


// vector4
class Vector4View : public BaseView
{
public:
	Vector4View( Vector4Property & property )
		: property_( property ),
		transient_( true ),
		addBarrier_( true )
	{}

	StringPropertyItem* item() { return (StringPropertyItem*)&*(propertyItems_.front()); }

	FloatPropertyItem* item( int index ) { return (FloatPropertyItem*)&*(propertyItems_[index+1]); }

	virtual void elect();

	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true );

	virtual void cloneValue( BaseView* another );

	virtual bool updateGUI();

	static Vector4Property::View * create( Vector4Property & property )
	{
		BW_GUARD;

		return new Vector4View( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	void setCurrentValue( Vector4& v, bool transient )
	{
		BW_GUARD;

		property_.pySet( Py_BuildValue( "ffff", v.x, v.y, v.z, v.w ),
													transient, addBarrier_ );
	}

	bool getCurrentValue( Vector4& v )
	{
		BW_GUARD;

		PyObject* pValue = property_.pyGet();
		if (!pValue) {
			PyErr_Clear();
			return false;
		}

		// Using !PyVector<Vector4>::Check( pValue ) doesn't work so we do it
		// this dodgy way
		if (BW::string(pValue->ob_type->tp_name) != "Math.Vector4")
		{
			PyErr_SetString( PyExc_TypeError, "Vector4View::getCurrentValue() "
				"expects a PyVector<Vector4>" );

    		Py_DECREF( pValue );
			return false;
		}

		PyVector<Vector4>* pv = static_cast<PyVector<Vector4>*>( pValue );
		Vector4 tv = pv->getVector();

		v.x = tv[0];
		v.y = tv[1];
		v.z = tv[2];
		v.w = tv[3];

		Py_DECREF( pValue );

		return true;
	}

	Vector4Property & property_;

	Vector4 newValue_;
	Vector4 oldValue_;
	Vector4 lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;
	Vector4 changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			Vector4Property_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};

// matrix
class MatrixView : public BaseView
{
public:
	MatrixView( GenMatrixProperty & property )
		: property_( property )
	{
	}

	StringPropertyItem* item() { return (StringPropertyItem*)&*(propertyItems_.front()); }

	virtual void elect();

	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		PropTableSetter pts(propTable_);

		BW::wstring newValue( item()->get() );
		Matrix v;
		if( StrToNumVec( newValue, (float*)v, 16 ) )
		{
			property_.pMatrix()->setMatrix( v );
			oldValue_ = v;
		}
	}

	virtual void cloneValue( BaseView* another )
	{
		BW_GUARD;

		MatrixView* view = (MatrixView*)another;
		item()->set( view->item()->get() );
	}

	virtual bool updateGUI()
	{
		BW_GUARD;

		Matrix newValue;
		property_.pMatrix()->getMatrix( newValue, true );

		if (newValue != oldValue_)
		{
			oldValue_ = newValue;
			item()->set( NumVecToStr( (float*)&newValue, 16 ) );
			return true;
		}
		return false;
	}

	static GenMatrixProperty::View * create( GenMatrixProperty & property )
	{
		BW_GUARD;

		return new MatrixView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

protected:

private:
	GenMatrixProperty & property_;
	Matrix oldValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			GenMatrixProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};




// 3-component colour and a float scalar
class ColourScalarView : public BaseView
{
public:
	ColourScalarView( ColourScalarProperty & property );

	~ColourScalarView();

	//indexing note - 5 property items.  ColourScalar, Int, Int, Int, Float
	ColourScalarPropertyItem* item() { return (ColourScalarPropertyItem*)&*(propertyItems_[0]); }
	IntPropertyItem* item( int index ) { return (IntPropertyItem*)&*(propertyItems_[index+1]); }
	FloatPropertyItem* floatItem() { return (FloatPropertyItem*)&*(propertyItems_[4]); }

	virtual void elect();
	
	virtual void onSelect()
	{
		BW_GUARD;

		property_.select();
	}

	virtual void onChange( bool transient, bool addBarrier = true );

	virtual void cloneValue( BaseView* another );

	virtual bool updateGUI();

	static ColourScalarProperty::View * create( ColourScalarProperty & property )
	{
		BW_GUARD;

		return new ColourScalarView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:
	Vector4 toVector( Moo::Colour c )
	{
		return Vector4(c.r, c.g, c.b, c.a);
	}

	Moo::Colour toColour( Vector4& v )
	{
		Moo::Colour c;
		c.r = v.x;
		c.g = v.y;
		c.b = v.z;
		c.a = v.w;
		return c;
	}

	bool fromString( const char* s, Moo::Colour& c )
	{
		BW_GUARD;

		if (*s == '#')
			s++;

		if (strlen(s) != 8)
			return false;

		char* endptr;

		char buf[3];
		buf[2] = '\0';

		buf[0] = s[0];
		buf[1] = s[1];
		c.r = (float)strtol(buf, &endptr, 16);

		buf[0] = s[2];
		buf[1] = s[3];
		c.g = (float)strtol(buf, &endptr, 16);

		buf[0] = s[4];
		buf[1] = s[5];
		c.b = (float)strtol(buf, &endptr, 16);

		buf[0] = s[6];
		buf[1] = s[7];
		c.a = (float)strtol(buf, &endptr, 16);

		return true;
	}

	void setCurrentValue( Moo::Colour& c, bool transient )
	{
		BW_GUARD;

		property_.pySet( Py_BuildValue( "ffff", c.r, c.g, c.b, c.a ),
													transient, addBarrier_ );
	}

	bool getCurrentValue(Moo::Colour& c)
	{
		BW_GUARD;

		PyObject* pValue = property_.pyGet();
		if (!pValue) {
			PyErr_Clear();
			return false;
		}

		// Using !PyVector<Vector4>::Check( pValue ) doesn't work so we do it
		// this dodgy way
		if (BW::string(pValue->ob_type->tp_name) != "Math.Vector4")
		{
			PyErr_SetString( PyExc_TypeError, "ColourScalarView::getCurrentValue() "
				"expects a PyVector<Vector4>" );

    		Py_DECREF( pValue );
			return false;
		}

		PyVector<Vector4>* pv = static_cast<PyVector<Vector4>*>( pValue );
		Vector4 v = pv->getVector();

		c.r = v[0];
		c.g = v[1];
		c.b = v[2];
		c.a = v[3];

		Py_DECREF( pValue );

		return true;
	}

	bool equal( Moo::Colour c1, Moo::Colour c2 )
	{		
		return	(int) c1.r == (int) c2.r &&
				(int) c1.g == (int) c2.g &&
				(int) c1.b == (int) c2.b &&
				almostEqual( c1.a, c2.a, 0.01f );		// comparing floats
	}

	ColourScalarProperty & property_;
	Moo::Colour oldValue_;
	Moo::Colour newValue_;
	Moo::Colour lastValue_;
	uint64 lastTimeStamp_;
	bool transient_;
	bool addBarrier_;
	Moo::Colour changedValue_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			ColourScalarProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};


/**
 *	This class implements a PropertyList view of the array.
 */
class ArrayView : public BaseView
{
public:
	ArrayView( ArrayProperty & property );

	ArrayPropertyItem* item() { return (ArrayPropertyItem*)&*(propertyItems_[0]); }

	virtual void elect();
	virtual void elected();
	
	virtual void onSelect() { property_.select(); }
	virtual void onChange( bool transient, bool addBarrier = true );

	virtual void cloneValue( BaseView * another);

	virtual bool updateGUI();

	virtual void addItem()
	{
		BW_GUARD;

		PropertyModifyGuard guard; property_.proxy()->addItem();
	}
	virtual void delItems()
	{
		BW_GUARD;

		PropertyModifyGuard guard; property_.proxy()->delItems();
	}
	virtual void delItem( int index )
	{
		BW_GUARD;

		PropertyModifyGuard guard; property_.delItem( index );
	}

	static ArrayProperty::View * create( ArrayProperty & property )
	{
		BW_GUARD;

		return new ArrayView( property );
	}

	virtual PropertyManagerPtr getPropertyManager() const { return property_.getPropertyManager(); }

private:

	ArrayProperty&	property_;

	class Enroller {
		Enroller() {
			BW_GUARD;

			ArrayProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};
BW_END_NAMESPACE

