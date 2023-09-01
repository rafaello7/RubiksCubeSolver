# Rubik's Cube Solver

Program pozwala ułożyć kostkę Rubika. Wyszukuje najmniejszą możliwą liczbę ruchów które układają kostkę.

## Wymagania

Program kompiluje się i uruchamia pod Linuksem. Do kompilacji wymaga programu `g++`. Do uruchomienia
wymaga komputera z minimum 4GB RAM. Jednakże w przypadku niektórych kostek wyszukiwanie rozwiązania
przebiega znacznie szybciej gdy komputer ma co najmniej 32GB RAM.

## Kompilacja

By skompilować program, należy wejść do katalogu ze źródłami i uruchomić `make`.

## Uruchamianie

Należy wejść do katalogu ze źródłami i uruchomić program `./cubesrv` (bez parametrów).
Ważne jest, aby bieżącym katalogiem był ten ze źródłami, gdyż program czyta dodatkowe pliki (_cube.html_,
_cube.css_ etc.) z bieżącego katalogu.

Program działa jako serwer http, nasłuchuje na porcie 8080. Tak więc po uruchomieniu należy wejść przeglądarką
na stronę [http://localhost:8080/](http://localhost:8080/).

Strona ma dwa tryby działania kostką: _Manipulate_ oraz _Edit_. Początkowo jest aktywny tryb _Manipulate_,
który pozwala kręcić ścianami kostki. Natomiast tryb _Edit_ pozwala na selekcję kolorów poszczególnych
kwadracików.

Jeśli ma się fizyczną kostkę do ułożenia, należy przejść w tryb _Edit_ (wybrać radio button `Edit`). Na
każdym kwadraciku pojawia się 6 kolorów do wyboru. Należy wyklikać odpowiednie kolory na wszystkich
ścianach, po czym kliknąć button `Apply`.

Kliknięcie w już wybrany kolor powoduje odznaczenie tego koloru jako wybranego i powrót do listy kolorów na
tym kwadraciku do wyboru.

Dopóki kolory na wszystkich ściankach nie są wybrane, button `Apply` pozostaje wyszarzony. Kliknięcie
`Apply` powoduje zatwierdzenie wyglądu kostki i przejście z powrotem do trybu `Manipulate`.

W trybie `Manipulate`, kliknięcie przycisku `Solve` zaczyna wyszukiwanie ruchów układających kostkę.
Zależnie od układu kostki i mocy komputera, wyszukiwanie rozwiązania może trwać od kilku sekund do godzin.
Lista ruchów układających kostkę pojawia się po prawej stronie kostki. Kliknięcie elementu na liście
powoduje przejście kostki do stanu pośredniego, tego po wybranym (klikniętym) ruchu.

