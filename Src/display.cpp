/*
 * display.cpp
 *
 */

#include "display.h"
#include "tools.h"
#include <string.h>
#include <stdio.h>

/*
 * Bitmaps
 */
static const uint8_t bmTemperature[] = {
  0b00010000,
  0b00101000,
  0b01101000,
  0b00101000,
  0b01101000,
  0b00111000,
  0b00111000,
  0b01111000,
  0b00111000,
  0b01111100,
  0b11111110,
  0b11111110,
  0b11111110,
  0b01111100,
  0b00111000
};

static const uint8_t bmDegree[] = {
  0b00111000,
  0b01000100,
  0b01000100,
  0b01000100,
  0b00111000
};

static const uint8_t bmNotCalibrated[] = {
  0b01011010,
  0b10011001,
  0b10011001,
  0b10011001,
  0b10011001,
  0b10011001,
  0b10000001,
  0b10011001,
  0b01011010
};

static const uint8_t bmTempGuageLeft[] = {
  0b00111100,
  0b01111110,
  0b01111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111011,
  0b11110011,
  0b01100010,
  0b00111100
};

static const uint8_t bmTempGuageRight[] = {
  0b11111100,
  0b00000010,
  0b00000001,
  0b00000001,
  0b00000010,
  0b11111100,
};

static const uint8_t bmLeftMark[] = {
  0b01100000,
  0b01110000,
  0b01111000,
  0b01111100,
  0b01111000,
  0b01110000,
  0b01100000
};

static const uint8_t bmCheckEmpty[] = {
  0b01111110,
  0b10000001,
  0b10000001,
  0b10000001,
  0b10000001,
  0b10000001,
  0b10000001,
  0b01111110,
};

static const uint8_t bmCheckFull[] = {
  0b01111110,
  0b10000001,
  0b10100101,
  0b10011001,
  0b10011001,
  0b10100101,
  0b10000001,
  0b01111110,
};

static const uint8_t bmArrow[] = {
  0b11100000,
  0b00111100,
  0b11111111,
  0b00111100,
  0b11100000
};

static const uint8_t bmTiltActive[] = {
  0b00000100, 0b00000000,
  0b01000100, 0b01000000,
  0b00101110, 0b10000000,
  0b00011111, 0b00000000,
  0b11111111, 0b11100000,
  0b00011111, 0b00000000,
  0b00101110, 0b10000000,
  0b01000100, 0b01000000,
  0b00000100, 0b00000000
};

static const uint8_t E[] = {
	0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xfc, 0x00, 0xfc, 0x00,
	0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xff, 0xfc, 0xff, 0xfc, 0xff, 0xfc,
	0xff, 0xfc, 0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00,
	0xfc, 0x00, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe
};

static const uint8_t O[] = {
	0x01, 0xff, 0x80, 0x07, 0xff, 0xe0, 0x1f, 0xff, 0xf0, 0x3f, 0xff, 0xf8,
	0x7f, 0xc3, 0xfc, 0x7f, 0x01, 0xfc, 0x7e, 0x00, 0xfe, 0xfe, 0x00, 0xfe,
	0xfc, 0x00, 0x7e, 0xfc, 0x00, 0x7e, 0xfc, 0x00, 0x7e, 0xfc, 0x00, 0x7e,
	0xfc, 0x00, 0x7e, 0xfc, 0x00, 0x7e, 0xfc, 0x00, 0xfe, 0xfe, 0x00, 0xfe,
	0xfe, 0x00, 0xfc, 0x7f, 0x01, 0xfc, 0x7f, 0x87, 0xf8, 0x3f, 0xff, 0xf8,
	0x1f, 0xff, 0xf0, 0x0f, 0xff, 0xc0, 0x03, 0xff, 0x00
};

static const uint8_t R[] = {
	0xff, 0xf8, 0x00, 0xff, 0xfe, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x80,
	0xfc, 0x3f, 0x80, 0xfc, 0x1f, 0xc0, 0xfc, 0x1f, 0xc0, 0xfc, 0x1f, 0xc0,
	0xfc, 0x1f, 0x80, 0xfc, 0x3f, 0x80, 0xff, 0xff, 0x00, 0xff, 0xfe, 0x00,
	0xff, 0xf8, 0x00, 0xff, 0xfc, 0x00, 0xfc, 0xfe, 0x00, 0xfc, 0x7f, 0x00,
	0xfc, 0x3f, 0x00, 0xfc, 0x3f, 0x80, 0xfc, 0x1f, 0x80, 0xfc, 0x1f, 0xc0,
	0xfc, 0x1f, 0xc0, 0xfc, 0x0f, 0xe0, 0xfc, 0x0f, 0xe0
};

static const uint8_t bmFan[4][32] = {
   {0x07, 0x00, 0x07, 0x00, 0x03, 0x80, 0x03, 0x80, 0x01, 0x80, 0x01, 0x80,
    0x01, 0x80, 0x03, 0xc0, 0x03, 0xc0, 0x0f, 0x70, 0x1c, 0x3b, 0x38, 0x1f,
    0x38, 0x0f, 0x70, 0x00, 0x38, 0x00, 0x00, 0x00 },
   {0x00, 0xc0, 0x00, 0xe0, 0x00, 0x70, 0x00, 0x70, 0x00, 0x60, 0x00, 0xc0,
    0x01, 0xc0, 0x2b, 0x80, 0x7f, 0xc0, 0xf1, 0x80, 0xe0, 0xc0, 0x00, 0xe0,
    0x00, 0x70, 0x00, 0x78, 0x00, 0x38, 0x00, 0x18 },
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0e, 0x20, 0x1e, 0x78, 0x38,
    0xfd, 0x70, 0xd7, 0xe0, 0x03, 0xc0, 0x01, 0x80, 0x01, 0x00, 0x01, 0x80,
    0x03, 0x80, 0x03, 0x80, 0x01, 0xc0, 0x00, 0xe0 },
   {0x00, 0x00, 0x20, 0x00, 0xf8, 0x00, 0xfc, 0x00, 0xde, 0x00, 0x06, 0x03,
    0x03, 0x8f, 0x01, 0xff, 0x03, 0xfe, 0x03, 0x80, 0x06, 0x00, 0x0e, 0x00,
    0x1c, 0x00, 0x3c, 0x00, 0x3e, 0x00, 0x0e, 0x00 }
};

static const char* k_proto[3] = {
	"Kp = %5d",
	"Ki = %5d",
	"Kd = %5d"
};

void DSPL::init(const u8g2_cb_t *rotation)  {
	u8x8_msg_cb msg_cb = u8x8_byte_stm32_hw_spi;
	if (HAL_OK == HAL_I2C_IsDeviceReady(&I2C_HANDLER, OLED_I2C_ADDR<<1, 2, 2)) {
		msg_cb = u8x8_byte_stm32_hw_i2c;
	}
//	u8g2_Setup_sh1106_128x64_noname_f(&u8g2, rotation, msg_cb, u8x8_gpio_and_delay_stm32);
	u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, rotation, msg_cb, u8x8_gpio_and_delay_stm32);
//	u8g2_Setup_ssd1309_128x64_noname2_f(&u8g2, rotation, msg_cb, u8x8_gpio_and_delay_stm32);
	saver_center[0] = d_width/2;
	saver_center[1] = d_height/2;
	begin();
}

void DSPL::status(const char *msg) {
	strncpy(msg_buff, msg, 7);
	msg_buff[7] = '\0';
}

void DSPL::msgClean(void) {
	msg_buff[0] = 0;
}

void DSPL::msgOFF(void) {
	static const char *msg = "OFF";
	status(msg);
}

void DSPL::msgON(void) {
	static const char *msg = "ON";
	status(msg);
}

void DSPL::msgCold(void) {
	static const char *msg = "Cold";
	status(msg);
}

void DSPL::msgReady(void) {
	static const char *msg = "Ready";
	status(msg);
}

void DSPL::msgIdle(void) {
	static const char *msg = "Idle";
	status(msg);
}

void DSPL::msgStandby(void) {
	static const char *msg = "Stby";
	status(msg);
}

void DSPL::msgBoost(void) {
	static const char *msg = "Boost";
	status(msg);
}

void DSPL::timeToOff(uint8_t time) {
	sprintf(msg_buff, "%2d", time);
}

void DSPL::tip(const char *tip_name) {
	strncpy(this->tip_name, tip_name, 9);
	this->tip_name[9] = '\0';
}

void DSPL::fanSpeed(uint8_t pcnt) {
	sprintf(tip_name, "Fan:%3d%c", pcnt, '%');
}

void DSPL::animateFan(uint8_t indx) {
	indx &= 0x3;													// Can be from 0 to 3
	U8G2::drawBitmap(0, d_height/2-8, 2, 16, bmFan[indx]);
	U8G2::sendBuffer();
}

/*
 * Main temperature display used in standby or working mode to show IRON or Hot Air Gun status
 * Parameters:
 * t_set			- The preset temperature, to be kept in main working mode
 * t_cur			- Current temperature of IRON or Hot Air Gun
 * t_amb			- The ambient temperature
 * p_applied		- The applied power to the device, percent
 * is_celsius		- The human readable units of the temperature (Celsius if true or Fahrenheit if false)
 * tip_calibrated	- If current tip calibrated
 * t_alter			- If greater than zero, is a alternate (Hot Air Gun or IRON) current temperature
 * tilt_iron_used	- In IRON working mode the status of the iron tilt switch (active or inactive)
 * fan_index		- Index of fan state bitmap sprite [0-3] (in Hot Air Gun mode)
 */
void DSPL::mainShow(uint16_t t_set, uint16_t t_cur, int16_t t_amb, uint8_t p_applied, bool is_celsius,
		bool tip_calibrated, uint16_t t_alter, uint8_t fan_index, bool tilt_iron_used) {
	t_set		= constrain(t_set, 0, 999);
	t_cur		= constrain(t_cur, 0, 999);
	p_applied	= constrain(p_applied, 0, 100);
	static char sym[] = {'C', '\0'};
	if (is_celsius) sym[0] = 'C'; else sym[0] = 'F';
	char buff[10];

	uint8_t preset_label = d_width - d_width / 4 - 10;
	uint8_t temp_bar = 0;
	if (t_cur > t_amb && (t_cur - t_amb) > 20) {
		temp_bar = map(t_cur, t_amb+20, t_set, 0, preset_label);
		if (temp_bar >= d_width-10) temp_bar = d_width - 11;
	}

	uint8_t p_height = gauge(p_applied, 10, 30);					// Applied power triangle height

	U8G2::clearBuffer();
	U8G2::setFont(u8g_font_profont15r);
	// Show preset temperature
	sprintf(buff, "%3d", t_set);
	U8G2::drawBitmap(0, 1, 1, 15, bmTemperature);
	uint8_t width = U8G2::getStrWidth(buff);
	U8G2::drawStr(15, 12, buff);
	U8G2::drawBitmap(16+width, 1, 1, 5, bmDegree);
	U8G2::drawStr(24+width, 12, sym);

	// Show status message: 'ON', 'OFF', 'Idle', etc.
	width = U8G2::getStrWidth(msg_buff);
	U8G2::drawStr(d_width-5 - width, 12, msg_buff);
	// Show tip name
	U8G2::drawStr(12, d_height, tip_name);
	if (!tip_calibrated)
		U8G2::drawBitmap(0, d_height-9, 1, 9, bmNotCalibrated);

	// Show the applied power
	if (p_height > 0)
		U8G2::drawTriangle(d_width-5, 45, d_width-5, 45-p_height, d_width-6-p_height/4, 45-p_height);


	if (t_alter > 0) {											// Show alternate device temperature
		sprintf(buff, "%3d", t_alter);
		width = U8G2::getStrWidth(buff);
		U8G2::drawStr(d_width-width-20, d_height, buff);
		U8G2::drawDisc(d_width-width-28, d_height-5, 2);
	} else {
		// Show the ambient temperature
		if (t_amb >= -9 && t_amb < 100) {
			sprintf(buff, "%2d", t_amb);
			width = U8G2::getStrWidth(buff);
			U8G2::drawStr(d_width-20-width, d_height, buff);
		}
	}
	// Show degree symbol and units ('C' or 'F')
	U8G2::drawBitmap(d_width-20, d_height-12, 1, 5, bmDegree);
	U8G2::drawStr(d_width-12, d_height, sym);


	// Show the current IRON or Hot Air Gun temperature
	sprintf(buff, "%3d", t_cur);
	U8G2::setFont(u8g2_font_kam28n);
	width = U8G2::getStrWidth(buff);
	U8G2::drawStr((d_width-width+1)/2, 42, buff);

	// Show temperature bar greed
	U8G2::drawHLine(5, 51, d_width-10);
	U8G2::drawVLine(5+preset_label, 47, 6);
	// Show the temperature bar
	if (temp_bar > 3) {
		U8G2::drawBox(5, 48, temp_bar, 3);
		U8G2::drawHLine(5, 47, temp_bar-1);
	}

	// Show IRON tilt switch status
	if (tilt_iron_used) {
		U8G2::drawBitmap(0, d_height/2-4, 2, 9, bmTiltActive);
	} else if (fan_index) {
		--fan_index;
		fan_index &= 0x3;										// Can be from 0 to 3
		U8G2::drawBitmap(0, d_height/2-8, 2, 16, bmFan[fan_index]);
	}
	U8G2::sendBuffer();
}

void DSPL::scrSave(SCR_MODE mode, uint16_t t_cur, uint16_t t_alter) {
	static const char *modes[4] = {	"OFF", "IRON", "STBY", "GUN" };
	U8G2::clearBuffer();
	char buff[6];
	uint8_t height = 46;
	uint8_t width  = 0;
	U8G2::setFont(u8g_font_profont15r);
	if (t_alter > 0) {
		height += 16;
		width = U8G2::getStrWidth(modes[(uint8_t)mode]);
		U8G2::drawStr(saver_center[0]-width/2, saver_center[1]-height/2+15, modes[(uint8_t)mode]);
		sprintf(buff, "%3d", t_alter);
		width = U8G2::getStrWidth(buff);
		U8G2::drawStr(saver_center[0]-width/2, saver_center[1]+height/2, buff);
		U8G2::setFont(u8g2_font_kam28n);
		sprintf(buff, "%3d", t_cur);
		width   = U8G2::getStrWidth(buff);
		U8G2::drawStr(saver_center[0]-width/2, saver_center[1]+15, buff);
	} else {
		width = U8G2::getStrWidth(modes[(uint8_t)mode]);
		U8G2::drawStr(saver_center[0]-width/2, saver_center[1]-height/2+13, modes[(uint8_t)mode]);
		U8G2::setFont(u8g2_font_kam28n);
		sprintf(buff, "%3d", t_cur);
		width   = U8G2::getStrWidth(buff);
		U8G2::drawStr(saver_center[0]-width/2, saver_center[1]+height/2, buff);
	}
	U8G2::sendBuffer();

	// calculate new message position
	if (saver_speed[0] > 0) {
		if (saver_center[0]+width/2 >= d_width) {				// Right border of the screen
			saver_speed[0] = -1;
		}
	} else {
		if ((int)saver_center[0]-width/2 <= 0) {				// Left border of the screen
			saver_speed[0] = 1;
		}
	}
	if (saver_speed[1] > 0) {
		if (saver_center[1]+height/2 >= d_height) {				// Bottom border of the screen
			saver_speed[1] = -1;
		}
	} else {
		if ((int)saver_center[1]-height/2 <= 0) {				// Top border of the screen
			saver_speed[1] = 1;
		}
	}
	saver_center[0] += saver_speed[0];
	saver_center[1] += saver_speed[1];
}

void  DSPL::tuneShow(uint16_t tune_temp, uint16_t temp, uint8_t pwr_pcnt) {
	if (temp > 4095) temp = 4095;
	char p_buff[5];
	sprintf(p_buff, "%3d%c", pwr_pcnt, '%');
	const char *title_buff 	= "Tune";
	char mtemp_buff[6];
	sprintf(mtemp_buff, "%3d", tune_temp);
	char sym[2]			= "C";
	U8G2::setFont(u8g_font_profont15r);
	uint8_t pcnt_width = U8G2::getStrWidth(p_buff) + 5;
	uint8_t p_len		= map(pwr_pcnt, 0, 100, 0, d_width-10-pcnt_width);
	uint8_t p_height	= gauge(pwr_pcnt, 10, 20);
	uint8_t t_len		= 0;
	if (temp <= 2048) {
		t_len			= map(temp, 0, 2048, 0, 20);
	} else {
		t_len			= map(temp, 2049, 4095, 20, d_width-16);
	}
	uint8_t pos_450		= map(3600, 2049, 4095, 20, d_width-16) + 8;

	U8G2::clearBuffer();
	// Show title
	uint8_t width = U8G2::getStrWidth(title_buff);
	U8G2::drawStr((d_width-width)/2, 15, title_buff);
	// Show status message: 'ON', 'OFF', 'Idle', etc.
		width = U8G2::getStrWidth(msg_buff);
		U8G2::drawStr(d_width-5 - width, 12, msg_buff);
	// Show power applied
	U8G2::drawTriangle(5, 35, 5+p_len, 35, 5+p_len, 35-p_height);
	U8G2::drawStr(d_width-pcnt_width, 35, p_buff);
	// Show temperature bar frame
	U8G2::drawBitmap(0, d_height-15-9, 1, 10, bmTempGuageLeft);
	U8G2::drawBitmap(d_width-8, d_height-15-7, 1, 6, bmTempGuageRight);
	U8G2::drawHLine(8, d_height-15-7, d_width-16);
	U8G2::drawHLine(8, d_height-15-2, d_width-16);
	U8G2::drawVLine(pos_450, d_height-15-2, 4);
	// Show temperature row bar
	U8G2::drawHLine(8, d_height-15-5, t_len);
	if (t_len > 1) {
		U8G2::drawHLine(8, d_height-15-6, t_len);
		U8G2::drawHLine(8, d_height-15-4, t_len-1);
	}
	// Show max temperature text
	width = U8G2::getStrWidth(mtemp_buff) + 16;
	U8G2::drawBitmap(d_width-16, d_height-12, 1, 5, bmDegree);
	U8G2::drawStr(d_width-width, d_height, mtemp_buff);
	U8G2::drawStr(d_width-8, d_height, sym);
	U8G2::sendBuffer();
}

void DSPL::pidInit(void) {
	data_index 		= 0;
	full_buff		= false;
	default_mode	= 0;
}

void DSPL::pidSetLowerAxisLabel(const char *label) {
	lower_axis[0]	= label[0];
	lower_axis[1]	= label[1];
	lower_axis[2]	= '\0';
}

void DSPL::pidModify(uint8_t index, uint16_t value) {
	if (index < 3) {
		default_mode	= HAL_GetTick() + 1000;						// Show new value for 1 second
		sprintf(modified_value, k_proto[index], value);
	}
}

void DSPL::autoPidInfo(const char *message) {
	default_mode	= HAL_GetTick() + 2000;							// Show the message for 2 seconds
	for (uint8_t i = 0; i < 19; ++i) {
		if (!(modified_value[i] = message[i]))
			break;
	}
	modified_value[19] = '\0';
}

void DSPL::autoPidCurrentLoop(uint16_t loop, uint32_t period) {
	default_mode	= HAL_GetTick() + 50000;						// Show new value for 50 seconds, near forever
	sprintf(modified_value, "#%d, P=%ld.%03ds", loop, period/1000, (uint16_t)period%1000);
}

void DSPL::pidPutData(int16_t temp, uint16_t disp) {
	uint8_t	i 	= data_index;
	temp 		= constrain(temp, -500, 500);						// Limit graph value
	disp		= constrain(disp,    0, 999);

	h_temp[i]	= temp;
	h_disp[i]	= disp;
	if (++i >= 80) {
		i = 0;
		full_buff = true;
	}
	data_index	= i;
}

void DSPL::pidShowGraph(uint8_t pwr) {
	const uint8_t temp_zero = 20;									// The temperature X-axis vertical coordinate
    const uint8_t disp_zero = 63 ;									// The dispersion  X-axis vertical coordinate
    const uint8_t temp_height = 40;									// The temperature graph height
    const uint8_t disp_height = 15;									// The dispersion  graph height
	int8_t temp[80];
	int8_t disp[80];
	pwr = constrain(pwr, 0, 99);

	// Calculate the transition coefficient for the temperature
	int16_t	min_t = 32767;
	int16_t max_t = -32767;
	uint8_t till = 80;
	if (!full_buff) till = data_index;
	for (uint8_t i = 0; i < till; ++i) {
		if (min_t > h_temp[i]) min_t = h_temp[i];
		if (max_t < h_temp[i]) max_t = h_temp[i];
	}
	if (min_t < 0)		min_t *= -1;
	if (max_t < min_t)	max_t = min_t;

	// Calculate the transition coefficient for the dispersion
	uint16_t max_d = 0;
	uint16_t min_d = 32767;
	for (uint8_t i = 0; i < till; ++i) {
		if (min_d > h_disp[i]) min_d = h_disp[i];
		if (max_d < h_disp[i]) max_d = h_disp[i];
	}

	// Normalize data to be plotted
	uint8_t ii = 0;
	if (full_buff) {
		ii = data_index + 1;
		if (ii >= 80) ii = 0;
	}
	for (uint8_t i = 0; i < till; ++i) {
		if (max_t) {
			if (h_temp[ii] > 0) {
				temp[i] = temp_zero - (h_temp[ii] * temp_height / max_t);
				if (temp[i] < 1) temp[i] = 1;
			} else {
				int16_t neg = h_temp[ii] * (-1);
				temp[i] = neg * temp_height / max_t + temp_zero;
				if (temp[i] > temp_height) temp[i] = temp_height;
			}
		} else {
			temp[i] = temp_zero;
		}
		if (max_d > min_d) {
			int16_t d = (h_disp[ii] - min_d) * disp_height / (max_d - min_d);
			d = constrain(d, 0, disp_height);
			disp[i] = disp_zero - d;
		} else {
			disp[i] = disp_zero;
		}
		if (++ii >= 80) ii = 0;
	}

	char max_t_buff[8];
	if (max_t > 999) {
		max_t_buff[0] = '\0';
	} else {
		sprintf(max_t_buff, "%2d", max_t);								// The temperature amplitude
	}

	char pwr_buff[8];
	sprintf(pwr_buff, "%2d%c", pwr, '%');

	// Check for temporary data instead of dispersion graph
	bool show_disp = true;
	if (default_mode && default_mode > HAL_GetTick()) {
		show_disp = false;
	} else {
		default_mode = 0;
	}

	bool show_disp_value = false;
	int8_t last = data_index - 1;
	if (last < 0) last = 99;
	char disp_value[4];
	if (max_d <= 999) {
		sprintf(disp_value, "%3d", max_d);
		show_disp_value = true;
	}

	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	// Show the temperature graph
	U8G2::drawHLine(26, temp_zero, d_width-34);							// The temperature axis
	U8G2::drawBitmap(111, temp_zero-2, 1, 5, bmArrow);					// The axis arrow
	U8G2::drawStr(d_width-10, temp_zero-5, "t");						// The axis label
	for (uint8_t i = 1; i < till-1; ++i) {
		U8G2::drawLine(i+26, temp[i-1], i+27, temp[i]);
	}
	if (max_t_buff[0]) {
		U8G2::drawStr(0, temp_zero-5, max_t_buff);
	}
	U8G2::drawStr(0, 40, pwr_buff);

	if (show_disp) {
		if (lower_axis[0]) {
			uint8_t width = U8G2::getStrWidth(lower_axis);
			U8G2::drawStr(d_width-width-2, 60, lower_axis);				// The lower axis label
		}
		// Show the dispersion graph
		for (uint8_t i = 1; i < till-1; ++i) {
			U8G2::drawLine(i+26, disp[i-1], i+27, disp[i]);
		}
		if (show_disp_value) {
			U8G2::drawStr(0, 60, disp_value);
		}
	} else {
		U8G2::drawStr(0, 62, modified_value);
	}
	U8G2::sendBuffer();
}

void DSPL::pidShowMenu(uint16_t pid_k[3], uint8_t index) {
	static const char* title = "Tune PID";
	char buff[12];

	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	// Show title
	uint8_t width = U8G2::getStrWidth(title);
	U8G2::drawStr((d_width-width)/2, 13, title);
	U8G2::drawHLine((d_width-width)/2, 15, width);
	// Show the Coefficient values
	for (uint8_t i = 0; i < 3; ++i) {
		sprintf(buff, k_proto[i], pid_k[i]);
		U8G2::drawStr(20, 28+i*13, buff);
		if (index == i) {
			U8G2::drawBitmap(0, 20+i*13, 1, 7, bmLeftMark);
		}
	}
	U8G2::sendBuffer();
}

void DSPL::calibShow(const char* tip_name, uint8_t ref_point, uint16_t current_temp, uint16_t real_temp, bool celsius,
		uint8_t power, bool on, bool ready, uint8_t int_temp_pcnt) {
	static const char* title 	= "Tip:";
	static const char* OFF		= "OFF";
	static const char* ON  		= "ON";
	static char sym[] = {'C', '\0'};

	if (celsius)
		sym[0] = 'C';
	else
		sym[0] = 'F';
	char ref_buff[16];
	sprintf(ref_buff, "Ref# %d", ref_point);

	uint8_t p_height = gauge(power, 10, 45);						// Applied power triangle height

	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	// Show title
	uint8_t width		= U8G2::getStrWidth(title);
	uint8_t width_tip	= U8G2::getStrWidth(tip_name);
	uint8_t total 		= width+width_tip+5;
	int8_t  start		= (d_width-total)/2;
	if (start < 0) start = 0;
	U8G2::drawStr(start, 13, title);
	U8G2::drawStr(start+width+5, 13, tip_name);
	U8G2::drawHLine(5, 15, d_width-10);
	// Show reference point number
	width = U8G2::getStrWidth(ref_buff);
	U8G2::drawStr(5, 33, ref_buff);
	// Show current temperature
	char temp_buff[10];
	sprintf(temp_buff, "%3d", current_temp);
	U8G2::drawBitmap(5, 57-15, 1, 15, bmTemperature);
	width = U8G2::getStrWidth(temp_buff);
	U8G2::drawStr(16, 57, temp_buff);
	U8G2::drawBitmap(16+1+width, 57-12, 1, 5, bmDegree);
	U8G2::drawStr(16+1+8+width, 57, sym);
	if (ready) {
		// Show real temperature
		U8G2::drawBitmap(70, 57-7, 1, 7, bmLeftMark);
		sprintf(temp_buff, "%3d", real_temp);
		U8G2::drawStr(80, 57, temp_buff);
	}
	// Show the power applied
	if (p_height > 0) {
		U8G2::drawTriangle(d_width-5, 63, d_width-5, 63-p_height, d_width-6-p_height/4, 63-p_height);
	}
	if ((p_height < 28) && on) {
		width = U8G2::getStrWidth(ON);
		U8G2::drawStr(d_width-width-5, 33, ON);
	}
	if (!on) {
		width = U8G2::getStrWidth(OFF);
		U8G2::drawStr(d_width-width-5, 33, OFF);
	}
	// Show internal temperature bar
	int_temp_pcnt = constrain(int_temp_pcnt, 0, 100);
	if (int_temp_pcnt > 1) {
		U8G2::drawBox(14, 62, int_temp_pcnt, 2);
	}
	U8G2::sendBuffer();
}

//---------------------- The Calibration display function ------------------------
void DSPL::calibManualShow(const char* tip_name, uint16_t ref_temp, uint16_t current_temp,
								uint16_t setup_temp, bool celsius, uint8_t power, bool on, bool ready) {
	static const char* title 	= "Tip:";
	static const char* OFF		= "OFF";
	static const char* ON  		= "ON";
	static const char* OK  		= "OK";
	static char sym[] = {'C', '\0'};
	static const uint16_t detail_area = 30;

	if (celsius)
		sym[0] = 'C';
	else
		sym[0] = 'F';
	char ref_buff[16];
	sprintf(ref_buff, "Set: %3d", ref_temp);

	uint8_t p_height = 0;										  		// Applied power triangle height
	if (power <= 10)
		p_height = power;
	else
		p_height = map(power-10, 0, 90, 10, 45);

	// calculate the length of the current temperature row bar
	const uint16_t setup_temp_pos = d_width-32-detail_area;
	uint16_t t_pos = 0;
	if (current_temp >= setup_temp) {
		uint16_t t_diff = constrain((current_temp - setup_temp)/8, 0, detail_area);
		t_pos = setup_temp_pos + t_diff;
	} else {
		uint16_t t_diff = (setup_temp - current_temp)/8;
		if (t_diff <= detail_area) {
			t_pos = setup_temp_pos - t_diff;
		} else {
			t_diff -= detail_area;
			t_pos   = setup_temp_pos - detail_area - constrain(t_diff / 8, 0, setup_temp_pos-detail_area-8);
		}
	}

	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	// Show title
	uint8_t width 		= U8G2::getStrWidth(title);
	uint8_t width_tip	= U8G2::getStrWidth(tip_name);
	uint8_t total 		= width+width_tip+5;
	int8_t  start = (d_width-total)/2;
	if (start < 0) start = 0;
	U8G2::drawStr(start, 13, title);
	U8G2::drawStr(start+width+5, 13, tip_name);
	U8G2::drawHLine(5, 15, d_width-10);
	// Show reference temperature
	width = U8G2::drawStr(5, 33, ref_buff);
	U8G2::drawBitmap(5+width, 33-12, 1, 5, bmDegree);
	U8G2::drawStr(5+8+width, 33, sym);
	if (ready) {
		// Show ready sign
		width = U8G2::getStrWidth(OK);
		U8G2::drawStr(d_width-width-5, 33, OK);
	}
	// Show temperature bar frame
	U8G2::drawBitmap(0, d_height-10-9, 1, 10, bmTempGuageLeft);
	U8G2::drawBitmap(d_width-24, d_height-10-7, 1, 6, bmTempGuageRight);
	U8G2::drawHLine(8, d_height-10-7, d_width-32);
	U8G2::drawHLine(8, d_height-10-2, d_width-32);
	// Show setup temperature mark
	U8G2::drawVLine(setup_temp_pos, d_height-10-2, 4);
	U8G2::drawVLine(setup_temp_pos, d_height-10-7-4, 4);
	// Show temperature row bar
	if (t_pos > 10) {
		U8G2::drawHLine(8, d_height-10-5, t_pos-9);
		U8G2::drawHLine(8, d_height-10-6, t_pos-9);
		U8G2::drawHLine(8, d_height-10-4, t_pos-10);
	}

	// Show the power applied
	if (p_height > 0) {
		U8G2::drawTriangle(d_width-5, 63, d_width-5, 63-p_height, d_width-6-p_height/4, 63-p_height);
	}
	if ((p_height < 28) && on) {
		width = U8G2::getStrWidth(ON);
		U8G2::drawStr(d_width-width-5, 33, ON);
	}
	if (!on) {
		width = U8G2::getStrWidth(OFF);
		U8G2::drawStr(d_width-width-5, 33, OFF);
	}
	U8G2::sendBuffer();
}

//---------------------- The Menu list display functions -------------------------
void DSPL::tipListShow(const char* title,  TIP_ITEM list[], uint8_t list_len, uint8_t index, bool name_only) {
	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	// Show title
	uint8_t width = U8G2::getStrWidth(title);
	U8G2::drawStr((d_width-width)/2, 13, title);
	U8G2::drawHLine((d_width-width)/2, 15, width);
	uint8_t left_mark_pos = 0;
	if (name_only) left_mark_pos = 15;
	// Show the tip list
	for (uint8_t i = 0; i <  list_len; ++i) {
		if (list[i].name[0]) {
			if (list[i].tip_index == index)
				U8G2::drawBitmap(left_mark_pos, 20+i*13, 1, 7, bmLeftMark);
			U8G2::drawStr(24, 28+i*13, list[i].name);
			if (!(list[i].mask & 2)) {									// The tip is not calibrated
				U8G2::drawBitmap(100, 19+i*13, 1, 9, bmNotCalibrated);
			}
			if (!name_only) {
				if (list[i].mask & 1) {									// The tip is active
					U8G2::drawBitmap(12, 20+i*13, 1, 8, bmCheckFull);
				} else {
					U8G2::drawBitmap(12, 20+i*13, 1, 8, bmCheckEmpty);
				}
			}
		}
	}
	U8G2::sendBuffer();
}

void DSPL::menuItemShow(const char* title, const char* item, const char* value, bool modify) {
	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	// Show title
	uint8_t width = U8G2::getStrWidth(title);
	U8G2::drawStr((d_width-width)/2, 13, title);
	U8G2::drawHLine((d_width-width)/2, 15, width);
	// Show the menu item
	width = U8G2::getStrWidth(item);
	uint8_t v_width	= U8G2::getStrWidth(value);
	if (value && value[0]) {											// Show in-place changed menu item
		U8G2::drawStr(10, 45, item);
		if (!modify) {
			if ((width+v_width+25) < d_width) {							// The item name and its value fits single line
				U8G2::drawStr(d_width-v_width-15, 45, value);
			} else {													// The value of the parameter is shown on the next line
				U8G2::drawStr(d_width-v_width, 60, value);
			}
		} else {
			U8G2::drawStr((d_width-v_width)/2, 60, value);
			U8G2::drawStr((d_width-v_width)/2-10, 60, "[");
			U8G2::drawStr((d_width+v_width)/2+2, 60, "]");
		}
	} else {
		U8G2::drawStr((d_width-width)/2, 45, item);
	}
	U8G2::sendBuffer();
}

void DSPL::errorShow(void) {
	U8G2::clearBuffer();
	if (err_msg[0] == '\0') {											// No error message specified, show big "ERROR"
		U8G2::drawBitmap(10, 20, 2, 23, E);
		U8G2::drawBitmap(10+18, 20, 3, 23, R);
		U8G2::drawBitmap(10+18+20, 20, 3, 23, R);
		U8G2::drawBitmap(10+18+20+20, 20, 3, 23, O);
		U8G2::drawBitmap(10+18+20+20+26, 20, 3, 23, R);
	} else {
		U8G2::setFont(u8g_font_profont15r);
		uint8_t len = strlen(err_msg);
		uint8_t line = 1;
		for (uint8_t start = 0; start < len; ) {
			uint8_t finish = start+1;
			while (++finish < len) {
				if (err_msg[finish] == '\n')
					break;
			}
			bool not_end = (finish < len);
			if (not_end)
				err_msg[finish] = '\0';
			uint8_t width = U8G2::getStrWidth(&err_msg[start]);
			if (width < d_width) {
				U8G2::drawStr((d_width-width)/2, line*13, &err_msg[start]);
			} else {
				U8G2::drawStr(0, line*13, &err_msg[start]);
			}
			if (++line > 4)												// Only 4 lines display cat fit
				break;
			if (not_end)
				err_msg[finish] = '\n';
			start = finish + 1;
		}
	}
	U8G2::sendBuffer();
}

void DSPL::errorMessage(const char *msg) {
	if (msg[0]) {
		strncpy(err_msg, msg, 40);
	} else {
		err_msg[0] = '\0';
	}
}

void DSPL::debugShow(bool gun_mode, uint16_t power, bool iron, bool gun, uint16_t data[4]) {
	char buff[14];
	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	if (gun_mode) U8G2::drawBitmap(0, 0, 2, 16, bmFan[0]);
	sprintf(buff, "%5d", power);
	U8G2::drawStr(0,  30, buff);
	for (uint8_t i = 0; i < 4; ++i) {
		sprintf(buff, "%5d", data[i]);
		U8G2::drawStr(60,  15*(i+1), buff);
	}
	sprintf(buff, "(%c-%c)", iron?'i':' ', gun?'g':' ');
	U8G2::drawStr(5,  62, buff);
	U8G2::sendBuffer();
}

void DSPL::showVersion(void) {
	static const char *title = "About";
	char buff[30];
	U8G2::setFont(u8g_font_profont15r);
	U8G2::clearBuffer();
	// Show title
	uint8_t width	= U8G2::getStrWidth(title);
	U8G2::drawStr((d_width-width)/2, 13, title);
	U8G2::drawHLine((d_width-width)/2, 15, width);
	// Show title
	sprintf(buff, "IRON & Hot Air Gun");
	width	= U8G2::getStrWidth(buff);
	U8G2::drawStr((d_width-width)/2, 30, buff);
	// Show software version
	sprintf(buff, "Controller v.%s", FW_VERSION);
	width	= U8G2::getStrWidth(buff);
	U8G2::drawStr((d_width-width)/2, 45, buff);
	// Print date of compilation
	sprintf(buff, "%s", __DATE__);
	width	= U8G2::getStrWidth(buff);
	U8G2::drawStr((d_width-width)/2, 63, buff);
	U8G2::sendBuffer();
}
