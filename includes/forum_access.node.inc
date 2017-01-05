<?php
/**
 * @file
 *
 * Integration with node system.
 */
use Drupal\Core\Access\AccessResult;

/**
 * Implements hook_node_grants().
 *
 * This function supplies the forum access grants. forum_access simply uses
 * roles as ACLs, so rids translate directly to gids.
 */
function forum_access_node_grants(\Drupal\Core\Session\AccountInterface $account, $op) {
  // @todo Gid can be only numeric so we convert role mchine_name to integer.
  $roles_gids = \Drupal::configFactory()->getEditable('forum_access.settings')->get('forum_access_roles_gids');
  $grants['forum_access'] = $roles_gids;
  return $grants;
}

/**
 * Implements hook_node_access_records().
 *
 * Returns a list of grant records for the passed in node object.
 * Checks to see if maybe we're being disabled.
 */
function forum_access_node_access_records(\Drupal\node\NodeInterface $node) {
  static $seers;

  $grants = &drupal_static(__FUNCTION__, array());
  $seers = &drupal_static(__FUNCTION__ . '__seers');
  $tid = _forum_access_get_tid($node);
  $roles_gids = \Drupal::configFactory()->getEditable('forum_access.settings')->get('forum_access_roles_gids');

  if ($tid) {
    if (!isset($grants[$tid])) {
      if (!isset($seers)) {
        $seers = user_roles(FALSE, 'bypass node access');
      }
      $result = forum_access_get_grants_by_tid($tid);
      foreach ($result as $grant) {
        if (isset($seers[$grant->rid])) {
          continue; // Don't provide any useless grants!
        }
        $grants[$tid][] = array(
          'realm'        => 'forum_access',
          // We change machine_name of role to its numeric equivalent.
          'gid'          => $roles_gids[$grant->rid],
          'grant_view'   => $grant->grant_view,
          'grant_update' => $grant->grant_update,
          'grant_delete' => $grant->grant_delete,
          'priority'     => $grant->priority,
        );
      }
    }

    if (isset($grants[$tid])) {
      return $grants[$tid];
    }
  }
}
