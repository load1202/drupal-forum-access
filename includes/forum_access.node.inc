<?php
/**
 * @file
 *
 * Integration with node system.
 */
use Drupal\Core\Access\AccessResult;
use \Drupal\Core\Render\Element;
use Drupal\node\Entity\Node;
use Drupal\node\NodeInterface;

/**
 * Implements hook_ENTITY_TYPE_create_access().
 */
function forum_access_node_create_access(\Drupal\Core\Session\AccountInterface $account, array $context, $entity_bundle) {
  if ($entity_bundle != 'forum') {
    return AccessResult::neutral();
  }

  // If we are on forum page we want to restrict access to create forum according to the forum access rules.
  $route = \Drupal::routeMatch();
  if ($route->getCurrentRouteMatch()->getRouteName() == 'forum.page') {
    $term = $route->getParameter('taxonomy_term');
    if (forum_access_access('create', $term->id())) {
      return AccessResult::allowed();
    }
    else {
      return AccessResult::forbidden();
    }
  }

  return AccessResult::neutral();
}
/**
 * Implements hook_node_access().
 */
function forum_access_node_access(\Drupal\node\NodeInterface $node, $op, \Drupal\Core\Session\AccountInterface $account) {
  if ($node->bundle() != 'forum') {
    return AccessResult::neutral();
  }

  // Disallow if user has no access to view.
  if (!forum_access_entity_access_by_tid('view', $node)) {
    return AccessResult::forbidden();
  }
  $author = $node->get('uid')->first()->get('entity')->getTarget()->getValue();

  switch($op) {
    case 'view':
      if ($node->isPublished()) {
        $access = forum_access_entity_access_by_tid($op, $node);
      }
      elseif ($author->id() == $account->id() && $account->hasPermission('view own unpublished content')) {
        $access = forum_access_entity_access_by_tid($op, $node);
      }
      break;

    case 'update':
      if ($op == 'update' && ($account->hasPermission('edit any forum content') || ($author->id() == $account->id() && $account->hasPermission('edit own forum content')))) {
        $access = forum_access_entity_access_by_tid('view', $node);
      }
      break;

    case 'delete':
      if ($op == 'delete' && ($account->hasPermission('delete any forum content') || ($author == $account->id() && $account->hasPermission('edit own forum content')))) {
        $access = forum_access_entity_access_by_tid('view', $node);
      }
      break;
  }

  if (isset($access)) {
    if ($access) {
      return AccessResult::allowed();
    }
    else {
      return AccessResult::forbidden();
    }
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
    // We want to give our rights only for published nodes.
    if ($node->isPublished()) {
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
}

/**
 * Implements hook_ENTITY_TYPE_insert().
 */
function forum_access_node_insert(Drupal\Core\Entity\EntityInterface $entity) {
  if ($entity->bundle() != 'forum') {
    return;
  }

  if ($tid = _forum_access_get_tid($entity)) {
    $acl_id = _forum_access_get_acl($tid, 'moderate');
    acl_node_add_acl($entity->id(), $acl_id, 1, 1, 1);
  }
}

/**
 * Implements hook_ENTITY_TYPE_update().
 */
function forum_access_node_update(Drupal\Core\Entity\EntityInterface $entity) {
  if ($entity->bundle() != 'forum') {
    return;
  }

  $old_tid = _forum_access_get_tid($entity->original);
  $current_tid = _forum_access_get_tid($entity);
  if (isset($old_tid)) {
    if ($current_tid == $old_tid) {
      return;
    }
    acl_node_clear_acls($entity->id(), 'forum_access');
  }

  // For changed and for previously unassigned terms we need to fake an insert.
  forum_access_node_insert($entity);
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

  // Get list of available lists.
  $tids = forum_access_form_node_form_get_available_tids($account, $node);
  $forum_tid = _forum_access_get_tid($node);

  // Leave only that forum options for which user has access.
  $form_options = &$form['taxonomy_forums']['widget']['#options'];
  foreach ($form_options as $tid => $name) {
    if (!isset($tids[$tid])) {
      unset($form_options[$tid]);
    }
  }

  // Apply modifications for Moderators (by role or uid).
  if (!$account->hasPermission('administer nodes') && forum_access_is_moderator($account, $forum_tid)) {
    foreach (Element::children($form) as $key) {
      switch ($key) {
        case 'sticky':
          $form[$key]['#access'] = TRUE;
          break;
        case 'revision':
          $form['revision']['#access'] = TRUE;
          break;
        case 'comment_forum':
          $form[$key]['#access'] = TRUE;
          break;
        case 'shadow':
          $form[$key]['#description'] .= ' ' . t('Note: Access to this topic and its shadows is controlled by the forum that contains the topic.');
          break;
        case 'taxonomy_forums':
          $form[$key]['widget']['#description'] .= t('Note: Moving a topic to a different forum may change its accessibility.');
        break;
      }
    }
  }
}

/**
 * Get list of available tids for specific account.
 */
function forum_access_form_node_form_get_available_tids($account, $node) {
  // We figure out what tids we want to show in form according to the forum access settings.
  $forum_access_settings = forum_access_get_settings_by_roles($account->getRoles(), 'create');
  $tids = array();
  foreach ($forum_access_settings as $setting) {
    $tids[$setting->tid] = $setting->tid;
  }

  // Also get all forums they happen to be able to moderate.
  $acl_access_settings = forum_access_get_settings_by_user('forum_access', $account->id(), 'moderate');
  foreach ($acl_access_settings as $setting) {
    $tids[$setting->figure] = $setting->figure;
  }

  // Make available current forum for existed node.
  if (!empty($node)) {
    $forum_tid = _forum_access_get_tid($node);
    $tids[$forum_tid] = $forum_tid;
  }

  return $tids;
}
