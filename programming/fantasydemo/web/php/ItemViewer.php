<?php

require_once( 'XHTML-MP-functions.php' );
require_once( 'Constants.php' );
require_once( 'Items.php' );

class ItemViewer
{
	var $itemId;
	var $className;
	var $itemTypeName;
	var $largeImageUrl;
	var $descriptionHtml;
	var $statisticsHtml;

	function ItemViewer( $itemId, $classNameBase = "item-detail" )
	{
		$this->itemId = $itemId;
		$this->classNameBase = $classNameBase;

		$this->itemTypeName = FALSE;
		$this->largeImageUrl = FALSE;
		$this->descriptionHtml = FALSE;
		$this->statisticsHtml = FALSE;
	}

	function acquireInfo( $itemType )
	{
		$itemTypeMgr =& ItemTypeManager::instance();
		$itemType =& $itemTypeMgr->itemTypes[$itemType];
		// based on itemType
		// somehow fill $this->largeImageUrl, $this->descriptionHtml,
		// $this->itemTypeName, $this->statisticsHtml
		$this->itemTypeName = $itemType->name;
		$this->largeImageUrl = $itemType->detailImgUrl;
		$this->descriptionHtml = $itemType->description;


		$rows = '';
		$rowNum = 0;
		$first = '-first';
		foreach ($itemType->stats as $statName => $statValue)
		{
			$rows .= xhtmlMpTableRow(
				xhtmlMpTableHeader(
						$statName. ":" ).
					xhtmlMpTableCell(
						$statValue ),
				"alt". (string)($rowNum % 2 + 1). $first
			);
			++$rowNum;
			$first = '';
		}
		$this->statisticsHtml = xhtmlMpTable( $rows, 0, '',
			'item-detail-statistics' );

	}

	function output()
	{

		$imageUrl = $this->largeImageUrl;

		if (isset( $_SESSION['display'] ) and
				$_SESSION['display']['resolution_width'])
		{
			$imageUrl = "Image.php?".
				"path=". $this->largeImageUrl. "&".
				'max_width='.
					(string)( 0.75 *
						(float)$_SESSION['display']['resolution_width']
					);
		}


		$contents =
			xhtmlMpTag( 'a', '', '', ' name="'. $this->classNameBase. '"' ).
			xhtmlMpDiv(
				xhtmlMpImg( $imageUrl, $this->itemTypeName ),
				$this->classNameBase. "-image" );
		$contents .= xhtmlMpDiv( $this->itemTypeName,
			$this->classNameBase. '-title' );

		if ($this->statisticsHtml)
		{
			$contents .= xhtmlMpDiv( $this->statisticsHtml,
				$this->classNameBase. "-stat" );
		}

		$contents .= xhtmlMpDiv( $this->descriptionHtml,
			$this->classNameBase. "-description" );


		$contents .= xhtmlMpDiv(
			xhtmlMpLink( '#top', 'top' ),
			$this->classNameBase. '-backtotop'
		);


		return xhtmlMpDiv( $contents, $this->classNameBase );
	}
}

?>
