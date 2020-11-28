#include "../include/Juego.h"
#include <unistd.h>

using namespace std;

Juego::Juego()
{
    //Para posicion aleatoria presas
    srand(time(NULL));

    initscr();
    //para que no imprima el caracter que se escriba
    noecho();
    cbreak();
    keypad(stdscr, true);
    //para que no se muestre el cursor
    curs_set(0);
    //para que no se bloquee mientras espera por una tecla
    nodelay(stdscr, TRUE);

    start_color();

    //color rojo modificado
    init_color(COLOR_RED, 650, 2, 2);
    //color gris personalizado

    //definicion de los demas colores
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_BLACK);
    init_pair(3, COLOR_BLACK, COLOR_GREEN);
    init_pair(4, COLOR_BLACK, COLOR_RED);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);

    //se pinta toda la terminal blanca
    wbkgd(stdscr, COLOR_PAIR(1));
    refresh();

    //definicion de niveles
    Nivel nivel1(50, 1, true);
    Nivel nivel2(150, 2, false);
    Nivel nivel3(300, 3, false);
    niveles = { nivel1, nivel2, nivel3 };
    nivel_actual = 0;
}

//imprime menu principal
void Juego::mainMenu()
{
    int i = 0;
    //se imprime la serpiente en pixel art

    int termH;
    int termW;
    getmaxyx(stdscr, termH, termW);

    int xsnake = (termW - 2 * snake_pxart.size()) / 2;

    for (; i < snake_pxart.size(); i++) {
        for (int j = 0; j < snake_pxart[i].size(); j++) {
            attron(COLOR_PAIR(snake_pxart[i][j] + 1));
            mvprintw(i + 2, 2 * j + xsnake, "  ");
            usleep(WAIT_TIME);
            refresh();
        }
    }

    //color negro para los textos
    attron(COLOR_PAIR(1));
    refresh();

    i += 3;

    //se imprime el texto con la informacion
    for (int k = 0; k < toprint.size(); k++) {
        string str = toprint[k];
        int x = (termW - str.length()) / 2;
        for (char& c : str) {
            mvaddch(i + k, x, chtype(c));
            refresh();
            usleep(2 * WAIT_TIME);
            x++;
        }

        //espacio adicional para los nombres
        if (k < 2 || k > 3)
            i++;
    }

    refresh();

    int ch = getch();

    while (true) {
        ch = getch();
        //si presiona enter, continua al juego
        if (ch == 10) {
            break;
        }
        //si presiona escape, se sale del juego
        else if (ch == 27) {
            erase();
            endwin();
            exit(0);
        }
    }
}

//actualiza el estado de la serpiente y del juego
bool Juego::update(int& ch)
{
    //para saber si el juego se termino por un mal movimiento
    bool gameFinished = false;
    
    Serpiente* snake = t->getSnake();
    vector<Punto>* presas = t->getPresas();

    //Se verifica si la culebra toca el muro al moverse
    bool touchesWall = false;
    Punto newCabeza = snake->moverCabeza(ch, touchesWall);
    //en caso de que toque las paredes y no este permitido, se termina el juego
    if (touchesWall && !niveles[nivel_actual].getpasaParedes()) 
    {
        gameFinished = true;
    }
    else {
        //revisamos si la posicion de la cabeza al moverse coincide con la de una presa
        int index = t->getPuntoIndex(newCabeza.getX(), newCabeza.getY(), presas);
        if (index > -1) {
            //de ser así, eliminamos la presa
            presas->erase(presas->begin() + index);
            //emitimos un sonido
            beep();
            //aumentamos puntake
            player->addCurrScore(10); //suma puntos por comer
            //modificamos el cuerpo de la culebra
            snake->comer(newCabeza);
            //generamos una nueva presa
            int np = 1;
            //si se pasa pasa de nivel cuando se come, se generan distintas presas
            if (player->getCurrScore() >= niveles[nivel_actual].getScore() && nivel_actual < niveles.size() - 1) {
                    nivel_actual++;
                    //se crea un nuevo tablero
                    t = new Tablero();
                    t->getSnake()->setD(KEY_UP);
                    np = niveles[nivel_actual].getNPresas();
                    gameFinished = false;
            }
            //se generan las presas
            t->randomXY(np);
        }
        else {
            //se valida si la culebra toca alguna parte del cuerpo
            int index = t->getPuntoIndex(newCabeza.getX(), newCabeza.getY(), snake->getCuerpo());
            if (index == -1 || (index == snake->getCuerpo()->size() - 1 && snake->getCuerpo()->size() <= N)) {
                snake->moverse(ch);
            }
            //de ser asi, se muere
            else {
                gameFinished = true;
            }
        }
    }
    return gameFinished;
}

void Juego::jugar()
{
    //variable para saber si el juego se termino
    bool gameFinished = false;
    //nombre de usuario
    string uname = "";

    while (true) {
        //imprime menu principal
        int termH;
        int termW;
        getmaxyx(stdscr, termH, termW);

        mainMenu();
        clear();
        refresh();

        //creamos el tablero
        t = new Tablero();

        if (uname == "") {
            string message = "Ingrese su nick:";
            //se lee el nombre
            uname = t->readLine(message);
            //se crea el jugador
            player = new Jugador(uname);
            usleep(WAIT_TIME);
        }

        //numero de presas a imprimir para el nivel actual
        int np = niveles[nivel_actual].getNPresas();
        t->randomXY(np);

        int ch = KEY_UP;

        //imprimir estadisticas
        string user = "Jugador: " + uname;
        mvprintw(2, (termW - (M + 4)) / 2, user.c_str());

        while (true) {
            //imprime el nivel y puntaje y se actualiza
            string nivel = "Nivel: " + to_string(nivel_actual + 1);
            mvprintw(2, (termW + (M + 4)) / 2 - nivel.length(), nivel.c_str());

            string score = "Puntaje: " + to_string(player->getCurrScore());
            mvprintw(3, (termW - (M + 4)) / 2, score.c_str());

            bool pparedes = niveles[nivel_actual].getpasaParedes();
            //imprimimos el tablero tomando en cuenta si se puede pasar paredes
            t->printGrid(pparedes);

            int aux_ch = getch();

            //valida que se presione ESC o las fechas de direccion, las demas teclas son ignoradas
            if (aux_ch != ERR && (aux_ch == 27 || (aux_ch == KEY_UP || aux_ch == KEY_DOWN || aux_ch == KEY_LEFT || aux_ch == KEY_RIGHT))) {
                //si es la tecla de ESC, se sale
                if (aux_ch == 27) {
                    erase();
                    endwin();
                    exit(0);
                }
                else if ((ch == KEY_DOWN && aux_ch != KEY_UP) || (ch == KEY_UP && aux_ch != KEY_DOWN) || (ch == KEY_LEFT && aux_ch != KEY_RIGHT) || (ch == KEY_RIGHT && aux_ch != KEY_LEFT)) 
                {
                    //cuando cambio de direccion a otro eje, se actualiza el cambio
                    ch = aux_ch;
                }
            }
            
            //se verifica si se termino el juego al presionar la tecla
            gameFinished = update(ch);
            //lo que hace una vez pierde
            if (gameFinished) {
                usleep(WAIT_TIME);
                clear();

                //muestra el fin de juego
                t->printGameOver(pparedes);
                usleep(1e6);
                
                //reestablece la variable
                gameFinished = false;

                //reestablece el jugador
                player->checkMaxScore();
                player->setNewFile();
                player->setCurrScore(0);
                //reestablece el nivel
                nivel_actual = 0;
                //reestablece la tecla
                ch = KEY_UP;
                break;
            }
        }
    }
    //elimina la ventana
    endwin();
}
