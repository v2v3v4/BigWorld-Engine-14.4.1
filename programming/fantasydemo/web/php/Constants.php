<?php
/**
 * Definitions for constants.
 */
ini_set('display_errors', TRUE);

/** output configuration **/
define( 'DEFAULT_STYLE' , 'styles/fantasydemo.css' );
define( 'DOCTYPE_XHTMLMP',
	'<!DOCTYPE html PUBLIC "-//WAPFORUM//DTD XHTML Mobile 1.0//EN" "http://www.wapforum.org/DTD/xhtml-mobile10.dtd">' );
define( 'DOCTYPE_HTML',
	'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">' );
define( 'CONTENTTYPE_HTML', 'text/html' );
define( 'CONTENTTYPE_XHTML', 'application/xml+xhtml' );

/** Authenticator configuration **/
define( 'AUTHENTICATOR_CLASS', 'BWAuthenticator');
require_once('BWAuthenticator.php' );
define( 'AUTHENTICATOR_INACTIVITY_PERIOD', '3600' );

/** Page configuration **/
define( 'LOGIN_PAGE_URL', 'Login.php' );
define( 'ACCOUNT_WELCOME_PAGE_URL', 'Characters.php' );
define( 'CHARACTER_WELCOME_PAGE_URL', 'News.php' );

/** Items **/

/*

	From fantasydemo/res/scripts/common/ItemBase.py:

	NONE_TYPE				= -1
	STAFF_TYPE				=  2
	STAFF_TYPE_2			=  3
	DRUMSTICK_TYPE			=  4
	SPIDER_LEG_TYPE			=  5
	BINOCULARS_TYPE			=  6
	SWORD_TYPE				=  7
	SWORD_TYPE_2			=  9
	GOBLET_TYPE				=  17
*/

define( 'BW_ITEM_TYPE_STAFF', 			2 );
define( 'BW_ITEM_TYPE_STAFF_2', 		3 );
define( 'BW_ITEM_TYPE_DRUMSTICK', 		4 );
define( 'BW_ITEM_TYPE_SPIDER_LEG', 		5 );
define( 'BW_ITEM_TYPE_BINOCULARS', 		6 );
define( 'BW_ITEM_TYPE_SWORD', 			7 );
define( 'BW_ITEM_TYPE_SWORD_2', 		9 );
define( 'BW_ITEM_TYPE_GOBLET', 			17 );

/** Trade handle states **/
define( 'TRADEHANDLE_STATE_OFFERING', 			1 );
define( 'TRADEHANDLE_STATE_OFFER', 				2 );
define( 'TRADEHANDLE_STATE_REPLYING', 			3 );
define( 'TRADEHANDLE_STATE_REPLY', 				4 );
define( 'TRADEHANDLE_STATE_ACCEPTING', 			5 );
define( 'TRADEHANDLE_STATE_REJECTED', 			6 );
define( 'TRADEHANDLE_STATE_REPLY_CANCELLING', 	7 );
define( 'TRADEHANDLE_STATE_OFFER_CANCELLING', 	8 );

define( 'DEBUG_ADDR', '' /* "127.0.0.1" */ );
define( 'DEBUG_PORT', "47300" );
define( 'DEBUG_BIND_ADDR', "127.0.0.1" );

$FD_ARTICLES = Array(
			Array(
				"title"		=> "Welcome!",
				'date'		=> 'Thu 23 Feb 06',
				"summary" 	=> "Welcome to BigWorld FantasyDemo mobile.",
				"body" 		=> "Welcome to BigWorld FantasyDemo mobile."
			),
			Array(
				"title"		=> "Ripper Race",
				'date'		=> 'Thu 23 Feb 06',
				"summary"	=> "Race around FantasyDemo with other players!",
				"body"		=> "Race around FantasyDemo with other players!"
			),
			Array(
				"title"		=> "Sword Duelling Championship",
				'date'		=> 'Thu 23 Feb 06',
				"summary"	=>
	'There will be a sword duelling championship held in front of the '.
	'pond in the village. All are invited!',
				"body"		=>
	"There will be a sword duelling championship held in front of the pond ".
	'in the village. Mercenaries can challenge each other to fight for the '.
	'glory, reputation, experience points and gold! All are invited! '
			)
		);

if (DEBUG_ADDR)
{
	initDebugSocket( DEBUG_ADDR, (int) DEBUG_PORT, DEBUG_BIND_ADDR );
}

$CHAR_CLASS_NAMES = Array(
	"ranger" 		=> "Ranger",
	"female_warrior" 	=> "Female Warrior",
	"ped_male" 		=> "Civilian",
	"barbarian"		=> "Barbarian"
);
?>
