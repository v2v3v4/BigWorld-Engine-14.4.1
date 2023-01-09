<?php

class BWError extends Exception
{
	function __construct( $msg )
	{
		// Exceptions can be created by the PHP, and given a string as the arg
		if (is_string( $msg ))
		{
			parent::__construct( $msg );
		}
		else
		{
			parent::__construct( $msg[0] );
		}
	}

	function message()
	{
		$className = get_class( $this );
		return $className . ": " . $this->getMessage();
	}
}

class BWStandardError extends BWError {}

// The following declarations match the error classes in
// bigworld/res/scripts/server_common/BWTwoWay.py
class BWMercuryError extends BWStandardError {}
class BWInternalError extends BWStandardError {}
class BWInvalidArgsError extends BWStandardError {}
class BWNoSuchEntityError extends BWStandardError {}
class BWNoSuchCellEntityError extends BWStandardError {}
class BWAuthenticateError extends BWStandardError {}

// Used when an error object has a type that is not matched by one of the
// exception classes declared above
class BWGenericError extends BWError
{
	function __construct( $excType, $args )
	{
		parent::__construct( "$excType: {$args[0]}", $args );
	}

	function message()
	{
		return $this->getMessage();
	}
}

function raiseBWError( $excType, $args )
{
	if (!is_subclass_of( $excType, "BWError" ))
	{
		throw new BWGenericError( $excType, $args );
	}
	
	throw new $excType( $args );
}

?>
