<?php

/**
 * @file
 * United States.
 */

/**
 * Returns a lat/lon pair of the approximate center of the given postal code in the given country.
 *
 * @param array $location
 *   An associative array $location where
 *     'street'       => the street portion of the location
 *     'supplemental' => additional street portion of the location
 *     'province'     => the province, state, or territory
 *     'country'      => lower-cased two-letter ISO code (REQUIRED)
 *     'postal_code'  => the international postal code for this location (REQUIRED)
 *
 * @return array
 *   An associative array where
 *      'lat' => approximate latitude of the center of the postal code's area
 *      'lon' => approximate longitude of the center of the postal code's area
 */
function location_latlon_rough_us($location = array()) {
  if (!isset($location['postal_code'])) {
    return NULL;
  }

  $row = db_query(
    "SELECT latitude, longitude FROM {zipcodes} where country = :country AND zip = :zip",
    array(
      ':country' => $location['country'],
      ':zip' => substr(str_pad($location['postal_code'], 5, '0', STR_PAD_LEFT), 0, 5),
    )
  )->fetchObject();

  if ($row) {
    return array('lat' => $row->latitude, 'lon' => $row->longitude);
  }
  else {
    return NULL;
  }
}

/**
 * Returns a lat/lon pair of the approximate center of the given postal code in the given country.
 *
 * @param array $location
 *   An associative array $location where only postal code and country are necessary, but can have the keys:
 *     'street'       => the street portion of the location
 *     'supplemental' => additional street portion of the location
 *     'province'     => the province, state, or territory
 *     'country'      => lower-cased two-letter ISO code (REQUIRED)
 *     'postal_code'  => the international postal code for this location (REQUIRED)
 *
 * @return array
 *   An associative array where
 *      'lat' => approximate latitude of the center of the postal code's area
 *      'lon' => approximate longitude of the center of the postal code's area
 */
function location_get_postalcode_data_us($location = array()) {
  if (!variable_get('location_us_full_zipcode_search', FALSE)) {
    $dash_index = strpos($location['postal_code'], '-');
    // First we strip slash off if we're dealing with a 9-digit US zipcode
    if (!($dash_index === FALSE)) {
      $location['postal_code'] = substr($location['postal_code'], 0, $dash_index);
    }
    $location['postal_code'] = str_pad($location['postal_code'], 5, "0", STR_PAD_LEFT);
  }

  // Now we pad the thing and query.
  $row = db_query("SELECT * FROM {zipcodes} where country = :country AND zip = :zip", array(
    ':country' => $location['country'],
    ':zip' => $location['postal_code']
  ))->fetchObject();
  if ($row) {
    return array(
      'lat' => $row->latitude,
      'lon' => $row->longitude,
      'city' => $row->city,
      'province' => $row->state,
      'country' => $row->country
    );
  }
  else {
    return NULL;
  }
}

/**
 * Exact us.
 *
 * @param array $location
 *   An associative array $location where
 *     'street'       => the street portion of the location
 *     'supplemental' => additional street portion of the location
 *     'province'     => the province, state, or territory
 *     'country'      => lower-cased two-letter ISO code (REQUIRED)
 *     'postal_code'  => the international postal code for this location (REQUIRED)
 *
 * @return NULL
 *   Deprecated.
 */
function location_latlon_exact_us($location = array()) {
  return NULL;
  // By uncommenting the line of code below, you legally acknowledge that you understand that you can only
  // do so under the terms of the Non-Commercial Share-alike license described at http://creativecommons.org/licenses/by-nc-sa/2.0/
  // return _location_latlon_exact_us_geocoder($location);
}

/**
 * Calls up a web-service to retrieve a lat/lon pair for a full, correct U.S. location.
 *
 * @param array $location
 *   An associative array that represents an location where
 *        'street'       => is the street location
 *        'supplemental' => any supplemental portion to the street location
 *        'city'         => city name
 *        'province'     => state, province, or territorial abbreviation
 *        'postal_code'  => postal code
 *
 * @return array
 *   An associative array where
 *        'lat' => Is a float value in latitude
 *        'lon' => Is a float value in longitude
 *    If the location supplied does not provide enough information, NULL is returned.
 *    "Enough information" means that there is either
 *    (a valid 'street' AND valid 'postal_code') OR (valid 'street' AND valid 'city' AND valid 'province')
 */
function _location_latlon_exact_us_geocoder($location = array()) {
  $location_string = '';
  if (isset($location['street']) && trim($location['street']) != '') {
    if (isset($location['postal_code'])) {
      $location_string = $location['street'] . ' ' . $location['postal_code'];
    }
    elseif (isset($location['city']) && isset($location['province']) && trim($location['city']) != '' && trim(
        $location['province']
      )
    ) {
      $location_string = $location['street'] . ', ' . $location['city'] . ', ' . $location['province'];
    }
    else {
      // Else geocoder.us won't do bidness with you.
      return NULL;
    }
  }
  else {
    return NULL;
  }

  $result = xmlrpc('http://rpc.geocoder.us/service/xmlrpc', array('geocode' => array($location_string)));

  if (is_array($result) && is_array($result[0]) && isset($result[0]['lat']) && is_numeric(
      $result[0]['lat']
    ) && isset($result[0]['long']) && is_numeric($result[0]['long'])
  ) {
    return array('lat' => $result[0]['lat'], 'lon' => $result[0]['long']);
  }

  return NULL;
}

/**
 * Yahoo link.
 */
function location_map_link_us_yahoo($location = array()) {
  $get_query = '?';
  if (isset($location['street'])) {
    $get_query .= 'addr=' . urlencode($location['street']) . '&amp;';
  }
  if ($location['province'] != '' || $location['city'] != '' || $location['postal_code'] != '') {
    $get_query .= 'csz=' . _location_us_yahoo_csz_get_field($location) . '&amp;';
  }
  $get_query .= 'country=' . urlencode($location['country']);

  return ('http://maps.yahoo.com/maps_result' . $get_query);
}

/**
 * Google link.
 */
function location_map_link_us_google($location = array()) {
  $query_params = array();
  $q = NULL;

  foreach (array('street', 'city', 'province', 'postal_code', 'country') as $field) {
    if (isset($location[$field])) {
      $query_params[] = $location[$field];
    }
  }

  if (location_has_coordinates($location)) {
    $q = urlencode($location['latitude'] . ' ' . $location['longitude'] . ' (' . implode(', ', $query_params) . ')');
  }
  else {
    if (count($query_params) > 0) {
      $q = urlencode(implode(", ", $query_params));
    }
  }

  if ($q != NULL) {
    return ('http://maps.google.com?q=' . $q);
  }
  else {
    return NULL;
  }
}

/**
 * MapQuest link.
 */
function location_map_link_us_mapquest($location = array()) {
  $get_query = '';
  if (isset($location['street'])) {
    $get_query .= 'address=' . urlencode($location['street']) . '&amp;';
  }

  if (isset($location['city'])) {
    $get_query .= 'city=' . urlencode($location['city']) . '&amp;';
  }

  if (isset($location['province'])) {
    $get_query .= 'state=' . urlencode($location['province']) . '&amp;';
  }

  if (isset($location['postal_code'])) {
    $get_query .= 'zipcode=' . urlencode($location['postal_code']);
  }
  if (strlen($get_query)) {
    return 'http://www.mapquest.com/maps/map.adp?searchtype=address&amp;country=US&amp;' . $get_query;
  }
  else {
    return NULL;
  }
}

/**
 * Providers for a links.
 *
 * @return array
 *   An array where
 *     -> the key is the word that helps identify the name of function that builds the link.  For example, a key of 'yahoo' means the name of the
 *        the function that builds a link to a map on Yahoo! Maps would be 'location_map_link_us_yahoo'
 *     -> the value is itself an array with 3 key/value pairs:
 *          'name' => points to the name of the mapping service.  For 'yahoo', this would be 'Yahoo! Maps'
 *          'url' => the url of the main page of the mapping service.  For 'yahoo', this would be 'http://maps.yahoo.com'
 *          'tos' => the url of the page that explains the map providers Terms of Service, or Terms of Use. For 'yahoo', this would be
 *                   'http://help.yahoo.com/help/us/maps/maps-24.html'
 */
function location_map_link_us_providers() {
  return array(
    'google' => array(
      'name' => t('Google Maps'),
      'url' => 'http://maps.google.com',
      'tos' => 'http://www.google.com/help/terms_local.html',
    ),
    'yahoo' => array(
      'name' => 'Yahoo! Maps',
      'url' => 'http://maps.yahoo.com',
      'tos' => 'http://help.yahoo.com/help/us/maps/maps-24.html',
    ),
    'mapquest' => array(
      'name' => 'MapQuest',
      'url' => 'http://www.mapquest.com',
      'tos' => 'http://www.mapquest.com/features/main.adp?page=legal',
    ),
  );
}

/**
 * Default providers.
 *
 * @return array
 *   An array of values that work as keys to the array returned by location_map_link_us_providers.  The idea is that if the
 *   administrator of the site has not yet had a chance to visit the "Map Links" subtab on the location module's settings page,
 *   that we can provide deep-linking to a relatively safe default.  By 'relatively safe', we mean that the Terms Of Service of
 *   the provider of the maps are flexible enough for most parties.
 *
 *   For the case of the U.S., 'google' has relatively flexible Terms Of Service, whereas Yahoo! Maps and MapQuest have more
 *   restrictive Terms Of Service.
 */
function location_map_link_us_default_providers() {
  return array('google');
}

/**
 * Geocode providers.
 */
function location_geocode_us_providers() {
  return array(
    'yahoo' => array(
      'name' => 'Yahoo! Maps Web Services',
      'url' => 'http://developer.yahoo.com/maps/rest/V1/geocode.html',
      'tos' => 'http://developer.yahoo.com/maps/mapsTerms.html',
    ),
  );
}

/**
 * Yahoo settings.
 */
function location_geocode_us_yahoo_settings() {
  $form = array();

  $form['location_geocode_us_yahoo_appid'] = array(
    '#type' => 'textfield',
    '#title' => t('Yahoo! Web Services Application ID'),
    '#size' => 64,
    '#maxlength' => 128,
    '#default_value' => variable_get('location_geocode_us_yahoo_appid', 'YahooDemo'),
    '#description' => t(
      'Unless you are using this site to test and develop, you will need to obtain a Yahoo! Web Services Application ID from the %network_link.  If you are using for development and testing purposes, you can use \'YahooDemo\' as your AppID.  When getting an Application ID from Yahoo!, be sure to review the %usage_policy.',
      array(
        '%network_link' => '<a href="http://api.search.yahoo.com/webservices/register_application">Yahoo! Developer Network</a>',
        '%usage_policy' => '<a href="http://developer.yahoo.com/usagePolicy/index.html">usage policy</a>',
      )
    ),
  );

  return $form;
}

/**
 * Geocode by Yahoo.
 */
function location_geocode_us_yahoo($location = array()) {
  $service_url = "http://api.local.yahoo.com/MapsService/V1/geocode?appid=" . variable_get(
      'location_geocode_us_yahoo_appid',
      "YahooDemo"
    ) . "&location=";

  $address = location_address2singleline($location);

  $http_reply = drupal_http_request($service_url . urlencode($address));

  // Address may have been improperly formatted or invalid.
  if ($http_reply->code == 400) {
    return NULL;
  }
  else {
    // Got a successful reply, but we only want to return if we have address-level precision.
    $matches = array();
    preg_match('/precision="([a-z]*)"/', $http_reply->data, $matches);

    if ($matches[1] != 'address') {
      // The precision we got back was not down to the street-address level.
      return NULL;
    }
    else {
      $lat_match = array();
      $lon_match = array();
      $latlon = array();

      if (preg_match('/<Latitude>(.*)<\/Latitude>/', $http_reply->data, $lat_match)) {
        $latlon['lat'] = $lat_match[1];
      }
      else {
        return NULL;
      }

      if (preg_match('/<Longitude>(.*)<\/Longitude>/', $http_reply->data, $lon_match)) {
        $latlon['lon'] = $lon_match[1];

        return $latlon;
      }
      else {
        return NULL;
      }
    }
  }
}

/**
 * Returns link.
 *
 * For now, assume site-admin wants American driving directions linked to Yahoo! Driving Directions.
 * Maybe later, we can add some kind of country-specific settings page that allows the site-admin to
 * decide which site to link to for driving directions.
 *
 * @param array $location_a
 *   is an associative array that represents a full location where
 *        'street'       => the street portions of the location
 *        'supplemental' => additional street portion of the location
 *        'province'     => the province, state, or territory
 *        'country'      => lower-cased two-letter ISO code (REQUIRED)
 *
 * @param array $location_b
 *   is associative array that represents a full location in the same way that
 *   parameter $location_b does.
 *
 * @returns string
 *   Link to driving directions.
 */
function location_driving_directions_link_us($location_a, $location_b) {
  return _location_driving_directions_link_us_yahoo($location_a, $location_b);
}

/**
 * Direction link by Yahoo.
 *
 * @param array $location_a
 *   is an associative array that represents a full location where
 *        'street'       => the street portions of the location
 *        'supplemental' => additional street portion of the location
 *        'province'     => the province, state, or territory
 *        'country'      => lower-cased two-letter ISO code (REQUIRED)
 *
 * @param array $location_b
 *   is associative array that represents a full location in the same way that
 *   parameter $location_b does.
 *
 * @return string
 *   a URL with HTTP GET variables
 *   Depending on how full the locationes are, the URL will either point to the driving directions
 *   on Yahoo! or, if only partial locationes are provided, a URL that points to the *form* for
 *   Yahoo! driving directions where the form is filled with whatever fields have been provided
 *   for the partial location(es).
 */
function _location_driving_directions_link_us_yahoo($location_a, $location_b) {
  if (trim($location_b['country']) != 'ca' && trim($location_b['country']) != 'us') {
    return '';
  }

  // These are the fields that need to be in each location if we are to provide a direct
  // link to yahoo directions.  If all of these fields don't have values, then we generate
  // a link to the *form* for Yahoo! driving directions rather than directly to the driving
  // directions themselves.

  foreach ($location_a as $field => $value) {
    $location_a[$field] = trim($value);
  }

  foreach ($location_b as $field => $value) {
    $location_b[$field] = trim($value);
  }

  if (_location_us_enough_fields_for_yahoo($location_a) && _location_us_enough_fields_for_yahoo($location_b)) {
    $yahoo_maps_path = 'dd_result';
  }
  else {
    $yahoo_maps_path = 'dd';
  }

  $get_query = '?';

  $get_query .= 'addr=' . urlencode($location_a['street']) . '&amp;';
  $get_query .= 'csz=' . _location_us_yahoo_csz_get_field($location_a) . '&amp;';
  $get_query .= 'country=' . urlencode($location_a['country']) . '&amp;';

  $get_query .= 'taddr=' . urlencode($location_b['street']) . '&amp;';
  $get_query .= 'tcsz=' . _location_us_yahoo_csz_get_field($location_b) . '&amp;';
  $get_query .= 'tcountry=' . urlencode($location_b['country']);

  $get_query .= '&amp;getrte=' . urlencode('Get Directions');

  return ('http://maps.yahoo.com/' . $yahoo_maps_path . $get_query);
}

/**
 * Fields fot Yahoo.
 */
function _location_us_enough_fields_for_yahoo($location) {
  // These are the fields that need to be in each location if we are to provide a direct
  // link to yahoo directions.  If all of these fields don't have values, then we generate
  // a link to the *form* for Yahoo! driving directions rather than directly to the driving
  // directions themselves.
  if (strlen($location['street']) && strlen($location['city']) && strlen($location['province'])) {
    return TRUE;
  }

  if (strlen($location['street']) && strlen($location['postal_code'])) {
    return TRUE;
  }

  if (strlen($location['street']) && strlen($location['city']) && strlen($location['province'])) {
    return TRUE;
  }

  return FALSE;
}

/**
 * Yahoo get fields.
 *
 * Don't mess with this function unless you understand its logic.  It has to do with
 * the question of "to comma or not to comma?"
 */
function _location_us_yahoo_csz_get_field($location) {
  // For some reasons, to the end of pinpointing a location, Yahoo! Maps and Driving Directions
  // do better a better job with retrieving info based strictly on a Canadian city/province
  // than on a Canadian postal code.
  if ($location['country'] = 'ca') {
    if (strlen($location['city']) && strlen($location['province'])) {
      return urlencode($location['city'] . ', ' . $location['province']);
    }

    if (strlen($location['postal_code'])) {
      return urlencode($location['postal_code']);
    }
  }
  else {
    if (strlen($location['postal_code'])) {
      return urlencode($location['postal_code']);
    }

    if (strlen($location['city']) && strlen($location['province'])) {
      return urlencode($location['city'] . ', ' . $location['province']);
    }
  }

  if (strlen($location['city']) || strlen($location['province'])) {
    if (strlen($location['city'])) {
      return urlencode($location['city']);
    }
    else {
      return urlencode($location['province']);
    }
  }

  return '';
}

/**
 * Geocoder oneline.
 */
function _location_us_geocoder_oneline($location = array()) {
  $line = '';

  $line .= $location['street'] . ', ';

  if (strlen($location['city']) && strlen($location['province']) && strlen($location['postal_code'])) {
    $line .= $location['city'] . ', ' . $location['province'] . ' ' . $location['postal_code'];
  }
  elseif (strlen($location['city']) && strlen($location['province'])) {
    $line .= $location['city'] . ', ' . $location['province'];
  }
  elseif (strlen($location['postal_code'])) {
    if (strlen($location['city']) || strlen($location['province'])) {
      if (strlen($location['city'])) {
        $line .= $location['city'] . ', ' . $location['postal_code'];
      }
      else {
        $line .= $location['province'] . ', ' . $location['postal_code'];
      }
    }
    else {
      $line .= $location['postal_code'];
    }
  }

  return $line;
}


/**
 * Location provinces.
 *
 * @return array
 *   An associative array of states/territories where
 *   -> the keys are integers starting from 1
 *   -> the values are the English names for those states/territories
 *
 * The states are grouped together at the beginning of the array and sorted
 * alphabetically.
 *
 * The territories are grouped together at the end of the array and sorted
 * alphabetically.
 */
function location_province_list_us() {
  return array(
    'AL' => t('Alabama'),
    'AK' => t('Alaska'),
    'AZ' => t('Arizona'),
    'AR' => t('Arkansas'),
    'CA' => t('California'),
    'CO' => t('Colorado'),
    'CT' => t('Connecticut'),
    'DE' => t('Delaware'),
    'DC' => t('District Of Columbia'),
    'FL' => t('Florida'),
    'GA' => t('Georgia'),
    'HI' => t('Hawaii'),
    'ID' => t('Idaho'),
    'IL' => t('Illinois'),
    'IN' => t('Indiana'),
    'IA' => t('Iowa'),
    'KS' => t('Kansas'),
    'KY' => t('Kentucky'),
    'LA' => t('Louisiana'),
    'ME' => t('Maine'),
    'MD' => t('Maryland'),
    'MA' => t('Massachusetts'),
    'MI' => t('Michigan'),
    'MN' => t('Minnesota'),
    'MS' => t('Mississippi'),
    'MO' => t('Missouri'),
    'MT' => t('Montana'),
    'NE' => t('Nebraska'),
    'NV' => t('Nevada'),
    'NH' => t('New Hampshire'),
    'NJ' => t('New Jersey'),
    'NM' => t('New Mexico'),
    'NY' => t('New York'),
    'NC' => t('North Carolina'),
    'ND' => t('North Dakota'),
    'OH' => t('Ohio'),
    'OK' => t('Oklahoma'),
    'OR' => t('Oregon'),
    'PA' => t('Pennsylvania'),
    'RI' => t('Rhode Island'),
    'SC' => t('South Carolina'),
    'SD' => t('South Dakota'),
    'TN' => t('Tennessee'),
    'TX' => t('Texas'),
    'UT' => t('Utah'),
    'VT' => t('Vermont'),
    'VA' => t('Virginia'),
    'WA' => t('Washington'),
    'WV' => t('West Virginia'),
    'WI' => t('Wisconsin'),
    'WY' => t('Wyoming'),
    'AS' => t('American Samoa'),
    'FM' => t('Federated States of Micronesia'),
    'GU' => t('Guam'),
    'MH' => t('Marshall Islands'),
    'MP' => t('Northern Mariana Islands'),
    'PW' => t('Palau'),
    'PR' => t('Puerto Rico'),
    'VI' => t('Virgin Islands'),
    'AA' => t('Armed Forces Americas'),
    'AE' => t('Armed Forces Europe'),
    'AP' => t('Armed Forces Pacific'),
  );
}


/**
 * Location provinces keyed by numeric.
 *
 * @return array
 *   An associative array of states/territories where
 *   -> the keys are integers starting from 1
 *   -> the values are the English names for those states/territories
 *
 * The states are grouped together at the beginning of the array and sorted
 * alphabetically.
 *
 * The territories are grouped together at the end of the array and sorted
 * alphabetically.
 *
 * Currently not being used, but may be in order to be compatible with CiviCRM
 * TODO: These numeric indices need to be changed to match those in the CiviCRM database
 */
function location_province_list_numeric_us() {
  return array(
    '001' => t('Alabama'),
    '002' => t('Alaska'),
    '003' => t('Arizona'),
    '004' => t('Arkansas'),
    '005' => t('California'),
    '006' => t('Colorado'),
    '007' => t('Connecticut'),
    '008' => t('Delaware'),
    '009' => t('District Of Columbia'),
    '010' => t('Florida'),
    '011' => t('Georgia'),
    '012' => t('Hawaii'),
    '013' => t('Idaho'),
    '014' => t('Illinois'),
    '015' => t('Indiana'),
    '016' => t('Iowa'),
    '017' => t('Kansas'),
    '018' => t('Kentucky'),
    '019' => t('Louisiana'),
    '020' => t('Maine'),
    '021' => t('Maryland'),
    '022' => t('Massachusetts'),
    '023' => t('Michigan'),
    '024' => t('Minnesota'),
    '025' => t('Mississippi'),
    '026' => t('Missouri'),
    '027' => t('Montana'),
    '028' => t('Nebraska'),
    '029' => t('Nevada'),
    '030' => t('New Hampshire'),
    '031' => t('New Jersey'),
    '032' => t('New Mexico'),
    '033' => t('New York'),
    '034' => t('North Carolina'),
    '035' => t('North Dakota'),
    '036' => t('Ohio'),
    '037' => t('Oklahoma'),
    '038' => t('Oregon'),
    '039' => t('Pennsylvania'),
    '040' => t('Rhode Island'),
    '041' => t('South Carolina'),
    '042' => t('South Dakota'),
    '043' => t('Tennessee'),
    '044' => t('Texas'),
    '045' => t('Utah'),
    '046' => t('Vermont'),
    '047' => t('Virginia'),
    '048' => t('Washington'),
    '049' => t('West Virginia'),
    '050' => t('Wisconsin'),
    '051' => t('Wyoming'),
    '052' => t('American Samoa'),
    '053' => t('Federated States of Micronesia'),
    '054' => t('Guam'),
    '055' => t('Marshall Islands'),
    '056' => t('Northern Mariana Islands'),
    '057' => t('Palau'),
    '058' => t('Puerto Rico'),
    '059' => t('Virgin Islands'),
  );
}

/**
 * Returns minimum and maximum latitude and longitude needed to create a bounding box.
 */
function location_bounds_us() {
  return array(
    'minlng' => -179.22745,
    'minlat' => -56.5109,
    'maxlng' => 158.80735,
    'maxlat' => 71.399467,
  );
}
