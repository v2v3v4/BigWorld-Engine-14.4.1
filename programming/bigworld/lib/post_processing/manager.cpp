#include "pch.hpp"
#include "manager.hpp"
#include "debug.hpp"
#include "moo/render_context.hpp"
#include "moo/mrt_support.hpp"
#include "pyscript/py_data_section.hpp"

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( PostProcessing::Manager )

#ifndef CODE_INLINE
#include "manager.ipp"
#endif


/*~ module PostProcessing
 *	@components{ client, tools }
 *
 *	The PostProcessing module provides access to the Post Processing framework
 *	in the BigWorld client and tools.
 */
namespace PostProcessing
{
	/**
	 *	Includes all the standard PostProcessing
	 *	python object tokens, to make sure they
	 *	are linked in to the executable.
	 *
	 *	Make sure you use PostProcessing::token
	 *	somewhere in your app to get all these.
	 */
	extern int PyPhase_token;
	extern int PyFilterQuad_token;
	extern int PyTransferQuad_token;
	extern int PyCopyBackBuffer_token;
	extern int PyVisualTransferMesh_token;
	extern int PyPointSpriteTransferMesh_token;

	int tokenSet = PyPhase_token | PyFilterQuad_token |
	PyCopyBackBuffer_token | PyVisualTransferMesh_token |
	PyPointSpriteTransferMesh_token | PyTransferQuad_token;

	/**
	 *	This method ticks all of the effects.
	 *	@param	dTime	delta frame time in seconds.
	 */
	void Manager::tick( float dTime )
	{
		BW_GUARD_PROFILER( PostProcessing_Tick );
		Effects::iterator it = effects_.begin();
		Effects::iterator end = effects_.end();
		for (; it != end; it++)
		{
			Effect* e = it->getObject();
			e->tick(dTime);
		}
	}


	/**
	 *	This method draws all of the effects.	 
	 */
	void Manager::draw()
	{
		BW_GUARD_PROFILER( PostProcessing_Draw );
		GPU_PROFILER_SCOPE(PostProcessing_Draw );
		Moo::rc().MRTSupport().bind();

		Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
		Moo::rc().setRenderState( 
			D3DRS_COLORWRITEENABLE, 
			D3DCOLORWRITEENABLE_RED | 
			D3DCOLORWRITEENABLE_GREEN | 
			D3DCOLORWRITEENABLE_BLUE | 
			D3DCOLORWRITEENABLE_ALPHA );

		if ( debug_.hasObject() )
		{
			debug_->beginChain( static_cast<uint32>( effects_.size() ) );
		}

		Effects::iterator it = effects_.begin();
		Effects::iterator end = effects_.end();
		for (; it != end; it++)
		{
			Effect* e = it->getObject();
			e->draw( debug_.getObject() );
		}

		Moo::rc().popRenderState();
		Moo::rc().MRTSupport().unbind();
	}


	/**
	 *	This method gets the post-processing chain of effects as a PyObject
	 *	@return	The PyEffects list.
	 */
	PyObject* Manager::getChain() const
	{
		PyObject * pList = PyList_New( effects_.size() );
		for ( size_t i=0; i<effects_.size(); i++)
		{
			Py_IncRef( effects_[i].getObject() );
			PyList_SET_ITEM( pList, i, effects_[i].getObject() );
		}
		return pList;
	}


	/**
	 *	This method sets the entire post-processing chain of effects.
	 *
	 *	@param	Any sequence of PyEffects.
	 *	@return	None.
	 */
	PyObject* Manager::setChain( PyObject * args )
	{
		size_t slen = PySequence_Size(args);
		PyObject * seq = args;

		PyObjectPtr tempSeq;

		if (slen == 1)
		{
			// Check to see if our one and only argument is a sequence.
			tempSeq = PyObjectPtr( PySequence_GetItem( args, 0 ), PyObjectPtr::STEAL_REFERENCE );
			if (PySequence_Check( tempSeq.get() ))
			{
				seq = tempSeq.get();
				slen = PySequence_Size( seq );
			}
		}

		int st = (int)(effects_.size()) - 1;
		size_t nEffects = 0;

		effects_.clear();

		for (size_t i = 0; i < slen; i++)
		{
			PyObject * pItem = PySequence_GetItem(seq, i);
			if (pItem != Py_None)
			{
				if (Effect::Check( pItem ))
				{
					effects_.push_back( static_cast<Effect*>(pItem) );
					nEffects++;
				}
			}
			else
			{
				nEffects++;
			}
			Py_DECREF( pItem );
		}

		if (slen > nEffects)
		{
			PyErr_Format( PyExc_TypeError,
				"PostProcessing.chain expects a sequence of Effects.  %d items"
				" in the sequence did not match this type", slen-effects_.size() );
		}

		Py_RETURN_NONE;
	}


	/*~ function PostProcessing.chain
	 *	@components{ client, tools }
	 *
	 *	This function sets or gets the entire post-processing chain of effects.
	 *
	 *	@param	Any sequence of PyEffects, or None.
	 *	@return	The global PyEffects list, if no arguments were passed in, or None.
	 */
	PyObject* Manager::py_chain( PyObject * args )
	{
		size_t slen = PySequence_Size(args);

		if ( 0 == slen )
		{
			return Manager::instance().getChain();
		}
		else
		{
			return Manager::instance().setChain( args );
		}
	}

	PY_MODULE_STATIC_METHOD( Manager, chain, _PostProcessing )


	/*~ function PostProcessing.load
	 *	@components{ client, tools }
	 *
	 *	This function load a post-processing chain.  Note
	 *	that the resulting chain is not automatically set as the global chain.
	 *
	 *	@param	DataSection
	 *	@return	PyEffects list.
	 */
	PyObject* Manager::py_load( PyObject * args )
	{
		PyObject * pObj;
		if ( PyArg_ParseTuple( args, "O", &pObj ) && ( PyDataSection::Check( pObj ) ))
		{
			Effects effects;
			PyDataSection * pDS = static_cast<PyDataSection*>(pObj);
			BW::vector<DataSectionPtr> effectSections;
			pDS->pSection()->openSections( "Effect", effectSections );
			BW::vector<DataSectionPtr>::iterator it = effectSections.begin();
			BW::vector<DataSectionPtr>::iterator en = effectSections.end();
			while (it != en)
			{
				DataSectionPtr pSect = *it++;
				EffectPtr e = EffectPtr( new Effect, true );
				if (e->load(pSect))
				{
					effects.push_back(e);
				}
			}

			PyObject * pList = PyList_New( effects.size() );
			for ( size_t i=0; i<effects.size(); i++)
			{
				Py_IncRef( effects[i].getObject() );
				PyList_SET_ITEM( pList, i, effects[i].getObject() );
			}
			return pList;
		}
		Py_RETURN_NONE;
	}

	PY_MODULE_STATIC_METHOD( Manager, load, _PostProcessing )


	/*~ function PostProcessing.save
	 *	@components{ client, tools }
	 *
	 *	This function saves the post-processing chain.
	 *
	 *	@param	DataSection
	 *	@return	None
	 */
	PyObject* Manager::py_save( PyObject * args )
	{
		PyObject * pObj;
		if ( PyArg_ParseTuple( args, "O", &pObj ) && ( PyDataSection::Check( pObj ) ))
		{
			PyDataSection * pDS = static_cast<PyDataSection*>(pObj);
			pDS->pSection()->delChildren();

			Effects::iterator it = Manager::instance().effects_.begin();
			Effects::iterator en = Manager::instance().effects_.end();
			while (it != en)
			{
				EffectPtr pE = *it;
				pE->save(pDS->pSection());
				++it;
			}
			pDS->pSection()->save();
		}
		else
		{
			PyErr_SetString( PyExc_TypeError, "PostProcessing::save() "
					"expects a PyDataSection object." );
				return NULL;
		}
		Py_RETURN_NONE;
	}

	PY_MODULE_STATIC_METHOD( Manager, save, _PostProcessing )


	/*~ function PostProcessing.debug
	 *	@components{ client, tools }
	 *
	 *	This function sets or gets the debug object for the PostProcessing
	 *	chain.
	 *
	 *	@param	A PostProcessing.Debug object, or None.
	 *	@return	A PostProcessing.Debug object, or None.
	 */
	PyObject* Manager::py_debug( PyObject * args )
	{
		size_t slen = PySequence_Size(args);

		if ( 0 == slen )	//This block gets the debug object
		{			
			if ( Manager::instance().debug_.hasObject() )
			{
				Py_IncRef( Manager::instance().debug_.getObject() );
				return Manager::instance().debug_.getObject();
			}
			else
			{
				Py_RETURN_NONE;
			}
		}
		else			//This block sets the debug object
		{
			PyObject * pObj;
			if ( PyArg_ParseTuple( args, "O", &pObj ) &&
				 ( pObj == Py_None || Debug::Check( pObj ) ))
			{
				if ( pObj != Py_None )
				{
					Manager::instance().debug_ = static_cast<Debug*>(pObj);
				}
				else
				{
					Manager::instance().debug_ = NULL;
				}
			}
			else
			{
				PyErr_SetString( PyExc_TypeError, "PostProcessing::debug() "
					"expects a PostProcessing::Debug object, or None" );
				return NULL;
			}
		}

		Py_RETURN_NONE;
	}

	PY_MODULE_STATIC_METHOD( Manager, debug, _PostProcessing )

	/*~ function PostProcessing.profile
	 *	@components{ client, tools }
	 *
	 *	This function profiles the post-processing chain, returning
	 *	the average GPU time incurred.
	 *
	 *	@param	N	(optional)number of samples to take
	 *	@return	float average GPU time, in seconds, taken to process the chain.
	 */
	PyObject* Manager::py_profile( PyObject * args )
	{
		size_t nSamples = 100;
		if (!PyArg_ParseTuple( args, "|i", &nSamples ))
		{
			PyErr_SetString( PyExc_TypeError, "PostProcessing::profile() "
					"expects an optional integer, representing "
					"the number of runs through the entire chain." );
				return NULL;
		}


		uint64 timerFreq64 = 1;
		ComObjectWrap<DX::Query> pFreqQuery;
		HRESULT hr = Moo::rc().device()->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &pFreqQuery);
		if (SUCCEEDED(hr))
		{
			pFreqQuery->Issue(D3DISSUE_BEGIN);
			pFreqQuery->Issue(D3DISSUE_END);
			while(S_FALSE == pFreqQuery->GetData( &timerFreq64, 
				sizeof(DWORD), D3DGETDATA_FLUSH )
				&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);
		}

		ComObjectWrap<DX::Query> pStartQuery;
		ComObjectWrap<DX::Query> pEndQuery;
		hr = Moo::rc().device()->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &pStartQuery);
		hr = Moo::rc().device()->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &pEndQuery);

		uint64 startTime64 = 0;
		uint64 endTime64 = 0;

		if (SUCCEEDED(hr))
		{
			//Don't want to record the speed of the preview window too..
			DebugPtr oldDebug = Manager::instance().debug();
			Manager::instance().debug(NULL);

			pStartQuery->Issue(D3DISSUE_BEGIN);
			pStartQuery->Issue(D3DISSUE_END);

			for (size_t i=0; i<nSamples; i++)
			{
				Manager::instance().draw();
			}

			pEndQuery->Issue(D3DISSUE_BEGIN);
			pEndQuery->Issue(D3DISSUE_END);

			while(S_FALSE == pStartQuery->GetData( &startTime64, 
				sizeof(DWORD), D3DGETDATA_FLUSH )
				&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);

			while(S_FALSE == pEndQuery->GetData( &endTime64, 
				sizeof(DWORD), D3DGETDATA_FLUSH )
				&& Moo::rc().device()->TestCooperativeLevel() != D3DERR_DEVICELOST);

			Manager::instance().debug(oldDebug);

			double timeMSec = (double)(endTime64 - startTime64) / (double)timerFreq64;
			return Script::getData( timeMSec / (double)nSamples );
		}

		Py_RETURN_NONE;
	}

	PY_MODULE_STATIC_METHOD( Manager, profile, _PostProcessing )
}	//namespace post-processing

BW_END_NAMESPACE

// manager.cpp
