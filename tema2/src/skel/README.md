## README - Tema 2 APD - Procesarea de documente folosind paradigma Map-Reduce
### Mihai Ionescu, 335CA
#### 15.12.2021

---

Rulare: Tema2 <workers> <in_file> <out_file>"

"Tema2" are funcția de coordonator.
"MapTask" și "ReduceTask" oferă rezultatele ("MapResult", "ReduceResult") prin funcția getResult.

### Tema2.java
Se citesc parametrii: număr de threaduri, numele fișierelor de input și output);
Se execută urmatoarele funcții secvențial:

- _readData_: Din fișierul de input, se citesc: lungimea max per fragment, numărul și numele fișierelor ce vor fi analizate
- _splitInput_: Se împarte fiecare fișier de intrare în fragmente de lungime egală (cu excepția ultimului). Se creează task-uri Map (**MapTask**) asociate fiecărui fragment, stocate în lista mapTaskList și inițializate cu: 
	- numele documentului;
	- dimensiunea fragmentului;
	- offset-ul fragmentului în document
- _launchMappers_: Se lansează în execuție taskurile Map descrise, folosind un ForkJoinPool și apeluri la funcția invoke pentru a genera task pool-ul inițial. Se așteaptă finalizarea tuturor taskurilor din pool (tot prin invoke, funcție blocantă), apoi se dă shutdown la ForkJoinPool.
- _combine_: După finalizarea execuției tuturor MapTasks, se combină rezultatele parțiale generate de acestea (**MapResult**, câmp în clasa MapTask). Se mapează fiecărui document o listă cu aceste rezultate. Se creează task-uri de tip **ReduceTask**, stocate în lista reduceTaskList, fiecare fiind inițializat cu lista care corespunde unui document.
- _launchReducers_: Se lansează în execuție taskurile Reduce, în același mod cu cel descris în launchMappers
- _writeOutput_: Se scriu în fișierul de output datele rezultate în urma operației de reducere per fiecare document (reținute în câmpurile de tip **ReduceResult** din fiecare **ReduceTask**). Acestea sunt sortate în funcție de rang.

### MapTask.java
Se execută următoarele funcții secvențial:

- _compute_:
	- _readFragment_: Se citește fragmentul determinat de offsetul și lungimea date, împreună cu **2 caractere în plus** (1 la început, 1 la sfârșit).
		- _adjustFragmentEnd_: Dacă s-a putut citi caracterul suplimentar de la sfârșit (adică fragmentul nu este ultimul din fișier), se verifică dacă ultimele 2 caractere din tot bufferul citit (ultimul caracter din fragmentul propriu-zis și caracterul suplimentar) sunt separatori. 
		În caz negativ, înseamnă că fragmentul se termină cu un cuvânt incomplet => se citesc în mod repetat fragmente parțiale de **lungime = min(10, dimensiunea fragmentului inițial)**, până când se întâlnește un separator sau EOF. Se concatenează bucățile de cuvânt citite la fragmentul obiectului MapTask.
		Alegerea acestui mod de citire este cauzată de ineficiența lui **BufferedReader** de a citi câte un caracter, pe rând.
		- _adjustFragmentStart_: Dacă charul suplimentar citit la început nu este separator => se elimină primul cuvânt din fragment.

	- _tokenize_: Se execută doar dacă fragmentul este nevid după prelucrări. Prin **StringTokenizer**, se împarte fragmentul în cuvinte. Cele de lungime maximă sunt păstrate în lista **maxWordList**. În mapul **dictionary** se rețin perechi de tipul (lungime cuvânt - număr apariții cuvânt).  

### ReduceTask.java
Se execută următoarele funcții secvențial:

- _compute_:
	- _combineStep_: Se parcurge lista de dicționare parțiale calculate precedent de MapTasks; se unifică în mapul **finalDictionary**, adunând valorile cu aceeași cheie. Se calculează lungimea (cheia) maximă rezultată (**maxLength**). În lista **finalMaxWordList**, se păstrează cuvintele de lungimea maxLength obținute în urma parcurgerii tuturor listelor parțiale.
	- _reduceStep_: Se calculează rangul documentului, folosind finalDictionary si finalMaxWordList:
	(suma de fibonacci[lungime cuvânt + 1] * număr apariții) / număr total cuvinte.

### MapResult.java
Reține:
	- numele documentului;
	- dicționarul asociat;
	- lista de cuvinte de lungime maximală

### ReduceResult.java
Reține:
	- numele documentului;
	- rangul rounjit;
	- lungimea maximă a unui cuvânt;
	- numărul de cuvinte de lungime maximă
 