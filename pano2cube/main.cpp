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
	auto e = op.add<popl::Implicit<int>>("e", "equirect-from-cubemap", "Generate an equirectangular image from six cube faces (inverse mode). Optional: 0 for jk2 style cubmaps. 1 for vue 'Horizontal Front Main' cubemaps, 2 for jamme mme_saveCubemap 1 (vertical, not currently supported), 3 for jamme mme_saveCubemap 2 (horizontal). For single image sources, the full source image filename should be specified as prefix.",0);
	auto c = op.add<popl::Implicit<int>>("c", "cloud", "Generate an image for a cloud layer instead of six cube faces",512);
	auto n = op.add<popl::Switch>("n", "no-transform-cloud", "The resulting cloud image will not require tcMod transform, only tcMod scale");
	auto r = op.add<popl::Switch>("r", "reflection", "Turn equirectangular into a refletionmap for tcgen environment. Since it only encodes a half circle, you get 2 variants (one side will always be mirrored to the other).");
	op.parse(argcO, argvO);
	auto args = op.non_option_args();

	if (args.size() < 3) {
		std::cout << "Need at least 3 arguments: Input file, side resolution, prefix[, rotation]\n";
		std::cout << op.help();
		std::cin.get();
		return 1;
	}

	bool doReflection = r->is_set();
	bool doInverse = e->is_set();
	bool doInverseVueHorizontalFrontMain = e->is_set() && e->value() == 1;
	bool doInverseJamme1 = e->is_set() && e->value() == 2;
	bool doInverseJamme2 = e->is_set() && e->value() == 3;
	int cloudHeight = c->is_set() ? c->value() : 0;
	bool doCloudTransform = !n->is_set();

	if (doInverseJamme1) {
		std::cout << "Jamme's mme_saveCubemap 1 format is not currently supported as input.\n";
		std::cout << op.help();
		std::cin.get();
		return 1;
	}

	std::string filenameToLoad(args[0]);
	int sideResolution = atoi(args[1].c_str());
	std::string prefix(args[2]);
	float rotation = args.size() > 3 ? atof(args[3].c_str()) : 0.0f;

	if (doInverse) {
		Mat imgs[6];

		if (doInverseJamme2) {
			static int faceMult[6][2] = {
				{ 5,-1 },{ 4,-1},{ 3,-1},{ 2,-1 },{ 1,ROTATE_90_COUNTERCLOCKWISE },{ 0,ROTATE_90_CLOCKWISE }
			};
			Mat fullImage = imread(prefix, IMREAD_UNCHANGED);
			if (fullImage.empty()) {
				std::cout << "Unable to open specified source image: " << prefix;
				return 1;
			}
			int xMult = fullImage.cols / 6;
			for (int i = 0; i < 6; i++) {
				imgs[i] = fullImage(Range(0, fullImage.rows), Range(faceMult[i][0] * xMult, (faceMult[i][0] + 1) * xMult));
				if (faceMult[i][1] != -1) {
					Mat rot = imgs[i].clone();
					rotate(imgs[i], rot, faceMult[i][1]);
					imgs[i] = rot;
				}
			}
		}
		else if (doInverseVueHorizontalFrontMain) {
			bool vueMakesSense = false;

			static int faceMult[6][3] = {
				{ 1,1,-1 },{ 2,1,-1},{ 3,1,-1},{ 0,1,-1 },{ 1,0,ROTATE_90_COUNTERCLOCKWISE },{ 1,2,ROTATE_90_CLOCKWISE }
			};
			static int faceOffsets[6][4] = {
				{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{ 0,0,0,0}
			};
			Mat fullImage = imread(prefix, IMREAD_UNCHANGED);
			if (fullImage.empty()) {
				std::cout << "Unable to open specified source image: " << prefix;
				return 1;
			}
			int xMult = fullImage.cols / 4;
			int yMult = fullImage.rows / 3;

			if (!vueMakesSense) {
				const Vec3f unit = { 1,1,1 };
				// unfortunately vue does not appear to place cubemap boundaries where they should mathematically be, they can be off by multiple pixels. god knows why
				// there appears to be no real logic behind the actual boundaries so we will simply measure....... (CRINGE)
				int xBorders[2] = { 0,0 };
				int yBorders[2] = { 0,0 };
				int checkHeight = yMult / 2; // take half the height of a tile (if things made sense anyway) and go from left to right and vice versa to find the boundaries
				int checkWidth = xMult / 2;
				for (int x = 0; x < fullImage.cols; x++) {
					for (int y = 0; y < checkHeight; y++) {
						if (fullImage.at<Vec3f>(y,x).dot(unit) != 0.0f) {
							xBorders[0] = x;
							goto findBorder1;
						}
					}
				}
				findBorder1:
				for (int x = fullImage.cols-1; x >= 0; x--) {
					for (int y = 0; y < checkHeight; y++) {
						if (fullImage.at<Vec3f>(y, x).dot(unit) != 0.0f) {
							xBorders[1] = x+1;
							goto findBorder2;
						}
					}
				}
				findBorder2:
				for (int y = 0; y < fullImage.rows; y++) {
					for (int x = 0; x < checkWidth; x++) {
						if (fullImage.at<Vec3f>(y,x).dot(unit) != 0.0f) {
							yBorders[0] = y;
							goto findBorder3;
						}
					}
				}
				findBorder3:
				for (int y = fullImage.rows-1; y >= 0; y--) {
					for (int x = 0; x < checkWidth; x++) {
						if (fullImage.at<Vec3f>(y,x).dot(unit) != 0.0f) {
							yBorders[1] = y+1;
							goto bordersfound;
						}
					}
				}
				bordersfound:
				for (int i = 0; i < 6; i++) {
					if (faceMult[i][0] == 1) {
						faceOffsets[i][0] = xBorders[0] - xMult;
						faceOffsets[i][1] = xBorders[1] - xMult * 2;
					}
					if (faceMult[i][0] == 2) {
						faceOffsets[i][0] = xBorders[1] - xMult*2;
					}
					if (faceMult[i][1] == 1) {
						faceOffsets[i][2] = yBorders[0] - yMult;
						faceOffsets[i][3] = yBorders[1] - yMult * 2;
					}
					if (faceMult[i][1] == 2) {
						faceOffsets[i][2] = yBorders[1] - yMult*2;
					}
				}
			}

			for (int i = 0; i < 6; i++) {
				std::cout << "Measured corrective vue cubemap offsets for face " << i << ": " << faceOffsets[i][0] << " " << faceOffsets[i][1] << " " << faceOffsets[i][2] << " " << faceOffsets[i][3] << "\n";
				imgs[i] = fullImage(Range(faceMult[i][1] * yMult + faceOffsets[i][2], (faceMult[i][1] + 1) * yMult + faceOffsets[i][3]), Range(faceMult[i][0] * xMult + faceOffsets[i][0], (faceMult[i][0] + 1) * xMult + faceOffsets[i][1]));
				if (faceMult[i][2] != -1) {
					Mat rot = imgs[i].clone();
					rotate(imgs[i], rot, faceMult[i][2]);
					imgs[i] = rot;
				}
			}
		}
		else {
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