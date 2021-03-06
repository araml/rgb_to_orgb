// std lib
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

// external libs
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// our libs
#include "orgb_matrix.h"

static float *convert_to_orgb(uchar *in_data, int width, int height, int channels) {
    float *out_data = new float[width * height * 3];
    //memcpy(out_data, in_data, width * height * 3);

    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            // we normalize dividing by 255 (also asume input is sRGB)
            float r = in_data[(i * width + k) * channels]     / 255.f;
            float g = in_data[(i * width + k) * channels + 1] / 255.f;
            float b = in_data[(i * width + k) * channels + 2] / 255.f;

            // Convert to [L', C1', C2']
            float luma = 0.2990f * r + 0.5870f * g + 0.1140f * b;
            float c1   = 0.5000f * r + 0.5000f * g - 1.0000f * b;
            float c2   = 0.8660f * r - 0.8660f * g + 0.0000f * b;

            float theta = atan2(c2, c1);
            float theta0 = 0;

            /*                      pi/2
             *                       |     atan2(y, x)
             *                       |
             *              compress | stretch
             *                       |
             *  -pi | pi    ------------------   0
             *                       |
             *              compress | stretch
             *                       |
             *                       |
             *                     -pi/2
             */

            if (theta > 0) {
                theta0 = theta < M_PI / 3.f
                             ? (3.f / 2.f) * theta
                             : M_PI / 2 + (3.f / 4.f) * (theta - M_PI / 3.f);
            } else {
                theta0 = theta > -M_PI / 3.f
                             ? (3.f / 2.f) * theta
                             : -M_PI / 2 + (3.f / 4.f) * (theta + M_PI / 3.f);
            }
            /*  rotation matrix
             * [ cos theta - sin theta] [c1]
             * [ sin theta   cos theta] [c2]
             */
            float rotation_angle = theta0 - theta;

            // rotate and convert to [Cyb, Crg]
            out_data[(i * width + k) * 3]     = luma;
            out_data[(i * width + k) * 3 + 1] = cos(rotation_angle) * c1 - sin(rotation_angle) * c2;
            out_data[(i * width + k) * 3 + 2] = sin(rotation_angle) * c1 + cos(rotation_angle) * c2;
        }
    }

    return out_data;
}

static uchar *convert_to_srgb(float *in_data, int width, int height,
                              float yb_shift = 0.f, float rg_shift = 0.f) {
    uchar *out_data = new uchar[width * height * 3];
   // memcpy(out_data, in_data, width * height * 3);

    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            float luma = in_data[(i * width + k) * 3];
            float c_yb = std::clamp(in_data[(i * width + k) * 3 + 1] + yb_shift, -1.f, 1.f);
            float c_rg = std::clamp(in_data[(i * width + k) * 3 + 2] + rg_shift, -1.f, 1.f);

            // we need to reverse the rotation
            float theta0 = atan2(c_rg, c_yb);
            float theta;
            if (theta0 > 0) {
                theta = theta0 < M_PI / 2
                            ? (2.f / 3.f) * theta0
                            : M_PI / 3.f + (4.f / 3.f) * (theta0 - M_PI / 2);
            } else {
                theta = theta0 > -M_PI / 2
                            ? (2.f / 3.f) * theta0
                            : -M_PI / 3.f + (4.f / 3.f) * (theta0 + M_PI / 2);
            }

            float rotation_angle = theta0 - theta;

            /* Inverse rotation? */
            /*  rotation matrix
             * [   cos theta  sin theta] [c_yb]
             * [ - sin theta  cos theta] [c_rg]
             */
            float c1 =   cos(rotation_angle) * c_yb + sin(rotation_angle) * c_rg;
            float c2 = - sin(rotation_angle) * c_yb + cos(rotation_angle) * c_rg;

            float r = 1.0000f * luma + 0.1140f * c1 + 0.7436f * c2;
            float g = 1.0000f * luma + 0.1140f * c1 - 0.4111f * c2;
            float b = 1.0000f * luma - 0.8860f * c1 + 0.1663f * c2;

            out_data[(i * width + k) * 3]     = uchar(r * 255.f + 0.5);
            out_data[(i * width + k) * 3 + 1] = uchar(g * 255.f + 0.5);
            out_data[(i * width + k) * 3 + 2] = uchar(b * 255.f + 0.5);
        }
    }

    return out_data;
}

/* Diff function to test the filter bijection
 */
uchar *difference(uchar *in1, uchar *in2, int width, int height) {
    uchar *out_data = new uchar[width * height * 3];

    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {

            uchar r1 = in1[(i * width + k) * 3];
            uchar g1 = in1[(i * width + k) * 3 + 1];
            uchar b1 = in1[(i * width + k) * 3 + 2];

            uchar r2 = in2[(i * width + k) * 3];
            uchar g2 = in2[(i * width + k) * 3 + 1];
            uchar b2 = in2[(i * width + k) * 3 + 2];

            short d1 = abs(r1 - r2);
            short d2 = abs(g1 - g2);
            short d3 = abs(b1 - b2);

            uchar v1 = 0;
            if (d1 > d2)
                v1 = d1 > d3 ? d1 : d3;
            else
                v1 = d2 > d3 ? d2 : d3;

            out_data[(i * width + k) * 3] = out_data[(i * width + k) * 3 + 1] =
                                            out_data[(i * width + k) * 3 + 2] = v1;
        }
    }

    return out_data;
}

orgb_matrix::orgb_matrix(const char *path) {
    uchar *img = stbi_load(path, &width, &height, &channels, 0);
    data = convert_to_orgb(img, width, height, channels);
    stbi_image_free(img);
}

orgb_matrix::~orgb_matrix() {
    delete[] data;
}

void orgb_matrix::shift_tone(float yb_shift, float rg_shift) {
    this->rg_shift = rg_shift;
    this->yb_shift = yb_shift;
}

uchar *orgb_matrix::to_rgb() {
    return convert_to_srgb(data, width, height, yb_shift, rg_shift);
}

void orgb_matrix::write_to_file(const char *path) {
    uchar *rgb_data = convert_to_srgb(data, width, height, yb_shift, rg_shift);
    stbi_write_png(path, width, height, channels, rgb_data, width * channels);
}

float orgb_matrix::yb_mean() const {
    float result = 0;
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            result += data[(i * width + k) * 3 + 1];
        }
    }
    return result / (width * height);
}

float orgb_matrix::rg_mean() const {
    float result = 0;
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            result += data[(i * width + k) * 3 + 2];
        }
    }
    return result / (width * height);
}


float orgb_matrix::yb_std_dev() const {
    float result = 0;
    float yb_m = yb_mean();
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            result += std::pow(yb_m - data[(i * width + k) * 3 + 1], 2);
        }
    }
    return sqrt(result / (width * height));
}

float orgb_matrix::rg_std_dev() const {
    float result = 0;
    float rg_m = rg_mean();
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            result += std::pow(rg_m - data[(i * width + k) * 3 + 2], 2);
        }
    }
    return sqrt(result / (width * height));
}
