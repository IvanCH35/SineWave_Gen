#define  F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>

volatile uint16_t n = 0;
volatile uint16_t adc_val = 0;

ISR(TIMER2_OVF_vect)
{
  n = n + 1;
}
ISR(ADC_vect)
{
  adc_val = ADC;
}


int main(void)
{
  sei();
  //setup output pins
  DDRD |= ((1<<DDD6) | (1<<DDD5));
  
  //compare output mode channel A, 
  //compare output mode channel B,
  //Phase correct PWM mode - Top = 0xFF
  TCCR0A |= ((1<<COM0A1) | (1<<COM0B1) | (1<<WGM00));
  
  /*
  //compare output mode channel A,
  //compare output mode channel B,
  //Phase correct PWM mode - Top = 0xFF
  //Fast PWM mode - Top = 0xFF
  TCCR0A |= ((1<<COM0A1) | (1<<COM0B1) | (1<<WGM01) | (1<<WGM00));
  */
  
  //no prescaling - max PWM frequency (T_pwm = 16us [Fast PWM], T_pwm = 31.875u [Phase correct PWM])
  TCCR0B |= (1<<CS00);
  
  
  //adc aref from AVcc,  ADC source fro ADC0 (PC0)
  ADMUX |= (1<<REFS0);
  //enable ADC, ADC - autotriger => free running, ADC inerupt enable, ADC prescaler (division factor = 128 --> f = 16M/128 = 125k -> 50k <= 125k <= 200k)
  ADCSRA|=((1<<ADEN)|(1<<ADATE)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));
  //start conversion
  ADCSRA|=(1<<ADSC);
  
//Change to timer 1
  //timer 2 no prescaler
  TCCR2B |= ((1 << CS20));
  //enable interupt (overflow)
  TIMSK2 |= (1 << TOIE2);


  uint16_t SAMPLE_RATE = 64;
  //max frekvnecia zavisla od frekvencie PWM a poctu vzoriek
  //pre Phase correct PWM:
    //SAMPLE_RATE = 64 ==> MAX_FREQUENCY = 245Hz
    //SAMPLE_RATE = 32 ==> MAX_FREQUENCY = 490Hz
  //pre Fast PWM:
    //SAMPLE_RATE = 64 ==> MAX_FREQUENCY = 480Hz
    //SAMPLE_RATE = 32 ==> MAX_FREQUENCY = 975Hz
  double MAX_FREQUENCY = 490;
  
  //polarita polvlny: 1 - "kladna", 0 - "zaporna"
  uint8_t positive = 1;
  
  uint8_t PWM_width = 0;
  uint8_t timer_read = 0;
  
  //cislo aktualnej vzorky
  uint8_t k = 1;
  
  double t;
  double U;
  double sine_frequency;
  
  //Time To Interrupt
  double TTI = 0;
  
  uint16_t old_adc_val = adc_val;
  while (1)
  {
    //Hystereza na ADC hodnotu (hodnota sa meni +/- 1), dostaneme ustelenejsiu frekvenciu
    if(((old_adc_val-2)>=(adc_val)) || ((adc_val)>=(old_adc_val+2)))
    {
      old_adc_val = adc_val;
    }
    
    //Minimalna frekvencia generovaneho sinus priebehu je nenulova
    old_adc_val = old_adc_val <1 ? 1 : old_adc_val;
    
    //Minimalna hodnota frekvencie generovaneho sinus priebehu: f = MAX_FREQUENCY/1023 = cca 0.5Hz
    sine_frequency = (MAX_FREQUENCY * old_adc_val) / 1023;
    
    //Time to interrupt, kazda vzorka ma ronvaky cas, jedna polvlna rozdelena na 64 vzoriek ==> TTI
    TTI = 1 / (2 * SAMPLE_RATE * sine_frequency);
    
    //citanie casovaca (aktualny cas)
    timer_read = TCNT2;

    //vypocet casu
    t = (n * 16e-6) + (timer_read * 62.5e-9);
    
    //vypocitany cas zacina od 0 pre 1 periodu
    if (t >= (1/sine_frequency))
    {
      n = 0;
      TCNT2 = 0;
      k = 0;
    }
    
    //ak t%TTI == 0, cas priebehu sa rovna casu vzorkovania vypocet U, nastavenie PWM
    if (t>=(k*TTI))
    {
      k+=1;
      
      U = sin(2 * M_PI * sine_frequency * t);
      positive = U >= 0 ? 1 : 0;
      
      U = fabs(U);
      PWM_width = (uint8_t)(U * 255);

      if (positive)
      {
        OCR0A = PWM_width;
        OCR0B = 0;
      }
      else
      {
        OCR0A = 0;
        OCR0B = PWM_width;
      }
    }
    
  }
}