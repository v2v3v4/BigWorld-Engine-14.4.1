<?php
require_once( 'AuthenticatedPage.php' );

class CharacterPage extends AuthenticatedXHTMLMPPage
{
	var $playerName;
	var $charClass;
	var $playerStatistics;
	var $position;
	var $direction;
	var $health;
	var $maxHealth;
	var $frags;

	function CharacterPage()
	{
		parent::AuthenticatedXHTMLMPPage( "Character" );
		if ($this->getDevice() == "Sony PlayStation Portable")
		{
			$this->styleHrefs[] = "styles/psp/character.css";
		}
		else if ($this->getDevice() ==
				"Research In Motion Ltd. BlackBerry 7100")
		{
			$this->styleHrefs[] = "styles/blackberry/character.css";
		}
		else
		{
			$this->styleHrefs[] = "styles/character.css";
		}
	}

	function initialise()
	{
		global $CHAR_CLASS_NAMES;
		$player = $this->auth->entity();

		try
		{
			$res = $player->webGetPlayerInfo();
		}
		catch( Exception $e )
		{
			$this->addExceptionMsgAndDisconnect( $e );
			return;	
		}
		
		debugObj( $res );

		$this->charClass 	= $res["charClass"];
		$this->playerName 	= $res["playerName"];
		$this->health 		= $res["health"];
		$this->maxHealth 	= $res["maxHealth"];
		$this->frags 		= $res["frags"];
		
		list( $x, $y, $z ) = $res["position"];
		$this->position		= sprintf( "(%.01f, %.01f, %.01f)", $x, $y, $z );

		list( $roll, $pitch, $yaw ) = $res["direction"];

		$radDegFactor = 360.0 / (2. * 3.1415);
		$this->direction	= sprintf( "(yaw=%.01f, pitch=%.01f, roll=%.01f) [deg]",
			$yaw * $radDegFactor,
			$pitch * $radDegFactor,
			$roll * $radDegFactor
		);

		$this->title = $this->playerName;

		$this->playerStatistics = Array(
			'Class'			=>  $CHAR_CLASS_NAMES[ $this->charClass ],
			'Strength' 		=> 	23,
			'Dexterity'		=> 	15,
			'Defense'		=> 	21,
			'Intelligence'	=>	18,
			"Health"		=>	$this->health.
				"&nbsp;/&nbsp;". $this->maxHealth,
			"Frags"			=>	$this->frags,
			"Status"		=>	$res["online"] ? "online":"offline"
		);
		if ($res["online"])
		{
			$this->playerStatistics['Position'] = $this->position;
			$this->playerStatistics['Direction'] = $this->direction;
		}
	}


	function renderBody()
	{
		global $CHAR_CLASS_NAMES;
		// statistics box
		$statsRows = '';
		$first = 'first';

		foreach ($this->playerStatistics as $statisticName => $statisticValue)
		{
			$statsRows .= xhtmlMpTableRow(
				xhtmlMpTableHeader( $statisticName. ":" ).
				xhtmlMpTableCell( $statisticValue,
					FALSE, '', ' width="30%"' ),
				'alt1-'. $first );
			$first = '';
		}

		$statisticsHtml = xhtmlMpTable( $statsRows, 0, '',
			'character-statistics', ' cellspacing="0" cellpadding="0"' );

		$imgHtml = 	xhtmlMpDiv(
			xhtmlMpImg( "Image.php?".
				"path=images/". $this->charClass. 
				"_profile_front.png&".
				"max_height=150",
				$this->playerName. "'s character profile" ),
			'character-portrait'
		);

		$descriptionHtml = xhtmlMpDiv(
			xhtmlMpSpan("FantasyDemo ". $CHAR_CLASS_NAMES[ $this->charClass ],
				'character-title' ).
			xhtmlMpSingleTag( 'br' ).
			"A young ". $CHAR_CLASS_NAMES[$this->charClass]. 
				", you hail from the chaotic Highlands of FantasyDemo, where
				great swords are forged from fiery craters, and great staves of
				power, imbued with powerful magics, are crafted by village
				mystics. These tools are yours to wield as you patrol the
				countryside, fighting marauding orcs and hunting giant
				spiders.",
			'character-description'
		);


		echo(
			xhtmlMpDiv(
				$imgHtml.
				$statisticsHtml.
				$descriptionHtml,
				'character'
			)
		);



	}

}

$page = new CharacterPage();

$page->render();
?>
