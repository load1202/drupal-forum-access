<?php

/**
 * @file
 *
 * Integration with ACL.
 */
use Drupal\Core\Database\Database;

/**
 * Returns the ACL ID of the forum.
 */
function _forum_access_get_acl($tid, $name) {
  $acl_id = acl_get_id_by_name('forum_access', $name, $tid);

  // Create acl if doesn't exists.
  if (!$acl_id) {
    $acl_id = acl_create_acl('forum_access', $name, $tid);

    // Update all nodes which are related to this term with term's ACL.
    $connection = Database::getConnection();

    $subselect = $connection->select('taxonomy_index', 'n');
    $subselect
      ->fields('n', array('nid'))
      ->condition('n.tid', $tid);

    acl_add_nodes($subselect, $acl_id, 1, 1, 1);
  }
  return $acl_id;
}
