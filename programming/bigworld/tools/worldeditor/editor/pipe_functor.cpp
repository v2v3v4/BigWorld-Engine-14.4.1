#include "pch.hpp"
#include "pipe_functor.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PipeFunctor )

PY_BEGIN_METHODS( PipeFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PipeFunctor )
PY_END_ATTRIBUTES()

PY_FACTORY( PipeFunctor, Functor )

PipeFunctor::PipeFunctor( ToolFunctor* f1,
	ToolFunctor* f2,
	PyTypeObject * pType ) :

	ToolFunctor( true, pType ),
	f1_(f1),
	f2_(f2)

{
}

void PipeFunctor::update( float dTime, Tool& tool )
{
	BW_GUARD;
	f1_->update( dTime, tool );
	f2_->update( dTime, tool );
}


void PipeFunctor::stopApplying( Tool & tool, bool saveChanges )
{
	BW_GUARD;
	f1_->stopApplying( tool, saveChanges );
	f2_->stopApplying( tool, saveChanges );
}


void PipeFunctor::onBeginUsing( Tool & tool )
{
	BW_GUARD;
	f1_->onBeginUsing( tool );
	f2_->onBeginUsing( tool );
}


void PipeFunctor::onEndUsing( Tool & tool )
{
	BW_GUARD;
	f2_->onEndUsing( tool );
	f1_->onEndUsing( tool );
}


bool PipeFunctor::isAllowedToDiscardChanges() const
{
	BW_GUARD;
	return f1_->isAllowedToDiscardChanges() && f2_->isAllowedToDiscardChanges();
}

bool PipeFunctor::handleKeyEvent( const KeyEvent & event, Tool& tool )
{
	BW_GUARD;

	bool handled = f1_->handleKeyEvent( event, tool );
	if (!handled)
	{
		handled = f2_->handleKeyEvent( event, tool );
	}
	return handled;
}


bool  PipeFunctor::handleMouseEvent( const MouseEvent & event, Tool& tool )
{
	BW_GUARD;

	bool handled = f1_->handleMouseEvent( event, tool );
	if (!handled)
	{
		handled = f2_->handleMouseEvent( event, tool );
	}
	return handled;
}


bool PipeFunctor::applying() const
{
	BW_GUARD;

	return f1_->applying() || f2_->applying();
}


PyObject * PipeFunctor::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject* f1;
	PyObject* f2;
	
	if (!PyArg_ParseTuple( args, "OO", &f1, &f2 ) ||
		!ToolFunctor::Check( f1 ) ||
		!ToolFunctor::Check( f2 ))
	{
		PyErr_SetString( PyExc_TypeError, "PipeFunctor() "
			"expects two ToolFunctors" );
		return NULL;
	}

	return new PipeFunctor( 
		static_cast<ToolFunctor*>( f1 ),
		static_cast<ToolFunctor*>( f2 )
		);
}
BW_END_NAMESPACE

