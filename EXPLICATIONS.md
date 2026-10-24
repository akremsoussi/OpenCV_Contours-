# Lire le code, etape par etape

## 1. Reglages

Une structure est simplement un regroupement de variables. Au lieu d'avoir des
variables eparpillees, `r.sobel` donne le seuil Sobel et `r.cannyBas` le seuil bas de Canny.
Les trackbars modifient ces valeurs a chaque tour de boucle.

## 2. Detecteurs

`detectSobel`, `detectLaplacian` et `detectCanny` implementent les trois detecteurs. Chacune retourne une image 0=fond et 255=contour.

- Sobel : derivees premieres gx/gy, norme, puis seuil.
- Laplacien : somme des derivees secondes, valeur absolue, puis seuil.
- Canny : gradient, suppression des non-maxima, puis hysteresis a deux seuils.

`detecter` choisit laquelle appeler selon le numero de la trackbar.

## 3. Flou

`preparer` applique un flou gaussien avant la detection. Le bruit produit des
variations parasites que les derivees peuvent amplifier. Le flou les attenue,
mais peut aussi effacer de petits details. Le filtre est fixe a 3x3 pour toutes les experiences.
On applique le meme pretraitement aux trois methodes.

## 4. Reference

Les references du TP ont des traits noirs, contrairement aux sorties des detecteurs.
`binariserReference` inverse alors sa polarite avec un seuillage inverse.
La reference devient blanche sur fond noir. Cette operation ne detecte rien :
elle rend les annotations deja presentes comparables a nos sorties.

## 5. Mesures exactes

`Mesures` regroupe les resultats. `countNonZero` compte les pixels blancs.

Dans `evaluer`, deux boucles visitent les pixels des deux masques. Si les deux
ont un contour au meme endroit, on ajoute une correspondance.

- D = nombre de pixels detectes.
- R = nombre de pixels de reference.
- C = nombre de correspondances.
- FP = D-C ; FN = R-C.
- P = C/(C+FP+FN).
- TFP = FP/(C+FP+FN).
- TFN = FN/(C+FP+FN).

La variable `total` est un double pour obtenir une division decimale.
Les quotients sont laisses a zero si le denominateur est nul.

## 6. Voisinage : lire les boucles sans se perdre

La premiere passe marque les correspondances exactes dans deux masques auxiliaires :
`utilise` concerne la reference et `dejaCorrect` concerne la detection.

Si le rayon vaut 1, on reprend les detections non appariees :

1. `y,x` selectionnent le pixel detecte.
2. `dy,dx` parcourent les decalages -1,0,1, donc ses neuf positions voisines.
3. `ny=y+dy` et `nx=x+dx` donnent la position candidate dans la reference.
4. On verifie les limites pour eviter un acces hors matrice.
5. Si cette position est un contour et n'est pas utilisee, on cree une paire.
6. `trouve=true` arrete la recherche pour ce pixel detecte.

Cette reservation evite de compter une seule reference comme plusieurs contours
corrects. Le choix du premier voisin est facile a expliquer mais n'assure pas
le nombre maximal de paires : c'est une limite assumee de la version simple.

## 7. Charger une paire

`charger` lit une image et sa reference en niveaux de gris, puis verifie leur
presence et leurs dimensions. Le programme cherche le meme nom dans le dossier de references. Sinon, il ajoute
`_gt_binary.pgm` au nom sans extension : `basket.pgm` devient `basket_gt_binary.pgm`.
Une erreur est expliquee via `throw`. `catch` recupere le message pour l'afficher.

## 8. Batch

`traiterLot` recoit la liste des fichiers et les reglages actuels.
Pour chaque image :

1. Charger l'image et sa reference.
2. Preparer l'image et binariser la reference.
3. Executer les trois detecteurs avec une boucle d=0,1,2.
4. Sauvegarder chaque resultat en PNG.
5. Evaluer aux rayons 0 et 1.
6. Ecrire une ligne CSV pour chaque evaluation.

Le CSV est un fichier texte dont les colonnes sont separees par des virgules.
`std::ofstream` l'ouvre ; `<<` ecrit dedans comme avec `std::cout`.
Les reglages sont dans les lignes pour savoir comment les scores ont ete obtenus.

## 9. Main

`argc/argv` recuperent les trois chemins donnes dans le terminal.
`std::filesystem` parcourt le dossier des images. On trie les noms pour avoir
un ordre stable. Ensuite on cree les trackbars et charge le premier couple.

La boucle lit les reglages, calcule les contours, evalue et affiche les images.
Elle recalcule a chaque tour pour garder le fonctionnement facile a suivre.
`waitKey(30)` permet de gerer les fenetres et de lire les touches.

N/P changent l'image ; B appelle la fonction batch ; Echap quitte.
Pendant le batch, la boucle attend la fin de la fonction : pas de threads.

## 10. Analyse des resultats

Expliquer les fonctions precedentes, montrer les parametres choisis et importer
le tableau CSV. Comparer les methodes sur les memes images, mais regler leurs
seuils independamment : leurs amplitudes ne sont pas equivalentes.

Comparer aussi les scores exacts et avec voisinage. Ajouter quelques captures
et commenter le bruit, l'epaisseur, les contours rates et la localisation.
Ne pas annoncer de meilleure methode avant d'avoir observe les resultats reels.
