<?php
/**
 * @file
 *
 * Integration with node system.
 */
use Drupal\Core\Access\AccessResult;

/**
 * Implements hook_node_access().
 */
function forum_access_node_access(\Drupal\node\NodeInterface $node, $op, \Drupal\Core\Session\AccountInterface $account) {
  if ($node->bundle() != 'forum') {
    return AccessResult::neutral();
  }

  // Check if user is moderator.
  $forum = $node->get('taxonomy_forums')->first()->getValue()['target_id'];
  $acl_moderate = acl_get_ids_by_user('forum_access', $account->id(), 'moderate', $forum);
  if (!empty($acl_moderate)) {
    return AccessResult::allowedIf(TRUE);
  }
  return AccessResult::allowedIf(FALSE);
}
