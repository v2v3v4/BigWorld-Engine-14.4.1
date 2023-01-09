<?php

require_once( 'AuthenticatedPage.php' );
require_once( 'Constants.php' );
class NewsPage extends AuthenticatedXHTMLMPPage
{
	var $articles;
	var $articleId;

    function NewsPage()
	{
        AuthenticatedXHTMLMPPage::AuthenticatedXHTMLMPPage( "Home" );
		if ($this->getDevice() == "Sony PlayStation Portable")
		{
			$this->styleHrefs[] = "styles/psp/news.css";
		}
		else if ($this->getDevice() ==
				"Research In Motion Ltd. BlackBerry 7100")
		{
			$this->styleHrefs[] = "styles/blackberry/news.css";

		}
		else
		{
			$this->styleHrefs[] = "styles/news.css";
		}
    }

	function initialise()
	{
		/*
		// Force logout if connection to the server is lost
		$account = $this->auth->entity();
		$playerinfo = $account->webGetPlayerInfo();
		
		if (is_string( $playerinfo ))
		{
			$this->addErrorMsg( $playerinfo );
			$_SESSION['err_msgs'][] = $playerinfo;
			$this->setRedirect( 'Disconnect.php' );
			return;
		}
		*/
		
		// we get articles from a global variable as defined in
		// Constants.php
		global $FD_ARTICLES;
		$this->articles = $FD_ARTICLES;

		// the articles could also come from an entity
		// e.g.
		// $newsagent = new RemoteEntity( "news_agent" );
		// $res = $newsagent->getNewsArticles();
		// $this->articles = $res['articles'];


		$this->articleId = -1;

		if (isset( $_REQUEST['article_id'] ))
		{
			// display this article's body
			$this->articleId = (int)($_REQUEST['article_id']);
		}
	}

    function renderBody()
	{
        $username = $_SESSION[AUTHENTICATOR_SESSION_KEY_TOKEN]
			[AUTHENTICATOR_TOKEN_KEY_USERNAME];

		$this->renderStatusMsgs();

		echo( xhtmlMpHeading( "Welcome ". $username, 2 ) );

		$sections = '';
		$rowCount = 0;
		$first = '-first';
		foreach ($this->articles as $index => $article)
		{

			$showDetailed = ($index == $this->articleId);
			$articleContents =
				xhtmlMpTag( 'a', '', '',
					' name="article-'. $index. '"' ).
				xhtmlMpDiv(
					$article['title'],
					'article-header' ).
				xhtmlMpDiv(
					$article['date'],
					'article-date' );
			if ($showDetailed)
			{
				debug( "showDetailed: $showDetailed" );
				$articleContents .= $article['body'];
			}
			else
			{
				$articleContents .= $article['summary'].
					xhtmlMpDiv(
						xhtmlMpLink(
							"News.php?article_id=$index#article-$index",
							"Read More..."
						),
						'article-readmore'
					);
			}

			$sections .= xhtmlMpDiv( $articleContents,
				'article-alt'.(string)($rowCount % 2 + 1). $first );

			++$rowCount;
			$first = '';
		}

		echo xhtmlMPDiv(
			$sections,
			'articles'
		);
    }

}

$page = new NewsPage();
$page->render();

?>
