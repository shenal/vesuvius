<?php
/**
 * @name         Google Translate
 * @version      1.2
 * @package      res
 * @author       Ramindu Deshapriya <rasade88@gmail.com>
 * @about        Developed in whole or part by the U.S. National Library of Medicine
 * @link         https://pl.nlm.nih.gov/about
 * @license	 http://www.gnu.org/licenses/lgpl-2.1.html GNU Lesser General Public License (LGPL)
 * @lastModified 2012.06.23
 * NOTE: This is only intended as a script, not to be called from a web context under any circumstances.
 */

if ( !defined($access) ) die('No direct access!!');

include_once('google_trans.inc');


$filelist = array();

$gtrans = new GTranslator(); 

foreach ( $filelist as $locale ) {
	$gtrans->buildGooglePO($locale);
}