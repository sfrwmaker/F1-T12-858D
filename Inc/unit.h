/*
 * unit.h
 *
 */

#ifndef UNIT_H_
#define UNIT_H_
#include "pid.h"
#include "stat.h"

// Common interface methods for IRON and Hot Air Gun
class UNIT : public PID, public PIDTUNE {
	public:
		UNIT(void)											{ }
		virtual				~UNIT(void)						{ }
		void				init(uint8_t c_len, uint16_t c_min, uint16_t c_max, uint8_t s_len, uint16_t s_min, uint16_t s_max);
		bool				isConnected(void) 				{ return current.status();						}
		uint16_t			unitCurrent(void)				{ return current.read();						} // Used in debug mode only
		void				updateCurrent(uint16_t value) 	{ current.update(value);						}
		uint16_t			reedInternal(void)				{ return sw.read();								}
		void				updateReedStatus(bool on)		{ sw.update(on?100:0);							} // Update Reed switch status
		bool 				isReedSwitch(bool reed);	// REED switch: TRUE if switch is shorten; else: TRUE if status has been changed
		virtual bool		isCold(void)				= 0;
		virtual bool		isOn(void)					= 0;
		virtual void		switchPower(bool On)		= 0;
		virtual uint16_t	presetTemp(void)			= 0;
		virtual void     	setTemp(uint16_t t)			= 0;
		virtual uint16_t	averageTemp(void)			= 0;
		virtual uint8_t     avgPowerPcnt(void)			= 0;
		virtual uint16_t    avgPower(void)				= 0;
		virtual uint16_t 	tmpDispersion(void)			= 0;
		virtual uint16_t	pwrDispersion(void)			= 0;
		virtual void		fixPower(uint16_t Power)	= 0;
		virtual uint16_t    getMaxFixedPower(void)		= 0;
	private:
		SWITCH 			current;							// The current through the unit
		SWITCH 			sw;									// Tilt switch of T12 or Reed switch of Hot Air Gun
};

#endif
