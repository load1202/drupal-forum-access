<?php
/**
 * @file
 *
 * Integration with user system.
 */
/**
 * Implements hook_ENTITY_TYPE_insert().
 */
function forum_access_user_role_insert(Drupal\Core\Entity\EntityInterface $entity) {
  $config = \Drupal::configFactory()->getEditable('forum_access.settings');
  $roles_gids = array_flip($config->get('forum_access_roles_gids'));

  $roles_gids[] = $entity->id();
  $config->set('forum_access_roles_gids', array_flip($roles_gids));
  $config->save();
}

/**
 * Implements hook_ENTITY_TYPE_delete().
 */
function forum_access_user_role_delete(Drupal\Core\Entity\EntityInterface $entity) {
  // Delete records from access tables.
  \Drupal::database()->delete('forum_access')
    ->condition('rid', $entity->id())
    ->execute();

  \Drupal::database()->delete('node_access')
    ->condition('gid', $entity->id())
    ->condition('realm', 'forum_access')
    ->execute();

  // Delete accordance between roles and numeric value.
  $config = \Drupal::configFactory()->getEditable('forum_access.settings');
  $roles_gids = $config->get('forum_access_roles_gids');
  unset($roles_gids[$entity->id()]);
  $config->set('forum_access_roles_gids', $roles_gids);
  $config->save();
}

/**
 * Check if user is moderator of specific forum.
 */
function forum_access_is_moderator($account, $tid) {
  $acl_id = acl_get_ids_by_user('forum_access', $account->id(), 'moderate', $tid);

  return !empty($acl_id);
}
