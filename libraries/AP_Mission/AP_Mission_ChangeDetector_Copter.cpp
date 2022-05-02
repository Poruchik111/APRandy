/// @file    AP_Mission_ChangeDetector_Copter.cpp
/// @brief   Detects changes in the next few nav commands in the mission

#include "AP_Mission_ChangeDetector_Copter.h"
#include <GCS_MAVLink/GCS.h>

extern const AP_HAL::HAL& hal;

// check for changes to mission. returns required response to change (if any)
// using_next_command should be set to try if waypoint controller is already using the next navigation command
AP_Mission_ChangeDetector_Copter::ChangeResponseType AP_Mission_ChangeDetector_Copter::check_for_mission_change(bool using_next_command)
{
    // take backup of command list
    MissionCommandList cmd_list_bak = mis_change_detect;

    uint8_t first_changed_cmd_idx = 0;
    if (!AP_Mission_ChangeDetector::check_for_mission_change(first_changed_cmd_idx)) {
        // the mission has not changed
        return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
    }

    // if the current command has change a reset is always required
    // ToDo: check this handles mission erased
    if (first_changed_cmd_idx == 0) {
        gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st comment changed, Reset");
        return AP_Mission_ChangeDetector_Copter::ChangeResponseType::RESET_REQUIRED;
    }

    // 2nd or 3rd command has been added, changed or deleted

    const bool cmd0_has_pause = (cmd_list_bak.cmd[0].p1 > 0);
    const bool cmd0_was_wp = (cmd_list_bak.cmd[0].id == MAV_CMD_NAV_WAYPOINT ||
                             cmd_list_bak.cmd[0].id == MAV_CMD_NAV_LOITER_UNLIM ||
                             cmd_list_bak.cmd[0].id == MAV_CMD_NAV_LOITER_TIME);
    const bool cmd0_was_spline = (cmd_list_bak.cmd[0].id == MAV_CMD_NAV_SPLINE_WAYPOINT);

    // if 1st segment is nether a wp or spline then return NONE
    if (!cmd0_was_wp && !cmd0_was_spline) {
        gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st was neither wp nor spline, None");
        return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
    }

// we don't check that we have 1
    const bool cmd1_was_wp = (cmd_list_bak.cmd[1].id == MAV_CMD_NAV_WAYPOINT ||
                             cmd_list_bak.cmd[1].id == MAV_CMD_NAV_LOITER_UNLIM ||
                             cmd_list_bak.cmd[1].id == MAV_CMD_NAV_LOITER_TIME);
    const bool cmd1_was_spline = (cmd_list_bak.cmd[1].id == MAV_CMD_NAV_SPLINE_WAYPOINT);
    // if 1st segment (wp or spline) has pause then return NONE
    if (cmd0_has_pause || (!cmd1_was_wp && !cmd1_was_spline)) {
        gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st has pause, None");
        return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
    }

    // if 1st is wp (without a pause), if add of 2nd then ADD_NEXT_WAYPOINT
    const bool cmd1_added = (cmd_list_bak.cmd_count == 1) && (mis_change_detect.cmd_count > 1);
    const bool cmd1_is_wp = (mis_change_detect.cmd[1].id == MAV_CMD_NAV_WAYPOINT ||
                             mis_change_detect.cmd[1].id == MAV_CMD_NAV_LOITER_UNLIM ||
                             mis_change_detect.cmd[1].id == MAV_CMD_NAV_LOITER_TIME);
    const bool cmd1_is_spline = (cmd_list_bak.cmd[1].id == MAV_CMD_NAV_SPLINE_WAYPOINT);

    // Currently we don't support set_destination_speed_max after leg has been started
    // Therefore we can't add a spline without reseting
    if (cmd0_was_wp && cmd1_added && cmd1_is_wp) {
        if (cmd1_is_wp) {
            // 2nd is wp
            gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is wp, no pause, 2nd added, AddNextWP");
            return AP_Mission_ChangeDetector_Copter::ChangeResponseType::ADD_NEXT_WAYPOINT;
        } else if(cmd1_is_spline) {
            // 2nd is spline
            gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is wp, no pause, 2nd added but not wp, AddNextWP");
            return AP_Mission_ChangeDetector_Copter::ChangeResponseType::RESET_REQUIRED;
        } else {
            // 2nd is nether wp or spline
            gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is wp, no pause, 2nd added but not wp, AddNextWP");
            return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
        }
    }

    if (cmd0_was_wp) {
        // 1st was wp
        if (cmd1_was_wp) {
            // 2nd was wp
            if (first_changed_cmd_idx == 1) {
                // if 2nd has changed
                if (using_next_command) {
                    gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is wp, 2nd wp changed, Reset");
                    return AP_Mission_ChangeDetector_Copter::ChangeResponseType::RESET_REQUIRED;
                } else {
                    gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is wp, not using changed 2nd wp, None");
                    return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
                }
            } else {
                // 3rd has changed
                gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is wp, 2nd wp same, None");
                return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
            }
        } else {
            // if 1st segment spline
            gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is wp, 2nd spline with change, Reset");
            return AP_Mission_ChangeDetector_Copter::ChangeResponseType::RESET_REQUIRED;
        }
    } else {
        // 1st was spline
        // we have already checked for other waypoint types
        if (cmd1_was_wp) {
            // 2nd was wp
            if (first_changed_cmd_idx == 1) {
                // 2nd has changed
                gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is spline, 2nd wp changed, None");
                return AP_Mission_ChangeDetector_Copter::ChangeResponseType::RESET_REQUIRED;
            } else {
                // 2nd not changed
                gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is spline, 2nd wp same, None");
                return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
            }
        } else if (cmd1_was_spline) {
            // 2nd was spline
            const bool cmd1_has_pause = (cmd_list_bak.cmd[1].p1 > 0);
// we don't check that we have 2
            const bool cmd2_was_wp = (cmd_list_bak.cmd[2].id == MAV_CMD_NAV_WAYPOINT ||
                             cmd_list_bak.cmd[2].id == MAV_CMD_NAV_LOITER_UNLIM ||
                             cmd_list_bak.cmd[2].id == MAV_CMD_NAV_LOITER_TIME);
            const bool cmd2_was_spline = (cmd_list_bak.cmd[2].id == MAV_CMD_NAV_SPLINE_WAYPOINT);
            if ((first_changed_cmd_idx == 2) && (cmd1_has_pause || (!cmd2_was_wp && !cmd2_was_spline))) {
                // 2nd not pause and 3nd has changed
                gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is spline, 2nd spline with pause, None");
                return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
            } else {
                gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: 1st is spline, 2nd spline with change, Reset");
                return AP_Mission_ChangeDetector_Copter::ChangeResponseType::RESET_REQUIRED;
            }
        }
    }

    // next command is being used and it has changed so reset required
    gcs().send_text(MAV_SEVERITY_CRITICAL, "check_for_mission_change: got to end");
    return AP_Mission_ChangeDetector_Copter::ChangeResponseType::NONE;
}

