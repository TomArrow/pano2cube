#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include "conversionCode.h"
#include <iostream>
#include "include/popl.hpp"

using namespace cv;

const char* faceNames[6] =
{
	"ft","lf","bk",
	"rt","up","dn"
};

int main(int argcO, char** argvO) {


	popl::OptionParser op("Allowed options");
	auto c = op.add<popl::Implicit<int>>("c", "cloud", "Generate an image for a cloud layer instead of six cube faces",512);
	op.parse(argcO, argvO);
	auto args = op.non_option_args();

	if (args.size() < 3) {
		std::cout << "Need at least 3 arguments: Input file, side resolution, prefix[, rotation]";
		std::cin.get();
		return 1;
	}

	int cloudHeight = c->is_set() ? c->value() : 0;

	std::string filenameToLoad(args[0]);
	int sideResolution = atoi(args[1].c_str());
	std::string prefix(args[2]);
	float rotation = args.size() > 3 ? atof(args[3].c_str()) : 0.0f;

	Mat img = imread(filenameToLoad,IMREAD_UNCHANGED);

	if (img.empty()) {
		std::cout << "Unable to open specified source image.";
		return 1;
	}

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

	if (cloudHeight) {
		std::stringstream ss;
		ss << prefix;
		ss << "_cloud";
		ss << ".hdr";
		Mat face(sideResolution, sideResolution, CV_32F);
		createCloudMapFace(img, face, cloudHeight, sideResolution, sideResolution);
		imwrite(ss.str(), face);
		std::cout << "dont forget: \ntcMod scale 0.31830988618379067153776752674503 0.31830988618379067153776752674503 and cloud height " << cloudHeight << "\n";
	}
	else {
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


	
}