<?php

/**
 * @file
 * Leaflet maps views integration.
 */

/**
 * Define leaflet views style.
 *
 * Implements hook_views_plugins().
 */
function leaflet_views_views_plugins() {
  $plugins = array(
    'module' => 'leaflet_views',
    'style' => array(
      'leaflet' => array(
        'title' => t('Leaflet Map'),
        'help' => t('Displays a View as a Leaflet map.'),
        'path' => drupal_get_path('module', 'leaflet'),
        'handler' => 'leaflet_views_plugin_style',
        'theme' => 'leaflet-map',
        'uses fields' => TRUE,
        'uses row plugin' => FALSE,
        'uses options' => TRUE,
        'uses grouping' => FALSE,
        'type' => 'normal',
        'even empty' => TRUE,
      ),
    ),
  );

  return $plugins;
}

/**
 * @class
 * Extension of the Views Plugin Syle for Leaflet Map
 * Adapted from the GeoField Map views module.
 */
class leaflet_views_plugin_style extends views_plugin_style {

  /**
   * Set default options
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['map'] = array('default' => '');
    $options['data_source'] = array('default' => '');
    $options['popup_source'] = array('default' => '');
    $options['icon'] = array('default' => '');
    $options['height'] = array('default' => '400');
    return $options;
  }

  /**
   * Options form
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $handlers = $this->display->handler->get_handlers('field');

    $data_source_options = $popup_source_options = array(
      '' => '<none>',
    );

    foreach ($handlers as $handle) {
      $popup_source_options[$handle->options['id']] = (!empty($handle->options['label'])) ? $handle->options['label'] : $handle->options['id'];

      if (!empty($handle->field_info['type']) && $handle->field_info['type'] == 'geofield') {
        $data_source_options[$handle->options['id']] = $handle->options['label'];
      }
    }

    $map_options = array('' => t('-- Select --'));
    foreach (leaflet_map_get_info() as $key => $map) {
      $map_options[$key] = t($map['label']);
    }

    if (count($data_source_options) == 1) {
      $form['error'] = array(
        '#markup' => 'Please add at least 1 geofield to the view',
      );
    }
    else {

      // Map preset.
      $form['data_source'] = array(
        '#type' => 'select',
        '#title' => t('Data Source'),
        '#description' => t('Which field contains geodata?'),
        '#options' => $data_source_options,
        '#default_value' => $this->options['data_source'] ? $this->options['data_source'] : '',
      '#required' => TRUE,
      );

      $form['popup_source'] = array(
        '#type' => 'select',
        '#title' => t('Popup Text'),
        '#options' => $popup_source_options,
        '#default_value' => $this->options['popup_source'] ? $this->options['popup_source'] : '',
      );

      $form['map'] = array(
        '#title' => t('Map'),
        '#type' => 'select',
        '#options' => $map_options,
        '#default_value' => $this->options['map'] ? $this->options['map'] : '',
        '#required' => TRUE,
      );
      
      $form['height'] = array(
        '#title' => t('Height'),
        '#type' => 'textfield',
        '#default_value' => $this->options['height'] ? $this->options['height'] : '',
        '#size' => 4,
        '#field_suffix' => t('px'),
      );
    }
  }

  /**
   * Renders view
   */
  function render() {
    if (!empty($this->view->live_preview)) {
      return 'No preview available';
    }
    $style_options = $this->view->style_options;

    $geo_data = (!empty($style_options['data_source'])) ? 'field_' . $style_options['data_source'] : NULL;
    $popup_data = (!empty($style_options['popup_source'])) ? $style_options['popup_source'] : NULL;
    $map_data = (!empty($style_options['map'])) ? $style_options['map'] : NULL;
    $height = (!empty($style_options['height'])) ? $style_options['height'] : '400';

    $data = array();
    if ($geo_data && $popup_data) {
      $this->render_fields($this->view->result);

      foreach ($this->view->result as $id => $result) {
        $geofield = (!empty($result->$geo_data)) ? $result->$geo_data : NULL;

        if (!empty($geofield)) {
          $description = $this->rendered_fields[$id][$popup_data];
          $raw_data = array();
          foreach ($geofield as $item) {
            $raw_data[] = $item['raw'];
          }
          $points = leaflet_process_geofield($raw_data);
          array_walk($points, create_function('&$p,$i,$t', '$p[\'popup\'] = $t;'), $description);
          $data = array_merge($data, $points);
        }
      }
      $map = leaflet_map_get_info($map_data);
      return leaflet_render_map($map, $data, $height);
    }
   return '';
  }
}
