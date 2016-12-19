<?php
/**
 * @file
 *
 * Theming, preprocess and process functions.
 */
use \Drupal\Core\Render\Element;

/**
 * Preprocess for output of form with roles and permissions.
 */
function template_preprocess_forum_access_table(&$variables) {
  $checkboxes = &$variables['form']['checkboxes'];

  foreach (Element::children($checkboxes) as $index => $child) {
    foreach (Element::children($checkboxes[$child]) as $index2 => $child2) {
      $variables['form']['rows'][$child2][] = $checkboxes[$child][$child2];
    }
  }
}