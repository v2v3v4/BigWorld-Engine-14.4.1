#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include "gizmo/general_properties.hpp"
#include "chunk/chunk_light.hpp"

BW_BEGIN_NAMESPACE

typedef SmartPointer< class GeneralEditor > GeneralEditorPtr;

class GeneralLight
{
public:
	void elect();
	void expel();
	void enabled( bool v );
	bool enabled();

	virtual void setMatrix( const Matrix& matrix ) {}

protected:
	bool enabled_;
	GeneralEditorPtr editor_;
};

class AmbientLight: public ReferenceCount
{
public:
	AmbientLight();
	Moo::Colour colour();
	void colour( Moo::Colour v );
	Moo::Colour light();
private:
	Moo::Colour light_;
};

class EditorAmbientLight : public GeneralLight	
{
public:
	EditorAmbientLight();
	SmartPointer<AmbientLight> light() { return light_; }
private:
	SmartPointer<AmbientLight> light_;
};

template <class CL> class EditorLight: public GeneralLight
{
public:

	EditorLight()
	{
		enabled_ = false;
		light_ = new CL();
		light_->colour( Moo::Colour( 1.f, 1.f, 1.f, 1.f ));
		editor_ = GeneralEditorPtr(new GeneralEditor(), true);
	}

	SmartPointer<CL> light() { return light_; }
	
	void position( Vector3& pos )
	{
		matrix_[3] = pos;
		matrixProxy_->setMatrix( matrix_ );
	}

	const Matrix& transform() const
	{
		return matrix_;
	}	

	virtual void direction( Vector3& dir )
	{
		dir.normalise();

		Vector3 up( 0.f, 1.f, 0.f );
		if (fabsf( up.dotProduct( dir ) ) > 0.9f)
			up = Vector3( 0.f, 0.f, 1.f );

		Vector3 xaxis = up.crossProduct( dir );
		xaxis.normalise();

		matrix_[2] = dir;
		matrix_[1] = dir.crossProduct( xaxis );
		matrix_[1].normalise();
		matrix_[0] = xaxis;

		matrixProxy_->setMatrix( matrix_ );
	}

	virtual void setMatrix( const Matrix& matrix )
	{
		matrix_ = matrix;
		matrixProxy_->setMatrix( matrix_ );
	}

protected:

	SmartPointer<CL> light_;
	MatrixProxyPtr matrixProxy_;
	Matrix matrix_;
};

class OmniLight: public EditorLight<Moo::OmniLight>
{
public:
	OmniLight();

	void setMatrix( const Matrix& matrix )
	{
		Matrix temp = matrix;
		position( temp[3] );
	}
};

class DirLight: public EditorLight<Moo::DirectionalLight>
{
public:
	DirLight();

	void setMatrix( const Matrix& matrix )
	{
		Matrix temp = matrix;
		direction( -temp[2] );
	}
};

class SpotLight: public EditorLight<Moo::SpotLight>
{
public:
	SpotLight();

	virtual void direction( Vector3& dir )
	{
		dir.normalise();

		Vector3 up( 0.f, 1.f, 0.f );
		if (fabsf( up.dotProduct( dir ) ) > 0.9f)
			up = Vector3( 0.f, 0.f, 1.f );

		Vector3 xaxis = up.crossProduct( dir );
		xaxis.normalise();

		matrix_[2] = dir;
		matrix_[1] = dir.crossProduct( xaxis );
		matrix_[1].normalise();
		matrix_[0] = xaxis;

		matrixProxy_->setMatrix( matrix_ );
	}

	void setMatrix( const Matrix& matrix )
	{
		matrix_ = matrix;
		Matrix temp = matrix;
		direction( -temp[2] );
	}
};

class Lights
{
public:

	static Lights* s_instance_;
	static Lights& instance()
	{
		return *s_instance_;
	}
	
	Lights();
	~Lights();
	
	bool newSetup();
	bool open( const BW::string& name, DataSectionPtr file = NULL );
	bool save( const BW::string& name = "" );

	void unsetLights();
	void setLights();
	
	void regenerateLightContainer();
	
	int numOmni()
	{
		MF_ASSERT( omni_.size() <= INT_MAX );
		return ( int ) omni_.size();
	}

	int numDir()
	{
		MF_ASSERT( dir_.size() <= INT_MAX );
		return ( int ) dir_.size();
	}

	int numSpot()
	{ 
		MF_ASSERT( spot_.size() <= INT_MAX );
		return ( int ) spot_.size();
	}
	
	EditorAmbientLight*	ambient() { return ambient_; }
	OmniLight*		omni( int i ) { return omni_[i]; }
	DirLight*		dir( int i ) { return dir_[i]; }
	SpotLight*		spot( int i ) { return spot_[i]; }

	Moo::LightContainerPtr lightContainer() { return lc_; }

	void dirty( bool val ) { dirty_ = val; }
	bool dirty() { return dirty_; }

	BW::string& name() { return currName_; }

private:

	EditorAmbientLight* ambient_;
	BW::vector<OmniLight*> omni_;
	BW::vector<ChunkOmniLight*> chunkOmni_;
	BW::vector<DirLight*> dir_;
	BW::vector<ChunkDirectionalLight*> chunkDir_;
	BW::vector<SpotLight*> spot_;
	BW::vector<ChunkSpotLight*> chunkSpot_;

	Moo::LightContainerPtr lc_;

	BW::string currName_;
	DataSectionPtr currFile_;
		
	bool dirty_;

	void disableAllLights();
	
};

BW_END_NAMESPACE

#endif // LIGHTS_HPP
