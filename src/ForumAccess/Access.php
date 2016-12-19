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
    $view_access = forum_access_forum_check_view($account);

    if (!empty($view_access)) {
      return AccessResult::allowedIf(TRUE);
    }

    return AccessResult::allowedIf(FALSE);
  }

  /**
   * Access check for specific forum page.
   */
  public function forumPage(AccountInterface $account, TermInterface $taxonomy_term) {
    $view_access = forum_access_forum_check_view($account, $taxonomy_term->id());

    if (!empty($view_access)) {
      return AccessResult::allowedIf(TRUE);
    }

    return AccessResult::allowedIf(FALSE);
  }
}