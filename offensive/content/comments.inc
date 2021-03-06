<?php
	// Include, and check we've got a connection to the database.
	require_once("admin/mysqlConnectionInfo.inc");
	if(!isset($link) || !$link) $link = openDbConnection();
	require_once("offensive/assets/functions.inc");
	require_once("offensive/assets/tabs.inc");
	require_once("offensive/assets/comments.inc");
	require_once("offensive/assets/classes.inc");

	function additionalCssFor($vote) {
		if(strpos($vote, 'bad') > 0) return " bad";
		if(strpos($vote, 'good') > 0) return " good";
		return "";
	}

	function title() {
		global $upload;
		return "[discuss] : " . $upload->filename();
	}

	function start() {
		global $upload, $uploader;
		mustLogIn();
		
		if( ! is_intger( $_REQUEST['fileid'] ) ){
			header( "Location: ./" );
			exit;
		}
		
		$upload = core_getupload($_REQUEST['fileid']);
		$uploader = $upload->uploader();

		clearSubscription($upload->id(), me()->id());

		// update pickuplink for discussions
		if($upload->type() == "topic") {
			PickupLink::content($upload->type());
			PickupLink::update($upload);
		}

	
	  // check to see if someone's posting.  
	  /* check if someone is being hoodwinked into voting by link.
	   * NB: only works if the client is reporting REFERERS. 
	   */
	  if($upload->type() != 'topic' &&
		 array_key_exists("submit", $_REQUEST) && array_key_exists("HTTP_REFERER", $_SERVER) &&
	     $_SERVER['HTTP_REFERER'] !== "" && strpos($_SERVER['HTTP_REFERER'], $_SERVER['SERVER_NAME']) === false) {
		echo "you've been had.";
		trigger_error("off-site tried to vote on non-topic upload, referer: ".$_SERVER['HTTP_REFERER'], E_USER_WARNING);
		return;
	  }
	  
	  // quietly snub them if they are not an admin and they are trying to post to the changeblog
	  if(array_key_exists("submit", $_REQUEST) && $upload->id() == "211604" && me()->status() != "admin") {
	      return;
	  }
	  
		if( array_key_exists("submit", $_POST)) {
			$fileid		 = array_key_exists("fileid", $_POST) ?
							$_POST['fileid'] : '';
			$comment	 = array_key_exists("comment", $_POST) ?
							trim($_POST['comment']) : "";
			$vote		 = array_key_exists("vote", $_POST) ?
							$_POST['vote'] : "";
			$offensive	 = array_key_exists("offensive", $_POST) && $_POST['offensive'] != "" ?
							1 : 0;
			$repost		 = array_key_exists("repost", $_POST) 	&& $_POST['repost'] != "" ?
							1 : 0;
			$subscribe	 = array_key_exists("subscribe", $_POST) && $_POST['subscribe'] != "" ?
							1 : 0;
			
			$commentid = postComment($fileid, $vote, $repost, $offensive, $comment, $subscribe);

			if($commentid === false) {
				echo "you've been had.";
				return;
			}

			if( array_key_exists("redirect", $_REQUEST) ) {
				header("Location: ".Link::upload($_REQUEST['fileid']));
				exit;
			}
			
			// redirecting to the same place prevents 'reload' from reposting the comment.
			header("Location: ".(is_numeric($commentid) ? Link::comment($commentid) : Link::thread($_REQUEST['fileid'])));
			exit;
		}
	}

	function head() {
?>
	<script type="text/javascript" src="/offensive/js/subscriptions.js"></script>

	<script type="text/javascript">
		$(document).ready(function() {
			$("#subscribeLink").click(function(e) {
				handle_subscribe($(this),e,$("#fileid").attr("value"));
			});
			$("#unsubscribeLink").click(function(e) {
				handle_subscribe($(this),e,$("#fileid").attr("value"));
			});
		});
	</script>
		
<?
	}
	
	function toggleThumbnailLink() {
		?><div style="margin:4px;font-weight:bold"><?
			if(me()->getPref("thumbnails_in_comments") == 1) {
				?><a href="<?= Link::setPref("thumbnails_in_comments", 0) ?>">hide thumbnail</a><?
			}
			else {
				?><a href="<?= Link::setPref("thumbnails_in_comments", 1) ?>">show thumbnail</a><?
			}
		?></div><?

	}

	function body() {
		global $upload, $uploader;
	
		$subscribed = subscribed($upload->id());

		$comments_exist = count($upload->getComments()) > 0;
	
		$comments_heading = "the dorks who came before you said:";
		$add_comment_heading = $comments_exist ? "and then you came along and were all:" : "you were first on the scene and were all:";

		$href = Link::upload($upload);
?>	

<div class="contentbox">
	<div class="blackbar"></div>
		<div class="heading"><?
    // changeblog special header
	if($upload->id() == "211604") {
		?>
		changeblog
		<br/><span style="color:#666699">beat to fit. paint to match.</span><br/>
		<?
	} else if( $upload->type() == 'topic' ) {
		$prefix = $uploader->status() == "admin" ? "" : "don't blame me,";

		echo htmlEscape($upload->filename());

		?><br/><span style="color:#666699"><?= $prefix ?> <?= $uploader->htmlUsername() ?> started it.</span><br/>
<?
	} else {
?>
		<a class="heading" id="pic" href="<?= $href ?>"><?= $upload->htmlFilename() ?></a><br/><span style="color:#666699">uploaded by <?= $uploader->htmlUsername() ?></span><br/>
<?
	}	
?></div>
	
<?
		global $activeTab;
		$activeTab = "discussions";
		tabs();
?>
	
		<div id="commentList" class="bluebox" style="text-align:left">	

<?php 

	$commentnum = 0;
	if( $comments_exist > 0) { 
        // changeblog doesn't say anything demeaning about the people who posted.
        if($upload->id() != "211604") { 
			echo "<b>$comments_heading</b>";
        }

		global $warudo;
		$numgood = 0;
		$numbad = 0;
		foreach($upload->getComments() as $comment) {
			$commenter = $comment->commenter();
		?>
		<a name="<?= $comment->id() ?>"></a>
		<div class="entry u<?= $commenter->id() ?><?
			if($comment->vote() == "this is good") echo " tig";
			if($comment->vote() == "this is bad") echo " tib";
			if($comment->tiar()) echo " tiar";
			if($comment->tmbo()) echo " tmbo";
			if(strlen($comment->text())) echo " comment";
		?>" style="<?= nextStyle($commentnum++)?>">
				<?= $comment->HTMLcomment(); ?>
				<br />
				<div class="timestamp"><a href="<?= Link::comment($comment) ?>"><?= $comment->timestamp() ?></a></div>
				
				&raquo; <?= $commenter->htmlUsername() ?>
				
				<?
							
				if( $comment->vote() ) {
					$thevote = $comment->vote();
    	
					$numgood += ($thevote == 'this is good') ? 1 : 0;
					$numbad += ($thevote == 'this is bad') ? 1 : 0;
    	
					echo "<span title=\"+$numgood -$numbad\" class='vote" . additionalCssFor( $thevote ) . "'> [ " . $thevote . " ]</span>";
				}
				
				
				if( $comment->tmbo() == 1 ) {
					?><span class="vote"> [ this might be offensive ]</span><?php
				}
				
				if( $comment->tiar() == 1 ) {
					?><span class="vote"> [ this is a repost ]</span><?php
				}
    	
				?>
		</div>
		
		<?php 
		// end of comments loop
		} 
	
	// end of "are there any comments"
	}
	
		// thumbnail
		if( $upload->type() != 'topic' && ($upload->type() != "audio" || $upload->thumb())) {
			?>
				<div style="background:#ccccff; border-top:1px solid #000033">
					<div style="float:right"><? toggleThumbnailLink() ?></div>
					<?
						if(me()->getPref("thumbnails_in_comments") == 1) {
							?>
							
							<a href="<?= $href ?>"
								<? if($upload->filtered()) { ?>
									onMouseOver='changesrc("th<?= $upload->id()?>","<?= $upload->thumbURL() ?>")'
							 		onMouseOut='changesrc("th<?= $upload->id() ?>","/offensive/graphics/th-filtered.gif")'
								<? } ?>
							><img name="th<?= $upload->id()?>" style="margin:12px;"
								src="<?= $upload->filtered()
									? "/offensive/graphics/th-filtered.gif" 
									: $upload->thumbURL() ?>" border="0"
							/></a>
							
							<?
						}
					?>
					
				</div>
				<div style="clear:both">
				</div>
			<?
		}

	?>
	
		

			</div>
			<div class="heading">
				<div style="float:right">					
					<? if( $subscribed ) { ?>
						<b><a id="unsubscribeLink" href="subscribe.php?un=1&fileid=<?= $upload->id() ?>" title="take this thread off my 'unread comments' watch list." class="orange">unsubscribe</a></b>
					<? }
						else { ?>
						<b><a id="subscribeLink" href="subscribe.php?fileid=<?= $upload->id() ?>" title="watch this thread for new comments." class="orange">subscribe</a></b>
					<?	}
					?>
				</div>

					<div>
					<? if ($upload->type() != "topic") { ?>
						<a class="heading" id="pic" href="<?= Link::upload($upload) ?>"><?= $upload->htmlFilename(); ?></a><br/>
					<? }
						else {
						 ?>&nbsp;<?
						}
					?>
					</div>

			</div>
	</div>

<?php
    // changeblog shouldn't tease the users with a comment field
    if(canComment($upload->id()) && ($upload->id() != "211604" || me()->status() == "admin")) {
?>

	<div class="contentbox">
		<div class="blackbar"></div>
			<div class="heading"><?= $add_comment_heading?></div>
			<div class="bluebox" style="text-align:center">			
			<a name="form"></a>
			<form method="POST" action="<?php echo $_SERVER['PHP_SELF']?>">

			
				<p>
					<input type="hidden" id="fileid" name="fileid" value="<?= $upload->id() ?>"/>
					<input type="hidden" name="c" value="comments"/>
					<textarea name="comment" rows="12" cols="64"></textarea>
				</p>
				


<?php 

	if( canVote($upload->id()) && $upload->file() && $upload->type() != 'topic' ) {
	
	?>	<div style="text-align:left;margin-left:10%">
			<input type="radio" name="vote" value="" checked="checked" id="novote"/>
			<br/>			
	<?php

		// show vote options	
		// oh jesus why are we doing this /o\
		$sql = "SHOW COLUMNS FROM offensive_comments LIKE 'vote'";
		$result = tmbo_query( $sql );
		$row = mysql_fetch_row($result);
		$options = explode("','",preg_replace("/(enum|set)\('(.+?)'\)/","\\2",$row[1]));
		
		
		foreach( $options as $option ) {
			?>
				<input type="radio" name="vote" value="<?php echo $option ?>" id="<?php echo $option?>"/>
				<label for="<?php echo $option?>">[ <?php echo $option?> ]</label><br/>
			
			<?php
		}
	?>
	
		<br/>

		<input type="checkbox" name="offensive" value="omg" id="tmbo"/>
		<label for="tmbo">[ this might be offensive ]</label><br/>
		
		<input type="checkbox" name="repost" value="police" id="repost"/>
		<label for="repost">[ this is a repost ]</label><br/>

		<br/>

		<input type="checkbox" name="subscribe" value="subscribe" id="subscribe"/>
		<label for="subscribe">[ subscribe ]</label><br/>


</div>

	<?php

	}
?>

						
				<p>
					<input type="submit" name="submit" value="go"/>
				</p>
				
				</form>
			</div>
			<div class="blackbar"></div>
		</div>
			

<?
    }
}

	function nextStyle($i) {
		global $style;
	    $style = $i % 2 ? "background:#bbbbee;" : "background:#ccccff";
		return $style;
	}


?>
