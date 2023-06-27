-- eams-point-at-turbine2.lua: aims gimbal at wind turbine using 360 lidar data
--
-- How to use
--    Connect 360 lidar (e.g. RPLidarS1, SF45b) to one of the autopilot's serial ports
--    Set SERIALx_PROTOCOL = 11 (Lidar360) where "x" corresponds to the serial port connected to the lidar
--    Set PRX1_TYPE = 5 (RPLidar) or 8 (SF45b)
--    Set SCR_ENABLE = 1 to enable scripting and reboot the autopilot

-- setup param block for aerobatics, reserving 30 params beginning with AERO_
local PARAM_TABLE_KEY = 112
local PARAM_TABLE_PREFIX = "EAMS_"
assert(param:add_table(PARAM_TABLE_KEY, "EAMS_", 5), 'could not add param table')

-- add a parameter and bind it to a variable
function bind_add_param(name, idx, default_value)
    assert(param:add_param(PARAM_TABLE_KEY, idx, name, default_value), string.format('could not add param %s', name))
    return Parameter(PARAM_TABLE_PREFIX .. name)
end

local JUMP_DIST = bind_add_param("JUMP_DIST", 1, 0.5)   -- object edge detected by a lidar jump of this many meters
local DEBUG = bind_add_param("DEBUG", 2, 0)             -- debug 0:Disabled, 1:Enabled
local ANGLE_MIN = bind_add_param("ANGLE_MIN", 3, -45)   -- minimum angle to check for obstacles
local ANGLE_MAX = bind_add_param("ANGLE_MAX", 4, 45)    -- maximum angle to check for obstacles
local PITCH = bind_add_param("PITCH", 5, 0)             -- pitch angle default in degrees (+ve up, -ve down).  zero to use current pitch target

local INIT_INTERVAL_MS = 3000       -- attempt to initialise at this interval
local UPDATE_INTERVAL_MS = 1000     -- update at this interval
local MAV_SEVERITY = {EMERGENCY=0, ALERT=1, CRITICAL=2, ERROR=3, WARNING=4, NOTICE=5, INFO=6, DEBUG=7}
local initialised = false           -- true once applet has been initialised

-- run initialisation checks
-- returns true on success, false on failure to initialise
function init()
  -- check proximity sensor has been enabled
  if proximity:num_sensors() < 1 then
    gcs:send_text(MAV_SEVERITY.CRITICAL, "EAMS: no proximity sensor found")
    return false
  end

  -- if we get this far then initialisation has completed successfully
  gcs:send_text(MAV_SEVERITY.CRITICAL, "EAMS: started")
  return true
end

function update()

  -- init
  if not initialised then
    initialised = init()
    return update, INIT_INTERVAL_MS
  end

  -- send detction parameters
  --proximity:set_sweep_params(DEBUG:get(), JUMP_DIST:get(), ANGLE_MIN:get(), ANGLE_MAX:get())

  -- check for closest object found during last sweep  
  local angle_deg = oadatabase:dir_to_largest_object()
  if angle_deg then
    if DEBUG:get() > 0 then
      gcs:send_text(MAV_SEVERITY.INFO, string.format("EAMS: closest at %f deg", angle_deg))
    end
    -- pitch angle comes from parameter if non-zero, otherwise uses current target pitch angle
    local target_pitch = PITCH:get()
    if target_pitch == 0 then
      local _curr_target_roll, curr_target_pitch, _curr_target_yaw, _yaw_is_ef = mount:get_angle_target(0)
      if curr_target_pitch then
        target_pitch = curr_target_pitch
      end
    end
    mount:set_angle_target(0, target_pitch, 0, angle_deg, false)
  end

  return update, UPDATE_INTERVAL_MS
end

return update(), 2000 -- first message may be displayed 2 seconds after start-up
