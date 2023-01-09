#include "pch.hpp"

#include "gizmo/general_properties.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/string_provider.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"

#include "mru.hpp"

#include "me_light_proxies.hpp"

DECLARE_DEBUG_COMPONENT( 0 )
#include "me_error_macros.hpp"

#include "lights.hpp"

BW_BEGIN_NAMESPACE


static AutoConfigString s_default_lights( "system/defaultLightsPath" );

/*static*/ Lights* Lights::s_instance_ = NULL;

void GeneralLight::elect()
{
	BW_GUARD;

	editor_->elect();
}

void GeneralLight::expel()
{
	BW_GUARD;

	editor_->expel();
}

void GeneralLight::enabled( bool v )
{
	BW_GUARD;

	enabled_ = v;
	Lights::instance().dirty( true );
	Lights::instance().regenerateLightContainer();
}

bool GeneralLight::enabled()
{
	return enabled_;
}

AmbientLight::AmbientLight()
{
	light_ = Moo::Colour( 1.f, 1.f, 1.f, 1.f );
}

Moo::Colour AmbientLight::colour()
{
	return light_;
}
void AmbientLight::colour( Moo::Colour v )
{
	BW_GUARD;

	light_ = v;
	Lights::instance().regenerateLightContainer();
}
Moo::Colour AmbientLight::light()
{
	return light_;
}


EditorAmbientLight::EditorAmbientLight()
{
	BW_GUARD;

	enabled_ = false;
	light_ = new AmbientLight();
	editor_ = GeneralEditorPtr(new GeneralEditor(), true);

	//Colour
	ColourProxyPtr colourProxy = new MeLightColourWrapper<AmbientLight>( light_ );
	ColourProperty* pColProp = new ColourProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/COLOUR" ), colourProxy );
	pColProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/AMBIENT_COLOUR") );
	editor_->addProperty( pColProp );
}

OmniLight::OmniLight()
{
	BW_GUARD;

	GeneralProperty* pProp = NULL;
	
	//Colour
	ColourProxyPtr colourProxy = new MeLightColourWrapper<Moo::OmniLight>( light_ );
	pProp = new ColourProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/COLOUR" ), colourProxy );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/OMNI_COLOUR" ) );
	editor_->addProperty( pProp );

	//Position
	matrixProxy_ = new MeLightPosMatrixProxy<Moo::OmniLight>(
		light_,
		&matrix_ );
	pProp = new GenPositionProperty(LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/POSITION" ), matrixProxy_);
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/OMNI_POSITION" ) );
	editor_->addProperty( pProp );

	//Inner Radius
	FloatProxyPtr minRadiusProxy = new MeLightFloatProxy<Moo::OmniLight>(
		light_,
		&(Moo::OmniLight::innerRadius),
		&(Moo::OmniLight::innerRadius),
		"full strength radius",
		10.f);
	pProp = new GenRadiusProperty(LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/RADIUS" ), minRadiusProxy, matrixProxy_, 0xbf10ff10, 2.f );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/OMNI_RADIUS" ) );
	editor_->addProperty( pProp );

	//Outer Radius
	FloatProxyPtr maxRadiusProxy = new MeLightFloatProxy<Moo::OmniLight>(
		light_,
		&(Moo::OmniLight::outerRadius),
		&(Moo::OmniLight::outerRadius),
		"fall-off radius",
		20.f);
	pProp = new GenRadiusProperty(LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/FALLOFF" ), maxRadiusProxy, matrixProxy_, 0xbf1010ff, 4.f);
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/OMNI_FALLOFF" ) );
	editor_->addProperty( pProp );

	//Multiplier
	FloatProxyPtr multiplierProxy = new MeLightFloatProxy<Moo::OmniLight>(
		light_,
		&(Moo::OmniLight::multiplier),
		&(Moo::OmniLight::multiplier),
		"multiplier",
		1.f);
	pProp = new GenFloatProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/MULTIPLIER" ), multiplierProxy );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/OMNI_INTENSITY" ) );
	editor_->addProperty( pProp );

	light_->worldTransform( Matrix::identity );
}

DirLight::DirLight()
{
	BW_GUARD;

	GeneralProperty* pProp = NULL;
	
	//Colour
	ColourProxyPtr colourProxy = new MeLightColourWrapper<Moo::DirectionalLight>( light_ );
	pProp = new ColourProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/COLOUR" ), colourProxy );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/DIRECTIONAL_COLOUR" ) );

	editor_->addProperty( pProp );

	//Rotation
	matrixProxy_ = new MeLightDirMatrixProxy<Moo::DirectionalLight>(
		light_,
		&matrix_ );
	pProp = new GenRotationProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/DIRECTION" ), matrixProxy_);
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/DIRECTIONAL_DIRECTION" ) );
	editor_->addProperty( pProp ); 

	//Multiplier
	FloatProxyPtr multiplierProxy = new MeLightFloatProxy<Moo::DirectionalLight>(
		light_,
		&(Moo::DirectionalLight::multiplier),
		&(Moo::DirectionalLight::multiplier),
		"multiplier",
		1.f);
	pProp = new GenFloatProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/MULTIPLIER" ), multiplierProxy );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/DIRECTIONAL_INTENSITY" ) );
	editor_->addProperty( pProp );

	light_->worldTransform( Matrix::identity );
}

SpotLight::SpotLight()
{
	BW_GUARD;

	GeneralProperty* pProp = NULL;
	
	//Colour
	ColourProxyPtr colourProxy = new MeLightColourWrapper<Moo::SpotLight>( light_ );
	pProp = new ColourProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/COLOUR" ), colourProxy );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/SPOT_COLOUR" ) );
	editor_->addProperty( pProp );
		
	//Position + Rotation
	matrixProxy_ = new MeSpotLightPosDirMatrixProxy<Moo::SpotLight>(
		light_,
		&matrix_ );
	pProp = new GenPositionProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/POSITION" ), matrixProxy_);
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/SPOT_POSITION" ) );
	editor_->addProperty( pProp );
	pProp = new GenRotationProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/DIRECTION" ), matrixProxy_);
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/SPOT_DIRECTION" ) );
	editor_->addProperty( pProp );

	//Set the initial position and direction
	position( Vector3( 0.f, 2.f, 0.f ));
	direction( Vector3( 0.f, -1.f, 0.f ));

	//Inner Radius
	FloatProxyPtr minRadiusProxy = new MeLightFloatProxy<Moo::SpotLight>(
		light_,
		&(Moo::SpotLight::innerRadius),
		&(Moo::SpotLight::innerRadius),
		"full strength radius",
		10.f);
	pProp = new GenRadiusProperty(LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/RADIUS" ), minRadiusProxy, matrixProxy_, 0xbf10ff10, 2.f );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/SPOT_RADIUS" ) );
	editor_->addProperty( pProp );

	//Outer Radius
	FloatProxyPtr maxRadiusProxy = new MeLightFloatProxy<Moo::SpotLight>(
		light_,
		&(Moo::SpotLight::outerRadius),
		&(Moo::SpotLight::outerRadius),
		"fall-off radius",
		20.f);
	pProp = new GenRadiusProperty(LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/FALLOFF" ), maxRadiusProxy, matrixProxy_, 0xbf1010ff, 4.f);
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/SPOT_FALLOFF" ) );
	editor_->addProperty( pProp );

	//Cone Angle
	FloatProxyPtr coneAngleProxy = new MeLightConeAngleProxy( light_, 30.f );
	pProp = new AngleProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/CONE_ANGLE" ), coneAngleProxy, matrixProxy_ );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/SPOT_CONE_ANGLE" ) );
	editor_->addProperty( pProp );

	//Multiplier
	FloatProxyPtr multiplierProxy = new MeLightFloatProxy<Moo::SpotLight>(
		light_,
		&(Moo::SpotLight::multiplier),
		&(Moo::SpotLight::multiplier),
		"multiplier",
		1.f);
	pProp = new GenFloatProperty( LocaliseUTF8( "MODELEDITOR/MODELS/LIGHTS/MULTIPLIER" ), multiplierProxy );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/MODELS/LIGHTS/SPOT_INTENSITY" ) );
	editor_->addProperty( pProp );

	light_->worldTransform( Matrix::identity );
}

Lights::Lights()
{
	BW_GUARD;

	s_instance_ = this;

	ambient_ = new EditorAmbientLight();
	
	//TODO: These values need to come from Options.xml
	for (int i=0; i<4; i++)
	{
		omni_.push_back( new OmniLight );
		chunkOmni_.push_back( new ChunkOmniLight );
	}

	for (int i=0; i<2; i++)
	{
		dir_.push_back( new DirLight );
		chunkDir_.push_back( new ChunkDirectionalLight );
	}

	for (int i=0; i<2; i++)
	{
		spot_.push_back( new SpotLight );
		chunkSpot_.push_back( new ChunkSpotLight );
	}

	regenerateLightContainer();

	currFile_ = NULL;

	dirty_ = false;
}

Lights::~Lights()
{
	BW_GUARD;

	//Moo::rc().lightContainer( NULL );
	//Moo::rc().specularLightContainer( NULL );

	delete ambient_;

	for (int i=0; i<numOmni(); i++)
	{
		delete omni_[i];
		delete chunkOmni_[i];
	}

	for (int i=0; i<numDir(); i++)
	{
		delete dir_[i];
		delete chunkDir_[i];
	}

	for (int i=0; i<numSpot(); i++)
	{
		delete spot_[i];
		delete chunkSpot_[i];
	}
}

void Lights::regenerateLightContainer()
{
	BW_GUARD;

	lc_ = new Moo::LightContainer;
	
	if (ambient()->enabled())
		lc_->ambientColour( ambient()->light()->light() );

	for (int i=0; i<numOmni(); i++)
		if (omni(i)->enabled())
			lc_->addOmni( omni(i)->light() );

	for (int i=0; i<numDir(); i++)
		if (dir(i)->enabled())
			lc_->addDirectional( dir(i)->light() );

	for (int i=0; i<numSpot(); i++)
		if (spot(i)->enabled())
			lc_->addSpot( spot(i)->light() );
}

void Lights::disableAllLights()
{
	BW_GUARD;

	ambient()->enabled( false );
	for (int i=0; i<numOmni(); i++)
	{
		omni(i)->enabled( false );
	}
	for (int i=0; i<numDir(); i++)
	{
		dir(i)->enabled( false );
	}
	for (int i=0; i<numSpot(); i++)
	{
		spot(i)->enabled( false );
	}
}

bool Lights::newSetup()
{
	BW_GUARD;

	if (dirty_)
	{
		int result = ::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/APP/ME_APP/LIGHTS_CHANGED_Q"),
			Localise(L"MODELEDITOR/APP/ME_APP/LIGHTS_CHANGED"), MB_YESNOCANCEL | MB_ICONWARNING );
		if( result == IDCANCEL )
		{
			return false;
		}
		if( result == IDYES )
		{
			if (!save())
			{
				return false;
			}
		}
		if( result == IDNO )
		{
			ME_WARNING_MSGW( Localise(L"MODELEDITOR/APP/ME_APP/LIGHTS_NOT_SAVED") );
		}
	}

	disableAllLights();
	currName_ = "";
	currFile_ = NULL;

	dirty_ = false;

	regenerateLightContainer();

	return true;
}

bool Lights::open( const BW::string& name, DataSectionPtr file /* = NULL */ )
{
	BW_GUARD;

	if (file == NULL)
	{
		file = BWResource::openSection( name );
		if (file == NULL) return false;
	}
		
	int omniCount = 0;
	int dirCount = 0;
	int spotCount = 0;

	if (!newSetup()) return false;

	BW::vector< DataSectionPtr > pLights;
	file->openSections( "Lights/Light", pLights );
	BW::vector< DataSectionPtr >::iterator lightsIt = pLights.begin();
	BW::vector< DataSectionPtr >::iterator lightsEnd = pLights.end();
	while (lightsIt != lightsEnd)
	{
		DataSectionPtr pLight = *lightsIt++;
		BW::string lightType = pLight->readString( "Type", "" );

		bool enabled = pLight->readBool("Enabled", false);
		Vector3 empty ( 0.f, 0.f, 0.f );
		Vector3 colourVector = pLight->readVector3( "Color", empty ) / 255.f;
		Moo::Colour colour(
			colourVector[2],
			colourVector[1],
			colourVector[0],
			1.f);
		Vector3 position = pLight->readVector3( "Location", empty );
		Vector3 direction = pLight->readVector3( "Orientation", empty );
		float innerRadius = pLight->readFloat( "Full_Radius", 0.f );
		float outerRadius = pLight->readFloat( "Falloff_Radius", 0.f );
		float cosConeAngle = (float)(cos( pLight->readFloat( "Cone_Size", 30.f ) * MATH_PI / 180.f ));
			
		if (lightType == "Ambient")
		{
			ambient()->enabled( enabled );
			ambient()->light()->colour( colour );
		}
		else if (lightType == "Omni")
		{
			if (omniCount >= numOmni()) continue;
			omni(omniCount)->enabled( enabled );
			omni(omniCount)->light()->colour( colour );
			omni(omniCount)->position( position );
			omni(omniCount)->light()->innerRadius( innerRadius );
			omni(omniCount)->light()->outerRadius( outerRadius );
			omni(omniCount)->light()->worldTransform( Matrix::identity );
			omniCount++;
		}
		else if (lightType == "Directional")
		{
			if (dirCount >= numDir()) continue;
			dir(dirCount)->enabled( enabled );
			dir(dirCount)->light()->colour( colour );
			dir(dirCount)->direction( direction );
			dir(dirCount)->light()->worldTransform( Matrix::identity );
			dirCount++;
		}
		else if (lightType == "Spot")
		{
			if (spotCount >= numSpot()) continue;
			spot(spotCount)->enabled( enabled );
			spot(spotCount)->light()->colour( colour );
			spot(spotCount)->position( position );
			spot(spotCount)->direction( direction );
			spot(spotCount)->light()->innerRadius( innerRadius );
			spot(spotCount)->light()->outerRadius( outerRadius );
			spot(spotCount)->light()->cosConeAngle( cosConeAngle );
			spot(spotCount)->light()->worldTransform( Matrix::identity );
			spotCount++;
		}
	}

	currName_ = name;
	currFile_ = file;

	dirty_ = false;

	regenerateLightContainer();

	return true;
}

void Lights::unsetLights()
{
	BW_GUARD;

	//remove old chunk lights
	for (int i=0; i<4; i++)
		chunkOmni_[i]->toss( NULL );
	for (int i=0; i<2; i++)
		chunkDir_[i]->toss( NULL );
	for (int i=0; i<2; i++)
		chunkSpot_[i]->toss( NULL );
}

void Lights::setLights()
{
	BW_GUARD;

	this->unsetLights();		

	//Moo::rc().lightContainer( this->lightContainer() );
	//Moo::rc().specularLightContainer( this->lightContainer() );

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if (!pSpace)
		return;

	//4 omnis
	for (int i=0; i<4; i++)
	{
		OmniLight* omni = omni_[i];
		if ( omni->enabled() )
		{
			Vector3 pos( omni->transform().applyToOrigin() );
			Chunk* pChunk = pSpace->findChunkFromPoint( pos );
			if (pChunk)
			{
				ChunkOmniLight* l = chunkOmni_[i];
				l->edTransform( omni->transform() );
				l->pLight() = omni->light();
				l->pLight()->position( pChunk->transformInverse().applyPoint(pos) );
				l->toss(pChunk);
			}
		}
	}

	//2 spots
	for (int i=0; i<2; i++)
	{
		SpotLight* spot = spot_[i];
		if ( spot->enabled() )
		{
			Vector3 pos( spot->transform().applyToOrigin() );
			Chunk* pChunk = pSpace->findChunkFromPoint( pos );
			if (pChunk)
			{
				ChunkSpotLight* l = chunkSpot_[i];
				l->edTransform( spot->transform() );
				l->pLight() = spot->light();
				l->pLight()->position( pChunk->transformInverse().applyPoint(pos) );
				l->toss(pChunk);
			}
		}
	}

	//2 directionals
	//TODO : Work out what to do with Chunk Directional Lights, they don't
	//seem to make much sense (as their influence is global but this conflicts
	//with the philosophy of Chunking)
	/*for (int i=0; i<2; i++)
	{
		DirLight* dir = dir_[i];
		if ( dir->enabled() )
		{
			Vector3 pos( dir->transform().applyToOrigin() );
			Chunk* pChunk = pSpace->findChunkFromPoint( pos );
			if (pChunk)
			{
				ChunkDirectionalLight* l = chunkDir_[i];
				l->edTransform( dir->transform() );
				l->pLight() = dir->light();				
				l->toss(pChunk);
			}
		}
	}*/
}

bool Lights::save( const BW::string& name /* = "" */ )
{
	BW_GUARD;

	DataSectionPtr file;
	
	if (name == "")
	{
		if (currFile_ == NULL)
		{
			static wchar_t szFilter[] =	L"Lighting Model (*.mvl)|*.mvl||";
			BWFileDialog fileDlg (
				false, L"", L"", BWFileDialog::FD_OVERWRITEPROMPT, szFilter);

			BW::string lightsDir;
			MRU::instance().getDir("lights", lightsDir, s_default_lights );
			BW::wstring wlightsDir = bw_utf8tow( lightsDir );
			fileDlg.initialDir( wlightsDir.c_str() );

			if (fileDlg.showDialog())
			{
				currName_ = BWResource::dissolveFilename( bw_wtoutf8( fileDlg.getFileName() ));
				file = BWResource::openSection( currName_, true );
				MRU::instance().update( "lights", currName_, true );
			}
			else
			{
				return false;
			}
		}
		else
		{
			file = currFile_;
		}
	}
	else
	{
		currName_ = name;
		file = BWResource::openSection( name, true );
	}

	file->delChildren(); // Clear any old lights out

	DataSectionPtr pLights = file->newSection( "Lights" );

	{
		EditorAmbientLight* light = ambient();
		Moo::Colour col = light->light()->colour();
		
		DataSectionPtr pLight = pLights->newSection("Light");
		pLight->writeString( "Type", "Ambient" );
		pLight->writeBool("Enabled", light->enabled());
		pLight->writeVector3( "Color", Vector3( col.b, col.g, col.r ) * 255.f);
	}

	for (int i=0; i<numOmni(); i++)
	{
		OmniLight* light = omni(i);
		Moo::Colour col = light->light()->colour();

		DataSectionPtr pLight = pLights->newSection("Light");
		pLight->writeString( "Type", "Omni" );
		pLight->writeBool("Enabled", light->enabled());
		pLight->writeVector3( "Color", Vector3( col.b, col.g, col.r ) * 255.f);
		pLight->writeVector3( "Location", light->light()->worldPosition() );
		pLight->writeFloat( "Full_Radius", light->light()->innerRadius() );
		pLight->writeFloat( "Falloff_Radius", light->light()->outerRadius() );
	}

	for (int i=0; i<numDir(); i++)
	{
		DirLight* light = dir(i);
		Moo::Colour col = light->light()->colour();

		DataSectionPtr pLight = pLights->newSection("Light");
		pLight->writeString( "Type", "Directional" );
		pLight->writeBool("Enabled", light->enabled());
		pLight->writeVector3( "Color", Vector3( col.b, col.g, col.r ) * 255.f);
		pLight->writeVector3( "Orientation", light->light()->direction() );
	}

	for (int i=0; i<numSpot(); i++)
	{
		SpotLight* light = spot(i);
		Moo::Colour col = light->light()->colour();

		DataSectionPtr pLight = pLights->newSection("Light");
		pLight->writeString( "Type", "Spot" );
		pLight->writeBool("Enabled", light->enabled());
		pLight->writeVector3( "Color", Vector3( col.b, col.g, col.r ) * 255.f);
		pLight->writeVector3( "Location", light->light()->worldPosition() );
		pLight->writeVector3( "Orientation", light->light()->direction() );
		pLight->writeFloat( "Full_Radius", light->light()->innerRadius() );
		pLight->writeFloat( "Falloff_Radius", light->light()->outerRadius() );
		pLight->writeFloat( "Cone_Size", (float)(180.f * acos(light->light()->cosConeAngle()) / MATH_PI) );
	}

	file->save();

	ME_INFO_MSGW( Localise(L"MODELEDITOR/MODELS/LIGHTS/SAVING", currName_ ) );

	currFile_ = file;
	
	dirty_ = false;

	return true;
}
BW_END_NAMESPACE

