#pragma once

#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

// The function definition below is lifted from https://stackoverflow.com/a/34720686
// Changes by me: The function definition was changed a bit from inline and put into this header file
// Original code written by Emiswelt in 2016 and edited by Peter Mortensen in 2022
// Has CC BY SA License as described here: https://stackoverflow.com/legal/terms-of-service/public#licensing 
void createCubeMapFace(const Mat& in, Mat& face,
    int faceId = 0, const int width = -1,
    const int height = -1);

// these are further altered for stuff, see definitions.
void createCloudMapFace(const Mat& in, Mat& face,
    int cloudHeight, bool transform, const int width,
    const int height);

void createEquirectFromCubeFaces(const Mat in[6], Mat& out,
    const int width,
    const int height);

void createReflectionMapVariant(const Mat& in, Mat& face,
    int sign, const int width,
    const int height);
