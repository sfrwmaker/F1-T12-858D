# F1-T12-858D
Soldering station for hakko t12 tips + 858D rework station

The project description is available here: https://www.hackster.io/sfrwmaker/stm32-based-soldering-and-rework-station-e3955b

REVISION HISTORY:

Nov 2019. First release created

Jan 2020. Second release created
  - Minor changes in the controller schematics
  - Now the Hot Air Gun reed switch controls working mode
  - The soldering iron can keep working while Hot Air Gun mode
  - Screen saver feature implemented
  
Jan 2020. Some updates of the second release
  - Minor changes in the controller schematics
  - Buzzer is always on issue fixed

Feb 11 2020. Error on Op-amp label on schematics fixed. LM328 - > LM358

Mar 23 2020. New firmware release, 2.02. See detailed description on the project page.
  - Updated Hot Air Gun cooling procedure
  - Debug mode added allowing monitoring of the internal parameters

Apr 17 2020. New firmware release, 2.03. See detailed description on the project page.
  - Two mode hardware switch supported: REED and TILT. Tilt switch algorithm updated. 
  - Fixed issue 'jammed on tip selection screen'. Now the main menu reacheble from tip selection mode by long press.
  - Ambient temperature and tilt switch readings now are available on debug screen.
  - Controller without soldering handle connected starts working in Hot Air Gun mode.

Apr 29 2020. New firmware release, 3.00. See detailed description on the project page.
  - Optional 220v safety relay added.
  - Rotary encoder acceleration deactivated.

May 12 2020. New firmware release, 3.01. See detailed description on the project page.
  - Standby mode issues fixed.
  - Temperature step feature added.
  
June 03 2020. New firmaware release, 3.02.
  - Hot Air GUN start issue fixed. The power is supplied to the Hot Gun in 1 second after it activated allowing enabling AC relay.
  - "About" menu item issue fixed. Now this menu is available.
  - Tilt switch procedures reviewed and updated.
  - The standby mode separated from main working mode and implemented as a new one.
  - Screen saver timeout now is a single entity. It is no longer depended on mode switching. To return to the normal screen mode, touch the encoder.
  - Software automatic power off procedure reviewed and updated. The controller can power off the iron in case of no tilt switch available.
  - Accelerated mode of rotary encoder implemented in setting low power temperature procedure in the main menu.
  
  Jan 03 2021. New firmware release, 4.00.
  - Display initialization method chhanged. Now the rotation parameter is inside the contstuctor in display.cpp.
  - Added support for 'Guarantee cool gun' feature. New feature requires the hardware to be updated.
    With new hardware schematics the controller guarantees the Hot Air Gun cooled before shutdown.
    You can put Hot Air Gun to the base then switch off the main power switch and leave your working place.
    The controller will switch-off the Hot Air Gun, waits till the Hot Air gun cooled and power-off itself.
    The new firmware release suports old hardware also. Of course, new 'guarantee cool gun feature' will not be available on old hardware.
    
  Jan 30 2021. New hardware and firmware release, 4.01. The schematics with relay has been simplified.
  - Support for 'Guarantee cool gun' feature changed. New feature requires the hardware to be updated. See stm32_T12+858D_v4.sch file in the eagle directory.
  
  Feb 01 2021. Release 4.02.
  - Fixed controller hangs after Hot Air Gun cooled issue.
  
  Apr 28 2021. Release 4.03.
  - Debug information about AC zero events added to the debug mode
  
  May 25 2021. Release 4.04.
  - New feature added. Now it is possible to manage soldering iron while in hot air gun mode. If the "keep iron" option is enabled in main menu, long-press the encoder toggles the soldering iron.
 
  May 30 2021. Release 4.05.
  - Automatic startup mode implemented. The soldering iron would power-up when the power applied to the controller if mode activated.
  - Boost mode updated. Now boost duration time can be increment by 20 seconds upto 320 seconds. Boost mode temperature now more stable.

  Jun 02 2021. Release 4.06
  - No "about" item menu bug fixed
  
  Sep 21 2021. Release 4.07
  - Support for system clock checking procedure added. The controller checks its clock speed at startup. In case the ckock is lower than 72 MHz, the error message will be displayed. To return to the working mode long press the encoder.
  
  Sep 22 2021. Revert to release 4.06
  - The controller speed inialized correctly alwas, the speed test is not necessary.
  - Some followers reported the new firmware does not work.
  - Restore old source files, rebuild the project. Perhaps, the issue is about the flash limit of the stm32 microcontroller.
 
 Apr 01 2022. Release 4.07
 - Rotaruy encoder manager improved
 - New PID parameters for the Hot Air Gun caliration procedure applied
 - New TIP, T12-D32 added to the tip list

 Nov 07 2023. Release 4.08
 - Implemented new PID algorithm, trying to prevent overheating the soldering iron during heating up.
 - Faster heat-up procedure of soldering iron.
 - New manage procedure for Hot Air Gun imported from my recent project of soldering station with JBC tips.
