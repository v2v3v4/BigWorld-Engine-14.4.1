#include "pch.hpp"
#include "lightning.hpp"


BW_BEGIN_NAMESPACE

static AutoConfigString s_lightningBmpName( "environment/lightningBmpName" );

/**
 *	This function decides whether or not there will be any lightning,
 *	and what form it will take if there will be. If it can draw it,
 *	it does.
 *
 *	@return The source (x,y,z) and remoteness (w) of any thunder.
 *		A remoteness of >= 1 means no thunder.
 */
Vector4 Lightning::decideLightning( float dTime )
{
	Vector4	thunder( 0, 0, 0, 100 );

	// if we were doing lightning before, we're not any more
	if (avgDensity_ == -1) avgDensity_ = 0.f;

	// when conflict is full on, we have a 70% chance of
	// lighting/thunder every second.
	if (rand() * conflict_ < RAND_MAX * 0.7 * dTime ) return thunder;

	// find a dark cloud
	BW::vector<CloudSpec*> possibles;
	for (uint c = 0; c < cloudStrata[0].clouds.size(); c++)
	{
		if (cloudStrata[0].clouds[c].midLum < 128+192/4)
		{
			possibles.push_back( &cloudStrata[0].clouds[c] );
		}
	}

	if (!possibles.size()) return thunder;

	CloudSpec & cs = *possibles[rand() % possibles.size()];

	// choose the type of lighting - sheet/intracloud/forked, and do it
	bool shouldFlashAmbient = false;
	float litype = float(rand() & 0xF);
	if (litype >= 13)	// sheet lightning / forked lightning
	{
		if (litype == 15)
		{
			// create an omni light at the centre of it
			// set something to draw it later... intersect with collision
			// scene if it exists :)

			float straySz = ((cs.radii.x + cs.radii.y)/2) * 0.5f;
			Vector2 topPos(
				cs.position.x + rand()*straySz/RAND_MAX,
				cs.position.y + rand()*straySz/RAND_MAX );

			if (topPos.length() < 500)	// TODO: should be set to 80% of farplane
				shouldFlashAmbient = true;

			Vector3 flashpoint(
				topPos[0],
				(1-topPos.length()/fullRange)*cloudStrata[0].height,
				topPos[1] );

			//dprintf( "Doing a lightning strike...\n" );
			this->lightningStrike( flashpoint );
			//dprintf( "Done.\n" );

			// just make this cloud light
			cs.lightning = 1;

			// and we'll have some thunder too, thanks
			thunder = Vector4(
				flashpoint + Moo::rc().invView().applyToOrigin(),
				topPos.length() / fullRange );
		}
		else
		{
			// make all the clouds light...
			for (uint c = 0; c < cloudStrata[0].clouds.size(); c++)
				if (cloudStrata[0].clouds[c].midLum < 128+192/4)
					cloudStrata[0].clouds[c].lightning = 1;
		}

		// flash the whole scene too if we have sufficient cover
		if (avgCover_ > 0.7 && litype == 15 && shouldFlashAmbient)
		{
			// basically want to set sun colour to fluorescent white
			// .. and something like that to the sky gradient
			avgDensity_ = -1.f;
		}
	}
	else					// intracloud lightning
	{
		cs.lightning = 1;
	}

	return thunder;
}


/**
 * TODO: to be documented.
 */
struct LightningFork
{
	Vector3	pos;
	Vector3	dir;
	float			width;
};

/**
 *	Create a lightning strike from the given point down
 */
void Sky::lightningStrike( const Vector3 & top )
{
	static Moo::Material	mat;

	if (mat.fogged())
	{
		Moo::TextureStage	ts1, nots;

		ts1.pTexture( Moo::TextureManager::instance()->get(s_lightningBmpName) );
			//"maps/system/Col_white.bmp" ) );
		ts1.colourOperation( Moo::TextureStage::MODULATE,
			Moo::TextureStage::CURRENT,
			Moo::TextureStage::TEXTURE );
		ts1.alphaOperation( Moo::TextureStage::MODULATE,
			Moo::TextureStage::CURRENT,
			Moo::TextureStage::TEXTURE );
		mat.addTextureStage( ts1 );

		mat.addTextureStage( nots );
		mat.fogged( false );

		mat.alphaBlended( true );
		mat.srcBlend( Moo::Material::ONE );
		mat.destBlend( Moo::Material::ONE );
	}

	Matrix		vprg = Moo::rc().view();
	vprg.translation( Vector3(0,0,0) );
	vprg.postMultiply( Moo::rc().projection() );

	Vector3 zrot = Moo::rc().view().applyToUnitAxisVector(2);
	zrot[1] = 0.f;
	zrot.normalise();

	// define and seed a stack of lightning forks
	static VectorNoDestructor<LightningFork>	stack;
	stack.clear();

	LightningFork seed;
	seed.pos = top;
	seed.dir = Vector3( 0, -1, 0 );
	seed.width = 10.f + rand() * 6.f / RAND_MAX;
	stack.push_back( seed );

	float	forkWidths[16];

	int forkTotal = 0;

	// fork until the stack is empty.
	//  termination guarantee is that width always decreases
	while (!stack.empty())
	{
		forkTotal++;
		if (forkTotal > 1024) break;

		LightningFork lf = stack.back();
		stack.pop_back();

		// figure out how many forks we're going to make.
		//  this depends on width. for large width, only 1 or two, with
		//  a strong bias to one being much bigger than the other
		//  for medium width, anywhere from 1 to 3
		//  for small width, 0 or 1
		int nforks = 0;
		if (lf.width > 8.f)
		{
			nforks = int(1.f + rand() * 1.9f / RAND_MAX);
		}
		else if (lf.width > 2.f)
		{
			nforks = int(1.f + rand() * 2.9f / RAND_MAX);
		}
		else
		{
			nforks = int(rand() * 1.9f / RAND_MAX);
		}

		// make up some unscaled width elements
		float	sumFWs = 0.f;
		for (int f = 0; f < nforks; f++)
		{
			float newFW = float(rand()) / RAND_MAX;
			forkWidths[f] = newFW;
			sumFWs += newFW;
		}

		// make sure width adds up to a little less than it is now
		if (lf.width > 1.f)
		{
			sumFWs *= lf.width/(lf.width-0.2f);
		}

		// invent and draw each fork
		for (int f = 0; f < nforks; f++)
		{
			// figure a direction
			Vector3 newdir(
				lf.dir[0] + (rand() * 0.5f / RAND_MAX) - 0.25f,
				lf.dir[1] + (rand() * 0.5f / RAND_MAX) - 0.25f,
				lf.dir[2] + (rand() * 0.5f / RAND_MAX) - 0.25f );
			newdir.normalise();

			// and a length (from 6 to 10m per segment)
			float newlen = 6.f + rand() * 4.f / RAND_MAX;

			// make a new record
			LightningFork nlf;
			nlf.pos = lf.pos + newdir * newlen;
			nlf.dir = newdir;
			nlf.width = forkWidths[f] * lf.width / sumFWs;
			if (f==0 && lf.width > 8.f)
			{
				nlf.width = lf.width;
				if (nlf.dir[1] > -0.5) nlf.dir = Vector3(0,-1,0);
			}

			// draw it from lf.pos to nlf.pos
			Moo::VertexTDSUV2	v[4];
			for (int i = 0; i < 4; i++)
			{
				vprg.applyPoint( v[i].pos_, Vector4(
					((i&2)?nlf.pos[0]:lf.pos[0]) +
						((i&1)?0.5f:-0.5f) * ((i&2)?nlf.width:lf.width) * zrot[2],
					((i&2)?nlf.pos[1]:lf.pos[1]),
					((i&2)?nlf.pos[2]:lf.pos[2]) +
						((i&1)?0.5f:-0.5f) * ((i&2)?nlf.width:lf.width) * zrot[0],
					1 ) );

				float maxz = v[i].pos_.w * 0.999f;
				v[i].pos_.z = min( maxz, v[i].pos_.z );

				v[i].colour_ = 0xffffffff;
				v[i].specular_ = 0xffffffff;

				v[i].uv_.x = (i&2)?1.f:0.f;
				v[i].uv_.y = (i&1)?1.f:0.f;
			}

			//Todo: find a better solution for this.
//			Moo::rc().addSortedTriangle( SortedTriangle( &v[0], &v[1], &v[2], &mat ) );
//			Moo::rc().addSortedTriangle( SortedTriangle( &v[3], &v[2], &v[1], &mat ) );

			// and add it to the stack if it's worthy
			if (nlf.width >= 1.f && nlf.pos[1] > -100.f) stack.push_back(nlf);
			// TODO: don't add it to the stack if it's under the terrain...
			//  or if the line to it intersects the scene...
			// (actually, kinda cool to leave it intersecting the scene :)
		}
	}
}

BW_END_NAMESPACE

// lightning.cpp
