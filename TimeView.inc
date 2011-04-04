<?php

// $Id$

/**
 * Copyright 2011 ILRI
 *
 * This file is part of TimeView.
 * 
 * TimeView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * TimeView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with TimeView.  If not, see <http://www.gnu.org/licenses/>.
*/

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
            	TimeView_onLoad('" . url('timeview/xml') . "', 'TimeView_timeline');
      	  });
    	  }
        if ($('#TimeView_input').length) {
          $(document).ready(function() {
            $('#timeview_start_date').AnyTime_picker( { format: '%Y-%m-%d %H:%i' } );
            $('#timeview_end_date').AnyTime_picker(   { format: '%Y-%m-%d %H:%i' } );
            
            $('#timeview_controls').html('<a id=\"timeview_all\">Select All</a><a id=\"timeview_none\">Select None</a>');
            $('#timeview_all').bind('click', function() {
              $('.timeview_checkboxes input').each(function(){
                this.checked = true;
              });
            });
            $('#timeview_none').bind('click', function() {
              $('.timeview_checkboxes input').each(function(){
                this.checked = false;
              });
            });
            
            if ($('#timeview_category').val() != 'other') {
              $('#timeview_other').hide();
            }
            $('#timeview_category').bind('change', function() {
              if ($('#timeview_category').val() == 'other') {
                $('#timeview_other').show();
              } else {
                $('#timeview_other').hide();
              }
            });
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
    if (array_key_exists('institutions', $event)) { $information .= '<small><em>' . str_replace('MOH', 'Min. of Health', $event['institutions']) . '</em></small><br>'; }
    if (array_key_exists('location', $event))     { $information .= '<em>' . ucwords($event['category'] . ' @ ' . $event['location']) . '</em><hr>'; }
    if ($event['comments'])                       { $information .= $event['comments'] . '<br>'; }
    if (array_key_exists('documents', $event) && $event['documents']) {
      $files = explode(',', $event['documents']);
      $information .= '<hr><strong>Attached Files</strong><br><ul>';
      foreach ($files as $file) {
        $information .= '<li><a href="' . drupal_get_path('module', 'TimeView') . '/uploads/' . $file . '">' . 
                             str_replace('_', ' ', $file) . '</a></li>';
      }
    }  
    $information .= '</ul>';
    if (user_access('edit TimeView') and array_key_exists('id', $event)) {
      $information .= '<hr> | ' . l(t('Edit'), 'timeview/edit/' . $event['id']) . ' | ' . 
                      l(t('Delete'), 'timeview/delete/' . $event['id']) . ' |';
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
 * @param int $id
 *   Loads only the given $id
 *
 * @return array(array) $events
 *   An associative array of arrays describing the database table.
*/
function _TimeView_load($id = NULL) {
  $date_format = $id ? 'Y-m-d H:i:s' : 'D M d Y H:i:s \G\M\TO';
  // Return an empty event if $id == 0
  if ($id === 0) {
    return array('id'           => '',
                 'name'         => '',
                 'location'     => '',
                 'start_date'   => '',
                 'end_date'     => '',
                 'institutions' => '',
                 'category'     => '',
                 'comments'     => '',
                 'documents'    => '');
  }
  try {
    $events = array();
    $records = db_select('timeview_event', 't')
                        ->fields('t') // Equivalent of "SELECT * FROM"
                        ->execute();
    while ($record = $records->fetchAssoc()) {
      $start = new DateTime(strtotime($record['start_date']));
      $record['start_date'] = date($date_format, $record['start_date']);
      $record['end_date'] = $record['end_date'] ? date($date_format, $record['end_date']) : '';
      if ($id and $id == $record['id']) {
        return $record;
      }
      array_push($events, $record);
    }
    return $events;
  }
  catch (Exception $e) {
    drupal_set_message($e, 'error');
  }
}

/**
 * Creates a small form to confirm event deletion.
 *
 * @param array $form
 *   The form that is to be generated, supplied by drupal
 * @param array &$form_state
 *   The current state of the form, supplied by drupal
 *
 * @return array $form
 *   The confirm deletion form.
*/
function TimeView_delete($form, &$form_state, $id) {
	$form = array();
  $form['id'] =
    array('#type' => 'hidden',
    '#value' => $id,
    );
	$form['submit'] = 
	  array('#type' => 'submit',
		'#prefix' => 'Are you sure you want to delete event ' . $id . '?',
	  '#value' => "Delete",
	  );
	return $form;
}

/**
 * Deletes an event from the database
 *
 * This function uses drupals built in database handlers and return a status
 * message once the function has run.
 *
 * @param int $id
 *   The $id to be deleted
 *
 * @return string $status
 *   Prints a status string to the screen
*/
function TimeView_delete_submit($form, &$form_state) {
  if ($form_state['values']['id']) {
    try {
      $trn = db_transaction();
      db_delete('timeview_event')
               ->condition('id', $form_state['values']['id'])
               ->execute();
    } catch (Exception $e) {
      $trn->rollback();
      watchdog_exception('type', $e);
    }
  }
	$form_state['redirect'] = 'timeview';
}

/**
 * Creates an array of events for each livestock sampling
 * 
 * @return array(array) $samplingTrips
 *   An array of associative arrays containing the events
*/
function _TimeView_SamplingTrips() {
  $samplingTrips = array();
  if (module_exists('DataBridge')) {
    $events = DataBridge_SamplingTrips();
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
  if (module_exists('DataBridge')) {
    $events = DataBridge_WildlifeTrips();
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
function _TimeView_Input($form, &$form_state, $values) {
	$categories = array();
	$results = db_select('timeview_event','t')
											 ->distinct()
                       ->fields('t', array('category'))
                       ->execute();
	while ($result = $results->fetchAssoc()) {
		$categories[$result['category']] = $result['category'];
	}
	$categories['other'] = 'Other';
  $form['id'] =
    array('#type' => 'hidden',
    '#value' => $values['id'],
    );
  $form['name'] = 
    array('#type' => 'textfield',
	  '#title' => t('Name of the timeline event'),
	  '#size' => 75,
	  '#maxlength' => 255,
	  '#description' => t("Enter the name of the event you want to log"),
	  '#required' => TRUE,
	  '#default_value' => $values['name']
	  );
  $form['location'] = 
    array('#type' => 'textfield',
    '#title' => t('Location of the timeline event'),
    '#size' => 75,
    '#maxlength' => 255,
    '#description' => t("Enter the location of this event."),
    '#required' => TRUE,
	  '#default_value' => $values['location']
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
	  '#default_value' => $values['start_date']
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
	  '#default_value' => $values['end_date']
    );
  $form['category'] = 
    array('#type' => 'select',
    '#id' => 'timeview_category',
    '#title' => t('Category'),
    '#options' => $categories,
//array("Sampling Trip" => "Sampling Trip",
//                        "Meeting"       => "Meeting",
//                        "Visitor"       => "Visitor",
//                        "other"         => "Other",),
    '#description' => t("Select the category that this event falls in."),
    '#required' => FALSE,
    '#default_value' => $values['category'],
    );
  $form['category_other'] = 
    array('#type' => 'textfield',
    '#prefix' => '<div id="timeview_other">',
    '#suffix' => '</div>',
    '#size' => 30,
    '#maxlength' => 50,
    '#description' => t("Define your category here."),
    '#default_value' => $values['category']
    );
  $form['institutions'] = 
    array('#type' => 'checkboxes',
    '#prefix' => '<div class="timeview_checkboxes">',
    '#suffix' => '<div id="timeview_controls"></div></div>',
    '#title' => 'Involved Institutions',
    '#options' => array('ICIPE' => t('ICIPE'),
                        'KEMRI' => t('KEMRI'),
                        'DVS' => t('DVS'),
                        'KWS' => t('KWS'),
                        'MOH' => t('Ministry of Health'),
                        'KARI' => t('KARI'),
                        'ILRI' => t('ILRI'),
                        ),
    '#default_value' => explode(', ', $values['institutions']),
    );
  $form['comments'] = 
    array('#type' => 'textarea',
    '#title' => t('Comments and Remarks.'),
    '#description' => t("Enter comments or remarks for this trip. You can also add links to open new pages, or download documents"),
    '#required' => FALSE,
    '#default_value' => $values['comments']
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
  
  if (!empty($form_state['values']['end_date']) and (strtotime($form_state['values']['start_date']) > strtotime($form_state['values']['end_date']))) {
		form_set_error('event_end', t('The end date of the event cannot be before the start date.'));
  }
  
  if ($form_state['values']['category'] == 'other' and empty($form_state['values']['category_other'])) {
		form_set_error('category', t('You need to define a category for your event.'));
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
    $category = $form_state['values']['category'] == 'other' ? $form_state['values']['category_other'] : $form_state['values']['category'];
    $fields = array('name' => $form_state['values']['name'], 
                 'location' => $form_state['values']['location'], 
                 'start_date' => strtotime($form_state['values']['start_date']), 
                 'end_date' => strtotime($form_state['values']['end_date']) 
                               ? strtotime($form_state['values']['end_date']) 
                               : NULL, 
                 'institutions' => str_replace(', 0', '', implode(', ', $form_state['values']['institutions'])), 
                 'category' => $category,
                 'comments' => $form_state['values']['comments'], 
                 'documents' => $form_state['values']['documents'],);
    if (!$form_state['values']['id']) {
      $id = db_insert('timeview_event')
                      ->fields($fields)
                      ->execute();
    } else {
      $id = db_update('timeview_event')
                      ->fields($fields)
                      ->condition('id', $form_state['values']['id'], '=')
                      ->execute();
    }
  }
  catch (Exception $e) {
    $trn->rollback();
    watchdog_exception('type', $e);
  }
  
	$form_state['redirect'] = 'timeview';
  // Unless the transaction was rolled back, it gets commited here 
}
