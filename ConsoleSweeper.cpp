#define OLC_PGE_APPLICATION
#include "CppSweeper.h"
#include "olcPixelGameEngine.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <tuple>
#include <chrono>
#include <thread>
#include <mutex>

enum class AIJob { MOVE, MOVE_EXECUTE, GAME, GAMELOOP };

class ConsoleSweeper : public olc::PixelGameEngine
{
public:
    CppSweeper game;
    CppSweeper_AI AI;
    std::thread ai_thread;
    std::mutex m;
    //The queue of moves made by the AI, to be displayed in the top menu
    std::vector<AI_Move> AImoves;
    //1: Beginner, 2: Advanced, 3: Expert
    char difficulty = 3;
    //The width and height of each cell in pixels
    const int cellWH = 25;
    //The width of the border between each cell
    const int borderW = 3;

    bool showEngineInternals = true;

    //Height of the upper menu which displays Cyan, the Knowledge Data, Probabilities and Decisions
    const int menuH = 200;

    //Width of the left hand side menu
    const int menuW = 280;

    //The top left corner of the playing field
    int fieldPosX = menuW + borderW + 35;
    int fieldPosY = menuH + borderW + 10;

    bool ai_thread_spawned = false;
    bool ai_thread_working = false;
    bool ai_thread_interrupt = false;
    //The number of AI games to to be done (unless interrupted)
    int ai_loop_counter = 0;
    //The total number of AI games done in a loop
    int ai_loop_games = 0;
    //The entire computation time spent on AI games
    float ai_loop_time = 0.0f;
    //Time on the current game and since application start
    float gameTime = 0.0f;
    float runTime = 0.0f;

    /*Variables to control the display of Cyan, the upper left hand emoji*/
    const int cyan_blockSize = 5;
    float cyan_drawTime = 0.0f;
    const int cyan_posX = menuW/3;
    const int cyan_posY = 80;
    const int cyan_width = 16;
    const int cyan_height = 13;
    std::string cyan_current;
    const std::string CYAN_DEFAULT =
        "...XXXXXXXXXX..."
        "..X..........X.."
        "..X..........X.."
        ".X..XX....XX..X."
        ".X..XX....XX..X."
        "X..............X"
        "X..............X"
        "X..............X"
        ".X...X....X...X."
        ".X....XXXX....X."
        "..X..........X.."
        "..X..........X.."
        "...XXXXXXXXXX...";

    const std::string CYAN_THINKING =
        "...XXXXXXXXXX..."
        "..X..........X.."
        "..X..........X.."
        ".X..XXX..XXX..X."
        ".X...X....X...X."
        "X..............X"
        "X..............X"
        "X..............X"
        ".X............X."
        ".X....XXXX....X."
        "..X..........X.."
        "..X..........X.."
        "...XXXXXXXXXX...";

    const std::string CYAN_SHOCKED =
        "...XXXXXXXXXX..."
        "..X..........X.."
        "..X..........X.."
        ".X..XX....XX..X."
        ".X..XX....XX..X."
        "X..............X"
        "X..............X"
        "X.....XXX......X"
        ".X...X...X....X."
        ".X....XXX.....X."
        "..X..........X.."
        "..X..........X.."
        "...XXXXXXXXXX...";

    const std::string CYAN_HAPPY =
        "...XXXXXXXXXX..."
        "..X..........X.."
        "..X..........X.."
        ".X...X....X...X."
        ".X..X.X..X.X..X."
        "X..............X"
        "X..............X"
        "X..............X"
        ".X...X....X...X."
        ".X....XXXX....X."
        "..X..........X.."
        "..X..........X.."
        "...XXXXXXXXXX...";

    const std::string CYAN_DEFEATED =
        "...XXXXXXXXXX..."
        "..X..........X.."
        "..X..........X.."
        ".X..X.X..X.X..X."
        ".X...X....X...X."
        "X..............X"
        "X..............X"
        "X..............X"
        ".X............X."
        ".X....XXXX....X."
        "..X..X....X..X.."
        "..X..........X.."
        "...XXXXXXXXXX...";

    bool OnUserCreate() override
    {
        game.AI = &AI;
        game.AI->m = &m;
        sAppName = "CppSweeper";
        return true;
    }

    bool OnUserDestroy() override
    {
        if (ai_thread_spawned)
        {
            ai_thread_interrupt = true;
            AI.interrupt = true;
            ai_thread.join();
        }
        return true;
    }

    std::tuple<int, int> ScreenToCell(int x, int y)
    {
        std::tuple<int, int> fieldCoord = std::tuple<int, int>(-1, -1);
        if ((x > fieldPosX) && (x < fieldPosX + (cellWH + borderW) * game.width + borderW)
            && (y > fieldPosY) && (y < fieldPosY + (cellWH + borderW) * game.height + borderW))
        {
            std::get<0>(fieldCoord) = max(min((x - fieldPosX) / (borderW + cellWH), game.width - 1), 0);
            std::get<1>(fieldCoord) = max(min((y - fieldPosY) / (borderW + cellWH), game.height - 1), 0);
        }

        return fieldCoord;
    }

    std::tuple<int, int> ScreenToCell(std::tuple<int, int> screenCoord)
    {
        return ScreenToCell(std::get<0>(screenCoord), std::get<1>(screenCoord));
    }

    std::tuple<int, int> CellToScreen(int x, int y)
    {
        return std::tuple<int, int>(fieldPosX + x * (borderW + cellWH), fieldPosY + y * (borderW + cellWH));
    }

    bool AIMove(bool execute)
    {
        std::tuple<int, int> move = AI.move(&game);
        while (!m.try_lock());
        if (AImoves.size() > 17)
            AImoves.erase(AImoves.begin());
        AImoves.push_back(AI.lastMove);
        m.unlock();
        if (move != std::tuple<int, int>(-1, -1))
        {
            if (execute)
                game.click(std::get<0>(move), std::get<1>(move));             
            return true;
        }
        else
            return false;
    }

    void AIGame()
    {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        bool successfulMove = true;
        while ((!game.gameWon() && !game.gameLost()) && (successfulMove) && (!ai_thread_interrupt))
        {
            successfulMove = AIMove(true); 
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        ai_loop_time += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000000.0f;
        cyan_drawTime = 0.25f;
        if (game.gameLost())
            cyan_current = CYAN_DEFEATED;   
        else 
            cyan_current = CYAN_HAPPY;
        ai_loop_games++;
       
    }

    void AI_thread(AIJob job)
    {
        ai_thread_working = true;
        switch (job)
        {
            case AIJob::MOVE:
            {
                AIMove(false);
                break;
            }
            case AIJob::MOVE_EXECUTE:
            {
                AIMove(true);
                break;
            }
            case AIJob::GAME:
            {
                AIGame();
                break;
            }
            case AIJob::GAMELOOP:
            {
                while ((!ai_thread_interrupt) && (ai_loop_counter > 0))
                {
                    AIGame();
                    ai_loop_counter--;
                    if ((ai_loop_counter > 0) && (!ai_thread_interrupt))
                    {
                        gameTime = 0.0f;
                        game.resetGame();
                    }
                }
            }
        }
        ai_thread_working = false;
    }

    std::string floatToString(float val, std::streamsize precision)
    {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(precision) << val;
        return stream.str();
    }

    void drawCyan(std::string cyan)
    {
        for (int x = 0; x < cyan_width; x++)
            for (int y = 0; y < cyan_height; y++)
            {
                switch (cyan[x + y * cyan_width])
                {
                case 'X': FillRect(cyan_posX + x * cyan_blockSize, cyan_posY + y * cyan_blockSize, cyan_blockSize, cyan_blockSize, olc::CYAN);
                }
            }
    }

    void drawKnowledge(int posX, int posY)
    {
        int offset = 15;
        DrawString(posX, posY + 5, "KNOWLEDGE", olc::CYAN, 1);
        while (!m.try_lock());
        std::vector<KnowledgeDatum> knowledge = AI.getKnowledge();
        for (auto itr = knowledge.begin(); itr != knowledge.end(); itr++)
        {
            std::string s = std::to_string(itr->mineCount) + " mines @";
            int n = 0;
            bool linebreak = false;
            for (auto itr2 = itr->neighbouringCells.begin(); itr2 != itr->neighbouringCells.end(); itr2++)
            {
                n++;
                if (n > 4)
                {
                    s += "\n";
                    linebreak = true;
                    n = 0;
                }
                int x = (*itr2)->x;
                int y = (*itr2)->y;
                s += "[" + std::to_string(x) + "," + std::to_string(y) + "] ";

            }
            DrawString(posX, posY + offset, s, olc::CYAN, 1);
            offset += 10;
            if (linebreak)
                offset += 10;
            if (offset > menuH - 30)
            {
                DrawString(posX, posY + offset, "(...)", olc::CYAN, 1);
                break;
            }
        }
        m.unlock();
    }

    void drawProbabilities(int posX, int posY)
    {
        int offsetX = 0;
        int offsetY = 45;
        long long samples = AI.samples();
        long long validSamples = AI.validSamples();
        DrawString(posX, posY + 5, "PROBABILISTIC ENGINE", olc::CYAN, 1);
        DrawString(posX, posY + 15, "Samples: " + std::to_string(samples * 100 / AI.maxSamples) + "%", olc::CYAN, 1);
        DrawString(posX, posY + 25, "Connected Components: " + std::to_string(AI.connectedComponents()), olc::CYAN, 1);
        DrawString(posX, posY + 35, "Boundary configurations found: " + std::to_string(validSamples), olc::CYAN, 1);
        std::string s;
        for (int x = 0; x < game.width; x++)
        {
            for (int y = 0; y < game.height; y++)
            {
                if (game.getCell(x, y)->isConstrained)
                {
                    std::string mineProbability;
                    {
                        std::string padding = "";
                        if (int(game.getCell(x, y)->mineProbability * 100) < 100)
                            padding += " ";
                        else if (int(game.getCell(x, y)->mineProbability * 100) < 10)
                            padding += " ";
                        mineProbability = std::to_string(int(game.getCell(x, y)->mineProbability * 100)) + "% " + padding;
                    }

                    std::string padding1, padding2;
                    if (x < 10)
                        padding1 = " ";
                    else
                        padding1 = "";
                    if (y < 10)
                        padding2 = " ";
                    else
                        padding2 = "";
                    s = "[" + std::to_string(x) + "," + padding1 + std::to_string(y) + padding2 + "] " + mineProbability;
                    if (posX + offsetX + 110 > ScreenWidth() - 200)
                    {
                        offsetX = 0;
                        offsetY += 10;
                        if (offsetY > menuH - 30)
                        {
                            DrawString(posX, posY + offsetY, "(...)", olc::CYAN, 1);
                            return;
                        }
                    }
                    olc::Pixel textColour;
                    if ((x == AI.minProbX()) && (y == AI.minProbY()))
                        textColour = olc::GREEN;
                    else
                        textColour = olc::CYAN;
                    DrawString(posX + offsetX, posY + offsetY, s, textColour, 1);
                    offsetX += 110;
                }
            }
        }
    }

    void drawGame()
    {
        FillRect(fieldPosX - borderW, fieldPosY - borderW, borderW + game.width*(borderW+cellWH), borderW + game.height * (borderW + cellWH), olc::DARK_GREY);
        long long validSamples = AI.validSamples();
        for (int x = 0; x < game.width; x++)
        {
            for (int y = 0; y < game.height; y++)
            {
                std::tuple<int, int> screenCoord = CellToScreen(x, y);
                VisibleCell* cell = game.getCell(x, y);
                //Draw borders between connected components
                if ((x > 0) && (cell->connectedComponent != -1) && (cell->connectedComponent != game.getCell(x - 1, y)->connectedComponent))
                    FillRect(std::get<0>(screenCoord)-borderW, std::get<1>(screenCoord)-borderW, borderW, cellWH + 2*borderW, olc::RED);
                if ((x < game.width - 1) && (cell->connectedComponent != -1) && (cell->connectedComponent != game.getCell(x + 1, y)->connectedComponent))
                    FillRect(std::get<0>(screenCoord) + cellWH, std::get<1>(screenCoord) - borderW, borderW, cellWH + 2 * borderW, olc::RED);
                if ((y < game.height - 1) && (cell->connectedComponent != -1) && (cell->connectedComponent != game.getCell(x, y + 1)->connectedComponent))
                    FillRect(std::get<0>(screenCoord) - borderW, std::get<1>(screenCoord) + cellWH, cellWH + 2 * borderW, borderW, olc::RED);
                if ((y > 0) && (cell->connectedComponent != -1) && (cell->connectedComponent != game.getCell(x, y - 1)->connectedComponent))
                    FillRect(std::get<0>(screenCoord) - borderW, std::get<1>(screenCoord) - borderW, cellWH + 2 * borderW, borderW, olc::RED);
                
                //Highlight the cell with minimum probability in green
                if ((x == AI.minProbX()) && (y == AI.minProbY()))
                    FillRect(std::get<0>(screenCoord) - borderW, std::get<1>(screenCoord) - borderW, cellWH + 2 * borderW, cellWH + 2 * borderW, olc::GREEN);
                
                //Highlight mines in red if the game is lost
                if ((game.gameLost()) && (cell->mine) && (game.lastClicked != std::tuple<int,int>(x,y)))
                    FillRect(std::get<0>(screenCoord), std::get<1>(screenCoord), cellWH, cellWH, olc::RED);
                else if (cell->clicked)
                {
                    //Highlight the clicked mine in dark red
                    if (cell->mine)
                        FillRect(std::get<0>(screenCoord), std::get<1>(screenCoord), cellWH, cellWH, olc::DARK_RED);
                    else
                    {
                        //Draw clicked cells and their count of neighbouring mines
                        FillRect(std::get<0>(screenCoord), std::get<1>(screenCoord), cellWH, cellWH, olc::WHITE);
                        if (cell->neighbouringMines > 0)
                            DrawString(std::get<0>(screenCoord) + cellWH / 4, std::get<1>(screenCoord) + cellWH / 4, std::to_string(cell->neighbouringMines), olc::BLACK, 2);
                    }

                }
                else if (cell->isConstrained)
                {
                    //Draw the cells neighbouring the boundary in cyan and draw their current probability estimate
                    if ((x == AI.minProbX()) && (y == AI.minProbY()))
                        FillRect(std::get<0>(screenCoord) - borderW, std::get<1>(screenCoord) - borderW, cellWH + 2 * borderW, cellWH + 2 * borderW, olc::GREEN);
                    if (cell->simMine)
                        FillRect(std::get<0>(screenCoord), std::get<1>(screenCoord), cellWH, cellWH, olc::CYAN);
                    else
                        FillRect(std::get<0>(screenCoord), std::get<1>(screenCoord), cellWH, cellWH, olc::DARK_CYAN);
                    DrawString(std::get<0>(screenCoord) + cellWH / 10, std::get<1>(screenCoord) + cellWH / 3, std::to_string((int)(cell->mineProbability*100)), olc::RED, 1);
                }
                else
                    FillRect(std::get<0>(screenCoord), std::get<1>(screenCoord), cellWH, cellWH, olc::BLACK);
                if (cell->flag)
                {
                    char c = '!';
                    DrawString(std::get<0>(screenCoord) + cellWH / 4, std::get<1>(screenCoord) + cellWH / 4, std::string(1, c), olc::WHITE, 2);
                }
            }
        }
    }

    void drawMenu()
    {
        if (game.gameLost())
            DrawString(5, menuH + 10, "GAME OVER!", olc::WHITE, 1);
        else if (game.gameWon())
            DrawString(5, menuH + 10, "GAME WON!", olc::WHITE, 1);  
        else
            DrawString(5, menuH + 10, "Playing...", olc::WHITE, 1);
        DrawString(5, menuH + 20, std::to_string(gameTime) + " Seconds", olc::WHITE, 1);
        DrawString(5, menuH + 30, "Flags: " + std::to_string(game.flagCount()), olc::WHITE, 1);

        DrawString(5, menuH + 60, "KEYS ", olc::WHITE, 1);
        olc::Pixel textColour = olc::WHITE;
        if (difficulty == 1)
            textColour = olc::GREEN;
        DrawString(5, menuH + 70, "1     : Beginner 9x9,10", textColour, 1);
        textColour = olc::WHITE;
        if (difficulty == 2)
            textColour = olc::GREEN;
        DrawString(5, menuH + 80, "2     : Intermediate 16x16,40", textColour, 1);
        textColour = olc::WHITE;
        if (difficulty == 3)
            textColour = olc::GREEN;
        DrawString(5, menuH + 90, "3     : Expert 30x16,99", textColour, 1);
        DrawString(5, menuH + 100, "SPACE : Reset Game", olc::WHITE, 1);
        DrawString(5, menuH + 110, "Z     : Toggle 0-cell on 1st click (" + std::to_string(game.firstClick_zeroNeighbours) + ")", olc::WHITE, 1);
        DrawString(5, menuH + 120, "M     : AI Move", olc::WHITE, 1);
        DrawString(5, menuH + 130, "G     : AI Game", olc::WHITE, 1);
        DrawString(5, menuH + 140, "L     : Toggle AI Loop", olc::WHITE, 1);
        DrawString(5, menuH + 150, "S     : Toggle Stochastic Method", olc::WHITE, 1);
        std::string maxSamples;
        switch (AI.stochasticMethod) {
        case StochasticMethod::METHOD_BACKTRACKING:
            DrawString(5, menuH + 160, "  (1) Boundary Backtracking", olc::WHITE, 1);
            if (AI.maxSamples >= 1000000)
                maxSamples = std::to_string(AI.maxSamples / 1000000) + " mil";
            else
                maxSamples = std::to_string(AI.maxSamples / 1000) + " k";
            DrawString(5, menuH + 170, "  +/-: Adjust Samples (" + maxSamples + ")", olc::WHITE, 1);
            break;
        case StochasticMethod::METHOD_AVGCONSTRAINT:
            DrawString(5, menuH + 160, "  (2) Min. Average Constraint", olc::WHITE, 1);
            break;
        case StochasticMethod::METHOD_SINGLECONSTRAINT:
            DrawString(5, menuH + 160, "  (3) Min. Single Constraint", olc::WHITE, 1);
            break;
        default:
            DrawString(5, menuH + 160, "  (4) Random", olc::WHITE, 1);
        }

        DrawString(5, menuH + 200, "STATS", olc::WHITE, 1);
        DrawString(5, menuH + 210, "Games : " + std::to_string(game.wins() + game.losses()), olc::WHITE, 1);
        DrawString(5, menuH + 220, "Wins  : " + std::to_string(game.wins()) + ", " + std::to_string((((float)game.wins() / (game.wins() + game.losses()))) * 100) + "%", olc::WHITE, 1);
        DrawString(5, menuH + 230, "Losses: " + std::to_string(game.losses()), olc::WHITE, 1);

        DrawString(5, menuH + 260, "AI STATS", olc::WHITE, 1);
        if (ai_loop_games > 0)
            DrawString(5, menuH + 270, "Seconds/game: " + std::to_string(ai_loop_time / ai_loop_games), olc::WHITE, 1);
        DrawString(5, menuH + 280, "Moves       : " + std::to_string(AI.moves));
        DrawString(5, menuH + 290, "Guesses     : " + std::to_string(AI.guesses) + ", " + std::to_string(((float)AI.guesses * 100) / (AI.moves)) + "%");

    }

    void resetStats()
    {
        ai_loop_games = 0;
        ai_loop_time = 0;
        game.resetStats();
        AI.moves = 0;
        AI.guesses = 0;
        gameTime = 0.0f;
    }

    bool gameOver()
    {
        return (game.gameLost() || game.gameWon());
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        runTime += fElapsedTime;

        if (ai_thread_spawned && !ai_thread_working)
        {
            ai_thread.join();
            ai_thread_spawned = false;
        }
            
        if ((GetMouse(0).bReleased))
        {
            std::tuple<int, int> screenCoord = std::tuple<int, int>(GetMouseX(), GetMouseY());

            if ((std::get<0>(screenCoord) >= cyan_posX) && (std::get<0>(screenCoord) <= cyan_posX + cyan_width * cyan_blockSize) &&
                (std::get<1>(screenCoord) >= cyan_posY) && (std::get<1>(screenCoord) <= cyan_posY + cyan_height * cyan_blockSize))
            {
                //Toggle display of the engine internals
                showEngineInternals = !showEngineInternals;
                if ((difficulty == 2) || (difficulty == 3))
                {
                    if (showEngineInternals)
                        fieldPosY = menuH + borderW + 10;
                    else
                        fieldPosY = menuH / 2;
                }

            }
            if ((!ai_thread_spawned) && (!gameOver()))
            {
                //handle the player clicking a field
                std::tuple<int, int> fieldCoord = ScreenToCell(screenCoord);
                if (fieldCoord != std::tuple<int, int>(-1, -1))
                {
                    cyan_current = CYAN_SHOCKED;
                    cyan_drawTime = 0.5f;
                    game.click(std::get<0>(fieldCoord), std::get<1>(fieldCoord));
                    AI.sortKnowledge();
                }
            }
        }
        else if ((GetMouse(1).bReleased) && (!ai_thread_spawned) && (!gameOver()))
        {
            //Handle the player toggling a flag
            std::tuple<int, int> fieldCoord = ScreenToCell(GetMouseX(), GetMouseY());
            if (fieldCoord != std::tuple<int, int>(-1, -1))
                game.toggleFlag(std::get<0>(fieldCoord), std::get<1>(fieldCoord));
        }
        else if ((GetKey(olc::Key::SPACE).bPressed) && (!ai_thread_spawned))
        {
            gameTime = 0.0f;
            game.resetGame();
        }
        else if ((GetKey(olc::Key::V).bPressed) && (!ai_thread_spawned))
            AI.rotate = !AI.rotate;
        else if ((GetKey(olc::Key::Z).bPressed) && (!ai_thread_spawned))
            game.firstClick_zeroNeighbours = !game.firstClick_zeroNeighbours;
        else if ((GetKey(olc::Key::M).bHeld) && (!gameOver()) && (!ai_thread_spawned))
        {
            cyan_current = CYAN_THINKING;
            cyan_drawTime = 0.5f;
            ai_thread_interrupt = false;
            ai_thread_spawned = true;
            ai_thread = std::thread(&ConsoleSweeper::AI_thread, this, AIJob::MOVE_EXECUTE);
            AI.sortKnowledge();
        }
        else if ((GetKey(olc::Key::N).bPressed) && (!gameOver()) && (!ai_thread_spawned))
        {
            cyan_current = CYAN_THINKING;
            cyan_drawTime = 0.5f;
            ai_thread_interrupt = false;
            ai_thread_spawned = true;
            ai_thread = std::thread(&ConsoleSweeper::AI_thread, this, AIJob::MOVE);
        }
        else if ((GetKey(olc::Key::L).bPressed))
        {
            if (!ai_thread_spawned)
            {
                ai_thread_interrupt = false;
                ai_loop_counter = 10000;
                ai_thread_spawned = true;
                ai_thread = std::thread(&ConsoleSweeper::AI_thread, this, AIJob::GAMELOOP);
            }
            else
            {
                AI.interrupt = true;
                ai_thread_interrupt = true;
            }
        }
        else if ((GetKey(olc::Key::G).bPressed) && (!game.gameLost() && !game.gameWon()))
        {
            if (!ai_thread_spawned)
            {
                ai_thread_interrupt = false;
                ai_loop_counter = 1;
                ai_thread_spawned = true;
                ai_thread = std::thread(&ConsoleSweeper::AI_thread, this, AIJob::GAME);
            }
            else
            {
                ai_thread_interrupt = true;
                AI.interrupt = true;
            }
        }
        else if (GetKey(olc::Key::K1).bPressed)
        {
            if (ai_thread_spawned)
            {
                ai_thread_interrupt = true;
                AI.interrupt = true;
                ai_thread.join();
                ai_thread_spawned = false;
            }   
            difficulty = 1;
            fieldPosX = menuW + borderW + 300;
            fieldPosY = menuH + borderW + 100;
            resetStats();
            game.width = 9;
            game.height = 9;
            game.mineCount = 10;
            game.resetGame();
        }
        else if (GetKey(olc::Key::K2).bPressed)
        {
            if (ai_thread_spawned)
            {
                ai_thread_interrupt = true;
                AI.interrupt = true;
                ai_thread.join();
                ai_thread_spawned = false;
            }
            difficulty = 2;
            fieldPosX = menuW + borderW + 200;
            if (showEngineInternals)
                fieldPosY = menuH + borderW;
            else
                fieldPosY = menuH / 2;
            resetStats();
            game.width = 16;
            game.height = 16;
            game.mineCount = 40;
            game.resetGame();
        }
        else if (GetKey(olc::Key::K3).bPressed)
        {
            if (ai_thread_spawned)
            {
                ai_thread_interrupt = true;
                AI.interrupt = true;
                ai_thread.join();
                ai_thread_spawned = false;
            }
            difficulty = 3;
            fieldPosX = menuW + borderW + 35;
            if (showEngineInternals)
                fieldPosY = menuH + borderW + 10;
            else
                fieldPosY = menuH / 2;
            resetStats();
            game.width = 30;
            game.height = 16;
            game.mineCount = 99;
            game.resetGame();
        }
        else if (GetKey(olc::Key::NP_ADD).bPressed)
        {
            if (AI.maxSamples >= 10000000)
                AI.maxSamples += 10000000;
            else if (AI.maxSamples >= 1000000)
                AI.maxSamples += 1000000;
            else if (AI.maxSamples >= 100000)
                AI.maxSamples += 100000;
            else if(AI.maxSamples >= 10000)
                AI.maxSamples += 10000;
            else
                AI.maxSamples += 1000;
        }
        else if ((GetKey(olc::Key::NP_SUB).bPressed))
        {
            if (AI.maxSamples > 10000000)
                AI.maxSamples -= 10000000;
            else if (AI.maxSamples > 1000000)
                AI.maxSamples -= 1000000;
            else if (AI.maxSamples > 100000)
                AI.maxSamples -= 100000;
            else if (AI.maxSamples > 10000)
                AI.maxSamples -= 10000;
            else if (AI.maxSamples > 1000)
                AI.maxSamples -= 1000;
        }
        else if (GetKey(olc::Key::S).bPressed)
        {
            if (AI.stochasticMethod == StochasticMethod::METHOD_BACKTRACKING)
                AI.stochasticMethod = StochasticMethod::METHOD_AVGCONSTRAINT;
            else if (AI.stochasticMethod == StochasticMethod::METHOD_AVGCONSTRAINT)
                AI.stochasticMethod = StochasticMethod::METHOD_SINGLECONSTRAINT;
            else if (AI.stochasticMethod == StochasticMethod::METHOD_SINGLECONSTRAINT)
                AI.stochasticMethod = StochasticMethod::METHOD_RND;
            else
                AI.stochasticMethod = StochasticMethod::METHOD_BACKTRACKING;
            resetStats();
        }

        if (!gameOver())
            gameTime += fElapsedTime;

        Clear(olc::BLACK);
        drawGame();
        drawMenu();

        if (ai_thread_spawned)
        {
            switch (((int)(runTime*2) % 3))
            {
                case 0:
                    DrawString(cyan_posX, cyan_posY - 30, "Calculating.", olc::CYAN, 1);
                    break;
                case 1:
                    DrawString(cyan_posX, cyan_posY - 30, "Calculating..", olc::CYAN, 1);
                    break;
                default:
                    DrawString(cyan_posX, cyan_posY - 30, "Calculating...", olc::CYAN, 1);
            }
        }
        if (cyan_drawTime > 0.0f)
        {
            drawCyan(cyan_current);
            cyan_drawTime -= fElapsedTime;
        }
        else if (ai_thread_spawned)
            drawCyan(CYAN_THINKING);
        else if (game.gameWon())
        {
            DrawString(cyan_posX, cyan_posY - 20, "Yay!", olc::CYAN, 1);
            drawCyan(CYAN_HAPPY);
        }
        else if (game.gameLost())
        {
            DrawString(cyan_posX, cyan_posY - 20, "Ouch..", olc::CYAN, 1);
            drawCyan(CYAN_DEFEATED);
        }
        else if ((game.firstClick()) && (game.losses() + game.wins() == 0))
        {
            DrawString(cyan_posX, cyan_posY - 30, "Hi, I'm Cyan.", olc::CYAN, 1);
            DrawString(cyan_posX - 60, cyan_posY - 20, "Make your turn, or let me have a go.", olc::CYAN, 1);
            DrawString(cyan_posX - 60, cyan_posY + cyan_height*cyan_blockSize + 10, "CLICK ME to show/hide my internals", olc::CYAN, 1);
            drawCyan(CYAN_DEFAULT);
        }
        else
            drawCyan(CYAN_DEFAULT);
  
        if (showEngineInternals)
        {
            drawKnowledge(menuW + 10, 0);
            DrawString(menuW + 320, 5, "&&", olc::CYAN, 1);
            drawProbabilities(menuW + 20 + 320, 0);
            DrawString(ScreenWidth() - 220, 5, "=>", olc::CYAN, 1);
            int offset = 15;

            DrawString(ScreenWidth() - 200, 5, "DECISIONS", olc::CYAN, 1);
            while (!m.try_lock());
            for (auto itr = AImoves.begin(); itr != AImoves.end(); ++itr)
            {
                switch (itr->moveType)
                {
                case MoveType::MOVE_DETERMINISTIC:
                    DrawString(ScreenWidth() - 200, 0 + offset, std::to_string(itr->moveNo) + ". Safe (" + std::to_string(itr->x) + "," + std::to_string(itr->y) + ")", olc::CYAN, 1);
                    break;
                case MoveType::MOVE_FIRSTCLICK:
                    DrawString(ScreenWidth() - 200, 0 + offset, std::to_string(itr->moveNo) + ". First Click (" + std::to_string(itr->x) + "," + std::to_string(itr->y) + ")", olc::CYAN, 1);
                    break;
                case MoveType::MOVE_NOMOVE:
                    DrawString(ScreenWidth() - 200, 0 + offset, "No move possible", olc::CYAN, 1);
                    break;
                case MoveType::MOVE_PROBABILISTIC:
                    DrawString(ScreenWidth() - 200, 0 + offset, std::to_string(itr->moveNo) + ". Guess (" + std::to_string(itr->x) + "," + std::to_string(itr->y) + ") " + floatToString(itr->probability * 100, 2) + "%", olc::CYAN, 1);
                    break;
                }
                offset += 10;
            }
            m.unlock();
        }

        //Cap framerate
        if (fElapsedTime < 0.02f)
            Sleep((int)((0.02f - fElapsedTime) * 1000));

        return true;
    }
};



int main()
{
    ConsoleSweeper game;
    if (game.Construct(1185, 685, 1, 1))
    {
        game.Start();
    }

}