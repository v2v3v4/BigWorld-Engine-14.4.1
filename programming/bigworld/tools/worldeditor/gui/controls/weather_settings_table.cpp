// weather_settings_table.cpp : implementation file
//
#include "pch.hpp"
#include "weather_settings_table.hpp"
#include "common/user_messages.hpp"
#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "resmgr/xml_section.hpp"
#include "ual/ual_manager.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
// Section: WeatherSettingsTable
///////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor
 */
WeatherSettingsTable::WeatherSettingsTable( UINT dlgId )
	: PropertyTable( dlgId )
	, pSystem_( NULL )	
{
}


/**
 *	Destructor
 */
WeatherSettingsTable::~WeatherSettingsTable()
{
}


void WeatherSettingsTable::initDragDrop()
{
	BW_GUARD;

	UalManager::instance().dropManager().add(
		new UalDropFunctor< WeatherSettingsTable >(
			propertyList(),
			"xml",
			this,
			&WeatherSettingsTable::doDrop,
			false,
			&WeatherSettingsTable::dropTest ) );
}


/**
 *	Initialises the property list from the weather settings.
 *
 *	@param			DataSectionPtr	weather system information.
 *  @returns		true if successful, false otherwise.
 */
bool WeatherSettingsTable::init( DataSectionPtr pSection, bool reset )
{
	BW_GUARD;

	if (!pSection)
	{
		if (reset)
		{
			propertyList()->deselectCurrentItem();

			if (editor_)
			{
				editor_->expel();
			}
		}

		return false;
	}

	if (pSection == pSystem_)
		return false;

	pSystem_ = pSection;

	PropTableSetter setter( this );

	int oldSel = propertyList()->GetCurSel();
	propertyList()->deselectCurrentItem();

	if ( editor_ )
	{
		editor_->expel();
	}

	// Must tell the smartpointer that the reference is already incremented,
	// because the PyObjectPlus base class increments the refcnt (!)
	editor_ = GeneralEditorPtr( new GeneralEditor(), true );

	GeneralProperty * gp ;

	STATIC_LOCALISE_NAME( s_specialEffectFile, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/SPECIAL_EFFECT_FILE" );
	STATIC_LOCALISE_NAME( s_specialEffectFileTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/SPECIAL_EFFECT_FILE/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_sunLight, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/SUNLIGHT" );
	STATIC_LOCALISE_NAME( s_sunLightTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/SUNLIGHT/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_ambient, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/AMBIENT" );
	STATIC_LOCALISE_NAME( s_ambientTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/AMBIENT/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_fog, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/FOG" );
	STATIC_LOCALISE_NAME( s_fogTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/FOG/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_fogFactor, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/FOG_FACTOR" );
	STATIC_LOCALISE_NAME( s_fogFactorTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/FOG_FACTOR/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_windSpeed, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/WIND_SPEED" );
	STATIC_LOCALISE_NAME( s_windSpeedTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/WIND_SPEED/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_windGustiness, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/WIND_GUSTINESS" );
	STATIC_LOCALISE_NAME( s_windGustinessTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/WIND_GUSTINESS/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_attenuation, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/ATTENUATION" );
	STATIC_LOCALISE_NAME( s_attenuationTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/ATTENUATION/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_numPasses, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/NUM_PASSES" );
	STATIC_LOCALISE_NAME( s_numPassesTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/NUM_PASSES/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_sensitivity, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/SENSITIVITY" );
	STATIC_LOCALISE_NAME( s_sensitivityTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/SENSITIVITY/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_filterWidth, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/FILTER_WIDTH" );
	STATIC_LOCALISE_NAME( s_filterWidthTooltip, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/FILTER_WIDTH/TOOLTIP" );
	STATIC_LOCALISE_NAME( s_multiplier, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/MULTIPLIER" );
	STATIC_LOCALISE_NAME( s_density, "WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/DENSITY" );
	STATIC_LOCALISE_NAME( s_bloomSettings, L"WORLDEDITOR/WORLDEDITOR/PROPERTIES/WEATHER_EDITOR/BLOOM_SETTINGS" );
	STATIC_LOCALISE_NAME( s_fileFilter, "Special Effect files(*.xml;*.sfx)|*.xml;*.sfx||" );
	//fx	string

	Name sectionName( pSystem_->sectionName() + '/' );

	TextProperty* tp = new TextProperty( s_specialEffectFile,
		new DataSectionStringProxy(
			pSystem_, "sfx", "onWeatherAdjust", "" ));	
	tp->fileFilter( s_fileFilter );
	tp->setGroup( sectionName );
	tp->UIDesc( s_specialEffectFileTooltip );
	editor_->addProperty(tp);
	
	//sun	Vector4
	gp = new ColourScalarProperty( s_sunLight, s_multiplier,
		new ClampedDataSectionColourScalarProxy(
			pSystem_,
			"sun",
			"onWeatherAdjust",
			Vector4(1.f,1.f,1.f,1.f),
			0.1f, 5.f, 1));
	gp->setGroup( sectionName );
	gp->UIDesc( s_sunLightTooltip );
	editor_->addProperty( gp );

	//ambient	Vector4
	gp = new ColourScalarProperty( s_ambient, s_multiplier,
		new ClampedDataSectionColourScalarProxy(
			pSystem_,
			"ambient",
			"onWeatherAdjust",
			Vector4(1.f,1.f,1.f,1.f),
			0.1f, 5.f, 1));
	gp->setGroup( sectionName );
	gp->UIDesc( s_ambientTooltip );
	editor_->addProperty(gp);

	//fog	Vector4
	gp = new ColourScalarProperty( s_fog, s_density,
		new ClampedDataSectionColourScalarProxy(
			pSystem_,
			"fog",
			"onWeatherAdjust",
			Vector4(1.f,1.f,1.f,1.f),
			1.f, 5.f, 1));
	gp->setGroup( sectionName );
	gp->UIDesc( s_fogTooltip );
	editor_->addProperty(gp);

	//fogFactor	float
	gp = new GenFloatProperty( s_fogFactor,
		new ClampedDataSectionFloatProxy(
			pSystem_,
			"fogFactor",
			"onWeatherAdjust",
			0.15f, 0.f, 2.f, 2) );
	gp->setGroup( sectionName );
	gp->UIDesc( s_fogFactorTooltip );
	editor_->addProperty(gp);

	//windSpeed	Vector2
	gp = new Vector2Property( s_windSpeed,
		new ClampedDataSectionVector2Proxy(
			pSystem_,
			"windSpeed",
			"onWeatherAdjust",
			Vector2(0.f,0.f),
			Vector2(-15.f,-15.f),
			Vector2(15.f,15.f)));
	gp->setGroup( sectionName );
	gp->UIDesc( s_windSpeedTooltip );
	editor_->addProperty(gp);

	//gustiness	float
	gp = new GenFloatProperty( s_windGustiness,
		new ClampedDataSectionFloatProxy(
			pSystem_,
			"windGustiness",
			"onWeatherAdjust",
			0.f, 0.f, 15.f, 1) );
	gp->setGroup( sectionName );
	gp->UIDesc( s_windGustinessTooltip );
	editor_->addProperty(gp);


	//bloom
	
	//attenuation Vector4
	gp = new ColourScalarProperty( s_attenuation, s_multiplier,
		new ClampedDataSectionColourScalarProxy(
			pSystem_,
			"bloom/attenuation",
			"onWeatherAdjust",
			Vector4(1.f,1.f,1.f,1.f),
			1.f, 2.f, 4));
	gp->setGroup( s_bloomSettings );
	gp->UIDesc( s_attenuationTooltip );
	editor_->addProperty(gp);

	//numPasses	int
	gp = new GenIntProperty( s_numPasses,
		new ClampedDataSectionIntProxy(
			pSystem_,
			"bloom/numPasses",
			"onWeatherAdjust",
			2, 0, 15, 1) );
	gp->setGroup( s_bloomSettings );
	gp->UIDesc( s_numPassesTooltip );
	editor_->addProperty(gp);

	//power	float
	gp = new GenFloatProperty( s_sensitivity,
		new ClampedDataSectionFloatProxy(
			pSystem_,
			"bloom/power",
			"onWeatherAdjust",
			8, 0.5f, 64.f, 1) );
	gp->setGroup( s_bloomSettings );
	gp->UIDesc( s_sensitivityTooltip );
	editor_->addProperty(gp);

	//width	int
	gp = new GenIntProperty( s_filterWidth,
		new ClampedDataSectionIntProxy(
			pSystem_,
			"bloom/width",
			"onWeatherAdjust",
			1, 1, 15, 1) );
	gp->setGroup( s_bloomSettings );
	gp->UIDesc( s_filterWidthTooltip );
	editor_->addProperty(gp);

	editor_->elect();

	return true;
}


/**
 *	DoDataExchange
 */
void WeatherSettingsTable::DoDataExchange(CDataExchange* pDX)
{
	PropertyTable::DoDataExchange(pDX);
}


/**
 *	MFC Message Map
 */
BEGIN_MESSAGE_MAP(WeatherSettingsTable, CFormView)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_MESSAGE(WM_SELECT_PROPERTYITEM, OnSelectPropertyItem)
	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)
	ON_MESSAGE(WM_DBLCLK_PROPERTYITEM, OnDblClkPropertyItem)
END_MESSAGE_MAP()


/**
 *	Called every frame by the tool. Inits the property list (if not already
 *	inited) and calls update on the base class.
 */
afx_msg LRESULT WeatherSettingsTable::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	init( NULL, false );
	update();
	return 0;
}


/**
 *	A list item has been selected
 */
afx_msg LRESULT WeatherSettingsTable::OnSelectPropertyItem(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		BW_GUARD;

		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onSelect();
	}

	return 0;
}


/**
 *	A list item's value has been changed
 */
afx_msg LRESULT WeatherSettingsTable::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		BW_GUARD;

		BaseView* relevantView = (BaseView*)lParam;
		relevantView->onChange(wParam != 0);
	}

	return 0;
}


/**
 *	A list item has been right-clicked
 */
afx_msg LRESULT WeatherSettingsTable::OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		BW_GUARD;

		PropertyItem* relevantView = (PropertyItem*)lParam;
		relevantView->onBrowse();
	}
	
	return 0;
}


RectInt WeatherSettingsTable::dropTest( UalItemInfo* ii )
{
	BW_GUARD;

	CRect rect = propertyList()->dropTest( CPoint( ii->x(), ii->y() ),
		BWResource::dissolveFilenameW( ii->longText() ) );
	return RectInt( rect.left, rect.bottom, rect.right, rect.top );
}


bool WeatherSettingsTable::doDrop( UalItemInfo* ii )
{
	BW_GUARD;

	return propertyList()->doDrop( CPoint( ii->x(), ii->y() ),
		BWResource::dissolveFilenameW( ii->longText() ) );
}

BW_END_NAMESPACE

