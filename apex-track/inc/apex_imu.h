#ifndef APEX_IMU_H
#define APEX_IMU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	float accel_x_g;
	float accel_y_g;
	float accel_z_g;
	float gyro_x_dps;
	float gyro_y_dps;
	float gyro_z_dps;
	float mag_x_ut;
	float mag_y_ut;
	float mag_z_ut;
	float temp_c;
	uint32_t timestamp_ms;
} imu_data_t;

typedef struct {
	float pitch_deg;
	float roll_deg;
	float yaw_deg;
} imu_orientation_t;

void imu_setup_task(void *pvParameters);
void imu_monitor_task(void *pvParameters);
void imu_update_filter(const imu_data_t *data, float dt_seconds, imu_orientation_t *orientation);

#ifdef __cplusplus
}
#endif

#endif
