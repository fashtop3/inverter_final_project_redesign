#ifndef WDT_RST
#define WDT_RST

void WDT_rst(void);

void WDT_rst(void)
{
    WDTCSR = 0x18;
    WDTCSR = 0x08;
    while(1);
}

#endif
