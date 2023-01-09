#ifdef _MSC_VER 
#pragma once
#endif

#ifndef SUPER_MODEL_ACTION_HPP
#define SUPER_MODEL_ACTION_HPP


#include "fashion.hpp"

#include "moo/reload.hpp"


BW_BEGIN_NAMESPACE

class ModelAction;
class ModelAnimation;
class SuperModel;


/**
 *	This class represents an action from a SuperModel perspective.
 */
class SuperModelAction :
	public TransformFashion,
	/*
	*	Note: SuperModelAction is always listening if it's superModel_ has 
	*	been reloaded, if you are pulling info out from it
	*	then please update it again in onReloaderReloaded which is 
	*	called when the superModel_ is reloaded so that related data
	*	will need be update again.and you might want to do something
	*	in onReloaderPriorReloaded which happen right before the superModel_ reloaded.
	*/
	public ReloadListener
{
public:
	float	passed_;		///< Real time passed since action started
	float	played_;		///< Effective time action has played at full speed
	float	finish_;		///< Real time action should finish (-ve => none)
	float	lastPlayed_;	///< Value of played when last ticked
	const ModelAction * pSource_;	// source action (first if multiple)
	const ModelAnimation * pFirstAnim_;	// top model first anim

	float blendRatio() const;

	void tick( SuperModel & superModel, float dtime );
	virtual void onReloaderReloaded( Reloader* pReloader );

	/**
	 *	SuperModelAction does not require models to re-calculate their
	 *	transform state after this fashion is applied.
	 *	@return false
	 */
	bool needsRedress() const { return false; }

private:
	SuperModel* pSuperModel_;
	BW::string actionName_;
	int		index[1];	// same deal as SuperModelAnimation

	SuperModelAction( SuperModel & superModel, const BW::string & name );
	friend class SuperModel;

	virtual void dress( SuperModel & superModel, Matrix& world );
	void pullInfoFromSuperModel();
};

typedef SmartPointer<SuperModelAction> SuperModelActionPtr;

BW_END_NAMESPACE


#endif // SUPER_MODEL_ACTION_HPP
