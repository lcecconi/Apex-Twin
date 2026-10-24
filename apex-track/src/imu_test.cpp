#include <Arduino.h>
#include <ICM_20948.h>
#include <math.h>

namespace {
constexpr uint8_t kSdaPin = 11;
constexpr uint8_t kSclPin = 12;
constexpr uint32_t kI2cFrequency = 400000;
constexpr uint32_t kSerialBaud = 230400;
constexpr float kDegreesToRadians = 0.01745329252f;
constexpr float kGravityMg = 1000.0f;
constexpr float kMadgwickBeta = 0.08f;

// User bank 2, FSYNC_CONFIG: 1 routes the FSYNC sample to TEMP_OUT_L.
constexpr uint8_t kFsyncConfigRegister = AGB2_REG_FSYNC_CONFIG;
constexpr uint8_t kFsyncToTemperatureLsb = 1;

ICM_20948_I2C imu;
uint32_t previousSampleMicros = 0;
float q0 = 1.0f;
float q1 = 0.0f;
float q2 = 0.0f;
float q3 = 0.0f;

bool findAddress(uint8_t &ad0Value)
{
    const uint8_t addresses[] = {0x68, 0x69};
    for (uint8_t address : addresses) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            ad0Value = address == 0x69 ? 1 : 0;
            return true;
        }
    }
    return false;
}

bool configureFsync()
{
    // FSYNC is sampled on the sensor's FSYNC input. Use an active-high signal
    // and store each sampled value in the temperature sensor LSB.
    bool success = true;
    success &= imu.cfgFsyncActiveLow(false) == ICM_20948_Stat_Ok;
    success &= imu.cfgFsyncIntMode(false) == ICM_20948_Stat_Ok;
    success &= imu.setBank(2) == ICM_20948_Stat_Ok;

    uint8_t fsyncConfig = kFsyncToTemperatureLsb;
    success &= imu.write(kFsyncConfigRegister, &fsyncConfig, 1) == ICM_20948_Stat_Ok;
    success &= imu.setBank(0) == ICM_20948_Stat_Ok;
    return success;
}

void normalizeQuaternion()
{
    const float norm = sqrtf((q0 * q0) + (q1 * q1) + (q2 * q2) + (q3 * q3));
    if (norm > 0.0f) {
        q0 /= norm;
        q1 /= norm;
        q2 /= norm;
        q3 /= norm;
    }
}

void updateQuaternion(float ax, float ay, float az,
                      float gx, float gy, float gz,
                      float mx, float my, float mz,
                      float dt)
{
    gx *= kDegreesToRadians;
    gy *= kDegreesToRadians;
    gz *= kDegreesToRadians;

    const float qDot1 = 0.5f * ((-q1 * gx) - (q2 * gy) - (q3 * gz));
    const float qDot2 = 0.5f * ((q0 * gx) + (q2 * gz) - (q3 * gy));
    const float qDot3 = 0.5f * ((q0 * gy) - (q1 * gz) + (q3 * gx));
    const float qDot4 = 0.5f * ((q0 * gz) + (q1 * gy) - (q2 * gx));

    const float accelNorm = sqrtf((ax * ax) + (ay * ay) + (az * az));
    const float magNorm = sqrtf((mx * mx) + (my * my) + (mz * mz));
    if (accelNorm <= 0.0f || magNorm <= 0.0f) {
        q0 += qDot1 * dt;
        q1 += qDot2 * dt;
        q2 += qDot3 * dt;
        q3 += qDot4 * dt;
        normalizeQuaternion();
        return;
    }

    ax /= accelNorm;
    ay /= accelNorm;
    az /= accelNorm;
    mx /= magNorm;
    my /= magNorm;
    mz /= magNorm;

    const float twoQ0 = 2.0f * q0;
    const float twoQ1 = 2.0f * q1;
    const float twoQ2 = 2.0f * q2;
    const float twoQ3 = 2.0f * q3;
    const float fourQ0 = 4.0f * q0;
    const float fourQ1 = 4.0f * q1;
    const float fourQ2 = 4.0f * q2;
    const float eightQ1 = 8.0f * q1;
    const float eightQ2 = 8.0f * q2;
    const float q0q0 = q0 * q0;
    const float q1q1 = q1 * q1;
    const float q2q2 = q2 * q2;
    const float q3q3 = q3 * q3;

    const float hx = (mx * (1.0f - 2.0f * q2q2 - 2.0f * q3q3)) +
                     (my * (twoQ1 * q2 - twoQ0 * q3)) +
                     (mz * (twoQ1 * q3 + twoQ0 * q2));
    const float hy = (mx * (twoQ1 * q2 + twoQ0 * q3)) +
                     (my * (1.0f - 2.0f * q1q1 - 2.0f * q3q3)) +
                     (mz * (twoQ2 * q3 - twoQ0 * q1));
    const float twoBx = sqrtf((hx * hx) + (hy * hy));
    const float twoBz = (mx * (twoQ1 * q3 - twoQ0 * q2)) +
                        (my * (twoQ0 * q1 + twoQ2 * q3)) +
                        (mz * (1.0f - 2.0f * q1q1 - 2.0f * q2q2));

    const float s0 = (-twoQ2 * (2.0f * (q1 * q3 - q0 * q2) - ax)) +
                     (twoQ1 * (2.0f * (q0 * q1 + q2 * q3) - ay)) -
                     (twoBz * q2 * (twoBx * (0.5f - q2q2 - q3q3) +
                                    twoBz * (q1 * q3 - q0 * q2) - mx)) +
                     ((-twoBx * q3 + twoBz * q1) *
                      (twoBx * (q1 * q2 - q0 * q3) +
                       twoBz * (q0 * q1 + q2 * q3) - my)) +
                     (twoBx * q2 * (twoBx * (q0 * q2 + q1 * q3) +
                                    twoBz * (0.5f - q1q1 - q2q2) - mz));
    const float s1 = (twoQ3 * (2.0f * (q1 * q3 - q0 * q2) - ax)) +
                     (twoQ0 * (2.0f * (q0 * q1 + q2 * q3) - ay)) -
                     (fourQ1 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - az)) +
                     (twoBz * q3 * (twoBx * (0.5f - q2q2 - q3q3) +
                                    twoBz * (q1 * q3 - q0 * q2) - mx)) +
                     ((twoBx * q2 + twoBz * q0) *
                      (twoBx * (q1 * q2 - q0 * q3) +
                       twoBz * (q0 * q1 + q2 * q3) - my)) +
                     ((twoBx * q3 - fourQ1) *
                      (twoBx * (q0 * q2 + q1 * q3) +
                       twoBz * (0.5f - q1q1 - q2q2) - mz));
    const float s2 = (-twoQ0 * (2.0f * (q1 * q3 - q0 * q2) - ax)) +
                     (twoQ3 * (2.0f * (q0 * q1 + q2 * q3) - ay)) -
                     (fourQ2 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - az)) +
                     ((-fourQ2 * q3 + twoBz * q1) *
                      (twoBx * (0.5f - q2q2 - q3q3) +
                       twoBz * (q1 * q3 - q0 * q2) - mx)) +
                     ((twoBx * q1 + twoBz * q3) *
                      (twoBx * (q1 * q2 - q0 * q3) +
                       twoBz * (q0 * q1 + q2 * q3) - my)) +
                     ((twoBx * q0 - fourQ2) *
                      (twoBx * (q0 * q2 + q1 * q3) +
                       twoBz * (0.5f - q1q1 - q2q2) - mz));
    const float s3 = (twoQ1 * (2.0f * (q1 * q3 - q0 * q2) - ax)) +
                     (twoQ2 * (2.0f * (q0 * q1 + q2 * q3) - ay)) +
                     ((-twoBx * q3 + twoBz * q1) *
                      (twoBx * (0.5f - q2q2 - q3q3) +
                       twoBz * (q1 * q3 - q0 * q2) - mx)) +
                     ((-twoBx * q0 + twoBz * q2) *
                      (twoBx * (q1 * q2 - q0 * q3) +
                       twoBz * (q0 * q1 + q2 * q3) - my)) +
                     (twoBx * q1 *
                      (twoBx * (q0 * q2 + q1 * q3) +
                       twoBz * (0.5f - q1q1 - q2q2) - mz));

    const float stepNorm = sqrtf((s0 * s0) + (s1 * s1) + (s2 * s2) + (s3 * s3));
    const float correction = stepNorm > 0.0f ? kMadgwickBeta / stepNorm : 0.0f;
    q0 += (qDot1 - correction * s0) * dt;
    q1 += (qDot2 - correction * s1) * dt;
    q2 += (qDot3 - correction * s2) * dt;
    q3 += (qDot4 - correction * s3) * dt;
    normalizeQuaternion();
}

void printMeasurements(const ICM_20948_AGMT_t &data)
{
    const uint32_t now = micros();
    float dt = previousSampleMicros == 0
                   ? 0.01f
                   : static_cast<float>(now - previousSampleMicros) * 1.0e-6f;
    previousSampleMicros = now;
    if (dt <= 0.0f || dt > 0.1f) {
        dt = 0.01f;
    }

    const float ax = imu.accX() / kGravityMg;
    const float ay = imu.accY() / kGravityMg;
    const float az = imu.accZ() / kGravityMg;
    updateQuaternion(ax, ay, az, imu.gyrX(), imu.gyrY(), imu.gyrZ(),
                     imu.magX(), imu.magY(), imu.magZ(), dt);

    Serial.printf(
        "{\"accel\":[%.3f,%.3f,%.3f],\"gyro\":[%.3f,%.3f,%.3f],"
        "\"mag\":[%.3f,%.3f,%.3f],\"temp\":%.3f,"
        "\"quaternion\":[%.6f,%.6f,%.6f,%.6f],\"fsync_temp_lsb\":%u}\n",
        imu.accX(), imu.accY(), imu.accZ(),
        imu.gyrX(), imu.gyrY(), imu.gyrZ(),
        imu.magX(), imu.magY(), imu.magZ(), imu.temp(),
        q0, q1, q2, q3,
        static_cast<unsigned>(data.tmp.raw.u8bit[1] & 0x01));
}
} // namespace

void setup()
{
    Serial.begin(kSerialBaud);
    delay(500);

    Wire.begin(kSdaPin, kSclPin, kI2cFrequency);
    Serial.printf("\nICM-20948 raw sensor quaternion test (SDA=IO%d, SCL=IO%d)\n",
                  kSdaPin, kSclPin);

    uint8_t ad0Value = 0;
    if (!findAddress(ad0Value)) {
        Serial.println("No ICM-20948 found at 0x68 or 0x69.");
        return;
    }

    imu.begin(Wire, ad0Value);
    Serial.printf("Sensor initialization at 0x%02X: %s\n",
                  ad0Value == 0 ? 0x68 : 0x69, imu.statusString());
    if (imu.status != ICM_20948_Stat_Ok) {
        Serial.println("Sensor initialization failed.");
        return;
    }

    if (!configureFsync()) {
        Serial.printf("FSYNC configuration failed: %s\n", imu.statusString());
        return;
    }

    Serial.println("Raw accel, gyro, magnetometer and temperature output enabled.");
    Serial.println("FSYNC is routed to TEMP_OUT_L; connect the external signal to FSYNC.");
}

void loop()
{
    if (!imu.dataReady()) {
        delay(1);
        return;
    }

    const ICM_20948_AGMT_t data = imu.getAGMT();
    if (imu.status == ICM_20948_Stat_Ok) {
        printMeasurements(data);
    } else {
        Serial.printf("Sensor read failed: %s\n", imu.statusString());
    }
}
