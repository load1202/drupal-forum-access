<?php
/**
 * @file
 *
 * Integration with comments.
 */
use Drupal\comment\CommentInterface;
use Drupal\node\NodeInterface;
use Drupal\Core\Access\AccessResult;

/**
 * Implements hook_query_TAG_alter().
 */
function forum_access_query_comment_filter_alter(Drupal\Core\Database\Query\AlterableInterface $query) {
  $entity = $query->getMetaData('entity');

  if (!($entity->getEntityTypeId() == 'node' && $entity->bundle() == 'forum')) {
    return;
  }

  // @todo Replace it with the function.
  $tid = $entity->get('taxonomy_forums')->first()->getValue()['target_id'];
  $access = array(
    'update' => forum_access_access('update', $tid),
    'delete' => forum_access_access('delete', $tid)
  );

  // If user has access to update or delete posts or he is moderator of forum allow him to see unpublished comments.
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

/**
 * Implements hook_ENTITY_TYPE_access().
 */
function forum_access_comment_access(\Drupal\Core\Entity\EntityInterface $entity, $operation, \Drupal\Core\Session\AccountInterface $account) {
  // Get parent entity of comment.
  $parent = $entity->get('entity_id')->first()->get('entity')->getTarget()->getValue();

  if (!($parent->getEntityTypeId() == 'node' && $parent->bundle() == 'forum')) {
    return AccessResult::neutral();
  }

  // @todo Check for Reply.
  // @todo Check for approve.
  if ($account->hasPermission('administer comments')) {
    return AccessResult::neutral();
  }

  if ($tid = _forum_access_get_tid($parent)) {
    $comment_author = $entity->get('uid')->first()->get('entity')->getTarget()->getValue();
    // Check if user is author of comment.
    $is_author = $account->id() == $comment_author->id();

    if ($operation == 'update'
        && (forum_access_access('update', $tid) || $account->hasPermission('edit any forum content') || $is_author && $account->hasPermission('edit own forum content'))) {
      return AccessResult::allowedIf(TRUE);
    }
    elseif ($operation == 'delete'
        && (forum_access_access('delete', $tid) || $account->hasPermission('delete any forum content') || $is_author && $account->hasPermission('delete own forum content'))) {
      return AccessResult::allowedIf(TRUE);
    }
  }

  return AccessResult::allowedIf(FALSE);
}
