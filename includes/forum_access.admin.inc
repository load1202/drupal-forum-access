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
  $tid = $form['tid']['#value'];

  // Basic fieldset.
  _forum_access_basic_taxonomy_term_fieldset($form);

  // Moderators fields.
  $form['forum_access']['permissions'] = _forum_access_forum_permissions_form();
  $form['forum_access']['moderators'] = _forum_access_forum_moderators_form($form_state, $tid);
  $form['actions']['submit']['#submit'][] = '_forum_access_form_submit';
}

/**
 * Get basic fieldset for taxonomy_term forum.
 */
function _forum_access_basic_taxonomy_term_fieldset(&$form) {
  // Add access fields to forum sections.
  $form['forum_access'] = array(
    '#type' => 'fieldset',
    '#title' => t('Access control'),
    '#collapsible' => TRUE,
    '#tree' => TRUE,
  );
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
  $tid = $form['tid']['#value'];

  // Basic fieldset.
  _forum_access_basic_taxonomy_term_fieldset($form);

  // Moderators fields.
  $form['forum_access']['moderators'] = _forum_access_forum_moderators_form($form_state, $tid);
  $form['actions']['submit']['#submit'][] = '_forum_access_form_submit';
}

/**
 * List of permissions.
 */
function _forum_access_forum_permissions_form() {
  $variables = array();
  $permissions = array(
    'access content' => 'node',
    'access comments' => 'comment',
    'create forum content' => 'node',
    'post comments' => 'comment',
    'skip comment approval' => 'comment',
    'edit own forum content' => 'node',
    'edit any forum content' => 'node',
    'delete own forum content' => 'node',
    'delete any forum content' => 'node',
    'administer comments' => 'comment',
    'administer forums' => 'forum',
    'administer nodes' => 'node',
    'access content overview' => 'node',
    'view own unpublished content' => 'node',
    'edit own comments' => 'comment',
  );

  foreach ($permissions as $perm => $module) {
    $key = '@' . str_replace(' ', '_', $perm);
    $variables[$key] = _forum_access_permission_link($module, $perm);
  }

  $form = array(
    '#type' => 'fieldset',
    '#title' => t('Permissions information'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['list'] = array(
    '#theme' => 'item_list',
    '#title' => t('Note that users need'),
    '#items' => array(
      t('the <strong>@access_content</strong> and <strong>@access_comments</strong> permissions <strong>AND <em>View</em></strong> to be able to see this forum and its content at all,', $variables),
      t('the <strong>@create_forum_content</strong> (and similar) permissions <strong>AND <em>Post</em></strong> to be able to create forum content, and', $variables),
      t('the <strong>@post_comments</strong> (and optionally <strong>@skip_comment_approval</strong>) permission AND <em>Post</em> to be able to post comments/replies;', $variables),
      t('the <strong>@edit_own_forum_content</strong> or <strong>@edit_any_forum_content</strong> (and similar) permissions (<strong>OR <em>Edit</em></strong>) can be added if desired, <strong>plus</strong>', $variables),
      t('the <strong>@delete_own_forum_content</strong> or <strong>@delete_any_forum_content</strong> (and similar) permissions (<strong>OR <em>Delete</em></strong>) if desired;', $variables),
      t('the <strong>@administer_comments</strong> (global!) permission <strong>OR <em>Edit</em>/<em>Delete</em></strong> to be able to edit/delete comments;', $variables),
      t('the <strong>@administer_forums</strong> permission <strong>AND <em>View</em></strong> to be able to administer forums (and change access!).', $variables),
      t('Furthermore note that content which is not published is treated in a different way by Drupal: it can be viewed only by its author
        (with the <strong>@view_own_unpublished_content</strong> permission) or by users with the <strong>@administer_nodes</strong> permission. Unpublished comments and
        replies are accessible to users with <strong><em>Edit</em> OR <em>Delete</em></strong>, <strong>OR</strong> with the <strong>@administer_comments</strong> permission,
        but they are never counted on the forum page.', $variables),
      t('The global <strong>@edit_own_comments</strong> permission is ignored, but the edit/delete forum content permissions are extended to comments;
        the per-forum <strong><em>Edit</em></strong> and <strong><em>Delete</em></strong> apply to both nodes and comments, too.', $variables),
    ),
  );
  return $form;
}

/**
 * Generate permissions link.
 */
function _forum_access_permission_link($module, $permission) {
  $permissions = &drupal_static(__FUNCTION__, array());

  if (empty($permissions)) {
    $permission_handler = \Drupal::service('user.permissions');
    $permissions_data = $permission_handler->getPermissions();

    foreach ($permissions_data as $perm => $perm_item) {
      $provider = $perm_item['provider'];
      $permissions[$provider][$perm] = strip_tags($perm_item['title']);
    }
  }

  return $permissions[$module][$permission];
}
