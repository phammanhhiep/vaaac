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
	res = std::min(width, height);
	halfRes = res / 2;
	// make the viewport a centered square
	int addX = 0, addY = 0;
	if (width > height) {
		addX = (width - height) / 2;
	} else if (height > width) {
		addY = (height - width) / 2;
	}
	// limit camera resolution ratio to 1:1
	frameBounds = cv::Rect(addX, addY, res, res);
	// determine reticle view area
	int reticleSize = 30;
	int reticlePos = halfRes - reticleSize / 2;
	reticleBounds = cv::Rect(reticlePos, reticlePos, reticleSize, reticleSize);
	// determine bfs sample size. this represents the size of
	// the scanned area for each node of the bfs algorithm.
	// the lower this value, the more accurate but the more noise.
	bfsSampleSize = 5;
	// fill up breadth first search array
	bfsOffsets.clear();
	for (int i = -1; i < 2; ++i) {
		for (int j = -1; j < 2; ++j) {
			bfsOffsets.push_back(std::make_pair<int, int>(i * bfsSampleSize,j * bfsSampleSize));
		}
	}
	// color conversion table used to change mask's white color to red 
	conversionTable = cv::Mat(1, 256, CV_8UC3);
	for (int i = 0; i < 255; ++i) {
		conversionTable.at<cv::Vec3b>(0, i) = cv::Vec3b(0, 0, 0);
	}
	conversionTable.at<cv::Vec3b>(0, 255) = cv::Vec3b(255, 0, 0);
}

vaaac::~vaaac() {
}

void vaaac::calibrateSkinTone() {
	int xCoord = width / 2 - 20;
	int yCoord = height / 2 - 20;
	int rectSizeX = std::min(40, halfRes);
	int rectSizeY = std::min(40, halfRes);
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
	cv::cvtColor(frame, mask, cv::COLOR_BGR2HSV);
	// binarization
	cv::inRange(mask, cv::Scalar(hLow, sLow, vLow), cv::Scalar(hHigh, sHigh, vHigh), mask);
	// noise reduction
	cv::Mat structuringElement = cv::getStructuringElement(cv::MORPH_ELLIPSE, {3, 3});
	cv::morphologyEx(mask, mask, cv::MORPH_OPEN, structuringElement);
	cv::dilate(mask, mask, cv::Mat(), {-1, -1}, 1);
	// find object union at center and clear rest of image
	int minX = reticleBounds.x;
	int minY = reticleBounds.y;
	int maxX = reticleBounds.x + reticleBounds.width;
	int maxY = reticleBounds.y + reticleBounds.height;
	int circleX = halfRes;
	int circleY = halfRes;
	int dist = 0;
	if (cv::mean(mask(reticleBounds))[0] > 0) {
		bool visited[res + 1][res + 1];
		memset(visited, 0, sizeof(visited));
		std::queue<std::pair<int, int>> q;
		for (auto& offset : bfsOffsets) {
			int x = halfRes + offset.first, y = halfRes + offset.second;
			q.push({x, y});
			visited[x][y];
		}
		while (!q.empty()) {
			std::pair<int, int> xy = q.front();
			int x = xy.first, y = xy.second;
			q.pop();
			if (visited[x][y] || x < 0 || y < 0 || x + bfsSampleSize > res || y + bfsSampleSize > res) continue; 
			visited[x][y] = true;
			if (cv::mean(mask(cv::Rect(x, y, bfsSampleSize, bfsSampleSize)))[0] == 0) continue;
			int oriDist = std::abs(halfRes - x) + std::abs(halfRes - y);
			if (oriDist > dist) {
				circleX = x;
				circleY = y;
				dist = oriDist;
			}
			minX = std::min(minX, x);
			minY = std::min(minY, y);
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);
			for (auto& offset : bfsOffsets) {
				q.push(std::make_pair<int, int>(x + offset.first, y + offset.second));
			}
		}
		// draw circle indicating furthermost point from origin
		cv::circle(mask, cv::Point(circleX, circleY), 5, cv::Scalar(255, 255, 255), 2);
		// draw object boundaries
		cv::rectangle(mask, cv::Rect(minX, minY, maxX - minX, maxY - minY), cv::Scalar(255, 255, 255), 5);
		// cut out everything out of the object boundaries inside the mask
		mask(cv::Rect(0, 0, minX, res)).setTo(cv::Scalar(0));
		mask(cv::Rect(minX, 0, res - minX, minY)).setTo(cv::Scalar(0));
		mask(cv::Rect(minX, maxY, res - minX, height - maxY)).setTo(cv::Scalar(0));
		mask(cv::Rect(maxX, minY, res - maxX, maxY - minY)).setTo(cv::Scalar(0));
	} else {
		// draw reticle area bounds
		cv::rectangle(mask, reticleBounds, cv::Scalar(255, 0, 0), 2);
	}
	// draw to screen
	cv::Mat out;
	cv::cvtColor(mask, mask, cv::COLOR_GRAY2RGB);
	//cv::LUT(mask, conversionTable, mask);
	cv::addWeighted(mask, 0.5, frame, 1.0, 0.0, out);	
	cv::imshow("out", out);

	// user input
	int key = cv::waitKey(1);
	if (key == 27) {
		ok = false;
		return;
	}
}
