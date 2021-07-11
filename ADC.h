#ifndef ADC_H
#define	ADC_H

#ifdef	__cplusplus
extern "C" {
#endif

void initADC();

    void readADCChannel(unsigned char channel)
    {
        ADCON0bits.CHS0 =  channel & 0x1; 
        ADCON0bits.CHS1 = (channel >> 1) & 0x1;
        ADCON0bits.CHS2 = (channel >> 2) & 0x1;
        ADCON0bits.CHS3 = (channel >> 3) & 0x1;
        ADCON0bits.GODONE = 1;
        return;
    }



    // Works in 40 mHZ
    void initADC()
    {
        ADCON1bits.PCFG3=1; 
        ADCON1bits.PCFG2=1;
        ADCON1bits.PCFG1=0;
        ADCON1bits.PCFG0=0;
        
        ADCON1bits.VCFG0 =0; 
        ADCON1bits.VCFG1=0;
        
        TRISAbits.RA0 = 1;
        TRISAbits.RA1 = 1;
        TRISAbits.RA2 = 1;
        
        ADCON2bits.ADCS2 = 1; 
        ADCON2bits.ADCS1 = 1;
        ADCON2bits.ADCS0 = 0;
        
        ADCON2bits.ACQT2 = 0; 
        ADCON2bits.ACQT1 = 0;
        ADCON2bits.ACQT0 = 1;
        
        ADCON2bits.ADFM = 1; 
        ADCON0bits.ADON = 1; 
           
    } 

    
#ifdef	__cplusplus
}
#endif

#endif	/* ADC_H */