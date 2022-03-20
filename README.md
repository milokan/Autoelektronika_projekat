# Uvod
Zadatak ovog projekta je realizacija softvera kojim se simulira mjerenje brzine kretanja automobila, kao i računanje pređenog puta. Kod je pisan u okruženju Microsoft Visual Studio Community 2022. Takođe, zadatak je bio i da se kod piše u skladu sa smjernicama definisanim MISRA standardom. 
# Opis zadatka projekta
Trenutni broj obrtaja se dobija na osnovu inkremenata enkodera koji se nalazi na točku automobila. To se simulira slanjem broja koji predstavlja broj inkremenata enkodera svakih 200ms sa kanala 0 simulatora serijske komunikacije (AdvUniCom). Inkrementi se kreću u opsegu od 0 do 36000, gdje 36000 predstavlja jedan pun obrtaj točka. Broj inkremenata se pomoću reda (eng. queue) proslijeđuje taskovima za računanje brzine i pređenog puta.

Komunikacija sa PC-em je takođe ostvarena preko simulatora serijske komunikacije, ali na kanalu 1. Preko kanala 1 šalje se naredba kojom se zadaje obim točka u cm. Potrebno je poslati karaktere OM, a zatim broj koji predstavlja obim točka u cm. Naredba koja se šalje je formata \00OMx\0d gdje '00' u heksadecimalnom zapisu predstavlja početak poruke, 'x' obim točka, a '0d' u heksadecimalnom zapisu predstavlja CR (Carriage Return), što je 13 decimalno, i označava kraj poruke.

Potrebno je omogućiti reset pređenog puta.

START i STOP komande služe za mjerenje puta 'prošao' pređenog u vremenskom razmaku između slanja ovih komandi. Na osnovu ovih komandi dobija se razlika puteva zabilježenih u trenutku njihovog slanja. Ove komande su realizovane kao tasteri za čiju se simulaciju koristi LED bar. Simulacija pritiska tastera se vrši klikom na odgovarajuću diodu na LED bar-u. Pritiskom na taster START se pali jedna dioda kojom se označava da je mjerenje aktivno, a koja se gasi pritiskom tastera STOP, odnosno označava da je mjerenje završeno.

Podaci o izračunatoj brzini i pređenom putu se šalju PC-u preko kanala 1 simulatora serijske komunikacije periodom od 1000ms. Podaci o brzini se šalju iz taska za računanje brzine, a podaci o pređenom putu iz taska koji služi samo za slanje podataka PC-u. Potrebno je uvesti zaštitu kojom se onemogućava istovremeno slanje ovih dvaju podataka.

Na simulatoru LCD displeja se prikazuju izmjereni podaci. Brzina osvježavanja displeja je 100ms.

# Periferije
