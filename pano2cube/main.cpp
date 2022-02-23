#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include "conversionCode.h"
#include <iostream>

using namespace cv;

const char* faceNames[6] =
{
	"lf","bk","rt",
	"ft","up","dn"
};

int main(int argc, char* argv[]) {

	if (argc < 4) {
		std::cout << "Need at least 3 arguments: Input file, side resolution, prefix";
		std::cin.get();
		return 1;
	}

	std::string filenameToLoad(argv[1]);
	int sideResolution = atoi(argv[2]);
	std::string prefix(argv[3]);

	Mat img = imread(filenameToLoad,IMREAD_UNCHANGED);
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