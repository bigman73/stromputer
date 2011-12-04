#ifndef LCDi2cNHD_h
#define LCDi2cNHD_h


#define _LCDEXPANDED				// If defined turn on advanced functions

#include <inttypes.h>

#include "Print.h"


class LCDi2cNHD : public Print {
	
public: 
	
LCDi2cNHD(uint8_t num_lines, uint8_t num_col, int i2c_address, uint8_t display);
	
	void command(uint8_t value);
	
	void init();
	
	void setDelay(int,int);
	
	virtual void write(uint8_t);
	
	void clear();
	
	void home();
	
	void on();
	
	void off();
	
	void cursor_on();
	
	void cursor_off();
	
	void blink_on();
	
	void blink_off();
	
	void setCursor(uint8_t Line, uint8_t Col );
	
	
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	// []
	// []	Extended Functions
	// []
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	
	
#ifdef _LCDEXPANDED		

	
	uint8_t status();
	
	void load_custom_character(uint8_t char_num, uint8_t *rows);
	
	 void setBacklight(uint8_t new_val);
	
	 void setContrast(uint8_t new_val);
	 
		
#endif
	
private:
	
	
};

#endif

