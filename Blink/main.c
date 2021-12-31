#define  F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>

volatile uint16_t n = 0;
volatile uint16_t sine_freq = 0;

ISR(TIMER2_OVF_vect)
{
	n = n + 1;
}
ISR(ADC_vect)
{
	sine_freq = ADC;
}


int main(void)
{
	sei();
	//setup PWM pin5, pin6
	DDRD |= ((1<<DDD6) | (1<<DDD5));
	
	//enable compare output mode channel A, Comc channel B, mode 3 (Fast PWM - Top = 0xFF)
	TCCR0A |= ((1<<COM0A1) | (1<<COM0B1) | (1<<WGM00));
	//no prescaling - max PWM frequency (T_pwm = 16us)
	TCCR0B |= (1<<CS00);
	
	//adc aref from AVcc,  ADC source fro ADC0 (PC0)
	ADMUX |= (1<<REFS0);
	//enable ADC, ADC - autotriger => free running, ADC inerupt enable, ADC prescaler (division factor = 128)
	ADCSRA|=((1<<ADEN)|(1<<ADATE)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));
	//start conversion
	ADCSRA|=(1<<ADSC);
	
	//timer 2 no prescaler
	TCCR2B |= ((1 << CS20));
	//enable interupt (overflow)
	TIMSK2 |= (1 << TOIE2);


	//  Serial.begin(2000000);
	uint16_t SAMPLE_RATE = 64;
	double MAX_FREQUENCY = 490;
	//double MAX_FREQUENCY = 976.5625;
	int positive = 1;
	uint8_t PWM_val = 0;
	double f ;
	double TTI = 0;
	uint8_t timer_read = 0;
	double t;
	double U;
	uint8_t k = 1;
	uint16_t old_fr = sine_freq;
	while (1)
	{
		if(((old_fr-5)>=(sine_freq)) || ((sine_freq)>=(old_fr+5)))
		{
			old_fr = sine_freq;
		}
		old_fr = old_fr <=1 ? 1 : old_fr;
		f = (MAX_FREQUENCY * old_fr) / 1023;
		TTI = 1 / (2 * SAMPLE_RATE * f);
		timer_read = TCNT2;

		t = (n * 16e-6) + (timer_read * 62.5e-9);
		if (t >= (1/f))
		{
			n = 0;
			TCNT2 = 0;
			k = 0;
		}
		if (t>=(k*TTI))
		{
			k+=1;
			
			U = sin(2 * M_PI * f * t);
			positive = U >= 0 ? 1 : 0;
			
			U = fabs(U);
			PWM_val = (uint8_t)(U * 255);

			if (positive)
			{
				//      Serial.println("Positive");
				//      Serial.println(PWM_val);
				//      Serial.println();
				OCR0A = PWM_val;
				OCR0B = 0;
			}
			else
			{
				//      Serial.println("Negative");
				//      Serial.println(PWM_val);
				//      Serial.println();
				OCR0A = 0;
				OCR0B = PWM_val;
			}
		}
		
	}
}