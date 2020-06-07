/*
 * MIT License
 * Copyright (c) 2020 Pablo Peñarroja
 */

/*
 * the following constants should be
 * altered in order to fit the user's
 * needs.
 * they're set to standard values
 * by default, and changing them may
 * dramatically alter the library's
 * behaviour
 */

//                                   //
//-------- c o n s t a n t s --------//
//                                   //

/*
 * this indicates whether to render
 * the processed image to a matrix 
 * 'frame'.
 * this matrix can be retrieved using
 * the 'getFrame()' method declared
 * bellow
 */
const bool RENDER_TO_FRAME = true;

/*
 * this indicates whether to create a
 * separate window and render the 
 * current frame to that window.
 * it requires the 'RENDER_TO_FRAME'
 * constant to be true
 */
const bool RENDER_TO_WINDOW = true;

/*
 * this indicates whether to render
 * a small text explaining to the user
 * what to do
 */
const bool RENDER_SAMPLE_TEXT = true;

/*
 * width and height of the skin color
 * sampling rectangle
 */
const int SAMPLE_AREA_WIDTH = 40;
const int SAMPLE_AREA_HEIGHT = 40;

/*
 * the bfs sample size represents the 
 * square root of the area to scan for
 * each node of the bfs algorithm.
 * the lower this value, the more 
 * accurate but the more noise, and the
 * more cpu usage required as well
 */
const int BFS_SAMPLE_SIZE = 4;

/*
 * width and height of the reticle:
 * the area centered at the middle of
 * the screen where the program will
 * look for the user's skin.
 * in case of finding it, the object
 * union find algorithm will start.
 */
const int RETICLE_SIZE = 40;

/*
 * this is the tolerance applied to 
 * the lower and higher bounds of the
 * HSV skin tone color sampled from
 * the user.
 * low values limit the range, thus
 * 
 */
const int MASK_LOW_TOLERANCE = 50;
const int MASK_HIGH_TOLERANCE = 25;

/*
 * the variance of the y component of
 * the angle for the last k frames is
 * stored in the 'yDelta' double-ended
 * queue.
 * this deque is processed every frame
 * trying to find a peak shape in it.
 * if a peak is found, then, it's
 * understood that an action has been
 * triggered.
 * as a consequence of this, the 
 * boolean value 'triggered' is set 
 * true for that frame.
 * this value has to be greater than
 * two
 */
const int TRIGGER_QUEUE_SIZE = 15;

/*
 * this is the minumum distance that
 * the aim point should travel upwards
 * in order to be considered a trigger
 * action.
 * it's represented as a percentage
 * of the screen real estate
 */
const double TRIGGER_MINIMUM_DISTANCE = 3.5;

/*
 * consider y1 as the origin y value
 * for a trigger action, and y2 as the
 * final y value, then:
 * this value represents the absolute
 * maximum allowed deviation from y1
 * to y2 in order for the action to
 * be valid.
 * it's represented as a percentage of
 * the screen real estate
 */
const double TRIGGER_ALLOWED_Y_DEVIATION = 6.0;

/*
 * same as 'TRIGGER_ALLOWED_Y_DEVIATION'
 * but for the x-axis
 */
const double TRIGGER_ALLOWED_X_DEVIATION = 3.0;

//                                 //
//--------  v  a  a  a  c  --------//
//                                 //

#pragma once

#include <vector>
#include <utility>
#include <queue>
#include <deque>

#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

class vaaac {
	private:
		// is everything working
		int ok;

		// resolution (square)
		int width;
		int height;
		int res;
		int halfRes;

		// computer vision vars
		cv::VideoCapture videoCapture;
		cv::Rect frameBounds;
		cv::Rect reticleBounds;
		cv::Mat frame;
		cv::Mat mask;

		// skin tone hsv color bounds
		int hLow;
		int hHigh;
		int sLow;
		int sHigh;
		int vLow;
		int vHigh;

		// breadth first search system
		std::vector<std::pair<int, int>> bfsOffsets;
		
		// aim point location
		double xAngle;
		double yAngle;

		// trigger system
		bool triggered;
		int TRIGGER_HALF_QUEUE_SIZE;
		int TRIGGER_MINIMUM_DISTANCE_PIXELS;
		int TRIGGER_ALLOWED_Y_DEVIATION_PIXELS;
		int TRIGGER_ALLOWED_X_DEVIATION_PIXELS;
		std::deque<std::pair<int, int>> yDelta;

	public:
		vaaac();
		~vaaac();

		inline bool isOk() {
			return ok == 2;
		}

		inline bool isTriggered() {
			return triggered;
		}

		inline double getXAngle() {
			return xAngle;
		}

		inline double getYAngle() {
			return yAngle;
		}

		inline cv::Mat getFrame() {
			return frame;
		}

		void calibrateSkinTone();
		void update();
};
