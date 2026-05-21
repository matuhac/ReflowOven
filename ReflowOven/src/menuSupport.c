#include "menuSupport.h"


//definovani stringu
char *noText = "-----";

//INTRO MENU
const char *introLine_1 = "START";
const char *introLine_2 = "NASTAVENI";

//START MENU
const char *startLine_1 = "Zahajit ohrev?";
const char *startLine_2 = "ENTER: POTVRDIT";
const char *startLine_3 = "CANCEL: ZRUSIT";

//NASTAVENI
const char *settingsLine_1 = "1. PREHEAT";
const char *settingsLine_2 = "2. SOAK";
const char *settingsLine_3 = "3. REFLOW";
const char *settingsLine_4 = "4. COOLING";

//KROKY
const char *stepPreheat_1 = "PREHEAT";
const char *stepPreheat_2 = "Teplota: ";

const char *stepSoak_1 = "SOAK";
const char *stepSoak_2 = "Doba: ";

const char *stepReflow_1 = "REFLOW";
const char *stepReflow_2 = "Teplota: ";

const char *stepCooling_1 = "COOLING";
const char *stepCooling_2 = "Teplota: ";

//OHREV MENu
const char *heatingLine_1_1 = "PREHEAT";
const char *heatingLine_1_2 = "SOAK";
const char *heatingLine_1_3 = "REFLOW";
const char *heatingLine_1_4 = "COOLING";

const char *heatingLine_2 = "Teplota: ";
const char *heatingLine_3 = "Setpoint:";
const char *heatingLine_4 = "Vystup:  ";

//error menu
const char *errorLine_1 = "DETEKOVANA CHYBA";
const char *errorLine_2 = "Cislo chyby:";
const char *errorLine_3 = "";
const char *errorLine_4 = "Smazat chybu";

//exit menu

const char *exitLine_1 = "OHREV DOKONCEN";
const char *exitLine_2 = "STISKEM LIBOVOLNE";
const char *exitLine_3 = "KLAVESY NAVRAT";
const char *exitLine_4 = "DO MENU";
