/**************************************************************************

Filename    :   fx_game_delegate.hpp
Content     :   Communication logic for CLIK GameDelegate
Created     :
Authors     :   Prasad Silva

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/

#ifndef INC_FxGameDelegateHandler_H
#define INC_FxGameDelegateHandler_H

#include "config.hpp"

class FxDelegate;
class FxDelegateArgs;

//
// Interface implemented by all callback handlers. These handlers register 
// callbacks with the fx_game_delegate.
//
class FxDelegateHandler : public RefCountBase<FxDelegateHandler, Stat_Default_Mem>
{
public:
	virtual ~FxDelegateHandler() {}

	//
	// All callback methods must have the following signature. To produce a response
	// to the callback, push parameters to the game delegate.
	//
	typedef void (*CallbackFn)(const FxDelegateArgs& params);

	//
	// Special handler callback signature to listen for all methods.
	//
	typedef void (*FallbackFn)(const FxDelegateArgs& params, const char* methodName);

	//
	// Interface implemented by callback registrars. The handler should 
	// pass the appropriate parameters to the Visit method.
	//
	class CallbackProcessor
	{
	public:
		virtual ~CallbackProcessor() {}
		virtual void    Process(const String& methodName, CallbackFn method) = 0;
	};

	//
	// Callback registrar visitor method
	// Implementations are expected to call the registrar's Process method
	// for all callbacks.
	//
	virtual void        Accept(CallbackProcessor* cbreg) = 0;
};


//////////////////////////////////////////////////////////////////////////


//
// Callback response parameters
// The 
//
class FxResponseArgsBase
{
public:
	virtual ~FxResponseArgsBase() {}
	virtual unsigned GetValues(GFx::Value** pparams) = 0;
};

//
// Callback response that uses stack based storage
//
template <int N>
class FxResponseArgs : public FxResponseArgsBase
{
public:
	FxResponseArgs() : Index(1) {}  
	void    Add(const GFx::Value& v)
	{
		if (Index > N) 
		{
			SF_DEBUG_WARNING(1, "Adding parameter out of bounds!");
			return;
		}
		Values[Index++] = v;
	}
	unsigned    GetValues(GFx::Value** pparams) { *pparams = Values; return Index; }
private:
	GFx::Value    Values[N+1];    // Space for response data
	unsigned        Index;
};

//
// Callback response that uses dynamically allocated storage
//
class FxResponseArgsList : public FxResponseArgsBase
{
public:
	FxResponseArgsList()                    { Args.PushBack(GFx::Value::VT_Null); }   // Space for response data
	void    Add(const GFx::Value& v)          { Args.PushBack(v); }
	unsigned    GetValues(GFx::Value** pparams)    { *pparams = &Args[0]; return (unsigned)Args.GetSize(); }
private:
	Array<GFx::Value>        Args;
};


//////////////////////////////////////////////////////////////////////////


//
// Parameters passed to the callback handler
//
class FxDelegateArgs
{
public:
	FxDelegateArgs(FxDelegateHandler* pthis, GFx::Movie* pmovie, 
		const GFx::Value* vals, unsigned nargs) : pThis(pthis), 
		pMovieView(pmovie), Args(vals), NArgs(nargs) {}

	FxDelegateHandler*  GetHandler() const      { return pThis; }
	GFx::Movie*       GetMovie() const        { return pMovieView; }

	const GFx::Value&     operator[](UPInt i) const
	{ 
		SF_ASSERT(i < NArgs);
		return Args[i]; 
	}
	unsigned                GetArgCount() const     { return NArgs; }

private:
	FxDelegateHandler*      pThis;
	GFx::Movie*           pMovieView;
	const GFx::Value*         Args;
	unsigned                    NArgs;
};


//////////////////////////////////////////////////////////////////////////


//
// Callback manager that marshals calls from ActionScript 
//
class FxDelegate : public GFx::ExternalInterface
{
public:
	//
	// Callback target
	//
	struct CallbackDefn
	{
		Ptr<FxDelegateHandler>         pThis;
		FxDelegateHandler::CallbackFn   pCallback;
	};

	// Fallback target
	struct FallbackDefn
	{
		Ptr<FxDelegateHandler>         pThis;
		FxDelegateHandler::FallbackFn   pCallback;
	};

	//
	// Callback hash
	//
	struct CallbackHashFunctor
	{
		UPInt  operator()(const char* data) const
		{
			UPInt  size = SFstrlen(data);
			return String::BernsteinHashFunction(data, size);
		}
	};
	typedef Hash<String, CallbackDefn, CallbackHashFunctor> CallbackHash;


	FxDelegate();

	//
	// Install and uninstall callbacks
	//
	void            RegisterHandler(FxDelegateHandler* callback);
	void            UnregisterHandler(FxDelegateHandler* callback);

	// Install and uninstall fallback handler
	void            RegisterFallbackHandler(FxDelegateHandler* pthis, FxDelegateHandler::FallbackFn pcallback)
	{
		Fallback.pThis = pthis;
		Fallback.pCallback = pcallback;
	}
	void            UnregisterFallbackHandler() { Fallback.pThis = NULL; }

	// Clear all callbacks/handlers
	void            ClearAll()
	{
		Callbacks.Clear();
		UnregisterFallbackHandler();
	}

	//
	// Call a method registered with the AS2 GameDelegate instance
	//
	static void    Invoke(GFx::Movie* pmovieView, const char* methodName, 
		FxResponseArgsBase& args);

	//
	// ExternalInterface callback entry point
	//
	void            Callback(GFx::Movie* pmovieView, const char* methodName, 
		const GFx::Value* args, unsigned argCount);

private:
	//
	// Callbacks installed with the game delegate
	//
	CallbackHash    Callbacks;

	//
	// Fallback handler
	//
	FallbackDefn    Fallback;
};





#endif // INC_FxGameDelegateHandler_H
