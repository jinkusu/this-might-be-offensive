<?
	// Include, and check we've got a connection to the database.
	require_once( 'admin/mysqlConnectionInfo.inc' );
	if(!isset($link) || !$link) $link = openDbConnection();

	require_once( 'offensive/assets/tabs.inc' );
	require_once( 'offensive/assets/functions.inc' );
	
	if(!array_key_exists("uid", $_REQUEST)) {
		trigger_error("must provide uid", E_USER_ERROR);
	}
	
	function start() {
		mustLogIn();

		if(me()->status() != 'admin') {
			header( "Location: ./" );
		}
	}

	function body() {
?>
<div class="contentbox">
	<div class="blackbar"></div>
	<div class="heading">spy stuff.</div>
<?
		tabs();
?>
		<div class="bluebox">
			<? iphistory( sqlEscape( $_REQUEST['uid'] ) ); ?>
		</div>
		<div class="blackbar"></div>
	</div>
		
<?
	}

	function iphistory( $uid ) {

?>

<table width="100%">

<?

		$numPerPage = 100;
		$page = array_key_exists("p", $_REQUEST) ? $_REQUEST['p'] : 0;
		if( ! is_intger( $page ) ) {
			$page = 0;
		}
		$start = ($page * $numPerPage);

		$sql = "SELECT distinct ip, max(timestamp) as timestamp, userid from ip_history where userid=$uid
			GROUP BY ip
					ORDER BY timestamp DESC LIMIT 100";

		$result = tmbo_query($sql);
		echo mysql_error();

?>
				<tr style="background:#bbbbee;text-align:left;">
					<td><div class="clipper">ip</div></td>
					<td><div class="clipper">last seen</div></td>
				</tr>
<?

		while( $row = mysql_fetch_assoc( $result ) ) {
			$css = (isset($css) && $css == "even_row") ? "odd_row" : "even_row";

			?>
				<tr class="<?= $css ?>">
					<td><div class="clipper"><a href="<?= Link::user($row['userid']) ?>"><?= $row['ip'] ?></a></div></td>
					<td><div class="clipper"><a href="<?= Link::user($row['userid']) ?>"><?= $row['timestamp'] ?></a></div></td>					
				</tr>
			<?
		}

		
		?>

	</table>


<?
	}

?>