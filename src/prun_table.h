#ifndef PRUN_TAB_H
#define PRUN_TAB_H
//G0=<U  D  L  R  F  B >  4.33·10^19  2,048 (2^11)
extern const uint8_t G0_tab[1024];
//G1=<U  D  L  R  F2 B2>  2.11·10^16  1,082,565 (12C4 ·3^7)
extern const uint8_t G1_tab[541283];
//G2=<U  D  L2 R2 F2 B2>  1.95·10^10  29,400 ( 8C4^2 ·2·3)
extern const uint8_t G2_tab[14700];
//G3=<U2 D2 L2 R2 F2 B2>  6.63·10^5   663,552 (4!^5/12)
extern const uint8_t G3_tab[331776];

#endif
