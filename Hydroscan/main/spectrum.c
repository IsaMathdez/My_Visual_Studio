
/*
 * ============================================================
 *                      HYDROSCAN
 * ------------------------------------------------------------
 * Archivo      : spectrum.c
 * Descripción  : Procesamiento espectral del oleaje
 * ============================================================
 */

#include "spectrum.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*==============================================================
                        CONFIGURACIÓN
==============================================================*/

static const char *TAG = "SPECTRUM";

/*
 * Frecuencias de interés para oleaje.
 * Se pueden modificar posteriormente.
 */

#define WAVE_MIN_HZ      0.12f
#define WAVE_MAX_HZ      1.00f

// Debug 
#define SPECTRUM_DEBUG   1

#if SPECTRUM_DEBUG
#define SPECTRUM_PRINTF(...) printf(__VA_ARGS__)
#else
#define SPECTRUM_PRINTF(...)
#endif


/*==============================================================
                VARIABLES DE DEBUG
==============================================================*/

static float debug_freq[256];
static float debug_peta[256];
static int debug_bins = 0;

/*==============================================================
                ELIMINAR MEDIA
==============================================================*/

static void remove_mean(float *x, int n)
{
    double sum = 0.0;

    for(int i=0;i<n;i++)
        sum += x[i];

    float mean = sum / n;

    for(int i=0;i<n;i++)
        x[i] -= mean;
}

/*==============================================================
                DETREND LINEAL
==============================================================*/

static void detrend_linear(float *x, int n)
{
    double sx = 0.0;
    double sy = 0.0;
    double sxx = 0.0;
    double sxy = 0.0;

    for(int i=0;i<n;i++)
    {
        sx += i;
        sy += x[i];
        sxx += (double)i*i;
        sxy += (double)i*x[i];
    }

    double den =
        n*sxx-sx*sx;

    if(fabs(den)<1e-12)
        return;

    double a =
        (n*sxy-sx*sy)/den;

    double b =
        (sy-a*sx)/n;

    for(int i=0;i<n;i++)
        x[i]-=(float)(a*i+b);
}

/*==============================================================
                MEDIA CUADRÁTICA
==============================================================*/

static float mean_square(
        const float *x,
        int n)
{
    double sum = 0.0;

    for(int i=0;i<n;i++)
        sum += (double)x[i]*x[i];

    return sum/n;
}

/*==============================================================
                DESVIACIÓN ESTÁNDAR
==============================================================*/

static float standard_deviation( // No se ha usado 
        const float *x,
        int n)
{
    double s = 0.0;
    double ss = 0.0;

    for(int i=0;i<n;i++)
    {
        s += x[i];
        ss += (double)x[i]*x[i];
    }

    double mean = s/n;

    double var =
        (ss/n)-(mean*mean);

    if(var<0.0)
        var=0.0;

    return sqrt(var);
}

/*==============================================================
            NUMERO DE SEGMENTOS WELCH
==============================================================*/

static int welch_segment_count(
        int samples,
        int segment_length,
        int step)
{
    if(samples < segment_length)
        return 0;

    int count = 0;

    for(int start = 0;
        start + segment_length <= samples;
        start += step)
    {
        count++;
    }

    return count;
}

/*==============================================================
            CÁLCULO DE MOMENTOS ESPECTRALES
==============================================================*/

static void compute_wave_moments(
        const float *acc,
        int samples,
        float fs,
        float *m0,
        float *m1,
        float *m2,
        float *fp)
{
    /*----------------------------------------------------------
                    PARAMETROS DE WELCH
    ----------------------------------------------------------*/

    const int segment_length = 256;

    const int overlap = segment_length / 2;

    const int step =
        segment_length - overlap;

    const int kmax =
        segment_length / 2;

    const float df =
        fs / segment_length;

    /*----------------------------------------------------------
                    LIMPIAR RESULTADOS
    ----------------------------------------------------------*/

    *m0 = 0.0f;
    *m1 = 0.0f;
    *m2 = 0.0f;
    *fp = 0.0f;

    debug_bins = 0;

    float peak_power = -1.0f;

    /*----------------------------------------------------------
                NUMERO DE SEGMENTOS WELCH
    ----------------------------------------------------------*/

    int segments =
        welch_segment_count(
            samples,
            segment_length,
            step);

    if(segments <= 0)
        return;

    /*----------------------------------------------------------
                PSD PROMEDIO
    ----------------------------------------------------------*/

    float Paa_average[129];

    memset(
        Paa_average,
        0,
        sizeof(Paa_average));
    
    /*----------------------------------------------------------
        RECORRER SEGMENTOS
    ----------------------------------------------------------*/

    for(int seg=0; seg<segments; seg++)
    {
        int start =
            seg * step;

        /*------------------------------------------
            ENERGIA DE LA VENTANA
        ------------------------------------------*/

        double U = 0.0;

        for(int n=0;n<segment_length;n++)
        {
            float w =
                0.5f *
                (1.0f -
                cosf(
                2.0f*M_PI*n/
                (segment_length-1)));

            U += w*w;
        }

        U /= segment_length;

        /*------------------------------------------
            DFT
        ------------------------------------------*/

        for(int k=0;k<=kmax;k++)
        {
            if((k%10)==0)
                vTaskDelay(1);

            double re = 0.0;
            double im = 0.0;

            for(int n=0;n<segment_length;n++)
            {
                float w =
                    0.5f *
                    (1.0f -
                    cosf(
                    2.0f*M_PI*n/
                    (segment_length-1)));

                float xn =
                    acc[start+n] * w;

                double ang =
                    -2.0 *
                    M_PI *
                    k *
                    n /
                    segment_length;

                re +=
                    xn *
                    cos(ang);

                im +=
                    xn *
                    sin(ang);
            }
        
            /*------------------------------------------
                PSD DEL SEGMENTO
            ------------------------------------------*/

            double mag2 =
                re*re + im*im;

            float Paa;

            if(k==0 || k==kmax)
            {
                Paa =
                    mag2 /
                    (segment_length *
                     segment_length *
                     U);
            }
            else
            {
                Paa =
                    2.0f *
                    mag2 /
                    (segment_length *
                     segment_length *
                     U);
            }

            Paa_average[k] += Paa;

        }   /* fin for(k) */

    }   /* fin for(seg) */

    /*----------------------------------------------------------
        PROMEDIAR PSD
    ----------------------------------------------------------*/

    for(int k=0;k<=kmax;k++)
    {
        Paa_average[k] /= segments;
    }

    /*----------------------------------------------------------
            CALCULAR MOMENTOS ESPECTRALES
    ----------------------------------------------------------*/

    for(int k=0;k<=kmax;k++)
    {
        float f = k * df;

        if(f < WAVE_MIN_HZ)
            continue;

        if(f > WAVE_MAX_HZ)
            continue;

        /*------------------------------------------
            PSD aceleración -> PSD elevación
        ------------------------------------------*/

        float omega =
            2.0f * M_PI * f;

        float omega2 =
            omega * omega;

        float Peta =
            Paa_average[k] /
            (omega2 * omega2 * df);

        /*------------------------------------------
                    DEBUG
        ------------------------------------------*/

        if(debug_bins < 256)
        {
            debug_freq[debug_bins] = f;
            debug_peta[debug_bins] = Peta;
            debug_bins++;
        }

        /*------------------------------------------
                MOMENTOS ESPECTRALES
        ------------------------------------------*/

        *m0 += Peta * df;
        *m1 += f * Peta * df;
        *m2 += f * f * Peta * df;

        /*------------------------------------------
                FRECUENCIA DE PICO
        ------------------------------------------*/

        if(Peta > peak_power)
        {
            peak_power = Peta;
            *fp = f;
        }
    }

#if SPECTRUM_DEBUG

    SPECTRUM_PRINTF("\n");
    SPECTRUM_PRINTF("========= WELCH =========\n");
    SPECTRUM_PRINTF("Segmentos     : %d\n", segments);
    SPECTRUM_PRINTF("Tam ventana   : %d\n", segment_length);
    SPECTRUM_PRINTF("Solape        : %d\n", overlap);
    SPECTRUM_PRINTF("df            : %.5f Hz\n", df);
    SPECTRUM_PRINTF("=========================\n");

    SPECTRUM_PRINTF("========= TOP PICOS ESPECTRALES =========\n");

    for(int i=0;i<debug_bins;i++)
    {
        SPECTRUM_PRINTF("%.3f Hz  -> %e\n",
                        debug_freq[i],
                        debug_peta[i]);
    }

    SPECTRUM_PRINTF("=========================================\n");

    SPECTRUM_PRINTF("m0    : %.8e\n", *m0);
    SPECTRUM_PRINTF("m1    : %.8e\n", *m1);
    SPECTRUM_PRINTF("m2    : %.8e\n", *m2);
    SPECTRUM_PRINTF("fp    : %.3f Hz\n", *fp);

#endif

}    

/*==============================================================
                PROCESAMIENTO ESPECTRAL
==============================================================*/

void spectrum_process(
        float *vertical_acc,
        float *roll,
        float *pitch,
        int samples,
        float fs,
        spectrum_result_t *result)
{
    remove_mean(vertical_acc,samples);

    detrend_linear(vertical_acc,samples);

    remove_mean(roll,samples);

    remove_mean(pitch,samples);

    compute_wave_moments(
            vertical_acc,
            samples,
            fs,
            &result->m0,
            &result->m1,
            &result->m2,
            &result->fp);

    result->Hs =
        4.0f*sqrtf(fmaxf(result->m0,0.0f));

    result->sigma =
        sqrtf(fmaxf(result->m0,0.0f));

    result->Tp =
        (result->fp>0.0f)?
        1.0f/result->fp:0.0f;

    result->Tm01 =
        (result->m1>0)?
        result->m0/result->m1:0.0f;

    result->Tm02 =
        (result->m2>0)?
        sqrtf(result->m0/result->m2):0.0f;

    float var_r =
        mean_square(roll,samples);

    float var_p =
        mean_square(pitch,samples);

    float cov = 0.0f;

    for(int i=0;i<samples;i++)
        cov += roll[i]*pitch[i];

    cov /= samples;

    float dir =
        0.5f*atan2f(
            2.0f*cov,
            var_r-var_p);

    result->direction =
        dir*180.0f/M_PI;

    if(result->direction<0)
        result->direction+=180.0f;


#if SPECTRUM_DEBUG

    //SPECTRUM_PRINTF("\n");

    SPECTRUM_PRINTF(
    "========= TOP PICOS ESPECTRALES =========\n");

    for(int i=0;i<debug_bins;i++)
    {
        SPECTRUM_PRINTF(
            "%.3f Hz  -> %e\n",
            debug_freq[i],
            debug_peta[i]);
    }

    SPECTRUM_PRINTF(
    "=========================================\n");

    SPECTRUM_PRINTF(
        "m0    : %.8e\n",
        result->m0);

    SPECTRUM_PRINTF(
        "m1    : %.8e\n",
        result->m1);

    SPECTRUM_PRINTF(
        "m2    : %.8e\n",
        result->m2);

    SPECTRUM_PRINTF(
        "Hs    : %.3f m\n",
        result->Hs);

    SPECTRUM_PRINTF(
        "Tp    : %.3f s\n",
        result->Tp);

    SPECTRUM_PRINTF(
        "Tm01  : %.3f s\n",
        result->Tm01);

    SPECTRUM_PRINTF(
        "Tm02  : %.3f s\n",
        result->Tm02);

    SPECTRUM_PRINTF(
        "fp    : %.3f Hz\n",
        result->fp);

#endif
}

// The end 