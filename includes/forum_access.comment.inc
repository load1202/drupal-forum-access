<?php
/**
 * @file
 *
 * Integration with comments.
 */
use Drupal\comment\CommentInterface;
use Drupal\node\NodeInterface;

/**
 * Implements hook_query_TAG_alter().
 */
function forum_access_query_comment_filter_alter(Drupal\Core\Database\Query\AlterableInterface $query) {
  $entity = $query->getMetaData('entity');

  if (!($entity instanceof NodeInterface && $entity->bundle() == 'forum')) {
    return;
  }

  $tid = $entity->get('taxonomy_forums')->first()->getValue()['target_id'];
  $access = array(
    'update' => forum_access_access('update', $tid),
    'delete' => forum_access_access('delete', $tid)
  );

  // If user has access to update or delete posts or is moderator of forum allow him to see unpublished comments.
  if (!($access['update'] || $access['delete'])) {
    return;
  }

  $conditions =& $query->conditions();

  foreach ($conditions as $key => $condition) {
    if (!is_numeric($key)) {
      continue;
    }

    if ($condition['field'] == 'c.status' && $condition['value'] == CommentInterface::PUBLISHED && $condition['operator'] == '=') {
      unset($conditions[$key]);
    }
  }
}