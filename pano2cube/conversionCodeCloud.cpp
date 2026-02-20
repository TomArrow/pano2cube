#define _USE_MATH_DEFINES
#include <math.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <algorithm>

using namespace cv;

// A bunch of the below is lifted from https://stackoverflow.com/a/34720686
// But changed to use the spherical cloud projection for the output as ingame for cloud layers



void createCloudMapFace(const Mat& in, Mat& face,
    int cloudHeight, const int width,
    const int height) {

    float inWidth = in.cols;
    float inHeight = in.rows;

    // Allocate map
    Mat mapx(height, width, CV_32F);
    Mat mapy(height, width, CV_32F);

    // Calculate adjacent (ak) and opposite (an) of the
    // triangle that is spanned from the sphere center
    //to our cube face.
    const float an = sin(M_PI / 4);
    const float ak = cos(M_PI / 4);

    //const float ftu = faceTransform[faceId][0];
    //const float ftv = faceTransform[faceId][1];

    float radiusWorld = 4096;
    float cloudRadius = radiusWorld + cloudHeight;

    // For each point in the target image,
    // calculate the corresponding source coordinates.
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            float sSrc = (float)x / (float)(width - 1);
            float tSrc = (float)y / (float)(height - 1);

            // assume we use tcmod scale 1/pi
            float sRad = sSrc * M_PI;
            float tRad = tSrc * M_PI;

            // create the vector that will point in the direction we want our value from
            float vector[3];
            vector[0] = cosf(sRad);
            vector[1] = cosf(tRad);
            vector[2] = sqrt(1.0 - vector[0] * vector[0] - vector[1] * vector[1]) * cloudRadius - radiusWorld;
            if (fpclassify(vector[2]) == FP_NAN) { // meh idk
                float somelen = sqrtf(vector[0] * vector[0] + vector[1] * vector[1]);
                vector[0] /= somelen;
                vector[1] /= somelen;
                vector[2] = sqrt(std::clamp(1.0f - vector[0] * vector[0] - vector[1] * vector[1],0.0f,1.0f)) * cloudRadius - radiusWorld;
                if (fpclassify(vector[2]) == FP_NAN) {
                    // huh
                    continue;
                }
            }

            vector[0] *= cloudRadius;
            vector[1] *= cloudRadius;

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
                    pitch = 0.5f*M_PI;
                }
                else {
                    pitch = 1.5f*M_PI;
                }
            }
            else {
                if (vector[0]) {
                    yaw = atan2(vector[1], vector[0]);
                }
                else if (vector[1] > 0) {
                    yaw = 0.5f*M_PI;
                }
                else {
                    yaw = 1.5f * M_PI;
                }
                if (yaw < 0) {
                    yaw += 2.0f*M_PI;
                }

                float forward = sqrtf(vector[0] * vector[0] + vector[1] * vector[1]);
                pitch = atan2(vector[2], forward);
                if (pitch < 0) {
                    pitch += 2.0f * M_PI;
                }
            }

            float u, v;

            u = yaw + M_PI; // turn it 180 to align it with the cubemap generatioon
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
            mapx.at<float>(x, y) = u;
            mapy.at<float>(x, y) = v;
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