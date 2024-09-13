# Memory-Allocator
NUME: Viorel-Cosmin Cucu
GRUPA: 314CA

    TEMA #1 -SEGREGATED FREE LISTS-

## struct dll_node_t

        Aceasta reprezinta structura unui nod ce se regaseste in listele din
    sfl si in listele dublu inlantuite ce reprezinta zonele de memorie ocupata.
    Contine:
    void *data - pentru a fi o structura genrica
    struct dll_node_t *prev, *next;
    
        Pentru a retine toate datele de care am nevoie intr-un nod am creat 
    structura info_nod_t. Ca sa pastrez o structura genrica pentru nod, fac de 
    fiecare data cast la acesata strucura pentru void *data din nod.


 ## struct segregated_free_list

        Aceasta reprezinta stuctura vectorului de liste ce contine memoria
    goala.
    struct lists **vector_liste  
	int nr_liste 
	int mem_aloc - memoria locata pentru vectorul de liste pentru a stii cand 
        sa dau realloc
	int tip_rec - 0 sau 1(pt bonus)

 ## INIT_HEAP
        Cu ajutorul functiei creare_heap() creez sfl-ul.

## MALLOC()

        Caut in sfl o lista ce contine blocuri cu dimensiunea mai mare sau 
    egala cu numarul de bytes ce trebuie sa aloc si ii iau pozitia. Apoi am 2 
    cazuri unul in care gasesc blocuri cu exact acea dimensiune si altul in care
    trebuie sa fragmentez un bloc mai mare, iar blocul ramas dupa fragmentare
    este pus la locul lui in sfl. Aici in cazul in care o lista de a mea ajunge
    sa aibe 0 elemente o las in vector si nu o elimin chiar daca stiu ca din
    punct de vedere a memoriei e mai bine sa o elimin. O las pentru ca cel mai 
    posibil o sa mai folosesc acea lista si repetarea functiei realloc nu 
    reprezinta o chestie prea eficienta, de aceea salvez si in structura
    "segregated_free_list" memoria alocata pentru vectorul de liste pentru 
    a realoca mereu cu dublul numarului curent de liste.


## FREE()

        Caut in lista ce contine memoria ocupata blocul ce are adresa data.
    Daca nu-l gasesc afisez "Invalid free", iar daca-l gasesc, il elimin din
    lista si cu ajutorul functiei aranjare_free_0() il pun la locul lui in sfl
    tinand cont de ordinea adreselor. In cazul in care tip_reconstituire este 1
    cu ajutorul functiilor gastit_tata() si aranjare_free_1() caut nodul tata 
    in sfl si apoi reconstituiesc blocul, il elimin pe nodul tata din sfl si 
    pun blocul nou la locul lui in sfl. La sfarsitul functiei daca 
    tip_reconstituire e 1 aplic functia verificare_tata() ce ia fiecare nod in
    parte din sfl si verifica daca se afla si nodul sau tata tot in sfl, daca
    gaseste reconstituie blocul.

## WRITE()

        Caut in lista ce contine memoria ocupata blocul ce are adresa data. Iau
    cazul in care datele furnizate se pot scrie intr-un singur bloc. In cazul 
    in care nu incap intr-un singur bloc vad daca memoria alocata contine 
    adrese contigue si scriu informatia data pe toate blocurile alocate. Daca
    nodul avea date deja, le suprascriu. In cazul in care nu se gaseste un nod
    cu acea adresa sau memoria nu e contigua afisez 
    "Segmentation fault (core dumped)", apelez dump_memory(), eliberez memoria
    si ies din program.

## read()
        Caut in fiecare nod din lista de memorie alocata adresa data, chiar
    daca aceasta nu e cea de inceput. Folosesc functia poz_read() sa vad daca 
    gasesc adresa data si sa returez pozitia de unde trebuie sa citesc. Apoi 
    vad daca ce citesc se afla intr-un singur bloc sau daca trebuie sa ma plimb
    pe mai multe blocuri ce au adresa contigua, in caz ca nu e contigua sau nu
    am gasit adresa in lista afisez "Segmentation fault (core dumped)".


        Posibil sa se poata face ici-colo niste optimizari, dar in mare nu cred
    ca pot face o implemantare mai buna.
        Aceasta tema mi-a demonstrat ca atentia la curs si laboratoarele chiar
    ajuta enorm in intelegerea acestor structuri de date.
    
