<?php
require_once( 'Authenticator.php' );
require_once( 'BigWorld.php' );

define( 'BW_AUTHENTICATOR_TOKEN_KEY_ENTITY_TYPE', 'entity_type' );
define( 'BW_AUTHENTICATOR_TOKEN_KEY_ENTITY_PATH', 'entity_path' );

class BWAuthenticator extends Authenticator
{
	function BWAuthenticator()
	{
		Authenticator::Authenticator();
	}

	function setEntityDetails( $type, $id )
	{
		$this->setVariable( BW_AUTHENTICATOR_TOKEN_KEY_ENTITY_TYPE, $type );
		$this->setVariable( BW_AUTHENTICATOR_TOKEN_KEY_ENTITY_PATH,
				"entities_by_id/" . $type . '/' . $id );
	}

	function entityType()
	{
		return $this->getVariable( BW_AUTHENTICATOR_TOKEN_KEY_ENTITY_TYPE );
	}

	function entity()
	{
		$entityPath =
			$this->getVariable( BW_AUTHENTICATOR_TOKEN_KEY_ENTITY_PATH );

		if (!isset( $entityPath ))
		{
			return FALSE;
		}

		return new RemoteEntity( $entityPath );
	}

	function authenticateUserPass ( $username, $pass )
	{
		// 	debug('username='. debugStringObj($username). ',
		// 		pass = '. debugStringObj($pass));
		if ($username == '')
		{
			throw new InvalidFieldError( "Username is empty" );
		}
		if ($pass == '')
		{
			throw new InvalidFieldError( "Password is empty" );
		}

		$db = new RemoteEntity( "db" );

		$result = $db->logOn(
				array( "username" => $username,
					"password" => $pass ) );

		debug( "logOn: " . debugStringObj( $result ) );

		$this->setSessionToken( array(
				AUTHENTICATOR_TOKEN_KEY_IP_ADDRESS => $_SERVER[ 'REMOTE_ADDR' ],
				AUTHENTICATOR_TOKEN_KEY_LAST_ACTIVITY => time(),
				AUTHENTICATOR_TOKEN_KEY_USERNAME => $username ) );

		$this->setEntityDetails( $result[ "type" ], $result[ "id" ] );
	}
}
?>
