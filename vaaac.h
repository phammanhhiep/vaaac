/*
 * MIT License
 * Copyright (c) 2020 Pablo Peñarroja
 */

#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include <queue>

#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

enum state : int {
	VAAAC_FINDING_SKIN_TONE,
	VAAAC_WORKING
};

class vaaac {
	private:
		// computer vision vars
		cv::VideoCapture videoCapture;
		cv::Rect frameBounds;
		cv::Rect reticleBounds;
		cv::Rect dynamicSearchBounds;
		cv::Mat frame;

		// standard containers
		std::vector<std::pair<int, int>> bfsOffsets;

		// resolution (square)
		int width;
		int height;
		int minRes;

		// skin tone hsv color bounds
		int hLow;
		int hHigh;
		int sLow;
		int sHigh;
		int vLow;
		int vHigh;
	public:
		vaaac();
		~vaaac();

		bool ok;
		void calibrateSkinTone();
		void update();
};
