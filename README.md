# _vaaac_
**_V_**  _ery_  
**_A_** _wesome_  
**_A_** _rm_  
**_A_** _ngle_  
**_C_** _alculator_  

*__an opencv-based, header-only library capable of calculating the user's arm inclination angle and whether the user desires to trigger an action by doing a simple motion with their hand.__*

[**_this is what you can do with vaaac_**](https://www.youtube.com/watch?v=YiGEf9hP55E)

## dependencies
[OpenCV](https://opencv.org/).

## usage
the whole library is composed of a single header file, so a good starting point would be to include the header file
```cpp
#include "vaaac.hpp"
```
after that exhausting amount of work, i'd recommend drinking a refreshing beverage of your choice, and procceed to the next step; creating a vaaac object instance
```cpp
vaaac* v = new vaaac();
```
just to make sure that the object has been instantiated properly, we may check and let the user know. just in case something is off
```cpp
if (!v->isOk()) {
  std::cout << "bad" << std::endl;
  delete v;
  return 1;
} else {
  std::cout << "good" << std::endl;
}
```
now that we know the user's computer is not going to explode, we may procceed to the main loop, where we should update the vaaac object for every frame, check if the user's skin is detected (with the 'isDetected()' method), retrieve the current view angles, and check if the user has triggered an action, in that order
```cpp
while (v->isOk()) {
  v->update();
  
  // in case that the user's skin has been detected
  if (v->isDetected()) {
    // retrieve arm angles
    double xAxis = v->getXAngle();
    double yAxis = v->getYAngle();
    std::cout << "x: " << xAxis << " " << "y: " << yAxis << std::endl;
    
    // check if user has triggered an action
    if (v->isTriggered) {
      std::cout << "fire!" << std::endl;
    }
  }
}
```
yeah, so that's basically it. i'd recommend tunning the constants available at the top of the header file in order to fit you needs. they're explained in depth right there, just go ahead and take a look.
