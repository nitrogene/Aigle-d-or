# Projet Aigle d'Or - Émulateur Oric Atmos

## Introduction
**Aigle d'Or** est un émulateur de l'Oric Atmos (ordinateur 8 bits emblématique des années 80), écrit en **C++17** pur pour la logique, et utilisant **Qt 6.11** pour l'interface graphique.  

Ce README sert à documenter la conception du projet, suivre son avancée et garder une trace des décisions prises lors de nos interactions successives.

## Architecture

Le projet adopte une séparation stricte (Modèle Core/GUI) :
1. **Core/** : La "carte mère" de l'émulateur. Aucune dépendance à Qt. Contient le matériel (CPU, RAM/ROM, ULA, VIA, Audio).
2. **GUI/** : Fenêtre principale (`QMainWindow`) Qt 6 qui récupère les inputs utilisateurs et dessine le buffer vidéo issu du Core.

### Choix Techniques Validés
- **Dynarec (JIT) éducatif** : Le CPU MOS 6502 (1 MHz) est recompilé afin de comprendre les mécanismes d'un Dynarec. L'état d'avancement du système est géré par un compteur de cycles permettant à l'horloge de respecter les timings de la machine d'origine.
- **Fast Load (TAPs)** : Chargement instantané des cassettes via patching de la ROM.
- **Vidéo** : Émulation des rendus de la puce ULA Oric vers un buffer en mémoire vidéo (copié dans une `QImage` Qt pour l'affichage).
- **Audio (PSG AY-3-8912)** : Synthétiseur ré-implémenté de zéro.

---

## 🛠 Roadmap

- `[x]` **Phase 1 : Architecture de Base**
  - Création des dossiers Core et GUI.
  - Squelette du CMakeLists.txt (Qt 6, C++17).
  - Validation du pipeline de compilation sous MSVC.
- `[x]` **Phase 2 : La Logique Mémoire et le CPU**
  - Implémenter le `MemoryBus` et le Memory Mapping complet de 64 Ko (Démarré).
  - Construire la matrice `Cpu6502.cpp` du 6502 (BCD, Flags) et les premiers opcodes.
- `[x]` **Phase 3 : Intégration Matérielle (Puces de base)**
  - ULA instanciée (Vidéo 240x224, Attributes, Qt Event Loop).
  - Branchement du BIOS (Fichier original de la ROM de l'Atmos 16 Ko).
- `[ ]` **Phase 4 : Interface et Chargement de TAPs**
  - Parseur de fichiers `.tap` (Fast load patch).
  - Lancement du projet par l'UI Qt, menus `Fichier > Ouvrir`.
- `[ ]` **Phase 5 : Audio**
  - Puce AY-3-8912 vers `QAudioSink`.

---

## 📔 Journal de Bord (Interactions)

### Mars 2026 - Initialisation
- **Cadrage du projet** : Discussions initiales sur le besoin (Qt 6 + Oric Atmos + Dynarec). Choix d'une architecture séparée robuste pour permettre au moteur de recompilation de tourner sans les verrous de l'UI.
- **Choix du compilateur / SDK** : Le CMake originel configurait les sources avec MinGW 64 (celui poussé par défaut par l'installateur Qt 6.10 local), ce qui causait des incompatibilités avec le compilateur MSVC imposé (`cl.exe`). **Décision :** Refuser la compatibilité bâtarde MinGW/MSVC et installer nativement **Qt 6.11 avec support MSVC 2022**.
- **Travail parallèle** : Création anticipée du module `MemoryBus` (48K RAM / 16K ROM, interception de `$0300`) en attendant le téléchargement de la toolchain Qt MSVC.
