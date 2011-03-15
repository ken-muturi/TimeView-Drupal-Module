<?php

// $Id$

/**
 * @file
 *   Functions and constants used by the TimeView module
*/

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
            	TimeView_onLoad('" . url('timeview') . "', 'TimeView_timeline');
      	  });
    	  }
        if ($('#TimeView_input').length) {
          $(document).ready(function() {
            $('#timeview_start_date').AnyTime_picker(
                    { format: '%Y-%m-%d %H:%i %@' } );
             $('#timeview_end_date').AnyTime_picker(
                    { format: '%Y-%m-%d %H:%i %@' } );
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
    $information = '';
    if (array_key_exists('institutions', $event)) { $information .= '<small><em>' . $event['institutions'] . '</em></small><br>'; }
    if (array_key_exists('location', $event))     { $information .= '<em>' . ucwords($event['category'] . ' @ ' . $event['location']) . '</em><hr>'; }
    if ($event['comments'])                       { $information .= $event['comments'] . '<br>'; }
    if (array_key_exists('documents', $event)) {
      $files = explode(',', $event['documents']);
      $information .= '<hr><strong>Attached Files</strong><br><ul>';
      foreach ($files as $file) {
        $information .= '<li><a href="' . drupal_get_path('module', 'TimeView') . '/uploads/' . $file . '">' . 
                             str_replace('_', ' ', $file) . '</a></li>';
      }
      $information .= '</ul>';
    }
    $node = $dom->createElement('event', $information);
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
      $start = new DateTime(strtotime($record['start_date']));
 //     $record['start_date'] = strftime('%a %b %d %Y %H:%M:%S', $start.getTimestamp());
      $record['start_date'] = date('D M d Y H:i:s \G\M\TO', $record['start_date']);
      $record['end_date'] = $record['end_date'] ? date('D M d Y H:i:s \G\M\TO', $record['end_date']) : '';
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

/**
 * A form to input new events to the timeline.
 *
 * This function uses JQuery UI functions to make input neater. If you wish to
 * avoid depending on the JQuery UI module modify this function. Nothing else
 * uses jQuery UI.
 *
 * @param array $form
 *   The form that is to be generated, supplied by drupal
 * @param array &$form_state
 *   The current state of the form, supplied by drupal
 *
 * @return array $form
 *   A form to input new events.
*/
function _TimeView_Input($form, &$form_state) {
  $form['name'] = 
    array('#type' => 'textfield',
	  '#title' => t('Name of the timeline event'),
	  '#size' => 75,
	  '#maxlength' => 255,
	  '#description' => t("Enter the name of the event you want to log"),
	  '#required' => TRUE,
	  );
  $form['location'] = 
    array('#type' => 'textfield',
    '#title' => t('Location of the timeline event'),
    '#size' => 75,
    '#maxlength' => 255,
    '#description' => t("Enter the location of this event."),
    '#required' => TRUE,
    );
  $form['start_date'] = 
    array('#type' => 'textfield',
    '#id' => "timeview_start_date",
    '#prefix' => "<table><tr><td>",
    '#suffix' => "</td>",
    '#title' => t('Start time'),
    '#size' => 30,
    '#maxlength' => 40,
    '#description' => t("Enter the start time of the event (YYYY-MM-DD HH:MM)."),
    '#required' => TRUE,
    );
  $form['end_date'] = 
    array('#type' => 'textfield',
    '#id' => "timeview_end_date",
    '#prefix' => "<td>",
    '#suffix' => "</td></tr></table>",
    '#title' => t('End time'),
    '#size' => 30,
    '#maxlength' => 40,
    '#description' => t("Enter the end time of the event (YYYY-MM-DD HH:MM)."),
    '#required' => FALSE,
    );
  $form['category'] = 
    array('#type' => 'select',
    '#title' => t('Category'),
    '#options' => array("sampling_trip" => "Sampling Trip",
                        "meeting" => "Meeting",
                        "visitor" => "Visitor"),
    '#description' => t("Select the category that this event falls in."),
    '#required' => FALSE,
    );
  $form['event_ICIPE'] = 
    array('#type' => 'checkbox',
    '#prefix' => "<table><tr><th colspan='7'>Involved Institutions</td><tr><td>",
    '#suffix' => "</td>",
    '#title' => t('ICIPE'),
    '#required' => FALSE,
    );
  $form['event_KEMRI'] = 
    array('#type' => 'checkbox',
    '#prefix' => "<td>",
    '#suffix' => "</td>",
    '#title' => t('KEMRI'),
    '#required' => FALSE,
    );
  $form['event_DVS'] = 
    array('#type' => 'checkbox',
    '#prefix' => "<td>",
    '#suffix' => "</td>",
    '#title' => t('DVS'),
    '#required' => FALSE,
    );
  $form['event_KWS'] = 
    array('#type' => 'checkbox',
    '#prefix' => "<td>",
    '#suffix' => "</td>",
    '#title' => t('KWS'),
    '#required' => FALSE,
    );
  $form['event_MoH'] = 
    array('#type' => 'checkbox',
    '#prefix' => "<td>",
    '#suffix' => "</td>",
    '#title' => t('Ministry of Health'),
    '#required' => FALSE,
    );
  $form['event_KARI'] = 
    array('#type' => 'checkbox',
    '#prefix' => "<td>",
    '#suffix' => "</td>",
    '#title' => t('KARI'),
    '#required' => FALSE,
    );
  $form['event_ILRI'] = 
    array('#type' => 'checkbox',
    '#prefix' => "<td>",
    '#suffix' => "</td></tr></table>",
    '#title' => t('ILRI'),
    '#required' => FALSE,
    );
  $form['comments'] = 
    array('#type' => 'textarea',
    '#title' => t('Comments and Remarks.'),
    '#description' => t("Enter comments or remarks for this trip. You can also add links to open new pages, or download documents"),
    '#required' => FALSE,
    );
  $form['documents'] = 
    array('#type' => 'file',
    '#attributes' => array('class' => array('multi'),
                           'name' => 'files[]'),
    '#title' => t('Attached Files'),
    '#description' => t("Upload files that are related to the event."),
    '#required' => FALSE,
  );
  $form['event_submit'] = 
    array('#type' => 'submit',
    '#value' => "Submit Form",
    '#title' => t('Submit.'),
    );
  return $form;
}

/**
 * Validates the input from _TimeView_Input before it is saved to the database.
 * 
 * This function makes sure that all dates are in the expected format, that all 
 * required fields are supplied and so on.
 *
 * @param array $form
 *   The form that is to be generated, supplied by drupal
 * @param array &$form_state
 *   The current state of the form, supplied by drupal
*/
function _TimeView_Input_validate(&$form, &$form_state) {
  $date_regexp = '/\d{4}-\d{2}-\d{2} \d{2}:\d{2}/';
  
  if (!preg_match($date_regexp, $form_state['values']['start_date'])) {
		form_set_error('event_start', t('You must enter a start date in the form (YYYY-MM-DD HH:MM) e.g. 2010-11-04 15:58.'));
  }
  
  if (!empty($form_state['values']['end_date']) and !preg_match($date_regexp, $form_state['values']['end_date'])) {
		form_set_error('event_end', t('You must enter an end date in the form (YYYY-MM-DD HH:MM) e.g. 2010-11-05 17:00.'));
  }
}

/**
 * Saves information from the input form to the database
 *
 * This is mainly a wrapper function around drupal_write_record, that formats
 * the output variables and converts timestamps to epoch time.
 *
 * @param array $form
 *   The form that is to be generated, supplied by drupal
 * @param array &$form_state
 *   The current state of the form, supplied by drupal
*/
function _TimeView_Input_submit($form_id, &$form_state) {
  $trn = db_transaction();
  $institutions = "";
  if ($form_state['values']['event_ICIPE']) { $institutions .= "ICIPE, "; }
  if ($form_state['values']['event_KEMRI']) { $institutions .= "KEMRI, "; }
  if ($form_state['values']['event_DVS'])   { $institutions .= "DVS, "; }
  if ($form_state['values']['event_KWS'])   { $institutions .= "KWS, "; }
  if ($form_state['values']['event_MoH'])   { $institutions .= "MoH, "; }
  if ($form_state['values']['event_KARI'])  { $institutions .= "KARI, "; }
  if ($form_state['values']['event_ILRI'])  { $institutions .= "ILRI, "; }
  if ($institutions) { $institutions = substr($institutions, 0, strlen($institutions) - 2); }
  
  try {
    foreach ($_FILES['files']['name'] as $key => $filename) {
      if (!$_FILES['files']['error'][$key]) {
        // Convert all white-space to underlines to get nicer URLs
        $filename = str_replace(' ', '_', $filename);
        $path = drupal_get_path('module', 'TimeView') . '/uploads/';
        // a VERY basic check if file exists, just adding a _ at the beginning
        // until the name is unique. TODO: better filename collision renaming.
        while (file_exists($path . $filename)) {
          $filename = '_' . $filename;
        }
        if (move_uploaded_file($_FILES['files']['tmp_name'][$key], $path . $filename)) {
          $form_state['values']['documents'] .= $filename . ',';
        }
      }
    }
    if ($form_state['values']['documents']) {
      $form_state['values']['documents'] = substr($form_state['values']['documents'], 0, -1);
    }
    $id = db_insert('timeview_event')
                    ->fields(array('name' => $form_state['values']['name'], 
                                 'location' => $form_state['values']['location'], 
                                 'start_date' => strtotime($form_state['values']['start_date']), 
                                 'end_date' => strtotime($form_state['values']['end_date']) 
                                               ? strtotime($form_state['values']['end_date']) 
                                               : NULL, 
                                 'institutions' => $institutions, 
                                 'category' => $form_state['values']['category'], 
                                 'comments' => $form_state['values']['comments'], 
                                 'documents' => $form_state['values']['documents'],
                    ))
                    ->execute();
  }
  catch (Exception $e) {
    $trn->rollback();
    watchdog_exception('type', $e);
  }
  
  // Unless the transaction was rolled back, it gets commited here 
}

