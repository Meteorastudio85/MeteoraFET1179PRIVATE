meteoraFET1179 - compressore FET ibrido VST3 (Windows)
=====================================================

COMPILARE SU GITHUB (repository nuovo)
1. Estrai lo zip. Crea un repository nuovo, premi "uploading an existing file".
2. Apri la cartella estratta, seleziona tutto (Ctrl+A, compresa la cartella .github) e trascinalo. Commit changes.
   Il workflow e' gia' al suo posto e trova da solo CMakeLists.txt.
   (Solo se nell'elenco non compare .github/workflows/build.yml: Add file -> Create new file con quel nome
   e incolla il file build.yml che trovi nello zip.)
3. Actions -> spunta verde -> scarica "meteoraFET1179-VST3-Windows".
4. Chiudi la DAW, copia "meteoraFET1179.vst3" in C:\Program Files\Common Files\VST3, rifai la scansione.

CATENA DEL SEGNALE
INPUT -> Coils2 d'ingresso (leggerissimo, Cheapness 0)
-> [sotto HPF: senza compressione | tra HPF e LPF: Logical4 (FET) | sopra LPF: senza compressione] -> risommati
-> [curva Pultec fissa + Tube2, con il suo blend] -> Coils2 d'uscita -> OUTPUT -> DRY/WET master
HPF e LPF non filtrano il suono: decidono su quali frequenze lavora il compressore
(crossover Linkwitz-Riley 24 dB/ott, che risommati danno di nuovo il segnale intero).

COMANDI
- INPUT (grande): -24 / +24 dB, alza per comprimere di piu' (come sul 1176). La tacca chiara sul pomello indica la posizione.
- OUTPUT (grande): -24 / +24 dB.
- ATTACK e RELEASE: in senso orario sono piu' veloci, come sul 1176.
  Attack circa 50 ms -> 1.5 ms, release circa 1.3 s -> 60 ms (misurati con un gradino a 1 kHz).
- 4:1 / 8:1 / 12:1 / 20:1: rapporti di Logical4. ALL: "tutti i tasti" - si spinge di piu' nel FET
  e il trasformatore d'uscita satura di piu', suono piu' aggressivo e sporco.
- HI-PASS FILTER: OFF (tutto a sinistra), poi da 20 Hz a 20 kHz. Esempio: a 50 Hz il compressore lavora da 50 Hz in su,
  sotto 50 Hz il suono passa senza compressione.
- LO-PASS FILTER: OFF (tutto a sinistra), poi da 20 kHz a 20 Hz. Sopra quella frequenza il suono passa senza compressione.
- CURVE nella banda blu (sotto OUTPUT): blend del blocco Pultec + Tube2 (0 = senza, 10 = tutto).
- DRY/WET in basso a sinistra (accanto al nastro "11"): dry/wet totale del plugin.
- Interruttore POWER in basso a destra (la levetta originale della foto): a destra = ON, a sinistra = OFF
  (bypass con dissolvenza di 30 ms).
- VU: riduzione di guadagno del compressore, scala VU classica 20-10-7-5-3-2-1-0 (+1 +2 +3 in rosso),
  con il vetro davanti (riflessi, ombra della cornice, un filo di polvere).
  A riposo sullo 0, scende a sinistra quando comprime.
- Logo Meteora (M nel cerchio): quando il plugin lavora si accende come il filamento di una valvola,
  si scalda in circa un secondo e mezzo e tremola appena; insieme si accendono VU e spia rossa.
- Spia rossa, VU e logo si spengono
  in OFF, con il bypass della DAW o quando la DAW non manda audio.
- Si apre a 1050 x 732 (prima si apriva piccolissimo per un errore, ora corretto); ridimensionabile
  dall'angolo o col clic destro (50-150%), la misura scelta viene ricordata nel progetto. Grafica a risoluzione doppia, illuminata
  da una luce calda tipo abat-jour al centro del pannello.

MISURE (seno a 1 kHz, INPUT 0 dB, attack e release a meta')
Riduzione di guadagno a 4:1: circa -1.7 dB a -24 dBFS, -6.4 dB a -12 dBFS, -11.8 dB a -6 dBFS.
Logical4 non ha un rapporto fisso: piu' il segnale e' forte piu' comprime, e sui segnali molto forti
"schiaccia" (l'uscita non cresce piu'). I tasti scelgono quanto e' aggressivo.
Curva Pultec (blend al 100%): shelf alto analogico da 5 kHz: +1.3 dB a 3 kHz, +4.0 a 5 kHz, +4.6 a 6.5 kHz,
+4.1 a 8 kHz, +3.1 a 12 kHz, +2.6 a 16 kHz; -0.3 dB a 1.2 kHz; bassi invariati.
HPF a 50 Hz, tono forte a -6 dBFS: a 30 Hz l'uscita passa da -12 dB (compresso) a +0.2 dB (non compresso);
a 1 kHz la compressione resta identica. A livello basso il suono non cambia (entro 0.3 dB);
con il dry/wet a meta' non si crea nessun buco alla frequenza di taglio.
Tube2 riportato a guadagno unitario; distorsione circa 0.7% a -12 dBFS.

LICENZE: vedi THIRD_PARTY_NOTICES.txt. JUCE 8 e' AGPLv3 o commerciale.
