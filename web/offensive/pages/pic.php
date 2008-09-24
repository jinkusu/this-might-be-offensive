<?php 
	set_include_path("../..");
	require_once("offensive/assets/header.inc");
	require_once("offensive/assets/functions.inc");
	require_once( 'admin/mysqlConnectionInfo.inc' );
	if(!isset($link) || !$link) $link = openDbConnection();
	require_once("offensive/assets/classes.inc");
	require_once("offensive/assets/comments.inc");

	mustLogIn();
	
	$me = new User($_SESSION["userid"]);

	$id = "";
	if(array_key_exists("random", $_REQUEST)) {
		$id = get_random_id();
	}
	if(array_key_exists("id", $_REQUEST)) {
		$id = $_REQUEST["id"];
	}
	
	if(!is_numeric($id)) {
		header( "Location: /offensive/" );
	}

	$upload = new Upload($id);
	if($upload->type() == "topic") {
		header("Location: /offensive/?c=comments&fileid=".$id);
	}

	// LEGACY: moves from cookie-based pickuplink to DB-based pickuplink for images.
	$cookiename = $_SESSION['userid'] . "lastpic";
	if(array_key_exists($cookiename, $_COOKIE) && is_numeric($_COOKIE[$cookiename])) {
		if($me->getPref("ipickup") === false || $me->getPref("ipickup") < $_COOKIE[$cookiename]) {
			$me->setPref("ipickup", $_COOKIE[$cookiename]);
		}
		setcookie( $cookiename, "", time()-3600, "/offensive/" );
	}
	
	if($me->getPref("ipickup") < $id) {
		$me->setPref("ipickup", $id);
	}

	function get_random_id() {
		$sql = "SELECT id FROM offensive_uploads WHERE type='image' AND status='normal' ORDER BY RAND() LIMIT 1";
		$res = tmbo_query($sql);
		$row = mysql_fetch_assoc( $res );
		return($row['id']);
	}

	function getFileSize( $fpath ) {
		$k = "";
		if( file_exists( $fpath ) ) {
			$size = filesize( $fpath );
			$k = byte_format($size);
		}
		return $k;
	}

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
   "http://www.w3.org/TR/html4/loose.dtd">

<html>
	<head>
		<meta http-equiv="Content-Type" content="text/html;charset=UTF-8">
		<META NAME="ROBOTS" CONTENT="NOARCHIVE" />
		<title>[<?= $upload->type() ?>] : <?= $upload->filename() ?> </title>
		<link rel="stylesheet" type="text/css" href="styles.php"/>
		<script type="text/javascript">
			self.file_id = "";
			
			// prevent sites from hosting this page in a frame;
			if( window != top ) {
				top.location.href = window.location.href;
			}
		</script>
		<script type="text/javascript" src="/offensive/js/jquery-1.2.6.min.js"></script>
		<!-- XXX: a lot of this picui stuff is going to have to move into this header so it can be customized -->
		<script type="text/javascript" src="/offensive/js/picui.js"></script>
		<script type="text/javascript" src="/offensive/js/subscriptions.js"></script>
		<script type="text/javascript" src="/offensive/js/jqModal.js"></script>
		<script type="text/javascript" src="/offensive/js/jqDnR.js"></script>

	</head>
	<body>
	<!-- message -->
	<div style="white-space:nowrap;overflow:hidden;padding:3px;margin-bottom:0px;background:#000033;color:#ff6600;font-size:10px;font-weight:bold;padding-left:4px;">
		<div id="instruction_link" style="float:right;"><a href="#" style="color:#ff6600">?</a></div>
		<div>consciousness doesn't really exist. it's just another one of our ideas.</div>
	</div>
	<div id="instructions" style="display:none;white-space:nowrap;overflow:hidden;padding:3px;margin-bottom:6px;background:#cccccc;color:#333333">← = newer. ↑ = index. → = older. ↓ = comments . + or = votes [ this is good ]. - votes [ this is bad ] .<br />
q = quick comment, Esc closes quick comment box, ? = random image.<br />
(because clicking is too hard.)</div>

	<!-- this window is not visible unless you do a quick comment -->
	<!-- data is fetched using ajax in js and put in #qc_bluebox  -->
	<div class="jqmWindow" id="dialog">
		<div class="blackbar"></div>
		<div class="heading"><table style="width: 100%;"><tr>
			<td align="left">and then you came along and were all:</td>
			<td class="qc_close" align="right"><a href="#" class="jqmClose">Close</a></td>
		</tr></table></div>
		<div class="bluebox" id="qc_bluebox" style="text-align: center">
		</div>
	</div> <!-- end quickcomment -->
	<div id="content">
		<div id="heading">

			&nbsp;&nbsp;

				<?
				
				/*
				 * navigation buttons, prev index next are dependant on type
				 */
				if($upload->type() == 'avatar') {?>
					<a href="../" id="next" style="visibility:hidden">newer</a> . <a id="index" href="/offensive/">index</a> . <a id="previous" href="../" style="visibility:hidden">older</a>
				<?
				} else {
					if($upload->next()) { ?>
						<a id="next" href="<?= $_SERVER['PHP_SELF'] ?>?id=<?= $upload->next()->id() ?>">newer</a>
					<? } else { ?>
						<a href="../" id="next" style="visibility:hidden">newer</a>
					<? } ?>
					. <a id="index" href="/offensive/">index</a> .
					<? if($upload->prev()) {?>
						<a id="previous" href="<?= $_SERVER['PHP_SELF'] ?>?id=<?= $upload->prev()->id() ?>">older</a>
					<? } else { ?>
						<a id="previous" href="../" style="visibility:hidden">older</a>
					<?}
				} ?>
				
				<!--
					comment block
				-->
				<a style="margin-left:48px;"
				   id="comments"
				   href="/offensive/?c=comments&fileid=<?= $upload->id() ?>">comments</a>
				(<span id="count_comment"><?= $upload->comments() ?></span>c
				+<span id="count_good"><?= $upload->goods() ?></span>
				-<span id="count_bad"><?= $upload->bads() ?></span><?
				if($tmbo > 0) { ?>
					<span style=\"color:#990000\">x<?= $upload->tmbos ?></span>";
				<? } ?>)
				&nbsp;(<a id="quickcomment" class="jqModal" href="#">quick</a>)

				<!--
					voting block
				-->
				<span style="margin-left:48px;">
					<?
					if(canVote($upload->id())) {
						$good_href = "href=\"/offensive/?c=comments&submit=submit&fileid=$id&vote=this%20is%20good&redirect=true\"";	
						$bad_href = "href=\"/offensive/?c=comments&submit=submit&fileid=$id&vote=this%20is%20bad&redirect=true\"";
						$class = "on";
					} else {
						$good_href = $bad_href = "";
						$class = "off";
					}
						
					?>
					<span id="votelinks" class="<?= $class ?>">
						vote: <a name="<?= $upload->id() ?>" id="good" class="votelink" <?= $good_href ?>>[ this is good ]</a> .
						<a name="<?= $upload->id() ?>" id="bad" class="votelink" <?= $bad_href ?>>[ this is bad ]</a>
					</span>
				</span>

				<!--
					subscribe block
				-->
				<span style="margin-left:48px;">
				<?	
					if(subscribed($upload->id())) { ?>
						<a id="unsubscribeLink" href="/offensive/subscribe.php?un=1&fileid=<?= $id ?>" title="take this file off my 'unread comments' watch list.">unsubscribe</a>
					<?	} else { ?>
						<a id="subscribeLink" href="/offensive/subscribe.php?fileid=<?= $id ?>" title="watch this thread for new comments.">subscribe</a>
					<?	} ?>
				</span>
				
				<!--
					filter block
				-->
				<!-- XXX: these are going away soon -->
				<span style="margin-left:48px;">nsfw filter: <?
					if($me->getPref("hide_nsfw") == 1) { ?>
						<a href="/offensive/setPref.php?p=hide_nsfw&v=">off</a> on
					<? } else { ?>
						off <a href="/offensive/setPref.php?p=hide_nsfw&v=1">on</a>
					<? } ?>
				</span>
						
				<span style="margin-left:48px;">tmbo filter: <?
					if($me->getPref("hide_tmbo") == 1) { ?>
							<a href="/offensive/setPref.php?p=hide_tmbo&v=">off</a> on
					<? } else { ?>
							off <a href="/offensive/setPref.php?p=hide_tmbo&v=1">on</a>
					<? } ?>
				</span>
			</div>

			<!--
				filename/size block
			-->
			<br /><br />
			<?= $upload->htmlFilename() ?> <span style="color:#999999"><? 
				if($upload->file() != "")
					echo getFileSize($upload->file());
			?></span>
			<br/>
			
			<!--
				username/time block
			-->
			<span style="color:#999999">
				uploaded by <a id="userLink" href="../?c=user&userid=<?= $upload->uploader()->id() ?>"><?= $upload->uploader()->username() ?></a> @ <?= $upload->timestamp() ?>
			</span>
			
			<!--
				squelch block
			-->
			<span style="margin-left:48px">
				<?
				if($me->squelched($upload->uploader()->id())) {
					?><a id="unsquelchLink" style="color:#999999" href="/offensive/setPref.php?unsq=<?= $upload->uploader()->id() ?>">unsquelch <?= $upload->uploader()->username() ?></a><?
				} else {
					?><a id="squelchLink" style="color:#999999" href="/offensive/setPref.php?sq=<?= $upload->uploader()->id() ?>">squelch <?= $upload->uploader()->username() ?></a><?
				}
				?>
			</span>
			<br/><br/>
			
			<!--
				image block
			-->
			<? if($me->squelched($upload->uploader()) || 
			    ($upload->is_nsfw() == 1 && $me->getPref("hide_nsfw") == 1) || 
			    ($upload->is_tmbo() == 1 && $me->getPref("hide_tmbo") == 1)) {
				?><div style="padding:128px;">[ filtered ] <!-- <?= $upload->uploader()->id() ?> --></div><?
			} else { ?>
				<div class="<?php echo $is_nsfw == 1 ? 'nsfw' : 'image' ?> u<?= $upload->uploader()->id() ?>">
					<? if($upload->file() != "") { ?>
						<a id="imageLink" href="<?= $upload->URL() ?>" target="_blank"><img src="<?= $upload->URL() ?>" style="border:none"/></a>
					<? } else { ?>
						<div style="padding:128px;">[ got nothin' for ya ]</div>
					<? } ?>
				</div>
			<? } ?>
			<br/><br/>
		</div>

		<? record_hit();
		include_once("analytics.inc"); ?>
	</body>
</html>