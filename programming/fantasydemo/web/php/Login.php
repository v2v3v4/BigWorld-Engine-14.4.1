<?php
require_once( 'AuthenticatedPage.php' );
require_once( 'Session.php' );
require_once( 'BWAuthenticator.php' );

define ('FORCE_PASSWORD', 'pass' );

class LoginPage extends XHTMLMPPage
{
	var $returnUrl;

	function LoginPage()
	{
		XHTMLMPPage::XHTMLMPPage( "Login" );
	}

	function initialise()
	{
		$auth = new BWAuthenticator();

		// handle logout request to invalidate
		if (isset( $_REQUEST['logout'] ) && $_REQUEST['logout'])
		{
			$auth->invalidateToken();
			session_unset();
			header( 'Location: '. LOGIN_PAGE_URL );
			exit;
		}

		// check authentication token if it exists, or a username/password
		// if it exists
		if ($auth->doesAuthTokenExist())
		{
			try
			{
				$auth->authenticateSessionToken();
			}
			catch( exception $e )
			{
				$this->addExceptionMsgAndDisconnect( $e );
			}
		}
		else if (isset( $_REQUEST['username'] )
			&& isset( $_POST['password'] ))
		{
			// password must be in POST parameters for security
			$pass = $_POST['password'];
			if (FORCE_PASSWORD != '')
			{
				// we have forced password to be something - only use for
				// demos!
				$pass = FORCE_PASSWORD;
			}

			try
			{
				$auth->authenticateUserPass(
					$_REQUEST['username'], $pass );
			}
			catch( InvalidFieldError $e )
			{
				$this->addExceptionMsg( $e );
				return;
			}
			catch( BWAuthenticateError $e )
			{
				$this->addExceptionMsg( $e );
				return;
			}
			catch( Exception $e )
			{
				$this->addExceptionMsgAndDisconnect( $e );
				return;
			}
		}
		else
		{
			// no auth token and no login attempt
			return;
		}
		if ($this->returnUrl)
		{
			header( 'Location: '. $this->returnUrl );
			session_write_close();
			exit;
		}
		else
		{
			if ($auth->entityType() == "Avatar")
			{
				header( 'Location: '. CHARACTER_WELCOME_PAGE_URL );
			}
			else
			{
				header( 'Location: '. ACCOUNT_WELCOME_PAGE_URL );
			}
			session_write_close();
			exit;
		}

	}


	function printFooter()
	{
		echo (
			xhtmlMpDiv(
				xhtmlMpImg( 'images/logo_bigworld.jpg',
					'BigWorld Logo' ),
				'footer'
			)
		);
	}

	function printErrorMsgs()
	{
		if (array_key_exists( 'err_msgs', $_SESSION ))
		{
			echo( xhtmlMpList(
				$_SESSION['err_msgs'], FALSE, 'error' ) );

			session_unregister( 'err_msgs' );
		}
	}

	function printInstructions()
	{
	}

	function printLoginForm()
	{

		$username = '';
		if (isset( $_REQUEST['username'] ))
		{
			$username = $_REQUEST['username'];
		}
		$form = new XHTMLMPForm();

		$rows =
			xhtmlMpTableRow(
				xhtmlMpTableCell(
					xhtmlMpHeading( 'User name', 2 )
				).
				xhtmlMpTableCell(
					xhtmlMpFormInputText( 'username',
						$username,
						'',
						'textfield',
						' size="10"' )
			) ).
			xhtmlMpTableRow(
				xhtmlMpTableCell(
					xhtmlMpHeading( 'Password', 2 )
				).
				xhtmlMpTableCell(
					xhtmlMpFormInputPassword( 'password',
						'',
						'',
						'textfield',
						' size="10"'
					)
				)
			).
			xhtmlMpTableRow(
				xhtmlMpTableCell(
					'&nbsp;' ).
				xhtmlMpTableCell(
					xhtmlMpFormSubmit( '', 'Login', 'button' )
				)
			);

		$form->setContents( xhtmlMpTable( $rows, 0, '100%', '',
			' cellpadding="0"' ) );

		$form->setAction( "Login.php" );
		$form->setMethod( "post" );
		$form->setClassName( "loginbox" );

		echo( $form->output() );
	}

	function renderBody()
	{
		echo( xhtmlMpDiv(
			xhtmlMpHeading(
				'&middot;'.
					strtoupper( "BigWorld FantasyDemo" ).
					'&middot;',
				1, 'heading'
			) ,
			'', ' id="login"' )
		);

		$this->printErrorMsgs();
		$this->printLoginForm();

		/*
		$device = GetDevice();

		$contents = ( xhtmlMpDiv(
			$device->getCapability( 'brand_name' ). ' '. 
			$device->getCapability( 'model_name' ). ' '.
			$device->getCapability( 'model_extra_info' ).' '.
			$device->getCapability( 'resolution_width' ).
			'x'.
			$device->getCapability( 'resolution_height' ),
			'user-agent-info'
			 )
		) ;

		echo( $contents );
		*/

		$this->printInstructions();
		$this->printFooter();
	}
}

$page = new LoginPage();
$page->render();

?>
