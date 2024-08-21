#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct listaIngredienti {
    char nome[255];
    int quantita;
    struct listaIngredienti* next;
}listaIng;

typedef struct listaRicette {
    char nome[255];
    listaIng* ingredienti;
    struct listaRicette* next;
} listaRic;

typedef struct ingredientiMagazzino{
    char nome[255];
    int quantita; // TOTAL QUANTITY
    int scadenza;
    struct listaIngredienti* next; // Con lo stesso nome // TODO: usare un HEAP
}ingMagazzino;

// HASH MAP per il magazzino
typedef struct magazzino {
    ingMagazzino *ingredienti[63];  // Un array di puntatori alle liste di ingredienti
} magazzino;

// burro ->  ingredientiMagazzino che si chiama burro {
//
// }

typedef struct ordine {
    int tempo_arrivo;
    char nome_ricetta[255]; // Potrebbe avere senso avere un * alla ricetta
    int quantita;
    int pesototale;
    struct ordine* next;
} ordine;


// CODA ORDINI IN SOSPESO
// CODA ORDINI IN CAMIONCINO


void aggiungiRicetta(listaRic** head, char* comando);
void rimuoviRicetta(listaRic** head, char* comando, ordine* ordiniInSospeso, ordine* camioncino);
void stampaRicette(listaRic* head);
void aggiungiIngredienti(listaIng** head, char* comando, int* posizione);
int comparazione(char* comando);
int verificaRicetta(listaRic* head, char* comando, int posizione);
int indiceCarattere(char c);
void inizializzaMagazzino(magazzino* mag);
void rifornimento(magazzino* mag, char* comando);
void stampaMagazzino(magazzino* mag, int n);
void processaOrdine(listaRic* head, magazzino* mag, int tempo, char* comando, ordine** ordiniInSospeso, ordine** camioncino);
void eliminaIngredientiScaduti(magazzino* mag, int giorno);
ingMagazzino* unisciListe(ingMagazzino* lis1, ingMagazzino* lis2);
ingMagazzino* mergeSort(ingMagazzino* testa);
void dividiLista(ingMagazzino* testa, ingMagazzino** meta1, ingMagazzino** meta2);
void aggiungiOrdineInSospeso(ordine** head, int tempo, char* nome_ricetta, int numero_elementi, int pesoTotale);
void stampaOrdiniInAttesa(ordine* head);
void spedizione(ordine** camioncino, int peso);
ordine* sortOrdersByWeight(ordine* head);
void controllaOrdiniInsospeso(listaRic* head, magazzino* mag, int giorno, ordine** ordineDaControllare, ordine** camioncino);
void rimuoviElemento(ordine** head, ordine* elemento);
void freeOrdini(ordine* head);
void freeMagazzino(magazzino* mag);
void freeRicette(listaRic* head);
void freeIngredienti(listaIng* head);


int main() {
    char comando[1024];
    listaRic* head = NULL;

    // MAGAZZINO
    magazzino mag;
    ordine* ordiniInSospeso = NULL;
    ordine* camioncino = NULL;
    inizializzaMagazzino(&mag);
    int ciclo=0;
    int giorno=0;
    int peso=0;
    scanf("%d %d", &ciclo, &peso);
    getchar();

    while (1)
    {
        if (fgets(comando, sizeof(comando), stdin) == NULL) {
            break;
        }
        comando[strcspn(comando, "\n")] = '\0';  // Rimuove il newline finale se presente

        if (strcmp(comando, "-1") == 0) {
            break;
        }

        char cmd[255];
        sscanf(comando, "%s", cmd);

        int cmdType = comparazione(cmd);

        if (cmdType == 1) {
            aggiungiRicetta(&head, comando);
        } else if (cmdType == 2) {
            rimuoviRicetta(&head, comando, ordiniInSospeso, camioncino);

        } else if (cmdType == 3) {
            rifornimento(&mag, comando);
            //stampaMagazzino(&mag, ciclo);
            controllaOrdiniInsospeso(head, &mag, giorno, &ordiniInSospeso, &camioncino);

        } else if (cmdType == 4) {
            processaOrdine(head, &mag, giorno, comando, &ordiniInSospeso, &camioncino);

        }else {
            printf("comando non riconosciuto\n");
        }
        giorno++;

        printf("siamo al giorno %d \n", giorno);
        if(giorno % ciclo == 0 && (giorno!=0)) {
            spedizione(&camioncino, peso);
        }

    }

    //stampaRicette(head);
    freeRicette(head);
    freeMagazzino(&mag);
    freeOrdini(ordiniInSospeso);
    freeOrdini(camioncino);

    return 0;
}


void aggiungiIngredienti(listaIng** head, char* comando, int* posizione) {
    listaIng* new;
    char nome[255];
    int quantita;


    while (sscanf(comando + *posizione, "%s %d", nome, &quantita) == 2) {
        new = malloc(sizeof(listaIng));
        if (!new) {
            printf("Errore di allocazione memoria\n");
            exit(1);
        }
        strcpy(new->nome, nome);
        new->quantita = quantita;
        new->next = *head;
        *head = new;

        while (comando[*posizione] != '\0' && comando[*posizione] != ' ') {
            (*posizione)++;
        }

        while (comando[*posizione] == ' ') {
            (*posizione)++;
        }

        while (comando[*posizione] != '\0' && comando[*posizione] != ' ') {
            (*posizione)++;
        }

        while (comando[*posizione] == ' ') {
            (*posizione)++;
        }
    }
}

void aggiungiRicetta(listaRic** head, char* comando) {
    int posizione = strlen("aggiungi_ricetta") + 1;

    // Verifica se la ricetta è già presente
    if (verificaRicetta(*head, comando, posizione) == 1)
        return;
    // Estrae il nome della ricetta dalla stringa di comando

    listaRic* new = malloc(sizeof(listaRic));
    if (!new) {
        printf("Errore di allocazione memoria\n");
        exit(1);
    }

    new->ingredienti = NULL;
    new->next = NULL;

    sscanf(comando + posizione, "%s", new->nome);
    posizione += strlen(new->nome) + 1;

    // Aggiungi gli ingredienti alla ricetta
    aggiungiIngredienti(&new->ingredienti, comando, &posizione);

    // Inserisci la nuova ricetta nella lista
    if (*head == NULL) {
        *head = new;
    } else {
        new->next = *head;
        *head = new;
    }

}



void rimuoviRicetta(listaRic** head, char* comando, ordine* ordiniInSospeso, ordine* camioncino) {
    //qui va aggiunto un controllo sugli ordini in sospeso!!!
    int posizione = strlen("rimuovi_ricetta") + 1;
    char nomeRicetta[255];
    sscanf(comando + posizione, "%s", nomeRicetta);

    listaRic* temp = *head;
    listaRic* prev = NULL;

    while (camioncino != NULL){
        if(strcmp(camioncino->nome_ricetta, nomeRicetta) == 0){
            printf("ordini in sospeso\n");
            return;
        }
        camioncino = camioncino->next;
    }

    while (ordiniInSospeso != NULL){
        if(strcmp(ordiniInSospeso->nome_ricetta, nomeRicetta) == 0){
            printf("ordini in sospeso\n");
            return;
        }
        ordiniInSospeso = ordiniInSospeso->next;
    }

    while (temp != NULL) {
        if (strcmp(temp->nome, nomeRicetta) == 0) {
            // Rimozione della ricetta
            if (prev == NULL) {
                *head = temp->next;
            } else {
                prev->next = temp->next;
            }
            free(temp);
            printf("rimossa\n");
            return;
        }
        prev = temp;
        temp = temp->next;
    }
    printf("non presente\n");
}

void stampaRicette(listaRic* head) {
    listaRic* temp = head;
    while (temp != NULL) {
        printf("Ricetta: %s\n", temp->nome);
        listaIng* temp2 = temp->ingredienti;
        while (temp2 != NULL) {
            printf("Ingrediente: %s, Quantita: %d\n", temp2->nome, temp2->quantita);
            temp2 = temp2->next;
        }
        temp = temp->next;
    }
}

int comparazione(char* comando) {

    if (strcmp(comando, "aggiungi_ricetta") == 0) {
        return 1;
    }
    if (strcmp(comando, "rimuovi_ricetta") == 0) {
        return 2;
    }
    if (strcmp(comando, "rifornimento") == 0) {
        return 3;
    }
    if (strcmp(comando, "ordine") == 0) {
        return 4;
    }
    return 5;
}

int verificaRicetta(listaRic* head, char* comando, int posizione) {
    listaRic* temp = head;
    char nomeRicetta[255];

    // Estrae il nome della ricetta dal comando a partire dalla posizione data
    sscanf(comando + posizione, "%s", nomeRicetta);

    // Scorre la lista delle ricette per verificare se esiste già una ricetta con lo stesso nome
    while (temp != NULL) {
        if (strcmp(temp->nome, nomeRicetta) == 0) {
            printf("ignorato\n");
            return 1;  // Esce dalla funzione se la ricetta è già presente
        }
        temp = temp->next;
    }

    // Se non esiste già, stampa "aggiunta"
    printf("aggiunta\n");


    return 0;
}


//------------------MAGAZZINO---------------------

// Funzione di hash per determinare l'indice dell'array in base al carattere iniziale
int indiceCarattere(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a';          // 'a' -> 0, 'b' -> 1, ..., 'z' -> 25
    } else if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 26;     // 'A' -> 26, 'B' -> 27, ..., 'Z' -> 51
    } else if (c >= '0' && c <= '9') {
        return c - '0' + 52;     // '0' -> 52, '1' -> 53, ..., '9' -> 61
    }else if (c == '_') {
        return 62;               // Indice per '_'
    }
    return -1;  // Carattere non valido
}

// Inizializza il magazzino impostando tutti i puntatori a NULL
void inizializzaMagazzino(magazzino* mag) {
    for (int i = 0; i < 63; i++) {
        mag->ingredienti[i] = NULL;
    }
}


void rifornimento(magazzino* head, char* comando) {
    char nome[255];
    int quantita;
    int scadenza;
    int posizione = strlen("rifornimento") + 1;



    while (sscanf(comando + posizione, "%s %d %d", nome, &quantita, &scadenza) == 3)
    {
        // Crea un nuovo nodo per l'ingrediente
        ingMagazzino* new = malloc(sizeof(ingMagazzino));
        if (!new) {
            printf("Errore di allocazione memoria\n");
            exit(1);
        }
        new->next = NULL;
        strcpy(new->nome, nome);
        new->quantita = quantita;
        new->scadenza = scadenza;

        // Calcola l'indice basato sul primo carattere del nome
        int indice = indiceCarattere(nome[0]);

        if (indice == -1) {
            printf("Nome ingrediente non valido: %s\n", nome);
            free(new); // Libera la memoria in caso di errore
            return;
        }

        // Inserisci l'ingrediente nella lista appropriata
        if (head->ingredienti[indice] == NULL) {
            head->ingredienti[indice] = new;
        } else {
            new->next = head->ingredienti[indice];
            head->ingredienti[indice] = new;
        }

        // Avanza la posizione nella stringa del comando
        while (comando[posizione] != '\0' && comando[posizione] != ' ') {
            posizione = posizione + 1;

        }
        // Salta gli spazi successivi
        while (comando[posizione] == ' ') {
            posizione = posizione + 1;
        }

        while (comando[posizione] != '\0' && comando[posizione] != ' ') {
            posizione = posizione + 1;
        }

        while (comando[posizione] == ' ') {
            posizione = posizione + 1;
        }

        while (comando[posizione] != '\0' && comando[posizione] != ' ') {
            posizione = posizione + 1;
        }

        while (comando[posizione] == ' ') {
            posizione = posizione + 1;
        }
    }

    // Stampa conferma di rifornimento
    printf("rifornito\n");
}



void stampaMagazzino(magazzino* mag,int n) {
    printf("Contenuto del magazzino:\n");

    for (int i = 0; i < 63; i++)
    {
        ingMagazzino* curr = mag->ingredienti[i];

        if (curr != NULL) {
            // Converti l'indice in un carattere iniziale
            char iniziale;
            if (i >= 0 && i <= 25) {
                iniziale = 'a' + i; // Lettere minuscole
            } else if (i >= 26 && i <= 51) {
                iniziale = 'A' + (i - 26); // Lettere maiuscole
            } else if (i >= 52 && i <= 61){
                iniziale = '0' + (i - 52); // Numeri
            } else {
                iniziale = '_';          // Underscore
            }

            printf("Lista per iniziale '%c':\n", iniziale);
            while (curr != NULL) {
                printf("Ingrediente: %s, Quantita: %d, Scadenza: %d\n", curr->nome, curr->quantita, curr->scadenza);
                curr = curr->next;
            }
        }
    }
}

//------------------ORDINI---------------------
void eliminaIngredientiScaduti(magazzino* mag, int giorno)
{
    for (int i = 0; i < 63; i++) {
        ingMagazzino* pos = mag->ingredienti[i];
        ingMagazzino* prec = NULL;

        while (pos != NULL) {
            if (pos->scadenza < giorno) {
                if (prec == NULL)
                    mag->ingredienti[i] = pos->next;
                else
                    prec->next = pos->next;

                ingMagazzino* temp = pos;
                pos = pos->next;
                free(temp);
            } else {
                prec = pos;
                pos = pos->next;
            }
        }
    }
}

void aggiungiOrdineInSospeso(ordine** head, int tempo, char* nome_ricetta, int numero_elementi, int pesoTotale) {
    ordine* new = (ordine*)malloc(sizeof(ordine));
    if (new == NULL) {
        printf("Errore di allocazione memoria\n");
        return;  // Exit the function instead of the program
    }
    new->tempo_arrivo = tempo;
    strcpy(new->nome_ricetta, nome_ricetta);
    new->quantita = numero_elementi;
    new->next = NULL;
    new->pesototale = pesoTotale;

    if (*head == NULL || (*head)->tempo_arrivo > tempo) {
        new->next = *head;
        *head = new;
    } else {
        ordine* current = *head;
        while (current->next != NULL && current->next->tempo_arrivo <= tempo) {
            current = current->next;
        }
        new->next = current->next;
        current->next = new;
    }
}


void processaOrdine(listaRic* head, magazzino* mag, int giorno, char* comando, ordine** ordiniInSospeso, ordine** camioncino) {
    char nomeRicetta[255];
    int numero_elementi;
    int posizione = strlen("ordine") + 1;
    int peso_totale = 0;  // Initialize total weight
    int flag = 0;

    // Elimina ingredienti scaduti e ordina il magazzino
    eliminaIngredientiScaduti(mag, giorno);
    for (int i = 0; i < 63; i++) {
        mag->ingredienti[i] = mergeSort(mag->ingredienti[i]);
    }

    sscanf(comando + posizione, "%s %d", nomeRicetta, &numero_elementi);

    listaRic* temp = head;
    while (temp != NULL && strcmp(temp->nome, nomeRicetta) != 0) {
        temp = temp->next;
    }

    if (temp == NULL) {
        printf("rifiutato\n");
        return;  // Exit the function instead of the program
    }

    listaIng* ingredienti = temp->ingredienti;
    while (ingredienti != NULL) {
        int indice = indiceCarattere(ingredienti->nome[0]);
        ingMagazzino* lista_mag = mag->ingredienti[indice];
        int quantita_tot = 0;

        while (lista_mag != NULL) {
            if (strcmp(lista_mag->nome, ingredienti->nome) == 0) {
                quantita_tot += lista_mag->quantita;
            }
            lista_mag = lista_mag->next;
        }

        if (quantita_tot < (ingredienti->quantita) * (numero_elementi)) {
            flag = 1;
        }

        peso_totale += (ingredienti->quantita) * numero_elementi;  // Add ingredient weight to total weight
        ingredienti = ingredienti->next;
    }

    if (flag == 1) {
        aggiungiOrdineInSospeso(ordiniInSospeso, giorno, nomeRicetta, numero_elementi, peso_totale);
        printf("accettato\n");
        return;  // Exit the function instead of the program
    }

    ingredienti = temp->ingredienti;
    while (ingredienti != NULL) {
        int indice = indiceCarattere(ingredienti->nome[0]);
        ingMagazzino* lista_mag = mag->ingredienti[indice];
        int quantita_richiesta = ingredienti->quantita * numero_elementi;

        while (quantita_richiesta > 0 && lista_mag != NULL) {
            if (strcmp(lista_mag->nome, ingredienti->nome) == 0) {
                if (lista_mag->quantita <= quantita_richiesta) {
                    quantita_richiesta -= lista_mag->quantita;
                    ingMagazzino* temp = lista_mag;
                    lista_mag = lista_mag->next;
                    free(temp);
                } else {
                    lista_mag->quantita -= quantita_richiesta;
                    quantita_richiesta = 0;
                }
            } else {
                lista_mag = lista_mag->next;
            }
        }

        mag->ingredienti[indice] = lista_mag;
        ingredienti = ingredienti->next;
    }

    ordine* newOrder = (ordine*)malloc(sizeof(ordine));
    if (newOrder == NULL) {
        printf("Errore di allocazione memoria\n");
        return;  // Exit the function instead of the program
    }
    newOrder->tempo_arrivo = giorno;
    strcpy(newOrder->nome_ricetta, nomeRicetta);
    newOrder->quantita = numero_elementi;
    newOrder->pesototale = peso_totale;
    newOrder->next = NULL;

    // Add the new order to the camioncino list
    if (*camioncino == NULL) {
        *camioncino = newOrder;
    } else {
        ordine* current = *camioncino;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newOrder;
    }

    printf("accettato\n");
}

void dividiLista(ingMagazzino* testa, ingMagazzino** meta1, ingMagazzino** meta2) {
    if (testa == NULL || testa->next == NULL) {
        *meta1 = testa;
        *meta2 = NULL;
        return;
    }
    ingMagazzino* pt1 = testa;
    ingMagazzino* pt2 = testa->next;
    while (pt2 != NULL && pt2->next != NULL) {
        pt1 = pt1->next;
        pt2 = pt2->next->next;
    }
    *meta1 = testa;
    *meta2 = pt1->next;
    pt1->next = NULL;
}

ingMagazzino* mergeSort(ingMagazzino* testa) {
    if (testa == NULL || testa->next == NULL) {
        return testa;
    }
    ingMagazzino* meta1;
    ingMagazzino* meta2;
    dividiLista(testa, &meta1, &meta2);
    meta1 = mergeSort(meta1);
    meta2 = mergeSort(meta2);
    return unisciListe(meta1, meta2);
}

ingMagazzino* unisciListe(ingMagazzino* lis1, ingMagazzino* lis2) {
    ingMagazzino tmp;
    ingMagazzino* coda = &tmp;
    while (lis1 != NULL && lis2 != NULL) {
        if (strcmp(lis1->nome, lis2->nome) < 0 ||
            (strcmp(lis1->nome, lis2->nome) == 0 && lis1->scadenza <= lis2->scadenza)) {
            coda->next = lis1;
            lis1 = lis1->next;
        } else {
            coda->next = lis2;
            lis2 = lis2->next;
        }
        coda = coda->next;
    }
    coda->next = (lis1 == NULL) ? lis2 : lis1;
    return tmp.next;
}

void stampaOrdiniInAttesa(ordine* head) {
    ordine* current = head;
    while (current != NULL) {
        printf("Tempo di arrivo: %d, Nome ricetta: %s, Quantita: %d\n", current->tempo_arrivo, current->nome_ricetta, current->quantita);
        current = current->next;
    }
}

//------------------CAMIONCINO---------------------

// Sort the orders by weight in descending order and if they have the same weight, sort them by arrival time
ordine* sortOrdersByWeight(ordine* head) {
    ordine* sorted = NULL;
    ordine* current = head;
    while (current != NULL) {
        ordine* next = current->next;
        if (sorted == NULL || sorted->pesototale < current->pesototale) {
            current->next = sorted;
            sorted = current;
        } else {
            ordine* temp = sorted;
            while (temp->next != NULL && temp->next->pesototale >= current->pesototale) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
        }
        current = next;
    }
    return sorted;
}

void rimuoviElemento(ordine** head, ordine* elemento) {
    ordine* temp = *head;
    ordine* prev = NULL;
    while(temp != NULL && temp != elemento){
        prev = temp;
        temp = temp->next;
    }
    if(temp == NULL){
        return;
    }
    if(prev == NULL){
        *head = temp->next;
    }else{
        prev->next = temp->next;
    }
    free(temp);
}

void spedizione(ordine** camioncino, int peso_massimo) {
    int peso_corrente = 0;
    ordine* current = *camioncino;
    ordine* selectedOrders = NULL;
    ordine* prev = NULL; // Puntatore per tenere traccia del nodo precedente

    if (current == NULL) {
        printf("camioncino vuoto\n");
        return;
    }

    while (current != NULL) {
        if (peso_corrente + current->pesototale <= peso_massimo) {
            peso_corrente += current->pesototale;
            // Aggiungi l'ordine alla lista degli ordini selezionati
            aggiungiOrdineInSospeso(&selectedOrders, current->tempo_arrivo, current->nome_ricetta, current->quantita, current->pesototale);

            printf("Ordine spedito: Giorno di arrivo: %d, Nome ricetta: %s, Quantita: %d, Peso totale: %d grammi\n",
                   current->tempo_arrivo, current->nome_ricetta, current->quantita, current->pesototale);

            // Rimuovi l'ordine dalla lista del camioncino
            if (prev == NULL) {
                *camioncino = current->next; // L'ordine è all'inizio della lista
            } else {
                prev->next = current->next; // L'ordine non è all'inizio della lista
            }
            ordine* temp = current;
            current = current->next;
            free(temp);
        } else {
            prev = current;
            current = current->next;
        }
    }

    // Ordina la lista selectedOrders in base al peso e tempo di arrivo
    selectedOrders = sortOrdersByWeight(selectedOrders);


    printf("Spedizione completata. Peso totale spedito: %d grammi\n", peso_corrente);
}


//Controllo degli ordini in sospeso
//1. ogni volta che c'è un rifornimento controlla se ci sono ordini in sospeso che possono essere evasi
//2. se ci sono ordini in sospeso che possono essere evasi, li evadi e li metti nel camioncino
//3. se ci sono ordini in sospeso che non possono essere evasi rimangono in sospeso
//4. se non ci sono ordini in sospeso non fa nulla

void controllaOrdiniInsospeso(listaRic* head, magazzino* mag, int giorno, ordine** ordineDaControllare, ordine** camioncino) {
    // Elimino gli ingredienti scaduti e ordino quelli rimasti
    eliminaIngredientiScaduti(mag, giorno);
    for (int i = 0; i < 63; i++) {
        mag->ingredienti[i] = mergeSort(mag->ingredienti[i]);
    }

    ordine* ptr = *ordineDaControllare;
    ordine* ordineSuccessivo = NULL;

    while (ptr != NULL) {
        ordineSuccessivo = ptr->next; // Salvo il prossimo ordine prima di potenzialmente rimuovere l'attuale
        listaRic* temp = head;
        int flag = 0;
        int peso_totale = 0;
        char nomeRicetta[255];
        strcpy(nomeRicetta, ptr->nome_ricetta);
        int numero_elementi = ptr->quantita;

        // Cerca la ricetta corrispondente nel magazzino
        while (temp != NULL && strcmp(temp->nome, nomeRicetta) != 0) {
            temp = temp->next;
        }
        if (temp == NULL) {
            printf("Ricetta non trovata\n");
            ptr = ordineSuccessivo; // Passa all'ordine successivo
            continue;
        }
        // Calcola il peso totale della ricetta e verifica la disponibilità
        listaIng* ingredienti = temp->ingredienti;
        while (ingredienti != NULL) {
            int indice = indiceCarattere(ingredienti->nome[0]);
            ingMagazzino* lista_mag = mag->ingredienti[indice];
            int quantita_tot = 0;
            // Calcola la quantità totale disponibile nel magazzino
            while (lista_mag != NULL) {
                if (strcmp(lista_mag->nome, ingredienti->nome) == 0) {
                    quantita_tot += lista_mag->quantita;
                }
                lista_mag = lista_mag->next;
            }
            if (quantita_tot < ingredienti->quantita * numero_elementi) {
                flag = 1;
            }
            peso_totale += (ingredienti->quantita * numero_elementi);
            ingredienti = ingredienti->next;
        }
        // Se non ci sono abbastanza ingredienti, lascia l'ordine in sospeso
        if (flag == 1) {
            ptr = ordineSuccessivo; // Passa all'ordine successivo
            continue;
        }
        // Rimuovi gli ingredienti necessari dal magazzino
        ingredienti = temp->ingredienti;
        while (ingredienti != NULL) {
            int indice = indiceCarattere(ingredienti->nome[0]);
            ingMagazzino* lista_mag = mag->ingredienti[indice];
            int quantita_richiesta = ingredienti->quantita * numero_elementi;

            while (quantita_richiesta > 0 && lista_mag != NULL) {
                if (strcmp(lista_mag->nome, ingredienti->nome) == 0) {
                    if (lista_mag->quantita <= quantita_richiesta) {
                        quantita_richiesta -= lista_mag->quantita;
                        ingMagazzino* temp = lista_mag;
                        lista_mag = lista_mag->next;
                        free(temp);
                    } else {
                        lista_mag->quantita -= quantita_richiesta;
                        quantita_richiesta = 0;
                    }
                } else {
                    lista_mag = lista_mag->next;
                }
            }
            mag->ingredienti[indice] = lista_mag;
            ingredienti = ingredienti->next;
        }
        // Aggiungi l'ordine al camioncino
        ordine* newOrder = (ordine*)malloc(sizeof(ordine));
        if (newOrder == NULL) {
            printf("Errore di allocazione memoria\n");
            return;
        }
        newOrder->tempo_arrivo = giorno;
        strcpy(newOrder->nome_ricetta, nomeRicetta);
        newOrder->quantita = numero_elementi;
        newOrder->pesototale = peso_totale;
        newOrder->next = NULL;

        if (*camioncino == NULL) {
            *camioncino = newOrder;
        } else {
            ordine* current = *camioncino;
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = newOrder;
        }

        rimuoviElemento(ordineDaControllare, ptr); // Rimuovi l'ordine corrente
        ptr = ordineSuccessivo; // Passa all'ordine successivo
    }
}


void freeOrdini(ordine* head) {
    ordine* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void freeMagazzino(magazzino* mag) {
    for (int i = 0; i < 63; i++) {
        ingMagazzino* curr = mag->ingredienti[i];
        ingMagazzino* temp;
        while (curr != NULL) {
            temp = curr;
            curr = curr->next;
            free(temp);
        }
    }
}

void freeRicette(listaRic* head) {
    listaRic* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        freeIngredienti(temp->ingredienti);  // Libera la lista di ingredienti
        free(temp);
    }
}

void freeIngredienti(listaIng* head) {
    listaIng* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}
