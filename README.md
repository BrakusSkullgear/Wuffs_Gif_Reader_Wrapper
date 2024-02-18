# Wuffs_Gif_Reader_Wrapper

Simple and easy to use C++ wrapper to read single frames from a gif-file using wuffs-library. It uses wuffs gifplayer-example as a base packed into a class without the confusing global variables. Feel free to report if you find any bugs or if I have misunderstood how to use wuffs properly.

Required external files:
- https://github.com/google/wuffs/blob/main/release/c/wuffs-v0.3.c
- https://github.com/nothings/stb/blob/master/stb_image_write.h (to write the frames to file)

Feel free to use my wrapper codes as you like, although you need the wuffs-license of course.

In case you want to see these codes in action in my game engine where they are used to play avatar animations, check out my game Dandy Boy Adventures:
- https://dandyboyoni.itch.io/dandyboyadventures WARNING: Contains adult content.
