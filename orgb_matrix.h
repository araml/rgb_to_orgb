#pragma once

using uchar = unsigned char;

uchar *difference(uchar *in1, uchar *in2, int width, int height);

struct orgb_matrix {
    orgb_matrix(const char *path);
    ~orgb_matrix();
    void shift_tone(float rg_shift, float by_shift);
    uchar *to_rgb();
    void write_to_file(const char *path);

private:
    float *data{nullptr};
    int width{0}, height{0}, channels{0};
    /* By default they're zero so it doesn't shift colors when converting to
     * sRGB
     */
    float rg_shift{0};
    float by_shift{0};
};
