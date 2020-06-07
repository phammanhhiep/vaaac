/*
 * MIT License
 * Copyright (c) 2020 Pablo Peñarroja
 */

#include <algorithm>

#include "vaaac.h"

vaaac::vaaac() {
	// webcam initialization
	videoCapture = cv::VideoCapture(0);
	videoCapture.set(cv::CAP_PROP_SETTINGS, 1);
	// check if it's alright
	ok = videoCapture.isOpened();
	if (!ok) {
		ok = false;
		return;
	}
	// resolution (1:1 aspect ratio)
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
	int reticlePos = halfRes - RETICLE_SIZE / 2;
	reticleBounds = cv::Rect(reticlePos, reticlePos, RETICLE_SIZE, RETICLE_SIZE);
	// fill up bfs offsets array
	bfsOffsets.clear();
	for (int i = -1; i < 2; ++i) {
		for (int j = -1; j < 2; ++j) {
			bfsOffsets.push_back(std::make_pair<int, int>(i * BFS_SAMPLE_SIZE,j * BFS_SAMPLE_SIZE));
		}
	}
	// precompute trigger system constants
	TRIGGER_HALF_QUEUE_SIZE = TRIGGER_QUEUE_SIZE / 2;
	TRIGGER_MINIMUM_DISTANCE_PIXELS = TRIGGER_MINIMUM_DISTANCE * res / 100.0;
	TRIGGER_ALLOWED_Y_DEVIATION_PIXELS = TRIGGER_ALLOWED_Y_DEVIATION * res / 100.0;
	TRIGGER_ALLOWED_X_DEVIATION_PIXELS = TRIGGER_ALLOWED_X_DEVIATION * res / 100.0;
	// clear y variance deque
	yDelta.clear();
}

vaaac::~vaaac() {
}

void vaaac::calibrateSkinTone() {
	if (ok & 1) {
		int xCoord = res / 2 - SAMPLE_AREA_WIDTH / 2;
		int yCoord = res / 2 - SAMPLE_AREA_HEIGHT / 2;
		int rectSizeX = std::min(SAMPLE_AREA_WIDTH, halfRes);
		int rectSizeY = std::min(SAMPLE_AREA_HEIGHT, halfRes);
		cv::Rect area(xCoord, yCoord, rectSizeX, rectSizeY);
		for (;;) {
			videoCapture >> frame;
			frame = frame(frameBounds);
			if (RENDER_SAMPLE_TEXT) {
				cv::putText(
						frame, 
						"fill the area with your skin.", 
						cv::Point(10 , area.y - 60),
						cv::FONT_HERSHEY_DUPLEX,
						1.0,
						cv::Scalar(255, 255, 255),
						1);
				cv::putText(
						frame, 
						"then press any key.", 
						cv::Point(10 , area.y - 20),
						cv::FONT_HERSHEY_DUPLEX,
						1.0,
						cv::Scalar(255, 255, 255),
						1);
			}
			/*
			 * check if user is done before
			 * drawing rectangle because,
			 * otherwise, the rectangle gets
			 * computed as the skin tone mean
			 */
			int key = cv::waitKey(1);
			if (key != -1) {
				break;
			}
			// now draw rectangle and draw
			if (RENDER_TO_WINDOW) {
				cv::rectangle(frame, area, cv::Scalar(255, 255, 255), 2);
				cv::imshow("calibrateSkinTone", frame);
			}
		}
		if (RENDER_TO_WINDOW) {
			cv::destroyWindow("calibrateSkinTone");
		}
		cv::Mat hsv;
		cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
		cv::Mat sample(hsv, area);
		cv::Scalar mean = cv::mean(sample);
		hLow = mean[0] - MASK_LOW_TOLERANCE;
		hHigh = mean[0] + MASK_HIGH_TOLERANCE;
		sLow = mean[1] - MASK_LOW_TOLERANCE;
		sHigh = mean[1] + MASK_HIGH_TOLERANCE;
		vLow = 0;
		vHigh = 255;
		ok = 2;
	}
}

void vaaac::update() {

	//                                    //
	//-- i m a g e  p r o c e s s i n g --//
	//                                    //

	// always false before processing
	triggered = false;
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
	/*
	 * check existance of object within
	 * the reticle area's
	 */
	int xMin = reticleBounds.x;
	int yMin = reticleBounds.y;
	int xMax = reticleBounds.x + reticleBounds.width;
	int yMax = reticleBounds.y + reticleBounds.height;
	int xAim = halfRes;
	int yAim = halfRes;
	xAngle = -1.0;
	yAngle = -1.0;
	if (cv::mean(mask(reticleBounds))[0] > 0) {
		bool visited[res + 1][res + 1];
		memset(visited, 0, sizeof(visited));
		std::queue<std::pair<int, int>> q;
		for (int i = halfRes - RETICLE_SIZE / 2; i <= halfRes + RETICLE_SIZE / 2; i += BFS_SAMPLE_SIZE) {
			for (int j = halfRes - RETICLE_SIZE / 2; j <= halfRes + RETICLE_SIZE / 2; j += BFS_SAMPLE_SIZE) {
				for (auto& offset : bfsOffsets) {
					int x = i + offset.first, y = j + offset.second;
					if (visited[x][y]) {
						continue;
					}
					q.push({x, y});
					visited[x][y];
				}
			}
		}
		memset(visited, 0, sizeof(visited));
		for (; !q.empty(); ) {
			std::pair<int, int> xy = q.front();
			int x = xy.first, y = xy.second;
			q.pop();
			if (visited[x][y] || x < 0 || y < 0 || x + BFS_SAMPLE_SIZE > res || y + BFS_SAMPLE_SIZE > res) continue; 
			visited[x][y] = true;
			if (cv::mean(mask(cv::Rect(x, y, BFS_SAMPLE_SIZE, BFS_SAMPLE_SIZE)))[0] == 0) continue;
			/*
			 * update furthermost point coordinates.
			 * works because the bfs algorithm always
			 * visits the furthermost element in the
			 * last place
			 */
			xAim = x;
			yAim = y;
			// update found object area bounds
			xMin = std::min(xMin, x);
			yMin = std::min(yMin, y);
			xMax = std::max(xMax, x);
			yMax = std::max(yMax, y);
			// add neighbors to queue
			for (auto& offset : bfsOffsets) {
				q.push(std::make_pair<int, int>(x + offset.first, y + offset.second));
			}
		}
		// make angles
		yAngle = (double)(halfRes - yAim) / halfRes * 90.0;
		xAngle = (double)(halfRes - xAim) / halfRes * 90.0;
		/*
		 * check if there's a clear peak in
		 * the y variance
		 */
		if (yDelta.size() < TRIGGER_QUEUE_SIZE) {
			yDelta.push_back({yAngle, xAngle});
		} else {
			yDelta.pop_front();
			yDelta.push_back({yAngle, xAngle});
			bool ok = true;
			int peak = yDelta[TRIGGER_HALF_QUEUE_SIZE].first;
			std::pair<int, int> left(-100, -100);
			std::pair<int, int> right(-100, -100);
			for (int j = 0; j < TRIGGER_HALF_QUEUE_SIZE && ok; ++j) {
				std::pair<int, int> current = yDelta[j];
				if (peak - current.first >= TRIGGER_MINIMUM_DISTANCE_PIXELS) {
					left = current;
					break;
				}
				if (current.first > peak) {
					ok = false;
				}
			}
			for (int j = TRIGGER_HALF_QUEUE_SIZE + 1; j < TRIGGER_QUEUE_SIZE && ok; ++j) {
				std::pair<int, int> current = yDelta[j];
				if (peak - current.first >= TRIGGER_MINIMUM_DISTANCE_PIXELS) {
					right = current;
					break;
				}
				if (current.first > peak) {
					ok = false;
				}
			}
			if (ok && left.first != -100 && right.first != -100) {
				if (std::abs(left.first - right.first) <= TRIGGER_ALLOWED_Y_DEVIATION_PIXELS) {
					if (std::abs(left.second - right.second) <= TRIGGER_ALLOWED_X_DEVIATION_PIXELS) {
						triggered = true;
						/*
						 * clean y variance deque.
						 * otherwise false positives will appear
						 */
						yDelta.clear();
					}
				}
			}
		}
	} else {
		// clear y variance deque
		yDelta.clear();
	}

	//                                   //
	//-------- r e n d e r i n g --------//
	//                                   //

	if (RENDER_TO_FRAME) {
		/*
		 * cut out everything outside of the 
		 * object boundaries
		 */
		mask(cv::Rect(0, 0, xMin, res)).setTo(cv::Scalar(0));
		mask(cv::Rect(xMin, 0, res - xMin, yMin)).setTo(cv::Scalar(0));
		mask(cv::Rect(xMin, yMax, res - xMin, height - yMax)).setTo(cv::Scalar(0));
		mask(cv::Rect(xMax, yMin, res - xMax, yMax - yMin)).setTo(cv::Scalar(0));
		/*
		 * draw rectangle indicating either
		 * the reticle bounds or
		 * the found object boundaries'
		 */
		cv::rectangle(mask, cv::Rect(xMin, yMin, xMax - xMin, yMax - yMin), cv::Scalar(255, 255, 255), 2);
		/*
		 * draw circle indicating furthermost
		 * point from origin
		 */
		cv::circle(frame, cv::Point(xAim, yAim), 5, cv::Scalar(255, 0, 255), 2);
		// convert mask to three channel
		cv::cvtColor(mask, mask, cv::COLOR_GRAY2RGB);
		// mix frame with mask
		cv::addWeighted(mask, 0.5, frame, 1.0, 0.0, frame);
		// present final image
		if (RENDER_TO_WINDOW) {
			cv::imshow("update", frame);
		}
	}
}
