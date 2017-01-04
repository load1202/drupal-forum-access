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

/**
 * Implements hook_query_TAG_alter().
 */
function forum_access_query_taxonomy_term_access_alter(Drupal\Core\Database\Query\AlterableInterface $query) {
  // Read meta-data from query, if provided.
  if (!$account = $query->getMetaData('account')) {
    $account = \Drupal::currentUser();
  }
  if (!$op = $query->getMetaData('op')) {
    $op = 'view';
  }

  // If $account can bypass node access, or there are no node access
  // modules, we don't need to alter the query.
  if ($account->hasPermission('bypass node access')) {
    return;
  }

  // Prevent duplicate records.
  $query->distinct();

  // Find all instances of the {taxonomy_term_data} table being joined --
  // could appear more than once in the query, and could be aliased.
  // Join each one to the forum_access table.
  $tables = $query->getTables();

  $rids = $account->getRoles();
  foreach ($tables as $talias => $tableinfo) {
    $table = $tableinfo['table'];
    if ($table == 'taxonomy_term_field_data') {
      // The node_access table has the access grants for any given node.
      $access_alias = $query->leftJoin('forum_access', 'fa', '%alias.tid = ' . $talias . '.tid');
      $acl_alias = $query->leftJoin('acl', 'acl', "%alias.figure = $talias.tid AND %alias.module = 'forum_access' AND %alias.name=:name",
        array(
          ':name' => 'moderate',
        )
      );
      $aclu_alias = $query->leftJoin('acl_user', 'aclu', "%alias.acl_id = $acl_alias.acl_id AND %alias.uid = :uid",
        array(
          ':uid' => $account->id(),
        )
      );
      $query->condition(db_or()
        ->isNull("$access_alias.rid")
        ->condition(db_and()
          ->condition("$access_alias.rid", $rids, 'IN')
          ->condition("$access_alias.grant_$op", 1, '>='))
        ->condition("$aclu_alias.uid", $account->id()));
    }
  }
}
