#pragma once
#include <vector>
#include <tuple>
#include <random>
#include <mutex>

// O------------------------------------------------------------------------------O
// | The games internal representation of each cell                               |
// O------------------------------------------------------------------------------O
struct Cell
{
	int x, y;
	bool flag = false;
	bool clicked = false;
	bool mine = false;
	int neighbouringMines = 0;
	std::vector<Cell*> neighbouringCells;
};

// O------------------------------------------------------------------------------O
// | The cell values visible to the player and engine. The true value of		  |
// | neighbouringMines and mine is only revealed once the cell is clicked,		  |
// | and altering these parameters has no influence on how the game is played out.|
// | Additional variables are used to store computational results of the engine.  |
// O------------------------------------------------------------------------------O
struct VisibleCell
{
	int x, y;
	std::vector<VisibleCell*> neighbouringCells;
	int neighbouringMines = 0;
	bool clicked = false;
	bool mine = false;
	bool flag = false;

	/*Variables stored by the engine*/
	double mineProbability = -1.0f;
	bool knownMine = false;
	int timesConstrained = 0;
	bool isConstrained = false;
	bool simMine = false;
	long long validSimMines = 0;
	int connectedComponent = -1;
};

// O------------------------------------------------------------------------------O
// | The engines internal representation of knowledge about mine locations,		  |
// | i.e. associations between cells and mine counts with 100% certainty.		  |
// | The engine will perform operations on this data (cf. updateKnowledge)		  |
// O------------------------------------------------------------------------------O
struct KnowledgeDatum
{
	int mineCount;
	char x, y;
	bool updated = false;
	std::vector<VisibleCell*> neighbouringCells;
};

enum class MoveType { MOVE_PROBABILISTIC, MOVE_NOMOVE, MOVE_DETERMINISTIC, MOVE_FIRSTCLICK };

// O------------------------------------------------------------------------------O
// | Used to control the stochastic method the engine employs.					  |
// | METHOD_RND:					stochasticMove_random						  |
// | METHOD_SINGLECONSTRAINT :		stochasticMove_singleConstraint				  |
// | METHOD_AVGCONSTRAINT :			stochasticMove_averageConstraint			  |
// | METHOD_BACKTRACKING			stochasticMove_BoundaryBacktracking			  |
// O------------------------------------------------------------------------------O
enum class StochasticMethod { METHOD_RND, METHOD_SINGLECONSTRAINT, METHOD_AVGCONSTRAINT, METHOD_BACKTRACKING };

//Forward declaration
class CppSweeper;

// O------------------------------------------------------------------------------O
// | Representation of the last move of the engine	                              |
// O------------------------------------------------------------------------------O
struct AI_Move
{
	MoveType moveType;
	int moveNo;
	double probability;
	int x, y;
};

// O------------------------------------------------------------------------------O
// | Stores data on each connected component		                              |
// O------------------------------------------------------------------------------O
struct ConnectedComponent
{
	std::vector<VisibleCell*> cellsToSet;
	std::vector<VisibleCell*> boundary;
	long long validSamples = 0;
	int label = -1;
};

// O------------------------------------------------------------------------------O
// | The engine class. The updateKnowledge-method is called by the game-class	  |
// | after each executed move to ensure that the engine's board state			  |
// | representation is accurate.												  |
// | Call the move-method to return the cell coordinates the engine deems optimal.|
// | Set the stochasticMethod-variable to control which function is used once no  |
// | cell is known to be safe.													  |
// O------------------------------------------------------------------------------O
class CppSweeper_AI
{
private:
	std::vector<ConnectedComponent> components;
	std::vector<KnowledgeDatum> knowledge;
	//Used to distribute maxSamples over s subsearches, i.e. maxSamples_=maxSamples/s (used by stochasticMove_BoundaryBacktracking)
	long long maxSamples_ = 0;
	long long samplesCurrentCycle_ = 0;
	long long totalSamples_ = 0;
	long long validSamples_ = 0;
	int unconstrainedCells = 0;
	int _minProbX = -1;
	int _minProbY = -1;
	int knownMines = 0;
	int labelConnectedComponents(CppSweeper* game, std::vector<VisibleCell*>* cellsToSet, std::vector<VisibleCell*>* boundary);
	void setProbabilitiesFromSamples(CppSweeper* game, std::vector<VisibleCell*>* cellsToSet);
	int nChoosek(int n, int k);
	void boundaryBacktracking(CppSweeper* game, std::vector<VisibleCell*>* boundary, std::vector<VisibleCell*>* cellsToSet, std::vector<VisibleCell*>::iterator cellToSet, int remainingMines);
	bool contains(std::vector<VisibleCell*> cells1, std::vector<VisibleCell*> cells2);
	bool checkConstraints(std::vector<VisibleCell*>* boundary);
	bool checkLocalUpperConstraints(VisibleCell* cellToSet);
	void label(CppSweeper* game, std::vector<VisibleCell*>* cellsToSet, std::vector<VisibleCell*>::iterator currentCell, std::vector<VisibleCell*>* boundary, int prevLabel);
	std::tuple<int, int> stochasticMove_BoundaryBacktracking(CppSweeper* game);
	std::tuple<int, int> stochasticMove_averageConstraint(CppSweeper* game);
	std::tuple<int, int> stochasticMove_singleConstraint(CppSweeper* game);
	std::tuple<int, int> stochasticMove_random(CppSweeper* game);
	std::tuple<int, int> getMinimumProbabilityCell(CppSweeper* game);
public:
	std::mutex* m;
	bool rotate = true;
	bool interrupt = false;
	long long maxSamples = 1000000;
	long long moves;
	long long guesses;
	AI_Move lastMove;
	int connectedComponents() { return components.size(); }
	int minProbX() { return _minProbX; }
	int minProbY() { return _minProbY; }
	long long samples() { return totalSamples_ + samplesCurrentCycle_; }
	long long validSamples() { return validSamples_; }
	const std::vector<KnowledgeDatum>& getKnowledge() { return knowledge; }
	void sortKnowledge();
	void updateKnowledge(CppSweeper* game, int x, int y);
	StochasticMethod stochasticMethod = StochasticMethod::METHOD_BACKTRACKING;
	std::tuple<int, int> move(CppSweeper* game);
	void toggleFlags(CppSweeper* game);
	void reset();
	CppSweeper_AI();
};

// O------------------------------------------------------------------------------O
// | The game-class.															  |  
// | Call click to execute a move; toggleFlag to set/delete a flag.				  |
// | A class attribute is a pointer to a CppSweeper_AI instance, to ensure		  |
// | that the engine "shadows" the game (through updateKnowledge).				  |
// O------------------------------------------------------------------------------O
class CppSweeper
{
private:
	Cell* field;
	VisibleCell* visibleField;
	std::default_random_engine generator;
	bool firstClick_ = true;
	int flagCount_ = mineCount;
	int uncoveredCells_ = 0;
	bool gameWon_ = false;
	bool gameLost_ = false;
	int wins_ = 0;
	int losses_ = 0;
	void uncoverNeighbours(int x, int y);
	void generateField(int safeX, int safeY);
	std::vector<Cell*> getNeighbourCells(int x, int y);
public:
	int width = 30;
	int height = 16;
	int mineCount = 99;
	bool firstClick_zeroNeighbours = false;
	std::tuple<int, int> lastClicked;
	CppSweeper_AI* AI;

	std::vector<VisibleCell*> getVisibleNeighbourCells(int x, int y);
	VisibleCell* getCell(int x, int y);
	VisibleCell* getCell(std::tuple<int, int> coord);
	int flagCount() { return flagCount_; }
	int uncoveredCells() { return uncoveredCells_; }
	bool firstClick() { return firstClick_;  }
	int wins() { return wins_;	}
	int losses() { return losses_; }
	bool gameWon() { return gameWon_; }
	bool gameLost() { return gameLost_; }
	bool click(int x, int y);
	void toggleFlag(int x, int y);
	void resetGame();
	void resetStats() { 
		wins_ = 0;
		losses_ = 0;
	}
	CppSweeper();
	~CppSweeper();
};

