<?php 
/**
* Sahana front controller, through which all actions are dispatched
*
* PHP version 4 and 5
*
* LICENSE: This source file is subject to LGPL license
* that is available through the world-wide-web at the following URI:
* http://www.gnu.org/copyleft/lesser.html
*
* @package    Sahana - http://sahana.sourceforge.net
* @author     http://www.linux.lk/~chamindra
* @copyright  Lanka Software Foundation - http://www.opensource.lk
*/

// Specify the base location of the Sahana insallation
// The base should not be exposed to the web for security reasons
// only the www sub directory should be exposed to the web

$global['approot'] = realpath(dirname(__FILE__)).'/../';
$approot = $global['approot'];
$global['previous']=false;

// include the base libraries for both the web installer and main app 
//require_once ($global['approot'].'inc/handler_error.inc');
require_once ($global['approot'].'inc/lib_config.inc');
require_once ($global['approot'].'inc/lib_modules.inc'); 

// === filter the GET and POST ===
shn_main_filter_getpost();

// === Setup if not setup and load configuration === 

// if installed the sysconf.inc will exist in the conf directory
// if not start the web installer
if (!file_exists($global['approot'].'conf/sysconf.inc')){

    // include the sysconfig template for basic conf dependancies
    require_once ($global['approot'].'conf/sysconf.inc.tpl'); 

    // launch the web setup wizard
    require ($global['approot'].'inst/setup.inc');

} else {

    // define the configuration priority order
    require_once ($global['approot'].'conf/conf-order.inc');

    // include the main sysconf file
    require ($global['approot'].'conf/sysconf.inc'); 

    // include the main libraries the system depends 
    require_once ($global['approot'].'inc/handler_db.inc');
    require_once ($global['approot'].'inc/lib_session/handler_session.inc');
    require_once ($global['approot'].'inc/lib_security/authenticate.inc');
    require_once ($global['approot'].'inc/lib_locale/handler_locale.inc'); 

    //include the user preferences
    include_once ($global['approot'].'inc/lib_user_pref.inc');
    shn_user_pref_populate();

    // load all the configurations based on the priority specified 
    // files and database, base and mods
    shn_config_load_in_order();

    // start the front controller pattern
    shn_main_front_controller();
}

// === process the GET and POST ===
function shn_main_filter_getpost()
{
    global $global;

    if(!$global['previous']){
        $global['action'] = (NULL == $_REQUEST['act']) ? 
                                "default" : $_REQUEST['act'];
        $global['module'] = (NULL == $_REQUEST['mod']) ? 
                                "home" : $_REQUEST['mod'];
    }
}


// === front controller ===
function shn_main_front_controller() 
{
    global $global;
    global $conf;
    $approot = $global['approot'];
    $action = $global['action'];
    $module = $global['module'];

    // define which stream library to use base on POST "stream" 
    if(isset($_REQUEST['stream']) && file_exists($approot."/inc/lib_stream_{$_REQUEST['stream']}.inc")){

        require_once $approot."/inc/lib_stream_{$_REQUEST['stream']}.inc";
        $stream_ = $_REQUEST['stream']."_";

    } else {
    // default to the HTML stream
        require_once $global['approot']."/inc/lib_stream_html.inc";
        $stream_ = null;
    }

    // Redirect the module based on the action performed
    // redirect admin functions through the admin module
    if (preg_match('/^adm/',$action)) {

        $global['effective_module'] = $module = 'admin';
        $global['effective_action'] = $action = 'modadmin';
    } // the orignal module and action is stored in $global

    // This is a redirect for the report action
    if (preg_match('/^rpt/',$action)) {

        $global['effective_module'] = $module = 'rs';
        $global['effective_action'] = $action = 'modreports';
    }
  
    // check the users access permissions for this action
    $module_function = 'shn_'.$stream_.$module.'_'.$action;
	$acl_enabled=shn_acl_get_state($module);

    // @TODO: test against admin function is wrong
    $allow = (!shn_acl_check_perms_action($_SESSION['user_id'], $module_function) || 
             !$acl_enabled)? true : false;

    // include the correct module file based on action and module
    $module_file = $approot.'mod/'.$module.'/main.inc';

    // default to the home page if the module main does not exist
    if (file_exists($module_file)) {
        include($module_file); 
    } else {
        include($approot.'mod/home/main.inc');
    }

    // stream (XHTML, XML, TEXT, etc) initialization
    // this includes the inclusion of various sections in XHTML including the HTTP header,
    // content header, menubar, login
    shn_stream_init();

    if($allow){ // if permission is allowed for user

        // compose and call the relevant module function 
        if (!function_exists($module_function)) {
            $module_function='shn_'.$stream_.$module.'_default';
        }

        $_SESSION['last_module']=$module;
        $_SESSION['last_action']=$action;
        $module_function(); 

    }else { 
        // otherwise show a unauthorized access message
        shn_error_display_restricted_access();
    }

    // close up the stream. In HTML send the footer
    shn_stream_close();
}
