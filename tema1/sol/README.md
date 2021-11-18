## README - Tema 1 APD - Paralelizarea unui algoritm genetic
### Mihai Ionescu, 335CA
### 18.11.2021
---

### arg.h
- conține structura-argument pentru thread_function
- include ID-ul threadului, bariera, datele corespunzătoare algoritmului genetic

### genetic_algorithm.c
În funcția **run_genetic_algorithm**, se alocă vectorii pentru generații și se
creează firele de execuție, lansându-le în execuție cu funcția de thread
**run_genetic_algorithm_parallel**.

În funcția de thread, implementarea a pornit de la instrucțiunile din
run_genetic_algorithm din schelet. S-au modificat următoarele:

- inițializarea vectorilor **current_generation** și **next_generation**. Cei
object_count indivizi au fost împărțiți în thread_count intervale 
**[start_obj_cnt, end_obj_cnt)**, fiecare thread ocupându-se de câte un 
interval;

- (iterația pentru fiecare generație nu se poate paraleliza, execuția fiind
secvențială prin natura algoritmului);

- **compute_fitness_function**, prin împărțirea indivizilor generației în
intervale, la fel ca mai sus

- funcția **cmpfunc** folosită de qsort compară acum datele din câmpul
**set_chromosomes_count** al indivizilor, care este actualizat de fiecare dată
înaintea apelului qsort. Actualizarea se face în paralel, fiecare thread 
calculând numărul de cromozomi setați care corespun unui grup de indivizi.
Această optimizare a avut impactul cel mai mare, având în vedere că în varianta
secvențială, recalcularea numărului de cromozomi se făcea la fiecare comparație
realizată de qsort.

- **selecția**, **mutația** și **crossover** au fost paralelizate prin aceeași
metodă, împărțind intervalul pe care se lucrează (un procent din 
[0, object_count]) pentru threads_count fire de execuție. 
În cazul crossover, secvențele de indecși trebuie să conțină doar numere pare,
deci capătul inferior al intervalului rezultat este incrementat în caz că este
impar.
De asemenea, în caz că procentul de 30% din părinți este un număr impar, se
copiază ultimul nemodificat pe un singur thread (testând dacă threadul curent
are o valoare fixată (0)).

- resetarea indecșilor generației nouă - paralelizată analog cu inițializarea

### Bariere
- sortarea cu qsort se realizează secvențial; este delimitată în funcția de 
thread prin două bariere:
	- prima - pentru a avea valorile funcției de fitness și
a numărului de cromozomi setați actualizate în momentul sortării;
	- a doua - pentru că operațiile următoare (selecție, mutație, crossover)
depind de ordinea indivizilor în vectorul-generație.
- a treia barieră a fost folosită la sfârșitul buclei, pentru sincronizarea
iterațiilor pentru fiecare generație.

### Restul instrucțiunilor
Funcțiile nemenționate din schelet nu au fost modificate.

Deoarece fiecare thread lucrează pe un interval separat, iar operațiile de
selectie, mutație, crossover scriu în zone diferite, continue de memorie, 
restul instrucțiunilor se execută în paralel independent, nefiind nevoie de
bariere / alte elemente pentru sincronizarea acestora (în afară de delimitarea
sortării).
