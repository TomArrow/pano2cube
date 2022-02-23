#pragma once

#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

void createCubeMapFace(const Mat& in, Mat& face,
    int faceId = 0, const int width = -1,
    const int height = -1);