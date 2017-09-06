#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <time.h>
#include "util.h"

#include <omp.h>
#include <queue>
#include <thread>
#include <mutex>

using namespace cv;
using std::string;
using std::cout;
using std::endl;


// Global Variables

typedef Point3_<uint8_t> Pixel;

const string video_directory = "C:\\Users\\a85360\\Documents\\Personal Workspace\\TimeDisplacement\\TimeDisplacement\\video\\";
const string temp_directory = video_directory + "tmp\\";

const string fileType = ".png";

std::mutex mu;

/*
Diffs two images in parallel.
Returns Mat with all values under threshold supressed.
--Holding onto this for the pixel access for later reference.
*/
Mat diffImg(Mat& a, Mat& b, const double threshold) {
	if (a.rows != b.rows || a.cols != b.cols)
		throw new std::exception("Error: Images different sizes..");

	Mat out;
	out.create(a.size(), a.type());

	// Parallel
	out.forEach<Pixel>([&](Pixel &p, const int * position) -> void {
		Pixel* ptr1 = a.ptr<Pixel>(position[0], position[1]);
		Pixel* ptr2 = b.ptr<Pixel>(position[0], position[1]);

		int diff = (ptr1->x - ptr2->x) * (ptr1->x - ptr2->x) +
			(ptr1->y - ptr2->y) * (ptr1->y - ptr2->y) +
			(ptr1->z - ptr2->z) * (ptr1->z - ptr2->z);

		if (diff < threshold) diff = 0;

		p.x = diff;
		p.y = diff;
		p.z = diff;
	});
	return out;
}

Mat getBuffer(const int frame) {
	return imread(temp_directory + std::to_string(frame) + fileType);
}

/* Creates a new window at given size with given name. */
void createWindow(const int w, const int h, const string name) {
	// Create a window
	namedWindow(name, WINDOW_NORMAL);
	resizeWindow(name, w, h);
}

/* Plays back a video from file, given a VideoCapture object. */
void playVideo(VideoCapture &cap) {
	// Create window
	int frame_width = (int)cap.get(CV_CAP_PROP_FRAME_WIDTH);
	int frame_height = (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	int frame_rate = (int)cap.get(CV_CAP_PROP_FPS);
	createWindow(frame_width, frame_height, "Video");

	Mat frame;
	bool playVideo = true;
	while (1) {
		// check if paused
		if (playVideo)
			cap >> frame;
		if (frame.empty())
			break;

		// display frame
		imshow("Video", frame);

		// delay for frame rate
		char c = (char)waitKey((int)(1000.0 / (float)frame_rate));
		// press ESC for exit
		if (c == 27)
			break;
		// press space for pause
		if (c == 32)
			playVideo = !playVideo;
	}


	waitKey(0);
	// cleanup
	destroyAllWindows();
}

/*
Writes bufferLength number of frames of a video to disk starting at bufferStart.
Returns true if successful.
*/
bool saveBufferFrames(VideoCapture &cap, int bufferStart, int bufferLength) {

	// check buffer does not go out of bounds
	if (bufferStart > (int)cap.get(CV_CAP_PROP_FRAME_COUNT) || bufferLength < 1 || bufferStart < 0) {
		return false;
	}
	else if (bufferLength + bufferStart > (int)cap.get(CV_CAP_PROP_FRAME_COUNT)) {
		bufferLength = (int)cap.get(CV_CAP_PROP_FRAME_COUNT) - bufferStart;
	}

	// move to frame
	cap.set(CV_CAP_PROP_POS_FRAMES, bufferStart);

	// save bufferLength number of frames to disk
	Mat frame;
	cap >> frame;
	for (int i = 0; i < bufferLength && !frame.empty(); i++) {
		const string name = std::to_string(bufferStart + i) + fileType;
		cv::imwrite(temp_directory + name, frame);
		cap >> frame;
	}
	return true;
}
Mat getFrameAt(VideoCapture &cap, const int frame) {
	// create new matrix
	Mat m;

	// check bounds
	if (frame < (int)cap.get(CV_CAP_PROP_FRAME_COUNT) && frame > 0) {
		// move to frame
		cap.set(CV_CAP_PROP_POS_FRAMES, frame);
		cap >> m;
	}

	// return matrix
	return m;
}


/* Combine a number of frames together into one output image. */
void combineFrames(VideoCapture &cap) {

	// useful variables
	int width = (int)cap.get(CV_CAP_PROP_FRAME_WIDTH);
	int height = (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	Size S = Size(width, height);
	int length = height;
	int frame_count = (int)cap.get(CV_CAP_PROP_FRAME_COUNT);

	// init output video
	int fourcc = CV_FOURCC('X', '2', '6', '4');
	int fps = (int)cap.get(CV_CAP_PROP_FPS);
	VideoWriter outVideo(video_directory + "Combined.avi", fourcc, fps, S);

	cout << "Starting read..." << endl;
	for (int x = 0; x < length; x++) {
		cap.set(CV_CAP_PROP_POS_FRAMES, 0);
		Mat img;
		cap >> img;

		for (int i = length - 1 - x; i < length; i++) {
			Mat frame;
			cap >> frame;
			frame.row(i).copyTo(img.row(i));
		}
		outVideo << img;
	}
	cout << "Intro Finished." << endl;
	for (int x = 0; x < frame_count - length; x++) {
		cap.set(CV_CAP_PROP_POS_FRAMES, x);
		Mat img;
		cap >> img;

		for (int i = 0; i < length; i++) {
			Mat frame;
			cap >> frame;
			frame.row(i).copyTo(img.row(i));
		}
		outVideo << img;
	}
	cout << "Body Finished." << endl;
	for (int x = 0; x < length; x++) {
		cap.set(CV_CAP_PROP_POS_FRAMES, frame_count - 1);
		Mat img;
		cap >> img;

		cap.set(CV_CAP_PROP_POS_FRAMES, frame_count - length - 1 + x);
		for (int i = 0; i < length - 1 - x; i++) {
			Mat frame;
			cap >> frame;
			frame.row(i).copyTo(img.row(i));
		}
		outVideo << img;
	}
	cout << "Outro Finished." << endl;
}

int main() {

	const string srcName = "Waves.mp4";

	// create temp folder
	//util::CreateTempDirectory(temp_directory);

	// declare capture object
	VideoCapture cap = VideoCapture(video_directory + srcName);
	if (!cap.isOpened()) {
		return -1;
	}

	cout << srcName << "  " << (int)cap.get(CV_CAP_PROP_FRAME_WIDTH) << "x" <<
		(int)cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;
	cout << "Frames: " << (int)cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;
	cout << "Length: " << (float)cap.get(CV_CAP_PROP_FRAME_COUNT) / (float)cap.get(CV_CAP_PROP_FPS) << " s\n" << endl;


	time_t time = clock(); // -------------- TIMING START
	int runs = 1;
	for (int i = 0; i < runs; i++) {
		combineFrames(cap);
		cout << "Run " << i << " completed." << endl;
	}

	time = clock() - time; // -------------- TIMING END
	cout << "\nTime: " << ((float)time / (CLOCKS_PER_SEC * runs)) << " s" << endl << endl;

	//VideoCapture combined = VideoCapture(video_directory + "Combined.mp4");
	//playVideo(combined);

	// cleanup
	cap.release();
	/*if (!util::RemoveTempDirectory(temp_directory)) {
		cout << "File removal error." << endl;
	};*/

	util::PressEnterToContinue();

	return 0;
}
