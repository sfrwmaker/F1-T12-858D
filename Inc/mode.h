/*
 * mode.h
 *
 *  Created on: 19 sep. 2019
 *      Author: Alex
 */

#ifndef MODE_H_
#define MODE_H_

#include "buzzer.h"
#include "core.h"
#include "oled.h"
#include "encoder.h"
#include "display.h"
#include "iron.h"
#include "gun.h"
#include "cfgtypes.h"
#include "config.h"
#include "stat.h"
#include "hw.h"

class MODE {
	public:
		MODE(HW *pCore)										{ this->pCore = pCore; 	}
		void			setup(MODE* return_mode, MODE* short_mode, MODE* long_mode);
		virtual void	init(void)							{ }
		virtual MODE*	loop(void)							{ return 0; }
		virtual			~MODE(void)							{ }
		void			ironMode(bool iron)					{ use_iron 	= iron; 	}
		void			keepIronWorking(bool iw)			{ keep_iron = iw; 		}
		MODE*			returnToMain(void);
	protected:
		void 			resetTimeout(void);
		void 			setTimeout(uint16_t t);
		bool			use_iron		= true;
		bool			keep_iron		= false;			// Keep iron working while in Hot Air Gun mode
		HW*				pCore			= 0;
		uint16_t		timeout_secs	= 0;				// Timeout to return to main mode, seconds
		uint32_t		time_to_return 	= 0;				// Time in ms when to return to the main mode
		uint32_t		update_screen	= 0;				// Time in ms when the screen should be updated
		MODE*			mode_return		= 0;				// Previous working mode
		MODE*			mode_spress		= 0;				// When encoder button short pressed
		MODE*			mode_lpress		= 0;				// When encoder button long  pressed

};

class SCRSAVER {
	public:
		SCRSAVER(void)										{ }
		void			init(uint8_t timeout)				{ to = timeout; reset(); }
		void			reset(void);
		bool 			scrSaver(void);
	private:
		uint32_t		scr_save_ms		= 0;				// Time to switch to Screen Saver mode (if > 0, ms)
		uint8_t			to				= 0;				// Timeout, minutes
		bool			scr_saver		= false;			// Is the screen saver active
};

//---------------------- The iron standby mode -----------------------------------
class MSTBY_IRON : public MODE, SCRSAVER {
	public:
		MSTBY_IRON(HW *pCore) : MODE(pCore)					{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
		void			setGunMode(MODE* gw)				{ gun_work = gw; }
	private:
		MODE*			gun_work		= 0;				// Hot Air Gun Work mode
		uint32_t		clear_used_ms	= 0;				// Time in ms when used flag should be cleared (if > 0)
		bool			used			= false;			// Whether the IRON was used (was hot)
		bool			cool_notified	= 0;				// Whether there was cold notification played
		bool			no_handle		= false;			// Whether soldering iron handle disconnected (no ambient sensor)
		uint16_t 		old_temp_set	= 0;
};

//-------------------- The iron main working mode, keep the temperature ----------
class MWORK_IRON : public MODE, SCRSAVER {
	public:
		MWORK_IRON(HW *pCore) : MODE(pCore), idle_pwr(ec)					{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
		void			setGunMode(MODE* gw)				{ gun_work = gw; }
	private:
		void 			adjustPresetTemp(void);
		void			resetStandbyMode(void);
		void			hwTimeout(uint16_t low_temp, bool tilt_active);
		void 			swTimeout(uint16_t temp, uint16_t temp_set, uint16_t temp_setH, uint32_t td, uint32_t pd, uint16_t ap, int16_t ip);
		MODE*			gun_work		= 0;				// Hot Air Gun Standby mode
		EMP_AVERAGE  	idle_pwr;							// Exponential average value for idle power
		bool 			auto_off_notified = false;			// The time (in ms) when the automatic power-off was notified
		bool      		ready			= false;			// Whether the IRON have reached the preset temperature
		bool			lowpower_mode	= false;			// Whether hardware low power mode using tilt switch
		uint32_t		ready_clear		= 0;				// Time when to clean 'Ready' message
		uint32_t		lowpower_time	= 0;				// Time when switch to standby power mode
		uint16_t 		old_temp_set	= 0;
		const uint16_t	period			= 500;				// Redraw display period (ms)
		const uint8_t	ec				= 5;				// The exponential average coefficient
};

//---------------------- The boost mode, shortly increase the temperature --------
class MBOOST : public MODE {
	public:
		MBOOST(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint16_t 		old_pos 	  	= 0;
		bool			heating			= false;
};

//---------------------- The tip selection mode ----------------------------------
class MSLCT : public MODE {
	public:
		MSLCT(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		TIP_ITEM		tip_list[3];
		uint32_t 		tip_begin_select	= 0;			// The time in ms when we started to select new tip
		uint8_t 		old_index = 3;
};

//---------------------- The Activate tip mode: select tips to use ---------------
class MTACT : public MODE {
	public:
		MTACT(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint8_t 	old_tip_index = 255;
};


//---------------------- The Menu mode -------------------------------------------
class MMENU : public MODE {
	public:
		MMENU(HW* pCore, MODE* m_boost, MODE* m_calib, MODE* m_act, MODE* m_tune, MODE* m_pid, MODE* m_gun_menu, MODE *m_about);
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*		mode_menu_boost;
		MODE*		mode_calibrate_menu;
		MODE*		mode_activate_tips;
		MODE*		mode_tune;
		MODE*		mode_tune_pid;
		MODE*		mode_gun_menu;
		MODE*		mode_about;
		uint8_t		off_timeout		= 0;					// Automatic switch off timeout in minutes or 0 to disable
		uint16_t	low_temp		= 0;					// The low power temperature (Celsius) 0 - disable tilt sensor
		uint8_t		low_to			= 0;					// The low power timeout, seconds
		uint8_t		scr_saver		= 0;					// Screen saver timeout in minutes or 0 to disable
		bool		buzzer			= true;					// Whether the buzzer is enabled
		bool		celsius			= true;					// Temperature units: C/F
		bool		keep_iron		= false;				// Keep the iron working While in Hot Air Gun Mode
		bool		reed			= false;				// IRON switch type: reed/tilt
		bool		temp_step		= false;				// The preset temperature step (1/5)
		uint8_t		set_param		= 0;					// The index of the modifying parameter
		uint8_t		m_len			= 18;					// The menu length
		uint8_t		mode_menu_item 	= 1;					// Save active menu element index to return back later
		const char* menu_name[19] = {
			"boost setup",
			"units",
			"buzzer",
			"keep iron",
			"switch type",
			"temp. step",
			"auto off",
			"standby temp",
			"standby time",
			"screen saver",
			"save",
			"cancel",
			"calibrate tip",
			"activate tips",
			"tune iron",
			"gun menu",
			"reset config",
			"tune iron PID",
			"about"
		};
		const uint16_t	min_standby_C	= 120;				// Minimum standby temperature, Celsius
};

//---------------------- Calibrate tip menu --------------------------------------
class MCALMENU : public MODE {
	public:
		MCALMENU(HW* pCore, MODE* cal_auto, MODE* cal_manual);
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*			mode_calibrate_tip;
		MODE*			mode_calibrate_tip_manual;
		uint8_t  		old_item = 4;
		const char* menu_list[4] = {
			"automatic",
			"manual",
			"clear",
			"exit"
		};
};

//---------------------- The calibrate tip mode: setup temperature ---------------
#define MCALIB_POINTS	8
class MCALIB : public MODE {
	public:
		MCALIB(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		bool 		calibrationOLS(uint16_t* tip, uint16_t min_temp, uint16_t max_temp);
		uint8_t		closestIndex(uint16_t temp);
		void 		updateReference(uint8_t indx);
		void 		buildFinishCalibration(void);
		uint8_t		ref_temp_index	= 0;					// Which temperature reference to change: [0-MCALIB_POINTS]
		uint16_t	calib_temp[2][MCALIB_POINTS];			// The calibration data: real temp. [0] and temp. in internal units [1]
		uint16_t	tip_temp_max	= 0;					// the maximum possible tip temperature in the internal units
		bool		ready			= false;				// Whether the temperature has been established
		bool		tuning			= false;
		int16_t		old_encoder 	= 3;
		const uint16_t start_int_temp = 600;				// Minimal temperature in internal units, about 100 degrees Celsius
};

//---------------------- The calibrate tip mode: manual calibration --------------
class MCALIB_MANUAL : public MODE {
	public:
		MCALIB_MANUAL(HW *pCore) : MODE(pCore)				{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		void 		buildCalibration(int8_t ablient, uint16_t tip[], uint8_t ref_point);
		void		restorePIDconfig(CFG *pCFG, IRON* pIron, HOTGUN* pHG);
		uint8_t		ref_temp_index	= 0;					// Which temperature reference to change: [0-3]
		uint16_t	calib_temp[4];							// The calibration temp. in internal units in reference points
		bool		ready			= 0;					// Whether the temperature has been established
		bool		tuning			= 0;					// Whether the reference temperature is modifying (else we select new reference point)
		uint32_t	temp_setready_ms	= 0;				// The time in ms when we should check the temperature is ready
		int16_t		old_encoder 	= 4;
		uint16_t	fan_speed		= 1500;					// The Hot Air Gun fan speed during calibration
};

//---------------------- The Boost setup menu mode -------------------------------
class MMBST : public MODE {
	public:
		MMBST(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint8_t			delta_temp	= 0;					// The temperature increment
		uint8_t			duration	= 0;					// The boost period (secs)
		uint8_t			mode		= 0;					// The current mode: 0: select menu item, 1 - change temp, 2 - change duration
		uint8_t 		old_item 	= 0;
		const char* boost_name[3] = {
			"temperature",
			"duration",
			"back to menu"
		};
};


//---------------------- The tune mode -------------------------------------------
class MTUNE : public MODE {
	public:
		MTUNE(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint16_t 	old_power 		= 0;
		bool		powered   		= true;
		bool		check_connected	= false;				// Flag indicating to check IRON or Hot Air Gun is connected
		uint32_t	check_delay		= 0;					// Time in ms when to start checking Hot Air Gun is connected
};

//---------------------- The PID coefficients tune mode --------------------------
class MTPID : public MODE {
	public:
		MTPID(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint32_t	data_update	= 0;						// When read the data from the sensors (ms)
		uint32_t	temp_setready_ms	= 0;				// The time in ms when we should check the temperature is ready
		uint8_t		data_index	= 0;						// Active coefficient
		bool        modify		= 0;						// Whether is modifying value of coefficient
		bool		on			= 0;						// Whether the IRON is turned on
		uint16_t 	old_index 	= 3;
};

//---------------------- The Hot Air Gun main working mode -----------------------
class MWORK_GUN : public MODE, SCRSAVER {
	public:
		MWORK_GUN(HW *pCore) : MODE(pCore)					{ }
		void			setIronModes(MODE* is, MODE* iw)				{ iron_standby = is; iron_working = iw; }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*			iron_standby	= 0;				// Standby IRON mode
		MODE*			iron_working	= 0;				// Working IRON mode
		uint32_t 		old_param		= 0;
		bool			edit_temp		= true;				// The rotary encoder is changing the temperature preset
		bool      		ready			= false;			// Whether the Hot Air Gun have reached the preset temperature
		uint32_t		return_to_temp	= 0;				// The time when to return to change the temperature
		uint32_t		ready_clear		= 0;				// Time when to clean 'Ready' message
		uint32_t		fan_animate		= 0;				// Time when draw new fan animation
		uint8_t			fan_angle		= 0;				// Current angle of fan icon [0..3]
		const uint16_t	edit_fan_timeout = 3000;			// The time to edit fan speed (ms)
};

//---------------------- Hot Air Gun setup menu ----------------------------------
class MENU_GUN : public MODE {
	public:
		MENU_GUN(HW* pCore, MODE* calib, MODE* pot_tune, MODE* pid_tune);
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*			mode_calibrate;
		MODE*			mode_tune;
		MODE*			mode_pid;
		uint8_t  		old_item	= 5;
		const char* menu_list[5] = {
			"calibrate",
			"tune gun",
			"tune gun PID",
			"clear",
			"exit"
		};
};

//---------------------- The Fail mode: display error message --------------------
class MFAIL : public MODE {
	public:
		MFAIL(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
};

//---------------------- The About dialog mode. Show about message ---------------
class MABOUT : public MODE {
	public:
		MABOUT(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
};

//---------------------- The Debug mode: display internal parameters ------------
class MDEBUG : public MODE {
	public:
		MDEBUG(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint16_t		old_power 		= 0;				// Old encoder value
		bool			gun_mode		= false;			// Gun/iron mode
		const uint16_t	max_iron_power 	= 300;
		const uint16_t	min_fan_speed	= 600;
		const uint16_t	max_fan_power 	= 1999;
};

#endif /* MODE_H_ */
