// Scopic task.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdlib>
#include <vector>
#include <iostream>
#include <iostream>
#include <cstring>

#include "stb_image.h"
#include "stb_image_write.h"

#include "orgb_matrix.h"

void write_mosaic_row(uchar *composed, uchar *mosaic[3], int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int k = 0; k < 3; k++) {
        memcpy(composed + (i * width * 3 * 3) + k * width * 3, mosaic[k] + i * width * 3, width * 3);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "error wrong number of args" << std::endl;
        std::cout << "usage ./image [image_name]" << std::endl;
        return -1;
    }
    orgb_matrix m(argv[1]);

    float shift_value = 0.05;
    int width = m.width;
    int height = m.height;
    uchar *composed = new uchar[width * height * 3 * 9];

    int idx = 0;
    /* kinda ugly */
    uchar *mosaic[9] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                        nullptr, nullptr, nullptr};

    /* Generate images */
    for (int i = -1; i <= 1; i++) {
        for (int k = -1; k <= 1; k++) {
            m.shift_tone(shift_value * i, shift_value * k);
            uchar *s = m.to_rgb();
            mosaic[idx] = s;
            idx++;
        }
    }

    for (int i = 0; i < 3; i++) {
        write_mosaic_row(composed + (width * height * 3 * 3 * i), mosaic + i * 3, width, height);
    }

    stbi_write_png("composed.png", width * 3, height * 3, 3,
                   composed, width * 3 * 3);

    for (int i = 0; i < 9; i++) {
        delete[] mosaic[i];
    }
}
