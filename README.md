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

Feb 11 2020. Error on Op-amp label on schematics fixed. LM328 - > LM 358

Mar 23 2020. New firmware release, 2.02. See detailed description on the project page.
  - Updated Hot Air Gun cooling procedure
  - Debug mode added allowing monitoring of the internal parameters

Apr 17 2020. New firmware release, 2.03. See detailed description on the project page.
  - Two mode hardware switch supported: REED and TILT. Tilt switch algorithm updated. 
  - Fixed issue 'jammed on tip selection screen'. Now the main menu reacheble from tip selection mode by long press.
  - Ambient temperature and tilt switch readings now are available on debug screen.
  - Controller without soldering handle connected starts working in Hot Air Gun mode.
