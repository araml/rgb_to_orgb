extern "C" {
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
}
#include "orgb_matrix.h"

#include "stb_image.h"
#include "stb_image_write.h"

#include <stdio.h>

void test_filter(void **data) {
    orgb_matrix m("image.png");
    uchar *rgb_to_orgb_image = m.to_rgb();
    // Assume for this test the image is RGB with no alpha
    int width, height, channels;
    uchar *original_img = stbi_load("image.png", &width, &height, &channels, 0);
    uchar *diff = difference(rgb_to_orgb_image, original_img, width, height);

    for (int i = 0; i < width * height * channels; i++) {
        assert_true(diff[i] == uchar(0));
    }

    return;
}

int main () {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_filter),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
