CC = g++ -g
CCFLAGS = -o vaaac -I./

vaaac: example/usageExample.cpp vaaac.cpp
	$(CC) $(CCFLAGS) -I/usr/include/opencv4/ example/usageExample.cpp vaaac.cpp -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_imgproc -lopencv_objdetect

clean:
	rm vaaac

.PHONY: all vaaac clean
