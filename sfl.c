#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIE(assertion, call_description)  \
	do {                                  \
		if (assertion) {                  \
			fprintf(stderr, "(%s, %d): ", \
					__FILE__, __LINE__);  \
			perror(call_description);     \
			exit(errno);                  \
		}                                 \
	} while (0)

typedef struct dll_node_t {
	void *data;
	struct dll_node_t *prev, *next;
} dll_node_t;

typedef struct info_nod_t {
	char *data;
	int adr_start;
	int adr_tata;
	int size;
} info_nod_t;

typedef struct lists {
	struct dll_node_t *head;
	int size;
	int data_size;

} lists;

typedef struct segregated_free_list {
	struct lists **vector_liste;
	int nr_liste;
	int mem_aloc;
	int tip_rec;
} segregated_free_list;

typedef struct dump {
	int mem_heap;
	int malloc_calls;
	int free_calls;
	int nr_frags;
} dump_t;

dll_node_t *dll_get_nth_node(lists *list, int n)
{
	dll_node_t *aux = list->head;
	for (int i = 0; i < n; i++)
		aux = aux->next;

	return aux;
}

segregated_free_list
*creare_heap(int adr_start, int nr_liste, int nr_bytes_lista, int tip_rec)
{
	segregated_free_list *sfl = calloc(1, sizeof(segregated_free_list));
	sfl->mem_aloc = nr_liste;
	sfl->nr_liste = nr_liste;
	sfl->tip_rec = tip_rec;
	sfl->vector_liste = calloc(nr_liste, sizeof(lists *));
	DIE(!sfl->vector_liste, "Alocarea vectorului de liste esuata.");
	int adr_curenta = adr_start;
	for (int i = 0; i < nr_liste; i++) {
		sfl->vector_liste[i] = calloc(1, sizeof(lists));
		DIE(!sfl->vector_liste[i], "Alocarea vectorului de liste esuata.");
		int dim = 1 << (3 + i);
		//reprezinta dimensiunea ficarui bloc din lista respectiva
		sfl->vector_liste[i]->data_size = dim;
		sfl->vector_liste[i]->size = 0;
		int nr_blocuri = nr_bytes_lista / dim;
		for (int j = 0; j < nr_blocuri; j++) {
			if (sfl->vector_liste[i]->size == 0) {
				dll_node_t *nod = calloc(1, sizeof(dll_node_t));
				DIE(!nod, "calloc failed");
				nod->prev = NULL;
				nod->next = NULL;
				sfl->vector_liste[i]->head = nod;
				info_nod_t *info = calloc(1, sizeof(info_nod_t));
				DIE(!info, "calloc failed");
				info->adr_start = adr_curenta;
				info->size = dim;
				nod->data = info;
				sfl->vector_liste[i]->size++;
			} else {
				dll_node_t *nod = calloc(1, sizeof(dll_node_t));
				DIE(!nod, "calloc failed");
				info_nod_t *info = calloc(1, sizeof(info_nod_t));
				DIE(!info, "calloc failed");
				info->adr_start = adr_curenta;
				info->adr_tata = 0;
				nod->prev = dll_get_nth_node(sfl->vector_liste[i], j - 1);
				nod->prev->next = nod;
				nod->next = NULL;
				sfl->vector_liste[i]->size++;
				info->size = dim;
				nod->data = info;
			}
			adr_curenta = adr_curenta + dim;
		}
	}

	return sfl;
}

lists *alocare(void)
{
lists *alocata = calloc(1, sizeof(lists));
alocata->head = NULL;
alocata->size = 0;
return alocata;
}

void ordonare_vector_liste(segregated_free_list *sfl)
{
for (int i = 0; i < sfl->nr_liste - 1; i++) {
	for (int j = i + 1; j < sfl->nr_liste; j++) {
		if (sfl->vector_liste[i]->data_size >
			sfl->vector_liste[j]->data_size) {
			lists *aux = sfl->vector_liste[i];
			sfl->vector_liste[i] = sfl->vector_liste[j];
			sfl->vector_liste[j] = aux;
			}
		}
	}
}

void aranjare_malloc(segregated_free_list *sfl, dll_node_t *nod)
{
	int poz;
	if (sfl->nr_liste >= sfl->mem_aloc) {
		sfl->mem_aloc = 2 * sfl->mem_aloc;
		sfl->vector_liste = realloc(sfl->vector_liste, sfl->mem_aloc *
		sizeof(lists *));
		DIE(!sfl->vector_liste, "rellocare esuata");
	}
	/*in cazul in care nu mai avem loc in vectorul de liste relocam
		vectorul de liste*/
	poz = sfl->nr_liste;
	sfl->nr_liste++;
	sfl->vector_liste[poz] = malloc(sizeof(lists));
	DIE(!sfl->vector_liste[poz], "malloc failed");
	info_nod_t *info = (info_nod_t *)nod->data;
	sfl->vector_liste[poz]->data_size = info->size;
	sfl->vector_liste[poz]->size = 1;
	sfl->vector_liste[poz]->head = nod;
	nod->next = NULL;
	nod->prev = NULL;
	ordonare_vector_liste(sfl);
}

dll_node_t *nodul_optim(lists *list, int val)
{
	dll_node_t *aux = list->head;
	info_nod_t *info1 = (info_nod_t *)aux->data;

	if (val < info1->adr_start)
		return NULL;

	for (int i = 0; i < list->size; i++) {
		info1 = (info_nod_t *)aux->data;
		if (!aux->next && info1->adr_start < val)
			return aux;

		info_nod_t *info2 = (info_nod_t *)aux->next->data;
		if (info1->adr_start < val && info2->adr_start > val)
			return aux;
		/*compar adresele cu adresa "val" si returnez nodul optim
		  astfel incat adresele sa fie in ordine crescatoare*/
		aux = aux->next;
	}
	return NULL;
}

void aranjare_free_0(segregated_free_list *sfl, dll_node_t *nod)
{
int ok = 1;
info_nod_t *info_nod = (info_nod_t *)nod->data;
for (int i = 0; i < sfl->nr_liste; i++)
	if (info_nod->size == sfl->vector_liste[i]->data_size) {
		ok = 0;
		//exista o lista cu dimensiunea egala cu dimensiunea blocului
		if (sfl->vector_liste[i]->size == 0) {
			sfl->vector_liste[i]->head = nod;
			sfl->vector_liste[i]->size++;
			nod->next = NULL;
			nod->prev = NULL;
			break;
		}
	dll_node_t *temp = nodul_optim(sfl->vector_liste[i], info_nod->adr_start);
	//pun nodul la locul lui in ordinea crescatoare a adreselor
	if (!temp) {
		nod->next = sfl->vector_liste[i]->head;
		sfl->vector_liste[i]->head->prev = nod;
		sfl->vector_liste[i]->head = nod;
		nod->prev = NULL;
	} else {
		if (temp->next) {
			nod->next = temp->next;
			temp->next->prev = nod;
		} else {
			nod->next = NULL;
		}
		temp->next = nod;
		nod->prev = temp;
	}
	sfl->vector_liste[i]->size++;
	break;
	}
if (ok == 1)
	aranjare_malloc(sfl, nod);
	//crearea unei noi liste in vectorul de liste
}

void aranjare_free_1(segregated_free_list *sfl, dll_node_t *nod,
					 dll_node_t *tata)
{
	info_nod_t *info_nod = (info_nod_t *)nod->data;
	info_nod_t *info_tata = (info_nod_t *)tata->data;
	info_nod->size = info_nod->size + info_tata->size;
	int poz = -1;
	for (int i = 0; i < sfl->nr_liste; i++)
		if (info_tata->size == sfl->vector_liste[i]->data_size) {
			poz = i;
			break;
		}

	if (sfl->vector_liste[poz]->size == 1) {
		sfl->vector_liste[poz]->head = NULL;
	} else if (!tata->prev) {
		sfl->vector_liste[poz]->head = tata->next;
		tata->next->prev = NULL;
	} else if (!tata->next) {
		tata->prev->next = NULL;
	} else {
		tata->prev->next = tata->next;
		tata->next->prev = tata->prev;
	}
	tata->prev = NULL;
	tata->next = NULL;
	sfl->vector_liste[poz]->size--;
	info_nod->adr_tata = info_tata->adr_tata;
	free(tata->data);
	free(tata);
	//reconstituiesc blocul si il elim pe nodul "tata" din vectorul de liste

	aranjare_free_0(sfl, nod);
	//pun blocul reconstituit la locul lui in vectorul de liste/sfl
}

void malloc_fara_frag(segregated_free_list *sfl, dll_node_t *aux,
					  lists *alloc, int i)
{
	if (alloc->size == 0) {
		//cazul in care lista ce are memoria alocata e goala
		alloc->head = aux;
		if (aux->next) {
			sfl->vector_liste[i]->head = aux->next;
			aux->next->prev = NULL;
			sfl->vector_liste[i]->size--;
		} else {
			sfl->vector_liste[i]->head = NULL;
			sfl->vector_liste[i]->size = 0;
		}
		aux->prev = NULL;
		aux->next = NULL;
		alloc->size++;
	} else {
		//cazul in care lista ce are memoria alocata nu e goala
		if (aux->next) {
			sfl->vector_liste[i]->head = aux->next;
			aux->next->prev = NULL;
			sfl->vector_liste[i]->size--;
		} else {
			sfl->vector_liste[i]->head = NULL;
			sfl->vector_liste[i]->size = 0;
		}
		/*aflu nodul ce se afla inaintea nodului "aux"(daca exista)
		apoi il leg dupa acesta*/
		info_nod_t *info_aux = (info_nod_t *)aux->data;
		dll_node_t *temp = nodul_optim(alloc, info_aux->adr_start);
		if (!temp) {
			aux->next = alloc->head;
			alloc->head->prev = aux;
			alloc->head = aux;
			aux->prev = NULL;
		} else {
			if (temp->next) {
				aux->next = temp->next;
				temp->next->prev = aux;
			} else {
				aux->next = NULL;
			}
			temp->next = aux;
			aux->prev = temp;
		}

		alloc->size++;
	}
}

void malloc_cu_frag(segregated_free_list *sfl, lists *alloc, dll_node_t *aux,
					dll_node_t *nod, int i)
{
	if (alloc->size == 0) {
		alloc->head = nod;
		nod->next = NULL;
		nod->prev = NULL;
		alloc->size++;
	} else {
		info_nod_t *info_nod = (info_nod_t *)nod->data;
		dll_node_t *temp = nodul_optim(alloc, info_nod->adr_start);
		if (!temp) {
			nod->next = alloc->head;
			alloc->head->prev = nod;
			alloc->head = nod;
			nod->prev = NULL;
		} else {
			if (temp->next) {
				nod->next = temp->next;
				temp->next->prev = nod;
			} else {
				nod->next = NULL;
			}
			temp->next = nod;
			nod->prev = temp;
		}
		alloc->size++;
	}
	if (aux->next) {
		sfl->vector_liste[i]->head = aux->next;
		aux->next->prev = NULL;
		sfl->vector_liste[i]->size--;
	} else {
		sfl->vector_liste[i]->head = NULL;
		sfl->vector_liste[i]->size = 0;
	}
}

int MALLOC(segregated_free_list *sfl, int nr_bytes, lists *alloc, int *nr_frags)
{
	int poz = sfl->nr_liste, ok = 0;
	for (int i = 0; i < sfl->nr_liste; i++) {
		if (sfl->vector_liste[i]->data_size >= nr_bytes) {
			poz = i;
			break;
		}
	}

	for (int i = poz; i < sfl->nr_liste; i++) {
		if (sfl->vector_liste[i]->size > 0) {
			ok = 1;
			dll_node_t *aux = sfl->vector_liste[i]->head;
			if (nr_bytes == sfl->vector_liste[i]->data_size) {
				malloc_fara_frag(sfl, aux, alloc, i);
				/*cazul in care gasesc in vectorul de liste o succesiune
				de liste ce au dimensiunea egala cu numarul de bytes pe care
				trebuie sa l aloc*/
			} else {
				*nr_frags = *nr_frags + 1;
				dll_node_t *nod = calloc(1, sizeof(dll_node_t));
				DIE(!nod, "malloc failed");
				info_nod_t *info_aux = (info_nod_t *)aux->data;
				info_nod_t *info_nod = calloc(1, sizeof(info_nod_t));
				DIE(!info_nod, "malloc failed");
				nod->data = info_nod;
				if (sfl->tip_rec == 1)
					info_nod->adr_tata = info_aux->adr_start + nr_bytes;
					//adresa blocului din care s-a rupt blocul curent
				info_nod->adr_start = info_aux->adr_start;
				info_nod->size = nr_bytes;
				info_nod->data = NULL;
				nod->next = NULL;
				nod->prev = NULL;
				malloc_cu_frag(sfl, alloc, aux, nod, i);
				aux->next = NULL;
				info_aux->adr_start = info_aux->adr_start + nr_bytes;
				info_aux->size = info_aux->size - nr_bytes;
				// in auxiliar se gaseseste blocul ramas dupa fragmentare
				aranjare_free_0(sfl, aux);
				//cu aceasta functie il punem la locul lui in sfl
			}
			break;
		}
	}

	if (ok == 0)
		printf("Out of memory\n");

	return ok;
}

dll_node_t *gasit_tata(segregated_free_list *sfl, int adr_tata)
{
	//caut nodul cu adresa "adr_tata" in sfl
	for (int i = 0; i < sfl->nr_liste; i++) {
		if (sfl->vector_liste[i]->size > 0) {
			dll_node_t *aux = sfl->vector_liste[i]->head;
			while (aux) {
				info_nod_t *info_aux = (info_nod_t *)aux->data;
				if (info_aux->adr_start == adr_tata)
					return aux;
				aux = aux->next;
			}
		}
	}
	return NULL;
}

void verificare_tata(segregated_free_list *sfl)
{
	for (int i = 0; i < sfl->nr_liste; i++) {
		dll_node_t *aux2 = NULL;
		dll_node_t *aux21 = NULL;
		if (sfl->vector_liste[i]->size > 0) {
			aux2 = sfl->vector_liste[i]->head;
			while (aux2) {
				info_nod_t *info_aux2 = (info_nod_t *)aux2->data;
				dll_node_t *tata = gasit_tata(sfl, info_aux2->adr_tata);
			//pentru fiecare nod din sfl verific daca are nodul tata prin sfl
				if (tata) {
					aux21 = aux2;
					if (sfl->vector_liste[i]->size == 1) {
						sfl->vector_liste[i]->head = NULL;
					} else if (!aux2->prev) {
						sfl->vector_liste[i]->head = aux2->next;
						aux2->next->prev = NULL;
					} else if (!aux2->next) {
						aux2->prev->next = NULL;
					} else {
						aux2->prev->next = aux2->next;
						aux2->next->prev = aux2->prev;
					}

					aux2->prev = NULL;
					aux2->next = NULL;
					sfl->vector_liste[i]->size--;
					//scot nodul din lista de blocuri libere
					aranjare_free_1(sfl, aux2, tata);
					//reconstituirea blocului
				}
				if (aux21) {
					aux2 = aux21->next;
					aux21 = NULL;
				} else {
					aux2 = aux2->next;
				}
			}
		}
	}
}

int FREE(segregated_free_list *sfl, lists *alocat, int adr)
{
	if (adr == 0) {
		return 1;
	} else if (!alocat->head) {
		printf("Invalid free\n");
	} else {
		int ok = 0;
		dll_node_t *aux = alocat->head;
		//caut nodul cu adresa "adr" in lista de blocuri alocate
		while (aux) {
			info_nod_t *info_aux = (info_nod_t *)aux->data;
			if (info_aux->adr_start == adr) {
				ok = 1;
				break;
			}
			aux = aux->next;
		}
		if (ok == 0) {
			printf("Invalid free\n");
			//daca nu l-am gasit afisez mesajul corespunzator
		} else {
			info_nod_t *info_aux = (info_nod_t *)aux->data;
			if (info_aux->data)
				free(info_aux->data);
			if (!aux->prev) {
				alocat->head = aux->next;
				if (aux->next)
					aux->next->prev = NULL;
				aux->next = NULL;
			} else if (!aux->next) {
				aux->prev->next = NULL;
				aux->prev = NULL;
			} else {
				aux->next->prev = aux->prev;
				aux->prev->next = aux->next;
				aux->next = NULL;
				aux->prev = NULL;
			}
			alocat->size--;
			//il scot din lista de blocuri alocate

			if (sfl->tip_rec == 1) {
				info_nod_t *info_aux = (info_nod_t *)aux->data;
				dll_node_t *tata = gasit_tata(sfl, info_aux->adr_tata);
				//caut tatal nodului curent in sfl
				if (tata)
					aranjare_free_1(sfl, aux, tata);
					//realizez reconstituirea blocului
				else
					aranjare_free_0(sfl, aux);
			} else {
				aranjare_free_0(sfl, aux);
			}
			if (sfl->tip_rec == 1)
				verificare_tata(sfl);
				//caut daca se mai afla prin sfl noduri ce nu s-au reconstituit
		}
		return ok;
	}
	return 0;
}

void dump_memory(segregated_free_list *sfl, lists *mem_alocata,
				 dump_t *info_dump)
{
	int free_blocks = 0, mem_aloc = 0;
	printf("+++++DUMP+++++\n");
	printf("Total memory: %d bytes\n", info_dump->mem_heap);
	dll_node_t *aux1 = mem_alocata->head;
	while (aux1) {
		info_nod_t *info_aux1 = (info_nod_t *)aux1->data;
		mem_aloc = mem_aloc + info_aux1->size;
		aux1 = aux1->next;
	}
	printf("Total allocated memory: %d bytes\n", mem_aloc);
	printf("Total free memory: %d bytes\n", info_dump->mem_heap - mem_aloc);
	for (int i = 0; i < sfl->nr_liste; i++)
		free_blocks = free_blocks + sfl->vector_liste[i]->size;

	printf("Free blocks: %d\n", free_blocks);
	printf("Number of allocated blocks: %d\n", mem_alocata->size);
	printf("Number of malloc calls: %d\n", info_dump->malloc_calls);
	printf("Number of fragmentations: %d\n", info_dump->nr_frags);
	printf("Number of free calls: %d\n", info_dump->free_calls);

	for (int i = 0; i < sfl->nr_liste; i++) {
		if (sfl->vector_liste[i]->size == 0)
			continue;
		printf("Blocks with %d bytes - %d free block(s) : ",
			   sfl->vector_liste[i]->data_size, sfl->vector_liste[i]->size);
		dll_node_t *aux = sfl->vector_liste[i]->head;
		for (int j = 0; j < sfl->vector_liste[i]->size; j++) {
			info_nod_t *info_aux = (info_nod_t *)aux->data;
			if (j == sfl->vector_liste[i]->size - 1)
				printf("0x%x", info_aux->adr_start);
			else
				printf("0x%x ", info_aux->adr_start);
			aux = aux->next;
		}
		printf("\n");
	}

	dll_node_t *aux = mem_alocata->head;
	printf("Allocated blocks :");
	while (aux) {
		info_nod_t *info_aux = (info_nod_t *)aux->data;
		printf(" (0x%x - %d)", info_aux->adr_start, info_aux->size);
		aux = aux->next;
	}
	printf("\n");
	printf("-----DUMP-----\n");
}

int WRITE(lists *alocat, int adr, char *date, int nr_bytes,
		  segregated_free_list *sfl, dump_t *info)
{
	dll_node_t *aux = alocat->head;
	info_nod_t *info_aux;
	while (aux) {
		info_aux = (info_nod_t *)aux->data;
		if (info_aux->adr_start == adr)
			break;
		aux = aux->next;
	}

	if (!aux) {
		printf("Segmentation fault (core dumped)\n");
		dump_memory(sfl, alocat, info);
		return 0;
	} else if (info_aux->size >= nr_bytes) {
		//cazul in care incape textul intr-un bloc
		if (!info_aux->data) {
			info_aux->data = calloc(1, info_aux->size + 1);
			DIE(!info_aux->data, "malloc failed");
		}
		memcpy(info_aux->data, date, nr_bytes);
	} else if (info_aux->size < nr_bytes) {
		int size = info_aux->size;
		while (size < nr_bytes) {
			info_aux = (info_nod_t *)aux->data;
			info_nod_t *info_aux_next = (info_nod_t *)aux->next->data;
			if (aux->next && info_aux_next->adr_start == info_aux->adr_start +
				info_aux->size) {
				size = size + info_aux_next->size;
				/*datele se extind pe mai multe blocuri si ma uit sa vad
				daca memoria alocata contigua e suficienta*/
				aux = aux->next;
			} else {
				printf("Segmentation fault (core dumped)\n");
				dump_memory(sfl, alocat, info);
				return 0;
			}
		}
		dll_node_t *temp = alocat->head;
		info_nod_t *info_temp;
		while (temp) {
			info_temp = (info_nod_t *)temp->data;
			if (info_temp->adr_start == adr)
				break;
			temp = temp->next;
		}
		int count = 0;
		if (size >= nr_bytes) {
			while (count < nr_bytes) {
				info_temp = (info_nod_t *)temp->data;
				if (!info_temp->data) {
					info_temp->data = calloc(1, info_temp->size + 1);
					DIE(!info_temp->data, "malloc failed");
				}
				if (count + info_temp->size > nr_bytes)
					memcpy(info_temp->data, date + count, nr_bytes - count);
					/*in cazul ca se afla la sfarsitul datelor copiaza
					exact cat e necesar*/
				else
					memcpy(info_temp->data, date + count, info_temp->size);
					/*in cazul in care se poate copia din date exact
					 dimensiunea blocului*/
				count = count + info_temp->size;
				temp = temp->next;
			}
		}
	}
	return 1;
}

int poz_read(dll_node_t *aux, int adr, int *ok)
{
	info_nod_t *info_aux = (info_nod_t *)aux->data;
	int a = info_aux->adr_start;
	int b = a + info_aux->size;
	if (a <= adr && adr < b) {
		*ok = 1;
		return adr - a;
	}
	return 0;
}

int read(lists *alocat, int adr, int nr_bytes, segregated_free_list *sfl,
		 dump_t *info)
{
int ok = 0, seg = 0;
dll_node_t *aux = alocat->head;
while (aux) {
	int poz = poz_read(aux, adr, &ok);
	//returneaza pozitia de la care trebuie sa citesc
	if (ok == 1) {
		info_nod_t *info_aux = (info_nod_t *)aux->data;
		char *propozitie = calloc(1, nr_bytes + 1);
		int marime = info_aux->size - poz;
		DIE(!propozitie, "malloc failed");
		if (marime > nr_bytes)
			memcpy(propozitie, info_aux->data + poz, nr_bytes);
			//citirea se face dintr-un singur bloc
		else
			memcpy(propozitie, info_aux->data + poz, marime);
		while (marime < nr_bytes) {
			info_aux = (info_nod_t *)aux->data;
			info_nod_t *info_aux_next = (info_nod_t *)aux->next->data;
			if (aux->next && info_aux_next->adr_start ==
				info_aux->adr_start + info_aux->size && info_aux_next->data) {
				memcpy(propozitie + marime, info_aux_next->data,
					   nr_bytes - marime);
				//citirea se face din mai multe blocuri
				marime = marime + info_aux_next->size;
				aux = aux->next;
			} else {
				seg = 1;
				//cazul in care citesc de la o adresa nealocata
				printf("Segmentation fault (core dumped)\n");
				free(propozitie);
				dump_memory(sfl, alocat, info);
				return 0;
			}
		}
		if (seg == 0) {
			printf("%s\n", propozitie);
			free(propozitie);
		}
		break;
	}

	aux = aux->next;
}

if (ok == 0) {
	printf("Segmentation fault (core dumped)\n");
	dump_memory(sfl, alocat, info);
	return 0;
	}
	return 1;
}

void DESTROY_HEAP(segregated_free_list *sfl, lists *mem_alocata)
{
	for (int i = 0; i < sfl->nr_liste; i++) {
		dll_node_t *aux = sfl->vector_liste[i]->head;
		while (aux) {
			dll_node_t *aux1 = aux;
			aux = aux->next;
			if (aux1->data)
				free(aux1->data);
			free(aux1);
			aux1 = NULL;
		}
		free(sfl->vector_liste[i]);
		sfl->vector_liste[i] = NULL;
	}
	free(sfl->vector_liste);
	sfl->vector_liste = NULL;
	free(sfl);
	sfl = NULL;
	dll_node_t *aux = mem_alocata->head;
	while (aux) {
		dll_node_t *aux1 = aux;
		aux = aux->next;
		info_nod_t *info_aux1 = (info_nod_t *)aux1->data;
		if (info_aux1->data) {
			free(info_aux1->data);
			info_aux1->data = NULL;
		}
		if (aux1->data)
			free(aux1->data);
		free(aux1);
		aux1 = NULL;
	}
	free(mem_alocata);
	mem_alocata = NULL;
}

int main(void)
{
	segregated_free_list *sfl = NULL;
	lists *memorie_alocata = alocare();
	dump_t *info = calloc(1, sizeof(dump_t));
	while (1) {
		char comanda[20];
		scanf("%s", comanda);
		if (strcmp(comanda, "INIT_HEAP") == 0) {
			int adr_start, nr_liste, nr_bytes_lista, tip_rec;
			scanf("%x %d %d %d", &adr_start, &nr_liste,
				  &nr_bytes_lista, &tip_rec);
			info->mem_heap = nr_bytes_lista * nr_liste;
			sfl = creare_heap(adr_start, nr_liste, nr_bytes_lista, tip_rec);
		} else if (strcmp(comanda, "MALLOC") == 0) {
			int nr;
			scanf("%d", &nr);
			int ok = MALLOC(sfl, nr, memorie_alocata, &info->nr_frags);
			if (ok == 1)
				info->malloc_calls++;
		} else if (strcmp(comanda, "FREE") == 0) {
			int adr;
			scanf("%x", &adr);
			int ok = FREE(sfl, memorie_alocata, adr);
			if (ok == 1)
				info->free_calls++;
		} else if (strcmp(comanda, "READ") == 0) {
			int adr, nr;
			scanf("%x %d", &adr, &nr);
			int seg = read(memorie_alocata, adr, nr, sfl, info);
			if (seg == 0) {
				DESTROY_HEAP(sfl, memorie_alocata);
				free(info);
				break;
			}
		} else if (strcmp(comanda, "WRITE") == 0) {
			int adr;
			scanf("%x", &adr);
			char date[600];
			getchar();
			fgets(date, 600, stdin);
			char *p = strtok(date, "\"");
			char *text = calloc(1, strlen(p) + 1);
			DIE(!text, "calloc failed");
			strcpy(text, p);
			int nr_bytes = atoi(strtok(NULL, "\n "));
			if (nr_bytes > (int)strlen(text))
				nr_bytes = strlen(text);
			int seg = WRITE(memorie_alocata, adr, text, nr_bytes, sfl, info);
			if (seg == 0) {
				free(text);
				DESTROY_HEAP(sfl, memorie_alocata);
				free(info);
				break;
			}
			free(text);
		} else if (strcmp(comanda, "DUMP_MEMORY") == 0) {
			dump_memory(sfl, memorie_alocata, info);
		}
		if (strcmp(comanda, "DESTROY_HEAP") == 0) {
			DESTROY_HEAP(sfl, memorie_alocata);
			free(info);
			break;
		}
	}
}
