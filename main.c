#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include "constants.h"


// Ajoutez cet énum pour gérer les états du jeu
typedef enum {
    GAME_STATE_TITLE_SCREEN,
    GAME_STATE_MENU,
    // Ajoutez d'autres états ici si nécessaire
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
} GameState;

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////
int game_is_running = false;
int last_frame_time = 0;
// Ajoutez une nouvelle variable globale pour la vitesse du vaisseau
const int SHIP_SPEED = 300;
const int SHIP_TURN_TIME = 200; // Temps en millisecondes pour afficher le vaisseau tournant
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



///////////////////////////////////////////////////////////////////////////////
// Function to initialize our SDL window
///////////////////////////////////////////////////////////////////////////////
int initialize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
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
                    // Ici, redémarrez le jeu ou retournez au menu principal
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

///////////////////////////////////////////////////////////////////////////////
// Render function to draw game objects in the SDL window
///////////////////////////////////////////////////////////////////////////////
void render(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect start_game_rect;

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
        break;
    case GAME_STATE_GAME_OVER:
        // Dessinez l'écran de game over ici
        // Inclure le score final et les options pour recommencer ou quitter
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
    SDL_DestroyTexture(background_texture);
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
