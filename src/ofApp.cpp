#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

//--------------------------------------------------------------
void ofApp::setup() {
	
	// modes
	cycleMode = false;
	drawContours = true;

    // pi camera settings
    vW = 640;
	vH = 480;
    
    settings.sensorWidth = vW;
    settings.sensorHeight = vH;
    settings.framerate = 24;
    settings.enableTexture = true;
    settings.enablePixels = true;
    //settings.brightness = 50;
    
    videoGrabber.setup(settings);
    
    // for forcing 255 alpha
	allocate(alpha255, vW, vH, CV_8UC4);
	alpha255 = Scalar(0, 0, 0, 255);
	
	// contour settings
    alpha = 0.1;
    minArea = 10;
    maxArea = 500;
    thresholdValue = 10;
    drawScale = (float)ofGetWidth()/(float)vW;
    detected = false; 
}	

//--------------------------------------------------------------
void ofApp::update() {
	
	// timer
    elapsed = ofGetElapsedTimeMillis();
	
	// video input and CV operations	
	if(videoGrabber.isFrameNew()) {

		frameMat = toCv(videoGrabber.getPixels());

		convertColor(frameMat, greyMat, CV_RGB2GRAY);
		GaussianBlur(greyMat, 21);
		
		if(accumMat.empty()) {

			greyMat.convertTo(accumMat, CV_32F);			
		}
			
		accumulateWeighted(greyMat, accumMat, alpha);
		convertScaleAbs(accumMat, accumMatScaled);
		absdiff(greyMat, accumMatScaled, diffMat);
		
		threshold(diffMat, thresh, thresholdValue);
		dilate(thresh, 2);
		
		contourFinder.setMinAreaRadius(minArea);
		contourFinder.setMaxAreaRadius(maxArea);
		
		if (cycleMode == true) {
			
			// cycle through image processing stages as more contours detected
			if (contourFinder.size() == 0) {
				copy (frameMat, imgSwitch);
			}
			else if (contourFinder.size() == 1) {
				copy (greyMat, imgSwitch);
			}
			else if (contourFinder.size() == 2) {
				copy (accumMatScaled, imgSwitch);
			}
			else if (contourFinder.size() == 3) {
				copy (diffMat, imgSwitch);
			}
			else if (contourFinder.size() >= 4) {
				copy (thresh, imgSwitch);
			}
			
			contourFinder.findContours(imgSwitch);
		}
		else {
			
			contourFinder.findContours(thresh);
		}

		// loop over the contours
		for(int i = 0; i < contourFinder.size(); i++) {
			
			// see if there are contours above a certain size
			if (contourFinder.size() >= 1 && contourFinder.getContourArea(i) < 4000) {		
				detected = false; 				
			}
			else {			
				detected = true; 				
			}
			
			//std::cout << "contour: " << i << " size: " << contourFinder.getContourArea(i)  << endl;
			//info << "cF: " << contourFinder.getContourArea(i)  << endl;
		}					
	}
	
}


//--------------------------------------------------------------
void ofApp::draw(){
	
	// as alpha channel may be zero
	ofDisableAlphaBlending();	
	
	if (!diffMat.empty()) {
		
		if (cycleMode == true) {
			
			toOf(imgSwitch, backgroundOutputImage);
		}
		else {
			
			toOf(diffMat, backgroundOutputImage);
		}
		
		backgroundOutputImage.update();
		backgroundOutputImage.draw(0, 0, ofGetWidth(), ofGetHeight());
	}

	// drawing contours
	if (drawContours == true) {
		
		ofPushMatrix();
		ofPushStyle();
		
			ofScale(drawScale);
			ofSetLineWidth(1);
			contourFinder.draw();
			
			ofNoFill();
			
			// loop over the contours again to draw rotating rects
			for(int i = 0; i < contourFinder.size(); i++) {
				
				// smallest rectangle that fits the contour
				ofSetColor(255, 0, 0);
				ofPolyline minAreaRect = toOf(contourFinder.getMinAreaRect(i));
				minAreaRect.draw();
			}
		
		ofPopStyle();
		ofPopMatrix();
	}
    
    //std::cout << "elapsed: " << elapsed << endl;
	//std::cout << "lastCapture: " << lastCapture << endl;
	//std::cout << "detected: " << detected << endl;
	//std::cout << "capCounter: " << capCounter << endl;

    if (detected == true) {
		
		// work out time since last capture and have a minimum time interval before next capture
		if (elapsed - lastCapture > 20000) {
			
			capCounter += 1;
		
			if (capCounter >= 18){
				
				imCntr++;
					
				// do the thing
				
				std::string frameName = std::to_string(ofGetMinutes()) + "_" + std::to_string(ofGetHours()) + "_" + std::to_string(ofGetDay()) + "_" + std::to_string(ofGetMonth()) + "_" + std::to_string(ofGetYear()) + "_" + std::to_string(imCntr) + ".jpg";
				
				std::string frameDir = "/home/pi/openFrameworks/apps/myApps/ocd-ofxCv-cts/bin/frame/" + frameName;
				std::string diffDir = "/home/pi/openFrameworks/apps/myApps/ocd-ofxCv-cts/bin/diff/" + frameName;
				
				imwrite(frameDir, frameMat);
				imwrite(diffDir, diffMat);
				
				std::cout << "[UPLOAD]" << endl;

				std::string uploadDropbox = "/home/pi/Dropbox-Uploader/dropbox_uploader.sh upload /home/pi/openFrameworks/apps/myApps/ocd-ofxCv-cts/bin/frame/" + frameName + " /ofx/" + frameName;
				std::string uploadDropbox2 = "/home/pi/Dropbox-Uploader/dropbox_uploader.sh upload /home/pi/openFrameworks/apps/myApps/ocd-ofxCv-cts/bin/diff/" + frameName + " /diff/" + frameName;
				system(uploadDropbox.c_str());
				system(uploadDropbox2.c_str());
				
				if (cycleMode == true) {

					ofPushMatrix();
					ofPushStyle();
					
						ofScale(drawScale);
						ofSetLineWidth(2);
						contourFinder.draw();
						
					ofPopStyle();
					ofPopMatrix();
				}
				
				capCounter = 19;
				
			}		
		}
		
		// time for capture event to take place (should be longer than minimum interval)
		if (elapsed - lastCapture > 40000 && capCounter == 19) {
			
			capCounter = 0;
			lastCapture = elapsed;
		}
	}
	else {
		capCounter = 0;
		
	}
	
	// draw framerate
	//stringstream info;
	//info << "APP FPS: " << ofGetFrameRate() << endl;
	//info << endl;
    //ofDrawBitmapStringHighlight(info.str(), 100, 100, ofColor(ofColor::black, 90), ofColor::yellow);

}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key) {

}

