# Detection de contours avec OpenCV

Application C++ pour comparer Sobel, le Laplacien et Canny sur des images
annotees. Une interface permet de regler les seuils et de comparer les contours
obtenus aux references. Le traitement par lot exporte les images et les mesures.

## Compilation et execution

Prerequis : Linux avec une session graphique, un compilateur C++17, Make,
pkg-config et OpenCV 4. Sous Windows, utiliser WSL avec une interface graphique.

Sur Ubuntu :

```bash
sudo apt install g++ make pkg-config libopencv-dev
make
make run
```

Le depot contient 40 images dans `images/` et leurs annotations dans `images/gt/`.
Pour utiliser un autre jeu de donnees :

```bash
./contours chemin/images chemin/references resultats
```

Les extensions reconnues sont `.png`, `.jpg`, `.jpeg`, `.bmp` et `.pgm`
(en minuscules). Les images et references doivent avoir les memes dimensions.
La reference porte le meme nom que l'image ou le suffixe `_gt_binary.pgm` :
`basket.pgm` correspond ainsi a `basket_gt_binary.pgm`.

## Utilisation

- Detecteur : 0 = Sobel, 1 = Laplacien, 2 = Canny.
- Curseurs : seuils Sobel et Laplacien, seuils bas et haut de Canny.
- **N / P** : image suivante / precedente.
- **B** : traiter toutes les images avec les trois detecteurs.
- **Echap** : quitter.

Les trois methodes utilisent un flou gaussien 3x3. Les references sont binarisees
au seuil 127, avec des traits noirs sur fond blanc. L'interface affiche les
mesures avec une tolerance de localisation d'un pixel (voisinage 3x3).

Le traitement par lot est synchrone : attendre le message de fin dans le terminal.
Les paires illisibles sont signalees et ignorees ; le nombre de paires traitees
est indique en fin d'execution.

## Resultats

Le dossier de sortie contient `mesures.csv` et trois sous-dossiers d'images :
`Sobel/`, `Laplacien/` et `Canny/`. Chaque paire valide produit six lignes CSV :
trois detecteurs, chacun evalue en correspondance exacte et avec tolerance 3x3.
Un lot complet de 40 paires produit donc 240 lignes de donnees.

Un nouveau traitement dans le meme dossier remplace les resultats precedents.
Pour conserver plusieurs experiences :

```bash
make run OUTPUT=resultats_experience_1
```

Pour D pixels detectes, R pixels de reference et C correspondances :

- FP = D - C ; FN = R - C.
- Performance = C / (C + FP + FN).
- TFP = FP / (C + FP + FN).
- TFN = FN / (C + FP + FN).

Ces ratios suivent les definitions du TP. Un denominateur nul donne zero.
L'appariement avec tolerance conserve d'abord les correspondances exactes,
puis associe chaque detection restante au premier voisin libre. Chaque pixel
de reference est utilise au plus une fois. Cette methode gloutonne depend de
l'ordre de parcours et ne garantit pas le maximum de correspondances.

## Organisation

- `contours.cpp` : detection, evaluation, interface et export CSV.
- `Makefile` : compilation et execution.
- `EXPLICATIONS.md` : description des fonctions et des mesures.
- `images/` : images du TP et references.
- `.github/workflows/build.yml` : verification de compilation sous Linux.

Les executables et resultats locaux sont exclus du suivi Git.
