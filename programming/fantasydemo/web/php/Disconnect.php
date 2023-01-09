<?php
require_once( 'Page.php' );
require_once( 'Session.php' );

class DisconnectPage extends XHTMLMPPage
{
	function DisconnectPage()
	{
		XHTMLMPPage::XHTMLMPPage( "Login" );
	}

	function initialise()
	{
	}

	function printErrorMsgs()
	{
		if (array_key_exists( 'err_msgs', $_SESSION ))
		{
			echo( xhtmlMpList( $_SESSION['err_msgs'], FALSE, 'error' ) );
			session_unregister( 'err_msgs' );
		}
	}

	function printForm()
	{
		$form = new XHTMLMPForm();

		$rows =
			xhtmlMpTableRow(
				xhtmlMpTableCell(
					'&nbsp;' ).
				xhtmlMpTableCell(
					xhtmlMpFormSubmit( '', 'Return to Login', 'button' )
				)
			);

		$form->setContents( xhtmlMpTable( $rows, 0, '100%', '',
			' cellpadding="0"' ) );

		$form->setAction( "Login.php?logout=1" );
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
		
		echo "You have been disconnected.";
		
		$this->printErrorMsgs();
		$this->printForm();
		$this->printFooter();
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

}

$page = new DisconnectPage();
$page->render();

?>
