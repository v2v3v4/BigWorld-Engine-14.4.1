#include "pch.h"
#include <vcl.h>
#pragma hdrstop
#include "lighting_plan.h"
#include "chunk/chunk.hpp"
#include "moo/omni_light.hpp"
#include "moo/spot_light.hpp"
#include "moo/directional_light.hpp"
#include "romp/geometrics.hpp"
#include "resmgr/bwresource.hpp"
#include "common_utility.h"

/**
 * 	This method loads the lighting plan from a file
 *
 *	@param filename		The name of the file to load
 */
bool LightingPlan::load( const BW::string & filename )
{
	bool result = false;

	DataSectionPtr spSection = BWResource::openSection( filename );
	if ( spSection )
    {
        //Load all the lights from the lights file
        DataSectionPtr spLights = spSection->openSection( "Lights" );
        if ( spLights )
        {
            BW::vector<DataSectionPtr> lightSections;

            spLights->openSections( "Light", lightSections );

            if ( lightSections.size( ) )
            {
                //Get ready to load lights
                lights_.clear();
				result = true;

                for ( int i = 0; i < lightSections.size( ); i++ )
                {
                    DataSectionPtr spLight = lightSections[i];

					VirtualLight vLight;
					
					if ( vLight.load( spLight ) )
					{
						this->add( vLight );
                    }
                }
            } //if light sections.size
        } //if spLights
	} //if pSection

	return result;
}


/**
 * 	This method loads a single virtual light from a data section.
 *
 *	@param spLightSection	The <Light></Light> section to load from
 */
bool LightingPlan::VirtualLight::load( DataSectionPtr spLightSection )
{
	Vector3 location = spLightSection->readVector3( "Location" );
	Vector3 orientation = spLightSection->readVector3( "Orientation", Vector3(0,0,1) );
	Vector3 scale = spLightSection->readVector3( "Scale", Vector3(1,1,1) );
	BW::string type = spLightSection->readString( "Type", "Omni" );
	enabled_ = spLightSection->readBool( "Enabled", true );
	Vector3 colour = spLightSection->readVector3( "Color", Vector3(1,1,1) );
	inner_ = spLightSection->readFloat( "Full_Radius", 5.f );
	outer_ = spLightSection->readFloat( "Falloff_Radius", 10.f );
	angle_ = spLightSection->readFloat( "Cone_Size", 15.f );
    visible_ = spLightSection->readBool( "Visible", true );

	if ( type == "Ambient" )
		type_ = LIGHT_AMBIENT;
	else if ( type == "Spot" )
		type_ = LIGHT_SPOT;
	else if ( type == "Directional" )
		type_ = LIGHT_DIRECTIONAL;
	else
		type_ = LIGHT_OMNI;

	transform_.lookAt( Vector3(0,0,0), orientation, Vector3(0,1,0) );
	transform_.invert();
    TColor col = CommonUtility::vectorToTColor( colour );
    colour_ = CommonUtility::TColorToMooColour( col );
	Vector3* translation = (Vector3*)&transform_._41;
	*translation = location;

	return true;
}


/**
 * 	This method calls the correct draw method for this light.
 */
void LightingPlan::VirtualLight::draw()
{
	switch ( type_ )
    {
    	case LIGHT_AMBIENT:
        break;
        case LIGHT_OMNI:
        	this->drawOmni();
        break;
        case LIGHT_SPOT:
        	this->drawSpot();
        break;
        case LIGHT_DIRECTIONAL:
        	this->drawDirectional();
        break;
    }
}


/**
 * 	This method draws an omni light, as two nestled spheres
 */
void LightingPlan::VirtualLight::drawOmni()
{
	int colour = colour_ & 0x00ffffff;
    colour |= 0x20000000;

	Geometrics::instance().wireSphere(
        transform_.applyToOrigin(),
        inner_,
        colour );

    Geometrics::instance().wireSphere(
        transform_.applyToOrigin(),
        outer_,
        colour );
}


/**
 * 	This method draws a spot light, as a cone and a box
 */
void LightingPlan::VirtualLight::drawSpot()
{
}


/**
 * 	This method draws a directional light, as a cylinder
 */
void LightingPlan::VirtualLight::drawDirectional()
{
}


/**
 * 	This method saves the lighting plan to a file
 *	@param filename		The name of the file to save
 */
bool LightingPlan::save( const BW::string & filename )
{
	bool result = false;

	DataSectionPtr spSection = BWResource::instance().rootSection()->openSection( filename, true );
	if ( spSection )
    {
		//Check if the file exists already
		DataSectionPtr spOldLights = spSection->openSection( "Lights", false );
		if ( spOldLights )
		{
			spSection->deleteSection( "Lights" );
		}

        //Save all the lights from the lights file
        DataSectionPtr spLights = spSection->openSection( "Lights", true );
        if ( spLights )
        {
			VirtualLights::iterator it = lights_.begin();
			VirtualLights::iterator end = lights_.end();

			while ( it != end )
			{
				VirtualLight& l = *it++;

				l.save( spLights );
			}

            result = spSection->save();
        } //if spLights
	} //if pSection

	return result;
}


/**
 * 	This method saves a light to a data section
 *	@param filename		The datasection to save to.
 */
bool LightingPlan::VirtualLight::save( DataSectionPtr spLight )
{
	bool result = false;

	DataSectionPtr spSection = spLight->newSection( "Light" );
	if ( spSection )
	{
    	BW::string type = "Omni";
		if ( type_ == LIGHT_AMBIENT )
			type = "Ambient";
		else if ( type_ == LIGHT_SPOT )
			type = "Spot";
		else if ( type_ == LIGHT_DIRECTIONAL )
			type = "Directional";
		else
			type = "Omni";

        spSection->writeString( "Type", type );
        
		spSection->writeVector3( "Location", transform_.applyToOrigin() );
		spSection->writeVector3( "Orientation", transform_.applyToUnitAxisVector(2) );
		spSection->writeVector3( "Scale", Vector3(1,1,1) );
		spSection->writeBool( "Enabled", enabled_ );
        spSection->writeBool( "Visible", visible_ );

		TColor col = CommonUtility::mooColourToTColor( colour_ );
		spSection->writeVector3( "Color", CommonUtility::TColorToVector( col ) );

		spSection->writeFloat( "Full_Radius", inner_ );
		spSection->writeFloat( "Falloff_Radius", outer_ );
		spSection->writeFloat( "Cone_Size", angle_ );

		result = true;
	}

	return result;
}



/**
 *	This method adds a new light to the plan, and updates the current light
 *	index.
 */
void LightingPlan::add()
{
	VirtualLight v;
	this->add( v );
}


/**
 *	This method adds a new light to the plan, and updates the current light
 *	index.
 */
void LightingPlan::add( const VirtualLight& vLight )
{
	lights_.push_back( vLight );
	current_ = lights_.size()-1;

    reIndex();
}


/**
 *	This method deletes the currently selected light from the plan.
 */
void LightingPlan::del()
{
	VirtualLight & l = light();

    if ( l.index_ >= 0 )
    {
		lights_.erase( lights_.begin() + l.index_ );
    }

    reIndex();
}


/**
 *	This method updates the indices of the lights.
 *	TODO : remove this horrible making-explicit-of-an-already-existing-implicit-thing
 */
void LightingPlan::reIndex()
{
	VirtualLights::iterator it = lights_.begin();
	VirtualLights::iterator end = lights_.end();

    int idx = 0;

    while ( it != end )
    {
    	(*it++).index_ = idx++;
    }
}


/**
 * 	This method draws all visible lights in the lighting plan.
 */
void LightingPlan::draw()
{
	VirtualLights::iterator it = lights_.begin();
	VirtualLights::iterator end = lights_.end();

    while ( it != end )
    {
    	VirtualLight& vl = *it++;
    	if ( vl.enabled_ )
        	vl.draw();
    }
}


int LightingPlan::current()
{
    //Need to do the int cast otherwise -1 (no light) is never returned
    return min(current_,int(lights_.size() - 1));
}

void LightingPlan::current( int index )
{
    current_ = index;
    if (lights_.size() != 0)
        current_ %= lights_.size();
    else
        current_ = -1;
}


/**
 *	This method sets the lighting plan into a chunk.
 */
void ChunkLightingPlan::to( Chunk& chunk )
{
	//TODO
	MF_ASSERT( "ChunkLightingPlan::to( Chunk& chunk ) has not been implemented" );
}


/**
 *	This method takes the lights from a chunk, and creates a plan.
 */
void ChunkLightingPlan::from( const Chunk& chunk )
{
	//TODO
	MF_ASSERT( "ChunkLightingPlan::to( Chunk& chunk ) has not been implemented" );
}


/**
 *	This method sets the lighting plan into a light container.
 */
void LightContainerLightingPlan::to( Moo::LightContainerPtr plc )
{
	MF_ASSERT( plc );

    plc->ambientColour( Moo::Colour( 0xff000000 ) );
    plc->omnis().clear();
    plc->directionals().clear();
    plc->spots().clear();

    VirtualLights::iterator it = lights_.begin();
	VirtualLights::iterator end = lights_.end();

    while ( it != end )
    {
    	VirtualLight& l = *it++;

		if ( l.enabled_ )
		{
    		switch ( l.type_ )
			{
				case LIGHT_AMBIENT:
					{
            			plc->ambientColour( l.colour_ );
					}
				break;
				case LIGHT_OMNI:
					{
						Moo::OmniLightPtr o = new Moo::OmniLight(
							l.colour_, l.transform_.applyToOrigin(), l.inner_, l.outer_ );
            			plc->addOmni( o );
					}
				break;
				case LIGHT_DIRECTIONAL:
					{
                    	Vector3 dir(0,0,0);
                        dir -= l.transform_.applyToUnitAxisVector(2);

						Moo::DirectionalLightPtr d = new Moo::DirectionalLight(
							l.colour_, dir );
						plc->addDirectional( d );
					}
				break;
				case LIGHT_SPOT:
					{
                    	Vector3 dir(0,0,0);
                        dir -= l.transform_.applyToUnitAxisVector(2);
                        
						Moo::SpotLightPtr s = new Moo::SpotLight(
							l.colour_, l.transform_.applyToOrigin(), dir,
							l.inner_, l.outer_, cosf( DEG_TO_RAD(l.angle_)) );
						plc->addSpot( s );
					}
				break;
				case LIGHT_NONE:	//fall through
				default:
				break;
			}
		}
    }
}


/**
 *	This method creates a lighting plan from a light container.
 */
void LightContainerLightingPlan::from( const Moo::LightContainerPtr plc )
{
	//TODO
	MF_ASSERT( "LightContainerLightingPlan::from( Moo::LightContainerPtr plc ) has not been implemented" );
}


/**
 *	This method creates the items in a combo box.
 */
void TFlatComboBoxLightingPlan::to( TFlatComboBox& cmb )
{
	//remove all items
    cmb.Clear();

    //add all items
    VirtualLights::iterator it = lights_.begin();
	VirtualLights::iterator end = lights_.end();

    while ( it != end )
    {
    	AnsiString name;
        VirtualLight& l = *it++;
    	switch ( l.type_ )
        {
            case LIGHT_AMBIENT:
            	name = "Ambient";
            break;
            case LIGHT_OMNI:
            	name = "Omni";
            break;
            case LIGHT_DIRECTIONAL:
            	name = "Directional";
            break;
            case LIGHT_SPOT:
            	name = "Spot";
            break;
            case LIGHT_NONE:	//fall through
            default:
            	name = "Undefined";
            break;
        }
        if ( l.enabled_ )
        	name += "[on]";
        else
        	name += "[off]";
		cmb.Items->Add( name );
    }
}


void TFlatComboBoxLightingPlan::from( LightingPlan& plan )
{
	this->lights_ = plan.lights();
}
