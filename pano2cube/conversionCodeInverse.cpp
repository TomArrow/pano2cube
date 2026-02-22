#define _USE_MATH_DEFINES
#include <math.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <algorithm>
#include <iostream>

using namespace cv;

// A bunch of the below is lifted from https://stackoverflow.com/a/34720686 
// logic mostly adapted from https://github.com/PaulMakesStuff/Cubemaps-Equirectangular-DualFishEye



void createEquirectFromCubeFaces(const Mat in[6], Mat& out,
    const int width,
    const int height) {

    float inWidth[6] = { in[0].cols,in[1].cols,in[2].cols,in[3].cols,in[4].cols,in[5].cols };
    float inHeight[6] = { in[0].rows,in[1].rows,in[2].rows,in[3].rows,in[4].rows,in[5].rows };

    // Allocate map
    Mat mapx[6] = { Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F) };
    Mat mapy[6] = { Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F),Mat(height, width, CV_32F) };
    Mat mask[6] = { Mat(height, width, CV_8U, cv::Scalar(0)),Mat(height, width, CV_8U, cv::Scalar(0)),Mat(height, width, CV_8U, cv::Scalar(0)),Mat(height, width, CV_8U, cv::Scalar(0)),Mat(height, width, CV_8U, cv::Scalar(0)),Mat(height, width, CV_8U, cv::Scalar(0)) };


    // For each point in the target image,
    // calculate the corresponding source coordinates.
    for (int Y = 0; Y < height; Y++) {
        for (int X = 0; X < width; X++) {

            float sSrc = (float)X / (float)(width - 1);
            float tSrc = (float)Y / (float)(height - 1);

            float phi = (1.0f-tSrc) * M_PI;
            float theta = 2.0f*sSrc * M_PI;


            // this logic here is based on/copied/adapted from https://github.com/PaulMakesStuff/Cubemaps-Equirectangular-DualFishEye
            // all of these range between 0 and 1
            float x = cosf(theta) * sinf(phi);
            float y = sinf(theta) * sinf(phi);
            float z = cosf(phi);

            float a = std::max(std::max(fabsf(x), fabsf(y)), fabsf(z));

            // one of these will equal either - 1 or +1
            float xx = x / a;
            float yy = y / a;
            float zz = z / a;

            float xPixel, yPixel;
            int imageSelect;
            if (yy == -1) { // square 1 left
                imageSelect = 1;
                xPixel = (((-1.0f * tanf(atanf(x / y)) + 1.0) / 2.0));
                yPixel = (((-1.0f * tanf(atanf(z / y)) + 1.0) / 2.0));
            }
            else if (xx == 1) { // square 2; front
                imageSelect = 2;
                xPixel = (((tanf(atanf(y / x)) + 1.0) / 2.0));
                yPixel = (((tanf(atanf(z / x)) + 1.0) / 2.0));
            }
            else if (yy == 1) { // square 3; right
                imageSelect = 3;
                xPixel = (((-1.0f * tanf(atanf(x / y)) + 1.0) / 2.0) );
                yPixel = (((tanf(atanf(z / y)) + 1.0) / 2.0));
            }
            else if (xx == -1) { // square 4; back
                imageSelect = 0;
                xPixel = (((tanf(atanf(y / x)) + 1.0) / 2.0));
                yPixel = (((-1.0f * tanf(atanf(z / x)) + 1.0) / 2.0));
            }
            else if (zz == 1) { // square 5; bottom
                imageSelect = 5;
                yPixel = 1.0f-(((tanf(atanf(y / z)) + 1.0) / 2.0));
                xPixel = (((-1.0f * tanf(atanf(x / z)) + 1.0) / 2.0));
            }
            else if (zz == -1) { // square 6; top
                imageSelect = 4;
                yPixel = (((-1.0f * tanf(atanf(y / z)) + 1.0) / 2.0));
                xPixel = 1.0f - (((-1.0f * tanf(atanf(x / z)) + 1.0) / 2.0));
            }
            else {
                std::cerr << "wtf";
            }

            yPixel = std::clamp(yPixel,0.0f,1.0f) * (float)(in[imageSelect].rows-1);
            xPixel = std::clamp(xPixel,0.0f,1.0f) * (float)(in[imageSelect].cols-1);

            // Save the result for this pixel in map
            mapx[imageSelect].at<float>(Y, X) = xPixel;
            mapy[imageSelect].at<float>(Y, X) = yPixel;
            mask[imageSelect].at<uchar>(Y, X) = 1;
        }
    }

    // Recreate output image if it has wrong size or type.
    //if (out.cols != width || out.rows != height ||
    //    face.type() != in.type()) {
    //    face = Mat(width, height, in.type());
    //}

    //for (int i = 0; i < 6; i++) {
    for (int i = 5; i >= 0; i--) {
        // Do actual resampling using OpenCV's remap
        Mat tmp = Mat(out.rows, out.cols, out.type());
        remap(in[i], tmp, mapx[i], mapy[i],
            INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0));
        tmp.copyTo(out, mask[i]); // idk if bug in this opencv version but this shit just sets anything 0 in mask to black, so cant use for combining
        //for (int Y = 0; Y < height; Y++) {
        //    for (int X = 0; X < width; X++) {
        //        if (mask[i].at<uchar>(Y, X)) {
        //            out.at<float>(Y, X) = tmp.at<float>(Y, X);
         //       }
         //   }
        //}
    }
}