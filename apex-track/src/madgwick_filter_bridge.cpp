#include "apex_imu.h"
#include "madgwick_filter.hpp"

#include <cmath>

static espp::MadgwickFilter filter(0.08f);

extern "C" void imu_update_filter(const imu_data_t *data, float dt_seconds,
                                   imu_orientation_t *orientation)
{
    constexpr float degrees_to_radians = 0.017453292519943295f;

    filter.update(
        dt_seconds,
        data->accel_x_g,
        data->accel_y_g,
        data->accel_z_g,
        data->gyro_x_dps * degrees_to_radians,
        data->gyro_y_dps * degrees_to_radians,
        data->gyro_z_dps * degrees_to_radians,
        data->mag_x_ut,
        data->mag_y_ut,
        data->mag_z_ut);
    filter.get_euler(orientation->pitch_deg, orientation->roll_deg, orientation->yaw_deg);
}