# Cel projektu
Proste urządzenie, które wyłączy światła mijania, gdy nie są potrzebne.
Dedykowane dla motocykla Suzuki V-Strom 650, roczniki od 2017 wzwyż.
Zaimplementowany algorytm realizuje moje wymagania, tzn. wpisuje się w mój sposób eksploatacji motocykla. 

## Od strony użytkownika
Zacznijmy od tego, że podchodzę do motocykla, włączam stacyjkę. Normalnie światła już świecą, ale po co. Póki co mam odcięty zapłon na _kill-switch_, a bieg na luzie. Włączam _kill-switch_, moto odpala, ale ja przecież jeszcze nigdzie nie jadę - kask trzeba ubrać, wsiąść, stopkę złożyć. I gdy już to wszystko jest zrobione, włączam bieg (magia, ta dam), światła się zapalają, a ja odjeżdżam. Niekoniecznie w stronę słońca, bo czasem razi, ale daleko.
Dojeżdżając do skrzyżowania, wyłączam bieg, ale wtedy światła już nie gasną. No chyba, że dojechałem do przejazdu kolejowego, i wiem, że chwilę postoję. Gaszę wtedy silnik _kill-switchem_, a światła automatycznie gasną. Zapalą się znów, gdy będzie włączony bieg oraz _kill-switch_ w pozycji do jazdy.
A gdyby mi po drodze przez przypadek zgasł silnik, to światła zgasną tylko na czas kręcenia rozrusznikiem.

Dodatkowo, mam do dyspozycji przycisk, który zmienia stan świateł na przeciwne (włączone-wyłączone). Przykładowo, gdy chcę sobie poświecić. Albo na czas jazdy w _offie_ wyłączyć światła. Przydaje się tutaj taki trik, a mianowicie przytrzymując przycisk w momencie włączania biegu (czyli wtedy, gdy światła się włączają) blokuję ich automatyczne włączenie.

## Dioda
Dioda generalnie odzwierciedla stan świateł, tzn. powinna być włączona, gdy światła są włączone, oraz wyłączona, gdy są wyłączone.
Dodatkowo, po włączeniu stacyjki, świeci się przez sekundę, nawet jeżeli światła powinny być wyłączone.

    ¯¯¯¯¯¯-___ ........... _-¯¯¯¯¯-_____-¯¯¯¯¯-_____-¯¯¯¯¯¯¯¯¯¯¯
    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----
    ↑     ^ sekunda i gaśnie       ^ a tu sobie powoli miga
    włączam stacyjkę                 oznaczając, że światła 
                                     się właśnie zapalają
Jeżeli dioda miga krótkimi, pojedynczymi „mrugnięciami”, to znaczy, że żarówka albo bezpiecznik są przepalone:

    _______¯_____¯_____¯____
    ...---+-----+-----+-----

Jeżeli miga jak szalona, to znaczy, że coś jest nie tak. Np wsadziliśmy żarówkę zbyt małej mocy:

    _¯_¯_¯_¯_¯_¯_¯_¯_¯_¯_¯_¯
    +-----+-----+-----+-----

Natomiast, jeżeli w trakcie normalnej jazdy, będzie przez sekundę mrugała jak szalona (pytanie, czy to da się zauważyć), oznacza to nieprzyjemną sytuację, gdy sterownik się zrestartował w wyniku jakiegoś błędu:

    ¯¯¯¯¯¯¯¯¯¯¯¯_¯_¯_¯_¯_¯_¯_¯¯¯¯¯¯¯¯¯¯
    +-----+-----+-----+-----+-----+----

   
# Sprzęt
## Mikrokontroler

Pracuję na ATTiny 84A, aczkolwiek projekt powinien dać się zaadaptować na inne mikrokontrolery o podobnych możliwościach sprzętowych. W szczególności na rodzinę ATTiny x4A.

### FUSE bits

Do domyślnej konfiguracji mikrokontrolera zostały wprowadzone następujące zmiany:
* włączenie timera watchdog,
* ustawienie detekcji spadku napięcia zasilania na około 4,3 V. Poniżej tej wartości mikrokontroler jest zablokowany w stanie RESET,
* maksymalna redukcja czasu oczekiwania po włączeniu zasilania.

**hfuse** - `0xCC`
`SPIEN = 0` - serial programmig interface enabled
`WDTON = 0` - watchdog timer always on
`BODLEVEL[2:0] = 100` - BOD level = 4,3 V

**lfuse** - `0xE2`
`CKDIV8 = 1` - NOT divide system clock
`SUT[1:0] = 10`  - startup time 14CK + 64 ms
`CKSEL[3:0] = 0010` - 8 MHz internal oscilator 

## Moduł mocy

TODO

## Interfejsy z systemami motocykla

TODO

# Programowanie

Ja korzystam po taniości z płytki Arduino połączonej z mikrokontrolerem, oraz _skeczu_ ArduinoISP dostępnego w ArduinoIDE.
Programowanie mikrokontrolera odbywa się przez `avrdude`, i np. żeby wejść w tryb interaktywny, należy wydać polecenie: `avrdude -v -p attiny84 -c arduino -P COM6 -b 19200 -t`, gdzie `COM6` oznacza nr portu, pod którym zarejestrowała się płytka Arduino podpięta do komputera.

Programowanie _fuse bit_ można wykonać wydając polecenie (w trybie interaktywnym `avrdude`):
`write lfuse 0 0xE2`
`write hfuse 0 0xCC`


# Oprogramowanie

Prawdopodobnie będziesz potrzebował(a) Microchip Studio, albo przynajmniej `avr-gcc`. 

## Pliki binarne

Kiedyś opublikuję :)
