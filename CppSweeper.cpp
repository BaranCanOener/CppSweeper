#include "CppSweeper.h"
#include <random>
#include <time.h>
#include <algorithm>

#define coord(x,y) x+(y)*(width)

std::vector<Cell*> CppSweeper::getNeighbourCells(int x, int y)
{
	std::vector<Cell*> neighbours;

	if (x > 0)
	{
		neighbours.push_back(&field[coord(x - 1, y)]);
		if (y > 0)
			neighbours.push_back(&field[coord(x - 1, y - 1)]);
		if (y < height - 1)
			neighbours.push_back(&field[coord(x - 1, y + 1)]);
	}
	if (x < width - 1)
	{
		neighbours.push_back(&field[coord(x + 1, y)]);
		if (y > 0)
			neighbours.push_back(&field[coord(x + 1, y - 1)]);
		if (y < height - 1)
			neighbours.push_back(&field[coord(x + 1, y + 1)]);
	}
	if (y < height - 1)
		neighbours.push_back(&field[coord(x, y + 1)]);
	if (y > 0)
		neighbours.push_back(&field[coord(x, y - 1)]);

	return neighbours;
}

std::vector<VisibleCell*> CppSweeper::getVisibleNeighbourCells(int x, int y)
{
	std::vector<VisibleCell*> neighbours;

	if (x > 0)
	{
		neighbours.push_back(&visibleField[coord(x - 1, y)]);
		if (y > 0)
			neighbours.push_back(&visibleField[coord(x - 1, y - 1)]);
		if (y < height - 1)
			neighbours.push_back(&visibleField[coord(x - 1, y + 1)]);
	}
	if (x < width - 1)
	{
		neighbours.push_back(&visibleField[coord(x + 1, y)]);
		if (y > 0)
			neighbours.push_back(&visibleField[coord(x + 1, y - 1)]);
		if (y < height - 1)
			neighbours.push_back(&visibleField[coord(x + 1, y + 1)]);
	}
	if (y < height - 1)
		neighbours.push_back(&visibleField[coord(x, y + 1)]);
	if (y > 0)
		neighbours.push_back(&visibleField[coord(x, y - 1)]);

	return neighbours;
}

VisibleCell* CppSweeper::getCell(int x, int y)
{
	return &visibleField[coord(x, y)];
}

VisibleCell* CppSweeper::getCell(std::tuple<int, int> coord)
{
	return &visibleField[coord(std::get<0>(coord), std::get<1>(coord))];
}

bool CppSweeper::click(int x, int y)
{
	if (gameLost_ || gameWon_ || (x >= width) || (x < 0) || (y >= height) || (y < 0) ||
		field[coord(x, y)].flag || field[coord(x, y)].clicked)
		return false;
	
	if (firstClick_)
	{
		generateField(x, y);
		firstClick_ = false;
	}

	lastClicked = std::tuple<int, int>(x, y);
	field[coord(x, y)].clicked = true;
	visibleField[coord(x, y)].clicked = true;
	visibleField[coord(x, y)].neighbouringMines = field[coord(x, y)].neighbouringMines;
	uncoveredCells_++;

	if (field[coord(x, y)].mine) {
		gameLost_ = true;
		for (int x_ = 0; x_ < width; x_++)
			for (int y_ = 0; y_ < height; y_++)
			{
				visibleField[coord(x_, y_)].mine = field[coord(x_, y_)].mine;
			}

		losses_++;
	}
	else if (uncoveredCells_ == width * height - mineCount)
	{
		if (AI != NULL)
			AI->updateKnowledge(this, x, y);
		gameWon_ = true;
		wins_++;
		flagCount_ = 0;
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				if (field[coord(x, y)].mine)
					field[coord(x, y)].flag = true;
				else
				{
					field[coord(x, y)].flag = false;
					field[coord(x, y)].clicked = true;
				}
			}
	}

	else
	{
		if (AI != NULL)
			AI->updateKnowledge(this, x, y);
		if (field[coord(x, y)].neighbouringMines == 0)
			uncoverNeighbours(x, y);
	}
	return true;
}

void CppSweeper::toggleFlag(int x, int y)
{
	if ((flagCount_ > 0) && (!field[coord(x, y)].flag))
	{
		field[coord(x, y)].flag = true;
		visibleField[coord(x, y)].flag = true;
		flagCount_--;
	}
	else if (field[coord(x, y)].flag)
	{
		field[coord(x, y)].flag = false;
		visibleField[coord(x, y)].flag = false;
		flagCount_++;
	}
}

void CppSweeper::uncoverNeighbours(int x, int y)
{
	for (unsigned i = 0; i < field[coord(x, y)].neighbouringCells.size(); i++)
	{
		std::vector<Cell*> neighbours = field[coord(x, y)].neighbouringCells;
		int neighbourX = neighbours.at(i)->x;
		int neighbourY = neighbours.at(i)->y;
		if (!field[coord(neighbourX, neighbourY)].clicked)
			click(neighbourX, neighbourY);
	}
}

void CppSweeper::generateField(int safeX, int safeY)
{
	if (firstClick_zeroNeighbours)
		mineCount = std::min(mineCount, width * height - 9);
	else
		mineCount = std::min(mineCount, width * height - 1);
	int minesToGenerate = mineCount;
	int x, y;

	while (minesToGenerate > 0)
	{
		std::uniform_int_distribution<int> distribution(0, width - 1);
		x = distribution(generator);
		std::uniform_int_distribution<int> distribution2(0, height - 1);
		y = distribution2(generator);
		if (firstClick_zeroNeighbours)
		{
			//Ensure 9 safe cells, i.e. that (x,y) and all its neighbours are not mines
			if (((x != safeX) || (y != safeY)) && ((x != safeX-1) || (y != safeY)) && ((x != safeX+1) || (y != safeY)) &&
				((x != safeX) || (y != safeY-1)) && ((x != safeX) || (y != safeY+1)) && ((x != safeX-1) or (y != safeY-1)) &&
				((x != safeX-1) or (y != safeY+1)) && ((x != safeX+1) or (y != safeY-1)) && ((x != safeX+1) or (y != safeY+1)) && (!field[coord(x, y)].mine))
			{
				field[coord(x, y)].mine = true;
				minesToGenerate--;
			}
		}
		else
		{
			//only ensure (x,y) is not a mine, i.e. one guaranteed safe cell
			if (((x != safeX) or (y != safeY)) && (!field[coord(x, y)].mine))
			{
				field[coord(x, y)].mine = true;
				minesToGenerate--;
			}
		}
	}

	//Update the field and visibleField data
	for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++)
		{
			field[coord(x, y)].neighbouringCells = getNeighbourCells(x, y);
			visibleField[coord(x, y)].neighbouringCells = getVisibleNeighbourCells(x, y);

			std::vector<Cell*> neighbours = field[coord(x, y)].neighbouringCells;
			int mc = 0;
			for (unsigned i = 0; i < neighbours.size(); i++)
				if (neighbours.at(i)->mine)
					mc++;
			field[coord(x, y)].neighbouringMines = mc;
		}
}

void CppSweeper::resetGame()
{
	if (field != nullptr) {
		delete[] field;
		delete[] visibleField;
	}

	field = new Cell[width * height];
	visibleField = new VisibleCell[width * height];
	for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++)
		{
			Cell* cell = &field[coord(x, y)];
			VisibleCell* visibleCell = &visibleField[coord(x, y)];
			cell->x = x;
			cell->y = y;
			visibleCell->x = x;
			visibleCell->y = y;
		}

	gameWon_ = false;
	gameLost_ = false;
	firstClick_ = true;
	uncoveredCells_ = 0;
	flagCount_ = mineCount;

	if ((AI != NULL))
	{
		while (!AI->m->try_lock());
		AI->m->unlock();
		AI->reset();
	}

}

CppSweeper::CppSweeper()
{
	srand(static_cast<unsigned int>(time(nullptr)));
	generator.seed(static_cast<unsigned int>(time(nullptr)));
	resetGame();
}

CppSweeper::~CppSweeper()
{
	delete[] field;
}

//O(n*m) check it cells1 is contained in cells2
bool CppSweeper_AI::contains(std::vector<VisibleCell*> cells1, std::vector<VisibleCell*> cells2)
{
	for (auto itr1 = cells1.begin(); itr1 != cells1.end(); itr1++)
	{
		auto pos = std::find(cells2.begin(), cells2.end(), *itr1);
		if (pos == cells2.end())
			return false;
	}
	return true;
}

void CppSweeper_AI::sortKnowledge()
{
	std::sort(knowledge.begin(), knowledge.end(), [](const KnowledgeDatum& comp1, const KnowledgeDatum& comp2) { return (comp1.mineCount < comp2.mineCount); });
}

//Assumes that x and y have been clicked last and hence updates the knowledge-variable. This method is called by CppSweeper::click()
void CppSweeper_AI::updateKnowledge(CppSweeper* game, int x, int y)
{
	while (!m->try_lock());
	VisibleCell* cell = game->getCell(x, y);
	if ((cell->clicked) && (!cell->mine))
	{
		cell->mineProbability = 0.0f;
		std::vector<KnowledgeDatum*> updatedKnowledge;

		//The clicked cell is not a mine, hence all entries of knowledge can be reduced by this cell
		for (auto it = knowledge.begin(); it != knowledge.end(); it++)
		{
			std::vector<VisibleCell*>* nb = &(it->neighbouringCells);
			auto pos = std::find(nb->begin(), nb->end(), cell);

			if (pos != nb->end())
			{
				nb->erase(pos);
				it->updated = true;
				updatedKnowledge.push_back(&(*it));
			}
		}

		//Initialize a new knowledge datum corresponding to the constraint imposed by the clicked cell
		KnowledgeDatum kd;
		kd.x = x;
		kd.y = y;
		kd.mineCount = cell->neighbouringMines;
		kd.neighbouringCells = cell->neighbouringCells;

		//There is nothing to do if the cell does not impose a constraint
		if (kd.mineCount == 0)
		{
			m->unlock();
			return;
		}

		//Ensure that the new knowledge datum kd is reduced by all already clicked cells and known mines
		{
			std::vector<VisibleCell*>::iterator it = kd.neighbouringCells.begin();
			while (it != kd.neighbouringCells.end())
			{
				if (((*it)->clicked) || ((*it)->knownMine))
				{
					if ((*it)->knownMine)
						kd.mineCount--;
					it = kd.neighbouringCells.erase(it);
				}
				else
					it++;
			}
		}
		knowledge.push_back(kd);

		//This block ensures deduction from subsets. Check whether knowledge-items contain one another, and if so, add the complement with the difference as mineCount
		std::vector<KnowledgeDatum> newKnowledge;
		for (auto it_updated = updatedKnowledge.begin(); it_updated != updatedKnowledge.end(); it_updated++)
		{
			for (auto it_knowledge = knowledge.begin(); it_knowledge != knowledge.end(); it_knowledge++)
				if (((*it_updated)->neighbouringCells.size() > 1) && (it_knowledge->neighbouringCells.size() > 0))
					//Check if *it_knowledge contains *it_updated
					if (((*it_updated)->neighbouringCells != (it_knowledge)->neighbouringCells) && contains((*it_updated)->neighbouringCells, it_knowledge->neighbouringCells))
					{
						KnowledgeDatum complement;
						complement.x = it_knowledge->x;
						complement.y = it_knowledge->y;
						complement.mineCount = it_knowledge->mineCount - (*it_updated)->mineCount;
						for (auto it_neighbour = it_knowledge->neighbouringCells.begin(); it_neighbour != it_knowledge->neighbouringCells.end(); it_neighbour++)
						{

							auto pos = std::find((*it_updated)->neighbouringCells.begin(), (*it_updated)->neighbouringCells.end(), *it_neighbour);
							if (pos == (*it_updated)->neighbouringCells.end())
								complement.neighbouringCells.push_back(*it_neighbour);
						}
						newKnowledge.push_back(complement);
					}
		}
		knowledge.insert(knowledge.end(), newKnowledge.begin(), newKnowledge.end());

		//This block ensures that knowledge where the minecount is equal to the number of neighbouring cells, all these cells are flagged as mines
		std::vector<VisibleCell*> knownMineCells;
		for (auto it_knowledge = knowledge.begin(); it_knowledge != knowledge.end(); it_knowledge++)
		{
			if (it_knowledge->neighbouringCells.size() == it_knowledge->mineCount)
				for (auto it_neighbour = it_knowledge->neighbouringCells.begin(); it_neighbour != it_knowledge->neighbouringCells.end(); it_neighbour++)
				{
					if ((*it_neighbour)->mineProbability != 1.0f)
						knownMines++;
					(*it_neighbour)->mineProbability = 1.0f;
					(*it_neighbour)->knownMine = true;
					knownMineCells.push_back(*it_neighbour);
				}

			//Clean up the knowledge-vector by removing entries that were reduced to zero during previous iterations of the loop
			if (it_knowledge->neighbouringCells.size() == 0)
			{
				it_knowledge = knowledge.erase(it_knowledge);
				if (it_knowledge != knowledge.begin())
					it_knowledge--;
			}
			if (it_knowledge >= knowledge.end())
				break;
		}

		//The reduction step - if a cell is now known to be a mine, it can be removed from all sets of knowledge
		for (auto it = knownMineCells.begin(); it != knownMineCells.end(); it++)
			{
				auto mineCell = *it;
					for (auto it = knowledge.begin(); it != knowledge.end(); it++)
					{
						auto nb = it->neighbouringCells;
						auto pos = std::find(it->neighbouringCells.begin(), it->neighbouringCells.end(), mineCell);
						//mineCell was found, hence remove it from the knowledge item and reduce the minecount
						if (pos != it->neighbouringCells.end())
						{
							it->neighbouringCells.erase(pos);
							it->mineCount--;
						}

						if (it->neighbouringCells.size() == 0)
						{
							it = knowledge.erase(it);
							if (it != knowledge.begin())
								it--;
							if (it == knowledge.end())
								break;
						}
					}
			}
	}
	m->unlock();
}

//Probability = max value imposed by all neighbouring constraints
//The algorithm returns the cell with the minimum probability.
std::tuple<int, int> CppSweeper_AI::stochasticMove_singleConstraint(CppSweeper* game)
{
	int remainingMines = game->mineCount - knownMines;
	double defaultProbability = ((double)remainingMines) / (double)((game->width * game->height - knownMines - game->uncoveredCells()));

	for (int x = 0; x < game->width; x++)
		for (int y = 0; y < game->height; y++)
		{
			game->getCell(x, y)->timesConstrained = 0;
			if (game->getCell(x, y)->clicked)
				game->getCell(x, y)->mineProbability = 0.0f;
			else if (game->getCell(x, y)->knownMine)
				game->getCell(x, y)->mineProbability = 1.0f;
			else
				game->getCell(x, y)->mineProbability = defaultProbability;
		}

	for (auto itr = knowledge.begin(); itr != knowledge.end(); itr++)
	{
		if (itr->neighbouringCells.size() > 0)
		{
			double newProbability = ((double)itr->mineCount) / itr->neighbouringCells.size();
			for (auto itr2 = itr->neighbouringCells.begin(); itr2 != itr->neighbouringCells.end(); itr2++)
			{
				(*itr2)->isConstrained = true;
				if ((!(*itr2)->clicked) && !((*itr2)->flag))
					if ((*itr2)->mineProbability < newProbability)
						(*itr2)->mineProbability = newProbability;
			}
		}
	}

	double minProbability = 1.0f;
	std::tuple<int, int> move = std::tuple<int, int>(-1, 1);
	for (int x = 0; x < game->width; x++)
		for (int y = 0; y < game->height; y++)
		{
			if ((!game->getCell(x, y)->clicked) && (game->getCell(x, y)->mineProbability < minProbability))
			{
				minProbability = game->getCell(x, y)->mineProbability;
				move = std::tuple<int, int>(x, y);
			}

		}

	lastMove.probability = minProbability;

	return move;
}

//Probability = average of all neighbouring constraints. The algorithm returns the cell with the minimum probability.
std::tuple<int, int> CppSweeper_AI::stochasticMove_averageConstraint(CppSweeper* game)
{
	int remainingMines = game->mineCount - knownMines;
	double defaultProbability = ((double)remainingMines) / (double)((game->width * game->height - knownMines - game->uncoveredCells()));

	std::vector<VisibleCell*> boundary;

	//Set the cells default values and determine whether a cell part of the boundary
	for (int x = 0; x < game->width; x++)
		for (int y = 0; y < game->height; y++)
		{
			game->getCell(x, y)->timesConstrained = 0;
			if (game->getCell(x, y)->clicked)
				game->getCell(x, y)->mineProbability = 0.0f;
			else if (game->getCell(x, y)->knownMine)
				game->getCell(x, y)->mineProbability = 1.0f;
			else
				game->getCell(x, y)->mineProbability = 0.0f;

			bool isBdry = false;
			if ((game->getCell(x, y)->clicked) && (game->getCell(x, y)->mineProbability != 1.0f))
			{
				for (auto itr = game->getCell(x, y)->neighbouringCells.begin(); itr != game->getCell(x, y)->neighbouringCells.end(); itr++)
				{
					if ((!(*itr)->clicked) && (!(*itr)->flag))
						isBdry = true;
				}

				if (isBdry)
					boundary.push_back(game->getCell(x, y));
			}
		}

	//Iterate over the boundary, i.e. the constraining cells
	for (auto itr = boundary.begin(); itr != boundary.end(); itr++)
	{
		if ((*itr)->neighbouringCells.size() > 0)
		{
			//Determine the amount of covered (i.e. unclicked) neighbouring cells, i.e. those to which the constraint applies
			int coveredNeighbours = 0;
			int flaggedMines = 0;
			
			for (auto itr2 = (*itr)->neighbouringCells.begin(); itr2 != (*itr)->neighbouringCells.end(); itr2++)
			{
				if (!((*itr2)->clicked) && !((*itr2)->flag))
					coveredNeighbours++;
				if (((*itr2)->flag))
					flaggedMines++;
			}

			//Determine the probability implied by the constraint
			double newProbability = ((double)(*itr)->neighbouringMines - flaggedMines) / coveredNeighbours;

			//Add newProbability to all neighbouring constrained cells
			for (auto itr2 = (*itr)->neighbouringCells.begin(); itr2 != (*itr)->neighbouringCells.end(); itr2++)
			{
				if ((!(*itr2)->clicked) && !((*itr2)->knownMine))
				{
					(*itr2)->isConstrained = true;
					(*itr2)->timesConstrained++;
					(*itr2)->mineProbability += newProbability;
				}
			}

		}
	}

	//Determine the average over all constraints and find the minimum
	double minProbability = 1.0f;
	std::tuple<int, int> move = std::tuple<int, int>(-1, 1);
	for (int x = 0; x < game->width; x++)
		for (int y = 0; y < game->height; y++)
		{
			if ((!game->getCell(x, y)->clicked) && (game->getCell(x, y)->timesConstrained > 0))
				game->getCell(x, y)->mineProbability = game->getCell(x, y)->mineProbability / game->getCell(x, y)->timesConstrained;
			if ((!game->getCell(x, y)->clicked) && ((game->getCell(x, y)->timesConstrained == 0)) && (!game->getCell(x, y)->flag))
				game->getCell(x, y)->mineProbability = defaultProbability;
			if ((!game->getCell(x, y)->clicked) && (game->getCell(x, y)->mineProbability < minProbability))
			{
				minProbability = game->getCell(x, y)->mineProbability;
				move = std::tuple<int, int>(x, y);
			}

		}
	lastMove.probability = minProbability;

	return move;
}

//Check whether mine & simMine settings are consistent with the passed boundary
bool CppSweeper_AI::checkConstraints(std::vector<VisibleCell*>* boundary)
{
	for (auto itr = boundary->begin(); itr != boundary->end(); itr++)
	{
		//Count the neighbouring mines and simulated mines
		int mc = 0;
		for (auto itr2 = (*itr)->neighbouringCells.begin(); itr2 != (*itr)->neighbouringCells.end(); itr2++)
			if ((*itr2)->knownMine || (*itr2)->simMine)
				mc++;
		if (mc != (*itr)->neighbouringMines)
			return false;
	}
	return true;
}

/*Check if the exact constraints imposed by the cells in boundary are satisfied, i.e. that
the sum of set flags and simulated mines surrounding each cell equals cell->neighbouringMines*/
bool CppSweeper_AI::checkLocalUpperConstraints(VisibleCell* cellToSet)
{
	//itr iterates through the neighbouring cells that impose a constraint, i.e. the boundary of cellToSet
	for (auto itr = (cellToSet)->neighbouringCells.begin(); itr != (cellToSet)->neighbouringCells.end(); itr++)
	{
		if ((*itr)->clicked)
		{
			int mc = 0;
			//itr2 iterates through the neighbours of each boundary cell, counting known and simulated mines
			for (auto itr2 = (*itr)->neighbouringCells.begin(); itr2 != (*itr)->neighbouringCells.end(); itr2++)
				if ((*itr2)->knownMine || (*itr2)->simMine)
					mc++;
			if (mc > (*itr)->neighbouringMines)
				return false;
		}
	}
	return true;
}


//Performs a Depth-First-Search to label all cells with simFlag=true and all boundary cells that are connected to currentCell with the same label as currentCell
//Two cells are connected if they share a common boundary
void CppSweeper_AI::label(CppSweeper* game, std::vector<VisibleCell*>* cellsToSet, std::vector<VisibleCell*>::iterator currentCell, std::vector<VisibleCell*>* boundary, int prevLabel)
{
	(*currentCell)->connectedComponent = prevLabel;
	if ((*currentCell)->isConstrained)
	{
		for (auto itr = (*currentCell)->neighbouringCells.begin(); itr != (*currentCell)->neighbouringCells.end(); itr++)
		{
			if ((!(*itr)->isConstrained) && ((*itr)->clicked))
			{
				if ((*itr)->connectedComponent == -1)
					components[prevLabel].boundary.push_back((*itr));
				label(game, cellsToSet, itr, boundary, prevLabel);
			}
		}
	}
	else if ((*currentCell)->clicked)
	{
		for (auto itr = (*currentCell)->neighbouringCells.begin(); itr != (*currentCell)->neighbouringCells.end(); itr++)
		{
			if ((*itr)->isConstrained)
				if ((*itr)->connectedComponent == -1)
				{
					
					components[prevLabel].cellsToSet.push_back((*itr));
					label(game, cellsToSet, itr, boundary, prevLabel);
				}
					
		}
	}
}

//Labels all connected components in cellsToSet&boundary and returns the number of conn. comp.
int CppSweeper_AI::labelConnectedComponents(CppSweeper* game, std::vector<VisibleCell*>* cellsToSet, std::vector<VisibleCell*>* boundary)
{
	components.clear();
	int curLabel = 0;
	for (auto itr = cellsToSet->begin(); itr != cellsToSet->end(); itr++)
	{

		if ((*itr)->connectedComponent == -1)
		{
			ConnectedComponent currentComponent;
			currentComponent.label = curLabel;
			components.push_back(currentComponent);
			components[curLabel].cellsToSet.push_back(*itr);
			//Iteratively call the label-method with each cell in cellsToSet
			label(game, cellsToSet, itr, boundary, curLabel);
			curLabel++;
		}
	}
	return curLabel;
}

void CppSweeper_AI::setProbabilitiesFromSamples(CppSweeper* game, std::vector<VisibleCell*>* cellsToSet)
{
	int remainingMines = game->mineCount - knownMines;
	double defaultProbability = ((double)remainingMines) / (double)((game->width * game->height - knownMines - game->uncoveredCells()));

	//Updates each cells mine probability from the number of total valid configuration samples and the number of samples where the cell is a mie
	for (auto itr = cellsToSet->begin(); itr != cellsToSet->end(); itr++)
	{
		int component = (*itr)->connectedComponent;
		if (this->components[component].validSamples > 0)
		{
			//At least one valid sample for the cells connected component was found
			if (((double)(*itr)->validSimMines) != this->components[component].validSamples)
				(*itr)->mineProbability = ((double)(*itr)->validSimMines) / (double)this->components[component].validSamples;
			else
				(*itr)->mineProbability = ((double)(*itr)->validSimMines) / (double)this->components[component].validSamples - 0.001f;
		}
		else
			(*itr)->mineProbability = defaultProbability;
			
	}

	getMinimumProbabilityCell(game);

	//counts the total number of valid samples over all connected components
	validSamples_ = 0;
	for (auto itr = components.begin(); itr != components.end(); itr++)
		validSamples_ += itr->validSamples;
}

int CppSweeper_AI::nChoosek(int n, int k)
{
	if (k > n)
		return 0;
	if (k * 2 > n)
		k = n - k;
	if (k == 0)
		return 1;

	int result = n;
	for (int i = 2; i <= k; i++) {
		result *= (n - i + 1);
		result /= i;
	}
	return result;
}

void CppSweeper_AI::boundaryBacktracking(CppSweeper* game, std::vector<VisibleCell*>* boundary, std::vector<VisibleCell*>* cellsToSet, std::vector<VisibleCell*>::iterator cellToSet, int remainingMines)
{
	if ((remainingMines < 0) || (this->samplesCurrentCycle_ >= maxSamples_) || (cellToSet == cellsToSet->end()) || interrupt)
		return;

	if (this->samplesCurrentCycle_ % 100000 == 0)
		setProbabilitiesFromSamples(game, cellsToSet);

	(*cellToSet)->simMine = true;
	VisibleCell* cell = (*cellToSet);

	if ((cellToSet == cellsToSet->end() - 1) || (remainingMines == 1))
	{
		this->samplesCurrentCycle_++;
		int nk = 0;
		if ((remainingMines > 1) && (unconstrainedCells <= 25))
			nk = nChoosek(unconstrainedCells, remainingMines - 1) - 1;

		bool validConfiguration = checkConstraints(boundary);
		if ((validConfiguration))
		{
			this->components[(*cellToSet)->connectedComponent].validSamples += 1 + nk;
			for (auto itr = cellsToSet->begin(); itr != cellsToSet->end(); itr++)
				if ((*itr)->simMine) {
					(*itr)->validSimMines++;
					(*itr)->validSimMines += nk;
				}
					
		}

	}
	else if (checkLocalUpperConstraints(cell))
		boundaryBacktracking(game, boundary, cellsToSet, (cellToSet)+1, remainingMines - 1);


	(*cellToSet)->simMine = false;
	if ((cellToSet == cellsToSet->end() - 1) || (remainingMines == 0))
	{
		this->samplesCurrentCycle_++;

		int nk = 0;
		if ((remainingMines > 0) && (unconstrainedCells <= 25))
			nk = nChoosek(unconstrainedCells, remainingMines) - 1;

		bool validConfiguration = checkConstraints(boundary);
		if ((validConfiguration))
		{
			this->components[(*cellToSet)->connectedComponent].validSamples += 1 + nk;
			for (auto itr = cellsToSet->begin(); itr != cellsToSet->end(); itr++)
				if ((*itr)->simMine)
				{
					(*itr)->validSimMines++;
					(*itr)->validSimMines += nk;
				}
		}

	}
	else
		boundaryBacktracking(game, boundary, cellsToSet, cellToSet + 1, remainingMines);
}

//Probability = #(simulations where the cell is a mine) / #(total valid simulations), i.e. the algorithm samples the configuration space of constrained cells.
//The algorithm returns the cell with the minimum probability
//Important note: The algorithm assumes that flags have been set at cells that are known with certainty to be mines; It assumes that the remaining flags equate the remaining mines.
std::tuple<int, int> CppSweeper_AI::stochasticMove_BoundaryBacktracking(CppSweeper* game)
{
	int remainingMines = game->mineCount - knownMines;
	double defaultProbability = ((double)remainingMines) / (double)((game->width * game->height - knownMines - game->uncoveredCells()));

	//The constraint imposing cells
	std::vector<VisibleCell*> boundary; 
	//the constrained cells along the boundary, to be probed
	std::vector<VisibleCell*> cellsToSet; 

	//Set default values and build up the boundary and constrained cells
	for (int x = 0; x < game->width; x++)
		for (int y = 0; y < game->height; y++)
		{
			auto cell = game->getCell(x, y);
			cell->connectedComponent = -1;
			cell->simMine = 0;
			cell->validSimMines = 0;
			cell->isConstrained = false;
			bool isBdry = false;
			bool isConstrained = false;

			//Check whether a clicked cell is part of the boundary
			if ((cell->clicked))
			{
				for (auto itr = cell->neighbouringCells.begin(); itr != cell->neighbouringCells.end(); itr++)
					if ((!(*itr)->clicked) && (!(*itr)->flag))
					{
						isBdry = true;
						break;
					}		
				if (isBdry)
					boundary.push_back(cell);
			}
			//Check whether an unclicked cell (that is not known to be a mine already) is constrained
			else if (cell->mineProbability < 1.0f)
			{
				for (auto itr = cell->neighbouringCells.begin(); itr != cell->neighbouringCells.end(); itr++)
					if ((*itr)->clicked)
					{
						isConstrained = true;
						break;
					}
				if (isConstrained)
				{
					cellsToSet.push_back(cell);
					cell->isConstrained = true;
				}

				/*Set default mine probabilities*/
				//positive bias for corner cells
				if ((x == game->width - 1) && (y == game->height - 1) || ((x == game->width - 1) && (y == 0)) || ((x == 0) && (y == game->height - 1)) || ((x == 0) && (y == 0)))
					cell->mineProbability = defaultProbability - 0.001f;
				//less positive bias for edge cells
				else if ((x == game->width - 1) || (x == 0) || (y == game->height - 1) || (y == 0))
					cell->mineProbability = defaultProbability - 0.0001f;
				else
					cell->mineProbability = defaultProbability;
			}
				
		}

	int componentCount = labelConnectedComponents(game, &cellsToSet, &boundary);
	samplesCurrentCycle_ = 0;
	totalSamples_ = 0;
	_minProbX = -1;
	_minProbY = -1;

	//Sort connected components by size
	std::sort(components.begin(), components.end(),
		[](const ConnectedComponent& component1, const ConnectedComponent& component2) { return (component1.cellsToSet.size() < component2.cellsToSet.size()); });
	for (unsigned i = 0; i < components.size(); i++)
	{
		//For each connected component, perform backtracking search along the boundary to estimate mine probabilities
		unconstrainedCells = game->width * game->height - game->uncoveredCells() - components.at(i).cellsToSet.size() - (game->mineCount - game->flagCount());
		if (components.at(i).cellsToSet.size() > 0)
		{
			//rotate==true: Perform a backtracking search with each cell at the front exactly one time
			if (rotate)
			{
				this->maxSamples_ = maxSamples / components.at(i).cellsToSet.size();
				for (unsigned j = 0; j < components.at(i).cellsToSet.size() - 1; j++)
				{
					samplesCurrentCycle_ = 0;
					boundaryBacktracking(game, &components.at(i).boundary, &components.at(i).cellsToSet, components.at(i).cellsToSet.begin(), game->flagCount());
					for (int x = 0; x < game->width; x++)
						for (int y = 0; y < game->height; y++)
							game->getCell(x, y)->simMine = false;
					totalSamples_ += samplesCurrentCycle_;
					std::rotate(components.at(i).cellsToSet.begin(), components.at(i).cellsToSet.begin() + 1, components.at(i).cellsToSet.end());
					setProbabilitiesFromSamples(game, &components.at(i).cellsToSet);
				}
			}
			else
			{
				this->maxSamples_ = maxSamples;
				samplesCurrentCycle_ = 0;
				boundaryBacktracking(game, &components.at(i).boundary, &components.at(i).cellsToSet, components.at(i).cellsToSet.begin(), game->flagCount());
				for (int x = 0; x < game->width; x++)
					for (int y = 0; y < game->height; y++)
						game->getCell(x, y)->simMine = false;
				setProbabilitiesFromSamples(game, &components.at(i).cellsToSet);
			}
		}
	}

	setProbabilitiesFromSamples(game, &cellsToSet);

	std::tuple<int, int> move = getMinimumProbabilityCell(game);

	lastMove.probability = game->getCell(move)->mineProbability;
	_minProbX = -1;
	_minProbY = -1;
	return move;
}

//Probability = uniform. The algorithm returns a randomly selected cell (that is not known to be a mine).
std::tuple<int, int> CppSweeper_AI::stochasticMove_random(CppSweeper* game)
{
	if (game->gameWon() || game->gameLost())
		return std::tuple<int, int>(-1, -1);
	std::tuple<int, int> rndMove = std::tuple<int, int>(rand() % game->width, rand() % game->height);
	while ((game->getCell(rndMove)->clicked) || (game->getCell(rndMove)->knownMine))
		rndMove = std::tuple<int, int>(rand() % game->width, rand() % game->height);
	return rndMove;;
}

std::tuple<int, int> CppSweeper_AI::getMinimumProbabilityCell(CppSweeper* game)
{
	double minProbability = 1.0f;
	std::tuple<int, int> minProbabilityCell = std::tuple<int,int>(-1,-1);
	for (int x = 0; x < game->width; x++)
		for (int y = 0; y < game->height; y++)
		{
			if ((!game->getCell(x, y)->clicked) && (game->getCell(x, y)->mineProbability < minProbability))
			{
				minProbability = game->getCell(x, y)->mineProbability;
				minProbabilityCell = std::tuple<int, int>(x, y);
				_minProbX = x;
				_minProbY = y;
			}

		}
	return minProbabilityCell;
}

//Returns the (x,y) coordinate of a move the engine deems optimal.
//The engine first attempts to deduce an optimal move using the knowledge it generated via calls of the updateKnowledge-method.
//If no safe cell can be deduced via this method, a probabilistic estimate is performed to find a move - which one is governed by 
//the variable stochasticMethod
std::tuple<int, int> CppSweeper_AI::move(CppSweeper* game)
{
	interrupt = false;
	lastMove.moveNo = game->uncoveredCells() + 1;

	if ((*game).firstClick())
	{
		lastMove.moveType = MoveType::MOVE_FIRSTCLICK;
		moves++;
		lastMove.x = 0;
		lastMove.y = 0;
		return std::tuple<int, int>(0, 0);
	}
	else if ((*game).gameWon() || (*game).gameLost())
	{
		lastMove.moveType = MoveType::MOVE_NOMOVE;
		return std::tuple<int, int>(-1, -1);
	}
	else
	{
		for (auto itr = knowledge.begin(); itr != knowledge.end(); itr++)
			//Check if a cell is known to be safe
			if ((itr->neighbouringCells.size() > 0) && (itr->mineCount == 0))
			{
				lastMove.moveType = MoveType::MOVE_DETERMINISTIC;
				moves++;
				lastMove.x = itr->neighbouringCells.at(0)->x;
				lastMove.y = itr->neighbouringCells.at(0)->y;
				components.clear();
				validSamples_ = 0;
				//toggle flags and reset values that were used by the stochastic engine
				for (int x = 0; x < (*game).width; x++)
					for (int y = 0; y < (*game).height; y++)
					{
						auto cell = (*game).getCell(x, y);
						if (((cell->knownMine) && !cell->flag) || (!(cell->knownMine) && cell->flag))
							(*game).toggleFlag(x, y);
						cell->isConstrained = false;
						cell->connectedComponent = -1;
					}
				return std::tuple<int, int>(itr->neighbouringCells.at(0)->x, itr->neighbouringCells.at(0)->y);
			}

		lastMove.moveType = MoveType::MOVE_PROBABILISTIC;
		guesses++;
		std::tuple<int, int> rndMove;

		switch (stochasticMethod)
		{
		case StochasticMethod::METHOD_BACKTRACKING:
		{
			rndMove = stochasticMove_BoundaryBacktracking(game);
			if (rndMove == std::tuple<int, int>(-1, -1))
				rndMove = stochasticMove_averageConstraint(game);
			break;
		}
		case StochasticMethod::METHOD_AVGCONSTRAINT:
		{
			rndMove = stochasticMove_averageConstraint(game);
			break;
		}
		case StochasticMethod::METHOD_SINGLECONSTRAINT:
		{
			rndMove = stochasticMove_singleConstraint(game);
			break;
		}
		default:
			rndMove = stochasticMove_random(game);
		}
		toggleFlags(game);

		lastMove.x = std::get<0>(rndMove);
		lastMove.y = std::get<1>(rndMove);
		return rndMove;
	}
}

void CppSweeper_AI::toggleFlags(CppSweeper* game)
{
	for (int x = 0; x < (*game).width; x++)
		for (int y = 0; y < (*game).height; y++)
		{
			auto cell = (*game).getCell(x, y);
			if (((cell->knownMine) && !cell->flag) || (!(cell->knownMine) && cell->flag))
				(*game).toggleFlag(x, y);
		}
}

CppSweeper_AI::CppSweeper_AI()
{
	
}

void CppSweeper_AI::reset()
{
	knownMines = 0;
	knowledge.clear();
}
