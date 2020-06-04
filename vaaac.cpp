/*
 * MIT License
 * Copyright (c) 2020 Pablo Peñarroja
 */

#include <algorithm>

#include "vaaac.h"

vaaac::vaaac() : ok(false) {
	videoCapture = cv::VideoCapture(0);
	videoCapture.set(cv::CAP_PROP_SETTINGS, 1);
	if (!videoCapture.isOpened()) {
		return;
	}
	width = videoCapture.get(cv::CAP_PROP_FRAME_WIDTH);
	height = videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT);
	minRes = std::min(width, height);
	// make the viewport a centered square
	int addX = 0, addY = 0;
	if (width > height) {
		addX = (width - height) / 2;
	} else if (height > width) {
		addY = (height - width) / 2;
	}
	// limit camera resolution ratio to 1:1
	frameBounds = cv::Rect(addX, addY, minRes, minRes);
	// determine reticle view area
	int reticleSize = 5;
	int reticlePos = minRes / 2 - reticleSize / 2;
	reticleBounds = cv::Rect(reticlePos, reticlePos, reticleSize, reticleSize);
	// fill up breadth first search array
	bfsOffsets.clear();
	for (int i = -1; i < 2; ++i) {
		for (int j = -1; j < 2; ++j) {
			bfsOffsets.push_back(std::make_pair<int, int>(i * reticleSize, j * reticleSize));
		}
	}
	// determine dynamic object search range
	dynamicSearchBounds = cv::Rect(reticlePos, reticlePos, reticleSize / 2, reticleSize / 2);
}

vaaac::~vaaac() {
}

void vaaac::calibrateSkinTone() {
	int xCoord = width / 2 - 20;
	int yCoord = height / 2 - 20;
	int rectSizeX = std::min(40, minRes / 2);
	int rectSizeY = std::min(40, minRes / 2);
	cv::Rect area(xCoord, yCoord, rectSizeX, rectSizeY);
	for (;;) {
		videoCapture >> frame;
		cv::rectangle(frame, area, cv::Scalar(255, 0, 0));
		cv::imshow("skintone", frame);
		int key = cv::waitKey(1);
		if (key == 27) {
			break;
		}
	}
	cv::Mat hsv;
	cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
	cv::Mat sample(hsv, area);
	cv::Scalar mean = cv::mean(sample);
	int offsetLow = 50;
	int offsetHigh = 25;
	hLow = mean[0] - offsetLow;
	hHigh = mean[0] + offsetHigh;
	sLow = mean[1] - offsetLow;
	sHigh = mean[1] + offsetHigh;
	vLow = 0;
	vHigh = 255;
	ok = true;
}

void vaaac::update() {
	// get current frame
	videoCapture >> frame;
	// reshape
	frame = frame(frameBounds);
	// to hsv
	cv::cvtColor(frame, frame, cv::COLOR_BGR2HSV);
	// binarization
	cv::Mat skinMask;
	cv::inRange(frame, cv::Scalar(hLow, sLow, vLow), cv::Scalar(hHigh, sHigh, vHigh), skinMask);
	// noise reduction
	cv::Mat structuringElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, {3, 3});
	cv::morphologyEx(skinMask, skinMask, cv::MORPH_OPEN, structuringElement);
	cv::dilate(skinMask, skinMask, cv::Mat(), {-1, -1}, 1);
	// find object union at center and clear rest of image
	int minX = reticleBounds.x;
	int minY = reticleBounds.y;
	int maxX = reticleBounds.x + reticleBounds.width;
	int maxY = reticleBounds.y + reticleBounds.height;
	if (cv::mean(skinMask(reticleBounds))[0] > 0) {
		bool visited[minRes + 1][minRes + 1];
		memset(visited, 0, sizeof(visited));
		std::queue<std::pair<int, int>> q;
		for (auto& offset : bfsOffsets) {
			int x = reticleBounds.x + offset.first, y = reticleBounds.y + offset.second;
			q.push({x, y});
			visited[x][y];
		}
		int reticleSize = reticleBounds.width;
		while (!q.empty()) {
			std::pair<int, int> xy = q.front();
			int x = xy.first, y = xy.second;
			q.pop();
			if (visited[x][y] || x < 0 || y < 0 || x + reticleSize > minRes || y + reticleSize > minRes) continue; 
			visited[x][y] = true;
			if (cv::mean(skinMask(cv::Rect(x, y, reticleSize, reticleSize)))[0] == 0) continue;
			minX = std::min(minX, x);
			minY = std::min(minY, y);
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);
			for (auto& offset : bfsOffsets) {
				q.push(std::make_pair<int, int>(x + offset.first, y + offset.second));
			}
		}
		cv::rectangle(skinMask, cv::Rect(minX, minY, maxX - minX, maxY - minY), cv::Scalar(255, 0, 255), 5);
	}
	// draw aim point
	cv::rectangle(skinMask, reticleBounds, cv::Scalar(255, 0, 0), 2);
	// draw to screen
	cv::imshow("out", skinMask);

	// user input
	int key = cv::waitKey(1);
	if (key == 27) {
		ok = false;
		return;
	}
}
