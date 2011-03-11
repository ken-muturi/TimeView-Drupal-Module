<?php

// $Id$

/**
 * @file
 *   Functions and constants used by the TimeView module
*/

/**
 * TimeView XML
*/
define('TIMEVIEW_XML', url('timeview', array('absolute' => True)));

/** 
 * Generates a javascript to initialize the timeline.
 *
 * The script does several things, first it adds the "TimeView_active" class to
 * the TimeView_timeline div in the TimeView block. This unhides the div and 
 * allows us to style it. 
 * Having this function generated from php allows us to add a properly 
 * formatted url on where to load the timeline xml.
 *
 * @return string $script
 *   A jQuery javascript that will initialize the timeline.
*/
function TimeView_jQuery() {
  $script = "
  (function ($) {

    Drupal.behaviors.TimeView = {
      attach: function(context, settings) {
        if ($('#TimeView').length) {
          $(document).ready(function() {
            	$('#TimeView_timeline').addClass('TimeView_active');
            	TimeView_onLoad('" . url(TIMEVIEW_XML) . "', 'TimeView_timeline');
      	  });
    	  }
      }
    };

  })(jQuery);
  ";
  return $script;
}

/**
 * Generates XML that is used by the timeline
 *
 * Uses the php DOM functions to generate XML from databases and static
 * content. This function does not return a drupal formatted page but an
 * XML-file formatted as an attachment.
 *
*/
function TimeView_XML() {
  drupal_add_http_header('Content-Type', 'text/xml');
//  drupal_add_http_header('Content-Disposition', 'attachment; filename=TimeView.xml;');
  
  $events = array_merge(_TimeView_load(), 
                        _TimeView_SamplingTrips(), 
                        _TimeView_WildlifeTrips());
  
  // Create the XML
  $dom = new DOMDocument('1.0', 'UTF-8');
  $dom->formatOutput = true;
  $data = $dom->createElement('data');
  $dataNode = $dom->appendChild($data);
  
  foreach ($events as $event) {
    $node = $dom->createElement('event', $event['comments']);
    $node->setAttribute('title', $event['name']);
    $node->setAttribute('start', $event['start_date']);
    if ($event['end_date']) { $node->setAttribute('end',   $event['end_date']); }
    
    $eventNode = $data->appendChild($node);
  }
  
  echo $dom->saveXML();
  // Return that we're fine
  exit(0);
}

/**
 * Saves information to the database
 *
 * This is mainly a wrapper function around drupal_write_record, that formats
 * the output variables and converts timestamps to epoch time.
 *
 * @param string name
 *   Name/Title of the event
 * @param string location
 *   Location of the event (optional)
 * @param string start_date
 *   Start date and time for the event
 * @param string end_date
 *   End date and time of the event (optional)
 * @param string institutions
 *   Institutions involved in the event (optional)
 * @param string category
 *   Category for this event (optional)
 * @param string comments
 *   Comments/Info about the event (optional)
 * @param string documents
 *   Attached documents
*/
function _TimeView_save($name, $loc, $start, $end = NULL, $inst = NULL, $cat = NULL, $comm = NULL, $docs = NULL) {
  $trn = db_transaction();
  try {
    $id = db_insert('timeview_event')
                    ->fields(array('name' => $name, 
                                 'location' => $loc, 
                                 'start_date' => strtotime($start), 
                                 'end_date' => strtotime($end) ? strtotime($end) : NULL, 
                                 'institutions' => $inst, 
                                 'category' => $cat, 
                                 'comments' => $comm, 
                                 'documents' => $docs,
                    ))
                    ->execute();
    return $id;
  }
  catch (Exception $e) {
    $trn->rollback();
    watchdog_exception('type', $e);
  }
  // Unless the transaction was rolled back, it gets commited here 
}

/**
 * Loads event information to the database
 *
 * This function uses drupals built in database handlers and return a formatted
 * array of records that can later be converted to XML for the timeline.
 *
 * @return array(array) $events
 *   An associative array of arrays describing the database table.
*/
function _TimeView_load() {
  try {
    $events = array();
    $records = db_select('timeview_event', 't')
                        ->fields('t') // Equivalent of "SELECT * FROM"
                        ->execute();
    while ($record = $records->fetchAssoc()) {
      $record['start_date'] = strftime('%a %b %d %Y %H:%M:%S', $record['start_date']);
      $record['end_date'] = $record['end_date'] ? strftime('%a %b %d %Y %H:%M:%S', $record['end_date']) : '';
      array_push($events, $record);
    }
    return $events;
  }
  catch (Exception $e) {
    drupal_set_message($e, 'error');
  }
}

/**
 * Creates an array of events for each livestock sampling
 * 
 * @return array(array) $samplingTrips
 *   An array of associative arrays containing the events
*/
function _TimeView_SamplingTrips() {
  $samplingTrips = array();
  if (module_exists('LabCollector')) {
    $events = LabCollector_SamplingTrips();
    foreach ( $events as $event ) {
      $trip['name']       = $event['name'];
      $trip['start_date'] = $event['start_date'];
      $trip['end_date']   = $event['end_date'];
      $trip['comments']   = $event['open'] . 
                            $event['private'] . 
                            '<hr><img src="' . $event['map'] . '">';
      array_push($samplingTrips, $trip);
    }
  }
  return $samplingTrips;
}

/**
 * Creates an array of events for each wildlife sampling
 * 
 * @return array(array) $wildlifeTrips
 *   An array of associative arrays containing the events
*/
function _TimeView_WildlifeTrips() {
  $wildlifeTrips = array();
  if (module_exists('LabCollector')) {
    $events = LabCollector_WildlifeTrips();
    foreach ( $events as $event ) {
      $trip['name']       = $event['name'];
      $trip['start_date'] = $event['start_date'];
      $trip['end_date']   = $event['end_date'];
      $trip['comments']   = $event['open'] . '<hr><img src="' . $event['map'] . '">';
      array_push($wildlifeTrips, $trip);
    }
  }
  return $wildlifeTrips;
}

