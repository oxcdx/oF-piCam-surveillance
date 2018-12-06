#include "ofMain.h"
#include "ofApp.h"

int main()
{
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetupOpenGL(1280, 960, OF_WINDOW);
	ofRunApp( new ofApp());
}
