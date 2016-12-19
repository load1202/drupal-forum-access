<?php

/**
 * @file
 * Contains integrations with forms.
 */
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountInterface;
use Drupal\taxonomy\Entity\Vocabulary;

/**
 * Implements hook_form_FORM_ID_alter().
 */
function forum_access_form_taxonomy_term_forums_forum_form_alter(&$form, \Drupal\Core\Form\FormStateInterface $form_state, $form_id) {
  $tid = $form['tid']['#value'];
  $settings = _forum_access_get_settings($tid);

  // Basic fieldset.
  _forum_access_basic_taxonomy_term_fieldset($form);

  // Moderators fields.
  $form['forum_access']['permissions'] = _forum_access_forum_permissions_form();
  $form['forum_access']['grants'] = _forum_access_forum_grants_form($form_state, $settings);
  $form['forum_access']['moderators'] = _forum_access_forum_moderators_form($form_state, $tid);
  $form['actions']['submit']['#submit'][] = '_forum_access_form_submit';
}

/**
 * Get basic fieldset for taxonomy_term forum.
 */
function _forum_access_basic_taxonomy_term_fieldset(&$form) {
  // Add access fields to forum sections.
  $form['forum_access'] = array(
    '#type' => 'details',
    '#title' => t('Access control'),
    '#collapsible' => TRUE,
    '#tree' => TRUE,
  );
}

/**
 * Create moderators list of users.
 */
function _forum_access_forum_moderators_form($form_state, $tid) {
  // Find our moderator ACL. If tid exists it means that term isn't new and is edited now.
  if (isset($tid)) {
    $acl_id = _forum_access_get_acl($tid, 'moderate');
    $form = acl_edit_form($form_state, $acl_id, t('Moderators'));
    $form['warning'] = array(
      '#type'   => 'item',
      '#markup' => t('Moderators receive all grants above.'),
    );
    $form['note'] = array(
      '#type'   => 'item',
      '#markup' => t('Note: Changes to moderators are not saved until you click Save below.'),
    );

    return $form;
  }
}

/**
 * Submit handler for forum access administrative form.
 */
function _forum_access_form_submit($form, FormStateInterface $form_state) {
  // Save roles permissions.
  _forum_access_form_roles_permissions_save($form_state);

  // Save moderators list.
  _forum_access_form_moderators_save($form_state);
}

/**
 * Save ACL configuration.
 */
function _forum_access_form_moderators_save(FormStateInterface $form_state) {
  $data = $form_state->getValue(['forum_access', 'moderators']);

  if (!empty($data)) {
    acl_save_form($data, 0);
  }
}

/**
 * Save roles permissions.
 */
function _forum_access_form_roles_permissions_save(FormStateInterface $form_state) {
  $access = $form_state->getValue(['forum_access', 'grants', 'checkboxes']);

  // Remove and re-create records.
  $tid = $form_state->getValue('tid');

  \Drupal::database()->delete('forum_access')
    ->condition('tid', $tid)
    ->execute();

  // Insert new values.
  $insert = \Drupal::database()->insert('forum_access');
  $insert->fields(
    array(
      'tid',
      'rid',
      'grant_view',
      'grant_update',
      'grant_delete',
      'grant_create',
      'priority'
    )
  );

  foreach ($access['view'] as $rid => $checked) {
    $insert->values(
      array(
        'tid' => $tid,
        'rid' => $rid,
        'grant_view' => (int) !empty($checked),
        'grant_update' => (int) !empty($access['update'][$rid]),
        'grant_delete' => (int) !empty($access['delete'][$rid]),
        'grant_create' => (int) !empty($access['create'][$rid]),
        'priority' => 0,
      )
    );
  }
  $insert->execute();
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function forum_access_form_taxonomy_term_forums_container_form_alter(&$form, \Drupal\Core\Form\FormStateInterface $form_state, $form_id) {
  $tid = $form['tid']['#value'];
  $settings = _forum_access_get_settings($tid);

  // Basic fieldset.
  _forum_access_basic_taxonomy_term_fieldset($form);

  // Moderators fields.
  $form['forum_access']['grants'] = _forum_access_container_grants_form($form_state, $settings);
  $form['forum_access']['moderators'] = _forum_access_forum_moderators_form($form_state, $tid);
  $form['actions']['submit']['#submit'][] = '_forum_access_form_submit';
}

/**
 * List of permissions.
 */
function _forum_access_forum_permissions_form() {
  $variables = array();
  $permissions = array(
    'access content' => 'node',
    'access comments' => 'comment',
    'create forum content' => 'node',
    'post comments' => 'comment',
    'skip comment approval' => 'comment',
    'edit own forum content' => 'node',
    'edit any forum content' => 'node',
    'delete own forum content' => 'node',
    'delete any forum content' => 'node',
    'administer comments' => 'comment',
    'administer forums' => 'forum',
    'administer nodes' => 'node',
    'access content overview' => 'node',
    'view own unpublished content' => 'node',
    'edit own comments' => 'comment',
  );

  foreach ($permissions as $perm => $module) {
    $key = '@' . str_replace(' ', '_', $perm);
    $variables[$key] = _forum_access_permission_link($module, $perm);
  }

  $form = array(
    '#type' => 'details',
    '#title' => t('Permissions information'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['list'] = array(
    '#theme' => 'item_list',
    '#title' => t('Note that users need'),
    '#items' => array(
      t('the <strong>@access_content</strong> and <strong>@access_comments</strong> permissions <strong>AND <em>View</em></strong> to be able to see this forum and its content at all,', $variables),
      t('the <strong>@create_forum_content</strong> (and similar) permissions <strong>AND <em>Post</em></strong> to be able to create forum content, and', $variables),
      t('the <strong>@post_comments</strong> (and optionally <strong>@skip_comment_approval</strong>) permission AND <em>Post</em> to be able to post comments/replies;', $variables),
      t('the <strong>@edit_own_forum_content</strong> or <strong>@edit_any_forum_content</strong> (and similar) permissions (<strong>OR <em>Edit</em></strong>) can be added if desired, <strong>plus</strong>', $variables),
      t('the <strong>@delete_own_forum_content</strong> or <strong>@delete_any_forum_content</strong> (and similar) permissions (<strong>OR <em>Delete</em></strong>) if desired;', $variables),
      t('the <strong>@administer_comments</strong> (global!) permission <strong>OR <em>Edit</em>/<em>Delete</em></strong> to be able to edit/delete comments;', $variables),
      t('the <strong>@administer_forums</strong> permission <strong>AND <em>View</em></strong> to be able to administer forums (and change access!).', $variables),
      t('Furthermore note that content which is not published is treated in a different way by Drupal: it can be viewed only by its author
        (with the <strong>@view_own_unpublished_content</strong> permission) or by users with the <strong>@administer_nodes</strong> permission. Unpublished comments and
        replies are accessible to users with <strong><em>Edit</em> OR <em>Delete</em></strong>, <strong>OR</strong> with the <strong>@administer_comments</strong> permission,
        but they are never counted on the forum page.', $variables),
      t('The global <strong>@edit_own_comments</strong> permission is ignored, but the edit/delete forum content permissions are extended to comments;
        the per-forum <strong><em>Edit</em></strong> and <strong><em>Delete</em></strong> apply to both nodes and comments, too.', $variables),
    ),
  );
  return $form;
}

/**
 * Generate permissions link.
 */
function _forum_access_permission_link($module, $permission) {
  $permissions = &drupal_static(__FUNCTION__, array());

  if (empty($permissions)) {
    $permission_handler = \Drupal::service('user.permissions');
    $permissions_data = $permission_handler->getPermissions();

    foreach ($permissions_data as $perm => $perm_item) {
      $provider = $perm_item['provider'];
      $permissions[$provider][$perm] = strip_tags($perm_item['title']);
    }
  }

  return $permissions[$module][$permission];
}

/**
 * Retrieve the settings for a taxonomy term.
 */
function _forum_access_get_settings($tid = NULL) {
  $settings = array(
    'view' => array(),
    'create' => array(),
    'update' => array(),
    'delete' => array(),
    'priority' => 0
  );

  $result = \Drupal::database()->select('forum_access', 'a')
    ->fields('a', array('tid', 'rid', 'grant_view', 'grant_update', 'grant_delete', 'grant_create', 'priority'))
    ->condition('tid', $tid)
    ->execute();

  foreach ($result as $access) {
    if ($access->grant_view) {
      $settings['view'][] = $access->rid;
    }
    if ($access->grant_update) {
      $settings['update'][] = $access->rid;
    }
    if ($access->grant_delete) {
      $settings['delete'][] = $access->rid;
    }
    if ($access->grant_create) {
      $settings['create'][] = $access->rid;
    }
    if ($access->rid == AccountInterface::AUTHENTICATED_ROLE) { // this is our reference
      $settings['priority'] = $access->priority;
    }
  }
  return $settings;
}

/**
 * Get form table with roles grants.
 */
function _forum_access_forum_grants_form($form_state, $settings) {
  $roles = user_roles();
  $administer_forums_roles = user_roles(FALSE, 'administer forums');
  $bypass_node_access_roles = user_roles(FALSE, 'bypass node access');

  $vid = \Drupal::config('forum.settings')->get('vocabulary');
  $vocabulary = Vocabulary::load($vid);

  $cols = array(
    'view'   => t('View this forum'),
    'create' => t('Post in this forum'),
    'update' => t('Edit posts'),
    'delete' => t('Delete posts'),
  );

  _forum_access_grants_form_elements($form, $roles, $administer_forums_roles, $bypass_node_access_roles, $cols, $settings);

  $form['info'] = array(
    '#type'            => 'item',
    '#description'     => t('For explanations of <em class="placeholder">special cases</em>, hover your mouse over role names.'),
  );

  return $form;
}

/**
 * Get roles permissions form for container.
 */
function _forum_access_container_grants_form($form_state, $settings) {
  $roles = user_roles();
  $administer_forums_roles = user_roles(FALSE, 'administer forums');
  $bypass_node_access_roles = user_roles(FALSE, 'bypass node access');

  $vid = \Drupal::config('forum.settings')->get('vocabulary');
  $vocabulary = Vocabulary::load($vid);

  $cols = array(
    'view'   => t('View this container'),
    'create' => t('See this container in the forums selection list'),
  );

  _forum_access_grants_form_elements($form, $roles, $administer_forums_roles, $bypass_node_access_roles, $cols, $settings);

  $form['info'] = array(
    '#type' => 'item',
    '#description' => t('Users who can see any forum or container within this one should get the <strong><em>View</em></strong> grant. <br />
      Users who can post to a forum within this container should get the <strong><em>See</em></strong> grant, so that the forum appears in the proper context in the %Forums selection list.',
      array('%Forums' => $vocabulary->label())),
  );

  return $form;
}

/**
 * Common grants form elements for forum and container.
 */
function _forum_access_grants_form_elements(&$form, $roles, $administer_forums_roles, $bypass_node_access_roles, $cols, $settings) {
  $form = array(
    '#theme' => 'forum_access_table',
  );

  // Build table rows.
  foreach ($roles as $rid => $role) {
    $form['rows'][$rid]['label'] = array(
      '#type' => 'item',
      // @todo Add check_plain.
      '#markup' => $role->label(),
    );

    $special = NULL;
    if (isset($administer_forums_roles[$rid])) {
      $special = t("This role has the '@administer_forums' permission, and granting 'View' enables the role holders to change the settings on this page, including Access Control!",
        array(
          '@administer_forums' => t('administer forums')
        )
      );

      if (isset($bypass_node_access_roles[$rid])) {
        $special .= ' ' . t("Because the role also has the '@bypass_node_access' permission, it has full access to all nodes either way.",
          array(
            '@bypass_node_access' => t('bypass node access')
          )
        );
      }
    }
    elseif (isset($bypass_node_access_roles[$rid])) {
      $special = t("This role has the '@bypass_node_access' permission and thus full access to all nodes.",
        array(
          '@bypass_node_access' => t('bypass node access')
        )
      );
    }

    if (isset($special)) {
      $form['rows'][$rid] += array(
        '#prefix' => '<em><span title="' . $special . '" class="forum-access-special-role">',
        '#suffix' => '</span></em>',
        '#disabled' => TRUE,
      );
    }
    $roles[$rid] = '';
  }

  $form['header']['roles'] = array(
    '#markup' => t('Roles'),
    '#tree'   => TRUE,
  );

  // Add checkboxes.
  foreach ($cols as $cid => $ctitle) {
    $form['checkboxes'][$cid] = array(
      '#type'          => 'checkboxes',
      '#options'       => $roles,
      '#default_value' => $settings[$cid],
    );
    $form['header'][$cid] = array(
      '#markup' => $ctitle,
      '#tree'   => TRUE,
    );
  }
}
