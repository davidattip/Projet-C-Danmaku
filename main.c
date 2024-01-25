#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "constants.h"


// Ajoutez cet énum pour gérer les états du jeu
typedef enum {
    GAME_STATE_TITLE_SCREEN,
    GAME_STATE_MENU,
    // Ajoutez d'autres états ici si nécessaire
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
} GameState;

typedef struct {
    float x;
    float y;
    float vel_x; // Ajout de la vitesse horizontale
    float vel_y;
    bool active; // Indique si le projectile est actif (en déplacement)
} Projectile;

typedef struct {
    float vel_x;
    float vel_y;
} Direction;

Direction shoot_directions[] = {
    {0, 300},     // Bas
    {100, 300},   // Diagonale bas droite
    {-100, 300},  // Diagonale bas gauche
    {200, 300},   // Plus à droite
    {-200, 300},  // Plus à gauche
    {-150, 300},
    {150, 300}
    // Ajoutez plus de directions ici si nécessaire
};

#define MAX_PROJECTILES 30 // Nombre maximum de projectiles en même temps



///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////
int game_is_running = false;
int last_frame_time = 0;
// Ajoutez une nouvelle variable globale pour la vitesse du vaisseau
const int SHIP_SPEED = 300;
const int SHIP_TURN_TIME = 200; // Temps en millisecondes pour afficher le vaisseau tournant
const int ENEMY_SPEED = 100;
bool movingRight = true;
int player_lives = 3; // Le joueur commence avec 3 vies
int score = 0;
int lives = 3;
TTF_Font* font_text = NULL;
SDL_Color white = { 255, 255, 255, 255 }; // Blanc
Mix_Music* backgroundMusic = NULL;

SDL_Texture* game_over_texture = NULL;
SDL_Texture* restart_texture = NULL;

Uint32 start_time = 0;
SDL_Rect life_rects[3]; // Pour afficher les 3 cœurs
SDL_Texture* heart_texture = NULL; // Texture pour le cœur


// Projectiles
Projectile projectiles[MAX_PROJECTILES];


SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* background_texture = NULL;
SDL_Texture* title_texture = NULL;
SDL_Texture* press_enter_texture = NULL;
SDL_Texture* limited_edition_texture = NULL;
SDL_Texture* start_game_texture = NULL;
SDL_Texture* choose_difficulty_texture = NULL;
// Textures pour le vaisseau spatial
SDL_Texture* ship_texture_up = NULL;
SDL_Texture* ship_texture_right = NULL;
SDL_Texture* ship_texture_left = NULL;
SDL_Texture* enemy_ship_texture = NULL;
SDL_Texture* projectile_texture = NULL;

GameState current_game_state = GAME_STATE_TITLE_SCREEN;

// Variable pour suivre l'état du vaisseau
Uint32 last_key_press_time = 0;
SDL_Texture* current_ship_texture = NULL;

///////////////////////////////////////////////////////////////////////////////
// Declare two game objects for the ball and the paddle
///////////////////////////////////////////////////////////////////////////////
struct game_object {
    float x;
    float y;
    float width;
    float height;
    float vel_x;
    float vel_y;
    
} ship,ball, paddle;

// Vaisseau ennemi
struct game_object enemy_ship;



///////////////////////////////////////////////////////////////////////////////
// Function to initialize our SDL window
///////////////////////////////////////////////////////////////////////////////
int initialize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(
        "Danmaku Ultimate",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );
    if (!window) {
        fprintf(stderr, "Error creating SDL Window.\n");
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "Error creating SDL Renderer.\n");
        return false;
    }
    


    // Initialize SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "Error initializing SDL_image: %s\n", IMG_GetError());
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        fprintf(stderr, "Error initializing SDL_ttf: %s\n", TTF_GetError());
        return false;
    }

return true;
}

///////////////////////////////////////////////////////////////////////////////
// Function to load textures
///////////////////////////////////////////////////////////////////////////////
SDL_Texture* load_texture(const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        fprintf(stderr, "Error creating surface: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        fprintf(stderr, "Error creating texture: %s\n", SDL_GetError());
        return NULL;
    }
    return texture;
}

///////////////////////////////////////////////////////////////////////////////
// Function to load text as textures
///////////////////////////////////////////////////////////////////////////////
SDL_Texture* load_text(const char* text, TTF_Font* font, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) {
        fprintf(stderr, "Error creating surface from text: %s\n", TTF_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        fprintf(stderr, "Error creating texture from text: %s\n", SDL_GetError());
        return NULL;
    }
    return texture;
}

///////////////////////////////////////////////////////////////////////////////
// Setup function that runs once at the beginning of our program
///////////////////////////////////////////////////////////////////////////////
void setup(void) {
    // Load the background image
    background_texture = load_texture("image_accueil.png");
    // Charger les textures pour le vaisseau
    ship_texture_up = load_texture("ship_up.png");
    ship_texture_right = load_texture("ship_right.png");
    ship_texture_left = load_texture("ship_left.png");
    current_ship_texture = ship_texture_up; // Texture de départ
    // Chargement de la musique de fond
    backgroundMusic = Mix_LoadMUS("music_fond.mp3");
    if (backgroundMusic == NULL) {
        fprintf(stderr, "Failed to load background music! SDL_mixer Error: %s\n", Mix_GetError());
        // Gérer l'erreur ou quitter
    }
    // Charger la texture du vaisseau ennemi et des projectiles
    enemy_ship_texture = load_texture("enemy_ship.png");
    projectile_texture = load_texture("enemy_boule.png");
    heart_texture = load_texture("coeur.png");
    for (int i = 0; i < player_lives; ++i) {
        life_rects[i].x = 10 + (i * 30); // 10 pixels de décalage plus largeur du cœur plus un peu d'espace
        life_rects[i].y = 10; // 10 pixels du haut de l'écran
        life_rects[i].w = 20; // Largeur du cœur
        life_rects[i].h = 20; // Hauteur du cœur
    }


    // Ouvre la police avec une plus grande taille pour le titre
    TTF_Font* font_title = TTF_OpenFont("PermanentMarker-Regular.ttf", 64); // Ajustez la taille selon les besoins
    if (!font_title) {
        fprintf(stderr, "Error loading font for title: %s\n", TTF_GetError());
        exit(1);
    }

    // Ouvre la police avec une taille plus petite pour le texte en dessous
    TTF_Font* font_text = TTF_OpenFont("PermanentMarker-Regular.ttf", 32); // Ajustez la taille selon les besoins
    if (!font_text) {
        fprintf(stderr, "Error loading font for text: %s\n", TTF_GetError());
        exit(1);
    }

    // Définit la couleur du texte rouge pour le titre
    SDL_Color red = { 255, 0, 0, 255 }; // Rouge

    // Définit la couleur du texte blanc pour le reste du texte
    SDL_Color white = { 255, 255, 255, 255 }; // Blanc

    // Crée des textures pour le texte
    title_texture = load_text("DANMAKU ULTIMATE", font_title, red);
    press_enter_texture = load_text("Press Enter to Go to MENU", font_text, white);
    limited_edition_texture = load_text("Limited Edition", font_text, white);
    game_over_texture = load_text("GAME OVER", font_title, red);
    restart_texture = load_text("Press Enter to Restart", font_text, white);

    // Création des textures pour les options de menu
    start_game_texture = load_text("Commencer le jeu", font_text, white);
    choose_difficulty_texture = load_text("Choisir la difficulté", font_text, white);

    // Ferme les polices après utilisation
    TTF_CloseFont(font_title);
    TTF_CloseFont(font_text);

    

    // Initialize  ball object moving down at a constant velocity
    ball.x = 10;
    ball.y = 20;
    ball.width = 20;
    ball.height = 20;
    ball.vel_x = 180;
    ball.vel_y = 140;

    // Initialiser le vaisseau spatial
    ship.x = WINDOW_WIDTH / 2;
    ship.y = WINDOW_HEIGHT / 2;
    ship.width = 64;  // La largeur de la texture du vaisseau
    ship.height = 64; // La hauteur de la texture du vaisseau
    ship.vel_x = 0;
    ship.vel_y = 0;

    // Initialiser le vaisseau ennemi
    enemy_ship.x = WINDOW_WIDTH / 2 - 32; // Centré en x, 32 étant la moitié de la largeur du vaisseau ennemi
    enemy_ship.y = 50; // Un peu en dessous du haut de l'écran
    enemy_ship.width = 64; // Supposons que le vaisseau ennemi a une largeur de 64 pixels
    enemy_ship.height = 64; // Supposons que le vaisseau ennemi a une hauteur de 64 pixels
    // Initialiser les projectiles
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        projectiles[i].active = false;
    }
    // Dans setup() ou au début d'un niveau
    if (backgroundMusic != NULL) {
        Mix_PlayMusic(backgroundMusic, -1); // -1 signifie en boucle indéfiniment
    }

    
}


///////////////////////////////////////////////////////////////////////////////
// Function to poll SDL events and process keyboard input
///////////////////////////////////////////////////////////////////////////////
void process_input(void) {
    SDL_Event event;
    static int menu_option = 0; // 0 pour "Commencer", 1 pour "Difficulté"
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            game_is_running = false;
            break;
        case SDL_KEYDOWN:
            switch (current_game_state) {
            case GAME_STATE_TITLE_SCREEN:
                if (event.key.keysym.sym == SDLK_RETURN) {
                    current_game_state = GAME_STATE_MENU;
                }
                break;
            case GAME_STATE_MENU:
                // Ici, ajoutez la logique pour naviguer dans le menu
                // par exemple, utiliser les touches fléchées pour changer de sélection,
                // et Entrée pour sélectionner une option.

                if (event.key.keysym.sym == SDLK_RETURN) {
                    if (menu_option == 0) {
                        current_game_state = GAME_STATE_PLAYING;
                    }
                    else if (menu_option == 1) {
                        // Logique pour choisir la difficulté
                    }
                }
                else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_UP) {
                    menu_option = 1 - menu_option; // Alterne entre 0 et 1
                }

                break;
            case GAME_STATE_PLAYING:
                // Ici, ajoutez la logique pour le déplacement du joueur, le tir, etc.
                switch (event.key.keysym.sym) {
                case SDLK_UP:
                    ship.vel_y = -SHIP_SPEED;
                    break;
                case SDLK_DOWN:
                    ship.vel_y = SHIP_SPEED;
                    break;
                case SDLK_LEFT:
                    ship.vel_x = -SHIP_SPEED;
                    last_key_press_time = SDL_GetTicks();
                    current_ship_texture = ship_texture_left;
                    break;
                case SDLK_RIGHT:
                    ship.vel_x = SHIP_SPEED;
                    last_key_press_time = SDL_GetTicks();
                    current_ship_texture = ship_texture_right;
                    break;
                }
                break;
                break;
            case GAME_STATE_GAME_OVER:
                if (event.key.keysym.sym == SDLK_RETURN) {
                    // Reset the game state and player lives to restart the game
                    current_game_state = GAME_STATE_PLAYING;
                    player_lives = 3;
                    setup(); // Reset the game setup
                    
                }
                break;
            }
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                game_is_running = false;
            }
            break;
        case SDL_KEYUP:
            if (current_game_state == GAME_STATE_PLAYING) {
                if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
                    ship.vel_y = 0;
                }
                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) {
                    ship.vel_x = 0;
                    // Réinitialiser la texture du vaisseau après la fin du mouvement
                    if (SDL_GetTicks() - last_key_press_time >= SHIP_TURN_TIME) {
                        current_ship_texture = ship_texture_up;
                    }
                }
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Setup function that runs once at the beginning of our program
///////////////////////////////////////////////////////////////////////////////
/* void setup(void) {
    // Initialize the ball object moving down at a constant velocity
    ball.x = 10;
    ball.y = 20;
    ball.width = 20;
    ball.height = 20;
    ball.vel_x = 180;
    ball.vel_y = 140;
}
*/
///////////////////////////////////////////////////////////////////////////////
// Update function with a fixed time step
///////////////////////////////////////////////////////////////////////////////

bool check_collision(struct game_object* a, Projectile* b) {
    if (a->x + a->width < b->x || b->x + 16 < a->x ||
        a->y + a->height < b->y || b->y + 16 < a->y) {
        return false; // Pas de collision
    }
    return true; // Collision
}

void reset_game(void) {
    player_lives = 3;
    score = 0;
    start_time = SDL_GetTicks();
    // Réinitialiser d'autres états de jeu si nécessaire
    // Réinitialiser la position du vaisseau, etc.
    ship.x = WINDOW_WIDTH / 2;
    ship.y = WINDOW_HEIGHT / 2;
    // Réinitialiser les projectiles
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        projectiles[i].active = false;
    }
    // Autres réinitialisations si nécessaire
}

void change_state_to_playing(void) {
    current_game_state = GAME_STATE_PLAYING;
    start_time = SDL_GetTicks(); // Démarre le compteur de score quand le jeu commence réellement

}

void handle_collisions(void) {
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (projectiles[i].active && check_collision(&ship, &projectiles[i])) {
            // Gérer la collision, par exemple en finissant le jeu
            projectiles[i].active = false; // Désactiver le projectile
            player_lives--; // Décrémenter une vie
            if (player_lives <= 0) {
                current_game_state = GAME_STATE_GAME_OVER;
            }
        }
    }
}


void update_projectiles(float delta_time) {
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (projectiles[i].active) {
            projectiles[i].x += projectiles[i].vel_x * delta_time;
            projectiles[i].y += projectiles[i].vel_y * delta_time;

            // Désactiver le projectile s'il sort de l'écran
            if (projectiles[i].y > WINDOW_HEIGHT || projectiles[i].x < 0 || projectiles[i].x > WINDOW_WIDTH) {
                projectiles[i].active = false;
            }
        }
    }
}

void shoot_projectile(void) {
    Uint32 current_time = SDL_GetTicks();
    int elapsed_time = (current_time - start_time) / 1000; // Temps écoulé en secondes

    // Calculer le nombre de directions actives basé sur le temps écoulé
    int active_directions = elapsed_time / 10 + 1; // Ajouter une direction toutes les 10 secondes
    int num_directions = sizeof(shoot_directions) / sizeof(shoot_directions[0]);
    if (active_directions > num_directions) {
        active_directions = num_directions; // Limiter au nombre total de directions disponibles
    }

    // Calculer l'augmentation de la vitesse des projectiles
    float speed_increase = (elapsed_time / 10) * 50; // Augmenter de 50 unités toutes les 10 secondes

    for (int dir_index = 0; dir_index < active_directions; ++dir_index) {
        for (int i = 0; i < MAX_PROJECTILES; ++i) {
            if (!projectiles[i].active) {
                projectiles[i].x = enemy_ship.x + enemy_ship.width / 2 - 8; // Centrer le projectile
                projectiles[i].y = enemy_ship.y + enemy_ship.height; // Partir de la base du vaisseau ennemi

                projectiles[i].vel_x = shoot_directions[dir_index].vel_x + speed_increase;
                projectiles[i].vel_y = shoot_directions[dir_index].vel_y + speed_increase;

                projectiles[i].active = true;
                break; // Ne pas créer plus d'un projectile à la fois par direction
            }
        }
    }
}






void update_enemy(float delta_time) {
    // Ajouter des tirs ici si nécessaire
    // Par exemple, tirer un projectile à intervalles réguliers
      // Faire bouger l'ennemi horizontalement
    if (movingRight) {
        enemy_ship.x += ENEMY_SPEED * delta_time;
        if (enemy_ship.x + enemy_ship.width > WINDOW_WIDTH) {
            movingRight = false;
        }
    }
    else {
        enemy_ship.x -= ENEMY_SPEED * delta_time;
        if (enemy_ship.x < 0) {
            movingRight = true;
        }
    }
    static Uint32 last_shot_time = 0;
    if (SDL_GetTicks() - last_shot_time > 750) { // Tirs toutes les 1 secondes
        shoot_projectile();
        last_shot_time = SDL_GetTicks();
    }
}

void update(void) {

    float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;

    switch (current_game_state) {
    case GAME_STATE_TITLE_SCREEN:
        // Pas de mise à jour nécessaire pour l'écran titre
        break;
    case GAME_STATE_MENU:
        // Mettez à jour la logique du menu ici si nécessaire
        break;
    case GAME_STATE_PLAYING:
        score = (SDL_GetTicks() - start_time) / 1000; // Convertissez le temps en secondes
        // Mettez à jour la position des objets, vérifiez les collisions, etc.
        // Mise à jour de la position du vaisseau
        ship.x += ship.vel_x * delta_time;
        ship.y += ship.vel_y * delta_time;

        // Vérifier que le vaisseau ne sort pas de l'écran
        if (ship.x < 0) ship.x = 0;
        if (ship.x + ship.width > WINDOW_WIDTH) ship.x = WINDOW_WIDTH - ship.width;
        if (ship.y < 0) ship.y = 0;
        if (ship.y + ship.height > WINDOW_HEIGHT) ship.y = WINDOW_HEIGHT - ship.height;

        // Réinitialiser la texture du vaisseau après la fin du mouvement
        if (SDL_GetTicks() - last_key_press_time >= SHIP_TURN_TIME) {
            current_ship_texture = ship_texture_up;
        }
        Uint32 elapsed_time = SDL_GetTicks() - start_time; // Temps écoulé depuis le début du jeu
        Uint32 shoot_interval = 750 - (elapsed_time / 10000) * 50; // Réduire l'intervalle de 50 ms toutes les 10 secondes

        if (shoot_interval < 200) { // Limiter l'intervalle minimum à 200 ms
            shoot_interval = 200;
        }
        static Uint32 last_shoot_time = 0;
        if (SDL_GetTicks() - last_shoot_time > shoot_interval) {
            shoot_projectile();
            last_shoot_time = SDL_GetTicks();
        }
        // Mise à jour de l'ennemi
        update_enemy(delta_time);
        update_projectiles(delta_time);
        // Gérer les collisions
        handle_collisions();
        break;
    case GAME_STATE_GAME_OVER:
        // Peut-être clignoter le texte ou mettre en place un délai avant de permettre la réinitialisation
        break;
    }

    // Get delta_time factor converted to seconds to be used to update objects
    

    // Store the milliseconds of the current frame to be used in the next one
    last_frame_time = SDL_GetTicks();

   /* // Move ball as a function of delta time
    ball.x += ball.vel_x * delta_time;
    ball.y += ball.vel_y * delta_time;

    // Check for ball collision with the window borders
    if (ball.x < 0) {
        ball.x = 0;
        ball.vel_x = -ball.vel_x;
    }
    if (ball.x + ball.height > WINDOW_WIDTH) {
        ball.x = WINDOW_WIDTH - ball.width;
        ball.vel_x = -ball.vel_x;
    }
    if (ball.y < 0) {
        ball.y = 0;
        ball.vel_y = -ball.vel_y;
    }
    if (ball.y + ball.height > WINDOW_HEIGHT) {
        ball.y = WINDOW_HEIGHT - ball.height;
        ball.vel_y = -ball.vel_y;
    }*/
}

// Fonction pour rendre le score à l'écran
void render_score(SDL_Renderer* renderer, TTF_Font* font, SDL_Color color, int score) {

    char score_text[50];
    sprintf_s(score_text, sizeof(score_text), "Score: %d", score);
    SDL_Texture* score_texture = load_text(score_text, font, color);
    SDL_Rect score_rect = { WINDOW_WIDTH - 150, 10, 0, 0 };
    SDL_QueryTexture(score_texture, NULL, NULL, &score_rect.w, &score_rect.h);
    SDL_RenderCopy(renderer, score_texture, NULL, &score_rect);
    SDL_DestroyTexture(score_texture);
}

///////////////////////////////////////////////////////////////////////////////
// Render function to draw game objects in the SDL window
///////////////////////////////////////////////////////////////////////////////
void render(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect start_game_rect;
    TTF_Font* font_text = TTF_OpenFont("PermanentMarker-Regular.ttf", 32); // Ajustez la taille selon les besoins

    switch (current_game_state) {
    case GAME_STATE_TITLE_SCREEN:
        // Dessinez l'écran titre ici

        // Dessine l'arrière-plan
        SDL_RenderCopy(renderer, background_texture, NULL, NULL);

        // Centre le texte du titre
        int title_width, title_height;
        SDL_Rect title_rect;
        SDL_QueryTexture(title_texture, NULL, NULL, &title_width, &title_height);
        title_rect.x = (WINDOW_WIDTH - title_width) / 2;
        title_rect.y = (WINDOW_HEIGHT / 4) - (title_height / 2); // Ajustez ces valeurs selon les besoins
        title_rect.w = title_width;
        title_rect.h = title_height;
        SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);

        // Positionne le texte "Press Enter to Start" en dessous du titre
        int enter_width, enter_height;
        SDL_Rect enter_rect;
        SDL_QueryTexture(press_enter_texture, NULL, NULL, &enter_width, &enter_height);
        enter_rect.x = (WINDOW_WIDTH - enter_width) / 2;
        enter_rect.y = (WINDOW_HEIGHT / 2) - (enter_height / 2); // Ajustez ces valeurs selon les besoins
        enter_rect.w = enter_width;
        enter_rect.h = enter_height;
        SDL_RenderCopy(renderer, press_enter_texture, NULL, &enter_rect);

        // Positionne le texte "Limited Edition" en bas de l'écran
        int limited_width, limited_height;
        SDL_Rect limited_rect;
        SDL_QueryTexture(limited_edition_texture, NULL, NULL, &limited_width, &limited_height);
        limited_rect.x = (WINDOW_WIDTH - limited_width) / 2;
        limited_rect.y = WINDOW_HEIGHT - limited_height - 50; // Ajustez ces valeurs selon les besoins
        limited_rect.w = limited_width;
        limited_rect.h = limited_height;
        SDL_RenderCopy(renderer, limited_edition_texture, NULL, &limited_rect);

        
        // Ajoutez d'autres textures si nécessaire
        break;
    case GAME_STATE_MENU:
        // Dessinez le menu ici
        // Inclure le dessin des options de menu et des sélections actuelles

         // Dessinez ici les options de menu
        
        SDL_QueryTexture(start_game_texture, NULL, NULL, &start_game_rect.w, &start_game_rect.h);
        start_game_rect.x = (WINDOW_WIDTH - start_game_rect.w) / 2;
        start_game_rect.y = WINDOW_HEIGHT / 2 - start_game_rect.h; // Ajustez ces positions selon les besoins
        SDL_RenderCopy(renderer, start_game_texture, NULL, &start_game_rect);

        SDL_Rect choose_difficulty_rect;
        SDL_QueryTexture(choose_difficulty_texture, NULL, NULL, &choose_difficulty_rect.w, &choose_difficulty_rect.h);
        choose_difficulty_rect.x = (WINDOW_WIDTH - choose_difficulty_rect.w) / 2;
        choose_difficulty_rect.y = WINDOW_HEIGHT / 2; // Ajustez ces positions selon les besoins
        SDL_RenderCopy(renderer, choose_difficulty_texture, NULL, &choose_difficulty_rect);
        break;

        break;
    case GAME_STATE_PLAYING:
        // Dessinez les objets de jeu ici
        ;
        // Inclure le joueur, les ennemis, les projectiles, etc.
        // Dessiner le vaisseau spatial
        SDL_Rect ship_rect = {
            (int)ship.x,
            (int)ship.y,
            (int)ship.width,
            (int)ship.height
        };
        SDL_RenderCopy(renderer, current_ship_texture, NULL, &ship_rect);
        // Dessiner le vaisseau ennemi
        SDL_Rect enemy_rect = {
            (int)enemy_ship.x,
            (int)enemy_ship.y,
            (int)enemy_ship.width,
            (int)enemy_ship.height
        };
        SDL_RenderCopy(renderer, enemy_ship_texture, NULL, &enemy_rect);
        // Dessiner les projectiles
        for (int i = 0; i < MAX_PROJECTILES; ++i) {
            if (projectiles[i].active) {
                SDL_Rect projectile_rect = {
                    (int)projectiles[i].x,
                    (int)projectiles[i].y,
                    16, // Largeur du projectile
                    16  // Hauteur du projectile
                };
                SDL_RenderCopy(renderer, projectile_texture, NULL, &projectile_rect);
            }
        }
        // Dessiner les vies
        for (int i = 0; i < player_lives; ++i) {
            SDL_RenderCopy(renderer, heart_texture, NULL, &life_rects[i]);
        }
        //TTF_Font* font_text = TTF_OpenFont("PermanentMarker-Regular.ttf", 32); // Ajustez la taille selon les besoins
        if (!font_text) {
            fprintf(stderr, "Error loading font for text: %s\n", TTF_GetError());
            exit(1);
        }
        render_score(renderer, font_text, white, score);
        break;
    case GAME_STATE_GAME_OVER:
        // Dessinez l'écran de game over ici
        // Inclure le score final et les options pour recommencer ou quitter

        // Render the game over text
        ;
        int game_over_width, game_over_height;
        SDL_Rect game_over_rect;
        SDL_QueryTexture(game_over_texture, NULL, NULL, &game_over_width, &game_over_height);
        game_over_rect.x = (WINDOW_WIDTH - game_over_width) / 2;
        game_over_rect.y = (WINDOW_HEIGHT / 2) - game_over_height - 20; // Adjust as needed
        game_over_rect.w = game_over_width;
        game_over_rect.h = game_over_height;
        SDL_RenderCopy(renderer, game_over_texture, NULL, &game_over_rect);

        // Render the restart text
        int restart_width, restart_height;
        SDL_Rect restart_rect;
        SDL_QueryTexture(restart_texture, NULL, NULL, &restart_width, &restart_height);
        restart_rect.x = (WINDOW_WIDTH - restart_width) / 2;
        restart_rect.y = (WINDOW_HEIGHT / 2) + 20; // Adjust as needed
        restart_rect.w = restart_width;
        restart_rect.h = restart_height;
        SDL_RenderCopy(renderer, restart_texture, NULL, &restart_rect);
        TTF_Font* font_text = TTF_OpenFont("PermanentMarker-Regular.ttf", 32); // Ajustez la taille selon les besoins
        if (font_text == NULL) {
            fprintf(stderr, "Error loading font for text: %s\n", TTF_GetError());
            exit(1);
        }
        render_score(renderer, font_text, white, score); // Utilisez la fonction render_score pour afficher le score
        TTF_CloseFont(font_text); // Fermez la police après utilisation
        break;
    }

    

    SDL_RenderPresent(renderer);


    /*    // Draw a rectangle for the ball object
    SDL_Rect ball_rect = {
        (int)ball.x,
        (int)ball.y,
        (int)ball.width,
        (int)ball.height
    };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &ball_rect);

    SDL_RenderPresent(renderer);

    */
}


///////////////////////////////////////////////////////////////////////////////
// Function to destroy SDL window and renderer
///////////////////////////////////////////////////////////////////////////////
void destroy_window(void) {
    // Libérer les textures du vaisseau spatial
    SDL_DestroyTexture(ship_texture_up);
    SDL_DestroyTexture(ship_texture_right);
    SDL_DestroyTexture(ship_texture_left);
    SDL_DestroyTexture(enemy_ship_texture);
    SDL_DestroyTexture(projectile_texture);
    SDL_DestroyTexture(background_texture);
    SDL_DestroyTexture(heart_texture);
    Mix_FreeMusic(backgroundMusic);
    
    backgroundMusic = NULL;
    

    // Quitter SDL_mixer
    Mix_Quit();



    if (font_text != NULL) {
        TTF_CloseFont(font_text);
        font_text = NULL;
    }

    SDL_DestroyTexture(game_over_texture);
    SDL_DestroyTexture(restart_texture);
    SDL_DestroyTexture(title_texture);
    SDL_DestroyTexture(press_enter_texture);
    SDL_DestroyTexture(limited_edition_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* args[]) {
    srand(time(NULL)); // Initialisation pour la génération de nombres aléatoires
    game_is_running = initialize_window();

    setup();

    while (game_is_running) {
        process_input();

       update();

        render();
    }

    destroy_window();

    return 0;
}
