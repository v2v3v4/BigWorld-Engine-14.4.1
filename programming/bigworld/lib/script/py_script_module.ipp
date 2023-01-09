// py_script_module.ipp

/**
 *	This method imports a given ScriptModule
 *	@param name			The name of the script module to import
 *	@param errorHandler The type of error handling to use if this method
 *		fails
 *	@returns			The imported module
 */
template <class ERROR_HANDLER>
/* static */ inline ScriptModule ScriptModule::import( const char * name,
	const ERROR_HANDLER & errorHandler )
{
	PyObject * pModule = PyImport_ImportModule( name );
	errorHandler.checkPtrError( pModule );
	return ScriptModule( pModule, ScriptObject::FROM_NEW_REFERENCE );
}


/**
 *	This method gets an existing module, or creates a new one of the module
 *	does not exists.
 *
 *	@param name			The name of the script module to create/get
 *	@param errorHandler The type of error handling to use if this method
 *		fails
 *	@returns			The module
 */
template <class ERROR_HANDLER>
/* static */ inline ScriptModule ScriptModule::getOrCreate( const char * name,
	const ERROR_HANDLER & errorHandler )
{
	PyObject * pModule = PyImport_AddModule( name );
	errorHandler.checkPtrError( pModule );
	return ScriptModule( pModule, ScriptObject::FROM_BORROWED_REFERENCE );
}


/**
 *	This method reloads an existing module.
 *
 *	@param module		The module to reload.
 *	@param errorHandler The type of error handling to use if this method
 *		fails.
 *	@returns			The reloaded module.
 */
template <class ERROR_HANDLER>
/* static */ inline ScriptModule ScriptModule::reload( ScriptModule module,
	const ERROR_HANDLER & errorHandler )
{
	PyObject * pModule = PyImport_ReloadModule( module.get() );
	errorHandler.checkPtrError( pModule );
	return ScriptModule( pModule, ScriptObject::FROM_NEW_REFERENCE );
}


/**
 *	This method adds an object to a module.
 *
 *	@param name			The name of the object to add.
 *	@param value		The object to be added as name.
 *	@param errorHandler The type of error handling to use if this method
 *		fails.
 *	@returns			True on success, false on failure.
 */
template <class ERROR_HANDLER>
inline bool ScriptModule::addObject( const char * name, 
	const ScriptObject & value, const ERROR_HANDLER & errorHandler ) const
{
	// PyModule_AddObject steals a reference, so we increment it
	// before hand.
	int result = PyModule_AddObject( this->get(), name, value.newRef() );
	errorHandler.checkMinusOne( result );
	return result != -1;
}


/**
 *	This method adds an type to a module
 *
 *	@param name			The name of the type to add
 *	@param value		The type to be added as name
 *	@param errorHandler The type of error handling to use if this method
 *		fails
 *	@returns			True on success, false on failure
 */
template <class ERROR_HANDLER>
inline bool ScriptModule::addObject( const char * name, PyTypeObject * value,
	const ERROR_HANDLER & errorHandler ) const
{
	// TODO: Should we inref the type, as it gets stolen
	int result = PyModule_AddObject( this->get(), name, (PyObject*)value );
	errorHandler.checkMinusOne( result );
	return result != -1;
}


/**
 *	This method adds an integer constant to a module
 *
 *	@param name			The name of the constant
 *	@param value		The constant value
 *	@param errorHandler The type of error handling to use if this method
 *		fails
 *	@returns			True on success, false on failure
 */
template <class ERROR_HANDLER>
inline bool ScriptModule::addIntConstant( const char * name, long value,
	const ERROR_HANDLER & errorHandler ) const
{
	int result = PyModule_AddIntConstant( this->get(), name, value );
	errorHandler.checkMinusOne( result );
	return result != -1;
}


/**
 *	This method gets the dict from a module.
 *
 *	@returns		The dict for the given module.
 */
inline ScriptDict ScriptModule::getDict() const
{
	// From python documentation:
	// "This function never fails."
	return ScriptDict( PyModule_GetDict( this->get() ),
		ScriptObject::FROM_BORROWED_REFERENCE );
}
