// Scopic task.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <cstdlib>
#include <vector>
#include <iostream>

#include "orgb_matrix.h"


int main() {
    orgb_matrix m("image.png");
    m.write_to_file("image_copy.png");
}
