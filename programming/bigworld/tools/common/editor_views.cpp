#include "pch.hpp"
#include "editor_views.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

BasePropertyTable* PropTable::s_propTable_ = NULL;	

void extractUnicode( PyObject* pValue, BW::wstring &s )
{
	BW_GUARD;

	// TODO:UNICODE: This might benifit from better error checking.
	if ( PyUnicode_Check( pValue ) )
	{
		PyObject * pUO = PyObject_Unicode( pValue );

		if (pUO)
		{
			// TODO:UNICODE: make sure the tools can do this (unicode strings )
			// without issues
			PyUnicodeObject* unicodeObj = reinterpret_cast<PyUnicodeObject*>(pUO);

			Py_ssize_t ulen = PyUnicode_GET_DATA_SIZE( unicodeObj ) / sizeof(Py_UNICODE);
			if (ulen >= 0)
			{
				// In theory this is bad, because we're assuming that 
				// sizeof(Py_UNICODE) == sizeof(wchar_t), and that ulen maps to
				// of characters that PyUnicode_AsWideChar will write into the 
				// destination buffer. In practice this is true, but for good measure
				// I'm going to stick in a compile-time assert.
				BW_STATIC_ASSERT( sizeof(Py_UNICODE) == sizeof(wchar_t), SizeOfPyUnicodeIsNotSizeOfWchar_t );
				s.resize(ulen);

				PyUnicode_AsWideChar( unicodeObj, &s[0], ulen );
			}

			Py_DECREF( pUO );
			return;
		}
		else
		{
			PyErr_Clear();
		}
	}
	else if ( PyString_Check( pValue ) )
	{
		PyObject * pSO = PyObject_Str( pValue );
		if ( pSO )
		{
			bw_utf8tow( PyString_AsString( pSO ), s );

			Py_DECREF( pSO );
			return;
		}
		else
		{
			PyErr_Clear();
		}
	}
}


BaseView::BaseView( )
{
}
	
void BaseView::expel()
{
	BW_GUARD;

	if (!propTable_)
	{
		return;
	}

	propTable_->clear();

	for (size_t i = 0; i < propertyItems_.size(); ++i)
		bw_safe_delete(propertyItems_[i]);

	propertyItems_.clear();
}

void BaseView::lastElected()
{
	BW_GUARD;

	if (PropTable::table())
	{
		PropTable::table()->addView( NULL );
		GeneralProperty::View::pLastElected( NULL );
	}
}

const BW::string BaseView::name()
{
	BW_GUARD;

	BW::string s;
	BW::string subGroup;

	if (propertyCount_ > 0 && propertyItems_[0]->getGroupDepth() > 1)
	{
		subGroup = propertyItems_[0]->getGroup().str();
		BW::string::size_type subGroupIdx = subGroup.find_first_of( '/' );

		if (subGroupIdx != BW::string::npos)
		{
			subGroup = subGroup.substr( subGroupIdx + 1 );
			s = subGroup + "::";
		}
	}

	if (propertyCount_ > 0)
	{
		if (!propertyItems_[0]->isGroupType() ||
			propertyItems_[0]->name() != subGroup.c_str())
		{
			for (size_t i = 0; i < propertyCount_; ++i)
			{
				s += propertyItems_[i]->name().c_str();
				s += "::";
			}
		}
	}
	return s;
}

//---------------------------------

CommonView::CommonView( BaseView* view )
{
	BW_GUARD;

	firstChange_ = true;
	views_.clear();
	views_.push_back( view );
	propertyItems_.assign( view->propertyItems().begin(), view->propertyItems().end() );
	for (PropertyItems::iterator i = propertyItems_.begin(); i != propertyItems_.end(); ++i)
	{
		(*i)->setChangeBuddy( this );
	}
}

CommonView::~CommonView()
{
	BW_GUARD;

	if (views_.size())
	{
		for (size_t i = 0; i < propertyItems_.size(); ++i)
		{
			propertyItems_[i]->setChangeBuddy( views_.front() );
			propertyItems_[i]->setGroup( Name( itemGroups_[i] ) );
		}
		views_.clear();
	}
	propertyItems_.clear();
	itemGroups_.clear();
}


void CommonView::onChangeInternal( bool skipFirst, bool transient )
{
	BW_GUARD;

	BW::vector< BaseView* >::iterator it = views_.begin();
	BaseView * pLastView = views_.back();
	if (!skipFirst)
	{
		(*it)->onChange( transient,
					(*it) == pLastView /*create undo barrier if it's the last*/ );
	}
	while (++it != views_.end())
	{
		(*it)->cloneValue( views_.front() );
		(*it)->onChange( transient,
				(*it) == pLastView /*create undo barrier if it's the last*/ );
	}
	updateDisplayItems();
}


void CommonView::onChange( bool transient, bool /*addBarrier*/ )
{
	BW_GUARD;

	PropTableSetter pts(propTable_);

	onChangeInternal( false, transient );
}

bool CommonView::updateGUI()
{
	BW_GUARD;

	if (views_[0]->updateGUI() && !firstChange_)
	{
		// If the first view changed, update the others skipping this view.
		onChangeInternal( true, false );
	}
	firstChange_ = false;
	for (size_t i = 1; i < views_.size(); ++i)
	{
		views_[i]->updateGUI();
	}
	updateDisplayItems();
	return false;
}

void CommonView::addItem()
{
	BW_GUARD;

	BW::vector< BaseView * > views = views_;
	PropertyModifyGuard guard;

	for (size_t i = 0; i < views.size(); ++i)
	{
		views[i]->addItem();
	}
}

void CommonView::delItems()
{
	BW_GUARD;

	BW::vector< BaseView * > views = views_;
	PropertyModifyGuard guard;

	for (size_t i = 0; i < views.size(); ++i)
	{
		views[i]->delItems();
	}
}

void CommonView::delItem( int index )
{
	BW_GUARD;

	BW::vector< BaseView * > views = views_;
	PropertyModifyGuard guard;

	for (size_t i = 0; i < views.size(); ++i)
	{
		for (size_t j = 0; j < views[i]->propertyCount(); ++j)
		{
			PropertyItem* item = views[i]->propertyItems()[j];

			if (item->arrayIndex() != -1)
			{
				(*item->arrayCallback())( item->arrayIndex() );
			}
		}
	}
}

void CommonView::cloneValue( BaseView* another )
{
	// do nothing
}

void CommonView::deleteSelf()
{
	BW_GUARD;

	if (views_.size())
	{
		for (size_t i = 0; i < propertyItems_.size(); ++i)
		{
			propertyItems_[i]->setChangeBuddy( views_[0] );
			propertyItems_[i]->setGroup( Name( itemGroups_[i] ) );
			
		}
		for (size_t i = 0; i < views_.size(); ++i)
		{
			views_[i]->deleteSelf();
		}
		propertyItems_.clear();
	}
	BaseView::deleteSelf();
}

void CommonView::expel()
{
	BW_GUARD;

	if (views_.size())
	{
		for (size_t i = 0; i < propertyItems_.size(); ++i)
		{
			propertyItems_[i]->setChangeBuddy( views_[0] );
			propertyItems_[i]->setGroup( Name( itemGroups_[i] ) );
		}
		for (size_t i = 0; i < views_.size(); ++i)
		{
			views_[i]->expel();
		}
		propertyItems_.clear();
	}
	BaseView::expel();
}

void CommonView::select()
{
	BW_GUARD;

	views_[0]->select();
}

void CommonView::onSelect()
{
	BW_GUARD;

	views_[0]->onSelect();
}

PropertyManagerPtr CommonView::getPropertyManager()
{
	BW_GUARD;

	return views_[0]->getPropertyManager();
}

void CommonView::setToDefault()
{
	BW_GUARD;

	for (size_t i = 0; i < views_.size(); ++i)
	{
		views_[i]->setToDefault();
	}
}

bool CommonView::isDefault()
{
	BW_GUARD;

	return views_[0]->isDefault();
}

void CommonView::addView( BaseView* view )
{
	BW_GUARD;

	views_.push_back( view );
}

void CommonView::updateDisplayItems()
{
	BW_GUARD;

	for (size_t i = 0; i < propertyItems_.size(); ++i)
	{
		const wchar_t * curPropertyStr = (LPCWSTR)propertyItems_[i]->value();

		propertyItems_[i]->multipleValue( false );
		for (size_t j = 1; j < views_.size(); ++j)
		{
			if (!isSameValue( curPropertyStr, views_[j]->propertyItems()[i]->value() ))
			{
				propertyItems_[i]->multipleValue( true );
				break;
			}
		}
		for (size_t j = 1; j < views_.size(); ++j)
		{
			views_[j]->propertyItems()[i]->multipleValue( propertyItems_[i]->multipleValue() );
		}
	}
}

bool CommonView::isSameValue( const wchar_t * v1, const wchar_t * v2 ) const
{
	BW_GUARD;

	bool sameStr = _wcsicmp( v1, v2 ) == 0;
	if (!sameStr)
	{
		float f1, f2;
		int n1 = swscanf_s(v1, L"%f", &f1);
		int n2 = swscanf_s(v2, L"%f", &f2);
		if (n1 == 1 && n2 == 1)
		{
			sameStr = almostEqual( f1, f2 );
		}
	}
	return sameStr;
}

void CommonView::initGroups()
{
	BW_GUARD;

	BW::string group = LocaliseUTF8( L"COMMON/EDITOR_VIEWS/OBJECT_SELECTED", views_.size() );
	for (size_t i = 0; i < propertyItems_.size(); ++i)
	{
		propertyItems_[i]->setChangeBuddy( this );
		BW::string thisGroup = propertyItems_[i]->getGroup().str();
		BW::string::size_type firstGroupEnd = thisGroup.find_first_of( '/' );
		if (firstGroupEnd != BW::string::npos)
		{
			thisGroup = group + thisGroup.substr( firstGroupEnd );
		}
		else
		{
			thisGroup = group;
		}
		itemGroups_.push_back( thisGroup );
		propertyItems_[i]->setGroup( Name( thisGroup ) );
	}
	updateDisplayItems();
}

//---------------------------------

void TextView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = getCurrentValue();

	StringPropertyItem* newItem = new StringPropertyItem(property_.name(), oldValue_.c_str());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->fileFilter( property_.fileFilter() );
	newItem->defaultDir( property_.defaultDir() );
	newItem->canTextureFeed( property_.canTextureFeed() );
	newItem->textureFeed( property_.textureFeed() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void StaticTextView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = getCurrentValue();

	PropertyItem* newItem = new StringPropertyItem(property_.name(), oldValue_.c_str(), true);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void TextLabelView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	PropertyItem* newItem = new LabelPropertyItem(property_.name(), property_.highlight());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void IDView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = getCurrentValue();

	PropertyItem* newItem = new IDPropertyItem(property_.name(), oldValue_.c_str());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void GroupView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	PropertyItem* newItem = new GroupPropertyItem(property_.name(), -1);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void ListTextView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = getCurrentValue();

	PropertyItem* newItem = new ComboPropertyItem(property_.name(),
												oldValue_.c_str(),
												property_.possibleValues());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void ChoiceView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = property_.proxyGet();
	
	BW::vector<BW::wstring> possibleValues;
	BW::map<int,BW::wstring> possibleValuesMap;

	BW::wstring oldStringValue;

	DataSectionPtr choices = property_.pChoices();
	for (DataSectionIterator iter = choices->begin();
		iter != choices->end();
		iter++)
	{
		DataSectionPtr pDS = *iter;
		BW::string name = property_.getName(pDS->sectionName(), pDS);
		BW::wstring wname; bw_utf8tow( name, wname );
		choices_[ wname ] = pDS->asInt( 0 );
		possibleValuesMap[ pDS->asInt( 0 ) ] = wname;
		if (pDS->asInt( 0 ) == oldValue_)
		{
			oldStringValue = wname;
		}
	}

	BW::map<int,BW::wstring>::iterator it = possibleValuesMap.begin();
	BW::map<int,BW::wstring>::iterator end = possibleValuesMap.end();
	for (; it!=end; ++it )
	{
			possibleValues.push_back( it->second );
	}

	// make sure the old string value is valid, otherwise select the first valid value
	bool setDefault = oldStringValue.empty();
	if (setDefault)
	{
		oldStringValue = possibleValues.front();
		//INFO_MSG( Localise(L"COMMON/EDITOR_VIEWS/CHOICEVIEW_ELECT", oldStringValue, property_.name() ) );
	}

	PropertyItem* newItem = new ComboPropertyItem(property_.name(),
													oldStringValue.c_str(),
													possibleValues);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );

	if (setDefault)
		onChange( true );		// update the actual object property
}

//---------------------------------

void GenBoolView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = property_.pBool()->get();

	PropertyItem* newItem = new BoolPropertyItem(property_.name(), oldValue_);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void GenFloatView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	newValue_ = property_.pFloat()->get();
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	FloatPropertyItem* newItem = new FloatPropertyItem(property_.name(), property_.pFloat()->get());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	float min, max;
	int digits;
	if(property_.pFloat()->getRange( min, max, digits ))
	{
		newItem->setRange( min, max, digits );
	}
	float def;
	if(property_.pFloat()->getDefault( def ))
	{
		newItem->setDefault( def );
	}
	static Name multiplier( "multiplier" );
	if( property_.name() == multiplier )	// <--- sooo dodgy
	{
		newItem->setRange( 0, 3, 1 );
	}
	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}


//---------------------------------

void GenIntView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	newValue_ = property_.pInt()->get();
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	PropertyItem* newItem = new IntPropertyItem(property_.name(), property_.pInt()->get());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	int min, max;
	if(property_.pInt()->getRange( min, max ))
		((IntPropertyItem*)newItem)->setRange( min, max );
	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void GenUIntView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	newValue_ = property_.pUInt()->get();
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	PropertyItem* newItem = new UIntPropertyItem( property_.name(), property_.pUInt()->get());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	uint32 min, max;
	if(property_.pUInt()->getRange( min, max ))
		((UIntPropertyItem*)newItem)->setRange( min, max );
	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void GenPositionView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	Matrix matrix;
	property_.pMatrix()->getMatrix( matrix );
	oldValue_ = matrix.applyToOrigin();

	propertyItems_.reserve(3);

	static BW::string s_x( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/X_NAME" ) );
	static BW::string s_y( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/Y_NAME" ) );
	static BW::string s_z( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/Z_NAME" ) );

	FloatPropertyItem* newItem = new FloatPropertyItem( property_.name().c_str() + s_x, oldValue_.x);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	newItem = new FloatPropertyItem( property_.name().c_str() + s_y, oldValue_.y);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	newItem = new FloatPropertyItem( property_.name().c_str() + s_z, oldValue_.z);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);

	propTable_->addView( this );
}

//---------------------------------

void GenRotationView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = rotation();

	propertyItems_.reserve(3);


	static BW::string s_yaw( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/YAW" ) );
	static BW::string s_pitch( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/PITCH" ) );
	static BW::string s_roll( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/ROLL" ) );

	FloatPropertyItem* newItem = new FloatPropertyItem( property_.name().c_str() + s_yaw, oldValue_.y);
	newItem->setGroup( property_.getGroup() );
	newItem->setDigits( 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);


	newItem = new FloatPropertyItem( property_.name().c_str() + s_pitch, oldValue_.x);
	newItem->setGroup( property_.getGroup() );
	newItem->setDigits( 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);


	newItem = new FloatPropertyItem( property_.name().c_str() + s_roll, oldValue_.z);
	newItem->setGroup( property_.getGroup() );
	newItem->setDigits( 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);

	propTable_->addView( this );
}

//---------------------------------

void GenScaleView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = scale();

	propertyItems_.reserve(3);

	static BW::string s_x( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/X_NAME" ) );
	static BW::string s_y( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/Y_NAME" ) );
	static BW::string s_z( LocaliseUTF8( L"COMMON/EDITOR_VIEWS/Z_NAME" ) );

	FloatPropertyItem* newItem = new FloatPropertyItem( property_.name().c_str() + s_x, oldValue_.x);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	newItem = new FloatPropertyItem( property_.name().c_str() + s_y, oldValue_.y);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	newItem = new FloatPropertyItem( property_.name().c_str() + s_z, oldValue_.z);
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);

	propTable_->addView( this );
}
//---------------------------------

ColourView::ColourView( ColourProperty & property ) 
	: property_( property ),
	transient_( true ),
	addBarrier_( true )
{
}

ColourView::~ColourView()
{
}

void ColourView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	Moo::Colour c;
	if (!getCurrentValue( c ))
		return;

	newValue_ = c;
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	wchar_t buf[256];
	bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d , %d , %d , %d", (int)(c.r), (int)(c.g), (int)(c.b), (int)(c.a) );
	ColourPropertyItem* newItem = new ColourPropertyItem(property_.name(), buf, 1 );
	newItem->setGroup( property_.getGroup() );
	newItem->setGroupDepth( newItem->getGroupDepth() + 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	propertyItems_.push_back(newItem);

	// we need to add the view here otherwise the children will be added in the view as well ( we don't want that. Trust me )
	int listLocation = propTable_->addView( this );

	STATIC_LOCALISE_NAME( s_red, L"COMMON/EDITOR_VIEWS/RED" );
	STATIC_LOCALISE_STRING( s_redEnd, L"COMMON/EDITOR_VIEWS/RED_END" );
	STATIC_LOCALISE_NAME( s_green, L"COMMON/EDITOR_VIEWS/GREEN" );
	STATIC_LOCALISE_STRING( s_greenEnd, L"COMMON/EDITOR_VIEWS/GREEN_END" );
	STATIC_LOCALISE_NAME( s_blue, L"COMMON/EDITOR_VIEWS/BLUE" );
	STATIC_LOCALISE_STRING( s_blueEnd, L"COMMON/EDITOR_VIEWS/BLUE_END" );
	STATIC_LOCALISE_NAME( s_alpha, L"COMMON/EDITOR_VIEWS/ALPHA" );
	STATIC_LOCALISE_STRING( s_alphaEnd, L"COMMON/EDITOR_VIEWS/ALPHA_END" );

	IntPropertyItem* newColItem = new IntPropertyItem( s_red, (int)(c.r));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newColItem->UIDesc( s_redEnd + property_.UIDesc().c_str() );

	newColItem->setRange( 0, 255 );
	propertyItems_.push_back(newColItem);

	newColItem = new IntPropertyItem( s_green, (int)(c.g));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newColItem->UIDesc( s_greenEnd + property_.UIDesc().c_str() );

	newColItem->setRange( 0, 255 );
	propertyItems_.push_back(newColItem);

	newColItem = new IntPropertyItem( s_blue, (int)(c.b));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newColItem->UIDesc( s_blueEnd + property_.UIDesc().c_str() );

	newColItem->setRange( 0, 255 );
	propertyItems_.push_back(newColItem);

	newColItem = new IntPropertyItem( s_alpha, (int)(c.a));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newColItem->UIDesc( s_alphaEnd + property_.UIDesc().c_str() );

	newColItem->setRange( 0, 255 );
	propertyItems_.push_back(newColItem);

	propTable_->propertyList()->collapseGroup( newItem,
		listLocation, false);
}

void ColourView::onChange( bool transient, bool addBarrier )
{
	BW_GUARD;

	PropTableSetter pts(propTable_);

	int r,g,b,a;

	swscanf( item()->get().c_str(), L"%d , %d , %d , %d", &r, &g, &b, &a );
	Moo::Colour c1( (FLOAT)r, (FLOAT)g, (FLOAT)b, (FLOAT)a );

	r = item(0)->get();
	g = item(1)->get();
	b = item(2)->get();
	a = item(3)->get();

	Moo::Colour c2( (FLOAT)r, (FLOAT)g, (FLOAT)b, (FLOAT)a );

	if (!equal( c2, oldValue_ ))
	{
		changedValue_ = oldValue_;
		wchar_t buf[256];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d , %d , %d , %d", (int)(c2.r), (int)(c2.g), (int)(c2.b), (int)(c2.a) );
		item()->set( buf );

		newValue_ = c2;
	}
	else if (!equal( c1, oldValue_ ))
	{
		// TODO: This looks quite arbritary to say the least. We need to do
		// something better here.
		changedValue_.r = c1.r + 1;
		changedValue_.g = c1.g + 1;
		changedValue_.b = c1.b + 1;
		changedValue_.a = c1.a + 1;
		item(0)->set( (int)(c1.r) );
		item(1)->set( (int)(c1.g) );
		item(2)->set( (int)(c1.b) );
		item(3)->set( (int)(c1.a) );

		newValue_ = c1;
	}

	transient_ = transient;
	addBarrier_ = addBarrier;
}

void ColourView::cloneValue( BaseView* another )
{
	BW_GUARD;

	ColourView* view = (ColourView*)another;

	if (!almostEqual( (float)view->item(0)->get(), view->changedValue_.r ))
	{
		item(0)->set( view->item(0)->get() );
	}
	if (!almostEqual( (float)view->item(1)->get(), view->changedValue_.g ))
	{
		item(1)->set( view->item(1)->get() );
	}
	if (!almostEqual( (float)view->item(2)->get(), view->changedValue_.b ))
	{
		item(2)->set( view->item(2)->get() );
	}
	if (!almostEqual( (float)view->item(3)->get(), view->changedValue_.a ))
	{
		item(3)->set( view->item(3)->get() );
	}

}

bool ColourView::updateGUI()
{
	BW_GUARD;

	Moo::Colour c;
	getCurrentValue( c );

	bool changed = false;

	if (c != oldValue_)
	{
		changedValue_ = oldValue_;
		newValue_ = c;
		oldValue_ = c;
		item(0)->set( (int)(c.r) );
		item(1)->set( (int)(c.g) );
		item(2)->set( (int)(c.b) );
		item(3)->set( (int)(c.a) );
		wchar_t buf[64];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d , %d , %d , %d", (int)(c.r), (int)(c.g), (int)(c.b), (int)(c.a) );
		item()->set( buf );
		changed = true;
	}
		
	if ((newValue_ != oldValue_) || (!transient_))
	{
		if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
		{
			if (!transient_)
			{
				setCurrentValue( lastValue_, true );
				lastValue_ = newValue_;
			}
			setCurrentValue( newValue_, transient_ );
			if (newValue_ != oldValue_)
			{
				changed = true;
			}
			if (changed)
			{
				changedValue_ = oldValue_;
			}
			oldValue_ = newValue_;
			lastTimeStamp_ = timestamp();
			transient_ = true;
		}
	}
	return changed;
}

//---------------------------------
ColourScalarView::ColourScalarView( ColourScalarProperty & property ) 
	: property_( property ),
	transient_( true ),
	addBarrier_( true ),
	changedValue_( 0xFFFFFFFF )
{
}

ColourScalarView::~ColourScalarView()
{
}

void ColourScalarView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}

	Moo::Colour c;
	if (!getCurrentValue( c ))
		return;

	newValue_ = c;
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	wchar_t buf[256];
	bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d , %d , %d , %0.2f", (int)(c.r), (int)(c.g), (int)(c.b), c.a );
	ColourScalarPropertyItem* newItem = new ColourScalarPropertyItem( property_.name(), buf, 1 );
	newItem->setGroup( property_.getGroup() );
	newItem->setGroupDepth( newItem->getGroupDepth() + 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	propertyItems_.push_back(newItem);

	// we need to add the view here otherwise the children will be added in the view as well ( we don't want that. Trust me )
	int listLocation = propTable_->addView( this );

	STATIC_LOCALISE_NAME( s_red, L"COMMON/EDITOR_VIEWS/RED" );
	STATIC_LOCALISE_STRING( s_redEnd, L"COMMON/EDITOR_VIEWS/RED_END" );
	STATIC_LOCALISE_NAME( s_green, L"COMMON/EDITOR_VIEWS/GREEN" );
	STATIC_LOCALISE_STRING( s_greenEnd, L"COMMON/EDITOR_VIEWS/GREEN_END" );
	STATIC_LOCALISE_NAME( s_blue, L"COMMON/EDITOR_VIEWS/BLUE" );
	STATIC_LOCALISE_STRING( s_blueEnd, L"COMMON/EDITOR_VIEWS/BLUE_END" );
	STATIC_LOCALISE_NAME( s_alpha, L"COMMON/EDITOR_VIEWS/ALPHA" );
	STATIC_LOCALISE_STRING( s_alphaEnd, L"COMMON/EDITOR_VIEWS/ALPHA_END" );

	IntPropertyItem* newColItem = new IntPropertyItem(s_red, (int)(c.r));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newColItem->UIDesc( s_redEnd + property_.UIDesc().c_str() );

	newColItem->setRange( 0, 255 );
	propertyItems_.push_back(newColItem);

	newColItem = new IntPropertyItem(s_green, (int)(c.g));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newColItem->UIDesc( s_greenEnd + property_.UIDesc().c_str() );

	newColItem->setRange( 0, 255 );
	propertyItems_.push_back(newColItem);

	newColItem = new IntPropertyItem(s_blue, (int)(c.b));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newColItem->UIDesc( s_greenEnd + property_.UIDesc().c_str() );

	newColItem->setRange( 0, 255 );
	propertyItems_.push_back(newColItem);
	
	FloatPropertyItem* newAlphaItem = new FloatPropertyItem(property_.scalarName(), c.a);
	newItem->addChild( newAlphaItem );
	float minVal, maxVal;
	int digits;
	if (property_.pColour()->getRange( minVal, maxVal, digits ))
	{
		newAlphaItem->setRange( minVal, maxVal, digits );
	}
	else
	{
		newAlphaItem->setRange( 0.f, 10.f, 1 );
	}
	newAlphaItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	if ( !property_.UIDesc().empty() )
		newAlphaItem->UIDesc( s_alphaEnd + property_.UIDesc().c_str() );
	
	propertyItems_.push_back(newAlphaItem);

	propTable_->propertyList()->collapseGroup( newItem,
		listLocation, false);
}

void ColourScalarView::onChange( bool transient, bool addBarrier )
{
	BW_GUARD;

	PropTableSetter pts(propTable_);

	int r,g,b;
	float a;

	sscanf( item()->get().c_str(), "%d , %d , %d , %f", &r, &g, &b, &a );
	Moo::Colour c1( (FLOAT)r, (FLOAT)g, (FLOAT)b, a );

	r = item(0)->get();
	g = item(1)->get();
	b = item(2)->get();
	a = floatItem()->get();

	Moo::Colour c2( (FLOAT)r, (FLOAT)g, (FLOAT)b, a );

	if (!equal( c2, oldValue_ ))
	{
		changedValue_ = oldValue_;
		char buf[256];
		bw_snprintf( buf, sizeof(buf), "%d , %d , %d , %0.2f", (int)(c2.r), (int)(c2.g), (int)(c2.b), c2.a );
		item()->set( buf );

		newValue_ = c2;
	}
	else if (!equal( c1, oldValue_ ))
	{
		changedValue_.r = c1.r + 1;
		changedValue_.g = c1.g + 1;
		changedValue_.b = c1.b + 1;
		changedValue_.a = c1.a + 1;
		item(0)->set( (int)(c1.r) );
		item(1)->set( (int)(c1.g) );
		item(2)->set( (int)(c1.b) );
		floatItem()->set( c1.a );

		newValue_ = c1;
	}

	transient_ = transient;
	addBarrier_ = addBarrier;
}


void ColourScalarView::cloneValue( BaseView* another )
{
	BW_GUARD;

	ColourScalarView* view = (ColourScalarView*)another;
	if (view->item(0)->get() != (int)(view->changedValue_.r))
	{
		item(0)->set( view->item(0)->get() );
	}
	if (view->item(1)->get() != (int)(view->changedValue_.g))
	{
		item(1)->set( view->item(1)->get() );
	}
	if (view->item(2)->get() != (int)(view->changedValue_.b))
	{
		item(2)->set( view->item(2)->get() );
	}
	if (!almostEqual( view->floatItem()->get(), view->changedValue_.a ))
	{
		floatItem()->set( view->floatItem()->get() );
	}
}


bool ColourScalarView::updateGUI()
{
	BW_GUARD;

	bool changed = false;
	Moo::Colour c;
	getCurrentValue( c );

	if (c != oldValue_)
	{
		changedValue_ = oldValue_;
		getCurrentValue( c );
		newValue_ = c;
		oldValue_ = c;
		item(0)->set( (int)(c.r) );
		item(1)->set( (int)(c.g) );
		item(2)->set( (int)(c.b) );
		floatItem()->set( c.a );
		char buf[64];
		bw_snprintf( buf, sizeof(buf), "%d , %d , %d , %0.2f", (int)(c.r), (int)(c.g), (int)(c.b), c.a );
		item()->set( buf );
		changed = true;
	}
		
	if ((newValue_ != oldValue_) || (!transient_))
	{
		if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
		{
			if (!transient_)
			{
				setCurrentValue( lastValue_, true);
				lastValue_ = newValue_;
			}
			setCurrentValue( newValue_, transient_ );
			changed = ((newValue_ != oldValue_) ? true : false);
			if (changed)
			{
				changedValue_ = oldValue_;
			}
			oldValue_ = newValue_;
			lastTimeStamp_ = timestamp();
			transient_ = true;
		}
	}

	return changed;
}

//---------------------------------

MultiplierFloatView::MultiplierFloatView( GenFloatProperty & property )
	: property_( property )
{
	BW_GUARD;

	static Name multiplier( "multiplier" );
	isMultiplier_ = property.name() == multiplier;
}

MultiplierFloatView::~MultiplierFloatView()
{
}

void MultiplierFloatView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	if (!isMultiplier_)
		return;

	oldValue_ = property_.pFloat()->get();

	propTable_->addView( this );

	lastSeenSliderValue_ = oldValue_;

	propTable_->addView( this );
}

void MultiplierFloatView::onChange( bool transient, bool addBarrier )
{
}

bool MultiplierFloatView::updateGUI()
{
	return false;
}

//---------------------------------

void PythonView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	oldValue_ = getCurrentValue();

	StringPropertyItem* newItem = new StringPropertyItem(property_.name(), oldValue_.c_str());
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	// newItem->fileFilter( property_.fileFilter() );
	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------

void Vector2View::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}

	Vector2 c = property_.pVector2()->get();
	newValue_ = c;
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	wchar_t buf[256];
	bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f", (float)(c.x), (float)(c.y) );
	//Vector2PropertyItem* newItem = new Vector2PropertyItem(property_.name(), buf, 1, false /*property_.getGroupDepth() + 1*/ );
	ColourPropertyItem* newItem = new ColourPropertyItem(property_.name(), buf, 1, false );	
	newItem->setGroup( property_.getGroup() );
	newItem->setGroupDepth( newItem->getGroupDepth()/* + 1*/ );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	propertyItems_.push_back(newItem);

	// we need to add the view here otherwise the children will be added in the view as well ( we don't want that. Trust me )
	int listLocation = propTable_->addView( this );

	Vector2 minVal,maxVal;
	int precis;
	bool setMinMax = property_.pVector2()->getRange(minVal, maxVal, precis);

	STATIC_LOCALISE_NAME( s_x, "COMMON/EDITOR_VIEWS/X" );
	STATIC_LOCALISE_NAME( s_y, "COMMON/EDITOR_VIEWS/Y" );

	FloatPropertyItem* newSubItem = new FloatPropertyItem(s_x, (float)(c.x));	
	newItem->addChild( newSubItem );
	newSubItem->setChangeBuddy(this);

	if (setMinMax)
	{
		newSubItem->setRange( minVal.x, maxVal.x, precis );
	}

	propertyItems_.push_back(newSubItem);


	newSubItem = new FloatPropertyItem(s_y, (float)(c.y));
	newItem->addChild( newSubItem );
	newSubItem->setChangeBuddy(this);

	if (setMinMax)
	{
		newSubItem->setRange( minVal.y, maxVal.y, precis );
	}

	propertyItems_.push_back(newSubItem);
	propTable_->propertyList()->collapseGroup( newItem,
		listLocation, false);
}

void Vector2View::onChange( bool transient, bool addBarrier )
{
	BW_GUARD;

	PropTableSetter pts(propTable_);

	float x,y;

	swscanf( item()->get().c_str(), L"%f , %f", &x, &y );
	Vector2 c1( x, y );

	x = item(0)->get();
	y = item(1)->get();

	Vector2 c2( x, y );

	if (c2 != oldValue_)
	{
		changedValue_ = oldValue_;
		wchar_t buf[256];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f", c2.x, c2.y );
		item()->set( buf );

		newValue_ = c2;
	}
	else if (c1 != oldValue_)
	{
		changedValue_.x = c1.x + 1;
		changedValue_.y = c1.y + 1;
		item(0)->set( c1.x );
		item(1)->set( c1.y );
		newValue_ = c1;
	}

	transient_ = transient;
	addBarrier_ = addBarrier;
}


void Vector2View::cloneValue( BaseView* another )
{
	BW_GUARD;

	Vector2View* view = (Vector2View*)another;
	if (!almostEqual( view->item(0)->get(), view->changedValue_.x ))
	{
		item(0)->set( view->item(0)->get() );
	}
	if (!almostEqual( view->item(1)->get(), view->changedValue_.y ))
	{
		item(1)->set( view->item(1)->get() );
	}
}


bool Vector2View::updateGUI()
{
	BW_GUARD;

	Vector2 v; // = property_.pVector4()->get();
	getCurrentValue( v );

	bool changed = false;

	if (v != oldValue_)
	{
		changedValue_ = oldValue_;
		newValue_ = v;
		oldValue_ = v;
		item(0)->set( v.x );
		item(1)->set( v.y );		
		wchar_t buf[256];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f", v.x, v.y );
		item()->set( buf );
		changed = true;
	}
		
	if ((newValue_ != oldValue_) || (!transient_))
	{
		if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
		{
			if (!transient_)
			{
				setCurrentValue( lastValue_, true );
				lastValue_ = newValue_;
			}
			setCurrentValue( newValue_, transient_ );
			changed = newValue_ != oldValue_;
			if (changed)
			{
				changedValue_ = oldValue_;
			}
			oldValue_ = newValue_;
			lastTimeStamp_ = timestamp();
			transient_ = true;
		}
	}

	return changed;
}

//---------------------------------

void Vector3View::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}

	Vector3 c = property_.pVector3()->get();
	newValue_ = c;
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	wchar_t buf[256];
	bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f , %.2f", (float)(c.x), (float)(c.y), (float)(c.z) );
	ColourPropertyItem* newItem = new ColourPropertyItem(property_.name(), buf, 1, false /*property_.getGroupDepth() + 1*/ );
	newItem->setGroup( property_.getGroup() );
	newItem->setGroupDepth( newItem->getGroupDepth() + 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	propertyItems_.push_back(newItem);

	// we need to add the view here otherwise the children will be added in the view as well ( we don't want that. Trust me )
	int listLocation = propTable_->addView( this );

	STATIC_LOCALISE_NAME( s_x, "COMMON/EDITOR_VIEWS/X" );
	STATIC_LOCALISE_NAME( s_y, "COMMON/EDITOR_VIEWS/Y" );
	STATIC_LOCALISE_NAME( s_z, "COMMON/EDITOR_VIEWS/Z" );

	FloatPropertyItem* newColItem = new FloatPropertyItem(s_x, (float)(c.x));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	propertyItems_.push_back(newColItem);

	newColItem = new FloatPropertyItem(s_y, (float)(c.y));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	propertyItems_.push_back(newColItem);

	newColItem = new FloatPropertyItem(s_z, (float)(c.z));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	propertyItems_.push_back(newColItem);

	propTable_->propertyList()->collapseGroup( newItem,
		listLocation, false);
}

void Vector3View::onChange( bool transient, bool addBarrier )
{
	BW_GUARD;

	PropTableSetter pts(propTable_);

	float x,y,z;

	swscanf( item()->get().c_str(), L"%f , %f , %f", &x, &y, &z );
	Vector3 c1( x, y, z );

	x = item(0)->get();
	y = item(1)->get();
	z = item(2)->get();

	Vector3 c2( x, y, z );

	if (c2 != oldValue_)
	{
		changedValue_ = oldValue_;
		wchar_t buf[256];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f , %.2f", c2.x, c2.y, c2.z );
		item()->set( buf );

		newValue_ = c2;
	}
	else if (c1 != oldValue_)
	{
		changedValue_.x = c1.x + 1;
		changedValue_.y = c1.y + 1;
		changedValue_.z = c1.z + 1;
		item(0)->set( c1.x );
		item(1)->set( c1.y );
		item(2)->set( c1.z );

		newValue_ = c1;
	}

	transient_ = transient;
	addBarrier_ = addBarrier;
}

void Vector3View::cloneValue( BaseView* another )
{
	BW_GUARD;

	Vector3View* view = (Vector3View*)another;
	if (!almostEqual( view->item(0)->get(), view->changedValue_.x ))
	{
		item(0)->set( view->item(0)->get() );
	}
	if (!almostEqual( view->item(1)->get(), view->changedValue_.y ))
	{
		item(1)->set( view->item(1)->get() );
	}
	if (!almostEqual( view->item(2)->get(), view->changedValue_.z ))
	{
		item(2)->set( view->item(2)->get() );
	}
}

bool Vector3View::updateGUI()
{
	BW_GUARD;

	bool changed = false;
	Vector3 v; // = property_.pVector3()->get();
	getCurrentValue( v );

	if (v != oldValue_)
	{
		changedValue_ = oldValue_;
		newValue_ = v;
		oldValue_ = v;
		item(0)->set( v.x );
		item(1)->set( v.y );
		item(2)->set( v.z );
		wchar_t buf[256];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f , %.2f", v.x, v.y, v.z );
		item()->set( buf );
		changed = true;
	}
		
	if ((newValue_ != oldValue_) || (!transient_))
	{
		if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
		{
			if (!transient_)
			{
				setCurrentValue( lastValue_, true );
				lastValue_ = newValue_;
			}
			setCurrentValue( newValue_, transient_ );
			changed = newValue_ != oldValue_;
			if (changed)
			{
				changedValue_ = oldValue_;
			}
			oldValue_ = newValue_;
			lastTimeStamp_ = timestamp();
			transient_ = true;
		}
	}
	return changed;
}

//---------------------------------

void Vector4View::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}

	Vector4 c = property_.pVector4()->get();
	newValue_ = c;
	oldValue_ = newValue_;
	lastValue_ = newValue_;
	lastTimeStamp_ = 0;

	wchar_t buf[256];
	bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f , %.2f , %.2f", (float)(c.x), (float)(c.y), (float)(c.z), (float)(c.w) );

	ColourPropertyItem* newItem = new ColourPropertyItem(property_.name(), buf, 1, false /*property_.getGroupDepth() + 1*/ );
	newItem->setGroup( property_.getGroup() );
	newItem->setGroupDepth( newItem->getGroupDepth() + 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	propertyItems_.push_back(newItem);

	// we need to add the view here otherwise the children will be added in the view as well ( we don't want that. Trust me )
	int listLocation = propTable_->addView( this );

	STATIC_LOCALISE_NAME( s_x, "COMMON/EDITOR_VIEWS/X" );
	STATIC_LOCALISE_NAME( s_y, "COMMON/EDITOR_VIEWS/Y" );
	STATIC_LOCALISE_NAME( s_z, "COMMON/EDITOR_VIEWS/Z" );
	STATIC_LOCALISE_NAME( s_w, "COMMON/EDITOR_VIEWS/W" );

	FloatPropertyItem* newColItem = new FloatPropertyItem(s_x, (float)(c.x));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	propertyItems_.push_back(newColItem);

	newColItem = new FloatPropertyItem(s_y, (float)(c.y));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	propertyItems_.push_back(newColItem);

	newColItem = new FloatPropertyItem(s_z, (float)(c.z));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	propertyItems_.push_back(newColItem);

	newColItem = new FloatPropertyItem(s_w, (float)(c.w));
	newItem->addChild( newColItem );
	newColItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	propertyItems_.push_back(newColItem);

	propTable_->propertyList()->collapseGroup( newItem,
		listLocation, false);
}

void Vector4View::onChange( bool transient, bool addBarrier )
{
	BW_GUARD;

	PropTableSetter pts(propTable_);

	float x,y,z,w;

	swscanf( item()->get().c_str(), L"%f , %f , %f , %f", &x, &y, &z, &w );
	Vector4 c1( x, y, z, w );

	x = item(0)->get();
	y = item(1)->get();
	z = item(2)->get();
	w = item(3)->get();

	Vector4 c2( x, y, z, w );

	if (c2 != oldValue_)
	{
		changedValue_ = oldValue_;
		wchar_t buf[256];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f , %.2f , %.2f", c2.x, c2.y, c2.z, c2.w );
		item()->set( buf );

		newValue_ = c2;
	}
	else if (c1 != oldValue_)
	{
		changedValue_.x = c1.x + 1;
		changedValue_.y = c1.y + 1;
		changedValue_.z = c1.z + 1;
		changedValue_.w = c1.w + 1;
		item(0)->set( c1.x );
		item(1)->set( c1.y );
		item(2)->set( c1.z );
		item(3)->set( c1.w );

		newValue_ = c1;
	}

	transient_ = transient;
	addBarrier_ = addBarrier;
}

void Vector4View::cloneValue( BaseView* another )
{
	BW_GUARD;

	Vector4View* view = (Vector4View*)another;
	if (!almostEqual( view->item(0)->get(), view->changedValue_.x ))
	{
		item(0)->set( view->item(0)->get() );
	}
	if (!almostEqual( view->item(1)->get(), view->changedValue_.y ))
	{
		item(1)->set( view->item(1)->get() );
	}
	if (!almostEqual( view->item(2)->get(), view->changedValue_.z ))
	{
		item(2)->set( view->item(2)->get() );
	}
	if (!almostEqual( view->item(3)->get(), view->changedValue_.w ))
	{
		item(3)->set( view->item(3)->get() );
	}

}

bool Vector4View::updateGUI()
{
	BW_GUARD;

	bool changed = false;
	Vector4 v; // = property_.pVector4()->get();
	getCurrentValue( v );

	if (v != oldValue_)
	{
		changedValue_ = oldValue_;
		newValue_ = v;
		oldValue_ = v;
		item(0)->set( v.x );
		item(1)->set( v.y );
		item(2)->set( v.z );
		item(3)->set( v.w );
		wchar_t buf[256];
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%.2f , %.2f , %.2f , %.2f", v.x, v.y, v.z, v.w );
		item()->set( buf );
		changed = true;
	}
		
	if ((newValue_ != oldValue_) || (!transient_))
	{
		if (MILLIS_SINCE( lastTimeStamp_ ) > 100.f)
		{
			if (!transient_)
			{
				setCurrentValue( lastValue_, true );
				lastValue_ = newValue_;
			}
			setCurrentValue( newValue_, transient_ );
			changed = newValue_ != oldValue_;
			if (changed)
			{
				changedValue_ = oldValue_;
			}
			oldValue_ = newValue_;
			lastTimeStamp_ = timestamp();
			transient_ = true;
		}
	}
	return changed;
}

//---------------------------------

void MatrixView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}
	
	property_.pMatrix()->getMatrix( oldValue_, true );


	PropertyItem* newItem = new StringPropertyItem(property_.name(), NumVecToStr( (float*)&oldValue_, 16 ).c_str() );
	newItem->setGroup( property_.getGroup() );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );

	propertyItems_.push_back(newItem);
	propTable_->addView( this );
}

//---------------------------------


/**
 *	Constructor
 */
ArrayView::ArrayView( ArrayProperty & property ) 
	: property_( property )
{
}


/**
 *	Called when the ArrayProperty is elected. It creates a relevant property
 *	item and adds it the property list.
 */
void ArrayView::elect()
{
	BW_GUARD;

	if (!(propTable_ = PropTable::table()))
	{
		return;
	}

	BW::string label = property_.name().str();
	label += " (array)";
	ArrayPropertyItem* newItem = new ArrayPropertyItem( Name( label ), property_.name(), L"", property_.proxy() );
	newItem->setGroup( property_.getGroup().str() + property_.name().c_str() );
	newItem->setGroupDepth( newItem->getGroupDepth() + 1 );
	newItem->setChangeBuddy(this);

	newItem->descName( property_.descName() );
	newItem->UIDesc( property_.UIDesc() );
	newItem->exposedToScriptName( property_.exposedToScriptName() );
	newItem->canExposeToScript( property_.canExposeToScript() );

	propertyItems_.push_back(newItem);

	propTable_->addView( this );

	// Must tell the property list that we are inserting an array, so the views
	// get displayed correctly and include a "Delete Item" button.
	propTable_->propertyList()->startArray(
		new BWFunctor1<ArrayView,int>(
			this, &ArrayView::delItem ),
			property_.proxy()->readOnly() );
}


/**
 *	Called when the ArrayProperty has finished electing. It notifies
 *	the propTable that an array is ended.
 */
void ArrayView::elected()
{
	BW_GUARD;

	// done adding items to the array property.
	propTable_->propertyList()->endArray();
}


/**
 *	Unused implementation of pure virtual method
 */
void ArrayView::onChange( bool transient, bool addBarrier )
{
}

void ArrayView::cloneValue( BaseView * another)
{
}

/**
 *	Unused implementation of pure virtual method
 */
bool ArrayView::updateGUI()
{
	return false;
}


// View factory initialiser.
TextView::Enroller TextView::Enroller::s_instance;
StaticTextView::Enroller StaticTextView::Enroller::s_instance;
TextLabelView::Enroller TextLabelView::Enroller::s_instance;
IDView::Enroller IDView::Enroller::s_instance;
GroupView::Enroller GroupView::Enroller::s_instance;
ListTextView::Enroller ListTextView::Enroller::s_instance;
ChoiceView::Enroller ChoiceView::Enroller::s_instance;
GenBoolView::Enroller GenBoolView::Enroller::s_instance;
GenFloatView::Enroller GenFloatView::Enroller::s_instance;
GenIntView::Enroller GenIntView::Enroller::s_instance;
GenUIntView::Enroller GenUIntView::Enroller::s_instance;
GenPositionView::Enroller GenPositionView::Enroller::s_instance;
GenRotationView::Enroller GenRotationView::Enroller::s_instance;
GenScaleView::Enroller GenScaleView::Enroller::s_instance;
ColourView::Enroller ColourView::Enroller::s_instance;
ColourScalarView::Enroller ColourScalarView::Enroller::s_instance;
MultiplierFloatView::Enroller MultiplierFloatView::Enroller::s_instance;
PythonView::Enroller PythonView::Enroller::s_instance;
Vector2View::Enroller Vector2View::Enroller::s_instance;
Vector3View::Enroller Vector3View::Enroller::s_instance;
Vector4View::Enroller Vector4View::Enroller::s_instance;
MatrixView::Enroller MatrixView::Enroller::s_instance;
ArrayView::Enroller ArrayView::Enroller::s_instance;
BW_END_NAMESPACE

