#define _USE_MATH_DEFINES
#include <math.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <algorithm>

using namespace cv;

// A bunch of the below is lifted from https://stackoverflow.com/a/34720686
// But changed to use tcgen environment style mapping


// sign distinguishes which side of the half circle we should use
void createReflectionMapVariant(const Mat& in, Mat& face,
    int sign, const int width,
    const int height) {

    float inWidth = in.cols;
    float inHeight = in.rows;

    // Allocate map
    Mat mapx(height, width, CV_32F);
    Mat mapy(height, width, CV_32F);

    // For each point in the target image,
    // calculate the corresponding source coordinates.
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            float sSrc = (float)x / (float)(width - 1);
            float tSrc = (float)y / (float)(height - 1);

            // create the vector that will point in the direction we want our value from
            // note that 
            float vector[3];
            vector[1] = 2.0f*sSrc-1.0f;
            vector[2] = 1.0f-2.0f*tSrc;
            vector[0] = sign*sqrt(1.0 - vector[2] * vector[2] - vector[1] * vector[1]);
            if (fpclassify(vector[0]) == FP_NAN) { // meh idk
                float somelen = sqrtf(vector[1] * vector[1] + vector[2] * vector[2]);
                vector[1] /= somelen;
                vector[2] /= somelen;
                vector[0] = sign*sqrt(std::clamp(1.0f - vector[2] * vector[2] - vector[1] * vector[1], 0.0f, 1.0f));
                if (fpclassify(vector[0]) == FP_NAN) {
                    // huh
                    continue;
                }
            }

            // normalize it
            float veclen = sqrtf(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
            if (veclen) {
                vector[0] /= veclen;
                vector[1] /= veclen;
                vector[2] /= veclen;
            }

            float pitch, yaw;
            if (!vector[1] && !vector[0])
            {
                yaw = 0;
                if (vector[2] > 0) {
                    pitch = 0.5f * M_PI;
                }
                else {
                    pitch = 1.5f * M_PI;
                }
            }
            else {
                if (vector[0]) {
                    yaw = atan2(vector[1], vector[0]);
                }
                else if (vector[1] > 0) {
                    yaw = 0.5f * M_PI;
                }
                else {
                    yaw = 1.5f * M_PI;
                }
                if (yaw < 0) {
                    yaw += 2.0f * M_PI;
                }

                float forward = sqrtf(vector[0] * vector[0] + vector[1] * vector[1]);
                pitch = atan2(vector[2], forward);
                if (pitch < 0) {
                    pitch += 2.0f * M_PI;
                }
            }

            float u, v;

            u = yaw;
            v = -pitch;

            // Map from angular coordinates to [-1, 1], respectively.
            u = u / (M_PI);
            v = v / (M_PI / 2);

            // Warp around, if our coordinates are out of bounds.
            while (v < -1) {
                v += 2;
                u += 1;
            }
            while (v > 1) {
                v -= 2;
                u += 1;
            }

            while (u < -1) {
                u += 2;
            }
            while (u > 1) {
                u -= 2;
            }

            // Map from [-1, 1] to in texture space
            u = u / 2.0f + 0.5f;
            v = v / 2.0f + 0.5f;

            u = u * (inWidth - 1);
            v = v * (inHeight - 1);

            if (fpclassify(u) == FP_NAN || fpclassify(v) == FP_NAN) {
                continue;
            }

            // Save the result for this pixel in map
            mapx.at<float>(y, x) = u;
            mapy.at<float>(y, x) = v;
        }
    }

    // Recreate output image if it has wrong size or type.
    if (face.cols != width || face.rows != height ||
        face.type() != in.type()) {
        face = Mat(width, height, in.type());
    }

    // Do actual resampling using OpenCV's remap
    remap(in, face, mapx, mapy,
        INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));
}