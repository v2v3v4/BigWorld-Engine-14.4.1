<?php
/**
 *	Defines the authenticator interface.
 */

require_once( 'Debug.php' );
require_once( 'Session.php' );

/**
 *	Session variable array keys.
 */
define( 'AUTHENTICATOR_SESSION_KEY_TOKEN', 			'token' );
define( 'AUTHENTICATOR_TOKEN_KEY_IP_ADDRESS', 		'ip_address' );
define( 'AUTHENTICATOR_TOKEN_KEY_LAST_ACTIVITY', 	'last_activity' );
define( 'AUTHENTICATOR_TOKEN_KEY_USERNAME', 		'username' );


/**
 *	The Authenticator class.
 */
class Authenticator
{
	function Authenticator()
	{
	}

	function authenticateSessionToken()
	{
		if (!isset( $_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN] ))
		{
			throw new BWAuthenticateError( "No authentication token exists" );
		}

		$token =& $_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN];

		// check that they haven't been inactive too long
		if (!isset( $token[AUTHENTICATOR_TOKEN_KEY_LAST_ACTIVITY] ))
		{
			throw new BWAuthenticateError( "Invalid authentication token" );
		}

		$lastActivity =& $token[AUTHENTICATOR_TOKEN_KEY_LAST_ACTIVITY];
		$timeNow = time();

		if ($timeNow >=
			$lastActivity + AUTHENTICATOR_INACTIVITY_PERIOD)
		{
			throw new BWAuthenticateError(
				'Session inactive for more than '.
				AUTHENTICATOR_INACTIVITY_PERIOD. ' seconds' );
		}

		// check that the cookie isn't spoofed by checking against IP
		// address
		if (!isset( $token[AUTHENTICATOR_TOKEN_KEY_IP_ADDRESS] ))
		{
			throw new BWAuthenticateError( 'Invalid authentication token' );
		}
			
		$address =& $token[AUTHENTICATOR_TOKEN_KEY_IP_ADDRESS];
		if ($address != $_SERVER['REMOTE_ADDR'])
		{
			throw new BWAuthenticateError( 'IP address changed' );
		}

		return;
	}

	function doesAuthTokenExist()
	{
		return isset( $_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN] );
	}

	function setSessionToken( $token )
	{
		$_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN] = $token;
	}

	function authenticateUserPass( $username, $pass )
	{
		if ($username == '')
		{
			throw new InvalidFieldError( "Username is empty" );
		}
		if ($pass == '')
		{
			throw new InvalidFieldError( "Password is empty" );
		}

		$this->setSessionToken( Array(
			AUTHENTICATOR_TOKEN_KEY_IP_ADDRESS => $_SERVER['REMOTE_ADDR'],
			AUTHENTICATOR_TOKEN_KEY_LAST_ACTIVITY => time(),
			AUTHENTICATOR_TOKEN_KEY_USERNAME => $username,
		) );
	}


	function updateLastActivity()
	{
		$_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN]
			[AUTHENTICATOR_TOKEN_KEY_LAST_ACTIVITY] = time();
	}

	function setVariable( $key, $value )
	{
		$_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN][$key] = $value;
	}

	function &getVariable( $key )
	{
		return $_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN][$key];
	}


	function invalidateToken()
	{
		session_unregister( AUTHENTICATOR_SESSION_KEY_TOKEN );
	}
}
?>
