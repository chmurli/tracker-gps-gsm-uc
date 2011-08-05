//////////////////////////////////////////////////////////////////////////////////////////////////
// makra.h - makra pomocnicze 
// 
// 
//////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __MAKRA_H_INCLUDED
#define __MAKRA_H_INCLUDED


// Makra upraszczające dostęp do portów
// *** Port
#define PORT(x) XPORT(x)
#define XPORT(x) (PORT##x)
// *** Pin
#define PIN(x) XPIN(x)
#define XPIN(x) (PIN##x)
// *** DDR
#define DDR(x) XDDR(x)
#define XDDR(x) (DDR##x)

// NOPek
#define NOP() {asm volatile("nop"::);}

// Ilość elementów tablicy
#define ELEMS(p) (sizeof(p)/sizeof(*p))		// lub (sizeof(p)/sizeof(p[0]))


#endif // __MAKRA_H_INCLUDED
