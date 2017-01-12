<?php
/**
 * @file
 *
 * Integration with node system.
 */
use Drupal\Core\Access\AccessResult;
use Drupal\node\Entity\Node;
use Drupal\node\NodeInterface;

/**
 * Implements hook_node_access().
 */
function forum_access_node_access(\Drupal\node\NodeInterface $node, $op, \Drupal\Core\Session\AccountInterface $account) {
  if ($node->bundle() != 'forum') {
    return AccessResult::neutral();
  }

  switch($op) {
    case 'view':
    case 'update':
    case 'delete':
      if (forum_access_entity_access_by_tid($op, $node)) {
        return AccessResult::allowed();
      }
      else {
        return AccessResult::forbidden();
      }
      break;
  }

  return AccessResult::neutral();
}

/**
 * Implements hook_node_grants().
 *
 * This function supplies the forum access grants. forum_access simply uses
 * roles as ACLs, so rids translate directly to gids.
 */
function forum_access_node_grants(\Drupal\Core\Session\AccountInterface $account, $op) {
  $roles_gids = \Drupal::configFactory()->getEditable('forum_access.settings')->get('forum_access_roles_gids');

  foreach($account->getRoles() as $role) {
    $grants['forum_access'][] = $roles_gids[$role];
  }

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

/**
 * Implements hook_ENTITY_TYPE_view_alter().
 */
function forum_access_node_view_alter(array &$build, Drupal\Core\Entity\EntityInterface $entity, \Drupal\Core\Entity\Display\EntityViewDisplayInterface $display) {
  $account = \Drupal::currentUser();
  if ($entity->bundle() != 'forum') {
    return;
  }

  // Don't show comment form is user has no access to it.
  if (!forum_access_entity_access_by_tid('create', $entity)) {
    $build['comment_forum'][0]['comment_form'] = array();
  }
}

/**
 * Implements hook_node_links_alter().
 */
function forum_access_node_links_alter(array &$links, NodeInterface $entity, array &$context) {
  $account = \Drupal::currentUser();
  if ($entity->bundle() != 'forum') {
    return;
  }

  // Don't show Comment link is user has no access to it.
  if (!forum_access_entity_access_by_tid('create', $entity)) {
    unset($links['comment__comment_forum']);
  }
}


/**
 * Implements hook_form_FORM_ID_alter().
 */
function forum_access_form_node_form_alter(&$form, \Drupal\Core\Form\FormStateInterface $form_state, $form_id) {
  if (!isset($form['taxonomy_forums'])) {
    return;
  }

  $node = $form_state->getFormObject()->getEntity();
  $account = \Drupal::currentUser();

  // True node administrators are all powerful and do NOT get their forms rewritten here.
  if ($account->hasPermission('bypass node access')) {
    return;
  }

  // We figure out what tids we want to show in form according to the forum access settings.
  $forum_access_settings = forum_access_get_settings_by_roles($account->getRoles(), 'create');
  $tids = array();
  foreach ($forum_access_settings as $setting) {
    $tids[$setting->tid] = $setting->tid;
  }

  // Also get all forums they happen to be able to moderate.
  $acl_access_settings = forum_access_get_settings_by_user('forum_access', $account->id(), 'moderate');
  foreach ($acl_access_settings as $setting) {
    $tids[$setting->tid] = $setting->tid;
  }

  // Make available current forum for existed node.
  if ($node = $form_state->getFormObject()->getEntity()) {
    $forum_tid = _forum_access_get_tid($node);
    $tids[$forum_tid] = $forum_tid;
  }

  // Leave only that forum options for which user has access.
  $form_options = &$form['taxonomy_forums']['widget']['#options'];
  foreach ($form_options as $tid => $name) {
    if (!isset($tids[$tid])) {
      unset($form_options[$tid]);
    }
  }
}
