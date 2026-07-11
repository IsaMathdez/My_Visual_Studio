
// Version 3.53.1 SENSOR DE OLEAJE
//
// Base kept from version 3.3 (stable capture, memory layout, timing guards)
// NEW: Debugging a_z_dyn_mss
// NEW: Vector for gravity debugging
//
// What this file does:
// - Reads MPU6050 at 10 Hz over I2C (SDA=21, SCL=22)
// - Collects bursts of 512 samples (~51.2 s)
// - Estimates attitude with a complementary filter
// - Removes gravity from the vertical acceleration
// - Converts acceleration spectrum to wave-elevation spectrum
// - Estimates significant wave height and wave periods with less drift
// - Prints significant wave height, peak/mean periods, and a
//   relative dominant direction in the sensor frame
//
// Important notes:
// - The direction produced here is RELATIVE to the sensor/body frame.
//   Absolute geographic wave direction needs a heading reference (magnetometer,
//   GNSS course-over-ground, or a known fixed orientation of the buoy).
// - For very small waves (e.g. ~4 cm peak-to-crest), mechanical stability,
//   calibration, and filtering are critical.
// - This version prints results to the serial console (ESP-IDF monitor).
//
// Fixes in this version:
// - Frequency-domain integration is used for wave height / period estimation.
// - The DFT loop yields periodically so the ESP32 watchdog remains happy.
// - The rest of the timing and buffer structure remains close to version 3.3.

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "driver/i2c.h"

#define TAG "WAVE_BUOY"

// -------------------- I2C / MPU6050 --------------------
#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_FREQ_HZ 400000

#define MPU6050_ADDR 0x68

#define MPU_PWR_MGMT_1 0x6B
#define MPU_SMPLRT_DIV 0x19
#define MPU_CONFIG 0x1A
#define MPU_GYRO_CONFIG 0x1B
#define MPU_ACCEL_CONFIG 0x1C
#define MPU_ACCEL_XOUT_H 0x3B

// -------------------- Sampling / processing --------------------
#define SAMPLE_RATE_HZ 10.0f
#define DT (1.0f / SAMPLE_RATE_HZ)

#define BURST_DURATION_SEC 60
#define WAIT_BETWEEN_BURSTS_SEC 5

#define BURST_SAMPLES \
    ((int)(SAMPLE_RATE_HZ * BURST_DURATION_SEC))

#define BURST_SECONDS \
    ((float)BURST_DURATION_SEC)

// Band limits used when integrating acceleration PSD -> elevation PSD.
// These help avoid excessive amplification of DC / very low frequency drift.
#define WAVE_MIN_HZ 0.12f // Estaba en 0.05
#define WAVE_MAX_HZ 1.0f  // Estaba en 2.0

// MPU6050 scale factors for configured ranges:
// Accel ±2g -> 16384 LSB/g
// Gyro ±250 dps -> 131 LSB/(deg/s)
#define ACC_LSB_PER_G 16384.0f
#define GYRO_LSB_PER_DPS 131.0f
#define G0 9.80665f

#define GRAVITY_ALPHA 0.995f

static const float ALPHA_CF = 0.98f; // Complementary filter weight, changed from 0.98 to 0.995 for better low-frequency response

// Buffers globales para evitar Stack Overflow
static float heave[BURST_SAMPLES];
static float roll_hist[BURST_SAMPLES];
static float pitch_hist[BURST_SAMPLES];

static float eta[BURST_SAMPLES];
static float r[BURST_SAMPLES];
static float p[BURST_SAMPLES];

static float acc[BURST_SAMPLES];
static float vel[BURST_SAMPLES];
static float disp[BURST_SAMPLES];

// added
static float debug_freq[256];
static float debug_peta[256];
static int debug_bins = 0;

// -------------------- Debug aceleracion vertical --------------------
static float az_min = 1000.0f;
static float az_max = -1000.0f;
static double az_sum = 0.0;
static double az_sum2 = 0.0;
static int az_count = 0;

// Gravity estimator (Earth frame)
static float grav_x = 0.0f;
static float grav_y = 0.0f;
static float grav_z = G0;

// -------------------- Small structs --------------------
typedef struct
{
    int16_t ax, ay, az;
    int16_t temp;
    int16_t gx, gy, gz;
} mpu_raw_t;

typedef struct
{
    float ax_g, ay_g, az_g;
    float gx_rads, gy_rads, gz_rads;
} mpu_phys_t;

// -------------------- I2C helpers --------------------
static esp_err_t i2c_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t i2c_read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1)
    {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t mpu6050_read_raw(mpu_raw_t *raw)
{
    uint8_t buf[14];
    esp_err_t err = i2c_read_bytes(MPU6050_ADDR, MPU_ACCEL_XOUT_H, buf, sizeof(buf));
    if (err != ESP_OK)
        return err;

    raw->ax = (int16_t)((buf[0] << 8) | buf[1]);
    raw->ay = (int16_t)((buf[2] << 8) | buf[3]);
    raw->az = (int16_t)((buf[4] << 8) | buf[5]);
    raw->temp = (int16_t)((buf[6] << 8) | buf[7]);
    raw->gx = (int16_t)((buf[8] << 8) | buf[9]);
    raw->gy = (int16_t)((buf[10] << 8) | buf[11]);
    raw->gz = (int16_t)((buf[12] << 8) | buf[13]);
    return ESP_OK;
}

static esp_err_t mpu6050_init(void)
{
    esp_err_t err;

    // Wake up
    err = i2c_write_byte(MPU6050_ADDR, MPU_PWR_MGMT_1, 0x00);
    if (err != ESP_OK)
        return err;

    vTaskDelay(pdMS_TO_TICKS(100));

    // Sample rate
    err = i2c_write_byte(MPU6050_ADDR, MPU_SMPLRT_DIV, 99);
    if (err != ESP_OK)
        return err;

    // DLPF
    err = i2c_write_byte(MPU6050_ADDR, MPU_CONFIG, 0x03);
    if (err != ESP_OK)
        return err;

    // Gyro ±250 dps
    err = i2c_write_byte(MPU6050_ADDR, MPU_GYRO_CONFIG, 0x00);
    if (err != ESP_OK)
        return err;

    // Accel ±2g
    err = i2c_write_byte(MPU6050_ADDR, MPU_ACCEL_CONFIG, 0x00);
    if (err != ESP_OK)
        return err;

    return ESP_OK;
}

// -------------------- Calibration --------------------
static void calibrate_gyro(float *gx_bias_rads, float *gy_bias_rads, float *gz_bias_rads)
{
    const int n = 300;
    double sx = 0, sy = 0, sz = 0;
    mpu_raw_t raw;

    ESP_LOGI(TAG, "Calibrando giroscopio... mantén la boya quieta");
    for (int i = 0; i < n; i++)
    {
        if (mpu6050_read_raw(&raw) == ESP_OK)
        {
            sx += raw.gx;
            sy += raw.gy;
            sz += raw.gz;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    float gx_dps = (float)(sx / n) / GYRO_LSB_PER_DPS;
    float gy_dps = (float)(sy / n) / GYRO_LSB_PER_DPS;
    float gz_dps = (float)(sz / n) / GYRO_LSB_PER_DPS;

    *gx_bias_rads = gx_dps * (float)M_PI / 180.0f;
    *gy_bias_rads = gy_dps * (float)M_PI / 180.0f;
    *gz_bias_rads = gz_dps * (float)M_PI / 180.0f;

    ESP_LOGI(TAG, "Bias gyro: gx=%.6f rad/s, gy=%.6f rad/s, gz=%.6f rad/s",
             *gx_bias_rads, *gy_bias_rads, *gz_bias_rads);

    ESP_LOGI(TAG, "Heap libre: %lu bytes",
             (unsigned long)esp_get_free_heap_size());
}

// -------------------- Signal helpers --------------------
static void remove_mean(float *x, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += x[i];
    float mean = (float)(sum / n);
    for (int i = 0; i < n; i++)
        x[i] -= mean;
}

static void detrend_linear(float *x, int n)
{
    // Least-squares fit y = a*i + b
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (int i = 0; i < n; i++)
    {
        double xi = (double)i;
        double yi = (double)x[i];
        sx += xi;
        sy += yi;
        sxx += xi * xi;
        sxy += xi * yi;
    }

    double denom = (n * sxx - sx * sx);
    if (fabs(denom) < 1e-12)
        return;

    double a = (n * sxy - sx * sy) / denom;
    double b = (sy - a * sx) / n;

    for (int i = 0; i < n; i++)
    {
        x[i] -= (float)(a * i + b);
    }
}

static float mean_square(const float *x, int n)
{
    double s = 0.0;
    for (int i = 0; i < n; i++)
        s += (double)x[i] * (double)x[i];
    return (float)(s / n);
}

static float stddev(const float *x, int n)
{
    double s = 0.0, ss = 0.0;
    for (int i = 0; i < n; i++)
    {
        s += x[i];
        ss += (double)x[i] * (double)x[i];
    }
    double m = s / n;
    double v = (ss / n) - m * m;
    if (v < 0)
        v = 0;
    return (float)sqrt(v);
}

static void update_gravity_estimate(
    float ax,
    float ay,
    float az)
{
    grav_x =
        GRAVITY_ALPHA * grav_x +
        (1.0f - GRAVITY_ALPHA) * ax;

    grav_y =
        GRAVITY_ALPHA * grav_y +
        (1.0f - GRAVITY_ALPHA) * ay;

    grav_z =
        GRAVITY_ALPHA * grav_z +
        (1.0f - GRAVITY_ALPHA) * az;
}

static float vertical_dynamic_acceleration(
    float ax,
    float ay,
    float az)
{
    float gx = grav_x;
    float gy = grav_y;
    float gz = grav_z;

    float gnorm = sqrtf(
        gx * gx +
        gy * gy +
        gz * gz);

    if (gnorm < 1e-6f)
        return 0.0f;

    gx /= gnorm;
    gy /= gnorm;
    gz /= gnorm;

    float aproj =
        ax * gx +
        ay * gy +
        az * gz;

    return aproj - G0;
}

// -------------------- DFT / spectral moments --------------------
// Returns moments m0, m1, m2 from the acceleration spectrum after
// converting it to the equivalent wave-elevation spectrum in the
// frequency domain: S_eta(f) = S_a(f) / (2*pi*f)^4.
static void compute_wave_moments_from_accel(const float *a_mss, int n, float fs,
                                            float *m0, float *m1, float *m2,
                                            float *f_peak)
{
    const int kmax = n / 2;
    const float df = fs / n;

    double a_sum = 0.0;
    for (int i = 0; i < n; i++)
    {
        a_sum += a_mss[i];
    }
    const float a_mean = (float)(a_sum / n);

    // Hann window power correction
    double wsum2 = 0.0;
    for (int i = 0; i < n; i++)
    {
        float w = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (n - 1)));
        wsum2 += (double)w * (double)w;
    }
    const float U = (float)(wsum2 / n);

    float local_m0 = 0.0f;
    float local_m1 = 0.0f;
    float local_m2 = 0.0f;
    float local_fpeak = 0.0f;
    float local_peak_power = -1.0f;

    debug_bins = 0;

    for (int k = 0; k <= kmax; k++)
    {
        // Keep the task watchdog happy during long spectral windows.
        if ((k % 10) == 0)
        {
            vTaskDelay(1);
        }

        double re = 0.0;
        double im = 0.0;

        for (int n0 = 0; n0 < n; n0++)
        {
            float w = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * n0 / (n - 1)));
            float xn = (a_mss[n0] - a_mean) * w;
            double ang = -2.0 * M_PI * k * n0 / n;
            re += xn * cos(ang);
            im += xn * sin(ang);
        }

        double mag2 = re * re + im * im;

        // One-sided acceleration PSD estimate.
        float Paa;
        if (k == 0 || k == kmax)
        {
            Paa = (float)(mag2 / ((double)n * (double)n * U));
        }
        else
        {
            Paa = (float)(2.0 * mag2 / ((double)n * (double)n * U));
        }

        float f = k * df;
        if (f < WAVE_MIN_HZ || f > WAVE_MAX_HZ)
        {
            continue;
        }

        // Convert acceleration PSD to elevation PSD by dividing by omega^4.
        float omega = 2.0f * (float)M_PI * f;
        float omega2 = omega * omega;
        float Peta = Paa / (omega2 * omega2);

        if (debug_bins < 256)
        {
            debug_freq[debug_bins] = f;
            debug_peta[debug_bins] = Peta;
            debug_bins++;
        }

        local_m0 += Peta;
        local_m1 += f * Peta;
        local_m2 += f * f * Peta;

        if (Peta > local_peak_power)
        {
            local_peak_power = Peta;
            local_fpeak = f;
        }
    }

    printf("\n");
    printf("========= TOP PICOS ESPECTRALES =========\n");

    for (int nprint = 0; nprint < 10; nprint++)
    {
        int idx_max = -1;
        float max_val = -1.0f;

        for (int i = 0; i < debug_bins; i++)
        {
            if (debug_peta[i] > max_val)
            {
                max_val = debug_peta[i];
                idx_max = i;
            }
        }

        if (idx_max >= 0)
        {
            printf("%2d) f=%.3f Hz   Peta=%e\n",
                   nprint + 1,
                   debug_freq[idx_max],
                   debug_peta[idx_max]);

            debug_peta[idx_max] = -1.0f;
        }
    }

    printf("=========================================\n");

    *m0 = local_m0 * df;
    *m1 = local_m1 * df;
    *m2 = local_m2 * df;
    *f_peak = local_fpeak;
}

// -------------------- Main processing --------------------
static void process_burst(const float *acc_mss, const float *roll_rad, const float *pitch_rad, int n)
{
    // Copy into scratch buffers so we can reuse the original names.
    memcpy(eta, acc_mss, sizeof(float) * n);
    memcpy(r, roll_rad, sizeof(float) * n);
    memcpy(p, pitch_rad, sizeof(float) * n);

    // Prepare acceleration for spectral integration.
    remove_mean(eta, n);
    detrend_linear(eta, n);
    remove_mean(r, n);
    remove_mean(p, n);

    // Spectral analysis on vertical acceleration converted to elevation spectrum.
    float m0 = 0.0f, m1 = 0.0f, m2 = 0.0f, fp = 0.0f;
    compute_wave_moments_from_accel(eta, n, SAMPLE_RATE_HZ, &m0, &m1, &m2, &fp);

    printf("m0 = %.8e\n", m0);

    float Hs_spec = 4.0f * sqrtf(fmaxf(m0, 0.0f));
    float sigma_eta = sqrtf(fmaxf(m0, 0.0f));
    float Tp = (fp > 1e-6f) ? (1.0f / fp) : 0.0f;
    float Tm01 = (m1 > 1e-9f) ? (m0 / m1) : 0.0f;
    float Tm02 = (m2 > 1e-9f) ? sqrtf(m0 / m2) : 0.0f;

    // Relative direction: principal axis of roll/pitch ellipse in sensor frame.
    float var_r = mean_square(r, n);
    float var_p = mean_square(p, n);
    float cov_rp = 0.0f;
    for (int i = 0; i < n; i++)
        cov_rp += r[i] * p[i];
    cov_rp /= n;

    float dir_rad = 0.5f * atan2f(2.0f * cov_rp, var_r - var_p);
    float dir_deg = dir_rad * 180.0f / (float)M_PI;
    if (dir_deg < 0)
        dir_deg += 180.0f; // principal axis is 0..180 deg

    // Simple crest-to-trough estimate within the burst (debug only).
    float eta_min = eta[0], eta_max = eta[0];
    for (int i = 1; i < n; i++)
    {
        if (eta[i] < eta_min)
            eta_min = eta[i];
        if (eta[i] > eta_max)
            eta_max = eta[i];
    }
    float peak_to_crest = eta_max - eta_min;

    // Seleccionamos las variables mas representativas
    float Hs = Hs_spec;        // Altura significativa
    float periodo_marino = Tp; // Periodo medio de cruce por cero espectral
    float direccion_ola_deg = dir_deg;

    float acc_rms = 0.0f;

    for (int i = 0; i < n; i++)
    {
        acc_rms += acc_mss[i] * acc_mss[i];
    }

    acc_rms = sqrtf(acc_rms / n);

    // --- REPORTE OCEANOGRAFICO AVANZADO ---
    printf("\n=================================================================\n");
    printf(" 🌊   REPORTE DE OLEAJE PROCESADO (METODO ESPECTRAL ESTIMADO)   \n");
    printf("=================================================================\n");
    printf(" [MUESTREO] Duracion de Rafaga: %.1f segundos (%d muestras)\n", BURST_SECONDS, BURST_SAMPLES);
    printf("-----------------------------------------------------------------\n");
    printf(" [METRICA] Altura Significativa (Hs) : %.2f metros\n", Hs);
    printf(" [METRICA] Periodo Medio de Ola (Tz) : %.1f segundos\n", periodo_marino);
    printf(" [METRICA] Direccion Dominante       : %.1f deg (Angulo de Ataque)\n", direccion_ola_deg);
    printf("-----------------------------------------------------------------\n");
    printf(" [ESTADO DEL MAR] Escala Beaufort Est.: ");

    if (Hs < 0.1f)
        printf("Mar Espejo / Calma Absoluta\n");
    else if (Hs < 0.5f)
        printf("Mar Rizada (Olas pequenas y cortas)\n");
    else if (Hs < 1.25f)
        printf("Marejadilla (Olas de viento local)\n");
    else if (Hs < 2.5f)
        printf("Marejada (Oleaje Oceanico Firme)\n");
    else
        printf("Fuerte Marejada (Condiciones Criticas en Costa)\n");

    printf("=================================================================\n");

    // Datos tecnicos opcionales para depuracion
    printf(" [DEBUG] Hs espectral : %.3f m\n", Hs_spec);
    printf(" [DEBUG] Tm01         : %.3f s\n", Tm01);
    printf(" [DEBUG] Tp           : %.3f s\n", Tp);
    printf(" [DEBUG] f_pico       : %.3f Hz\n", fp);
    printf(" [DEBUG] Periodo Pico: %.2f s\n", (fp > 0.001f) ? (1.0f / fp) : 0.0f);
    printf(" [DEBUG] Acc RMS     : %.4f m/s²\n", acc_rms);
    printf(" [DEBUG] sigma(eta)   : %.4f m\n", sigma_eta);
    printf(" [DEBUG] Pico-cresta  : %.3f m\n", peak_to_crest);
    printf("=================================================================\n");
}

// -------------------- app_main --------------------
void app_main(void)
{
    // I2C init
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
        .clk_flags = 0,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0));
    ESP_LOGI(TAG, "I2C inicializado en SDA=%d, SCL=%d\n", I2C_SDA_PIN, I2C_SCL_PIN);

    ESP_ERROR_CHECK(mpu6050_init());
    ESP_LOGI(TAG, "MPU6050 listo\n");

    float gx_bias = 0.0f, gy_bias = 0.0f, gz_bias = 0.0f;
    calibrate_gyro(&gx_bias, &gy_bias, &gz_bias);

    float roll = 0.0f, pitch = 0.0f;
    bool first_attitude = true;

    int idx = 0;
    TickType_t last_wake = xTaskGetTickCount();

    ESP_LOGI(TAG, "Entrando al while principal\n");

    while (1)
    {
        // Keep a fixed sample rate around 10 Hz
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS((int)(1000.0f / SAMPLE_RATE_HZ)));

        mpu_raw_t raw;
        if (mpu6050_read_raw(&raw) != ESP_OK)
        {
            ESP_LOGW(TAG, "No se pudo leer MPU6050\n");
            continue;
        }

        mpu_phys_t s;
        s.ax_g = (float)raw.ax / ACC_LSB_PER_G;
        s.ay_g = (float)raw.ay / ACC_LSB_PER_G;
        s.az_g = (float)raw.az / ACC_LSB_PER_G;
        s.gx_rads = ((float)raw.gx / GYRO_LSB_PER_DPS) * (float)M_PI / 180.0f - gx_bias;
        s.gy_rads = ((float)raw.gy / GYRO_LSB_PER_DPS) * (float)M_PI / 180.0f - gy_bias;
        s.gz_rads = ((float)raw.gz / GYRO_LSB_PER_DPS) * (float)M_PI / 180.0f - gz_bias;

        // Estimate roll/pitch from accelerometer
        float roll_acc = atan2f(s.ay_g, s.az_g);
        float pitch_acc = atan2f(-s.ax_g, sqrtf(s.ay_g * s.ay_g + s.az_g * s.az_g));

        // Complementary filter
        if (first_attitude)
        {
            roll = roll_acc;
            pitch = pitch_acc;
            first_attitude = false;
        }
        else
        {
            roll = ALPHA_CF * (roll + s.gx_rads * DT) + (1.0f - ALPHA_CF) * roll_acc;
            pitch = ALPHA_CF * (pitch + s.gy_rads * DT) + (1.0f - ALPHA_CF) * pitch_acc;
        }

        // New: Debugging vertical acceleration
        float ax = s.ax_g * G0;
        float ay = s.ay_g * G0;
        float az = s.az_g * G0;

        update_gravity_estimate(
            ax,
            ay,
            az);

        float a_z_dyn_mss =
            vertical_dynamic_acceleration(
                ax,
                ay,
                az);

        // Estadísticas de la aceleración vertical
        if (a_z_dyn_mss < az_min)
            az_min = a_z_dyn_mss;

        if (a_z_dyn_mss > az_max)
            az_max = a_z_dyn_mss;

        az_sum += a_z_dyn_mss;
        az_sum2 += a_z_dyn_mss * a_z_dyn_mss;
        az_count++;

        // Store history for burst processing
        if (idx < BURST_SAMPLES)
        {
            heave[idx] = a_z_dyn_mss;
            roll_hist[idx] = roll;
            pitch_hist[idx] = pitch;
            idx++;
        }

        // When burst is full, process it
        if (idx >= BURST_SAMPLES)
        {
            float az_mean = az_sum / az_count;

            float az_std = sqrtf((az_sum2 / az_count) - (az_mean * az_mean));

            printf("\n");
            printf("=========== ANALISIS a_z_dyn_mss ===========\n");
            printf("Min      : %.4f m/s²\n", az_min);
            printf("Max      : %.4f m/s²\n", az_max);
            printf("Media    : %.4f m/s²\n", az_mean);
            printf("Std Dev  : %.4f m/s²\n", az_std);
            printf("Muestras : %d\n", az_count);
            printf("===========================================\n");

            printf("\nProcediendo a procesar la rafaga...\n");

            process_burst(heave, roll_hist, pitch_hist, BURST_SAMPLES);

            vTaskDelay(pdMS_TO_TICKS(10)); // Agregado

            printf("=============================================================\n");
            printf(" Esperando %d segundos para la proxima rafaga...\n",
                   WAIT_BETWEEN_BURSTS_SEC);
            printf("=============================================================\n");

            vTaskDelay(pdMS_TO_TICKS(
                WAIT_BETWEEN_BURSTS_SEC * 1000));

            // Reset for the next burst

            az_min = 1000.0f;
            az_max = -1000.0f;
            az_sum = 0.0;
            az_sum2 = 0.0;
            az_count = 0;

            idx = 0;
            memset(heave, 0, sizeof(heave));
            memset(roll_hist, 0, sizeof(roll_hist));
            memset(pitch_hist, 0, sizeof(pitch_hist));

            // Re-lock timing to avoid accumulated drift from processing time
            last_wake = xTaskGetTickCount();

            printf("\nProcediendo a recibir la rafaga...\n");
        }
    }
}
