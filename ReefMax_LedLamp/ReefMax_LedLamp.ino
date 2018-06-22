/*
 Name:		ReefMax_LedLamp.ino
 Created:	2018-06-22 11:06:28
 Author:	MarcinSith
*/

#include <EEPROM.h>
#include <Wire.h>	//biblioteki RTC
#include <DS3231.h>

#include <LiquidCrystal_I2C.h> // dolaczenie pobranej biblioteki I2C dla LCD

DS3231 clock;
RTCDateTime dt;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Ustawienie adresu ukladu na 0x27

int H = NULL;			//aktualny czas
int M = NULL;
int S = NULL;
int Y = NULL;
int Mn = NULL;
int D = NULL;
int czasLetni = NULL;

int godzina = 0;	// godzina do zmiany czasu	gdy recznie zmieniamy czas  
int minuta = 0;		// minuta do zmiany czasu 
					//---------------------------------------------------------------------------------------------------------
const int channelA = 11;	// piny kanaów lampy 
const int channelB = 10;
const int channelC = 9;

const int Button = A0;		// pin przycisków 

//---------------------------------------------------------------------------------------------------------
int PWM_day_A = 200;// NULL;		//maksymalna moc swiecenia w dzien 
int PWM_day_B = 200;// NULL;
int PWM_day_C = 200;// NULL;


//---------------------------------------------------------------------------------------------------------
int PWM_night_A = 0;// NULL;	//maksymalna moc swiecenia w nocy
int PWM_night_B = 2;// NULL;
int PWM_night_C = 0;// NULL;


//---------------------------------------------------------------------------------------------------------
int PWM_A = 0;			//aktualna moc swiecenia kana³u A,B,C
int PWM_B = 0;
int PWM_C = 0;


//---------------------------------------------------------------------------------------------------------
const int maxPower = 200;	//maksymalna moc
const int minPower = 0;		//minimalna moc

//---------------------------------------------------------------------------------------------------------
int sunrise_H = 9;// NULL;		//czas wschodu s³onca
int sunrise_M = 0;// NULL;

int sunset_H = 21;// NULL;		//czas zachodu slonca
int sunset_M = 10;// NULL;

//---------------------------------------------------------------------------------------------------------
bool isDay = false;				// flagi pory dnia/nocy
bool isNight = false;
//---------------------------------------------------------------------------------------------------------
int menu = 15;	//licznik menu
//---------------------------------------------------------------------------------------------------------

const int menuButt = 1;		// prze³¹cza pozycje menu
const int upButt   = 2;		// inkrementuje aktualn¹ wartoœæ
const int downButt = 3;		// dekrementuje aktualn¹ wartoœæ
const int lastButt = 4;		// zatwierdza tryb aklimatyzacji lub korekty czasu

//---------------------------------------------------------------------------------------------------------
int tAklimatyzacja = 2;			// czas aklimatyzacji okreœlony w minutach
int aklimmatyzacjaFlaga = -1;	// flaga odpowiedzialna za za³¹czenie trybu aklimatyzacji  -1 wylaczona    1-wlaczona
int aklimH = 0, aklimM = 0;		// czas rozpoczecia aklimatyzacji
bool aklimStage1 = false, aklimStage2 = false, aklimStage3 = false;		//etapy aklimatyzacji

//--------------   < <   F  U  N  K  C  J  E   > >  -------------------------------------------------------

/*------------------ < < incrementPWM(int &pwm) > >-------------------------------------------------------------------
	Inkrementuje aktualn¹ moc œwiecenia danego kana³u (pwm) do wartoœci maksymalnej maxPower
	Prarametrem wejœciowym jest referencja do zmiennej pwm okreœlaj¹cej aktualn¹ moc œwiecenia danego kana³u 
---------------------------------------------------------------------------------------------------------------------*/
void incrementPWM(int &pwm)
{
	pwm++;
	if (pwm >= maxPower)pwm = maxPower;
}


/*-------------------- < <	decrementPWM(int &pwm)  > > --------------------------------------------------------------
	Dekrementuje aktualn¹ moc œwiecenia danego kana³u (pwm) do wartoœci maksymalnej minPower
	Prarametrem wejœciowym jest referencja do zmiennej pwm okreœlaj¹cej aktualn¹ moc œwiecenia danego kana³u
---------------------------------------------------------------------------------------------------------------------*/
void decrementPWM(int &pwm)
{
	pwm--;
	if (pwm <= minPower)pwm = minPower;
}


/*--------------------- < <  incrementH(int &H)  > > ------------------------------------------------------------------
	Inkrementuje zmienn¹ przechowuj¹c¹ godzinê 
	Prarametrem wejœciowym jest referencja do zmiennej H przechowuj¹c¹ godzinê
---------------------------------------------------------------------------------------------------------------------*/
void incrementH(int &H)
{
	H++;
	if (H > 23)H = 0;
}


/*---------------------- < <  decrementH(int &H)  > > -----------------------------------------------------------------
	Dekrementuje zmienn¹ przechowuj¹c¹ godzinê
	Prarametrem wejœciowym jest referencja do zmiennej H przechowuj¹c¹ godzinê
---------------------------------------------------------------------------------------------------------------------*/
void decrementH(int &H)
{
	H--;
	if (H < 0)H = 23;
}


/*---------------------- < <  incrementM(int &M)  > > ----------------------------------------------------------------
	Inkrementuje zmienn¹ przechowuj¹c¹ minuty 
	Prarametrem wejœciowym jest referencja do zmiennej M przechowuj¹c¹ minuty
---------------------------------------------------------------------------------------------------------------------*/
void incrementM(int &M)
{
	M++;
	if (M > 59)M = 0;
}


/*---------------------- < <  decrementM(int &M)  > > ----------------------------------------------------------------
Dekrementuje zmienn¹ przechowuj¹c¹ minuty
Prarametrem wejœciowym jest referencja do zmiennej M przechowuj¹c¹ minuty
---------------------------------------------------------------------------------------------------------------------*/
void decrementM(int &M)
{
	M--;
	if (M < 0)M = 59;
}


/*-------------------------- < <  przycisk()  > > ---------------------------------------------------------------------
	Funkcja odpowiada za odczytywanie stanu na wejœciu analogowym i na podstawie odczytanej wartoœci okreœla, który 
	przycisk zosta³ wciœniêty. 

	Parametrem wyjœciowym jest jedna ze zmiennych (lastButt, downButt, upButt, menuButt) której wartoœæ okreœla 
	wciœniêty przycisk.
---------------------------------------------------------------------------------------------------------------------*/
int przycisk() {
	int buttonValue = analogRead(Button);
	Serial.println(buttonValue);
	if (buttonValue > 0) {
		if (buttonValue < 940) {
			return lastButt;
		}
		else if (buttonValue < 970) {
			return downButt;
		}
		else if (buttonValue < 995) {
			return upButt;
		}
		else if (buttonValue > 1005) {
			delay(200);
			return menuButt;
		}
	}
	return 0;
}

/*-------------------------- < <  mainMenu()  > > ---------------------------------------------------------------------
	Funkcja odpowiada za przechodzenie przez kolejne pozycje w menu.
---------------------------------------------------------------------------------------------------------------------*/
void mainMenu()
{
	int buttOn = przycisk();
	if (buttOn != 0)
	{//?
		if (buttOn == menuButt) {
			menu++;
			if (menu > 15) { menu = 0; }
		}
	}
	if (menu < 14) {
		setUP();
	}
	else if (menu == 14) {
		lcd.setCursor(0, 1);
		lcd.print("                       ");
	}
	else {
		printActualPWM();
	}
}


/*-------------------------- < <  printActualPWM()  > > --------------------------------------------------------------
	Funkcja odpowiada za wyœwietlenie aktualnej mocy œwiecenia ka¿dego kana³u w %
---------------------------------------------------------------------------------------------------------------------*/
void printActualPWM()
{
	lcd.setCursor(0, 1);
	lcd.print(pwm2Power(PWM_A));
	lcd.print("%");
	if (pwm2Power(PWM_A)<100) { lcd.setCursor(3, 1);	lcd.print(" "); }
	if (pwm2Power(PWM_A)<10) { lcd.setCursor(2, 1);	lcd.print(" "); }

	lcd.setCursor(5, 1);
	lcd.print(pwm2Power(PWM_B));
	lcd.print("%");
	if (pwm2Power(PWM_B) <100) { lcd.setCursor(8, 1);	lcd.print(" "); }
	if (pwm2Power(PWM_B) <10) { lcd.setCursor(7, 1);	lcd.print(" "); }

	lcd.setCursor(10, 1);
	lcd.print(pwm2Power(PWM_C));
	lcd.print("%");
	if (pwm2Power(PWM_C) <100) { lcd.setCursor(13, 1);	lcd.print(" "); }
	if (pwm2Power(PWM_C) <10) { lcd.setCursor(12, 1);	lcd.print(" "); }

}


/*-------------------------- < <  aklimatyzacja()  > > --------------------------------------------------------------
	Funkcja odpowiada za przeprowadzenie trybu aklimatyzacji. Podzielona jest na trzy etapy (aklimStage1 : aklimStage3)
	- aklimStage1 - ca³kowite œciemnienie lampy
	- sklimStage2 - pozostawienie œciemnionej lampy na okreœlony czas
	- aklimStage3 - sprawdzenie aktualnej pory dnia/nocy i rozjaœnienie lampy do maksymanej mocy dziennej/nocnej
---------------------------------------------------------------------------------------------------------------------*/
void aklimatyzacja()
{
	//sciemnianie
	//if czy sciemnic
	if (aklimStage1)//etap pierwszy sciemnia lampe do 0 
	{
		do
		{
			decrementPWM(PWM_A, 0);
			decrementPWM(PWM_B, 0);
			decrementPWM(PWM_C, 0);

			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);

			getTime_and_Date();
			printTime();
			printActualPWM();
			delay(200);
		} while ((PWM_A != 0) || (PWM_B != 0) || (PWM_C != 0));
		menu = 15;
		aklimH = H;
		aklimM = M;
		aklimStage1 = false;
		aklimStage2 = true;
	}

	if (aklimStage2)	// etap 2 utrzymuje lampê zgaszon¹ przez zadany czas
	{

		if (((H * 60) + M) == ((aklimH * 60) + aklimM + tAklimatyzacja))
		{
			aklimStage2 = false;
			aklimH = 0;
			aklimM = 0;
			aklimStage3 = true;
		}
	}
	lcd.setCursor(8, 0);
	lcd.print("Czas:");
	lcd.print(aklimM + tAklimatyzacja - M);	// pozosta³y czas aklimatyzacji

								
	if (aklimStage3)	//etap 3 rozjasnienie lampy  po uplywie zadanego czasu
	{
		if ((((sunrise_H * 60) + sunrise_M) < ((H * 60) + M)) && (((H * 60) + M) < ((sunset_H * 60) + sunset_M)))	//sprawdzenie pory dnia/nocy
		{//dzien
			isDay = true;
			isNight = false;
		}
		else
		{//noc
			isDay = false;
			isNight = true;
		}
		softStart();
		aklimStage3 = false;
		aklimmatyzacjaFlaga = -1;
		lcd.setCursor(8, 0);
		lcd.print("           ");
	}
}


/*-------------------------- < <  setUP()  > > ----------------------------------------------------------------------
	Funkcja odpowiada za wyœwietlenie i obs³ugê menu
---------------------------------------------------------------------------------------------------------------------*/
void setUP() {
	int buttOn = 0;
	if (menu == 0) {

		lcd.setCursor(0, 1);
		lcd.print("Chn A Day=");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementPWM(PWM_day_A);
		}
		else if (buttOn == downButt) {
			decrementPWM(PWM_day_A);
		}
		lcd.print(pwm2Power(PWM_day_A));
		lcd.print("%     ");
	}
	else 	if (menu == 1) {
		lcd.setCursor(0, 1);
		lcd.print("Chn A Night=");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementPWM(PWM_night_A);
		}
		else if (buttOn == downButt) {
			decrementPWM(PWM_night_A);
		}
		lcd.print(pwm2Power(PWM_night_A));
		lcd.print("%    ");
	}
	else 	if (menu == 2) {
		lcd.setCursor(0, 1);
		lcd.print("Chn B Day=");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementPWM(PWM_day_B);
		}
		else if (buttOn == downButt) {
			decrementPWM(PWM_day_B);
		}
		lcd.print(pwm2Power(PWM_day_B));
		lcd.print("%    ");
	}
	else 	if (menu == 3) {
		lcd.setCursor(0, 1);
		lcd.print("Chn B Night=");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementPWM(PWM_night_B);
		}
		else if (buttOn == downButt) {
			decrementPWM(PWM_night_B);
		}
		lcd.print(pwm2Power(PWM_night_B));
		lcd.print("%     ");
	}
	else 	if (menu == 4) {
		lcd.setCursor(0, 1);
		lcd.print("Chn C Day=");//kanal A i C sa takie same wiec maja takie same sterowania wiec C robi za fizyczne bo nie ma osobnego sterowania na C 
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementPWM(PWM_day_C);
		}
		else if (buttOn == downButt) {
			decrementPWM(PWM_day_C);
		}
		lcd.print(pwm2Power(PWM_day_C));
		lcd.print("%     ");
	}
	else 	if (menu == 5) {
		lcd.setCursor(0, 1);
		lcd.print("Chn C Night=");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementPWM(PWM_night_C);
		}
		else if (buttOn == downButt) {
			decrementPWM(PWM_night_C);
		}
		lcd.print(pwm2Power(PWM_night_C));
		lcd.print("%      ");
	}
	else 	if (menu == 6) {
		lcd.setCursor(0, 1);
		lcd.print("G wschodu ");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementH(sunrise_H);
		}
		else if (buttOn == downButt) {
			decrementH(sunrise_H);
		}
		lcd.print(sunrise_H);
		lcd.print("       ");
	}
	else 	if (menu == 7) {
		lcd.setCursor(0, 1);
		lcd.print("M wschodu ");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementM(sunrise_M);
		}
		else if (buttOn == downButt) {
			decrementM(sunrise_M);
		}
		lcd.print(sunrise_M);
		lcd.print("       ");
	}
	else 	if (menu == 8) {
		lcd.setCursor(0, 1);
		lcd.print("G zachodu ");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementH(sunset_H);
		}
		else if (buttOn == downButt) {
			decrementH(sunset_H);
		}
		lcd.print(sunset_H);
		lcd.print("       ");
	}
	else 	if (menu == 9) {
		lcd.setCursor(0, 1);
		lcd.print("M zachodu ");
		buttOn = przycisk();
		if (buttOn == upButt) {
			incrementM(sunset_M);
		}
		else if (buttOn == downButt) {
			decrementM(sunset_M);
		}
		lcd.print(sunset_M);
		lcd.print("       ");
	}
	else 	if (menu == 10) {
		lcd.setCursor(0, 1);
		lcd.print("Akilm. ");
		buttOn = przycisk();
		if (buttOn == upButt) {
			tAklimatyzacja++;
			if (tAklimatyzacja > 180)tAklimatyzacja = 0;

		}
		else if (buttOn == downButt) {
			tAklimatyzacja--;
			if (tAklimatyzacja < 0)tAklimatyzacja = 0;

		}
		lcd.print(tAklimatyzacja);
		lcd.print(" ");

		if (buttOn == lastButt)
		{
			aklimmatyzacjaFlaga *= -1;
		}
		if (aklimmatyzacjaFlaga > 0)
		{
			lcd.print(" ON");
			aklimStage1 = true;
			lcd.setCursor(0, 1);
			lcd.print("                       ");
		}
		else
		{
			lcd.print(" OFF");
			aklimStage1 = false;
		}
	}
	else 	if (menu == 11) {//pozwala na zmiane czasu (ustawienie f=gofziny i minuty)
		lcd.setCursor(0, 1);
		lcd.print("Godz=");
		lcd.print(godzina);
		lcd.print(" min=");
		lcd.print(minuta);
		lcd.print("       ");
		buttOn = przycisk();
		if (buttOn == upButt)
		{
			godzina++;
			if (godzina > 23) godzina = 0;
		}
		else if (buttOn == downButt)
		{
			minuta++;
			if (minuta > 59)minuta = 0;
		}
		else if (buttOn == lastButt)
		{
			// Lub recznie (YYYY, MM, DD, HH, II, SS
			clock.setDateTime(dt.year, dt.month, dt.day, godzina, minuta, dt.second);
			lcd.setCursor(0, 1);
			lcd.print("- - ZAPISANO - -");
			delay(1000);
			lcd.setCursor(0, 1);
			lcd.print("                ");
			menu = 15;
		}
	}
	else 	if (menu == 12) {
		lcd.setCursor(0, 1);
		lcd.print("                  ");
		menu = 15;
	}
	else 	if (menu == 13) {
		lcd.setCursor(0, 1);
		lcd.print("                  ");
		menu = 15;
	}
}


/*-------------------------- < <  getTime_and_Date()  > > -----------------------------------------------------------
	Funkcja pobiera aktualny czas z uk³adu RTC
---------------------------------------------------------------------------------------------------------------------*/
void getTime_and_Date()
{
	// Ustawiany date i godzine kompilacji szkicu
	// clock.setDateTime(__DATE__, __TIME__);
	// Lub recznie (YYYY, MM, DD, HH, II, SS
	// clock.setDateTime(2014, 4, 13, 19, 21, 00);

	dt = clock.getDateTime();
	H = dt.hour;
	M = dt.minute;
	S = dt.second;
	Y = dt.year;
	Mn = dt.month;
	D = dt.day;

	if (czasLetni == 1)	//modyfikuje czas na czas letni (bez dodania godziny jest czas zimowy 
	{
		H += 1;
		if (H > 23) { H = 0; }
	}
}


/*-------------------------- < <  printTime()  > > ------------------------------------------------------------------
	Funkcja odpowiada za wyœiwetlenie aktualnego czasu na wyœwietlaczu 
---------------------------------------------------------------------------------------------------------------------*/
void printTime() {
	lcd.setCursor(0, 0);
	if (H < 10) {
		lcd.print("0");
		lcd.print(H);
	}
	else {
		lcd.print(H);
	}
	lcd.print(":");
	if (M < 10) {
		lcd.print("0");
		lcd.print(M);
	}
	else {
		lcd.print(M);
	}

	if (isDay == true) {
		lcd.setCursor(6, 0);
		lcd.print("D");
	}
	else {
		lcd.setCursor(6, 0);
		lcd.print("N");
	}
}


/*--------------------- < < incrementPWM(int &pwm, int maxPwm)  > > --------------------------------------------------
	Funkcja inkrementuje zmienn¹ pwm (danego kana³u) do jej maksymalnej wartoœci maxPwm okreœlonej przez u¿ytkownika.
	Prarametrem wejœciowym jest referencja do zmiennej pwm przechowuj¹c¹ aktualn¹ moc œwiecenia danego kana³u oraz
	wartoœæ do jakiej mo¿na rozjaœniæ dany kana³
---------------------------------------------------------------------------------------------------------------------*/
void incrementPWM(int &pwm, int maxPwm)
{
	pwm++;
	if (pwm >= maxPwm)pwm = maxPwm;
}


/*--------------------- < < decrementPWM(int &pwm, int minPWM)  > > --------------------------------------------------
	Funkcja dekrementuje zmienn¹ pwm (danego kana³u) do jej minimalnej wartoœci minPwm okreœlonej przez u¿ytkownika.
	Prarametrem wejœciowym jest referencja do zmiennej pwm przechowuj¹c¹ aktualn¹ moc œwiecenia danego kana³u oraz
	wartoœæ do jakiej mo¿na œciemniæ dany kana³
---------------------------------------------------------------------------------------------------------------------*/
void decrementPWM(int &pwm, int minPWM)
{
	pwm--;
	if (pwm <= minPWM)pwm = minPWM;
}


/*--------------------- < < softStart()  > > ------------------------------------------------------------------------
	Funkcja rozjaœnia lampê do zadanej po uruchomieniu lampy lub przeprowadzeniu trybu aklimatyzacji
---------------------------------------------------------------------------------------------------------------------*/
void softStart()	
{
	if (isDay)
	{
		do
		{
			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);

			incrementPWM(PWM_A, PWM_day_A);
			incrementPWM(PWM_B, PWM_day_B);
			incrementPWM(PWM_C, PWM_day_C);

			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);

			getTime_and_Date();
			printTime();
			lcd.setCursor(6, 0);
			lcd.print("SoftStart  ");
			printActualPWM();
			delay(500);
		} while ((PWM_A != PWM_day_A) || (PWM_B != PWM_day_B) || (PWM_C != PWM_day_C));
	}
	else if (isNight)
	{
		do
		{
			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);

			incrementPWM(PWM_A, PWM_night_A);
			incrementPWM(PWM_B, PWM_night_B);
			incrementPWM(PWM_C, PWM_night_C);

			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);

			getTime_and_Date();
			printTime();
			lcd.setCursor(6, 0);
			lcd.print("SoftStart  ");
			printActualPWM();
			delay(500);
		} while ((PWM_A != PWM_night_A) || (PWM_B != PWM_night_B) || (PWM_C != PWM_night_C));
	}
	lcd.setCursor(6, 0);
	lcd.print("         ");
}

/*--------------------- < < wschod()  > > ----------------------------------------------------------------------------
	Funkcja rozjaœnia lampê symuluj¹c wschód s³oñca
---------------------------------------------------------------------------------------------------------------------*/
void wschod()
{
	do
	{
		if (S % 3 == 0)
		{
			if (PWM_A > PWM_day_A) { decrementPWM(PWM_A, PWM_day_A); }
			else { incrementPWM(PWM_A, PWM_day_A); }
			if (PWM_B > PWM_day_B) { decrementPWM(PWM_B, PWM_day_B); }
			else { incrementPWM(PWM_B, PWM_day_B); }
			if (PWM_C > PWM_day_C) { decrementPWM(PWM_C, PWM_day_C); }
			else { incrementPWM(PWM_C, PWM_day_C); }
		}

		analogWrite(channelA, PWM_A);
		analogWrite(channelB, PWM_B);
		analogWrite(channelC, PWM_C);

		getTime_and_Date();
		printTime();
		lcd.setCursor(6, 0);
		lcd.print("Wschod");
		printActualPWM();
		delay(1000);
	} while ((PWM_A != PWM_day_A) || (PWM_B != PWM_day_B) || (PWM_C != PWM_day_C));
	lcd.setCursor(6, 0);
	lcd.print("      ");
}


/*--------------------- < < zachod()  > > ----------------------------------------------------------------------------
	Funkcja œciemnia lampê symuluj¹c zachód s³oñca
---------------------------------------------------------------------------------------------------------------------*/
void zachod()
{
	do
	{
		if (S % 3 == 0)
		{
			if (PWM_A > PWM_night_A) { decrementPWM(PWM_A, PWM_night_A); }
			else { incrementPWM(PWM_A, PWM_night_A); }
			if (PWM_B > PWM_night_B) { decrementPWM(PWM_B, PWM_night_B); }
			else { incrementPWM(PWM_B, PWM_night_B); }
			if (PWM_C > PWM_night_C) { decrementPWM(PWM_C, PWM_night_C); }
			else { incrementPWM(PWM_C, PWM_night_C); }
		}

		analogWrite(channelA, PWM_A);
		analogWrite(channelB, PWM_B);
		analogWrite(channelC, PWM_C);

		getTime_and_Date();
		printTime();
		lcd.setCursor(6, 0);
		lcd.print("Zachod");
		printActualPWM();
		delay(1000);
	} while ((PWM_A != PWM_night_A) || (PWM_B != PWM_night_B) || (PWM_C != PWM_night_C));
	lcd.setCursor(6, 0);
	lcd.print("      ");
}


/*--------------------- < < pwm2Power(int pwm)  > > ------------------------------------------------------------------
	Funkcja wylicza procentow¹ moc œwiecenia danego kana³u. 
	Parametrem wejœciowym jest wartoœæ zmiennej pwm danego kana³u
	Parametrem wyjœciowym jest moc œwiecenia danego kana³u w %
---------------------------------------------------------------------------------------------------------------------*/
int pwm2Power(int pwm)
{
	return pwm / 2;
}


void initLamp()
{
	pinMode(channelA, OUTPUT);
	pinMode(channelB, OUTPUT);
	pinMode(channelC, OUTPUT);

	analogWrite(channelA, minPower);
	analogWrite(channelB, minPower);
	analogWrite(channelC, minPower);
}

void initButton()
{
	pinMode(Button, INPUT);
}

/*

void zmianaCzasu()
{
// czas letni
if (Y == 2018 && Mn == 3 && D == 25 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1;  }
if (Y == 2019 && Mn == 3 && D == 31 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1;  }
if (Y == 2020 && Mn == 3 && D == 29 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1;  }
if (Y == 2021 && Mn == 3 && D == 28 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1;  }
if (Y == 2022 && Mn == 3 && D == 27 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1; }
if (Y == 2023 && Mn == 3 && D == 26 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1;  }
if (Y == 2024 && Mn == 3 && D == 31 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1; }
if (Y == 2025 && Mn == 3 && D == 30 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1;  }
if (Y == 2026 && Mn == 3 && D == 29 && H == 2 && M == 0 && czasLetni == 0) { czasLetni = 1; }
//czas zimowy
if (Y == 2018 && Mn == 10 && D == 28 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0;  }
if (Y == 2019 && Mn == 10 && D == 27 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0; }
if (Y == 2020 && Mn == 10 && D == 25 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0;  }
if (Y == 2021 && Mn == 10 && D == 31 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0;  }
if (Y == 2022 && Mn == 10 && D == 30 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0;  }
if (Y == 2023 && Mn == 10 && D == 29 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0; }
if (Y == 2024 && Mn == 10 && D == 27 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0;  }
if (Y == 2025 && Mn == 10 && D == 26 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0; }
if (Y == 2026 && Mn == 10 && D == 25 && H == 3 && M == 0 && czasLetni == 1) { czasLetni = 0;  }
}
*/
//--------------------------S E T U P------S E T U P------------------------------------------
void setup()
{
	Serial.begin(9600);
	clock.begin();
	lcd.begin(16, 2);
	initLamp();
	initButton();


	lcd.backlight(); // zalaczenie podwietlenia 

	getTime_and_Date();

	if ((((sunrise_H * 60) + sunrise_M) < ((H * 60) + M)) && (((H * 60) + M) < ((sunset_H * 60) + sunset_M)))
	{//dzien
		isDay = true;
		isNight = false;
	}
	else
	{//noc
		isDay = false;
		isNight = true;
	}
	softStart();

}

void loop()
{
	getTime_and_Date();
	//zmianaCzasu();
	printTime();

	// zaczynamy wchod s³onca
	if (H == sunrise_H && M == sunrise_M && (S == 5 || S == 10))
	{
		wschod();
		isDay = true;
		isNight = false;
	}
	//zaczynamy zachod s³oñca
	if (H == sunset_H && M == sunset_M && (S == 5 || S == 10))
	{
		zachod();
		isDay = false;
		isNight = true;
	}
	//ustaw odpowiednia moc w zaleznosci od tego czy jest dzien 	
	if (aklimmatyzacjaFlaga == -1)
	{
		if (isDay)
		{
			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);

			if (PWM_A != PWM_day_A)PWM_A = PWM_day_A;
			if (PWM_B != PWM_day_B)PWM_B = PWM_day_B;
			if (PWM_C != PWM_day_C)PWM_C = PWM_day_C;

			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);
		}

		//ustaw odpowiednia moc w zaleznosci od tego czy jest noc
		if (isNight)
		{
			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);

			if (PWM_A != PWM_night_A)PWM_A = PWM_night_A;
			if (PWM_B != PWM_night_B)PWM_B = PWM_night_B;
			if (PWM_C != PWM_night_C)PWM_C = PWM_night_C;

			analogWrite(channelA, PWM_A);
			analogWrite(channelB, PWM_B);
			analogWrite(channelC, PWM_C);
		}
	}
	else
	{
		aklimatyzacja();
	}
	//------------------------------------------------------------------------------------
	mainMenu();
	delay(100);
}