/*
 * Copyright (c) 2021-2025, Jacques Gagnon
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include "hid_generic.h"
#include "ps.h"
#include "wireless.h"

static to_generic_t to_generic_func[BT_TYPE_MAX] = {
    hid_to_generic, /* BT_HID_GENERIC */
    NULL, /* BT_PS3 (disabled) */
    NULL, /* BT_WII (disabled) */
    ps_to_generic, /* BT_PS */
    NULL, /* BT_SW (disabled) */
    NULL, /* BT_SW2 (disabled) */
};

static fb_from_generic_t fb_from_generic_func[BT_TYPE_MAX] = {
    hid_fb_from_generic, /* BT_HID_GENERIC */
    NULL, /* BT_PS3 (disabled) */
    NULL, /* BT_WII (disabled) */
    ps_fb_from_generic, /* BT_PS */
    NULL, /* BT_SW (disabled) */
    NULL, /* BT_SW2 (disabled) */
};

int32_t wireless_to_generic(struct bt_data *bt_data, struct wireless_ctrl *ctrl_data) {
    int32_t ret = -1;

    if (to_generic_func[bt_data->base.pids->type]) {
        ret = to_generic_func[bt_data->base.pids->type](bt_data, ctrl_data);
    }

    return ret;
}

bool wireless_fb_from_generic(struct generic_fb *fb_data, struct bt_data *bt_data) {
    bool ret = false;

    if (fb_from_generic_func[bt_data->base.pids->type]) {
        ret = fb_from_generic_func[bt_data->base.pids->type](fb_data, bt_data);
    }

    return ret;
}

