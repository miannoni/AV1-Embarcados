/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 

#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"

#define BOTAO_PIO      PIOA
#define BOTAO_PIO_ID   ID_PIOA
#define BOTAO_IDX  4
#define BOTAO_IDX_MASK (1 << BOTAO_IDX)

volatile Bool flag_do_btn;
volatile Bool f_rtt_alarme = false;
volatile int tempo = 0;
volatile int distancia = 0;

struct ili9488_opt_t g_ili9488_display_opt;

void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}


void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}	
}

void callbk_btn(void){
	flag_do_btn = true;
}

void RTT_Handler(void)
{
	uint32_t ul_status;
	
	ul_status = rtt_get_status(RTT);
	
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }

	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		f_rtt_alarme = true;
	}
}

void io_init(void)
{
	pmc_enable_periph_clk(BOTAO_PIO_ID);
	pio_configure(BOTAO_PIO, PIO_INPUT, BOTAO_IDX_MASK, PIO_PULLUP);
	pio_handler_set(BOTAO_PIO, BOTAO_PIO_ID, BOTAO_IDX_MASK, PIO_IT_RISE_EDGE, callbk_btn);
	pio_enable_interrupt(BOTAO_PIO, BOTAO_IDX_MASK);
	NVIC_EnableIRQ(BOTAO_PIO_ID);
	NVIC_SetPriority(BOTAO_PIO_ID, 4);
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}


int main(void) {
	board_init();
	sysclk_init();	
	configure_lcd();
	io_init();
	char buffer[32];
	flag_do_btn = false;
	f_rtt_alarme = true;
	int deltadist = 0;
	int velocidade = 0;
	float perimetro = 0.65*3.141592653;
	
	while(1) {
		if (flag_do_btn == true) {
			distancia += perimetro;
			deltadist += perimetro;
			flag_do_btn = false;
		}
		if (f_rtt_alarme){

			uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
			uint32_t irqRTTvalue  = 2;
			RTT_init(pllPreScale, irqRTTvalue);
			
			tempo += irqRTTvalue/2;
			
			if (tempo%4 == 0) {
				velocidade = ((deltadist/4)*3.6);
				
				ili9488_draw_filled_rectangle(55, 50, ILI9488_LCD_WIDTH-1-5, 200);
			
				if (velocidade != 0) {
					sprintf(buffer, "Vel: %d km/h", velocidade);
				} else {
					sprintf(buffer, "Vel: 0 km/h");
				}
				font_draw_text(&calibri_36, buffer, 55, 50, 1);
				
				sprintf(buffer, "Dist.: %d m", distancia);
				font_draw_text(&calibri_36, buffer, 30, 100, 1);
				
				deltadist = 0;
							
			}
			
			if (tempo <= 1) {
				sprintf(buffer, "Vel: 0 km/h");
				font_draw_text(&calibri_36, buffer, 55, 50, 1);
				
				sprintf(buffer, "Dist.: 0 m");
				font_draw_text(&calibri_36, buffer, 30, 100, 1);
			}
			
			sprintf(buffer, "Tmp: %02d:%02d:%02d", (tempo/3600)%24, (tempo/60)%60, tempo%60);
			font_draw_text(&calibri_36, buffer, 30, 150, 1);
			
			f_rtt_alarme = false;
		}
	}
}