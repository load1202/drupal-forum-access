<?php
/**
 * @file
 *
 * Integration with user system.
 */

/**
 * Implements hook_ENTITY_TYPE_delete().
 */
function forum_access_user_role_delete(Drupal\Core\Entity\EntityInterface $entity) {
  \Drupal::database()->delete('forum_access')
    ->condition('rid', $entity->id())
    ->execute();

  \Drupal::database()->delete('node_access')
    ->condition('gid', $entity->id())
    ->condition('realm', 'forum_access')
    ->execute();
}
