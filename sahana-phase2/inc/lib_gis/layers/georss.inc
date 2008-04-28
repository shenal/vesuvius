<?php

/**
* PHP version 5
* 
* @author       Mifan Careem <mifan@respere.com>
* @author       Fran Boon <flavour@partyvibe.com>
* @copyright  Lanka Software Foundation - http://www.opensource.lk
* @package    Sahana - http://sahana.lk/
* @library      GIS
* @version     $Id: georss.inc,v 1.7 2008-04-28 21:56:31 franboon Exp $
* @license      http://www.gnu.org/copyleft/lesser.html GNU Lesser General Public License (LGPL)
*/

global $global;
include_once $global['approot'].'inc/lib_gis/layer_class.inc';

class georss implements layer{

    function print_form()
    {
        global $conf;
        shn_form_fsopen(_t('GeoRSS'));
        shn_form_fsopen(_t('INFO'));
        echo _t("<p>Sahana can make use of Feeds from servers using the <a href='http://en.wikipedia.org/wiki/GeoRSS' target=_blank>GeoRSS</a> standard </p>");
        echo _t("<p>Enable GeoRSS Layers by selecting the checkbox option below.</p>");
        shn_form_fsclose();
        shn_form_fsopen(_t('ENABLE'));
        $checked='';
        if (!$conf['gis_ol_georss']==0){
            $checked='checked';
        }
        shn_form_checkbox(_t('Enable GeoRSS layers'),'georss_enable',$checked,array('value'=>1));
        shn_form_fsclose();
        if (!$conf['gis_ol_georss']==0){
            shn_form_fsopen(_t('NUMBER'));
            shn_form_text(_t('Number of GeoRSS layers'),'gis_ol_georss','size=2 maxlength=2',array('value'=>$conf['gis_ol_georss']));
            shn_form_fsclose();
            $name_help=_t("Name displayed in Layer-Switcher (mandatory)");
            $description_help=_t("Only displayed in GIS Catalogue (optional)");
            $url_help=_t("Address of the GeoRSS service (mandatory)");
            $projection_help=_t("Coordinate system used to display the data, as defined by <a href=\"http://www.epsg.org\" target=_blank>European Petroleum Survey Group</a> (mandatory). If left blank, then EPSG:4326 is assumed as this is the default WGS84.");
            for ($i=0; $i<$conf['gis_ol_georss']; $i++) {
                $j=$i+1;
                shn_form_fsopen(_t('LAYER '.$j));
                shn_form_text(_t('Name'),'gis_ol_georss_'.$j.'_name',null,array('req'=>true,'value'=>$conf['gis_ol_georss_'.$j.'_name'],"help"=>$name_help));
                shn_form_text(_t('Description'),'gis_ol_georss_'.$j.'_description','size=60',array('value'=>$conf['gis_ol_georss_'.$j.'_description'],"help"=>$description_help));
                shn_form_text(_t('URL'),'gis_ol_georss_'.$j.'_url','size=60',array('req'=>true,'value'=>$conf['gis_ol_georss_'.$j.'_url'],"help"=>$url_help));
                $projection = "EPSG:4326";
                if (!null == $conf['gis_ol_georss_'.$j.'_projection']) {
                    $projection = $conf['gis_ol_georss_'.$j.'_projection'];
                }
                shn_form_text(_t('Projection'),'gis_ol_georss_'.$j.'_projection',null,array('req'=>true,'value'=>$projection,"help"=>$projection_help));
                shn_form_fsclose();
            }
        }
        shn_form_fsclose();
    }

    function commit_values()
    {
        global $global;
        global $conf;
        $db = $global['db'];                  
        for ($i=0; $i<$conf['gis_ol_georss']; $i++) {
            $j=$i+1;
            $description = 'gis_ol_georss_'.$j.'_description';
            $query = "SELECT * FROM config WHERE confkey='$description' AND module_id='admin'";
            $res = $db->Execute($query);
            if(!$res->EOF){
                $q = "UPDATE config SET value = '{$_POST[$description]}' WHERE module_id = 'admin' AND confkey = '$description'";
                $db->Execute($q);
            }
            else{
                $insert = "INSERT INTO config(module_id,confkey,value) VALUES ('admin','$description','{$_POST[$description]}')";
                $db->Execute($insert);
            }
            $name = 'gis_ol_georss_'.$j.'_name';
            if(!null==$_POST[$name]){
                $query = "SELECT * FROM config WHERE confkey='$name' AND module_id='admin'";
                $res = $db->Execute($query);
                if(!$res->EOF){
                    $q = "UPDATE config SET value = '{$_POST[$name]}' WHERE module_id = 'admin' AND confkey = '$name'";
                    $db->Execute($q);
                }
                else{
                    $insert = "INSERT INTO config(module_id,confkey,value) VALUES ('admin','$name','{$_POST[$name]}')";
                    $db->Execute($insert);
                }
            }
            $url = 'gis_ol_georss_'.$j.'_url';
            if(!null==$_POST[$url]){
                $query = "SELECT * FROM config WHERE confkey='$url' AND module_id='admin'";
                $res = $db->Execute($query);
                if(!$res->EOF){
                    $q = "UPDATE config SET value = '{$_POST[$url]}' WHERE module_id = 'admin' AND confkey = '$url'";
                    $db->Execute($q);
                }
                else{
                    $insert = "INSERT INTO config(module_id,confkey,value) VALUES ('admin','$url','{$_POST[$url]}')";
                    $db->Execute($insert);
                }
            }
        }
            // Checkboxes return nothing if unticked!
        if (null == $_POST['georss_enable']){
            $gis_ol_georss=0;
        } else {
            if (!null == $_POST['gis_ol_georss']){
                $gis_ol_georss=$_POST['gis_ol_georss'];
            } else {
                $gis_ol_georss=1;
            }
        }
        if (!$gis_ol_georss == $conf['gis_ol_georss']) {
            $query = "SELECT * FROM config WHERE confkey='gis_ol_georss' AND module_id='admin'";
            $res = $db->Execute($query);
            if(!$res->EOF){
                $q = "UPDATE config SET value = '$gis_ol_georss' WHERE module_id = 'admin' AND confkey = 'gis_ol_georss'";
                $db->Execute($q);
            }
            else{
                $insert = "INSERT INTO config(module_id,confkey,value) VALUES ('admin','gis_ol_georss','$gis_ol_georss')";
                $db->Execute($insert);
            }
        }
    }
    /**
     * @ToDo Change to an Add/Edit/Delete mode to avoid 1 long form?
     * NB Low-priority. This version works for now. Do development in georss2.inc
     */
    //function edit_item() {
    //}
    //function edit_item() {
    //}
}
 
?>