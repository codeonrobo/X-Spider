import os
import cv2
from base_camera import BaseCamera
import argparse
import numpy as np
import datetime
import time
import threading
import imutils

curpath = os.path.realpath(__file__)
thisPath = "/" + os.path.dirname(curpath)

faceCascade = cv2.CascadeClassifier(thisPath + '/haarcascade_frontalface_default.xml')
print(thisPath + '/haarcascade_frontalface_default.xml')

class CVThread(threading.Thread):
    font = cv2.FONT_HERSHEY_SIMPLEX

    def __init__(self, *args, **kwargs):
        super(CVThread, self).__init__(*args, **kwargs)
        self.__flag = threading.Event()
        self.__flag.clear()

        self.CVThreading = 0
        self.WDdrawing = 0
        self.FDdrawing = 0

        self.avg = None
        self.motionCounter = 0
        self.lastMovtionCaptured = datetime.datetime.now()
        self.frameDelta = None
        self.thresh = None
        self.cnts = None
        self.imgCV =None

        self.faces = None

    def mode(self, imgInput):
        self.imgCV = imgInput
        self.resume()

    def elementDraw(self,imgInput):
        if self.WDdrawing:
            cv2.rectangle(imgInput, (self.mov_x, self.mov_y), (self.mov_x + self.mov_w, self.mov_y + self.mov_h), (128, 255, 0), 1)
        try:
            for (x,y,w,h) in self.faces:
                cv2.rectangle(imgInput,(x,y),(x+w,y+h),(255,0,0),2)
        except:
            pass
        return imgInput


    def watchDog(self, imgInput):
        # print("watchDog running")
        timestamp = datetime.datetime.now()
        gray = cv2.cvtColor(imgInput, cv2.COLOR_BGR2GRAY)
        gray = cv2.GaussianBlur(gray, (21, 21), 0)

        if self.avg is None:
            print("[INFO] starting background model...")
            self.avg = gray.copy().astype("float")
            return 'background model'

        cv2.accumulateWeighted(gray, self.avg, 0.5)
        self.frameDelta = cv2.absdiff(gray, cv2.convertScaleAbs(self.avg))

        self.thresh = cv2.threshold(self.frameDelta, 5, 255,
            cv2.THRESH_BINARY)[1]
        self.thresh = cv2.dilate(self.thresh, None, iterations=2)
        self.cnts = cv2.findContours(self.thresh.copy(), cv2.RETR_EXTERNAL,
            cv2.CHAIN_APPROX_SIMPLE)
        self.cnts = imutils.grab_contours(self.cnts)

        for c in self.cnts:
            # if the contour is too small, ignore it
            if cv2.contourArea(c) < 5000:
                continue
     
            # compute the bounding box for the contour, draw it on the frame,
            # and update the text
            (self.mov_x, self.mov_y, self.mov_w, self.mov_h) = cv2.boundingRect(c)
            self.WDdrawing = 1
            
            self.motionCounter += 1
            self.lastMovtionCaptured = timestamp

        if (timestamp - self.lastMovtionCaptured).seconds >= 0.5:
            self.WDdrawing = 0

        self.pause()

    def faceDetection(self, imgIn):
        grayGen = cv2.cvtColor(imgIn, cv2.COLOR_BGR2GRAY)
        self.faces = faceCascade.detectMultiScale(
                grayGen,     
                scaleFactor=1.2,
                minNeighbors=5,     
                minSize=(20, 20)
            )
        if self.faces:
            # self.FDdrawing = 1
            print('FD!')
        else:
            # self.FDdrawing = 0
            pass


    def pause(self):
        self.__flag.clear()

    def resume(self):
        self.__flag.set()

    def run(self):
        while 1:
            self.__flag.wait()
            self.CVThreading = 1
            try:
                self.watchDog(self.imgCV)
                self.faceDetection(self.imgCV)
            except Exception as e:
                # print(str(e))
                pass
            self.CVThreading = 0
            pass


class Camera(BaseCamera):
    video_source = 0

    def __init__(self):
        if os.environ.get('OPENCV_CAMERA_SOURCE'):
            Camera.set_video_source(int(os.environ['OPENCV_CAMERA_SOURCE']))
        super(Camera, self).__init__()

    @staticmethod
    def set_video_source(source):
        Camera.video_source = source

    @staticmethod
    def frames():
        camera = cv2.VideoCapture(Camera.video_source)
        if not camera.isOpened():
            raise RuntimeError('Could not start camera.')

        cvt = CVThread()
        cvt.start()

        while True:
            # read current frame
            _, img = camera.read()

            if cvt.CVThreading:
                cvt.pause()
            else:
                cvt.resume()
                cvt.mode(img)

            try:
                img = cvt.elementDraw(img)
            except:
                pass
            
            # encode as a jpeg image and return it
            yield cv2.imencode('.jpg', img)[1].tobytes()