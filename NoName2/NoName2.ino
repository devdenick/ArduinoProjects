//
// NB! This is a file generated from the .4Dino file, changes will be lost
//     the next time the .4Dino file is built
//
#include "gfx4desp32_gen4_ESP32_43CT.h"

gfx4desp32_gen4_ESP32_43CT gfx = gfx4desp32_gen4_ESP32_43CT();

#include "NoName2Const.h"    // Note. This file will not be created if there are no generated graphics

// Uncomment if using GC* in program flash.
// #include "NoName2GCx.h"     // Note. This file will be generated when graphics destination program flash

void setup()
{
  gfx.begin();
  gfx.Cls();
  gfx.ScrollEnable(false);
  gfx.BacklightOn(true);
  gfx.Orientation(PORTRAIT);
  gfx.SmoothScrollSpeed(5);
  gfx.TextColor(WHITE, BLACK); gfx.Font(2);  gfx.TextSize(1);
// Uncomment one of the three followin statements depending on the type of graphics file you are using GCI, GCJ, or GC* in program flash.
//gfx.Open4dGFX("NoName2"); // Opens DAT and GCI files for read using filename without extension.
gfx.Open4dGFX("NoName2.gcj"); // Opens GCJ file for read using complete filename.
//gfx.Open4dGFX(NoName2, NoName2_size); // Opens GCJ from program space using array name and size
//gfx.Open4dGFX(NoName2_dat, NoName2_dat_size, NoName2_gci, NoName2_gci_size); // Opens DAT/GCI from program space using array name and size
  gfx.touch_Set(TOUCH_ENABLE);                // Global touch enabled
  gfx.imageTouchEnable(iRotaryswitch1, true) ;               // init_Rotaryswitch1 enable touch of widget (on Form1)
  gfx.UserImages(iRotaryswitch1,0) ;                         // init_Rotaryswitch1 show initially, if required (on Form1)
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

      // Form1 1.1 generated 16/07/2026 10:56:27
        int button = gfx.ImageTouchedAuto();    // use default for keyboards and buttons
        gfx.Button(1, 101, 67, RED, YELLOW, FONT1, 1, 1, "Button1");
        val = gfx.getImageValue(button);
        switch (button)
        {                                                     // start button selection **do not alter, remove or duplicate this line**
          // case, one for each button or keyboard, default should end up as -1
        }                                                     // end button selection **do not alter, remove or duplicate this line**
    }
  }
}

