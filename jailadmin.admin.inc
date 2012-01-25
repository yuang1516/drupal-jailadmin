<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Mount.php');
require_once('classes/Service.php');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_admin_settings($form, &$form_state) {
    $jails = get_all_jails_for_select();
    $templates = get_all_templates_for_select();

    if (count($jails)) {
        $form['jails'] = array(
            '#type' => 'fieldset',
            '#title' => t('Configured Jails'),
            '#collapsible' => TRUE,
            '#collapsed' => FALSE,
        );

        $form['jails']['existing'] = array(
            '#type' => 'select',
            '#title' => t('Existing Jails'),
            '#multiple' => TRUE,
            '#description' => t('Select one or more to delete'),
            '#options' => $jails,
        );

        $form['jails']['zfs_destroy'] = array(
            '#type' => 'checkbox',
            '#title' => t('Destroy ZFS Dataset?'),
        );
    }

    $form['templates'] = array(
        '#type' => 'fieldset',
        '#title' => t('Template Jails'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
    );

    if (count($templates)) {
        $form['templates']['existing_templates'] = array(
            '#type' => 'select',
            '#title' => t('Existing Templates'),
            '#description' => t('Select one or more to delete'),
            '#multiple' => TRUE,
            '#options' => $templates,
        );
    }

    $form['templates']['new_template_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Name'),
        '#size' => 60,
    );

    $form['templates']['new_template_snapshot'] = array(
        '#type' => 'textfield',
        '#title' => t('ZFS Dataset@Snapshot'),
        '#size' => 60,
    );

    $form['new_jail'] = array(
        '#type' => 'fieldset',
        '#title' => t('Add New Jail'),
        '#description' => t('Only set basic info here. Networking and services are configured elsewhere.'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
    );

    $form['new_jail']['new_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Name'),
        '#size' => 30,
    );

    $form['new_jail']['new_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path'),
        '#size' => 30,
    );

    $form['new_jail']['new_dataset'] = array(
        '#type' => 'textfield',
        '#title' => t('Dataset'),
        '#size' => 30,
    );

    $form['new_jail']['new_route'] = array(
        '#type' => 'textfield',
        '#title' => t('Default Route'),
        '#size' => 30,
    );

    if (count($templates)) {
        $form['new_jail']['new_template'] = array(
            '#type' => 'select',
            '#title' => t('Template Jail'),
            '#options' => $templates,
            '#description' => t('Only if creating from a template'),
        );
    }

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Submit'),
    );

    return $form;
}

function jailadmin_admin_settings_submit($form, &$form_state) {
    if (isset($form_state['values']['existing'])) {
        foreach (array_filter($form_state['values']['existing']) as $name) {
            $destroy = FALSE;
            if (isset($form_state['values']['zfs_destroy']) && $form_state['values']['zfs_destroy'] == '1')
                $destroy = TRUE;

            $j = Jail::Load($name);
            $j->Delete($destroy);
        }
    }

    if (isset($form_state['values']['existing_templates'])) {
        foreach (array_filter($form_state['values']['existing_templates']) as $template) {
            if (strlen($template) == 0)
                continue;

            db_delete('jailadmin_templates')
                ->condition('snapshot', $template)
                ->execute();
        }
    }

    if (isset($form_state['values']['new_name']) && strlen($form_state['values']['new_name'])) {
        $template = '';
        $name = $form_state['values']['new_name'];
        $path = $form_state['values']['new_path'];
        $dataset = $form_state['values']['new_dataset'];
        $route = $form_state['values']['new_route'];

        if (isset($form_state['values']['new_template']) && strlen($form_state['values']['new_template']))
            $template = $form_state['values']['new_template'];

        $j = new Jail;
        $j->name = $name;
        $j->path = $path;
        $j->dataset = $dataset;
        $j->route = $route;

        $j->Create($template);

        $form_state['input']['new_name'] = '';
        $form_state['input']['new_path'] = '';
        $form_state['input']['new_dataset'] = '';
        $form_state['input']['new_route'] = '';
        $form_state['input']['new_template'] = '';
    }

    if (isset($form_state['values']['new_template_name']) && strlen($form_state['values']['new_template_name'])) {
        db_insert('jailadmin_templates')
            ->fields(array(
                'name' => $form_state['values']['new_template_name'],
                'snapshot' => $form_state['values']['new_template_snapshot'],
            ))->execute();

        $form_state['input']['new_template_name'] = '';
        $form_state['input']['new_template_snapshot'] = '';
    }

    $form_state['rebuild'] = TRUE;
}

function jailadmin_admin_settings_network($form, &$form_state) {
    $networks = get_all_networks_for_select();

    if (count($networks)) {
        $form['jailadmin_networks'] = array(
            '#type' => 'fieldset',
            '#title' => t('Existing Networks'),
            '#collapsible' => TRUE,
            '#collapsed' => FALSE,
        );

        $form['jailadmin_networks']['network'] = array(
            '#type' => 'select',
            '#description' => t('Select a network to modify'),
            '#options' => $networks,
        );

        if (isset($form_state['input']['network'])) {
            $network = Network::Load($form_state['input']['network']);
            $disabled = FALSE;

            if ($network->IsOnline())
                $disabled = TRUE;

            $form['jailadmin_networks']['device'] = array(
                '#type' => 'textfield',
                '#title' => t('Device'),
                '#default_value' => t($network->device),
                '#size' => 60,
                '#disabled' => $disabled,
            );

            $form['jailadmin_networks']['ip'] = array(
                '#type' => 'textfield',
                '#title' => t('IP'),
                '#default_value' => t($network->ip),
                '#size' => 60,
                '#disabled' => $disabled,
            );

            if ($disabled) {
                $form['jailadmin_networks']['disabled_notice'] = array(
                    '#type' => 'markup',
                    '#prefix' => '<div>',
                    '#markup' => t('NOTE: Network is online. Cannot modify while online.'),
                    '#suffix' => '</div>',
                );
            }

            $form['jailadmin_networks']['bring_online'] = array(
                '#type' => 'submit',
                '#value' => t('Bring Online'),
                '#submit' => array('jailadmin_admin_settings_network_bring_online'),
                '#disabled' => $disabled,
            );

            $form['jailadmin_networks']['bring_offline'] = array(
                '#type' => 'submit',
                '#value' => t('Bring Offline'),
                '#submit' => array('jailadmin_admin_settings_network_bring_offline'),
                '#disabled' => !$disabled,
            );

            $form['jailadmin_networks']['delete'] = array(
                '#type' => 'submit',
                '#value' => t('Delete'),
                '#submit' => array('jailadmin_admin_settings_network_delete'),
                '#disabled' => $disabled,
            );
        }
    }

    $form['jailadmin_new_network'] = array(
        '#type' => 'fieldset',
        '#title' => 'New Network',
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
    );

    $form['jailadmin_new_network']['new_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Name'),
        '#size' => 60,
    );

    $form['jailadmin_new_network']['new_device'] = array(
        '#type' => 'textfield',
        '#title' => t('Device'),
        '#size' => 60,
    );

    $form['jailadmin_new_network']['new_ip'] = array(
        '#type' => 'textfield',
        '#title' => t('IP'),
        '#size' => 60,
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save'),
    );

    return $form;
}

function jailadmin_admin_settings_network_submit($form, &$form_state) {
    if (isset($form_state['values']['new_name']) && strlen($form_state['values']['new_name'])) {
        $network = new Network;

        $network->name = $form_state['values']['new_name'];
        $network->device = $form_state['values']['new_device'];
        $network->ip = $form_state['values']['new_ip'];

        $network->Create();

        /* Reset form to be able to select the network without attempting to create a new one of the same name */
        $form_state['input']['new_name'] = '';
        $form_state['input']['new_ip'] = '';
        $form_state['input']['new_device'] = '';
    }

    if (isset($form_state['values']['device']) && strlen($form_state['values']['device'])) {
        $network = Network::Load($form_state['values']['network']);

        $network->device = $form_state['values']['device'];
        $network->ip = $form_state['values']['ip'];

        $network->Persist();
    }

    $form_state['rebuild'] = TRUE;
}

function jailadmin_admin_settings_network_bring_online($form, &$form_state) {
    $network = Network::Load($form_state['values']['network']);

    if ($network->BringOnline())
        drupal_set_message(t('Network @network brought online', array('@network' => $network->name)));
    else
        drupal_set_message(t('Network @network failed to start', array('@network' => $network->name)), 'error');

    $form_state['rebuild'] = TRUE;
}

function jailadmin_admin_settings_network_bring_offline($form, &$form_state) {
    $network = Network::Load($form_state['values']['network']);

    if ($network->BringOffline())
        drupal_set_message(t('Network @network brought offline', array('@network' => $network->name)));
    else
        drupal_set_message(t('Network @network failed to be brought offline', array('@network' => $network->name)), 'error');

    $form_state['rebuild'] = TRUE;
}

function jailadmin_admin_settings_network_delete($form, &$form_state) {
    $network = Network::Load($form_state['values']['network']);

    $network->Delete();

    drupal_set_message(t('Network @network deleted', array('@network' => $network->name)));

    $form_state['rebuild'] = TRUE;
}