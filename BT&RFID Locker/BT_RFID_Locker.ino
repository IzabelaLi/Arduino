/******************************************************************************/
/*Biblioteki*/

#include <Servo.h> // biblioteka do obsługi serwa
#include <MFRC522.h> // biblioteka do modułu RFID
#include <SPI.h> // biblioteka do komunikacji szeregowej urządzeń peryferyjnych ( tu: MFRC522)
#include <LiquidCrystal.h> // biblioteka do obsługi wyświetlacza LCD


/*Zmienne globalne*/

byte numerUID[4]; // tablica do której zczytywany będzie 4-bajtowy numer UID karty
String tagID = ""; // kompletny numer UID karty przekonwertowany do zmiennej stringowej
String masterTag =""; // Karta Administratora definiowana raz na początku programu
String Tagi[10] = {}; // tablica numerów tagów które są zautoryzowane (upoważnione do otworzenia zamka)
int licznik = 0; // licznik umożliwiający zapis tagów w tablicy
int odleglosc; // liczba odczytana z czujnika analogowego
boolean TagOdczytany = false;
boolean PoprawnyTag = false;
boolean DrzwiOtwarte = false;

/*Deklaracja instancji*/

MFRC522 mfrc522(10,9); // Parametry -> (SS, RST) Pozostałe są niekonfigurowalne.
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // Parametry -> (RS, En, D4, D5, D6, D7)
Servo serwo;

/*****************************************************************************/

void setup() {
  
/*konfiguracja wyjscia dla LED*/
  pinMode(A5,OUTPUT);
  
/*Inicjacja kolejno : SPI, MFRC522, LCD*/
  SPI.begin();        
  mfrc522.PCD_Init(); 
  lcd.begin(16, 2);   
  
/*Ustawienie serwa w wyjściowej pozycji*/
  serwo.attach(8);  
  serwo.write(0); 
  delay(250);
  serwo.detach();
  
/*Początkowy komunikat wyświetlany na LCD*/
  lcd.print("    Zeskanuj");
  lcd.setCursor(0, 1);
  lcd.print("Karte Administratora");
  
/*Oczekiwanie na zeskanowanie Master Tagu*/
  while (!TagOdczytany) {
    TagOdczytany = getID();
    if ( TagOdczytany == true) {
      masterTag = tagID;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   Master Tag");
      lcd.setCursor(0, 1);
      lcd.print("  Zdefiniowany.");
    }
  }
  TagOdczytany = false;

 /*Wyswietlenie wyjsciowego komunikatu*/ 
  Wyswietl_lcd();
  
 /*Rozpoczęcie komunikacji szeregowej*/
  Serial.begin(9600);


}

/**************************************************************************/

void loop() {

/*odczytanie odległosci z czujnika optycznego*/
  int odleglosc = analogRead(A0);
 
/**************************************************************************/    
/*********************** Gdy drzwi są zamknięte ***************************/ 
/**************************************************************************/ 

  if (odleglosc <150) {

/*wywołanie funkcji pozwalającej na otworzenie zamka przez BT*/
servo(); 

/*Oczekiwanie na zeskanowanie karty*/   
    if ( ! mfrc522.PICC_IsNewCardPresent()) { 
      return;
    }
/*Sczytanie danych z karty*/   
    if ( ! mfrc522.PICC_ReadCardSerial()) {   
      return;
    }
    tagID = "";
/*Czytanie następuje szeregowo bajt po bajcie, w czterokrotnej petli gdyż adres karty jest 4-bajtowy*/    
    for ( uint8_t i = 0; i < 4; i++) {  
      numerUID[i] = mfrc522.uid.uidByte[i];
      
/*Funkcja concat łącząca sczytane bajty w jeden łańcuch String*/
      tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); 
    }
    tagID.toUpperCase();
    
/*Zakończenie sczytywania*/
    mfrc522.PICC_HaltA(); 

/*Znacznik poprawnego sczytania*/
    PoprawnyTag = false;
    
/*Sprawdzenie czy zeskanowany Tag to Karta Administratora*/
    if (tagID == masterTag) {
      lcd.clear();

/*W przypadku pozytywnym wejscie w Tryb ustawień*/
      lcd.print(" Tryb ustawien");
      lcd.setCursor(0, 1);
      lcd.print(" Dodaj/Usun Tag");

/*Oczekiwanie na zeskanowanie tagu, który ma byc dodany lub usunięty*/
      while (!TagOdczytany) { 
        
/*Funkcja sczytująca numer UID karty*/ 
        TagOdczytany = getID(); 
        
/*Jeżeli UID zostało odczytane porównanie go do numerów z tablicy zautoryzowanych tagów*/
        if ( TagOdczytany == true) { 
          for (int i = 0; i < 100; i++) { 
            if (tagID == Tagi[i]) { 
              
/*W przypadku znalezienia numeru UID zostaje on usunięty */
              for (i; i < licznik; i++) {
                Tagi[i]=Tagi[i+1];
                
               }
              licznik--; 
              
/*Komunikat o usunięciu tagu*/              
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print(" Tag usunieto!");
              
/*Wiadomosc wyjsciowa na ekranie*/

              Wyswietl_lcd();
              
              TagOdczytany = false;
              
              return;
              
            }
          }
          
/*Gdy tag nie został odnaleziony zostanie on dodany do tablicy*/
          Tagi[licznik] = tagID; 

/*Komunikat o dodaniu tagu*/            
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("   Tag dodano!");
          
          Wyswietl_lcd();
          licznik++;
          TagOdczytany = false;
          return;
        }
      }
    }
    else { 
      
/*Jeżeli zeskanowany tag nie jest Kartą Administratora 
- sprawdzenie czy znajduje się w tablicy zautoryzowanych tagów*/
    for (int i = 0; i < 10; i++) { 

/*W przypadku pozytywnym otworzenie zamka i wyswietlenie komunikatu*/
      if (tagID == Tagi[i]) { 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Tag poprawny!");
    
        serwo.attach(8);   
        serwo.write(45);
        delay(250);
        serwo.detach();
        Wyswietl_lcd();
        PoprawnyTag = true;
      }
    }
/*Jeżeli zeskanowany tag nie jest Master tagiem i nie jest zautoryzowany*/
    if (PoprawnyTag == false) { 

/*Wyswietlenie na ekranie odmowy dostępu*/
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Odmowa dostepu!");
      Wyswietl_lcd();
    }
    }}
    
/**************************************************************************/    
/*********************** Gdy drzwi są otwarte *****************************/ 
/**************************************************************************/ 

  else {

 /*Zaswiec LED (tak długo jak drzwi są otwarte) i wyswietl komunikat o otwarciu drzwi*/
    digitalWrite(A5,HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Drzwi Otwarte!");
    while (!DrzwiOtwarte) {
      odleglosc = analogRead(A0);
      if (odleglosc < 150) {
        DrzwiOtwarte = true;
        
      }
    }
    DrzwiOtwarte = false;
    digitalWrite(A5,LOW);
    delay(500);
    serwo.attach(8);
    serwo.write(0);
    delay(250);
    serwo.detach();
    Wyswietl_lcd();
  }
}

/******************************************************************************/

//Funkcja pozwalająca na sterowanie serwem przez BT
void servo(){

//Jezeli z aplikacji BT zostaną przesłane dane
if(Serial.available() > 0){   

//Wysteruj serwo wpisujac je jako pozycje
    int serwopoz = Serial.read(); 
    int pozycja = serwopoz-125; /*To dzialanie ma na celu skorygowanie pozycji serwa 
    w zwiazku z koniecznoscia jej zmiany wynikacjacej z późniejszego montażu oraz 
    tego, że aplikacja w programie Mitt App z jakiegoś powodu nie chciała się zaktualizować
    w telefonie, w założeniu miało być int pozycja = servopos*/
    serwo.attach(8); 
    serwo.write(pozycja);
    delay(250);
    serwo.detach(); 
  }
}
//Procedura wyświetlająca wyjściową wiadomość na ekranie
void Wyswietl_lcd() {
  delay(1500);
  lcd.clear();
  lcd.print("Kontrola Dostepu");
  lcd.setCursor(0, 1);
  lcd.print("  Zbliz Karte");
}

//Funkcja odczytująca UID karty
uint8_t getID() {

  if ( ! mfrc522.PICC_IsNewCardPresent()) { //Jeżeli w zasięgu PCD znajduje się PICC kontynuuj
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Jeżeli numer UID zostanie odczytany kontynuuj
    return 0;
  }
  tagID = "";
  
  for ( uint8_t i = 0; i < 4; i++) {  // Odczytywanie numeru UID bajt po bajcie, karta ma pamięć 4 bajtów
    numerUID[i] = mfrc522.uid.uidByte[i];
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); // Zamiana pobranych bajtów w Stringi i połączenie w jedną zmienną Stringową
  }
  tagID.toUpperCase();
  mfrc522.PICC_HaltA(); // Zatrzymanie odczytywania
  return 1;
}
