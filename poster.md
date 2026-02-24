# Conception et implémentation d’un looper audio temps réel embarqué sur microcontrôleur

## Problématique

Comment implémenter un **système de bouclage audio multitrack temps réel** sur microcontrôleur, en garantissant :

* une synchronisation stricte entre pistes,
* une latence minimale,
* une stabilité temporelle sur longue durée,
* et un fonctionnement autonome sans ordinateur ?

## Définition fonctionnelle

Un looper est un système d’enregistrement et de restitution audio cyclique permettant la superposition incrémentale de couches sonores synchronisées.

Contrairement à une simple lecture en boucle, un looper multitrack impose :

* une cohérence temporelle entre flux audio indépendants,
* une gestion dynamique des enregistrements,
* un mixage temps réel stable.

![dessin_looper](/home/elie/insa/3TC/son/Projet-SON/looper_dessin.png)

## Architecture matérielle

* Microcontrôleur : **Teensy 4.0**
* Codec audio SGTL5000 (ADC/DAC 16 bits, 44.1 kHz)
* Stockage : carte SD 16 Go
* Interface utilisateur : boutons + potentiomètres
* Le tout est monté sur breadboard avec câblages

![schema_elec_looper](/home/elie/insa/3TC/son/Projet-SON/schema_elec_looper.png)

##  Architecture logicielle

### Pipeline audio

```
Entrée micro
   ↓
ADC (44.1 kHz, 16 bits)
   ↓
Buffer audio (AudioRecordQueue)
   ↓
Écriture SD (.raw)
   ↓
Lecture SD (AudioPlaySdRaw)
   ↓
Mixeur numérique
   ↓
DAC
   ↓
Sortie audio
```

Le traitement repose sur la **Teensy Audio Library**, organisée en blocs de 128 échantillons.

## ⚠ Contraintes d’ingénierie

### 1. Contraintes temporelles

* Fréquence d’échantillonnage : 44.1 kHz
* Bloc audio : 128 samples
* Période d’interruption ≈ 2.9 ms

Le système doit :

* Lire et Écrire sur SD
* Mixer jusqu’à 4 pistes
* Gérer l’interface utilisateur

Tout ces "processus" doivent se faire de manière synchronisée, en réduisant le décalage au minimum, et souvent de manière simultanée.

### 2. Contraintes mémoire

* RAM limitée (1 Mo) 
* stockage des pistes sur carte SD

### 3. Problème de la Synchronisation

L’utilisateur ne peut pas déclencher l’enregistrement exactement au début de la boucle.

Solution adoptée :

* La première piste définit une durée de référence `loopLength`
* Mesure via horloge microseconde (`micros()`)
* Les overdubs sont automatiquement tronqués à cette durée
* Le rebouclage est piloté par comparaison temporelle :

```c++
if (temps_actuel > début_piste + loopLength)
    relancer lecture
```

Approche : **synchronisation temporelle absolue**, et non indexée sur la position du fichier audio.

![schema_implantation_pistes](/home/elie/insa/3TC/son/Projet-SON/sync_pistes_schema.png)

## Performances mesurées (bullshit)

* Échantillonnage : 44.1 kHz
* Résolution : 16 bits
* Nombre maximal de pistes : 4
* Stockage : flux brut PCM (.raw)
* Latence perçue : < 10 ms

## Analyse critique

### Points forts

* Architecture simple et robuste
* Séparation claire acquisition / stockage / lecture
* Scalabilité jusqu’à 4 pistes sans instabilité

### Limites

* Synchronisation basée sur horloge logicielle → dérive possible à très long terme
* Pas de quantification au tempo
* Risque de saturation lors du mixage (somme linéaire)
* Pas d’horloge audio matérielle partagée entre lecture et enregistrement

## Perspectives d'amélioration

* Synchronisation basée sur compteur d’échantillons (sample-accurate looping)
* Implémentation d’un scheduler temps réel plus déterministe
* Buffer circulaire en RAM + écriture asynchrone SD
* Gestion dynamique du gain (compression / normalisation)
* Implémentation d’effets DSP embarqués (reverb, delay, transposition, filtrages, etc)

Ces améliorations paraissent réalisables, mais nécessiteraient un matériel plus performant en terme de RAM notamment.
