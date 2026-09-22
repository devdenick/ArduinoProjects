//
// NB! This is a file generated from the .4Dino file, changes will be lost
//     the next time the .4Dino file is built
//
#include "gfx4desp32_gen4_ESP32_43CT.h"

gfx4desp32_gen4_ESP32_43CT gfx = gfx4desp32_gen4_ESP32_43CT();

#include "NoName1Const.h"    // Note. This file will not be created if there are no generated graphics

// Uncomment if using GC* in program flash.
// #include "NoName1GCx.h"     // Note. This file will be generated when graphics destination program flash

void setup()
{
  gfx.begin();
  gfx.Cls();
  gfx.ScrollEnable(false);
  gfx.BacklightOn(true);
  gfx.Orientation(PORTRAIT_R);
  gfx.SmoothScrollSpeed(5);
  gfx.TextColor(WHITE, BLACK); gfx.Font(2);  gfx.TextSize(1);
// Uncomment one of the three followin statements depending on the type of graphics file you are using GCI, GCJ, or GC* in program flash.
//gfx.Open4dGFX("NoName1"); // Opens DAT and GCI files for read using filename without extension.
gfx.Open4dGFX("NoName1.gcj"); // Opens GCJ file for read using complete filename.
//gfx.Open4dGFX(NoName1, NoName1_size); // Opens GCJ from program space using array name and size
//gfx.Open4dGFX(NoName1_dat, NoName1_dat_size, NoName1_gci, NoName1_gci_size); // Opens DAT/GCI from program space using array name and size
  gfx.touch_Set(TOUCH_ENABLE);                // Global touch enabled
} // end Setup **do not alter, remove or duplicate this line**

void loop()
{
  // put your main code here, to run repeatedly:
  int itouched, val ;
  if(gfx.touch_Update())
  {
    itouched = gfx.imageTouched() ;
    switch (itouched)
    {                                                         // start touched selection **do not alter, remove or duplicate this line**
      // case statements for Knobs and Sliders go here
      default :                                               // end touched selection **do not alter, remove or duplicate this line**
        int button = gfx.ImageTouchedAuto();    // use default for keyboards and buttons
        val = gfx.getImageValue(button);
        switch (button)
        {                                                     // start button selection **do not alter, remove or duplicate this line**
          // case, one for each button or keyboard, default should end up as -1
        }                                                     // end button selection **do not alter, remove or duplicate this line**
    }
  }
}

