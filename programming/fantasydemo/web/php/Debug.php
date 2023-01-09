<?php

/**
 * Debug functions.
 */

define( 'DEBUG_TAB_SIZE', '4' );

$deferErrorPrinting = FALSE;
$deferredErrors = Array();


$debugSocket = FALSE;
$debugSocketAddr = FALSE;
$debugSocketPort = FALSE;


/**
 * UDP socket debugging.
 */
function initDebugSocket( $addr = DEFAULT_DEBUG_ADDRESS,
		$port = DEFAULT_DEBUG_PORT,
		$bindAddr = "0.0.0.0" )
{
	global $debugSocket;
	global $debugSocketAddr;
	global $debugSocketPort;

	$debugSocket = socket_create( AF_INET, SOCK_DGRAM, 0 );
	socket_bind( $debugSocket, $bindAddr );
	$debugSocketAddr = $addr;
	$debugSocketPort = $port;
}


/**
 * If called with TRUE, defers error output printing by the debug output
 * handler until  deferErrorPrinting is called with FALSE, at which point
 * the output is printed.
 *
 * @param val whether to start or stop deferring error output
 */
function deferErrorPrinting( $val )
{
	global $deferErrorPrinting;
	global $deferredErrors;

	if ($deferErrorPrinting && !$val && count( $deferredErrors ) > 0)
	{
		// print out the saved errors
		echo "<!-- deferred error output follows\n";
		foreach ($deferredErrors as $errorContents)
		{
			echo "$errorContents\n";
		}
		echo " deferred error output above-->\n";
		$deferredErrors = Array();
	}

	$deferErrorPrinting = $val;

}


/**
 * Returns a debug string representation of the given PHP object.
 *
 * @param $obj the PHP object
 * @return string
 */
function debugStringObj( $obj, $level=0 )
{
	// output buffering - capture what is being echoed
	ob_start();

	if ($level > 0)
	{
		$indent = str_repeat( ' ', $level * DEBUG_TAB_SIZE );
	}
	else
	{
		$indent = '';
	}

	if (gettype( $obj ) == 'string')
	{
		echo( $indent. '"'. $obj. '"' );
	}
	else if (gettype( $obj ) == 'boolean')
	{
		if ($obj)
		{
			echo $indent. 'True';
		}
		else
		{
			echo $indent. 'False';
		}
	}
	else if (gettype( $obj ) == 'array')
	{
		echo( $indent. "Array {\n" );
		$first = true;
		foreach ($obj as $key => $value)
		{
			if (!$first)
			{
				echo ", \n";
			}
			echo( debugStringObj( $key, $level + 1 ). ' => '.
				debugStringObj( $value, $level + 1 ) );
			$first = false;
		}
		echo( "\n". $indent. "}" );
	}
	else if (gettype( $obj ) == 'integer' ||
		gettype( $obj ) == 'double')
	{
		echo( $indent. $obj );
	}
	else if (gettype( $obj ) == 'NULL')
	{
		echo( $indent. "(null)" );
	}
	else
	{
		echo( $indent. print_r( $obj ) );
	}
	$contents = ob_get_contents();
	ob_end_clean();
	return $contents;
}


function errnoToString( $errno )
{
	switch ($errno)
	{
		case E_ERROR: 			return "E_ERROR";
		case E_WARNING: 		return "E_WARNING";
		case E_PARSE:			return "E_PARSE";
		case E_NOTICE:			return "E_NOTICE";
		case E_CORE_ERROR:		return "E_CORE_ERROR";
		case E_COMPILE_ERROR:	return "E_COMPILE_ERROR";
		case E_COMPILE_WARNING:	return "E_COMPILE_WARNING";
		case E_USER_ERROR:		return "E_USER_ERROR";
		case E_USER_WARNING:	return "E_USER_WARNING";
		case E_USER_NOTICE:		return "E_USER_NOTICE";
		case E_ALL:				return "E_ALL";
		default:				return "Error Level: $errno";
	}
}


/**
 * Debug error handler.
 */
function debugErrorHandler( $errno, $errstring, $errfile='', $errline='',
		$errcontext='' )
{
	global $deferErrorPrinting;
	global $deferredErrors;

	error_log( "[". errnoToString($errno). "] ($errfile:$errline) :\n".
		$errstring );

	// just print the above description for E_STRICT errors under PHP 5
	if ($errno == 2048)
	{
		return;
	}
	$contents = errnoToString( $errno ). ' : '. $errstring. " \n";
	$contents .= "in file ". $errfile. ' on line '. $errline. "\n";

	if ($errno & (E_ERROR | E_CORE_ERROR | E_COMPILE_ERROR | E_USER_ERROR))
	{
		$contents .= "\nContext: \n";

		foreach (array_keys( $errcontext ) as $key)
		{
			$value =& $errcontext[$key];
			$contents .= $key. " = ". debugStringObj( $value ). "\n";
		}
	}

	$contents .= "\nBacktrace: \n";

	$bt = debug_backtrace();
	foreach ($bt as $framenum => $frame)
	{
		if ($framenum == 0)
		{
			continue;
		}
		$contents .= "[${framenum}]: ";
		if (array_key_exists( 'class', $frame ))
		{
			$contents .= $frame['class'] . '::';
		}
		$contents .= $frame['function']. '()';
		if (array_key_exists( 'file', $frame )
				&& array_key_exists( 'line', $frame ))
		{
			$contents .= 'in '. $frame['file']. ':'. $frame['line'];
		}
		if (array_key_exists( 'args', $frame ))
		{
			$contents .= "\nargs (";
			$first = true;
			foreach ($frame as $arg)
			{
				if (!$first)
				{
					$contents .= ', '. objectToString( $arg );
				}
				else
				{
					$contents .= objectToString( $arg );
				}
				$first = false;
			}
			$contents .= ")";
		}
		$contents .= "\n\n";
	}


	debug( $contents );
}

function objectToString( $arg )
{
	if (gettype( $arg ) == "object")
	{
		return get_class( $arg ). " instance";
	}
	else
	{
		return (string)($arg);
	}
}


/**
 * Outputs a XHTML comment with the given text.
 */
function debug( $text )
{
	global $deferErrorPrinting;
	global $deferredErrors;

	global $debugSocket;
	global $debugSocketAddr;
	global $debugSocketPort;


	if ($debugSocket)
	{
		$sentText = $text;
		while (strlen( $sentText ) > 0)
		{
			$num = socket_sendto( $debugSocket,
				$sentText, strlen( $sentText ), 0,
				$debugSocketAddr, $debugSocketPort );
			if ($num < 0)
			{
				break;
			}
			$sentText = substr( $sentText, $num );
		}
	}

	error_log( $text );

	if ($deferErrorPrinting)
	{
		$deferredErrors[] = $text;
	}
	else
	{
		echo( "<!-- \n$text\n -->\n" );
	}
}

function debugObj( $obj )
{
	debug( debugStringObj( $obj ) );
}

// set the error handler
set_error_handler( 'debugErrorHandler' );

?>
