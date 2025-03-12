# arduino-cam

ESP32-CAM project that integrates with a TFT screen.

| ![Back image for the ESP32-CAM project](img_back.webp) | ![Front image for the ESP32-CAM project](img_front.webp) |
| ------------------------------------------------------ | -------------------------------------------------------- |
| ![Printing demo](img_demo_1.webp)                      | ![Printing demo](img_demo_2.webp)                        |
| ------------------------------------------------------ | -------------------------------------------------------- |

## Limitations

-   GPIO 16 is used as a button but also by the camera
    -   Workaround: Don't use it for taking photos
-   MicroSD card is unusable when display is turned on
    -   Workaround: End TFT connection while saving files

## Todo

-   Selecting file to read from for printing
-   Bluetooth serial printing?
-   Gallery
-   Flash toggle
-   Finetune filter
-   Nicer UI
-   Handling errors (don't save without SD card)
-   Prompt whether to print image
