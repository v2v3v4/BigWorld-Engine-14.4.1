/**************************************************************************

Filename    :   fx_game_delegate.cpp
Content     :   Communication logic for CLIK GameDelegate
Created     :
Authors     :   Prasad Silva

Copyright   :   Copyright 2011 Autodesk, Inc. All Rights reserved.

Use of this software is subject to the terms of the Autodesk license
agreement provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

**************************************************************************/

#include "pch.hpp"
#include "fx_game_delegate.hpp"

using namespace GFx;

//
// Visitor to register callbacks
//
class AddCallbackVisitor : public FxDelegateHandler::CallbackProcessor
{
public:
	AddCallbackVisitor(FxDelegateHandler* pthis, FxDelegate::CallbackHash* phash)
		: pThis(pthis), pHash(phash) {}

	void Process(const String& methodName, FxDelegateHandler::CallbackFn method)
	{
		FxDelegate::CallbackDefn cbt;
		cbt.pThis = pThis;
		cbt.pCallback = method;
		pHash->Add(methodName, cbt);
	}

private:
	FxDelegateHandler*          pThis;
	FxDelegate::CallbackHash*   pHash;
};


//
// Visitor to unregister callbacks
//
class RemoveCallbackVisitor : public FxDelegateHandler::CallbackProcessor
{
public:
	RemoveCallbackVisitor(FxDelegateHandler* pthis, FxDelegate::CallbackHash* phash)
		: pThis(pthis), pHash(phash) {}

	void Process(const String& methodName, FxDelegateHandler::CallbackFn method)
	{
		SF_UNUSED(method);
		pHash->Remove(methodName);
	}

private:
	FxDelegateHandler*          pThis;
	FxDelegate::CallbackHash*   pHash;
};

//////////////////////////////////////////////////////////////////////////

FxDelegate::FxDelegate()
{

}

void FxDelegate::RegisterHandler(FxDelegateHandler* callback)
{
	AddCallbackVisitor reg(callback, &Callbacks);
	callback->Accept(&reg);
}

void FxDelegate::UnregisterHandler(FxDelegateHandler* callback)
{
	RemoveCallbackVisitor reg(callback, &Callbacks);
	callback->Accept(&reg);
}

void FxDelegate::Invoke(Movie* pmovieView, const char* methodName, 
						FxResponseArgsBase& args)
{
	Value* pv = NULL;
	unsigned nv = args.GetValues(&pv);
	pv[0] = methodName;
	pmovieView->Invoke("call", NULL, pv, nv);
}

void FxDelegate::Callback(Movie* pmovieView, const char* methodName, const Value* args, unsigned argCount)
{
	SF_ASSERT(argCount > 0);  // Must at least have a uid parameter

	CallbackDefn* pcb = Callbacks.GetAlt(methodName);
	if (pcb != NULL) 
	{
		FxDelegateArgs params(pcb->pThis, 
			pmovieView, 
			&args[0], 
			argCount);
		pcb->pCallback(params);
	}
	else if (Fallback.pThis.GetPtr() != NULL)
	{
		FxDelegateArgs params(Fallback.pThis, 
			pmovieView, 
			&args[0], 
			argCount);
		Fallback.pCallback(params, methodName);
	}
}


