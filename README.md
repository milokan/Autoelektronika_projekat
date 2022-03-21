# Projekat iz Autoelektronike - Mjerenje brzine kretanja automobila

## Uvod
Zadatak ovog projekta je realizacija FreeRTOS softvera kojim se simulira mjerenje brzine kretanja automobila, kao i računanje pređenog puta. Kod je pisan u okruženju Microsoft Visual Studio Community 2022. Takođe, zadatak je bio i da se kod piše u skladu sa smjernicama definisanim MISRA standardom.

## Opis zadatka projekta
Trenutni broj obrtaja se dobija na osnovu inkremenata enkodera koji se nalazi na točku automobila. To se simulira slanjem broja koji predstavlja broj inkremenata enkodera svakih 200ms sa kanala 0 simulatora serijske komunikacije (AdvUniCom). Inkrementi se kreću u opsegu od 0 do 36000, gdje 36000 predstavlja jedan pun obrtaj točka. Broj inkremenata se pomoću reda (eng. queue) proslijeđuje taskovima za računanje brzine i pređenog puta.

Komunikacija sa PC-em je takođe ostvarena preko simulatora serijske komunikacije, ali na kanalu 1. Preko kanala 1 šalje se naredba kojom se zadaje obim točka u cm. Potrebno je poslati karaktere OM, a zatim broj koji predstavlja obim točka u cm. Naredba koja se šalje je formata \00OMx\0d gdje '00' u heksadecimalnom zapisu predstavlja početak poruke, 'x' obim točka, a '0d' u heksadecimalnom zapisu predstavlja CR (Carriage Return), što je 13 decimalno, i označava kraj poruke.

Potrebno je omogućiti reset pređenog puta.

START i STOP komande služe za mjerenje puta 'prošao' pređenog u vremenskom razmaku između slanja ovih komandi. Na osnovu ovih komandi dobija se razlika puteva zabilježenih u trenutku njihovog slanja. Ove komande su realizovane kao tasteri za čiju se simulaciju koristi LED bar. Simulacija pritiska tastera se vrši klikom na odgovarajuću diodu na LED bar-u. Pritiskom na taster START se pali jedna dioda kojom se označava da je mjerenje aktivno, a koja se gasi pritiskom tastera STOP, odnosno označava da je mjerenje završeno.

Podaci o izračunatoj brzini i pređenom putu se šalju PC-u preko kanala 1 simulatora serijske komunikacije periodom od 1000ms. Podaci o brzini se šalju iz taska za računanje brzine, a podaci o pređenom putu iz taska koji služi samo za slanje podataka PC-u. Potrebno je uvesti zaštitu kojom se onemogućava istovremeno slanje ovih dvaju podataka.

Na simulatoru sedam-segmentnog displeja se prikazuju izmjereni podaci. Brzina osvježavanja displeja je 100ms.

## Periferije
Periferije koje se koriste u ovom projektu su LED bar, sedam-segmentni displej i serijska komunikacija. Ove periferije nisu korištene u fizičkoj formi, već su korišteni programi koji zapravo predstavljaju simulaciju ovih periferija. To su: LED_bars_plus simulator logičkih ulaza i izlaza, odnosno LED dioda, 7seg_mux simulator sedam-segmentnog displeja i AdvUniCom simulator serijske komunikacije.

## Pregled taskova
Ovaj projekat sadrži sedam taskova koji su opisani u nastavku. U funkciji 'void main_demo(void)' se vrši inicijalizacija korišćenih periferija, postavljanje handler-a za prekidnu rutinu za prijem preko serijske komunikacije i kreiranje potrebnih semafora, redova i task-ova. Na kraju se poziva raspoređivač task-ova.

### void ukupni_predjeni_put(void* pvParameters)
Ovaj task računa pređeni put na osnovu zadatog obima točka i broja obrtaja po formuli 'ukupni_put = obim_tocka * obrtaji'. Prvo se preko reda 'inkrementi_q' primaju podaci o broju inkremenata enkodera, tj. o broju koji stiže sa kanala 0 simulatora serijske komunikacije svakih 200ms. Podaci se smještaju u promjenljivu 'rec_buf' pomoću koje se računa trenutni broj inkremenata tako što se oni uvećavaju za vrijednost 'rec_buf' svakim prolaskom kroz petlju, čime se simulira kretanje automobila. Uzeto je da 36000 inremenata predstavlja jedan obrtaj točka. Na osnovu toga, svaki put kada trenutni inkrementi dostignu broj veći ili jednak od 36000 promjenljva 'obrtaji' se uvećava za 1, čime se simulira obrtanje točka tokom kretanja. Obrtaji su inicijalno postavljeni na 0. Obrtaji se računaju tako što se promjenljiva 'obrtaji' sabere sa količnikom trenutnih inkremenata i 36000 (formula: obrtaji = obrtaji + ink_tren / (uint16_t)36000). Znači na početku su obrtaji 0 i nakon prvih 36000 inkremenata desi se jedan obrtaj, pa sljedećim prolaskom kroz petlju drugi itd. Nakon izračunavanja obrtaja trenutni inkrementi dobijaju ostatak dijeljenja sa 36000 kako bi se računanje dalje nastavilo od te vrijednosti. Pritiskom na prvu diodu od dole u trećem stupcu resetuje se put tako što se obrtaji postave na 0, a zatim se opisani postupak opet nastavlja.

### void mjerenje_brzine(void* pvParameters)
Zadatak ovog taska je računanje brzine kretanja automobila. Takođe se preko reda prima podatak o broju inkremenata koji stižu sa kanala 0 svakih 200ms. Zatim se računa koliko se cm od ukupnog obima točka pređe za jedan inkrement, odnosno svakog inkrementa, tako što se obim podijeli sa 36000, gdje je 36000 maksimalni broj inkremenata enkodera, i rezultat se smješta u promjenljivu 'ink_cm' (formula: ink_cm = (float)obim_tocka / 36000). Na osnovu te vrijednosti izračunava se brzina tako što se ta vrijednost pomnoži sa brojem koji stiže sa kanala 0 svakih 200ms, tj. sa vrijednosšću iz promjenljive 'rec_buf', što zapravo predstavlja mjeru koliko cm se pređe od ukupnog obima za taj konkretan broj inkremenata koji stižu, a to zapravo predstavlja pređen put u 200ms, gdje 200ms predstavlja vrijeme za koje se desi određen broj inkremenata (formula: brzina = (ink_cm * rec_buf) / 0.2). Npr. ako se sa kanala 0 svakih 200ms šalje broj inkremenata 12000 to znači da se točak u 200ms obrne za trećinu obima, odnosno punog obrtaja. Zatim se dalje u task-u vrši ispis podatka o izračunatoj brzini, odnosno slanje podatka PC-u. To se vrši pomoću brojača čijim inkrementovanjem se šalje po jedan karakter datog stringa. Kada se pošalje zadnji karakter brojač se resetuje na 0 i postupak slanja se ponavlja ispočetka. Pošto se originalno šalju podaci o putu, a da se podaci ne bi preklapali i slali istovremeno, uvedena je zaštita tako što se pritiskom na drugu diodu od dole u trećem stupcu obustavlja slanje podatka o putu, a prelazi na slanje podatka o brzini. Isključenjem te diode opet se šalje podatak o putu.

### void led_bar_tsk(void* pvParameters)
Ovaj task manipuliše radom LED bar-a i ispisom podataka na displej. Pritiskom na prvu diodu od dole u prvom stupcu na displeju se ispisuje trenutni pređen put. Pritiskom na drugu diodu od dole u istom stupcu prikazuje se podatak o brzini kretanja. Pritiskom, a potom isključenjem prve diode od dole u drugom stupcu podatak o trenutnom putu će se upisati u promjenljivu 'start_put'. Dok se ne pritisne druga dioda od dole u istom stupcu svijetliće četvrta dioda od dole u čevrtom stupcu kao indikator da je mjerenje aktivno. Pritiskom, a potom isključenjem druge diode od dole u tom stupcu podatak o trenutnom putu će se upisati u promjenljivu 'stop_put'. Pritiskom na treću diodu od dole u prvom stupcu na displeju će se prikazati razlika ova dva puta (prosao_put = stop_put - start_put).

### void SerialSend_Task0(void* pvParameters)
Ovaj task svakih 200ms šalje poruku oblika Ixxxxx. u kojoj x-evi predstavljaju broj koji predstavlja inkremente kao odgovor na okidačku poruku koju predstavlja karakter 'i' koji se šalje kanalu 0 serijske. Ovo je omogućeno štikliranjem opcije 'Auto' u prozoru AdvUniCom-a.

### void SerialReceive_Task0(void* pvParameters)
Ovaj task iz gore primljene poruke izvlači x-eve, odnosno broj inkremenata koji se šalje. Pomoću promjenljive 'cc' se očitavaju primljeni karakteri. Karakteri koji stižu sa kanala 0 se smještaju u niz iz kojeg se izvlače karakteri od interesa koji u ovom slučaju predstavljaju brojeve. Izvučeni karakteri se prevode u cjelobrojnu vrijednost koja se šalje preko reda ostalim taskovima za manipulaciju sa njom. Ovaj task čeka na semafor kojeg će da pošalje prekidna rutina svaki put kada neki karakter stigne na kanal 0.

### void SerialSend_Task1(void* pvParameters)
Ovaj task vrši ispis podatka o pređenom putu, odnosno njihovo slanje PC-u. Podaci se šalju isto kao i podaci o brzini postupkom opisanim gore.

### void SerialReceive_Task1(void* pvParameters)
Ovaj task obrađuje poruku pristiglu sa kanala 1 formata \00OMX\0d. Kao što je već opisano, 00 i 0d u heksadecimalnom formatu predstavljaju početak i kraj poruke. Karakteri O i M označavaju na šta se karakteri odnose, a karakter, odnosno karakteri od interesa su cifre broja X koji predstavlja zadati obim točka. Nakon izvlačenja karaktera oni bivaju prevedeni u odgovarajuću cjelobrojnu vrijednost koja se smješta u globalnu promjenljivu da bi se koristila i u drugim dijelovima koda. I ovaj task čeka na semafor kojeg šalje prekidna rutina kada stigne neki karakter na kanal 1.

## Uputstvo za simulaciju

