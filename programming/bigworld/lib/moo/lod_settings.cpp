// Module Interface
#include "pch.hpp"
#include "lod_settings.hpp"

// BW Tech Headers
#include "moo/graphics_settings.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: LodSettings
// -----------------------------------------------------------------------------

/**
 *	Initialises the graphics settings registry.
 *
 */
void LodSettings::init(DataSectionPtr resXML)
{
	BW_GUARD;
	// register object lod settigns
	typedef Moo::GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;
	this->lodSettings_ = Moo::makeCallbackGraphicsSetting(
			"OBJECT_LOD", "Object Level of Detail", *this, 
			&LodSettings::setLodOption,
			-1, false, false);

	if (resXML.exists())
	{
		DataSectionIterator sectIt  = resXML->begin();
		DataSectionIterator sectEnd = resXML->end();
		while (sectIt != sectEnd)
		{
			static const float undefined = -1.0f;
			float value = (*sectIt)->readFloat("value", undefined);
			BW::string label = (*sectIt)->readString("label");
			if (!label.empty() && value != undefined)
			{
				this->lodSettings_->addOption(label, label, true);
				this->lodOptions_.push_back(value);
			}
			++sectIt;
		}
	}
	else
	{
		this->lodSettings_->addOption("HIGH", "High", true);
		this->lodOptions_.push_back(1.0f);
	}

	Moo::GraphicsSetting::add(this->lodSettings_);
}


float LodSettings::applyLodBias(float distance)
{
	BW_GUARD;
#ifdef EDITOR_ENABLED
	if (ignoreLods_)
	{
		return 0;
	}
	else
#endif 
	if (this->lodSettings_.exists())
	{
		MF_ASSERT(this->lodSettings_->activeOption() < int(this->lodOptions_.size()));
		return distance / this->lodOptions_[this->lodSettings_->activeOption()];
	}
	else
	{
		return distance;
	}
}


void LodSettings::applyLodBias(float & lodNear, float & lodFar)
{
	BW_GUARD;
#ifdef EDITOR_ENABLED
	if (ignoreLods_)
	{
		lodNear = 0;
		lodFar  = 1000000.0f;
	}
	else 
#endif
	if (this->lodSettings_.exists()) 
	{
		MF_ASSERT(this->lodSettings_->activeOption() < int(this->lodOptions_.size()));
		const float & lodFactor = this->lodOptions_[this->lodSettings_->activeOption()];
		lodNear = lodNear * lodFactor;
		lodFar  = lodFar  * lodFactor;
	}
}


/**
 *	Returns singleton FloraSettings instance.
 */
LodSettings & LodSettings::instance()
{
	static LodSettings instance;
	return instance;
}


void LodSettings::setLodOption(int optionIdx)
{}


/**
*	This help methods calculate the lod value associated to
*	the world matrix of the to be rendered object
*
*	@param	matWorld	The world matrix to render with.
*	@returns			the lod value.
*/
float LodSettings::calculateLod( const Matrix & matWorld )
{
	float yScale = matWorld.applyToUnitAxisVector(1).length();
	return calculateLod( matWorld.applyToOrigin(), yScale );
}

/**
*	This help methods calculate the lod value associated to
*	the position and yScale of the to be rendered object.
*
*	This function must return a value >= 0 because there are special
*	constants which are < 0.
*	Such as Model::LOD_HIDDEN or Model::LOD_MISSING.
*
*	@param	position	the position of the rendered object
*	@param	yScale		the yScale of the rendered object
*	@returns			the lod value.
*/
float LodSettings::calculateLod( const Vector3& position, float yScale  )
{
	const Matrix & mooView = Moo::rc().lodView();

	float distance = position.dotProduct(
		Vector3( mooView._13, mooView._23, mooView._33 ) ) + mooView._43;

 	// Clamp it between 0-max lod
 	distance = std::max( distance, 0.0f );

	distance = applyLodBias(distance);
	return (distance / max( 1.f, yScale )) * Moo::rc().lodZoomFactor();
}

BW_END_NAMESPACE

// settings_registry.cpp
