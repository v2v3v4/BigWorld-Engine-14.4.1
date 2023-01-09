<?php
/**
 * Functions for printing XHTML-MP.
 */

$XHTML_BLOCK_ELEMENTS = Array(
	"p", "div",
	"h1", "h2", "h3", "h4", "h5", "h6",
	"blockquote", "pre", "br", "hr",
	"link", "head", "body", "script", "style",
	"title", "ul", "ol", "li"
);


function xhtmlMpMouseHighlightRow( $cellContents, $baseClass, $highlightClass )
{
	$attrString = '';

	$attrString = xhtmlMpAddAttribute( $attrString, "onMouseOver",
		'this.className = \''. $highlightClass.'\';'
	);

	$attrString = xhtmlMpAddAttribute( $attrString, "onMouseOut",
		'this.className = \''. $baseClass.'\';'
	);

	return xhtmlMpTableRow( $cellContents, $baseClass, $attrString );
}


function xhtmlMpTable($rowsContent, $border='', $width='', $class='',
		$attrString='')
{
	if (strlen($border))
	{
		$attrString = xhtmlMpAddAttribute( $attrString, "border", $border );
	}
	if (strlen( $width ))
	{
		$attrString = xhtmlMpAddAttribute( $attrString, "width", $width );
	}
	return xhtmlMpTag( 'table', $rowsContent, $class, $attrString ). "\n";
}


function xhtmlMpTableHeader( $cellsContent, $class='', $attrString='' )
{
	return xhtmlMpTag( 'th', $cellsContent, $class, $attrString ). "\n";
}


function xhtmlMpTableRow( $cellsContent, $class='', $attrString='' )
{
	return xhtmlMpTag( 'tr', $cellsContent, $class, $attrString )."\n";
}


function xhtmlMpTableCell( $content, $nowrap=FALSE, $class='', $attrString='' )
{
	if ($nowrap)
	{
		$attrString = xhtmlMpAddAttribute( $attrString, "nowrap", "nowrap" );
	}
	return xhtmlMpTag( 'td', $content, $class, $attrString ). "\n";
}


/**
 * Returns the output for a heading.
 */
function xhtmlMpHeading( $text, $num=1, $class='', $attrString='' )
{
    return xhtmlMpTag( "h".(string)($num), $text, $class, $attrString );
}

/**
 * Returns the output for a link.
 */
function xhtmlMpLink( $href, $contents, $class='', $attrString='' )
{
    $attrString = xhtmlMpAddAttribute ( $attrString, 'href', $href );
    return xhtmlMpTag( 'a', $contents, $class, $attrString );
}

function xhtmlMpImg( $href, $alt, $width='', $height='', $class='',
		$attrString='' )
{
    $attrString = xhtmlMpAddAttribute( $attrString, 'src', $href );
	$attrString = xhtmlMpAddAttribute( $attrString, 'alt', $alt );
    if ($height != '')
	{
        $attrString = xhtmlMpAddAttribute( $attrString, 'height', $height );
    }
    if ($width != '')
	{
        $attrString = xhtmlMpAddAttribute( $attrString, 'width', $width );
    }
    return xhtmlMpSingleTag( 'img', $class, $attrString );
}

class XHTMLMPForm
{
	var $contents = '';
	var $submitValue = 'Submit';
	var $action = FALSE;
	var $method = 'post';
	var $hiddens = array();
	var $submitName = 'submit';
	var $className = '';
	var $attrString = '';

	function XHTMLMPForm()
	{
		// constructor
	}

	function setContents( $contents )
	{
		$this->contents = $contents;
	}

	function setSubmitValue( $submitValue )
	{
		$this->submitValue = $submitValue;
	}

	function setSubmitName( $submitName )
	{
		$this->submitName = $submitName;
	}

	function setAction( $action )
	{
		$this->action = $action;
	}

	function setMethod( $method )
	{
		switch ($method)
		{
			case "post":
			case "get":
				$this->method = $method;
			break;

			default:
				trigger_error(
					E_ERROR,
					"XHTMLMPForm::setMethod: method is not either ".
					"get/post: $method" );
		}
	}

	function addHidden( $name, $value )
	{
		$this->hiddens[$name] = $value;
	}

	function setClassName( $className )
	{
		$this->className = $className;
	}

	function setAttrString( $attrString )
	{
		$this->attrString = $attrString;
	}

	function output()
	{
		return xhtmlMpForm( $this->contents,
			$this->action,
			$this->method,
			$this->hiddens,
			$this->submitName,
			$this->className,
			$this->attrString
		);
	}
}


function xhtmlMpForm( $contents, $action='',
		$method="post", $hiddens=array(), $submitName='submit',
		$class='', $attrString='' )
{
	$attrString = xhtmlMpAddAttribute( $attrString, 'method', $method );
	if ($action)
	{
		$attrString = xhtmlMpAddAttribute( $attrString, 'action', $action );
	}
	foreach ($hiddens as $key => $value )
	{
		$contents .= xhtmlMpFormInput( 'hidden', $key, $value ). "\n";
	}
	return xhtmlMpTag( "form", "\n". $contents, $class, $attrString ). "\n";
}


function xhtmlMpFormInputRadio( $name, $value, $selected=FALSE,
		$class='', $attrString='' )
{
	if ($selected)
	{
		$attrString = xhtmlMpAddAttribute( $attrString, 'checked', "true" );
	}
	return xhtmlMpFormInput( 'radio', $name, $value, $class, $attrString );
}


function xhtmlMpFormInputRadioList( $name, $values, $default= FALSE,
		$class='', $attrString='' )
{
	$contents = '';

	$attrString = xhtmlMpAddAttribute( $attrString, 'name', $name );

	foreach( $values as $key => $valueName )
	{
		if ($default !== FALSE && $default == $key)
		{
			$optionAttrString = xhtmlMpAddAttribute( $attrString, 'checked', 'checked' );
		}
		else
		{
			$optionAttrString = '';
		}

		$contents .= xhtmlMpFormInput( 'radio', $name, $key, '', $optionAttrString ).
			$valueName. xhtmlMpSingleTag( 'br' );
	}
	return xhtmlMpDiv( $contents, $class, $attrString );
}

function xhtmlMpFormInputPassword( $name, $value='', $size='', $class='',
		$attrString='' )
{
	if ($size != '')
	{
		$attrString = xhtmlMpAddAttribute( $attrString, "size", $size );
	}
	return xhtmlMpFormInput( "password", $name, $value, $class, $attrString );
}

function xhtmlMpFormInputText( $name, $value='', $size='', $class='',
		$attrString='' )
{
	if ($size != '')
	{
		$attrString = xhtmlMpAddAttribute( $attrString, "size", $size );
	}
	return xhtmlMpFormInput( "text", $name, $value, $class, $attrString );
}

function xhtmlMpFormSubmit( $name, $value, $class='', $attrString='' )
{
	return xhtmlMpFormInput( 'submit', $name, $value, $class, $attrString );
}

function xhtmlMpFormInput( $type, $name, $value='', $class='', $attrString='' )
{
	$attrString = xhtmlMpAddAttribute( $attrString, 'type', $type );
	if (strlen( $name ))
	{
		$attrString = xhtmlMpAddAttribute( $attrString, 'name', $name );
	}
	if (strlen( $value ))
	{
		$attrString = xhtmlMpAddAttribute( $attrString, 'value', $value );
	}
	return xhtmlMpSingleTag( "input", $class, $attrString );
}

/**
 * Returns the output for a paragraph.
 */
function xhtmlMpPara( $text, $class='', $attrString='' )
{
    return xhtmlMpTag( "p", $text, $class, $attrString );
}

/**
 * Returns the output for a paragraph.
 */
function xhtmlMpCheckBox( $name, $checked=false, $class='', $attrString='' )
{
    if ($checked)
	{
        $attrString = xhtmlMpAddAttribute( $attrString, 'checked', 'checked' );
    }
    $attrString = xhtmlMpAddAttribute( $attrString, 'name', $name );
	$attrString = xhtmlMpAddAttribute( $attrString, 'type', 'checkbox' );
	$attrString = xhtmlMpAddAttribute( $attrString, 'value', '1' );
    return xhtmlMpSingleTag( "input", $class, $attrString );
}

/**
 * Returns the output for a DIV element.
 */
function xhtmlMpDiv( $text, $class='', $attrString='' )
{
    return xhtmlMpTag( "div", $text, $class, $attrString );
}

function xhtmlMpSpan( $text, $class='', $attrString='' )
{
    return xhtmlMpTag( "span", $text, $class, $attrString );
}

function xhtmlMpList( $arrListContents, $ordered=false, $class='',
		$attrString='' )
{
    $contents = '';
    for ($i = 0; $i < count( $arrListContents ); ++$i)
	{
        $contents .= xhtmlMpTag( "li", $arrListContents[$i] );
    }
    if ($ordered)
	{
        return xhtmlMpTag( "ol", $contents, $class, $attrString );
    }
    else
	{
        return xhtmlMpTag( "ul", $contents, $class, $attrString );
    }
}


function xhtmlMpTag( $tagName, $contents, $className='', $attrString='' )
{
    if ($className != '')
	{
		$attrString = xhtmlMpAddAttribute( $attrString, "class", $className );
    }

    if (in_array( $tagName, $GLOBALS['XHTML_BLOCK_ELEMENTS'] ))
	{
		$out = "<". $tagName. ($attrString !== "" ? " $attrString":""). ">\n". $contents.
			"</".$tagName.">\n";
	}
	else
	{
		$out = "<". $tagName. ($attrString !== "" ? " $attrString":""). ">". $contents.
			"</".$tagName.">";
	}
	return $out;
}


/**
 *	Return a a single XHTML element (no enclosures
 *	attribute string that adds the given key-value pair.
 *
 *	@param attributeString	the given attribute string, or empty for a new one
 *	@param key				the given key value
 *	@param value			the given value string
 *	@return					the modified attribute string
 */
function xhtmlMpSingleTag( $tagName, $className='', $attrString='' )
{
    if ($className != '')
	{
		$attrString = xhtmlMpAddAttribute( $attrString, "class", $className );
	}
	if (in_array( $tagName, $GLOBALS['XHTML_BLOCK_ELEMENTS'] ))
	{
		$out = "<". $tagName. ($attrString !== "" ? " $attrString":""). "/>\n";
	}
	else
	{
		$out = "<". $tagName. ($attrString !== "" ? " $attrString":""). "/>";

	}
    return $out;
}


/**
 *	Return a modified XHTML element attribute string based on the given
 *	attribute string that adds the given key-value pair.
 *
 *	@param attributeString	the given attribute string, or empty for a new one
 *	@param key				the given key value
 *	@param value			the given value string
 *	@return					the modified attribute string
 */
function xhtmlMpAddAttribute( $attributeString, $key, $value )
{
	if ($attributeString == '')
	{
		return "{$key}=\"{$value}\"";
	}
    return $attributeString. ' '. $key. '="'. $value . '"';
}

?>
