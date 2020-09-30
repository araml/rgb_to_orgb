// Scopic task.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdlib>
#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using uchar = unsigned char;

/*
struct rgb {
    uchar r;
    uchar g;
    uchar b;
};

struct matrix {
    matrix(uchar *in_data, int width, int height) : width(width), height(height) {
        data = new uchar[width * height * 3];
        memcpy(data, in_data, width * height * 3);
    }

    matrix(int width, int height) : width(width), height(height) {
        data = new uchar[width * height * 3];
    }

    ~matrix() { delete[] data; }

    rgb& at(int i, int j) {
        return (rgb &)&data[(i * width + j) * 3];
    }

    uchar *data;
    int width, height;
};*/

/* Notes:  
 * - Expects image with no alpha channel (channel width is hardcoded)
 */

float *convert_to_orgb(uchar *in_data, int width, int height, int channels) {
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
                                            ? (3.f/2.f) * theta
                                            : M_PI / 2 + (3.f / 4.f) * (theta - M_PI / 3);
            } else {
                theta0 = theta > -M_PI / 3.f 
                                            ? (3.f / 2.f) * theta
                                            : M_PI / 2 + (3.f / 4.f) * (theta - M_PI / 3);
            }
            /* Remember rotation matrix  
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

uchar *convert_to_srgb(float *in_data, int width, int height) {
    uchar *out_data = new uchar[width * height * 3];
   // memcpy(out_data, in_data, width * height * 3);

    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            float luma = in_data[(i * width + k) * 3]; 
            float c_yb = in_data[(i * width + k) * 3 + 1];
            float c_rg = in_data[(i * width + k) * 3 + 2];

            // we need to reverse the rotation
            float theta0 = atan2(c_rg, c_yb);
            float theta;
            if (theta0 > 0) {
                theta = theta0 < M_PI / 2 
                                            ? theta0 / (1.5f)
                                            : M_PI / 3.f + (4.f / 3.f) * (theta0 - M_PI / 2);
            } else {
                theta = theta0 > - M_PI / 2 
                                            ? theta0 / (1.5f)
                                            : M_PI / 3.f + (4.f / 3.f) * (theta0 - M_PI / 2);
            }

            float rotation_angle = theta0 - theta;
            
            /* Inverse rotation? */
            float c1 =   cos(rotation_angle) * c_yb + sin(rotation_angle) * c_rg;
            float c2 = - sin(rotation_angle) * c_yb + cos(rotation_angle) * c_rg;
           
            float r = 1.0000f * luma + 0.1140f * c1 + 0.7436f * c2;
            float g = 1.0000f * luma + 0.1140f * c1 - 0.4111f * c2;
            float b = 1.0000f * luma - 0.8860f * c1 + 0.1663f * c2;

            out_data[(i * width + k) * 3]     = uchar(r * 255.f);
            out_data[(i * width + k) * 3 + 1] = uchar(g * 255.f);
            out_data[(i * width + k) * 3 + 2] = uchar(b * 255.f);
        }
    }

    return out_data;
}

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

            out_data[(i * width + k) * 3] =
            out_data[(i * width + k) * 3 + 1] =
            out_data[(i * width + k) * 3 + 2] = v1;
        }
    }

    return out_data;
}


int main() {
    int width, height, channels;
    uchar *img = stbi_load("image.png", &width, &height, &channels, 0);
    std::cout << "Image size x:" << width << " y:" << height << " channels:" << channels << std::endl;

    /*
    for (int i = 0; i < width; i++) 
        std::cout << (int)img[(i * 4) + 0] << " " << (int)img[i * 4 + 1] << " " << (int)img[i * 4 + 2] << " " << (int)img[i * 4 + 3] << std::endl;
    */

    float *orgb_image = convert_to_orgb(img, width, height, channels);
    uchar *rgb_image = convert_to_srgb(orgb_image, width, height);

    uchar *diff = difference(img, rgb_image, width, height);

    stbi_write_png("image write from orgb.png", width, height, channels, rgb_image, width * channels);
    stbi_write_png("diff.png", width, height, channels, diff, width * channels);
    stbi_image_free(img);
    delete[] orgb_image;
    delete[] rgb_image;




    img = stbi_load("color test.png", &width, &height, &channels, 0);
    float *orgb_color = convert_to_orgb(img, width, height, channels);
    for (int i = 0; i < width; i++)
        std::cout << (int)img[i * 4 + 0] << " "
        << (int)img[i * 4 + 1] << " "
        << (int)img[i * 4 + 2] << std::endl;
        //<< (int)img[i * 4 + 3] << std::endl;
    std::cout << "AFTER" << std::endl;


    uchar *rgb_color_test = convert_to_srgb(orgb_color, width, height);
    for (int i = 0; i < width; i++)
        std::cout << (int)rgb_color_test[i * 3 + 0] << " " 
                  << (int)rgb_color_test[i * 3 + 1] << " " 
                  << (int)rgb_color_test[i * 3 + 2] << " " << std::endl;
   /* 
   
    */
}
