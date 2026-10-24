#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// 1. LES REGLAGES : ils seront lus depuis les trackbars.
struct Reglages {
    int detecteur = 0; // 0 : Sobel, 1 : Laplacien, 2 : Canny
    int sobel = 100;
    int laplacien = 100;
    int cannyBas = 50;
    int cannyHaut = 150;
};

// Parametres fixes pour les images du TP.
constexpr int RAYON_FLOU = 1; // Filtre gaussien 3x3.
constexpr int RAYON_EVALUATION = 1; // Affichage avec tolerance 3x3.
constexpr int REFERENCE_NOIRE = 1; // Traits noirs sur fond blanc.

// 2. DETECTEURS SOBEL, LAPLACIEN ET CANNY.
cv::Mat detectSobel(const cv::Mat& img, double seuil) {
    cv::Mat gx, gy, magnitude, contours;
    cv::Sobel(img, gx, CV_32F, 1, 0, 3);
    cv::Sobel(img, gy, CV_32F, 0, 1, 3);
    cv::magnitude(gx, gy, magnitude);
    cv::compare(magnitude, seuil, contours, cv::CMP_GT);
    return contours;
}

cv::Mat detectLaplacian(const cv::Mat& img, double seuil) {
    cv::Mat reponse, force, contours;
    cv::Laplacian(img, reponse, CV_32F, 3);
    force = cv::abs(reponse);
    cv::compare(force, seuil, contours, cv::CMP_GT);
    return contours;
}

cv::Mat detectCanny(const cv::Mat& img, double bas, double haut) {
    cv::Mat contours;
    cv::Canny(img, contours, std::min(bas, haut), std::max(bas, haut), 3, true);
    return contours;
}

cv::Mat preparer(const cv::Mat& img, int rayonFlou) {
    if (rayonFlou == 0) return img.clone();
    cv::Mat resultat;
    int taille = 2 * rayonFlou + 1;
    cv::GaussianBlur(img, resultat, cv::Size(taille, taille), 0);
    return resultat;
}

cv::Mat detecter(const cv::Mat& imgPreparee, int choix, const Reglages& r) {
    if (choix == 0) return detectSobel(imgPreparee, r.sobel);
    if (choix == 1) return detectLaplacian(imgPreparee, r.laplacien);
    return detectCanny(imgPreparee, r.cannyBas, r.cannyHaut);
}

// La reference doit aussi avoir : 255 = contour, 0 = fond.
cv::Mat binariserReference(const cv::Mat& image, bool traitsNoirs) {
    cv::Mat reference;
    cv::threshold(image, reference, 127, 255,
                  traitsNoirs ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);
    return reference;
}

// 3. LES CINQ COMPTAGES ET LES TROIS RATIOS DE L'ENONCE.
struct Mesures {
    int detectes = 0, reference = 0, corrects = 0;
    int fauxPositifs = 0, fauxNegatifs = 0;
    double performance = 0, tfp = 0, tfn = 0;
};

Mesures evaluer(const cv::Mat& contours, const cv::Mat& reference, int rayon) {
    if (contours.size() != reference.size())
        throw std::runtime_error("Image et reference de tailles differentes");
    if (contours.empty() || contours.type() != CV_8UC1 || reference.type() != CV_8UC1)
        throw std::runtime_error("Masques grayscale non vides requis");
    if (rayon != 0 && rayon != 1)
        throw std::runtime_error("Le rayon doit valoir 0 ou 1");

    Mesures m;
    m.detectes = cv::countNonZero(contours);
    m.reference = cv::countNonZero(reference);
    // Chaque pixel de reference peut etre utilise UNE SEULE FOIS.
    cv::Mat utilise(reference.size(), CV_8UC1, cv::Scalar(0));
    cv::Mat dejaCorrect(contours.size(), CV_8UC1, cv::Scalar(0));

    // Premiere passe : conserver toutes les correspondances exactes.
    for (int y = 0; y < contours.rows; ++y) {
        for (int x = 0; x < contours.cols; ++x) {
            if (contours.at<uchar>(y,x) != 0 && reference.at<uchar>(y,x) != 0) {
                ++m.corrects;
                utilise.at<uchar>(y,x) = 255;
                dejaCorrect.at<uchar>(y,x) = 255;
            }
        }
    }

    // Deuxieme passe facultative : chercher un voisin de reference encore libre.
    // Methode gloutonne simple : premier voisin libre, ordre lignes puis colonnes.
    // Elle donne des comptages coherents mais pas forcement le maximum de paires.
    if (rayon == 1) {
        for (int y = 0; y < contours.rows; ++y) {
            for (int x = 0; x < contours.cols; ++x) {
                if (contours.at<uchar>(y,x) == 0 || dejaCorrect.at<uchar>(y,x) != 0)
                    continue;
                bool trouve = false;
                for (int dy = -1; dy <= 1 && !trouve; ++dy) {
                    for (int dx = -1; dx <= 1 && !trouve; ++dx) {
                        int ny = y + dy, nx = x + dx;
                        if (ny < 0 || ny >= reference.rows || nx < 0 || nx >= reference.cols)
                            continue;
                        if (reference.at<uchar>(ny,nx) != 0 && utilise.at<uchar>(ny,nx) == 0) {
                            utilise.at<uchar>(ny,nx) = 255;
                            ++m.corrects;
                            trouve = true;
                        }
                    }
                }
            }
        }
    }
    m.fauxPositifs = m.detectes - m.corrects;
    m.fauxNegatifs = m.reference - m.corrects;
    double total = double(m.corrects) + m.fauxPositifs + m.fauxNegatifs;
    if (total > 0) {
        m.performance = m.corrects / total;
        m.tfp = m.fauxPositifs / total;
        m.tfn = m.fauxNegatifs / total;
    }
    return m;
}

const std::vector<std::string> noms = {"Sobel", "Laplacien", "Canny"};

// 4. CHARGEMENT D'UNE PAIRE IMAGE / REFERENCE.
void charger(const fs::path& cheminImage, const fs::path& dossierReferences,
             cv::Mat& image, cv::Mat& reference) {
    image = cv::imread(cheminImage.string(), cv::IMREAD_GRAYSCALE);
    // Exemple : basket.pgm -> basket_gt_binary.pgm dans images/gt.
    fs::path cheminReference = dossierReferences / cheminImage.filename();
    if (!fs::exists(cheminReference)) {
        cheminReference = dossierReferences / (cheminImage.stem().string() + "_gt_binary.pgm");
    }
    reference = cv::imread(cheminReference.string(), cv::IMREAD_GRAYSCALE);
    if (image.empty() || reference.empty())
        throw std::runtime_error("Image/reference absente ou illisible : " + cheminImage.filename().string());
    if (image.size() != reference.size())
        throw std::runtime_error("Tailles differentes : " + cheminImage.filename().string());
}

// 5. BATCH : memes reglages pour toutes les images, et les trois detecteurs.
void traiterLot(const std::vector<fs::path>& fichiers, const fs::path& references,
                const fs::path& sortie, const Reglages& r) {
    fs::create_directories(sortie);
    std::ofstream csv(sortie / "mesures.csv");
    if (!csv) throw std::runtime_error("Impossible de creer le CSV");
    csv.exceptions(std::ios::badbit | std::ios::failbit);
    csv << "image,detecteur,rayon,detectes,reference,corrects,FP,FN,performance,TFP,TFN,"
           "seuil_sobel,seuil_laplacien,canny_bas,canny_haut,rayon_flou,reference_noire\n";
    int reussies = 0;
    for (const auto& fichier : fichiers) {
        cv::Mat img, ref;
        try {
            charger(fichier, references, img, ref);
        } catch (const std::exception& e) {
            std::cerr << e.what() << " : paire ignoree\n";
            continue;
        }
        ref = binariserReference(ref, REFERENCE_NOIRE != 0);
        cv::Mat preparee = preparer(img, RAYON_FLOU);
        for (int d = 0; d < 3; ++d) {
            cv::Mat contours = detecter(preparee, d, r);
            fs::create_directories(sortie / noms[d]);
            // Garder le nom complet evite de confondre image.jpg et image.png.
            auto destination = sortie / noms[d] / (fichier.filename().string() + ".png");
            if (!cv::imwrite(destination.string(), contours))
                throw std::runtime_error("Impossible d'enregistrer un contour");
            for (int rayon = 0; rayon <= 1; ++rayon) {
                Mesures m = evaluer(contours, ref, rayon);
                // Doubler les guillemets eventuels du nom pour un CSV valide.
                std::string nom;
                for (char c : fichier.filename().string()) {
                    if (c == '"') nom += '"';
                    nom += c;
                }
                csv << '"' << nom << "\"," << noms[d] << ',' << rayon << ','
                    << m.detectes << ',' << m.reference << ',' << m.corrects << ','
                    << m.fauxPositifs << ',' << m.fauxNegatifs << ','
                    << m.performance << ',' << m.tfp << ',' << m.tfn << ','
                    << r.sobel << ',' << r.laplacien << ','
                    << std::min(r.cannyBas,r.cannyHaut) << ',' << std::max(r.cannyBas,r.cannyHaut)
                    << ',' << RAYON_FLOU << ',' << REFERENCE_NOIRE << '\n';
            }
        }
        ++reussies;
        std::cout << "Traitee : " << fichier.filename() << '\n';
    }
    std::cout << "Lot termine : " << reussies << '/' << fichiers.size()
              << " paires. Resultats dans " << sortie << '\n';
}

void ajouterTrackbar(const std::string& fenetre, const std::string& nom, int maximum, int valeur) {
    cv::createTrackbar(nom, fenetre, nullptr, maximum);
    cv::setTrackbarPos(nom, fenetre, valeur);
}

// 6. MAIN : FICHIERS, TRACKBARS ET CLAVIER.
int main(int argc, char** argv) {
    try {
        if (argc != 4) {
            std::cout << "Usage : ./contours DOSSIER_IMAGES DOSSIER_REFERENCES DOSSIER_SORTIE\n";
            return 1;
        }
        std::vector<fs::path> fichiers;
        for (const auto& entree : fs::directory_iterator(argv[1])) {
            std::string extension = entree.path().extension().string();
            if (entree.is_regular_file() && (extension == ".png" || extension == ".jpg" ||
                extension == ".jpeg" || extension == ".pgm" || extension == ".bmp"))
                fichiers.push_back(entree.path());
        }
        std::sort(fichiers.begin(), fichiers.end());
        if (fichiers.empty()) throw std::runtime_error("Aucune image dans le dossier");

        Reglages r;
        const std::string fenetre = "TP contours";
        cv::namedWindow(fenetre, cv::WINDOW_NORMAL);
        ajouterTrackbar(fenetre, "Detecteur 0S 1L 2C", 2, r.detecteur);
        ajouterTrackbar(fenetre, "Seuil Sobel", 1500, r.sobel);
        ajouterTrackbar(fenetre, "Seuil Laplacien", 3000, r.laplacien);
        ajouterTrackbar(fenetre, "Canny bas", 1500, r.cannyBas);
        ajouterTrackbar(fenetre, "Canny haut", 1500, r.cannyHaut);

        int index = 0;
        cv::Mat img, ref;
        charger(fichiers[index], argv[2], img, ref);
        std::cout << "N : suivante | P : precedente | B : batch | Echap : quitter\n";
        while (true) {
            r.detecteur = cv::getTrackbarPos("Detecteur 0S 1L 2C", fenetre);
            r.sobel = cv::getTrackbarPos("Seuil Sobel", fenetre); r.laplacien = cv::getTrackbarPos("Seuil Laplacien", fenetre);
            r.cannyBas = cv::getTrackbarPos("Canny bas", fenetre); r.cannyHaut = cv::getTrackbarPos("Canny haut", fenetre);

            cv::Mat contours = detecter(preparer(img,RAYON_FLOU),r.detecteur,r);
            cv::Mat reference = binariserReference(ref,REFERENCE_NOIRE != 0);
            Mesures m = evaluer(contours,reference,RAYON_EVALUATION);
            // Une seule grande image : trois vues en haut, les mesures en bas.
            const int largeur = 360, hauteur = 300;
            cv::Mat panneau(470, 3 * largeur, CV_8UC3, cv::Scalar(30,30,30));
            const std::vector<cv::Mat> vues = {img, reference, contours};
            const std::vector<std::string> titres = {"Photo originale", "Reference", "Contours calcules"};
            for (int i = 0; i < 3; ++i) {
                cv::putText(panneau, titres[i], {i * largeur + 10, 25},
                            cv::FONT_HERSHEY_SIMPLEX, 0.6, {255,255,255}, 1);
                // Adapter la taille sans deformer l'image ; les mesures utilisent l'original.
                double echelle = std::min(double(largeur) / vues[i].cols,
                                          double(hauteur) / vues[i].rows);
                cv::Mat petite, couleur;
                cv::resize(vues[i], petite, cv::Size(), echelle, echelle, cv::INTER_NEAREST);
                cv::cvtColor(petite, couleur, cv::COLOR_GRAY2BGR);
                cv::Rect zone(i * largeur + (largeur - couleur.cols) / 2,
                              40 + (hauteur - couleur.rows) / 2, couleur.cols, couleur.rows);
                couleur.copyTo(panneau(zone));
            }
            auto texte = [&](const std::string& ligne, int y) {
                cv::putText(panneau, ligne, {10,y}, cv::FONT_HERSHEY_SIMPLEX,
                            0.5, {255,255,255}, 1);
            };
            texte(noms[r.detecteur] + " | " + fichiers[index].filename().string()
                  + " | Image " + std::to_string(index + 1) + "/" + std::to_string(fichiers.size()), 365);
            texte("Detectes : " + std::to_string(m.detectes) + " | Reference : "
                  + std::to_string(m.reference) + " | Corrects : " + std::to_string(m.corrects)
                  + " | FP : " + std::to_string(m.fauxPositifs) + " | FN : " + std::to_string(m.fauxNegatifs), 390);
            texte(cv::format("Performance : %.2f %% | TFP : %.2f %% | TFN : %.2f %% | Voisinage : %s",
                  100 * m.performance, 100 * m.tfp, 100 * m.tfn, RAYON_EVALUATION ? "3x3" : "exact"), 415);
            texte("N: suivante | P: precedente | B: traiter le lot | Echap: quitter", 450);
            cv::imshow(fenetre,panneau);
            int key = cv::waitKey(30) & 255;
            if (key == 27 || cv::getWindowProperty(fenetre,cv::WND_PROP_VISIBLE) < 1) break;
            if (key == 'b' || key == 'B') traiterLot(fichiers,argv[2],argv[3],r);
            int suivant = index;
            if (key == 'n' || key == 'N') suivant = (index+1) % int(fichiers.size());
            if (key == 'p' || key == 'P') suivant = (index+int(fichiers.size())-1) % int(fichiers.size());
            if (suivant != index) {
                // Charger dans des variables temporaires pour garder l'ancienne image en cas d'erreur.
                cv::Mat nouvelle, nouvelleRef;
                try {
                    charger(fichiers[suivant],argv[2],nouvelle,nouvelleRef);
                    img = nouvelle; ref = nouvelleRef; index = suivant;
                } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
            }
        }
        cv::destroyAllWindows();
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << '\n';
        return 1;
    }
    return 0;
}
