<?php 

	set_include_path("../..");
	require_once("offensive/assets/header.inc");
	require_once("offensive/assets/functions.inc");

	mustLogIn();

	$id = $_REQUEST['id'];
	if( ! is_numeric( $id ) ) {
		header( "Location: ../" );
	}

	$cookiename = $_SESSION['userid'] . "lastpic";

	$lastpic = array_key_exists($cookiename, $_COOKIE) ? $_COOKIE[ $cookiename ] : "";
	
	if(!$readonly && (!is_numeric( $lastpic ) || $id > $lastpic)) {
		setcookie( $cookiename, "$id", time()+3600 * 24 * 365, "/offensive/" );
	}

	require_once( 'admin/mysqlConnectionInfo.inc' );
	if(!isset($link) || !$link) $link = openDbConnection();
	require_once('offensive/assets/getPrefs.inc');
	require_once('offensive/assets/functions.inc');
	
	$id = $_REQUEST['id'];

	function thisOrZero( $value ) {
		return (is_numeric( $value ) ? $value : 0);
	}


	function writeNav( $id ) {
	
		global $filename, $is_nsfw, $is_tmbo, $uploader, $uploaderid, $timestamp, $year, $month, $day, $nextid, $previd;
		
		$sql = "SELECT offensive_uploads.*, users.username, users.userid,
					(select min( id ) from offensive_uploads where id > $id AND type='image' and status='normal') as nextid,
					(select max( id ) from offensive_uploads where id < $id AND type='image' and status='normal') as previd
				FROM offensive_uploads, users
				WHERE id = $id 
					AND offensive_uploads.userid = users.userid
					AND type='image'
				LIMIT 1";
#					AND type='image' AND users.account_status != 'locked'
					
		$result = tmbo_query( $sql );
		$row = mysql_fetch_assoc( $result );
			
		$filename = $row['filename'];
		$nextid = $row['nextid'];
		$is_nsfw = ( $row['nsfw'] == 1 || strpos( $filename, "nsfw" ) || strpos( $filename, "NSFW" ) );
		$previd = $row['previd'];		
		$is_tmbo = $row['tmbo'];
		$uploader = $row['username'];
		$uploaderid = $row['userid'];
		$time = strtotime( $row['timestamp'] );
		$year = date( "Y", $time );
		$month = date( "m", $time );			
		$day = date( "d", $time );
		$timestamp = date( "Y-m-d h:i:s a", strtotime( $row['timestamp'] ) );
			

	}

	function fileNav( $nextid, $previousid, $uploader_id, $uploader_name ) {
		if( isset( $nextid ) ) {
		 ?>
			<a id="next" href="<? echo $_SERVER['PHP_SELF']?>?id=<?= $nextid ?>">newer</a>
		<? } 
		else {
			?><a href="../" id="next" style="visibility:hidden">newer</a><?
		} ?>
		 . <a id="index" href="/offensive/">index</a> .
		 <? if( isset( $previousid ) ) { ?>
			<a id="previous" href="<? echo $_SERVER['PHP_SELF']?>?id=<?= $previousid ?>">older</a>
		<? } 
	}

	function getFileSize( $fpath ) {
		$k = "";
		if( file_exists( $fpath ) ) {
			$size = filesize( $fpath );
			$k = byte_format($size);
		}
		return $k;
	}

	$good = 0;
	$bad = 0;
	$tmbo = 0;
	$repost = 0;
	$comments = 0;

	$sql = "SELECT good, bad, tmbo, repost, comments from offensive_count_cache c
			WHERE threadid=$id";
	
	$result = tmbo_query( $sql );
	if( mysql_num_rows( $result ) > 0 ) {
		list( $good, $bad, $tmbo, $repost, $comments  ) = mysql_fetch_array( $result );
	}


	writeNav( $id );

	$filepath = get_include_path()."/".getFile($id, $filename, $timestamp);
	
	if( !file_exists( $filepath ) || $filename == "") {
		?><!-- header("Location: /offensive/404.php"); --><?
	}
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
   "http://www.w3.org/TR/html4/loose.dtd">

<html>
	<head>
		<META NAME="ROBOTS" CONTENT="NOARCHIVE" />
		<title>[ this might be offensive ]<?= $filename ? " : ".$filename: ""; ?> </title>
		<link rel="stylesheet" type="text/css" href="styles.php"/>
		<script type="text/javascript">
			self.file_id = "";
			
			// prevent sites from hosting this page in a frame;
			if( window != top ) {
				top.location.href = window.location.href;
			}
		</script>
		<script type="text/javascript" src="offensive.js"></script>

	</head>
	<body onload="doOnloadStuff()" onkeydown="return handleKeyDown( event );">
	<!-- message -->
	<div style="white-space:nowrap;overflow:hidden;padding:3px;margin-bottom:0px;background:#000033;color:#ff6600;font-size:10px;font-weight:bold;padding-left:4px;">
		<div style="float:right;"><a href="#" style="color:#ff6600" onclick="toggleVisibility( document.getElementById( 'instructions' ) ); return false">?</a></div>
		<div>consciousness doesn't really exist. it's just another one of our ideas.</div>
	</div>
	<div id="instructions" style="display:none;white-space:nowrap;overflow:hidden;padding:3px;margin-bottom:6px;background:#cccccc;color:#333333">left arrow = newer . up arrow = index . right arrow = older . down arrow = comments . plus key = [ this is good ] . minus key = [ this is bad ] . (because clicking is too hard.)</div>

	<div id="content">
		<div id="heading">

			&nbsp;&nbsp;

				<? fileNav( $nextid, $previd, $uploaderid, $uploader ); ?>
				 <a style="margin-left:48px;" id="comments" href="/offensive/?c=comments&fileid=<? echo $id?>">comments</a> (<?php echo "{$comments}c +$good -$bad"; if( $tmbo > 0 ) { echo " <span style=\"color:#990000\">x$tmbo</span>"; }?>)	

						<span style="margin-left:48px;">
						<?
							$votelinks_enabled = false;
							if($uploaderid != $_SESSION['userid']) {
								$sql = "SELECT *
								          FROM offensive_comments
									 WHERE userid = ".$_SESSION['userid']."
									   AND fileid = $id
									   AND vote != ''";
								$res = tmbo_query($sql);
								if(mysql_num_rows($res) == 0) {
									$votelinks_enabled = true;
								}
							}
							votelinks( $id, $votelinks_enabled );
						?>
						</span>

						<span style="margin-left:48px;">
						<?	// subscriptions
							$sql = "SELECT * FROM offensive_subscriptions WHERE userid=" . $_SESSION['userid'] . " AND fileid=$id";
							$res = tmbo_query( $sql );
							$subscribed = mysql_num_rows( $res ) > 0 ? true : false;
							if( $subscribed ) { ?>
								<a id="unsubscribeLink" href="/offensive/subscribe.php?un=1&fileid=<?= $id ?>" title="take this file off my 'unread comments' watch list.">unsubscribe</a>
						<?	} else { ?>
								<a id="subscribeLink" href="/offensive/subscribe.php?fileid=<?= $id ?>" title="watch this thread for new comments.">subscribe</a>
						<?	} ?>
						</span>
						<span style="margin-left:48px;">nsfw filter: <?php
							if( array_key_exists("prefs", $_SESSION) &&
							    is_array($_SESSION['prefs']) &&
							    array_key_exists("hide nsfw", $_SESSION['prefs']) &&
							    $_SESSION['prefs']['hide nsfw'] == 1 ) {
								?>
									<a href="/offensive/setPref.php?p=1&v=">off</a> on
								<?php
							}
							else {
								?>
									off <a href="/offensive/setPref.php?p=1&v=2">on</a>
								<?php
							}
						?></span>
						
						<span style="margin-left:48px;">tmbo filter: <?php
							if( array_key_exists("prefs", $_SESSION) &&
							    is_array($_SESSION['prefs']) &&
							    array_key_exists("hide tmbo", $_SESSION['prefs']) &&
							    $_SESSION['prefs']['hide tmbo'] == 1 ) {
								?>
									<a href="/offensive/setPref.php?p=3&v=">off</a> on
								<?php
							}
							else {
								?>
									off <a href="/offensive/setPref.php?p=3&v=2">on</a>
								<?php
							}
						?></span>
			</div>

			<?

				$imgfilename = "$filename";

			?>

			<br /><br />
			<?
				echo $is_nsfw == 1 ? "<span style=\"color:#990000\">[nsfw]</span>" : "";
				echo $is_tmbo == 1 ? "<span style=\"color:#990000\">[tmbo]</span>" : "";
// XXX: at some point I want to remove [nsfw] and [tmbo] if it exists in the filename. hmm...
				echo " ".htmlEscape($filename); ?> <span style="color:#999999"><?= getFileSize( $filepath ) ?></span>
			<br/>
			<span style="color:#999999">
				uploaded by <a id="userLink" href="../?c=user&userid=<? echo $uploaderid ?>"><? echo htmlEscape($uploader); ?></a> @ <?= $timestamp ?>
			</span>	
			<span style="margin-left:48px">
				<?
				if( isSquelched( $uploaderid ) ) {
					?><a id="unsquelchLink" style="color:#999999" href="/offensive/setPref.php?unsq=<?= $uploaderid ?>">unsquelch <?= $uploader ?></a><?
				}
				else {
					?><a id="squelchLink" style="color:#999999" href="/offensive/setPref.php?sq=<?= $uploaderid ?>">squelch <?= $uploader ?></a><?
				}
				?>
			</span>
			<br/><br/>
			<?
					if( hideImage($is_nsfw, $is_tmbo, $uploaderid) ) {
						?><div style="padding:128px;">[ filtered ] <!-- <?= $uploaderid ?> --></div><?
					} else {
						?>
						<div class="<?php echo $is_nsfw == 1 ? 'nsfw' : 'image' ?> u<?= $uploaderid ?>">
							<? 
								$imgurl = '';
								if($filepath != '')
									$imgurl = getFileURL($id, $filename, $timestamp);
								if($imgurl != '') {
							?>
							<a id="imageLink" href="<?= $imgurl ?>" target="_blank"><img src="<?= $imgurl ?>" style="border:none"/></a>
							<? } else echo "got nothin' for ya."; ?>
						</div>
	
						<?						
					}
					
			?>
			<br/><br/>
			
			<?
	
	function DateCmp($a, $b) {
		return ($a[1] < $b[1]) ? -1 : 1;
	}

	function SortByDate(&$files) {
		usort($files, 'DateCmp');
	}
	
	function votelinks( $id, $enabled ) {
		$good_href = $enabled ? "href=\"/offensive/?c=comments&submit=submit&fileid=$id&vote=this%20is%20good&redirect=true\"" : "";
		$bad_href = $enabled ? "href=\"/offensive/?c=comments&submit=submit&fileid=$id&vote=this%20is%20bad&redirect=true\"" : "";
		$class = $enabled ? "on" : "off";
	?>
		<span id="votelinks" class="<?= $class ?>">
		vote: <a id="good" class="votelink" <?= $good_href ?>>[ this is good ]</a> . 
			  <a id="bad" class="votelink" <?= $bad_href ?>>[ this is bad ]</a>
		</span>
	<?
	}

		?>
		</div>

<? 
	record_hit();
	include_once("analytics.inc"); 
?>

	</body>
</html>