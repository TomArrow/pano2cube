#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include "conversionCode.h"
#include <iostream>

using namespace cv;

const char* faceNames[6] =
{
	"ft","lf","bk",
	"rt","up","dn"
};

int main(int argc, char* argv[]) {

	if (argc < 4) {
		std::cout << "Need at least 3 arguments: Input file, side resolution, prefix[, rotation]";
		std::cin.get();
		return 1;
	}

	std::string filenameToLoad(argv[1]);
	int sideResolution = atoi(argv[2]);
	std::string prefix(argv[3]);
	float rotation = argc > 4 ? atof(argv[4]) : 0.0f;

	Mat img = imread(filenameToLoad,IMREAD_UNCHANGED);

	// Should we rotate?
	if (rotation != 0.0f) {

		// Normalize rotation
		while (rotation >= 360.0f) {
			rotation -= 360.0f;
		}
		while (rotation < 0.0f) {
			rotation += 360.0f;
		}
		int rotationCols = ((rotation / 360.0f) * (float)img.cols + 0.5f);
		if (rotationCols > 0 && rotationCols < img.cols) { // If nothing at all changes, why bother.
			Mat tmp = img.clone();
			img.colRange(0, img.cols - rotationCols).copyTo(tmp.colRange(rotationCols, img.cols));
			img.colRange(img.cols - rotationCols, img.cols).copyTo(tmp.colRange(0, rotationCols));
			img.release();
			img = tmp;
		}
	}

	for (int i = 0; i < 6; i++) {
		std::stringstream ss;
		ss << prefix;
		ss << "_";
		ss << faceNames[i];
		ss << ".hdr";
		Mat face(sideResolution, sideResolution, CV_32F);
		createCubeMapFace(img, face, i, sideResolution, sideResolution);
		imwrite(ss.str(), face);
	}

	
}