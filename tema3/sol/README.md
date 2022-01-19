## README - Tema 3 APD - Calcule colaborative în sisteme distribuite
### Mihai Ionescu, 335CA
#### 19.01.2022

---

Rulare: 
- make
- mpirun -np <numar_procese> ./tema3 <dimensiune_vector> <eroare_comunicatie>

## Convenție de nume
Funcțiile helper:
- ale coordonatorilor 1, 2 - au prefixul "c_";
- ale coordonatorului 0 - au prefixul "c0_";
- ale workerilor au prefixul "w_"

## Flow
### c_establish_topology(), w_recv_array_chunk()
Cele 3 procese-coordonator își calculează propriul vector incomplet de părinți
(*parents*). Fiecare dintre ei îl trimite celorlalți 2, aceștia completând
vectorul propriu cu informațiile obținute. După, fiecare trimite acest vector
către proprii workeri. Un worker își determină coordonatorul prin parents[rank].
Fiecare apel MPI_Send e însoțit de un mesaj de tip M(sursă, destinație) afișat
în consolă.

### BARIERĂ 1
Folosită pentru asigurarea că toate nodurile au primit topologia completă.

### c0_assign_arr_to_coords(), c_recv_array_chunk()
Coordonatorul 0 creează un vector de lungime = parametrul dat la execuție v_len.
Împarte acest array în bucăți cu numărul egal cu numărul total de workeri,
folosind formulele:

```c
int start = ID * (double)N / P;
int end = min((ID + 1) * (double)N / P, N);
```

În vectorul *start_pos*, stochează poziția de start din array corespunzătoare
fiecărui coordonator. 
Trimite către fiecare coordonator (nu și sieși) porțiunea de vector asignată,
egală cu lungimea a num_workers(coordonator) chunk-uri.
Pentru a trimite și recepționa aceste porțiuni, se fac două apeluri MPI_Send:
se trimite inițial un int reprezentând lungimea porțiunii, apoi se trimite 
porțiunea în sine.


### c_assign_arr_to_workers(), w_recv_array_chunk()
Fiecare coordonator împarte propria bucată de array în mai multe porțiuni, pe
care le trimite fiecărui worker asignat acestuia (în același mod ca mai
devreme).

### w_compute(), w_send_array_result(), c_recv_arr_from_workers()
Fiecare worker procesează propriul chunk, înmulțind elementele cu 2. Trimite
chunkul înapoi spre coordonator.
Coordonatorii primesc rezultatele și le asamblează, recalculând poziția de start
în vector conform identificatorului dat workerului în momentul împărțirii.

### BARIERĂ 2
Folosită pentru asigurarea că procesul 0 a terminat distribuirea vectorului
inițial către ceilalți coordonatori, înainte de a iniția trimiterea rezultatlor
către acesta.

### c_send_arr_to_c0(), c0_assemble_final_array()
Coordonatorii 1 și 2 trimit rezultatele către coordonatorul 0. Acesta le
primeșțe, reasamblează vectorul pe baza lor și a propriului chunk asamblat
din rezultatele primite de la workerii acestuia. 

### BARIERĂ 3
Folosită pentru evitarea afișării de mesaje de tip SEND la consolă în timpul
printării vectorului final.

### c0_print_final_array()
La final, coordonatorul 0 afișează vectorul rezultat.
