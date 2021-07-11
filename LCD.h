#ifndef LCD_H
#define	LCD_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <xc.h>

#define LCD_RS PORTEbits.RE2
#define LCD_EN PORTEbits.RE1
#define LCD_PORT PORTD
#define LCD_PULSE_TIME 1

#define LCD_FS_1LINE 0x30
#define LCD_FS_2LINE 0x38
#define ROW1 0x80
#define ROW2 0xC0
#define LCD_OFF 0x08
#define LCD_ON  0x0C
#define LCD_CURSOR_ON 0x0A
#define LCD_CURSOR_BLINK 0x09
#define LCD_ENTRY_FORWARD 0x06
#define LCD_ENTRY_REVERSE 0x04
#define LCD_ENTRY_SLIDE 0x05
#define LCD_CLEAR 0x01
#define LCD_CURSOR_MOVE_RIGHT 0x14
#define LCD_CURSOR_MOVE_LEFT 0x10
#define LCD_CONTENT_MOVE_RIGHT 0x1C
#define LCD_CONTENT_MOVE_LEFT 0x18
  
typedef unsigned char byte;
byte lcd_x = 1, lcd_y = 1;
  
void InitLCD(void);
void LCDCmd(unsigned char cmd);
void LCDDat(unsigned char dat);
void LCDStr(const char* str);


void LCDCmd(unsigned char cmd) {
  LCD_EN = 0;
  LCD_RS = 0;
  unsigned char temp_for_portd = PORTD;
  LCD_PORT = cmd;
  LCD_EN = 1;
  __delay_us(LCD_PULSE_TIME);
  LCD_EN = 0;
  PORTD = temp_for_portd;
}

void LCDDat(unsigned char dat) {
  LCD_EN = 0;
  LCD_RS = 1;
  unsigned char temp_for_portd = PORTD;
  LCD_PORT = dat;
  LCD_EN = 1;
  __delay_us(LCD_PULSE_TIME);
  LCD_EN = 0;
  LCD_RS = 0;
  PORTD = temp_for_portd;
}


void LCDGoto(byte p_2, byte p_1) {
  if(p_1==1) {
    LCDCmd(ROW1+((p_2-1)%16));
  } else {
    LCDCmd(ROW2+((p_2-1)%16));
  }
  lcd_x = p_2;
  lcd_y = p_1;
}


void LCDStr(const char* str) {
  for (unsigned char i = 0; str[i] != 0; i++) {
    LCDDat(str[i]);
    lcd_x ++;
    if(lcd_x == 17) {
      lcd_x = 1;
      lcd_y++;
      if (lcd_y == 3) {
         lcd_y = 1;
      }
      LCDGoto(lcd_x, lcd_y);
    }
  }
}


void InitLCD(void) {
  TRISEbits.RE1 = 0;
  TRISEbits.RE2 = 0;
  TRISD = 0;
  
  
  LCD_EN = 0;
  LCD_RS = 0;
  LCDCmd(LCD_FS_2LINE);
  LCDCmd(LCD_OFF);
  LCDCmd(LCD_ON);
  LCDCmd(LCD_ENTRY_FORWARD);
  LCDCmd(LCD_CLEAR);
  LCDCmd(ROW1);
}




void LCDAddSpecialCharacter(byte character_index, byte * data) {
  LCDCmd(0b01000000+(character_index*8));
  for(byte i=0; i < 8; i++) {
    LCDDat(data[i]);
  }
  LCDGoto(lcd_x, lcd_y);
    
}
#ifdef	__cplusplus
}
#endif

#endif	/* LCD_H */