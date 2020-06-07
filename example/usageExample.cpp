#include "vaaac.h"
#include "vaaac.h"

int main(int argc, char *argv[]) {
	vaaac *v = new vaaac();

	/*
	 * skin tone calibration is required
	 * if you don't hardcode the values
	 * yourself
	 */
	v->calibrateSkinTone();

	while (v->isOk()) {
		// new frame
		v->update();
	
		double angleX = v->getXAngle();
		double angleY = v->getYAngle();

		// if triggered, print arm angles
		if (v->isTriggered()) {
			std::cout << "triggered at:\n" << "x: " << angleX << ", y: " << angleY << std::endl << std::endl;
		}

		// user input
		int key = cv::waitKey(1);
		if (key == 27) {
			break;
		}
	}
	delete v;
	return 0;
}
