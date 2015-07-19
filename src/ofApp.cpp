#include "ofApp.h"
using namespace cv;
using namespace ofxCv;

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowPosition(+1921, 0);
    
    ofToggleFullscreen();
    ofSetFrameRate(12);
    
    player.loadMovie("eye.avi");
    player.play();
    player.setLoopState(OF_LOOP_NORMAL);
    
    thresh.allocate(1920/4, 1200/4,  OF_IMAGE_GRAYSCALE);
    orig.allocate(1920/4, 1200/4,  OF_IMAGE_GRAYSCALE);
    
    contourFinder.setMinAreaRadius(10);
    contourFinder.setMaxAreaRadius(200);
    
    drawWidth = 1920;
    drawHeight = 1200;
    
    flowWidth = drawWidth/4;
    flowHeight = drawHeight/4;
    
    // Flow & Mask
    opticalFlow.setup(flowWidth, flowHeight);
    velocityMask.setup(drawWidth, drawHeight);
    
    // Fluid
    fluid.setup(flowWidth, flowHeight, drawWidth, drawHeight, true);
    
    // Particles
    particleFlow.setup(flowWidth, flowHeight, drawWidth, drawHeight);
    
    // Visualisation
    displayScalar.allocate(flowWidth, flowHeight);
    velocityField.allocate(flowWidth / 4, flowHeight / 4);
    temperatureField.allocate(flowWidth / 4, flowHeight / 4);
    
    // Draw Forces
    numDrawForces = 6;
    flexDrawForces = new ftDrawForce[numDrawForces];
    flexDrawForces[0].setup(drawWidth, drawHeight, FT_DENSITY, true);
    flexDrawForces[0].setName("draw full res");
    flexDrawForces[1].setup(flowWidth, flowHeight, FT_VELOCITY, true);
    flexDrawForces[1].setName("draw flow res 1");
    flexDrawForces[2].setup(flowWidth, flowHeight, FT_TEMPERATURE, true);
    flexDrawForces[2].setName("draw flow res 2");
    flexDrawForces[3].setup(drawWidth, drawHeight, FT_DENSITY, false);
    flexDrawForces[3].setName("draw full res");
    flexDrawForces[4].setup(flowWidth, flowHeight, FT_VELOCITY, false);
    flexDrawForces[4].setName("draw flow res 1");
    flexDrawForces[5].setup(flowWidth, flowHeight, FT_TEMPERATURE, false);
    flexDrawForces[5].setName("draw flow res 2");
    
    // Camera
    didCamUpdate = false;
    cameraFbo.allocate(1920/4, 1200/4);
    cameraFbo.clear();
    
    setupGui();
    
    lastTime = ofGetElapsedTimef();
    lastMouse.set(0,0);
    
    
    font.loadFont("Akkurat-Mono.ttf", 14);
    font.setLetterSpacing(1.1);
    
    x = 0;
    toggleGuiDraw = false;
    
    fondo.loadImage("fondo.png");
    fondo.crop(0, 0, 200, 1080);
    
    
}

//--------------------------------------------------------------
void ofApp::update(){
   player.update();
   
    if(player.isFrameNew()){
        ofPixels pixels = player.getPixelsRef();
        img.setFromPixels(player.getPixelsRef());
        img.resize(1920/2, 1200/2);
       
        gray = img;
        inverted = gray;
        inverted.invert();
        thresh.setFromPixels(inverted.getPixelsRef());
        threshold(thresh, 230);
        
        contourFinder.findContours(thresh);
        
        
        thresh.update();
        didCamUpdate = true;
        
    }

}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofClear(0);
    
    if (didCamUpdate) {
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        
        cameraFbo.begin();
        thresh.draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
        cameraFbo.end();
        ofPopStyle();
        
        opticalFlow.setSource(cameraFbo.getTextureReference());
        opticalFlow.update(deltaTime);
        
        velocityMask.setDensity(cameraFbo.getTextureReference());
        velocityMask.setVelocity(opticalFlow.getOpticalFlow());
        velocityMask.update();
    }
    
    fluid.addVelocity(opticalFlow.getOpticalFlowDecay());
    fluid.addDensity(velocityMask.getColorMask());
    fluid.addTemperature(velocityMask.getLuminanceMask());
    
    for (int i=0; i<numDrawForces; i++) {
        flexDrawForces[i].update();
        if (flexDrawForces[i].didChange()) {
            // if a force is constant multiply by deltaTime
            float strength = flexDrawForces[i].getStrength();
            if (!flexDrawForces[i].getIsTemporary())
                strength *=deltaTime;
            switch (flexDrawForces[i].getType()) {
                case FT_DENSITY:
                    fluid.addDensity(flexDrawForces[i].getTextureReference(), strength);
                    break;
                case FT_VELOCITY:
                    fluid.addVelocity(flexDrawForces[i].getTextureReference(), strength);
                    particleFlow.addFlowVelocity(flexDrawForces[i].getTextureReference(), strength);
                    break;
                case FT_TEMPERATURE:
                    fluid.addTemperature(flexDrawForces[i].getTextureReference(), strength);
                    break;
                case FT_PRESSURE:
                    fluid.addPressure(flexDrawForces[i].getTextureReference(), strength);
                    break;
                case FT_OBSTACLE:
                    fluid.addTempObstacle(flexDrawForces[i].getTextureReference());
                default:
                    break;
            }
        }
    }
    
    fluid.update();
    
    int w = ofGetWindowWidth();
    int h = ofGetWindowHeight();
    ofClear(0,0);

    
    
    if (player.isLoaded())
    {
        
        ofPushMatrix();
//        ofScale(0.9, 0.9, 1);
//        ofTranslate(0.1 * w, 0.05 * h);
//        
        ofPushMatrix();
        ofTranslate(w, h);
        ofRotate(-180 );
            inverted.draw(w/2, h/2, w/2, h/2);
            gray.draw(w/2, 0, w/2, h/2);
            thresh.draw(0, 0, w/2, h/2);
        
        ofPushMatrix();
        ofPushStyle();
        ofScale(1, 0.9, 1);
        ofSetLineWidth(2);
        ofSetColor(255, 0, 0);
        contourFinder.draw();
        ofPopStyle();
        ofPopMatrix();
        
        ofPushMatrix();
        ofPushStyle();
        ofTranslate(w/2, 0);
        ofScale(1, 0.9, 1);
        ofSetLineWidth(2);
        ofSetColor(255, 0, 0);
        contourFinder.draw();
        ofPopStyle();
        ofPopMatrix();
        
        ofPushMatrix();
        ofPushStyle();
        ofTranslate(w/2, h/2);
        ofScale(1, 0.9, 1);
        ofSetLineWidth(2);
        ofSetColor(255, 0, 0);
        contourFinder.draw();
        ofPopStyle();
        ofPopMatrix();
        
        ofPushMatrix();
        ofPushStyle();
        ofTranslate(0, h/2);
        ofScale(1, 0.9, 1);
        ofSetLineWidth(2);
        ofSetColor(255, 0, 0);
        contourFinder.draw();
        ofPopStyle();
        ofPopMatrix();
        
        
        
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        velocityField.setSource(fluid.getVelocity());
        velocityField.draw(0, 0, w/2, h/2);
        
        
        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        
        displayScalar.setSource(fluid.getVelocity());
        
        displayScalar.draw(0, h/2,  w/2, h/2);
        ofPopMatrix();
        
        
        ofPopMatrix();
        
    }
    
    if(ofGetFrameNum() % 30 == 0)
        cout << ofGetFrameRate() << endl;
    
    if (toggleGuiDraw) {
        guiFPS = ofGetFrameRate();
        if (visualisationMode.get() >= numVisualisationModes)
            visualisationMode.set(numVisualisationModes-1);
        visualisationName.set(visualisationModeTitles[visualisationMode.get()]);
        gui.draw();
        ofCircle(ofGetMouseX(), ofGetMouseY(), ofGetWindowWidth() / 600.0);
    }
    
//    string msg;
//    float flow = opticalFlow.getAverageFlow();
//    msg = "Flow: " + ofToString(flow);
//    font.drawString(msg, 0, ofGetHeight() - 30);
//    
//    
//    x = (x + 1) % ofGetWidth();
//    
//    ofLine(x, ofGetHeight(), x,  ofGetHeight() - flow * 5000);

    ofEnableAlphaBlending();
    fondo.draw(0, 0);
    ofDisableAlphaBlending();
    
    
    ofSetColor(255);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}



//--------------------------------------------------------------
void ofApp::setupGui() {
    
    gui.setup("settings");
    gui.setDefaultBackgroundColor(ofColor(0, 0, 0, 127));
    gui.setDefaultFillColor(ofColor(160, 160, 160, 160));
    gui.add(guiFPS.set("FPS", 0, 0, 60));
    gui.add(doFullScreen.set("fullscreen (F)", false));
    doFullScreen.addListener(this, &ofApp::setFullScreen);
    gui.add(toggleGuiDraw.set("show gui (G)", false));
    gui.add(doFlipCamera.set("flip camera (C)", true));
    numVisualisationModes = 12;
    gui.add(visualisationMode.set("visualisation mode", 11, 0, numVisualisationModes - 1));
    gui.add(visualisationName.set("MODE", "draw name"));
    
    visualisationModeTitles = new string[numVisualisationModes];
    visualisationModeTitles[0] = "Source         (0)";
    visualisationModeTitles[1] = "Optical Flow   (1)";
    visualisationModeTitles[2] = "Flow Mask      (2)";
    visualisationModeTitles[3] = "Fluid Velocity (3)";
    visualisationModeTitles[4] = "Fluid Pressure (4)";
    visualisationModeTitles[5] = "Fld Temperature(5)";
    visualisationModeTitles[6] = "Fld Divergence (6)";
    visualisationModeTitles[7] = "Fluid Vorticity(7)";
    visualisationModeTitles[8] = "Fluid Buoyancy (8)";
    visualisationModeTitles[9] = "Fluid Obstacle (9)";
    visualisationModeTitles[10] = "Fluid Color    (-)";
    visualisationModeTitles[11] = "Fld Composite  (=)";
    
    int guiColorSwitch = 0;
    ofColor guiHeaderColor[2];
    guiHeaderColor[0].set(160, 160, 80, 200);
    guiHeaderColor[1].set(80, 160, 160, 200);
    ofColor guiFillColor[2];
    guiFillColor[0].set(160, 160, 80, 200);
    guiFillColor[1].set(80, 160, 160, 200);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(opticalFlow.parameters);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(velocityMask.parameters);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(fluid.parameters);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(particleFlow.parameters);
    
    visualisationParameters.setName("visualisation");
    visualisationParameters.add(showScalar.set("show scalar", true));
    visualisationParameters.add(showField.set("show field", true));
    visualisationParameters.add(displayScalarScale.set("display scalar scale", 0.15, 0.05, 0.5));
    displayScalarScale.addListener(this, &ofApp::setDisplayScalarScale);
    visualisationParameters.add(velocityFieldArrowScale.set("arrow scale", 0.1, 0.0, 0.5));
    velocityFieldArrowScale.addListener(this, &ofApp::setVelocityFieldArrowScale);
    visualisationParameters.add(temperatureFieldBarScale.set("temperature scale", 0.25, 0.05, 0.5));
    temperatureFieldBarScale.addListener(this, &ofApp::setTemperatureFieldBarScale);
    visualisationParameters.add(visualisationLineSmooth.set("line smooth", false));
    visualisationLineSmooth.addListener(this, &ofApp::setVisualisationLineSmooth);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(visualisationParameters);
    
    leftButtonParameters.setName("mouse left button");
    for (int i=0; i<3; i++) {
        leftButtonParameters.add(flexDrawForces[i].parameters);
    }
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(leftButtonParameters);
    
    rightButtonParameters.setName("mouse right button");
    for (int i=3; i<6; i++) {
        rightButtonParameters.add(flexDrawForces[i].parameters);
    }
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(rightButtonParameters);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(doResetDrawForces.set("reset draw forces (D)", false));
    doResetDrawForces.addListener(this,  &ofApp::resetDrawForces);
    
    gui.loadFromFile("settings.xml");
    gui.minimizeAll();
    
    toggleGuiDraw = true;
    
}
