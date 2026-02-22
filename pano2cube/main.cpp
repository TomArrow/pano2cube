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


// TODO do a tcgen environment kinda hack so we can do a projection that fits that? for suitable reflections for a given skybox?
// however i think it only encodes a half circle since the worldspace reflection ray vector[1] and [2] become the uv coords.
// aka 
// st[0] = 0.5 + reflected[1] * 0.5;
// st[1] = 0.5 - reflected[2] * 0.5;
int main(int argcO, char** argvO) {


	popl::OptionParser op("Allowed options");
	auto e = op.add<popl::Switch>("e", "equirect-from-cubemap", "Generate an equirectangular image from six cube faces (inverse mode)");
	auto c = op.add<popl::Implicit<int>>("c", "cloud", "Generate an image for a cloud layer instead of six cube faces",512);
	auto n = op.add<popl::Switch>("n", "no-transform-cloud", "The resulting cloud image will not require tcMod transform, only tcMod scale");
	auto r = op.add<popl::Switch>("r", "reflection", "Turn equirectangular into a refletionmap for tcgen environment. Since it only encodes a half circle, you get 2 variants (one side will always be mirrored to the other).");
	op.parse(argcO, argvO);
	auto args = op.non_option_args();

	if (args.size() < 3) {
		std::cout << "Need at least 3 arguments: Input file, side resolution, prefix[, rotation]";
		std::cout << op.help();
		std::cin.get();
		return 1;
	}

	bool doReflection = r->is_set();
	bool doInverse = e->is_set();
	int cloudHeight = c->is_set() ? c->value() : 0;
	bool doCloudTransform = !n->is_set();

	std::string filenameToLoad(args[0]);
	int sideResolution = atoi(args[1].c_str());
	std::string prefix(args[2]);
	float rotation = args.size() > 3 ? atof(args[3].c_str()) : 0.0f;

	if (doInverse) {
		Mat imgs[6];

		for (int i = 0; i < 6; i++) {
			std::stringstream ss;
			ss << prefix;
			ss << "_";
			ss << faceNames[i];
			ss << ".hdr";
			imgs[i] = imread(ss.str(), IMREAD_UNCHANGED);
			if (imgs[i].empty()) {
				std::cout << "Unable to open specified source image: " << ss.str();
				return 1;
			}
		}
		Mat output(sideResolution/2, sideResolution, CV_32F);
		createEquirectFromCubeFaces(imgs, output, sideResolution, sideResolution / 2);


		// Should we rotate?
		if (rotation != 0.0f) {
			rotation = -rotation;

			// Normalize rotation
			while (rotation >= 360.0f) {
				rotation -= 360.0f;
			}
			while (rotation < 0.0f) {
				rotation += 360.0f;
			}
			int rotationCols = ((rotation / 360.0f) * (float)output.cols + 0.5f);
			if (rotationCols > 0 && rotationCols < output.cols) { // If nothing at all changes, why bother.
				Mat tmp = output.clone();
				output.colRange(0, output.cols - rotationCols).copyTo(tmp.colRange(rotationCols, output.cols));
				output.colRange(output.cols - rotationCols, output.cols).copyTo(tmp.colRange(0, rotationCols));
				output.release();
				output = tmp;
			}
		}

		imwrite(filenameToLoad, output);
	}
	else {

		Mat img = imread(filenameToLoad, IMREAD_UNCHANGED);

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
		
		if (doReflection) {
			for (int i = 0; i < 2; i++) {
				std::stringstream ss;
				ss << prefix;
				ss << "_env";
				ss << (i+1);
				ss << ".hdr";
				Mat face(sideResolution, sideResolution, CV_32F);
				createReflectionMapVariant(img,face, i == 0 ? 1 : -1, sideResolution, sideResolution);
				imwrite(ss.str(), face);
			}
		}
		else if (cloudHeight) {
			std::stringstream ss;
			ss << prefix;
			ss << "_cloud";
			ss << ".hdr";
			Mat face(sideResolution, sideResolution, CV_32F);
			createCloudMapFace(img, face, cloudHeight, doCloudTransform, sideResolution, sideResolution);
			imwrite(ss.str(), face);
			if (doCloudTransform) {
				std::cout << "dont forget: \ntcMod transform 0.31830988618379067153776752674503 -0.31830988618379067153776752674503 0.31830988618379067153776752674503 0.31830988618379067153776752674503 -0.5 0.5 and cloud height " << cloudHeight << "\n";
			}
			else {
				std::cout << "dont forget: \ntcMod scale 0.31830988618379067153776752674503 0.31830988618379067153776752674503 and cloud height " << cloudHeight << "\n";
			}
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


	
}