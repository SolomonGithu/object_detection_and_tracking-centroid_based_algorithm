#include "utils.h"

// Credits: https://seeklogo.com/images/E/espressif-systems-logo-1350B9E771-seeklogo.com.png

/* This file sets the HTML content and the JSON data that contains the bounding boxes and tracked objects*/

String index_html;
String current_found_objects_JSONData;
bool isFirstBB = true;
bool started_found_objects_JSONData = false;
bool started_tracked_objects_JSONData = false;

void set_index_html() {
  index_html = "<meta charset=\"utf-8\"/>\n" \
               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> \n" \
               "<link rel='icon' type='image/x-icon' href='https://seeklogo.com/images/E/espressif-systems-logo-1350B9E771-seeklogo.com.png'> " \
               "<title>ESP32 object detection and tracking</title> \n" \
               "<style>\n" \
               "body { "\
               "  background-color: #ffffff;\n" \
               "  font-family: 'Gill Sans', sans-serif;\n" \
               "  height: 100%\n" \
               "}" \
               "#content {\n" \
               "  justify-content: center;\n" \
               "  align-items: center;\n" \
               "  text-align: center;\n" \
               "}" \
               "#video_img {\n" \
               "  border-radius: 20px;\n " \
               "  box-shadow: 0 3px 10px rgba(0, 0, 0, 0.5);\n " \
               "  display: block;\n" \
               "  margin-left: auto;\n" \
               "  margin-right: auto;\n" \
               "}\n" \
               "</style>\n" \
               "<body>" \
               "<div id=\"content\">" \
               "<h2 style=\"text-align:center; font-size:25px; color:#14151A\">ESP32 live object detection and tracking using Edge Impulse FOMO</h2>" \
               "<p> Tracking <span id='tracked_items_count'></span> nut(s)</p>" \
               "<img id='video_img' src=\"video\" style=\"width: 480px; height:480px;\">" \
               "<canvas id='videoCanvas' width='480px' height='480px'></canvas>" \
               "</div>" \
               "<script>" \
               "const objects_name = 'nut '; " \
               "const bounding_boxes_to_current_image_size_scale = 5; " \
               "var current_bounding_boxes; " \
               "const objects_circle_radius = 3; " \
               "const text_distance_from_centroid = 3; " \
               "const circle_lineWidth = 2; " \
               "var video_img = document.getElementById('video_img'); " \
               "var videoCanvas = document.getElementById('videoCanvas'); " \
               "const ctx = videoCanvas.getContext('2d'); " \
               "async function get_found_objects(){ " \
               "  const response = await fetch('http://" + ip_address + "/found_objects'); " \
               "  var json_response = await response.json(); " \
               "  console.log('json_response = ', JSON.stringify(json_response)); " \
               "  current_bounding_boxes = json_response['bounding_boxes']; " \
               "  if (json_response['bounding_boxes']){" \
               "      console.log('Bounding boxes found in the response..'); " \
               "      console.log('current_bounding_boxes : ', current_bounding_boxes); " \
               "      draw_FOMO_circles_on_objects(); " \
               "  } " \
               "  if(json_response['tracked_objects']){" \
               "      console.log('Tracked objects found in the response..'); " \
               "      let current_tracked_objects = json_response['tracked_objects']; " \
               "      let number_of_tracked_objects = current_tracked_objects.length; " \
               "      console.log('Current number of tracked objects = ', number_of_tracked_objects); " \
               "      document.getElementById('tracked_items_count').innerHTML = number_of_tracked_objects; " \
               "      for (var x = 0; x < number_of_tracked_objects; x++) { " \
               "        ctx.font = 'bold 20px Arial'; " \
               "        ctx.fillStyle = 'rgb(255, 0, 0)'; " \
               "        let text_X_position = (current_tracked_objects[x]['centroid_X'] - text_distance_from_centroid ) * bounding_boxes_to_current_image_size_scale; " \
               "        let text_Y_position = (current_tracked_objects[x]['centroid_Y'] - text_distance_from_centroid ) * bounding_boxes_to_current_image_size_scale; " \
               "        let object_fillText = objects_name + current_tracked_objects[x]['id']; " \
               "        ctx.fillText(object_fillText, text_X_position, text_Y_position); " \
               "     } " \
               "  } " \
               "  else{" \
               "      console.log('No bounding boxes found in the response!'); " \
               "      document.getElementById('tracked_items_count').innerHTML = 0; " \
               "      var video_img = document.getElementById('video_img'); " \
               "      var videoCanvas = document.getElementById('videoCanvas'); " \
               "      const ctx = videoCanvas.getContext('2d'); " \
               "      ctx.clearRect(0, 0, videoCanvas.width, videoCanvas.height); " \
               "  } " \
               "} " \
               "function draw_FOMO_circles_on_objects(){ " \
               "  videoCanvas.style.position = 'absolute'; " \
               "  videoCanvas.style.left = video_img.offsetLeft + 'px'; " \
               "  videoCanvas.style.top = video_img.offsetTop + 'px'; " \
               "  var number_of_bounding_boxes = current_bounding_boxes.length; " \
               "  console.log('Current number of bounding boxes = ', number_of_bounding_boxes); " \
               "  ctx.clearRect(0, 0, videoCanvas.width, videoCanvas.height); " \
               "  for (var x = 0; x < number_of_bounding_boxes; x++){ " \
               "      let centerX = current_bounding_boxes[x]['bounding_box_X'] + (current_bounding_boxes[x]['bounding_box_Width'] / 2); " \
               "      let centerY = current_bounding_boxes[x]['bounding_box_Y'] + (current_bounding_boxes[x]['bounding_box_Height'] / 2); " \
               "      console.log('centerX: %s, centerY: %s', centerX, centerY); " \
               "      let circle_X_coordinate = centerX * bounding_boxes_to_current_image_size_scale; " \
               "      let circle_Y_coordinate = centerY * bounding_boxes_to_current_image_size_scale; " \
               "      ctx.beginPath(); " \
               "      ctx.arc(circle_X_coordinate, circle_Y_coordinate, objects_circle_radius, 0, Math.PI * 2, false); "\
               "      ctx.closePath(); " \
               "      ctx.lineWidth = circle_lineWidth; " \
               "      ctx.strokeStyle = 'red'; " \
               "      ctx.stroke(); " \
               "  } " \
               "} " \
               "function periodic_fetch(){ " \
               "  console.log('---------------------------------------------------'); " \
               "  get_found_objects(); " \
               "  setTimeout(periodic_fetch, 1000); " \
               "} " \
               "periodic_fetch(); " \
               "</script>" \
               "</body>";
}

void append_found_objects_to_JSON(int bounding_box_X, int bounding_box_Y, int bounding_box_Width, int bounding_box_Height) {
  /* creates a JSON with the bounding boxes and tracked objects of the current void loop run, something like this:
    {
      "bounding_boxes": [
          {
              "bounding_box_X": 8,
              "bounding_box_Y": 8,
              "bounding_box_Width": 8,
              "bounding_box_Height": 16
          },
          {
              "bounding_box_X": 24,
              "bounding_box_Y": 24,
              "bounding_box_Width": 8,
              "bounding_box_Height": 8
          }
      ]
    }

    The JSON is then returned by the /found_objects HTTP GET route
  */

  if (!started_found_objects_JSONData) {
    current_found_objects_JSONData = "{\"bounding_boxes\":[";
    started_found_objects_JSONData = true;
  }

  if (started_found_objects_JSONData) {
    if (isFirstBB) {
      current_found_objects_JSONData += "{";
      current_found_objects_JSONData += "\"bounding_box_X\":" + String(bounding_box_X) + ",";
      current_found_objects_JSONData += "\"bounding_box_Y\":" + String(bounding_box_Y) + ",";
      current_found_objects_JSONData += "\"bounding_box_Width\":" + String(bounding_box_Width) + ",";
      current_found_objects_JSONData += "\"bounding_box_Height\":" + String(bounding_box_Height);
      current_found_objects_JSONData += "}";

      isFirstBB = false;
    }
    else {
      current_found_objects_JSONData += ",{";
      current_found_objects_JSONData += "\"bounding_box_X\":" + String(bounding_box_X) + ",";
      current_found_objects_JSONData += "\"bounding_box_Y\":" + String(bounding_box_Y) + ",";
      current_found_objects_JSONData += "\"bounding_box_Width\":" + String(bounding_box_Width) + ",";
      current_found_objects_JSONData += "\"bounding_box_Height\":" + String(bounding_box_Height);
      current_found_objects_JSONData += "}";
    }
  }
}

void append_tracked_objects_to_JSOData(int arr[][3], int number_of_tracked_objects) {
  /* appends the JSON with the object IDs and centroids, something like this:
       "tracked_objects":[
          {
              "id":0,
              "centroid_X":20,
              "centroid_Y":12
          },
          {
              "id":1,
              "centroid_X":28,
              "centroid_Y":28
          }
      ]
  */

  if (!started_tracked_objects_JSONData) {
    current_found_objects_JSONData += "\"tracked_objects\":[";
    started_tracked_objects_JSONData = true;
  }

  if (started_tracked_objects_JSONData) {
    
    for (int i = 0; i < number_of_tracked_objects; i++) {
      current_found_objects_JSONData += "{";
      current_found_objects_JSONData += "\"id\":" + String(arr[i][0]) + ",";
      current_found_objects_JSONData += "\"centroid_X\":" + String(arr[i][1]) + ",";
      current_found_objects_JSONData += "\"centroid_Y\":" + String(arr[i][2]);
      current_found_objects_JSONData += "}";

      // put comma if this is not the last item
      if ((number_of_tracked_objects - i) != 1) {
        current_found_objects_JSONData += ",";
      }
    }
    current_found_objects_JSONData += "]}";
  }
}

void create_JSONData_for_noBBs() {
  current_found_objects_JSONData = "{\"no_bounding_boxes\": \"no_BBs_found\"}";
}

void end_current_bounding_boxes_JSONData() {
  current_found_objects_JSONData += "],";
}

void clear_current_found_objects_JSONData() {
  current_found_objects_JSONData = "";
  started_found_objects_JSONData = false;
  isFirstBB = true;
  started_tracked_objects_JSONData = false;
}

/* Found objects JSON example ---------------------------------------------- */

/* example of JSON returned by /found_objects:
 {
    "bounding_boxes": [
        {
            "bounding_box_X": 32,
            "bounding_box_Y": 16,
            "bounding_box_Width": 8,
            "bounding_box_Height": 8
        },
        {
            "bounding_box_X": 8,
            "bounding_box_Y": 24,
            "bounding_box_Width": 16,
            "bounding_box_Height": 8
        }
    ],
    "tracked_objects": [
        {
            "id": 5,
            "centroid_X": 16,
            "centroid_Y": 28
        },
        {
            "id": 6,
            "centroid_X": 36,
            "centroid_Y": 20
        }
    ]
}
 */
