#include "pch.hpp"

#include "girth.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"


BW_BEGIN_NAMESPACE

static AutoConfigString s_girthSettings( "editor/girthSettings" );


Girth::Girth( DataSectionPtr ds )
{
	girth_ = ds->asFloat();
	width_ = ds->readFloat( "width" );
	height_ = ds->readFloat( "height" );;
	depth_ = ds->readFloat( "depth" );
	maxSlope_ = ds->readFloat( "maxSlope", 45.f );
	maxClimb_ = ds->readFloat( "maxClimb", height_ / 3.0f );
	always_ = !!ds->openSection( "always" );

	if (width_ <= 0.f || height_ <= 0.f || depth_ < 0.f)
	{
		CRITICAL_MSG( "The girth settings (width: %g, height: %g, depth_: %g) "
			"is incorrect.", width_, height_, depth_ );
	}
}


Girths::Girths()
{
	BW::vector<DataSectionPtr> girths;

	DataSectionPtr ds = BWResource::openSection( s_girthSettings );

	if (!ds)
	{
		CRITICAL_MSG( "The girth settings were saved in navgen_settings.xml "
			"inside bigworld\\tools\\misc. Now they use a file specified by "
			"editor/girthSettings in resources.xml which defaults to "
			"helpers/girths.xml" );
	}
	else
	{
		ds->openSections( "girth", girths );

		for (BW::vector<DataSectionPtr>::const_iterator iter = girths.begin();
			iter != girths.end(); ++iter)
		{
			girths_.push_back( Girth( *iter ) );
		}
	}
}


const Girth& Girths::operator[]( size_t index ) const
{
	return girths_.at( index );
}

BW_END_NAMESPACE
