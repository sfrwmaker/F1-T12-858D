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
  
June 03 2020. New fermaware release, 3.02.
  - Hot Air GUN start issue fixed. The power is supplied to the Hot Gun in 1 second after it activated allowing enabling AC relay.
  - "About" menu itmem issue fixed. Now this menu is available.
  - Tilt switch procedures reviewed and updated.
  - The standby mode separated from main working mode and implemented as a new one.
  - Screen saver timeout now is a single entity. It is no longer depended on mode switching. To return to the normal screen mode, touch the encoder.
  - Software automatic power off procedure reviewed and updated. The controller can power off the iron in case of no tilt switch available.
  - Accelerated mode of rotary encoder implemented in setting low power temperature procedure in the main menu.
