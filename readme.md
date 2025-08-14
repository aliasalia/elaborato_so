# Repo for OS project - Year 2025

## Implementazione di **Specifiche opzione 2 (Pthread e FIFO su Linux / macOS)**

Modello di trasferimento: il client invia al server il percorso del file e riceve l'impronta.

Per la sufficienza:

- Implementare server che riceve richieste ed invia risposte, usando FIFO
- Implementare client che invia richiesta e riceve risposta, usando FIFO
- Istanziare thread distinti per elaborare richieste multiple in modo concorrente

Per arrivare al massimo voto:

- Schedulare le richieste pendenti in ordine di dimensione del file
- Introdurre un limite al numero di thread in esecuzione fissato
- Implementare il caching in memoria delle coppie percorso-hash già servite, così da restituire i valori computati nel caso di percorsi ripetuti
- Gestire richieste multiple simultanee per un dato percorso processando una sola richiesta ed attendendo il risultato nelle restanti richieste

## Compilazione del progetto

1. Spostarsi nella cartella principale os_project_2025
2. Creare la cartella *build* -> `mkdir build`
3. Spostarsi in *build* -> `cd build`
4. Lanciare il comando `cmake ..`
5. Compilare con `make`