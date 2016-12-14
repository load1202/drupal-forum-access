<?php

/**
 * @file
 * Contains integrations with forms.
 */
use Drupal\Core\Form\FormStateInterface;

/**
 * Implements hook_form_FORM_ID_alter().
 */
function forum_access_form_taxonomy_term_forums_forum_form_alter(&$form, \Drupal\Core\Form\FormStateInterface $form_state, $form_id) {
  forum_access_taxonomy_term_access_form($form, $form_state);
}

/**
 * General wrapper for forum and container administrative forms.
 */
function forum_access_taxonomy_term_access_form(&$form, \Drupal\Core\Form\FormStateInterface $form_state) {
  $tid = $form['tid']['#value'];
  // Add access fields to forum sections.
  $form['forum_access'] = array(
    '#type' => 'fieldset',
    '#title' => t('Access control'),
    '#collapsible' => TRUE,
    '#tree' => TRUE,
  );

  $form['forum_access']['moderators'] = _forum_access_forum_moderators_form($form_state, $tid);

  $form['actions']['submit']['#submit'][] = '_forum_access_form_submit';
}

/**
 * Create moderators list of users.
 */
function _forum_access_forum_moderators_form($form_state, $tid) {
  // Find our moderator ACL. If tid exists it means that term isn't new and is edited now.
  if (isset($tid)) {
    $acl_id = _forum_access_get_acl($tid, 'moderate');
    $form = acl_edit_form($form_state, $acl_id, t('Moderators'));
    $form['warning'] = array(
      '#type'   => 'item',
      '#markup' => t('Moderators receive all grants above.'),
    );
    $form['note'] = array(
      '#type'   => 'item',
      '#markup' => t('Note: Changes to moderators are not saved until you click Save below.'),
    );

    return $form;
  }
}

/**
 * Submit handler for forum access administrative form.
 */
function _forum_access_form_submit($form, FormStateInterface $form_state) {
  // Save moderators list.
  _forum_access_form_moderators_save($form_state);
}

/**
 * Save ACL configuration.
 */
function _forum_access_form_moderators_save(FormStateInterface $form_state) {
  $data = $form_state->getValue(['forum_access', 'moderators']);

  if (!empty($data)) {
    acl_save_form($data, 0);
  }
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function forum_access_form_taxonomy_term_forums_container_form_alter(&$form, \Drupal\Core\Form\FormStateInterface $form_state, $form_id) {
  forum_access_taxonomy_term_access_form($form, $form_state);
}
