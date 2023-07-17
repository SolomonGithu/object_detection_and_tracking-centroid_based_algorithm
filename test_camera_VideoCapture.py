# import the opencv library
import cv2

camera_id = 0
#camera_id = 'http://'

# define a video capture object
vid = cv2.VideoCapture(camera_id)

#frame_width = int(cv2.VideoCapture(camera_id).get(3))
#frame_height = int(cv2.VideoCapture(camera_id).get(4))
#recorded_cv2_window = cv2.VideoWriter('opencv_window_record.avi',cv2.VideoWriter_fourcc('M','J','P','G'), 30, (frame_width,frame_height))

while(True):    
    # Capture the video frame
    # by frame
    ret, frame = vid.read()
  
    # Naming a window, optional
    cv2.namedWindow("Object detection", cv2.WINDOW_NORMAL)
    # Resizing a window, optional
    cv2.resizeWindow("Object detection", 300, 700)

    # Display the resulting frame
    cv2.imshow('frame', frame)

    # Display the resulting frame on a window equal to the camera resolution, optional
    #recorded_cv2_window.write(frame)

    # set the 'q' button to close the window
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# After the loop release the cap object
vid.release()
# Destroy all the windows
cv2.destroyAllWindows()
