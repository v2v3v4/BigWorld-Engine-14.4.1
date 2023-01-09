<?php

require_once('XHTML-MP-functions.php');

class MenuBar
{
	var $items;


	function MenuBar()
	{
		$this->items = array();
	}


	function addItem( $title, $href )
	{
		$this->items[$title] = $href;
	}


	function removeItem( $title )
	{
		unset( $this->items[$title] );
	}


	function contents()
	{
		$links = Array();
		foreach ($this->items as $title => $href)
		{
			$title = str_replace( ' ', '&nbsp;', $title );
			$links[] = xhtmlMpLink( $href, $title );
		}
		return xhtmlMpDiv(
			implode(' &middot;',  $links ),
			'menubar'
		);

	}
}

?>
