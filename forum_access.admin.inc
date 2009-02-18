<?php
// $Id$

/**
 * @file forum_access.admin.inc
 *
 * Include file for forum_access.module, containing (sub-)page handling
 * (form_alter) and batch code.
 *
 */

/**
 * Rewrite the forum administration page with our new access rules.
 */
function _forum_access_forum_form($form_id, &$form, $container) {
  if (isset($form['tid']['#value']) && !forum_access_access($form['tid']['#value'], 'view')) {
    return;  // Deny access to (hide) the FA controls if the user doesn't have View access to the forum.
  }

  $rids = array();
  $result = db_query("SELECT r.rid, r.name FROM {role} r ORDER BY r.name");
  while ($obj = db_fetch_object($result)) {
    $rids[$obj->rid] = $obj->name;
  }

  $fa_priority = 0;
  if (isset($form['tid']['#value'])) {  // edit
    $result = db_query("SELECT * FROM {forum_access} where tid=%d", $form['tid']['#value']);

    $view = $update = $delete = $create = array();
    while ($forum_access = db_fetch_object($result)) {
      $row_received = TRUE;
      if ($forum_access->grant_view) {
        $view[] = $forum_access->rid;
      }
      if ($forum_access->grant_update) {
        $update[] = $forum_access->rid;
      }
      if ($forum_access->grant_delete) {
        $delete[] = $forum_access->rid;
      }
      if ($forum_access->grant_create) {
        $create[] = $forum_access->rid;
      }
      if ($forum_access->rid == DRUPAL_AUTHENTICATED_RID) {  // this is our reference
        $fa_priority = $forum_access->priority;
      }
    }
  }
  else {  // create
    // Default to all users can read; all logged in users can post.
    $view = array(DRUPAL_ANONYMOUS_RID, DRUPAL_AUTHENTICATED_RID);
    $create = array(DRUPAL_AUTHENTICATED_RID);
    $update = $delete = array();
  }

  $form['forum_access'] = array('#type' => 'fieldset',
    '#title' => t('Access control'),
    '#collapsible' => TRUE,
    '#tree' => TRUE,
  );

  $tr = 't';
  $form['forum_access']['permissions'] = array(
    '#type' => 'fieldset',
    '#title' => $tr('Permissions'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $variables = array(
    '!access_content'                 => '<em>'. l($tr('access content'), 'admin/user/permissions', array('fragment' => 'module-node', 'html' => TRUE)). '</em>',
    '!access_comments'                => '<em>'. l($tr('access comments'), 'admin/user/permissions', array('fragment' => 'module-comment', 'html' => TRUE)). '</em>',
    '!create_forum_topics'            => '<em>'. l($tr('create forum topics'), 'admin/user/permissions', array('fragment' => 'module-forum', 'html' => TRUE)). '</em>',
    '!post_comments'                  => '<em>'. l($tr('post comments'), 'admin/user/permissions', array('fragment' => 'module-comment', 'html' => TRUE)). '</em>',
    '!post_comments_without_approval' => '<em>'. l($tr('post comments without approval'), 'admin/user/permissions', array('fragment' => 'module-comment', 'html' => TRUE)). '</em>',
    '!edit_own_forum_topics'          => '<em>'. l($tr('edit own forum topics'), 'admin/user/permissions', array('fragment' => 'module-forum', 'html' => TRUE)). '</em>',
    '!edit_any_forum_topics'          => '<em>'. l($tr('edit any forum topics'), 'admin/user/permissions', array('fragment' => 'module-forum', 'html' => TRUE)). '</em>',
    '!delete_own_forum_topics'        => '<em>'. l($tr('delete own forum topics'), 'admin/user/permissions', array('fragment' => 'module-forum', 'html' => TRUE)). '</em>',
    '!delete_any_forum_topics'        => '<em>'. l($tr('delete any forum topics'), 'admin/user/permissions', array('fragment' => 'module-forum', 'html' => TRUE)). '</em>',
    '!administer_comments'            => '<em>'. l($tr('administer comments'), 'admin/user/permissions', array('fragment' => 'module-comment', 'html' => TRUE)). '</em>',
    '!administer_forums'              => '<em>'. l($tr('administer forums'), 'admin/user/permissions', array('fragment' => 'module-forum', 'html' => TRUE)). '</em>',
    '!administer_nodes'               => '<em>'. l($tr('administer nodes'), 'admin/user/permissions', array('fragment' => 'module-node', 'html' => TRUE)). '</em>',
  );
  if (!$container) {
    $form['forum_access']['permissions'][] = array(
      '#type' => 'markup',
      '#value' => '<div>'. t('Note that users need') .'<ul style="margin-top: 0"><li>'.
                           t('the !access_content and !access_comments permissions <strong>AND <em>View</em></strong> to be able to see content at all,', $variables) .'</li><li>'.
                           t('the !create_forum_topics (and similar) permissions <strong>AND <em>Post</em></strong> to be able to create forum content, and', $variables) .'</li><li>'.
                           t('the !post_comments or !post_comments_without_approval permission to be able to post comments and replies;', $variables) .'</li><li>'.
                           t('the !edit_own_forum_topics or !edit_any_forum_topics (and similar) permissions (<strong>OR <em>Edit</em></strong>) can be added if desired, <strong>plus</strong>', $variables) .'</li><li>'.
                           t('the !delete_own_forum_topics or !delete_any_forum_topics (and similar) permissions (<strong>OR <em>Delete</em></strong>) if desired;', $variables) .'</li><li>'.
                           t('the !administer_comments permission to be able to administer comments and replies (everywhere!),', $variables) .'</li><li>'.
                           t('the !administer_forums permission <strong>AND <em>View</em></strong> to be able to administer forums (and change access!).', $variables) .'</li></ul>'.
                           t('Furthermore note that content which is not published is treated in a different way by Drupal: it can be viewed only by its author or by users with the !administer_nodes permission. Unpublished comments and replies are accessible to users with the !administer_comments permission only.', $variables) .'</div>',
    );
  }

  // Column titles:
  $form['forum_access']['headers']['view'] = array(
    '#type' => 'item',
    '#prefix' => '<div class="forum-access-div">',
    '#title' => ($container ? t('View this container') : t('View this forum')),
    '#suffix' => '</div>',
  );
  $forum_vocabulary = taxonomy_vocabulary_load(variable_get('forum_nav_vocabulary', ''));
  $form['forum_access']['headers']['create'] = array(
    '#type' => 'item',
    '#prefix' => '<div class="forum-access-div" id="container">',
    '#title' => t('Show this container in %Forums selection list', array('%Forums' => $forum_vocabulary->name)),
    '#suffix' => '</div>',
  );
  if (!$container) {
    $form['forum_access']['headers']['create'] = array(  // overwrite!
      '#type' => 'item',
      '#prefix' => '<div class="forum-access-div">',
      '#title' => t('Post in this forum'),
      '#suffix' => '</div>',
    );
    $form['forum_access']['headers']['update'] = array(
      '#type' => 'item',
      '#prefix' => '<div class="forum-access-div">',
      '#title' => t('Edit posts'),
      '#suffix' => '</div>',
    );
    $form['forum_access']['headers']['delete'] = array(
      '#type' => 'item',
      '#prefix' => '<div class="forum-access-div">',
      '#title' => t('Delete posts'),
      '#suffix' => '</div>',
    );
  }
  $form['forum_access']['headers']['clearer'] = array(
    '#value' => '<div class="forum-access-clearer"></div>',
  );

  // Column content (checkboxes):
  $form['forum_access']['view'] = array(
    '#type' => 'checkboxes',
    '#prefix' => '<div class="forum-access-div">',
    '#suffix' => '</div>',
    '#options' => $rids,
    '#default_value' => $view
  );
  $form['forum_access']['create'] = array(
    '#type' => 'checkboxes',
    '#prefix' => '<div class="forum-access-div">',
    '#suffix' => '</div>',
    '#options' => $rids,
    '#default_value' => $create
  );
  if (!$container) {
    $form['forum_access']['update'] = array(
      '#type' => 'checkboxes',
      '#prefix' => '<div class="forum-access-div">',
      '#suffix' => '</div>',
      '#options' => $rids,
      '#default_value' => $update
    );
    $form['forum_access']['delete'] = array(
      '#type' => 'checkboxes',
      '#prefix' => '<div class="forum-access-div">',
      '#suffix' => '</div>',
      '#options' => $rids,
      '#default_value' => $delete
    );
  }
  $form['forum_access']['clearer'] = array(
    '#value' => '<div class="forum-access-clearer"></div>',
  );

  drupal_add_css(drupal_get_path('module', 'forum_access') .'/forum_access.css');

  // Find our moderator ACL:
  if (isset($form['tid']['#value'])) {  // edit, not new
    $acl_id = db_result(db_query("SELECT acl_id from {acl} WHERE module = 'forum_access' AND name = '%d'", $form['tid']['#value']));
    if (!$acl_id) { // create one
      $acl_id = acl_create_new_acl('forum_access', $form['tid']['#value']);
      // update every existing node in this forum to use this acl.
      $result = db_query("SELECT nid FROM {term_node} WHERE tid = %d", $form['tid']['#value']);
      while ($node = db_fetch_object($result)) {
        // all privs to this ACL.
        acl_node_add_acl($node->nid, $acl_id, 1, 1, 1);
      }
    }
    $form['forum_access']['acl'] = acl_edit_form($acl_id, t('Moderators'));
    $form['forum_access']['acl']['note1'] = array(
      '#type' => 'markup',
      '#value' => '<div>'. t('Moderators receive <strong><em>View</em></strong>, <strong><em>Post</em></strong>, <strong><em>Edit</em></strong>, and <strong><em>Delete</em></strong>. They also need the following permissions: !access_content, !access_comments, !create_forum_topics (and similar), !post_comments_without_approval, and possibly !administer_comments (which is a global permission!).', $variables) .'</div>',
      '#weight' => -1,
    );
    if (strlen($form['forum_access']['acl']['user_list']['#default_value']) <= 6 && empty($form_state['post']['forum_access']['acl']['add']))
    {
      $form['forum_access']['acl']['#collapsed'] = TRUE;
    }
    $form['forum_access']['acl']['note2'] = array(
      '#type' => 'markup',
      '#value' => '<div>'. t('Note: Changes to moderators are not saved until you click [!Save] below.', array('!Save' => $tr('Save'))) .'</div>',
    );
  }

  foreach (module_implements('node_access_records') as $module) {
    $na_modules[$module] = $module;
  }
  unset($na_modules['forum_access']);
  unset($na_modules['acl']);
  if (count($na_modules) && !$container) {
    $form['forum_access']['interference'] = array(
      '#type' => 'fieldset',
      '#title' => t('Module interference'),
      '#collapsible' => TRUE,
    );
    $variables = array(
      '!content_type' => $tr('content type'),
      '%content_type' => node_get_types('name', 'forum'),
      '!Forum_Access' => 'Forum Access',
      '!Content_Access' => l('Content Access', 'http://drupal.org/project/content_access'),
      '@Content_Access' => 'Content Access',
      '!ACL' => 'ACL',
      '!module_list' => '<ul><li>'. implode($na_modules, '</li><li>') .'</li></ul>',
    );
    $form['forum_access']['interference'][] = array(
      '#type' => 'item',
      '#value' => '<p>'. t("Besides !Forum_Access (and !ACL) you have installed the following node access module(s): !module_list   The grants of every module are combined for each node. Access can only be granted, not removed &mdash; if a certain module grants a permission, the other(s) cannot deny it.", $variables) .'</p>',
      '#description' => t('Forums can contain other content types besides %content_type; !Forum_Access will contribute the grants defined above to every node in this forum, but other node access control modules may also contribute their grants, especially to nodes of types other than %content_type.', $variables),
    );

    if (module_exists('content_access')) {
      $ca_settings = variable_get('content_access_settings', array());
      foreach (array('view', 'update', 'delete', 'per_node') as $type) {
        $value = content_access_get_settings($type, 'forum');
        if (!empty($value)) {
          $ca_interferes = TRUE;
        }
      }
      $ca_priority = content_access_get_settings('priority', 'forum');
      $is_conflict = $ca_priority >= $fa_priority && !empty($ca_interferes) || $ca_priority > $fa_priority;
      $variables += array(
        '!link' => l(t('@Content_Access configuration for the %content_type type', $variables), 'admin/content/node-type/forum/access', array('html' => TRUE)),
        '%Advanced' => $tr('Advanced'),
      );
      $specifically = ($ca_priority == $fa_priority ? t('Specifically, any grants given by !Content_Access cannot be taken back by !Forum_Access.', $variables) : '');
      if ($is_conflict) {
        $form['forum_access']['interference']['by_content_access'] = array(
          '#type' => 'fieldset',
          '#title' => 'Content Access',
          '#collapsible' => FALSE,
          '#attributes' => array('class' => 'error'),
        );
        $form['forum_access']['interference']['by_content_access'][] = array(
          '#value' => '<div>'. t('You have set the !Content_Access module to control access to content of type %content_type&mdash;this can interfere with proper operation of !Forum_Access!', $variables) ." $specifically</div>",
        );
        if ($ca_priority == $fa_priority) {
          $form['forum_access']['interference']['by_content_access'][] = array(
            '#value' => '<div>'. t("Unless you really know what you're doing, we recommend that you go to the !link page and clear all checkboxes. This will instruct @Content_Access to leave the %content_type nodes alone. However, if you put nodes of other !content_types into forums as well, then these !content_types will continue to have this problem.", $variables) .'</div>',
          );
        }
        else {
          $form['forum_access']['interference']['by_content_access'][] = array(
            '#value' => '<div>'. t("The priority of @Content_Access ($ca_priority) is higher than the priority of !Forum_Access ($fa_priority), which means the latter is <b>completely disabled</b> for the %content_type type! Unless you really know what you're doing, we recommend that you go to the !link page, change the priority (under %Advanced) to 0, and clear all checkboxes.", $variables) .'</div>',
          );
        }
      }
      else {
        $form['forum_access']['interference'][] = array(
          '#value' => '<p>'. t('Note: You have installed the !Content_Access module, which has the capability to grant access to content that would otherwise be protected by !Forum_Access. Be careful when configuring @Content_Access!', $variables) .'</p>',
        );
      }
    }

    $form['forum_access']['interference']['advanced'] = array(
      '#type' => 'fieldset',
      '#title' => t('Advanced'),
      '#collapsible' => TRUE,
      '#collapsed' => !($fa_priority != 0),
    );
    $form['forum_access']['interference']['advanced']['priority'] = array(
      '#type' => 'weight',
      '#title' => t('Priority of !Forum_Access node grants in this forum', $variables),
      '#default_value' => $fa_priority,
      '#description' => t("If you have no other node access control modules installed, you should leave this at the default 0. <br /> Otherwise you can raise or lower the priority of !Forum_Access' grants. Out of all the grants contributed to a node, only those with the highest priority are used, and all others are discarded.", $variables),
    );
  }

  $variables = array(
    '!Forum_Access'        => l('Forum Access', 'http://drupal.org/project/forum_access'),
    '!ACL'                 => l('ACL', 'http://drupal.org/project/acl'),
    '%Module_interference' => t('Module interference'),
    '!Forum_Access-dev'    => l('Forum&nbsp;Access&nbsp;6.x-1.x-dev', 'http://drupal.org/node/96795', array('html' => TRUE)),
    '!ACL-dev'             => l('ACL&nbsp;6.x-1.x-dev', 'http://drupal.org/node/96794', array('html' => TRUE)),
    '%devel_node_access'   => 'devel_node_access',
    '!Devel'               => l('Devel', 'http://drupal.org/project/devel'),
    '!DNA'                 => 'DNA',
    '!debug_mode'          => l('debug mode', 'admin/settings/devel', array('fragment' => 'edit-devel-node-access-debug-mode')),
    '!dna_summary'         => l('devel/node_access/summary', 'devel/node_access/summary'),
    '!Rebuild_permissions' => '['. $tr('Rebuild permissions') .']',
    '!Post_settings_link'  => l('admin/content/node-settings', 'admin/content/node-settings'),
    '!Forum_Access_'       => l('Forum Access', 'http://drupal.org/project/issues/forum_access'),
    '!ACL_'                => l('ACL', 'http://drupal.org/project/issues/acl'),
  );
  $form['forum_access']['troubleshooting'] = array(
    '#type' => 'fieldset',
    '#title' => t('Trouble-shooting node access'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['forum_access']['troubleshooting'][] = array(
    '#type' => 'item',
    '#value' => '<div>'.    t("In case of problems, follow these steps until you've got it worked out:") .'<ol style="margin-top: 0"><li>'.
                            t("Update to the 'recommended' !Forum_Access and !ACL releases for your version of Drupal.", $variables) .'</li><li>'.
      (count($na_modules) ? t("Read %Module_interference above and update your other node access modules.", $variables) .'</li><li>' : '').
                            t("Check the release notes of the development snapshots for issues that might have been fixed in !Forum_Access-dev or !ACL-dev since the latest release.", $variables) .'</li><li>'.
                            t("Install the %devel_node_access module (!DNA, part of the !Devel module) and enable its !debug_mode: !DNA will show you all the grants that actually control your nodes in a footer block on each node's page.", $variables) .'</li><li>'.
                            t("Additional insight can be gained from !dna_summary and by enabling the second !DNA block.", $variables) .'</li><li>'.
                            t("Click !Rebuild_permissions on !Post_settings_link and check DNA for changes.", $variables) .'</li><li>'.
                            t("Check the issues queues of !Forum_Access_ and !ACL_ for existing reports and possible solutions.", $variables) .'</li><li>'.
                            t("If all of this hasn't helped, then pick ONE node that is misbehaving, create a new issue in the issues queue, describe the problem... <ul><li> what did you do? </li><li> what did you expect? </li><li> what happened instead? </li></ul> ... and <strong>include all the DNA records</strong> for that one node. <br /> Be sure to indicate paths (URLs) for every page and module that you mention.") .'</li></ol></div>',
  );
  $form['forum_access']['troubleshooting'][] = array(
    '#type' => 'item',
    '#value' => '<div>'. t("Note: You should not keep the !Devel module enabled on a production site.", $variables) .'</div>',
  );

  // Move some stuff down so our block goes in a nice place.
  $form['submit']['#weight'] = 10;
  $form['delete']['#weight'] = 10;

  $form['#validate'][] = '_forum_access_form_validate';
  $form['#submit'][] = '_forum_access_form_submit';
}

function _forum_access_form_validate($form, &$form_state) {
  global $user;
  
  if ($user->uid == 1) {
    return;
  }
  $access = $form_state['values']['forum_access']; // shortcut
  foreach ($access['view'] as $rid => $checked) {
    if ($checked && isset($user->roles[$rid])) {
      return;
    }
  }
  form_set_error('forum_access][view', t('You must assign %View access to a role that you hold.', array('%View' => 'View')));
}

function _forum_access_form_submit($form, &$form_state) {
  db_query("DELETE FROM {forum_access} WHERE tid = %d", $form_state['values']['tid']);
  $access = $form_state['values']['forum_access']; // shortcut
  $fa_priority = isset($access['interference']['advanced']['priority']) ? $access['interference']['advanced']['priority'] : 0;
  if (array_key_exists('acl', $access)) {
    acl_save_form($access['acl'], $fa_priority);
  }
  foreach ($access['view'] as $rid => $checked) {
    $grants[] = array(
      'realm' => 'forum_access',
      'gid' => $rid,
      'grant_view' => (bool) $checked,
      'grant_update' => !empty($access['update'][$rid]),
      'grant_delete' => !empty($access['delete'][$rid]),
      'priority' => $fa_priority,
    );
    db_query("INSERT INTO {forum_access} (tid, rid, grant_view, grant_update, grant_delete, grant_create, priority) VALUES (%d, %d, %d, %d, %d, %d, %d)", $form_state['values']['tid'], $rid, (bool) $checked, !empty($access['update'][$rid]), !empty($access['delete'][$rid]), !empty($access['create'][$rid]), $fa_priority);
  }

  if ($form_state['values']['form_id'] != 'forum_form_container') {
    // mass update in batch mode, modeled after node.module
    $batch = array(
      'title' => t('Updating content access permissions'),
      'file' => drupal_get_path('module', 'forum_access') .'/forum_access.admin.inc',
      'operations' => array(
        array('_forum_access_update_batch_operation', array($form_state['values']['tid'])),
      ),
      'finished' => '_forum_access_update_batch_finished'
    );
    batch_set($batch);
  }
}

/**
 * Batch operation for forum_access_form_submit().
 *
 * This is a mutlistep operation : we go through all nodes by packs of 20.
 * The batch processing engine interrupts processing and sends progress
 * feedback after 1 second execution time.
 */
function _forum_access_update_batch_operation($tid, &$context) {
  if (empty($context['sandbox'])) {
    // Initiate multistep processing.
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_node'] = 0;
    $context['sandbox']['max'] = db_result(db_query("SELECT COUNT(DISTINCT n.nid) FROM {node} n LEFT JOIN {term_node} tn ON tn.nid = n.nid WHERE tn.tid = %d", $tid));
  }

  // Process the next 20 nodes.
  $limit = 20;
  $result = db_query_range("SELECT n.nid FROM {node} n LEFT JOIN {term_node} tn ON tn.nid = n.nid WHERE n.nid > %d AND tn.tid = %d ORDER BY n.nid ASC", $context['sandbox']['current_node'], $tid, $context['sandbox']['current_node'], 0, $limit);
  while ($row = db_fetch_array($result)) {
    $loaded_node = node_load($row['nid'], NULL, TRUE);
    // To preserve database integrity, only aquire grants if the node
    // loads successfully.
    if (!empty($loaded_node)) {
      node_access_acquire_grants($loaded_node);
    }
    $context['sandbox']['progress']++;
    $context['sandbox']['current_node'] = $loaded_node->nid;
  }

  // Multistep processing : report progress.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Post-processing for forum_access_form_submit().
 */
function _forum_access_update_batch_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message(t('The content access permissions have been updated.'));
  }
  else {
    drupal_set_message(t('The content access permissions have not been properly updated.'), 'error');
    node_access_needs_rebuild(TRUE);
  }
  cache_clear_all();
}

/**
 * Remove unusable 'edit' links from overview form.
 */
function _forum_access_forum_overview($form_id, &$form) {
  global $user;
  if ($user->uid == 1) {
    return;
  }
  foreach ($form as $key => $value) {
    if (preg_match('/^tid:(.*):0$/', $key, $matches)) {
      if (!forum_access_access($matches[1], 'view')) {
        unset($form[$key]['edit']['#value']);
      }
    }
  }
}

/**
 * Add warnings on Content Access admin forms where CA wants
 * to control the same content types as we do.
 */
function _forum_access_content_access_admin_form() {
  $tr = 't';
  $variables = array('!content_type' => $tr('content type'), '!Forum_Access' => 'Forum Access', '!Forum_Access_link' => l('Forum Access', 'admin/content/forum'));
  if (arg(3) == 'forum') {
    drupal_set_message(t('Note: In Drupal, access can only be granted, not taken away. Whatever access you grant here will not be reflected on the !Forum_Access_link settings, but !Forum_Access can only allow <i>more</i> access, not less.', $variables), 'warning');
  }
  else {
    $vid = variable_get('forum_nav_vocabulary', '');
    $vocabulary = taxonomy_vocabulary_load($vid);
    if (isset($vocabulary->nodes[arg(3)]))
    {
      drupal_set_message(t('Note: Nodes of this !content_type can be put inside forums, where access to them will also be controlled by !Forum_Access.<br />In Drupal, access can only be granted, not taken away. Whatever access you grant here will not be reflected on the !Forum_Access_link settings, and vice versa, but any node access module can only allow <i>more</i> access, not less.', $variables), 'warning');
    }
  }
}