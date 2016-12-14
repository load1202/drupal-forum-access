<?php
namespace Drupal\forum_access\ForumAccess;

use Drupal\Core\Session\AccountInterface;
use Drupal\Core\Access\AccessResult;
use Drupal\node\NodeInterface;
use Drupal\taxonomy\TermInterface;

/**
 * Access checks for forum.
 */
class Access {
  /**
   * Access check for forum index page.
   */
  public function forumIndex(AccountInterface $account) {
    // Check if user is moderator at least in one forum.
    $acl_moderate = acl_get_ids_by_user('forum_access', $account->id(), 'moderate');
    if (!empty($acl_moderate)) {
      return AccessResult::allowedIf(TRUE);
    }

    return AccessResult::allowedIf(FALSE);
  }

  /**
   * Access check for specific forum page.
   */
  public function forumPage(AccountInterface $account, TermInterface $taxonomy_term) {
    // Check if user is moderator at least in one forum.
    $acl_moderate = acl_get_ids_by_user('forum_access', $account->id(), 'moderate', $taxonomy_term->id());
    if (!empty($acl_moderate)) {
      return AccessResult::allowedIf(TRUE);
    }

    return AccessResult::allowedIf(FALSE);
  }
}
