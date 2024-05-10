#!/usr/bin/env python

# Credits: this script has been adapted from Edge Impulse's image classify.py example
# https://github.com/edgeimpulse/linux-sdk-python/blob/master/examples/image/classify.py

# Developed by: Solomon Muhunyo Githu, 17th July 2023.

import cv2
import os
import time
import sys
from edge_impulse_linux.image import ImageImpulseRunner
from centroid_tracker import CentroidTracker

# define a video capture object
camera_id = 0 
#camera_id = 'http://' 

runner = None

# initialize our centroid tracker
centroid_tracker_object = CentroidTracker()
(H, W) = (None, None)

bb_rects = []
tracked_cards = 0

# if you don't want to see a video preview, set this to False
show_camera_feed = True
if (sys.platform == 'linux' and not os.environ.get('DISPLAY')):
    show_camera_feed = False

def now():
    return round(time.time() * 1000)

def main():
    dir_path = os.path.dirname(os.path.realpath(__file__))
    # there are two Linux x86 .eim; the difference is the model's image input size (160x160 px and 320x320 px)
    modelfile = os.path.join(dir_path, 'modelfile/' ,'modelfile_320x320_image.eim')

    print('MODEL: ' + modelfile)

    with ImageImpulseRunner(modelfile) as runner:
        try:
            model_info = runner.init()
            print('Loaded runner for "' + model_info['project']['owner'] + ' / ' + model_info['project']['name'] + '"')
            labels = model_info['model_parameters']['labels']

            camera = cv2.VideoCapture(camera_id)

            ret = camera.read()
            if ret:
                backendName = camera.getBackendName()
                w = camera.get(3)
                h = camera.get(4)
                print("Camera %s (%s x %s) in port %s selected." %(backendName,h,w, camera_id))
                camera.release()
            else:
                raise Exception("Couldn't initialize selected camera.")

            next_frame = 0 # limit to ~10 fps here

            print(" ==== Getting classification runner response ====")
            for res, img in runner.classifier(camera_id):
                
                print("Image shape ", img.shape)

                if (next_frame > now()):
                   time.sleep((next_frame - now()) / 1000)
                
                print('EI classification runner response', res)
                if "classification" in res["result"].keys():
                    print('Result (%d ms.) ' % (res['timing']['dsp'] + res['timing']['classification']), end='')
                    for label in labels:
                        score = res['result']['classification'][label]
                        print('%s: %.2f\t' % (label, score), end='')
                    print('', flush=True)

                elif "bounding_boxes" in res["result"].keys():
                    print('Found %d bounding boxes (%d ms.)' % (len(res["result"]["bounding_boxes"]), res['timing']['dsp'] + res['timing']['classification']))
                    for bb in res["result"]["bounding_boxes"]:
                        print('\t%s (%.2f): x=%d y=%d w=%d h=%d' % (bb['label'], bb['value'], bb['x'], bb['y'], bb['width'], bb['height']))
                        
                        # for each object that is a card, we get it's bounding boxes and track it
                        if(bb['label'] == 'card'):
                            cv2.rectangle(img, (bb['x'], bb['y']), (bb['x'] + bb['width'], bb['y'] + bb['height']), (0, 128, 0), 1)

                            # copy found card's bounding box to a list
                            cards_bb = [bb['x'], bb['y'], (bb['x'] + bb['width']), (bb['y'] + bb['height'])]
                            
                            global bb_rects
                            bb_rects.append(cards_bb) # append the card's bounding box to global variable

                next_frame = now() + 100

                # update our centroid tracker with the found card's bounding boxes
                tracked_objects = centroid_tracker_object.update(bb_rects)

                bb_rects = [] # reset bb_rects

                tracked_cards = len(tracked_objects.items()) # get the count of objects being tracked
                print("Tracked cards: ",tracked_cards)

                if (show_camera_feed):
                    
                    header_text = "Tracking " + str(tracked_cards) + " card(s)"
                    # cv2.putText(image, text, org, font, fontScale, color[, thickness[, lineType[, bottomLeftOrigin]]])
                    cv2.putText(img, header_text, (0, 15),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)
                                     
                    # loop over the tracked objects
                    for (objectID, centroid) in tracked_objects.items():
                        # draw both the ID of the object and the centroid of the object on the frame
                        text = "Card {}".format(objectID)
                        cv2.putText(img, text, (centroid[0] - 25, centroid[1] - 25),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
                        #cv2.circle(image, center_coordinates, radius, color, thickness)
                        cv2.circle(img, (centroid[0], centroid[1]), 2, (0, 255, 0), -1)

                    # Name the window
                    cv2.namedWindow('EI object detection and tracking', cv2.WINDOW_NORMAL)
                    # Resizing a window: name, width, height
                    cv2.resizeWindow('EI object detection and tracking', 600, 600)
                    
                    cv2.imshow('EI object detection and tracking', cv2.cvtColor(img, cv2.COLOR_RGB2BGR))

                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        break
                
        finally:
            if (runner):
                runner.stop()
    
    # After the loop release the camera object
    cv2.VideoCapture(camera_id).release()
    # Destroy all the windows
    cv2.destroyAllWindows()
    print("Exited!")

if __name__ == "__main__":
   main()
