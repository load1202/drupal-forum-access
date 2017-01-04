<?php
/**
 * @file
 *
 * Integration with taxonomy.
 */

/**
 * Implements hook_ENTITY_TYPE_delete().
 */
function forum_access_taxonomy_term_delete(Drupal\Core\Entity\EntityInterface $entity) {
  $vid = $entity->get('vid')->first()->getValue()['target_id'];

  // Delete {forum_access} records when forums are deleted.
  if ($vid == \Drupal::config('forum.settings')->get('vocabulary')) {
    \Drupal::database()->delete('forum_access')
      ->condition('tid', $entity->id())
      ->execute();
  }
}
