#ifndef ENTITY_CALLBACK_BUFFER_HPP
#define ENTITY_CALLBACK_BUFFER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_vector.hpp"

#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

class Entity;
typedef SmartPointer<Entity> EntityPtr;

class BufferedScriptCallback
{
public:

	EntityPtr entity;
	ScriptObject callable;
	ScriptArgs args;
	const char * errorPrefix;

};

typedef BW::vector< BufferedScriptCallback > BufferedScriptCallbacks;

class EntityCallbackBuffer
{
public:

	EntityCallbackBuffer();

	void enableHighPriorityBuffering();
	void disableHighPriorityBuffering();

	void enterBufferedSection();
	void leaveBufferedSection();

	bool isBuffering() const { return callbacksPrevented_ == 0; }

	void storeCallback(
		EntityPtr entity, 
		const ScriptObject & funcCallable, const ScriptArgs & args,
		const char * errorPrefix );

private:

	void flush();

	void execute( BufferedScriptCallbacks& scriptCalls ) const;

	// Members
	
	int callbacksPrevented_;
	int allowCallbacksOverride_;

	BufferedScriptCallbacks buffer_;
	BufferedScriptCallbacks highPriorityBuffer_;
};

BW_END_NAMESPACE

#endif // #ifndef ENTITY_CALLBACK_BUFFER_HPP
