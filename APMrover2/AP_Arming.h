#pragma once

#include <AP_Arming/AP_Arming.h>
#include <AC_Fence/AC_Fence.h>

/*
  a rover-specific arming class
 */
class AP_Arming_Rover : public AP_Arming
{
public:
    static AP_Arming_Rover create(const AP_AHRS &ahrs_ref, const AP_Baro &baro, Compass &compass, const AP_BattMonitor &battery,
                                  const AC_Fence &fence) {
        return AP_Arming_Rover{ahrs_ref, baro, compass, battery, fence};
    }

    constexpr AP_Arming_Rover(AP_Arming_Rover &&other) = default;

    /* Do not allow copies */
    AP_Arming_Rover(const AP_Arming_Rover &other) = delete;
    AP_Arming_Rover &operator=(const AP_Baro&) = delete;

    bool pre_arm_checks(bool report) override;
    bool pre_arm_rc_checks(const bool display_failure);
    bool gps_checks(bool display_failure) override;

protected:
    AP_Arming_Rover(const AP_AHRS &ahrs_ref, const AP_Baro &baro, Compass &compass,
                    const AP_BattMonitor &battery, const AC_Fence &fence)
        : AP_Arming(ahrs_ref, baro, compass, battery),
          _fence(fence)
    {
    }

    enum HomeState home_status() const override;
    bool fence_checks(bool report);

private:
    const AC_Fence& _fence;
};
