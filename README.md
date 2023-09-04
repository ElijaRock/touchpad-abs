# Touchpad abs
A touchpad driver that behaves similarly to a touchscreen or graphics tablet (absolute positioning) for Linux

## Installation ~10 min
Clone this repository and cd into it:
```
git clone 'https://github.com/ElijaRock/touchpad-abs' && cd touchpad-abs
```

Download `evtest` for your Linux distribution. Run it as root user and find the file path for your touchpad.
```
sudo evtest |& grep Touchpad
```

You should see a line like this:
```
/dev/input/event11:     ELAN1201:00 0000:0000 Touchpad
```

Make sure to write down the number for this device. Now run the following command as root, move your finger to the right edge of the touchpad, and record the maximum values you see. Replace "event11" with your event.
```
sudo evtest /dev/input/event11 |& grep 'code 53'
```

Repeat the preceding step but move your finger to the bottom edge this time to record the maximum y value.
```
sudo evtest /dev/input/event11 |& grep 'code 54'
```

You should have written down three numbers by now:
    
    1. The event number of your touchpad, e.g. "event11"
    
    2. The maximum x value of the touchpad, e.g. "3192"

    3. The maximum y value of the touchpad, e.g. "1824"

Record the resolution of your screen. Those are all the values you need. Unless you want to fine-tune the area of the simulated touchpad, you can skip the next step.

If you want to fine-tune the area of the touchpad, measure the width and height with a ruler, and write down the dimensions in millimeters. Then decide on the adjusted smaller area you want to use.

If you have followed the preceding step to fine-tune the area, you should now have two extra data:

    4. The dimensions of your touchpad in millimeters

    5. The dimensions of the area you want to work with in millimeters

Now that you are ready to run the program, open "set_loc.c" in a text editor to add your values or "set_loc_mini.c" if you want to work with a reduced area.

"TOUCHPAD_EVENT_X" is the event number of your touchpad, with the word "event" preceding it. For example: "event11"
"RES_WIDTH" and "RES_HEIGHT" store the pixel width and heights of your computer screen respectively. For example: "1920" and "1080"
"MAX_ABS_X" and "MAX_ABS_Y" are the values you recorded for the maximum x and y values of your touchpad with `evtest` respectively. For example: "3192" and "1824"

If you are using a reduced area, then:
"TOUCHPAD_WIDTH_MM" and "TOUCHPAD_HEIGHT_MM" are the width and height of your touchpad dimensions in millimeters respectively.
"ADJUSTED_WIDTH_MM" and "ADJUSTED_HEIGHT_MM" store the reduced area you want to use.

That's all for configuration on the user side. You may now compile the program using `gcc`. If it's not there you can install it with your Linux distributions's package manager.

Now run the following command to compile the program:

Full area: `gcc -o touchpad set_loc.c`
Reduced area: `gcc -o touchpad set_loc_mini.c`

Now you should be able to run `sudo ./touchpad` and enjoy.
