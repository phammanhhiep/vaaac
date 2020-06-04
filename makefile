CC = g++ -g
CCFLAGS = -o vaaac 

vaaac: usageExample.cpp vaaac.cpp
	$(CC) $(CCFLAGS) -I/usr/include/opencv4/ usageExample.cpp vaaac.cpp -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_imgproc -lopencv_objdetect

clean:
	rm vaaac

.PHONY: all vaaac clean
