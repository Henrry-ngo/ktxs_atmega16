#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#define LCD_Dir	DDRC				//Xac dinh cong giao tiep LCD
#define LCD_Port	PORTC				//Xac dinh cong du lieu LCD
#define RS			PINC0		//Dinh nghia kenh RS
#define RW			PINC1 		//Ding nghia kenh EN
#define EN			PINC2
#define degree_sysmbol		0xDF		//Bieu duong do
#define cbi(port, bit)		port &= ~(1<<bit)
#define sbi(port, bit)		port |= (1<<bit)

char Temperature[20];
float celsius;
char tx_message[10];

void LCD_Command( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0);		//gui nibble cao
	LCD_Port &= ~ (1<<RS);				//RS = 0  su dung thanh ghi IR
	LCD_Port &= ~ (1<<RW);
	LCD_Port |= (1<<EN);				//EN = 1 Kich hoat xung
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_us(200);
	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);		//Gui nibble thap
	LCD_Port &= ~ (1<<RW);
	LCD_Port |= (1<<EN);				//EN = 1 Kich hoat xung
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}


void LCD_Char( unsigned char data )
{
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0);		//Gui nibble cao
	LCD_Port |= (1<<RS);
	LCD_Port &= ~ (1<<RW);				//Rs = 1 : thanh ghi DR
	LCD_Port|= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_us(200);
	LCD_Port = (LCD_Port & 0x0F) | (data << 4);				// Gui nibble thap
	LCD_Port &= ~ (1<<RW);
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}
void LCD_Init (void)			//Ham khoi tao LCD
{
	LCD_Dir = 0xFF;		//output
	_delay_ms(20);		//Tao tre bat LCD luon> 15ms
	LCD_Command(0x02);	//khoi tao lcd 4 bit
	LCD_Command(0x28);          //2 dong, ma tran 5 * 7 o che ?o 4 bit
	LCD_Command(0x0c);          //Bat man hinh, tat con tro
	LCD_Command(0x06);          //Con tro tang, dich chuyen con tro sang phai
	LCD_Command(0x01);          //Xoa man hinh
	_delay_ms(2);
}


void LCD_String (char *str)			//Ham gui chuoi
{
	int i;
	for(i=0;str[i]!=0;i++)			//Gui tung ki tu cho den ki tu Null
	{
		LCD_Char (str[i]);
	}
}

void LCD_String_xy (char row, char pos, char *str)		// Ham gui chuoi toi vi tri xy
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);		//lenh cua hang dau tien va vi tri bat buoc <16
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);		//lenh cua hang thu hai va vi tri bat buoc <16
	LCD_String(str);				//Goi ham
}

void LCD_Clear()
{
	LCD_Command (0x01);		//xoa man hinh
	_delay_ms(2);
	LCD_Command (0x80);		//dua con tro ve vi tri ban dau
}

void ADC_Init()
{	
	ADMUX = 0x40;	// Can le phai, dien ap tham chieu = Avcc,
			//chon kenh dau vao chuyen doi ADC0
	ADCSRA = 0x83;          //Bat ADC, bo Chia /8
}

int ADC_Read()
{
	ADMUX = 0x40;		        // Can le phai, dien ap tham chieu = Avcc,
				       			//chon kenh dau vao chuyen doi ADC0
	ADCSRA |= 0x43;		        // ADCSC = 1, bat dau chuyen doi, gia tri chia tan = 8
	while(!(ADCSRA & 0x10));	// Kiem tra chuyen doi ket thuc chua
	ADCSRA |= 0x10;		// Xoa co ngat bang cach ghi 1 toi bit ADIF
	
	return ADCW;
}

void UART_Init()			//ham khoi tao UART
{
	UBRRH=0;
	UBRRL=51;		//Cai dat toc do Baud la 9600 voi tan so 8Mhz
	UCSRA=0x00;		//Khoi tao su dung UART

	UCSRC=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);			// Set thanh ghi UCSRC, khong chan le, 8 bit du lieu, 1 bit stop
	UCSRB|=(1<<RXEN)|(1<<TXEN)|(1<<RXCIE);	//Khai bao truyen du lieu, nhan du lieu va ngat khi nhan
}

void UART_TxChar(unsigned char ch)		//Ham truyen ky tu UART
{

	while (! (UCSRA & (1<<UDRE)));	//Cho cho den khi bit UDRE = 0
	UDR = ch;
}

void UART_TxString(char* str)				//Hien thi
{
	for(int i = 0; str[i] != 0; i++)
	{
		UART_TxChar(str[i]);	//Xuat ra mang can truyen
	}
}

unsigned char UART_RxChar()
{
	while(! (UCSRA & (1<<RXC)));
	return UDR;
}

int main()
{
	DDRA = 0x00;
	DDRB = 0xFF;
	DDRD = 0xFD;
	
	TCCR0 |=(1<<WGM00)|(1<<COM01)|(1<<COM00)|(1<<CS00)|(1<<CS02);  	// su dung phase correct PWM, inverted PWM,  bo chia 1024
	TIMSK=0;
	OCR0=255;		//Duty cycle = 0
	LCD_Init();		//Khoi tao LCD
	ADC_Init();		//Khoi tao ADC
	UART_Init();
	while(1)
	{
		LCD_String_xy(0, 0,"NHIET DO:");
		celsius = (ADC_Read()/2.046);
		sprintf(Temperature,"%d%cC  ", (int)celsius, degree_sysmbol);	//Chuyen doi gia tri so nguyen thanh chuoi ASCII
		LCD_String_xy(0, 9,Temperature);		//gui du lieu chuoi de in
		_delay_ms(1000);
		memset(Temperature,0,10);			//sao chep chuoi de inxs
		if(celsius < 25)
		{
			cbi(PORTB,0);	// Tat den 0
			cbi(PORTB,1);	// Tat den 1
			cbi(PORTB,2);	// Tat den 2
			OCR0=255;		// khong cho quat quay
			cbi(PORTA,7);	// Buzzer tat
			LCD_String_xy(1, 0,"NHIET DO ON DINH");
		}
		if(celsius >= 30 && celsius < 35)
		{
			sbi(PORTB,1);	// Bat den 1
			cbi(PORTB,0);	// Tat den 0
			cbi(PORTB,2);	// Tat den 2
			OCR0=76;		// Cho quat quay o cap do 2
			cbi(PORTA,7);	// Buzzer tat
			LCD_String_xy(1, 0,"CAP DO ");
			LCD_String_xy(1, 7," 2: 70 % ");
		}
		if(celsius >= 35)
		{
			sbi(PORTB,2);	// Bat den 2
			cbi(PORTB,0);	// Tat den 0
			cbi(PORTB,1);	// Tat den 1
			OCR0=0;			// Cho quat quay o cap do 3
			sbi(PORTA,7);	// Buzzer keu
			LCD_String_xy(1, 0,"CAP DO ");
			LCD_String_xy(1, 7," 3: 100 %");
		}
		sprintf(tx_message, "%d\r\n", (int)celsius);
		_delay_ms(0.1);
		UART_TxString(tx_message);
	} 
}
