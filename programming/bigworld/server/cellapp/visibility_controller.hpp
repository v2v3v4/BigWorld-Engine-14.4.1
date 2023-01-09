#ifndef VISIBILITY_CONTROLLER_HPP
#define VISIBILITY_CONTROLLER_HPP

#include "controller.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class controls the visibility of the entity it is attached to.
 */
class VisibilityController : public Controller
{
	DECLARE_CONTROLLER_TYPE( VisibilityController )
public:
	VisibilityController( float visibleHeight = 2.f );
	~VisibilityController();

	virtual void writeGhostToStream( BinaryOStream & stream );
	virtual bool readGhostFromStream( BinaryIStream & stream );

	//virtual void writeRealToStream( BinaryOStream& stream );
	//virtual bool readRealFromStream( BinaryIStream& stream );

	virtual void startGhost();
	virtual void stopGhost();

	static FactoryFnRet New( float visibleHeight, int userArg = 0 );
	PY_AUTO_CONTROLLER_FACTORY_DECLARE( VisibilityController,
		ARG( float, OPTARG( int, 0, END ) ) )


	float visibleHeight() const		{ return visibleHeight_; }
	void visibleHeight( float h );

private:
	float	visibleHeight_;
};

BW_END_NAMESPACE

#endif // VISIBILITY_CONTROLLER_HPP
