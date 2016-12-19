<?php
/**
 * @file
 *
 * Handle common forum access functionality.
 */

/**
 * Check access to view forums and containers.
 */
function forum_access_forum_check_view($account, $tid = NULL) {
  // Check if user is moderator at least in one forum.
  $acl_moderate = acl_get_ids_by_user('forum_access', $account->id(), 'moderate', $tid);
  if (!empty($acl_moderate)) {
    return TRUE;
  }

  // Check for general permissions.
  if ($account->hasPermission('bypass node access')) {
    return TRUE;
  }
  if (!$account->hasPermission('access content')) {
    return FALSE;
  }

  // Check if user has access to forum according to his role.
  $view_access = forum_access_check_operation_taxonomy_term('view', $account, $tid);

  if (!empty($view_access)) {
    return TRUE;
  }

  return FALSE;
}
/**
 * Check if user has access to specific operation to forum section(by taxonomy_term).
 */
function forum_access_check_operation_taxonomy_term($op, $account, $tid = NULL) {
  $settings['op'] = $op;
  $settings['tid'] = $tid;

  $query = \Drupal::database()->select('forum_access', 'fa')
    ->fields('fa', array('rid'));

  forum_access_modify_check_query($query, $settings);

  $query->condition('fa.rid', $account->getRoles(), 'IN');

  return $query->countQuery()
    ->execute()
    ->fetchField();
}

/**
 * Modify query according to settings.
 */
function forum_access_modify_check_query($query, $settings) {
  // Modify by operation.
  switch ($settings['op']) {
    case 'view':
      $query->condition('grant_view', 0, '>');
      break;
    case 'update':
      $query->condition('grant_update', 0, '>');
      break;
    case 'delete':
      $query->condition('grant_delete', 0, '>');
      break;
    case 'create':
      $query->condition('grant_delete', 0, '>');
      break;
  }

  // Modify by tid.
  if (!empty($settings['tid'])) {
    $query->condition('tid', $settings['tid']);
  }
}