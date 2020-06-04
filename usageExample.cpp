#include "vaaac.h"

int main(int argc, char *argv[]) {
	vaaac *v = new vaaac();
	v->calibrateSkinTone();
	while (v->ok) {
		v->update();
	}
	delete v;
	return 0;
}
