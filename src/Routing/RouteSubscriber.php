<?php
namespace Drupal\forum_access\Routing;

use Drupal\Core\Routing\RouteSubscriberBase;
use Symfony\Component\Routing\RouteCollection;

/**
 * Listens to the dynamic route events.
 */
class RouteSubscriber extends RouteSubscriberBase {

  /**
   * {@inheritdoc}
   */
  public function alterRoutes(RouteCollection $collection) {
    // Check access to the index forum page.
    if ($route = $collection->get('forum.index')) {
      // @todo Check if we can previous custom access without rewrite.
      $route->setRequirement('_custom_access', '\Drupal\forum_access\ForumAccess\Access::forumIndex');
    }

    // Check Access for the specific forum.
    if ($route = $collection->get('forum.page')) {
      $route->setRequirement('_custom_access', '\Drupal\forum_access\ForumAccess\Access::forumPage');
    }
  }
}
