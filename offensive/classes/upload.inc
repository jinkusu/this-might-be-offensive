<?

class Upload {
	private static $gdata = array();
	private $data;

	public function __construct($info) {
		/*
		 * info must be an id or aggregate array.  for more information about aggregate arrays,
		 * see this file's header comment.  the Upload class supports the following nested
		 * aggregate arrays:
		 * 		* User data with keys prefixed by user_
		 */
		if(is_array($info)) {
			if(!array_key_exists("id", $info))
				trigger_error("cannot prepopulate an Upload object without an id", E_USER_ERROR);

			// set up the object alias for this object's data in the global data structure
			if(!array_key_exists($info['id'], self::$gdata)) {
				// create if it doesn't already exist
				self::$gdata[$info['id']] = array('exists' => null);
			}
			$this->data =& self::$gdata[$info['id']];

			/*
			 * strip out keys that start with user_ into their own array,
			 * to be used to prepopulate a member User object in this Upload object
			 */
			$userarr = array();
			foreach($info as $key => $val) {
				if(strpos($key, "user_") === 0) {
					$userarr[substr($key, 5)] = $val;
				} else if($key != "exists") {
					$this->data[$key] = $val;
				}
			}

			/*
			 * grab the userid from the data as well, if available.  this will help
			 * rescue the failure case where user data not including the userid
			 * is included in the incoming data, which would cause it to be discarded
			 * and an error generated.
			 */
			if(array_key_exists("userid", $this->data))
				$userarr["userid"] = $this->data["userid"];

			// if we found any user data, create a User object from it.
			if(count($userarr) > 0) {
				$this->data["uploader"] = new User($userarr);
			}
		}
		/* it's a little silly that this conditional is the best way I can
		 * think of to test if a variable of any type is losslessly castable
		 * to an int (or already an int).  is_int() should be smarter.
		 */
		else if(is_intger($info)) {
			if(!array_key_exists($info, self::$gdata)) {
				self::$gdata[$info] = array('exists' => null);
			}
			$this->data =& self::$gdata[$info];
			$this->data['id'] = $info;
		} else {
			trigger_error("type mismatch: constructor argument must be an integer or array", E_USER_ERROR);
		}
	}

	private function getUploadRow($element=false) {
		// if we haven't hit the db for this upload already, do it
		if(($element == false || !array_key_exists($element, $this->data)) &&
		   $this->data['exists'] === null) {
			// get information we need from the database.
			$sql = "SELECT * FROM offensive_uploads WHERE id = ".$this->data["id"];
			$result = tmbo_query($sql);
			
			// if the upload doesn't exist...  well what can we do?
			if(mysql_num_rows($result) != 1) {
				$this->data['exists'] = false;
			} else {
				$row = mysql_fetch_assoc($result);
				foreach($row as $key => $val) {
					$this->data[$key] = $val;
				}
				$this->data['exists'] = true;
			}
		}
		// fulfil the query
		if($element) {
			if(array_key_exists($element, $this->data))
				return $this->data[$element];
			// if not, register an error.
			trigger_error("$element requested from offensive_uploads table, but was not available",
			              E_USER_ERROR);
		}
		return $this->data['exists'];
	}
	
	public function exists() {
		return $this->getUploadRow() && $this->status() != "pending";
	}
	
	public function expired() {
		if(!$this->exists()) return true;
		if($this->file() == '') return true;
		return false;
	}
	
	/*
	 * functions that deal with data available in the database upload row
	 */
	
	public function id() {
		return $this->data["id"];
	}
	
	public function filename() {
		return $this->getUploadRow("filename");
	}
	
	public function timestamp() {
		return $this->getUploadRow("timestamp");
	}
	
	public function is_tmbo() {
		return ($this->getUploadRow("tmbo") == 1);
	}
	
	public function is_nsfw() {
		return ($this->getUploadRow("nsfw") == 1);
	}
	
	public function type() {
		return $this->getUploadRow("type");
	}
	
	public function status() {
		return $this->getUploadRow("status");
	}
	
	/*
	 * functions that deal with comment and vote information that are
	 * stored in the offensive_count_cache
	 */
	private function getCacheRow($element) {
		// try to close requests immediately if they are simple element queries
		if(!array_key_exists($element, $this->data)) {
			// get information we need from the database.
			$sql = "SELECT good AS goods, bad AS bads, tmbo AS tmbos, repost AS tiars, comments,
			        timestamp AS last_active
			        FROM offensive_count_cache WHERE threadid = ".$this->data["id"];
			$result = tmbo_query($sql);
        	
			// if the upload doesn't exist...  well what can we do?
			if(mysql_num_rows($result) != 1) return false;
        	
			$row = mysql_fetch_assoc($result);
			foreach($row as $key => $val) {
				$this->data[$key] = $val;
			}
		}
      
		if(array_key_exists($element, $this->data))
			return $this->data[$element];
		// if not, register an error.
		trigger_error("$element requested from offensive_count_cache table, but was not available",
		              E_USER_ERROR);
	}
	
	public function goods() {
		return is_intger($this->getCacheRow("goods")) 
			? $this->getCacheRow("goods")
			: 0;
	}
	
	public function bads() {
		return is_intger($this->getCacheRow("bads"))
			? $this->getCacheRow("bads")
			: 0;
	}
	
	public function tmbos() {
		return is_intger($this->getCacheRow("tmbos"))
			? $this->getCacheRow("tmbos")
			: 0;
	}
	
	public function tiars() {
		return is_intger($this->getCacheRow("tiars"))
			? $this->getCacheRow("tiars")
			: 0;
	}
	
	public function comments() {
		return is_intger($this->getCacheRow("comments"))
			? $this->getCacheRow("comments")
			: 0;
	}
	
	public function last_active() {
		$last_active = $this->getCacheRow("last_active");
		return ($last_active == null ? $this->timestamp() : $last_active);
	}
	
	public function subscribed() {
		if(!array_key_exists('subscribed', $this->data)) {
			$sql = "SELECT * FROM offensive_subscriptions
				WHERE userid=".me()->id()." AND fileid=".$this->id();
			$this->data['subscribed'] = mysql_num_rows(tmbo_query($sql)) > 0 ? true : false;
		}
		
		return (bool)$this->data['subscribed'];
	}
	
	/*
	 * User slinging code
	 */
	
	public function uploader() {
		if(!array_key_exists("uploader", $this->data)) {
			if(array_key_exists("userid", $this->data)) {
				/*
				 * if we already have a userid, the User class can do the
				 * database work, it already knows how.
				 */
				$this->data["uploader"] = new User($this->getUploadRow("userid"));
			} else {
				/*
				 * if we have to go to the database anyway, we may as well
				 * grab the entire user row, to prevent a duplicate database call
				 * from the User class
				 */
				$sql = "SELECT u.* FROM users u
				        JOIN offensive_uploads up ON up.userid = u.userid
				        WHERE up.id = ".$this->id();
				
				$result = tmbo_query($sql);
				
				/* this error should be impossible.  if we ever hit it, we have a
				 * datastore inconsistency problem on our hands. */
				if(mysql_num_rows($result) !== 1) {
					trigger_error("user failure, user for this upload does not exist?", E_USER_ERROR);
				}
				
				$this->data["uploader"] = new User(mysql_fetch_assoc($result));
			}
		}
		
		return $this->data["uploader"];
	}
	
	/*
	 * comment slinging code
	 */
	public function getComments() {
		if(!array_key_exists("thecomments", $this->data)) {
			$this->data["thecomments"] = array();
			
			$sql = "SELECT com.*,
			               u.userid AS user_userid,
			               u.username AS user_username,
			               u.created AS user_created,
			               u.account_status AS user_account_status,
			               u.timestamp AS user_timestamp
			        FROM offensive_comments com, users u
			        WHERE u.userid = com.userid AND fileid = ".$this->data["id"].
			       " ORDER BY timestamp ASC";
			$result = tmbo_query($sql);
			while(false !== ($row = mysql_fetch_assoc($result))) {
				$this->data["thecomments"][] = new Comment($row);
			}
		}
		
		return $this->data["thecomments"];
	}
	
	/*
	 * upload slinging code
	 */
	public function next() {
		if(!array_key_exists("next", $this->data)) {
			$blocksql = "";
			if(me()) {
				$blocksql = "AND userid NOT IN (SELECT squelched FROM offensive_squelch WHERE userid = ".me()->id().")
                       AND userid NOT IN (SELECT userid FROM offensive_squelch WHERE squelched = ".me()->id().")";
			}
			
			$sql = "SELECT * FROM offensive_uploads WHERE id > {$this->id()} AND type='{$this->type()}' AND status='normal'
			        $blocksql
              ORDER BY id ASC LIMIT 1";	
			$result = tmbo_query($sql);
			if(mysql_num_rows($result) == 0) {
				$this->data["next"] = false;
			} else {
				$this->data["next"] = new Upload(mysql_fetch_assoc($result));
			}
		}
		
		return $this->data["next"];
	}
	
	public function next_filtered() {
		if(!me()) return $this->next();
		$me = me();
		
		if(!array_key_exists("next_filtered", $this->data)) {			
			$tmbo_sql = me()->getPref("hide_tmbo") ? " AND NOT tmbo " : "";
			$nsfw_sql = me()->getPref("hide_nsfw") ? " AND NOT nsfw " : "";

			$sql = "SELECT * FROM offensive_uploads WHERE id > {$this->id()} AND type='{$this->type()}' AND status='normal'
					$tmbo_sql
					$nsfw_sql
					AND userid NOT IN (SELECT squelched FROM offensive_squelch WHERE userid = {$me->id()})
					AND userid NOT IN (SELECT userid FROM offensive_squelch WHERE squelched = {$me->id()})
					ORDER BY id ASC LIMIT 1";
			$result = tmbo_query($sql);
			if(mysql_num_rows($result) == 0) {
				$this->data["next_filtered"] = false;
			} else {
				$this->data["next_filtered"] = new Upload(mysql_fetch_assoc($result));
			}
		}
		
		return $this->data["next_filtered"];
	}
	
	public function prev() {
		if(!array_key_exists("prev", $this->data)) {
      
			$sql = "SELECT * FROM offensive_uploads WHERE id < ".$this->id()." AND type='".$this->type()."' AND status='normal'
					ORDER BY id DESC LIMIT 1";
			$result = tmbo_query($sql);
			if(mysql_num_rows($result) == 0) {
				$this->data["prev"] = false;
			} else {
				$this->data["prev"] = new Upload(mysql_fetch_assoc($result));
			}
		}
		
		return $this->data["prev"];
	}

	public function prev_filtered() {
		if(!me()) return $this->prev();

		if(!array_key_exists("prev_filtered", $this->data)) {
			$tmbo_sql = me()->getPref("hide_tmbo") ? " AND NOT tmbo " : "";
			$nsfw_sql = me()->getPref("hide_nsfw") ? " AND NOT nsfw " : "";
      
			$sql = "SELECT * FROM offensive_uploads WHERE id < {$this->id()} AND type='{$this->type()}' AND status='normal'
          			$tmbo_sql
          			$nsfw_sql
					AND userid NOT IN (SELECT squelched FROM offensive_squelch WHERE userid = ".me()->id()." )
					ORDER BY id DESC LIMIT 1";
			$result = tmbo_query($sql);
			if(mysql_num_rows($result) == 0) {
				$this->data["prev_filtered"] = false;
			} else {
				$this->data["prev_filtered"] = new Upload(mysql_fetch_assoc($result));
			}
		}
		
		return $this->data["prev_filtered"];
	}
	
	/*
	 * filtering code
	 */
	public function filtered() {
		return $this->squelched() ||
		       $this->filtered_nsfw() ||
		       $this->filtered_tmbo() ||
		       $this->filtered_bad();
	}
	
	public function blocked() {
		return (me() != false
			? me()->blocked($this->uploader())
			: false);
	}

	public function squelched() {
		return (me() != false
			? me()->squelched($this->uploader())
			: false);
	}
	
	public function filtered_nsfw() {
		return (me() != false
			? me()->getPref("hide_nsfw") == 1 && $this->is_nsfw() == 1
			: false);
	}
	
	public function filtered_tmbo() {
		return (me() != false
			? me()->getPref("hide_tmbo") == 1 && $this->is_tmbo() == 1
			: false);
	}
	
	public function filtered_bad() {
		return (me() != false
			? me()->getPref("hide_bad") == 1
			  && $this->bads() > 10
			  && $this->bads() / 9 > $this->goods()
			: false);
	}
	
	/*
	 * string slinging code
	 */
	
	private function basePath($timestamp=null, $nopath=false) {
		if($timestamp == null) $timestamp = $this->timestamp();
		
		if(!is_intger($timestamp))
			$timestamp = strtotime($timestamp);
		
		$year = date( "Y", $timestamp );
		$month = date( "m", $timestamp );
		$day = date( "d", $timestamp );
		
		return ($nopath ? "" : get_include_path()."/")."offensive/uploads/$year/$month/$day/".$this->type()."/";
	}
		
	private function fileConform($path) {
		if($path == '' || !file_exists($path)) return false;
		
		$target = $this->basePath();
		ensureDirExists($target);
		$target .= $this->id()."_".$this->filename();
		
		// if timestamp is numeric, we want to move, not copy
		if(!rename($path, $target)) {
			trigger_error("rename($path, $target) failed", E_USER_WARNING);
			return false;
		}
		
		trigger_error("CONFORM: moved $path to $target", E_USER_NOTICE);
		return true;
	}
	
	public function file() {
		$id = $this->id();
		$filename = $this->filename();
				
		// uploads with files only
		if($this->type() == 'topic') {
			return '';
		}

		$retname = $this->basePath().$id."_".$filename;

		// if there are still files somewhere following the old naming scheme, this will flush them out.
		if(file_exists($this->basePath().$filename)) {
			rename($this->basePath().$filename, $retname);
		}

		if(file_exists($retname)) {
			return $retname;
		}
		
		/**********************************
		 * UH OH!  image not found!
		 * ignore the man behind the curtain
		 * maybe a backup restoration caused unicodes to break,
		 * or caused things to shift into yesterday.  or tomorrow.
		 * sigh.
		 */
		if(count(glob($this->basePath().$id."_*")) == 1) {
			list($ffilename) = glob($this->basePath().$id."_*");
			$filename = basename($ffilename);
			$filename = substr($ffilename, strpos($ffilename, "_") + 1);
			$sql = "UPDATE offensive_uploads SET timestamp=timestamp, filename = '".sqlEscape($filename)."' WHERE id = ".$this->id();
			tmbo_query($sql);
			return $ffilename;
		}
		
		$timestamp = strtotime($this->timestamp());
			
		$yesterday = $timestamp - 86400;
		$retname = $this->basePath($yesterday).$id."_".$filename;
		if($this->fileConform($retname)) return $retname;
			
		$tomorrow = $timestamp + 86400;
		$retname = $this->basePath($tomorrow).$id."_".$filename;
		if($this->fileConform($retname)) return $retname;

		return '';
	}
	
	public function URL() {
		// uploads with files only
		// if there's no file, why bother with a URL?
		if($this->type() == 'topic'
		   || $this->file() == '') {
			return '';
		}

		return "/".$this->basePath($this->timestamp(), true)
		       .$this->id()."_".rawurlencode($this->filename());
	}
	
	private function thumbConform($path) {
		if($path == '' || !file_exists($path)) return false;
		
		$target = $this->basePath()."thumbs/";
		ensureDirExists($target);
		$target .= "th".$this->id().($this->type() == "audio" ? "" : getFileExtension($path));

		if(!rename($path, $target)) {
			trigger_error("rename($path, $target) failed", E_USER_WARNING);
		}

		trigger_error("CONFORM: moved $path to $target", E_USER_NOTICE);
		return true;
	}
	
	public function thumb() {
		$id = $this->id();
		$timestamp = $this->timestamp();
		$type = $this->type();
		$ext = getFileExtension($this->filename());
		$thumbpath = "thumbs/th$id".($type == "audio" ? "" : $ext);

		// files only
		if($type == 'topic') return '';

		$retname = $this->basePath().$thumbpath;

		if(file_exists($retname)) {
			return $retname;
		}
		
		/**********************************
		 * UH OH!  image not found!
		 * ignore the man behind the curtain
		 * what about yesterday?  or tomorrow?
		 */
		
		$timestamp = strtotime($timestamp);
			
		$yesterday = $timestamp - 86400;
		$retname = $this->basePath($yesterday).$thumbpath;
		if(Upload::thumbConform($retname)) return $retname;
			
		$tomorrow = $timestamp + 86400;
		$retname = $this->basePath($tomorrow).$thumbpath;
		if(Upload::thumbConform($retname)) return $retname;

		if($type == "audio") {
			return "";
		}
		
		// still no good :( … is there a big image we can create a thumb from?
		$bigfile = $this->file();
		if($bigfile == '') {
			return '';
		}
		
		$retname = $this->basePath().$thumbpath;
		
		// man, I sure hope this doesn't happen often.
		// this will probably hurt if we get dogpiled, but this is a pretty uncommon code path
		$tmpfile = get_include_path()."/offensive/quarantine/$id".rand().$ext;
		copy($bigfile, $tmpfile);
        
		$cmd = "convert -resize 100x100 $tmpfile $tmpfile";
		$err = shell_exec($cmd);
		if(strlen($err > 0)) {
			trigger_error("ImageMagick failed: $err", E_USER_WARNING);
			return '';
		}
		ensureDirExists(dirname($retname));
		rename($tmpfile, $retname);
        
		trigger_error("CONFORM: resized $bigfile to $retname", E_USER_NOTICE);
        
		return $retname;
	}
	
	public function thumbURL() {
		// images only
		// if the file or thumb don't exist, why bother?
		if($this->type() == 'topic' || $this->file() == '' || $this->thumb() == '') {
			return "/offensive/graphics/previewNotAvailable.gif";
		}
		
		return "/".$this->basePath(null, true)."thumbs/th".$this->id().
		       ($this->type() == "audio" ? "" : getFileExtension($this->filename()));
	}
	
	public function commentLabel($showvotes=true) {
		$comments = $this->comments();

		$retval = '<span class="comments">'.$comments."</span>&nbsp;"
		         .'<span class="commentlabel">comment'.($comments == 1 ? "" :"s")."</span>";
		if(!$showvotes) return $retval;

		$retval .= '&nbsp;('
		          .'+<span class="goods">'.$this->goods().'</span>'
		          .'&nbsp;-<span class="bads">'.$this->bads().'</span>'
		          .($this->tmbos() > 0 ? '&nbsp;x<span class="tmbos">'.$this->tmbos().'</span>' : "")
		          .")";
		return $retval;
	}

	public function htmlFilename($showtype=false) {
		$tags = array();

		if($showtype) {
			$tags[] = $this->type().":";
		}

		if($this->is_nsfw()) {
			$tags[] = "[nsfw]";
		}

		if($this->is_tmbo()) {
			$tags[] = "[tmbo]";
		}

		// TODO: consume tags from image name

		return (count( $tags ) > 0 ? "<span class=\"tags\">" . implode( "", $tags ) . "</span> " : "")
		       .htmlEscape($this->filename());
	}
	
	public function dimensions() {
		$file = $this->file();
		if(strlen($file) == 0) {
			trigger_error("tried to get dimensions for an upload without a file", E_USER_WARNING);
			return false;
		}
		
		if($this->type() == "audio") {
			trigger_error("no support yet for dimensions of audio files (or their album art)", E_USER_WARNING);
			return array("?", "?");
		}
		
		$arr = getimagesize($file);
		if($arr === false) {
			trigger_error($file." is not a valid image", E_USER_WARNING);
			return array("?", "?");
		}
		          // width  , height
		return array($arr[0], $arr[1]);
	}

	// API support
	public function api_data() {

		$ret = array(
				"id" => $this->id(),
				"userid" => $this->uploader()->id(),
				"filename" => $this->filename(),
				"timestamp" => $this->timestamp(),
				"nsfw" => $this->is_nsfw(),
				"tmbo" => $this->is_tmbo(),
				"type" => $this->type(),
				"username" => $this->uploader()->username(),
				"subscribed" => $this->subscribed(),
				"vote_good" => $this->goods(),
				"vote_bad" => $this->bads(),
				"vote_tmbo" => $this->tmbos(),
				"vote_repost" => $this->tiars(),
				"comments" => $this->comments(),
				"filtered" => $this->filtered(),
				"last_active" => $this->last_active()
			);
		if($this->type() != "topic" && $this->file() != '') {
			$ret["link_file"] = $this->URL();
		}
		if(($this->type() == "image" || $this->type() == "avatar")
		   && $this->file() != "") {
			$arr = getimagesize($this->file());
			if($arr === false) { trigger_error($this->file()." is not a valid image!?", E_USER_ERROR); }
			
			$ret["width"] = $arr[0];
			$ret["height"] = $arr[1];
			
			$ret["link_thumb"] = $this->thumbURL();
			
			$arr = getimagesize($this->thumb());
			
			$ret["thumb_width"] = $arr[0];
			$ret["thumb_height"] = $arr[1];
		}
		
		return $ret;
	}
}

?>