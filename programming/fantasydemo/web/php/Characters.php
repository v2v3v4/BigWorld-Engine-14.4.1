<?php
require_once( 'Authenticator.php' );
require_once( 'AuthenticatedPage.php' );
require_once( 'Constants.php' );

class CharactersPage extends AuthenticatedXHTMLMPPage
{
	var $characterList;

	function CharactersPage()
	{
		AuthenticatedXHTMLMPPage::AuthenticatedXHTMLMPPage( "Characters" );
		$this->menubar->removeItem( 'Home' );
		$this->menubar->removeItem( 'Character' );
		$this->menubar->removeItem( 'Inventory' );
		$this->menubar->removeItem( 'AuctionHouse' );
		$this->menubar->removeItem( 'MyAuctions' );
		$this->menubar->removeItem( 'Logout' );
		$this->menubar->addItem( 'Logout', '?logout=1' );
	}

	function initialise()
	{
		$account = $this->auth->entity();

		if(isset( $_GET['logout'] ))
		{
				$this->setRedirect( 'Login.php?logout=1' );
				return;
		}
		
		if (!$account)
		{
			$_SESSION['err_msgs'][] = "Unable to access account";
			$this->setRedirect( 'Disconnect.php' );
			return;
		}

		if ($this->auth->entityType() == "Avatar")
		{
			$this->setRedirect( CHARACTER_WELCOME_PAGE_URL );
			return;
		}

		if (isset( $_GET[ 'new_character_name' ] ))
		{
			try
			{
				if ($_GET[ 'new_character_name' ] == '')
				{
					throw new InvalidFieldError( "Character name is empty" );
				}

				$res = $account->webCreateCharacter(
					array( "name" => $_GET[ 'new_character_name' ] ) );
			}
			catch( Exception $e )
			{
				$this->addExceptionMsg( $e,	"Could not create character" );
			}

			debug( "webCreateCharacter". debugStringObj( $res ) );

			$this->addStatusMsg( "Character creation successful" );
		}

		if (isset( $_GET['character'] ))
		{
			try
			{
				$res = $account->webChooseCharacter( array(
						"name" => $_GET[ 'character' ],
						"type" => "Avatar" ) );
			}
			catch( Exception $e )
			{
				$this->addExceptionMsgAndDisconnect( $e );
				return;
			}

			$this->addStatusMsg( "Login successful" );
			debug( debugStringObj( $res ) );

			$this->auth->setEntityDetails( $res[ "type" ], $res[ "id" ] );

			$this->setRedirect( 'News.php' );
		}

		try
		{
			$res = $account->webGetCharacterList();
		}
		catch( Exception $e )
		{
			$this->addExceptionMsgAndDisconnect( $e );
			return;
		}

		$this->characterList = $res["characters"];
	}

	function renderBody()
	{
		global $CHAR_CLASS_NAMES;
		$this->renderErrorMsgs();

		echo xhtmlMpHeading( "Character List", 2 );

		if (!count( $this->characterList ))
		{

		}
		$rows = '';
		$count = 0;

		$height = 100;
		if (isset( $_SESSION['display'] ))
		{
			$height = $_SESSION['display']['resolution_height'] / 3;
		}

		foreach ($this->characterList as $character)
		{
			$row = xhtmlMpTableRow(
				xhtmlMpTableCell(
					xhtmlMpLink( '?character='. $character['name'],
						xhtmlMpImg( 'Image.php?path=images/'. 
								$character['charClass']. 
								'_profile_front.png&max_height='.$height,
							$character['name']. "'s Portrait" )
					), FALSE, 'portrait'
				).
				xhtmlMpTableCell(
					xhtmlMpLink( '?character='. $character['name'],
						$character['name'] ).
						' ('. $CHAR_CLASS_NAMES[ $character['charClass'] ]. ')', 
					FALSE, 'name'
				),
				($count++ % 2) ? "alt1":"alt2"

			);
			$rows .= $row;
		}

		$newCharacterForm = new XHTMLMPForm();
		$newCharacterForm->setMethod( "get" );
		$newCharacterForm->setContents( 
			"Character Name: ".
			xhtmlMpFormInputText( "new_character_name" ).
			xhtmlMpFormSubmit( "", "Create" ) );
		$rows .= xhtmlMpTableRow( 
			xhtmlMpTableCell( 'Add new character' ).
			xhtmlMpTableCell(
				$newCharacterForm->output() ),

			($count++ % 2) ? "alt1":"alt2"
		);
					
			
		echo xhtmlMpTable( $rows, "0", '', "character_list",
			' cellspacing="0" cellpadding="0"' );
	}
}

$page = new CharactersPage();
$page->render();
?>
