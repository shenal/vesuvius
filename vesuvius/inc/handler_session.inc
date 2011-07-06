<?
/**
 * @name         Session Handler
 * @version      1.0
 * @package      framework
 * @author       Ravindra De Silva <ravindra@opensource.lk><ravidesilva@iee.org>
 * @author       Greg Miernicki <g@miernicki.com> <gregory.miernicki@nih.gov>
 * @about        Developed in whole or part by the U.S. National Library of Medicine
 * @link         https://pl.nlm.nih.gov/about
 * @link         http://sahanafoundation.org
 * @license	 http://www.gnu.org/copyleft/lesser.html GNU Lesser General Public License (LGPL)
 * @lastModified 2011.0401
 */


// following code decides the session management approach (e.g write sessions to the database or files)
global $global;
require_once($global['approot'].'inc/lib_security/constants.inc');
require_once($global['approot'].'inc/lib_security/lib_auth.inc');
require_once($global['approot'].'mod/lpf/lib_lpf.inc');

//start the session
shn_session_start();

// authenticate user
$user_data = shn_authenticate_user();

switch($user_data["result"]) {
	case LOGGEDIN:
		$global["module"] = isset($conf['default_module']) ? $conf['default_module'] : "home";
		$global["action"] = isset($conf['default_action']) ? $conf['default_action'] : "default";
		$global['previous'] = false;
		shn_session_change($user_data);
		break;
	case LOGGEDOUT:
		shn_session_end($user_data);
		header("Location: ".makeBaseUrlMinusEvent());
	case LOGINFAILED:
		$global["module"]= "pref";
		$global["action"]= "loginForm";
		$global['previous']= false;
	default:
		break;
}

session_update_activity();

/**
* this function is a wrapper around session_start()
* it includes checks to reduce session hijacking
* therefore this function will also start or
* retreive a session like session_start() ,but
* with better security
*
* @access public
* @return void
*/
function shn_session_start() {
	// start httponly cookies to prevent xss attacks
	session_set_cookie_params(0, "/", null, false, true);

	// retrieves or starts a session
	if(!isset($_SESSION)) {
		session_start();
	}

	// We will always regenerate the key for anonymous users to avert them hijacking a previous session_id without passing through the login process
	/*
	// Since the guest user cant do much , no one would try to hijack their session, so let the attackers hijack this useless session
	if (ANONYMOUS_USER == $_SESSION['user_id']){
		return true;
	}
	*/

	// We have to protect the session, so lets try to make it hard for session hijacking. The use of IPADDRESS to prevent session hijacking has
	// been rejected by many many PHP experts, what they say is its relatively easy to spoof an IP,on the other hand many networks change the IP for every request e.g AOL with millions of users
	if (isset($_SESSION['initiated'])) {
		// user agent maybe not present in every request, so check only if its set, so users with user agent are more secure against their session being hijacked
		if (isset($_SESSION['HTTP_USER_AGENT'])) {
			if ($_SESSION['HTTP_USER_AGENT'] != md5($_SERVER['HTTP_USER_AGENT'])) {
				change_session_to_anonymous();
			}
		}
		if (!session_is_valid()) {
			change_session_to_anonymous();
		}
	} else {
		change_session_to_anonymous();
	}
}



function session_is_valid() {
	global $global;
	$db = $global['db'];
	$time = time();
	$sess_id = session_id();
	$sql = "
		select inactive_expiry, expiry, secret
		from sessions where session_id='{$sess_id}';
	";
	$res = $db->Execute($sql);
	if (($res==null) || ($res->EOF)) {
		return false;
	} else {
		$in_expiry = $res->fields["inactive_expiry"];
		$expiry = $res->fields["expiry"];
		$secret = $res->fields["secret"];
		// changed to allow up to 8 hours of inactivity, and session allowed to age up to 16 hours ~ we always will regenerate sessid upon relogin from this method, since its more secure
		if (($time < $in_expiry+(3600*8)) && ($time < $expiry+(3600*16))) {
			return true;
		} else {
			return false;
		}
	}
}



function session_update_activity(){
	global $global;
	$db      = $global['db'];
	$time    = time();
	$sess_id = session_id();
	$sql     = "update sessions set inactive_expiry={$time} where session_id='{$sess_id}'";
	$res     = $db->Execute($sql);
	return $res;
}



function change_session_to_anonymous() {
	$key = isset($_SESSION['security_captcha_key']) ? $_SESSION['security_captcha_key'] : null;
	session_regenerate_id();
	$_SESSION = array(); // reset the session to empty any extraneous data
	$_SESSION['user_agent'] = md5($_SERVER['HTTP_USER_AGENT']);
	$_SESSION['initiated']  = true;
	$_SESSION['logged_in']  = false;
	$_SESSION["user"]       = "Anonymous";
	$_SESSION["user_id"]    = ANONYMOUS_USER;
	$_SESSION['security_captcha_key'] = $key; // restore captca key just in case
}



/**
* when the user changes privilege levels (e.g login)
* its recommended to regenerate the session id
* this function does that and in addition registers
* ,changes several session variables to reflect
* change of privileges
*
* @param mixed $user_data
* @access public
* @return void
*/
function shn_session_change($user_data) {
	global $global;
	$db = $global['db'];

	// regenerate the session id and send to the client, this  is essential as user obtained different privileges
	session_regenerate_id();
	$_SESSION['logged_in'] = true;
	$sess_id = session_id();
	$key     = genkey($sess_id);
	$secret  = genkey($sess_id);
	$time    = time();
	$sql = "
		insert into sessions (session_id, sess_key, secret, inactive_expiry, expiry)
		values('{$sess_id}', '{$key}', '{$secret}', {$time}, {$time});
	";
	$res     = $db->Execute($sql);
	$expiry  = time() + 14400; // 4 hours
	$_SESSION['user'] = $user_data["user"];

	// quick hack to get the display name for session
	// updated by Greg -- added group id and group name to the session data (used in some of our modules)
	$sql = "
		select *
		from users as u
		inner join person_uuid as p on u.p_uuid = p.p_uuid
		inner join sys_user_to_group as s on p.p_uuid = s.p_uuid
		inner join sys_user_groups as g on g.group_id = s.group_id
		where u.user_name='".$user_data["user"]."';
	";
	$res = $global['db']->GetRow($sql);
	if($res) {
		$user_name = $res[0]." ".$res[1];
	}
	$_SESSION['user_name']   = $res["user_name"];
	$_SESSION['full_name']   = $res["full_name"];
	$_SESSION['user_id']     = $res["user_id"];
	$_SESSION['user_p_uuid'] = $res["p_uuid"];
	$_SESSION['group_id']    = $res["group_id"];
	$_SESSION['group_name']  = $res["group_name"];
	return ;
}



/**
* this function is a wrapper around session_regenerate_id()
* it will empty the session variables.
*
* @param mixed $user_data
* @access public
* @return void
*/
function shn_session_end($user_data) {
	global $global;
	$db = $global['db'];
	// regenerate the session id and send to the client, this  is essential as user obtained different privileges
	$sess_id = session_id();
	$sql = "delete from sessions where session_id = '{$sess_id}'";
	$res = $db->Execute($sql);
	session_regenerate_id();
	$_SESSION['logged_in'] = false;
	$_SESSION['user']      = "Anonymous";
	$_SESSION['user_id']   = ANONYMOUS_USER;
	return ;
}



/**
* function which checks whether a Session variable is already registered
* since there are many modules ,there is the possibility of one module
* overiding a session varibale of another.
* proper naming convention should reduce that, in addition its recommended
* to call this function to see if the variable is already registered
*
* @param mixed $sess_var
* @access public
* @return void
*/
function shn_session_is_registered($sess_var) {
	if(!isset($_SESSION[$sess_var])) {
		return true;
	} else {
		return false;
	}
}

